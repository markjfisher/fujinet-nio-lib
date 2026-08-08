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

#include "fujinet-nio.h"
#include "fn_platform.h"
#include "fn_protocol.h"
#include "fn_internal.h"

#ifndef FN_AMIGA_BAUD
#define FN_AMIGA_BAUD 19200
#endif

/* Read timeout: number of 100ms polling loops before giving up */
#ifndef FN_AMIGA_READ_TIMEOUT_LOOPS
#define FN_AMIGA_READ_TIMEOUT_LOOPS 50   /* 50 * 100ms = 5 seconds */
#endif

#ifndef FN_TRANSPORT_WIRE_BUF_SIZE
#define FN_TRANSPORT_WIRE_BUF_SIZE ((FN_MAX_PACKET_SIZE * 2) + 2)
#endif

/* Module state */
static struct MsgPort   *_serial_port = NULL;
static struct IOExtSer  *_serial_req  = NULL;
static BYTE              _device_open = 0;

static UBYTE _wire_buf[FN_TRANSPORT_WIRE_BUF_SIZE];

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

static uint8_t slip_write_frame(const uint8_t *request, uint16_t req_len)
{
    uint16_t i;
    uint8_t byte;
    uint8_t esc[2];
    uint8_t end_byte = SLIP_END;

    /* Leading END */
    if (serial_write(&end_byte, 1) != FN_OK) return FN_ERR_IO;

    for (i = 0; i < req_len; i++) {
        byte = request[i];
        if (byte == SLIP_END) {
            esc[0] = SLIP_ESCAPE;
            esc[1] = SLIP_ESC_END;
            if (serial_write(esc, 2) != FN_OK) return FN_ERR_IO;
        } else if (byte == SLIP_ESCAPE) {
            esc[0] = SLIP_ESCAPE;
            esc[1] = SLIP_ESC_ESC;
            if (serial_write(esc, 2) != FN_OK) return FN_ERR_IO;
        } else {
            if (serial_write(&byte, 1) != FN_OK) return FN_ERR_IO;
        }
    }

    /* Trailing END */
    if (serial_write(&end_byte, 1) != FN_OK) return FN_ERR_IO;
    return FN_OK;
}

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

    if (OpenDevice("serial.device", 0,
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

    _device_open = 1;
    return FN_OK;
}

uint8_t fn_transport_ready(void)
{
    return _device_open ? 1 : 0;
}

uint8_t fn_transport_exchange(void)
{
    const uint8_t *request  = _fn_transport_ctx.request;
    uint16_t       req_len  = _fn_transport_ctx.req_len;
    uint8_t       *response = _fn_transport_ctx.response;
    uint16_t       resp_max = _fn_transport_ctx.resp_max;
    uint16_t       raw_len  = 0;
    uint8_t        byte;
    int            loops;
    uint8_t        ret;

    if (!_device_open) return FN_ERR_NOT_FOUND;
    if (!request || req_len == 0 || !response) return FN_ERR_INVALID;

    if (resp_max > FN_TRANSPORT_WIRE_BUF_SIZE) {
        resp_max = FN_TRANSPORT_WIRE_BUF_SIZE;
    }

    /* Send the SLIP-encoded request */
    ret = slip_write_frame(request, req_len);
    if (ret != FN_OK) return ret;

    /* Read SLIP-encoded response one byte at a time.
     * We stop when we see the closing 0xC0 after at least one non-END byte,
     * or when we time out. */
    loops = 0;
    while (raw_len < resp_max) {
        if (serial_read_byte(&byte) != FN_OK) {
            /* Treat a read error as a poll timeout — retry up to limit */
            loops++;
            if (loops >= FN_AMIGA_READ_TIMEOUT_LOOPS) {
                return FN_ERR_TIMEOUT;
            }
            continue;
        }
        loops = 0;  /* reset timeout on successful byte */

        _wire_buf[raw_len++] = byte;

        /* Detect end of SLIP frame: leading C0 ... trailing C0 */
        if (raw_len >= 2 &&
            _wire_buf[0] == SLIP_END &&
            byte == SLIP_END) {
            break;
        }
    }

    /* SLIP-decode the response */
    _fn_transport_ctx.resp_len = fn_slip_decode(_wire_buf, raw_len, response);
    if (_fn_transport_ctx.resp_len == 0) {
        return FN_ERR_IO;
    }

    return FN_OK;
}

void fn_transport_close(void)
{
    if (!_device_open) return;
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
