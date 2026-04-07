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
    uint16_t resp_len;
    uint8_t result;

    req_len = fn_build_header(_fn_req_buf, FN_DEVICE_CLOCK, command, FN_HEADER_SIZE + payload_len);
    if (req_len == 0) {
        return FN_ERR_INTERNAL;
    }

    while (payload_len > 0) {
        _fn_req_buf[req_len++] = *payload++;
        --payload_len;
    }

    _fn_req_buf[4] = fn_calc_checksum(_fn_req_buf, req_len);

    result = fn_transport_exchange(_fn_req_buf, req_len, _fn_resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }

    return fn_parse_response_header(_fn_resp_buf, resp_len, status, data_offset, data_len);
}
