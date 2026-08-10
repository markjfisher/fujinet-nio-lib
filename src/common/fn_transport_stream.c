/**
 * @file fn_transport_stream.c
 * @brief Adapter from the library transport state to the shared stream session.
 *
 * Platform channel implementations provide byte I/O. The shared session owns
 * SLIP framing, flushing, timeouts, and the one-outstanding-request rule.
 */

#include "fujinet-nio.h"
#include "fn_channel.h"
#include "fn_internal.h"
#include "fn_platform.h"
#include "fn_session.h"
#include "fn_protocol.h"

#ifndef FN_TRANSPORT_WIRE_BUF_SIZE
#define FN_TRANSPORT_WIRE_BUF_SIZE ((FN_MAX_PACKET_SIZE * 2) + 2)
#endif

static uint8_t _stream_wire_buf[FN_TRANSPORT_WIRE_BUF_SIZE];
static fn_stream_session_t _stream_session;
static uint8_t _stream_session_initialized;

static uint8_t channel_open(void *context)
{
    (void)context;
    return fn_channel_init();
}

static void channel_close(void *context)
{
    (void)context;
    fn_channel_close();
}

static uint8_t channel_write_byte(void *context, uint8_t value,
                                  uint16_t timeout_ms)
{
    (void)context;
    return fn_channel_write_byte(value, timeout_ms);
}

static uint8_t channel_read_byte(void *context, uint8_t *value,
                                 uint16_t timeout_ms)
{
    (void)context;
    return fn_channel_read_byte(value, timeout_ms);
}

static void channel_flush(void *context)
{
    (void)context;
    fn_channel_drain_rx();
}

static const fn_stream_channel_ops_t _stream_channel_ops = {
    channel_open,
    channel_close,
    channel_write_byte,
    channel_read_byte,
    channel_flush
};

uint8_t fn_transport_init(void)
{
    uint8_t result;

    if (!_stream_session_initialized) {
        result = fn_stream_session_init(&_stream_session, &_stream_channel_ops,
                                        0, _stream_wire_buf,
                                        sizeof(_stream_wire_buf));
        if (result != FN_OK) return result;
        _stream_session_initialized = 1;
    }
    return fn_stream_session_open(&_stream_session);
}

uint8_t fn_transport_ready(void)
{
    return _stream_session.opened;
}

uint8_t fn_transport_exchange(void)
{
    return fn_transport_exchange_buffers(_fn_transport_ctx.request,
                                         _fn_transport_ctx.req_len,
                                         _fn_transport_ctx.response,
                                         _fn_transport_ctx.resp_max,
                                         &_fn_transport_ctx.resp_len);
}

uint8_t fn_transport_exchange_buffers(const uint8_t *request,
                                      uint16_t request_length,
                                      uint8_t *response,
                                      uint16_t response_capacity,
                                      uint16_t *response_length)
{
    return fn_stream_session_request(&_stream_session, request,
                                     request_length, response,
                                     response_capacity, response_length,
                                     FN_TRANSPORT_TIMEOUT);
}

void fn_transport_close(void)
{
    fn_stream_session_close(&_stream_session);
}
