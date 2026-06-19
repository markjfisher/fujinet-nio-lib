/**
 * @file fn_transport_stream.c
 * @brief Common SLIP-over-byte-stream transport.
 *
 * This layer owns FujiBus frame exchange over a byte-oriented channel. Platform
 * channel implementations provide only byte read/write and readiness functions.
 */

#include "fujinet-nio.h"
#include "fn_channel.h"
#include "fn_internal.h"
#include "fn_platform.h"
#include "fn_protocol.h"

#ifndef FN_TRANSPORT_WIRE_BUF_SIZE
#define FN_TRANSPORT_WIRE_BUF_SIZE ((FN_MAX_PACKET_SIZE * 2) + 2)
#endif

#ifndef FN_TRANSPORT_WRITE_TIMEOUT
#define FN_TRANSPORT_WRITE_TIMEOUT 1000
#endif

#ifndef FN_TRANSPORT_POLL_TIMEOUT
#define FN_TRANSPORT_POLL_TIMEOUT 10
#endif

static uint8_t _wire_buf[FN_TRANSPORT_WIRE_BUF_SIZE];

static uint8_t stream_write_frame(const uint8_t *request, uint16_t req_len)
{
    uint16_t i;
    uint8_t result;
    uint8_t byte;

    result = fn_channel_write_byte(SLIP_END, FN_TRANSPORT_WRITE_TIMEOUT);
    if (result != FN_OK) {
        return result;
    }

    for (i = 0; i < req_len; ++i) {
        byte = request[i];

        if (byte == SLIP_END) {
            result = fn_channel_write_byte(SLIP_ESCAPE, FN_TRANSPORT_WRITE_TIMEOUT);
            if (result != FN_OK) {
                return result;
            }
            result = fn_channel_write_byte(SLIP_ESC_END, FN_TRANSPORT_WRITE_TIMEOUT);
        } else if (byte == SLIP_ESCAPE) {
            result = fn_channel_write_byte(SLIP_ESCAPE, FN_TRANSPORT_WRITE_TIMEOUT);
            if (result != FN_OK) {
                return result;
            }
            result = fn_channel_write_byte(SLIP_ESC_ESC, FN_TRANSPORT_WRITE_TIMEOUT);
        } else {
            result = fn_channel_write_byte(byte, FN_TRANSPORT_WRITE_TIMEOUT);
        }

        if (result != FN_OK) {
            return result;
        }
    }

    return fn_channel_write_byte(SLIP_END, FN_TRANSPORT_WRITE_TIMEOUT);
}

static uint16_t stream_read_frame(uint8_t *buffer, uint16_t buffer_len, uint16_t timeout_ms)
{
    uint16_t waited_ms;
    uint16_t len;
    uint8_t byte;
    uint8_t saw_start;

    waited_ms = 0;
    len = 0;
    saw_start = 0;

    while (waited_ms < timeout_ms) {
        if (!fn_channel_read_byte(&byte, FN_TRANSPORT_POLL_TIMEOUT)) {
            waited_ms += FN_TRANSPORT_POLL_TIMEOUT;
            continue;
        }

        if (!saw_start) {
            if (byte != SLIP_END) {
                continue;
            }
            saw_start = 1;
        }

        if (len >= buffer_len) {
            return 0;
        }

        buffer[len++] = byte;

        if (len >= 2 && byte == SLIP_END) {
            return len;
        }
    }

    return 0;
}

uint8_t fn_transport_init(void)
{
    return fn_channel_init();
}

uint8_t fn_transport_ready(void)
{
    return fn_channel_ready();
}

uint8_t fn_transport_exchange(void)
{
    const uint8_t *request;
    uint8_t *response;
    uint16_t req_len;
    uint16_t resp_max;
    uint16_t raw_len;
    uint16_t decoded_len;
    uint8_t result;

    request = _fn_transport_ctx.request;
    response = _fn_transport_ctx.response;
    req_len = _fn_transport_ctx.req_len;
    resp_max = _fn_transport_ctx.resp_max;

    if (!fn_channel_ready()) {
        return FN_ERR_NOT_READY;
    }

    if (request == 0 || req_len == 0 || response == 0 || resp_max == 0) {
        return FN_ERR_INVALID;
    }

    fn_channel_drain_rx();

    result = stream_write_frame(request, req_len);
    if (result != FN_OK) {
        return result;
    }

    raw_len = stream_read_frame(_wire_buf, sizeof(_wire_buf), FN_TRANSPORT_TIMEOUT);
    if (raw_len == 0) {
        return FN_ERR_TIMEOUT;
    }

    decoded_len = fn_slip_decode(_wire_buf, raw_len, response);
    if (decoded_len == 0) {
        return FN_ERR_IO;
    }

    if (decoded_len > resp_max) {
        return FN_ERR_IO;
    }

    _fn_transport_ctx.resp_len = decoded_len;
    return FN_OK;
}

void fn_transport_close(void)
{
    fn_channel_close();
}
