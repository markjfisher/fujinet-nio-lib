#include "fn_internal.h"

static uint8_t fn_calc_checksum_skip_offset(const uint8_t *data,
                                            uint16_t len,
                                            uint16_t skip_offset)
{
    uint16_t chk;
    uint16_t i;

    chk = 0;
    for (i = 0; i < len; ++i) {
        if (i == skip_offset) {
            continue;
        }
        chk += data[i];
        chk = ((chk >> 8) + (chk & 0xFF)) & 0xFFFF;
    }

    return (uint8_t)(chk & 0xFF);
}

uint8_t fn_parse_response_header(const uint8_t *response,
                                 uint16_t resp_len,
                                 uint8_t *status,
                                 uint16_t *data_offset,
                                 uint16_t *data_len)
{
    uint16_t pkt_len;
    uint8_t checksum;

    if (resp_len < FN_HEADER_SIZE) {
        return FN_ERR_INVALID;
    }

    pkt_len = FN_READ_LE16(response, 2);
    if (pkt_len != resp_len) {
        return FN_ERR_INVALID;
    }

    checksum = fn_calc_checksum_skip_offset(response, resp_len, 4);
    if (checksum != response[4]) {
        return FN_ERR_IO;
    }

    if (response[5] != 0) {
        return FN_ERR_UNSUPPORTED;
    }

    *status = FN_OK;
    *data_offset = FN_HEADER_SIZE;
    *data_len = resp_len - FN_HEADER_SIZE;

    return FN_OK;
}
