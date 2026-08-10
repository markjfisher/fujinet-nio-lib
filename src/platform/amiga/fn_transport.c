/*
 * fn_transport.c - AmigaOS Transport Implementation
 *
 * Uses serial.device to communicate with fujinet-nio over RS-232.
 * Implements SLIP framing for the FujiBus protocol.
 *
 * Requires: exec.library, serial.device
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * Serial port configuration:
 *   Baud rate: 19200 (default) — set FN_AMIGA_BAUD at compile time to override
 *   Format: 8N1
 *   Flow control: none (XON/XOFF disabled — required for binary SLIP data)
 *
 * See: contracts/rs232-hardware.md, contracts/fujibus-protocol.md
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <devices/serial.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>
#ifndef FN_AMIGA_EXPLICIT_LIFECYCLE
#include <stdlib.h>
#endif

#include "fujinet-nio.h"
#include "fn_platform.h"
#include "fn_internal.h"
#include "fn_session.h"

#ifndef FN_AMIGA_BAUD
#define FN_AMIGA_BAUD 19200
#endif

#ifndef FN_TRANSPORT_WIRE_BUF_SIZE
#define FN_TRANSPORT_WIRE_BUF_SIZE ((FN_MAX_PACKET_SIZE * 2) + 2)
#endif

/* Module state */
static struct MsgPort   *_serial_port = NULL;
static struct IOExtSer  *_serial_req  = NULL;
static BYTE              _device_open = 0;
static const UBYTE        _serial_device_name[] = "serial.device";

static uint8_t _wire_buf[FN_TRANSPORT_WIRE_BUF_SIZE];
static fn_stream_session_t _session;
static uint8_t _session_initialized;

void fn_transport_close(void);

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

static uint8_t serial_write(const uint8_t *buf, uint16_t len)
{
    _serial_req->IOSer.io_Command  = CMD_WRITE;
    _serial_req->IOSer.io_Data     = (APTR)buf;
    _serial_req->IOSer.io_Length   = len;
    if (DoIO((struct IORequest *)_serial_req) != 0) {
        return FN_ERR_IO;
    }
    return FN_OK;
}

static uint8_t serial_read_byte(uint8_t *byte_out)
{
    _serial_req->IOSer.io_Command  = CMD_READ;
    _serial_req->IOSer.io_Data     = (APTR)byte_out;
    _serial_req->IOSer.io_Length   = 1;
    if (DoIO((struct IORequest *)_serial_req) != 0) {
        return FN_ERR_IO;
    }
    return FN_OK;
}

/* The Amiga serial device is opened by fn_transport_init(). The shared
 * session therefore owns framing and request/response bounds, while these
 * callbacks provide only the byte-oriented channel contract. */
static uint8_t session_open(void *context)
{
    (void)context;
    return FN_OK;
}

static void session_close(void *context)
{
    (void)context;
}

static uint8_t session_write_byte(void *context, uint8_t value,
                                  uint16_t timeout_ms)
{
    (void)context;
    (void)timeout_ms;
    return serial_write(&value, 1);
}

static uint8_t session_read_byte(void *context, uint8_t *value,
                                 uint16_t timeout_ms)
{
    (void)context;
    (void)timeout_ms;
    return serial_read_byte(value) == FN_OK ? 1 : 0;
}

static void session_flush(void *context)
{
    (void)context;
}

static const fn_stream_channel_ops_t _session_ops = {
    session_open,
    session_close,
    session_write_byte,
    session_read_byte,
    session_flush
};

/* -------------------------------------------------------------------------
 * Public platform interface
 * ---------------------------------------------------------------------- */

uint8_t fn_transport_init(void)
{
    if (_device_open) {
        return FN_OK;
    }

    /* CreatePort/DeletePort and CreateExtIO/DeleteExtIO are amiga.lib functions
     * compatible with all Kickstart versions including 1.3 (exec V34).
     * CreateMsgPort/CreateIORequest require exec V36 (Kickstart 2.0+). */
    _serial_port = CreatePort(NULL, 0);
    if (!_serial_port) {
        return FN_ERR_IO;
    }

    _serial_req = (struct IOExtSer *)CreateExtIO(_serial_port,
                                                 sizeof(struct IOExtSer));
    if (!_serial_req) {
        DeletePort(_serial_port);
        _serial_port = NULL;
        return FN_ERR_IO;
    }

    if (OpenDevice(_serial_device_name, 0,
                   (struct IORequest *)_serial_req, 0) != 0) {
        DeleteExtIO((struct IORequest *)_serial_req);
        DeletePort(_serial_port);
        _serial_req  = NULL;
        _serial_port = NULL;
        return FN_ERR_NOT_FOUND;
    }

    /* Configure serial parameters */
    _serial_req->io_Baud      = FN_AMIGA_BAUD;
    _serial_req->io_ReadLen   = 8;
    _serial_req->io_WriteLen  = 8;
    _serial_req->io_StopBits  = 1;
    /* SERF_XDISABLED: disable XON/XOFF — essential for binary SLIP data */
    _serial_req->io_SerFlags  = SERF_XDISABLED;

    _serial_req->IOSer.io_Command = SDCMD_SETPARAMS;
    if (DoIO((struct IORequest *)_serial_req) != 0) {
        CloseDevice((struct IORequest *)_serial_req);
        DeleteExtIO((struct IORequest *)_serial_req);
        DeletePort(_serial_port);
        _serial_req  = NULL;
        _serial_port = NULL;
        return FN_ERR_IO;
    }

    if (fn_stream_session_init(&_session, &_session_ops, 0, _wire_buf,
                               sizeof(_wire_buf)) != FN_OK ||
        fn_stream_session_open(&_session) != FN_OK) {
        CloseDevice((struct IORequest *)_serial_req);
        DeleteExtIO((struct IORequest *)_serial_req);
        DeletePort(_serial_port);
        _serial_req  = NULL;
        _serial_port = NULL;
        return FN_ERR_IO;
    }
    _session_initialized = 1;
    _device_open = 1;
    /* CLI applications use process-exit cleanup. Resident drivers are built
     * with FN_AMIGA_EXPLICIT_LIFECYCLE because they have no process exit and
     * must release the transport through their device lifecycle instead. */
#ifndef FN_AMIGA_EXPLICIT_LIFECYCLE
    atexit(fn_transport_close);
#endif
    return FN_OK;
}

uint8_t fn_transport_ready(void)
{
    return _device_open ? 1 : 0;
}

uint8_t fn_transport_exchange(void)
{
    if (!_device_open || !_session_initialized) return FN_ERR_NOT_FOUND;
    return fn_stream_session_request(&_session,
                                     _fn_transport_ctx.request,
                                     _fn_transport_ctx.req_len,
                                     _fn_transport_ctx.response,
                                     _fn_transport_ctx.resp_max,
                                     &_fn_transport_ctx.resp_len,
                                     FN_TRANSPORT_TIMEOUT);
}

void fn_transport_close(void)
{
    if (!_device_open) return;
    fn_stream_session_close(&_session);
    _session_initialized = 0;
    CloseDevice((struct IORequest *)_serial_req);
    DeleteExtIO((struct IORequest *)_serial_req);
    DeletePort(_serial_port);
    _serial_req  = NULL;
    _serial_port = NULL;
    _device_open = 0;
}

const char *fn_platform_name(void)
{
    return "amiga";
}
