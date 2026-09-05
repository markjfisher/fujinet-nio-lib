#include "fn_internal.h"
#include "fn_platform.h"

uint8_t fn_clock_exchange(uint8_t command,
                          const uint8_t *payload,
                          uint16_t payload_len,
                          uint8_t *status,
                          uint16_t *data_offset,
                          uint16_t *data_len)
{
    uint16_t req_len;
    uint8_t result;

    req_len = fn_build_header(_fn_req_buf, FN_DEVICE_CLOCK, command, FN_HEADER_SIZE + payload_len);
    if (req_len == 0) {
        return FN_ERR_INTERNAL;
    }

    while (payload_len > 0) {
        _fn_req_buf[req_len++] = *payload++;
        --payload_len;
    }

    _fn_req_buf[FN_CHECKSUM_OFFSET] = fn_calc_packet_checksum(_fn_req_buf, req_len);

    _fn_transport_ctx.request = _fn_req_buf;
    _fn_transport_ctx.req_len = req_len;
    _fn_transport_ctx.response = _fn_resp_buf;
    _fn_transport_ctx.resp_max = FN_MAX_PACKET_SIZE;

    result = fn_transport_exchange();
    if (result != FN_OK) {
        return result;
    }

    _fn_parse_ctx.response = _fn_resp_buf;
    _fn_parse_ctx.resp_len = _fn_transport_ctx.resp_len;
    result = fn_parse_response_header();
    if (result != FN_OK) {
        return result;
    }

    *status = _fn_parse_ctx.status;
    *data_offset = _fn_parse_ctx.data_offset;
    *data_len = _fn_parse_ctx.data_len;
    return FN_OK;
}

uint8_t fn_clock_response_byte(uint16_t offset)
{
    return _fn_resp_buf[offset];
}
