#include "fn_internal.h"

static const uint8_t fn_field_size_table[8] = { 0, 1, 1, 1, 1, 2, 2, 4 };
static const uint8_t fn_field_count_table[8] = { 0, 1, 2, 3, 4, 1, 2, 1 };

uint8_t fn_parse_response_header(void)
{
    uint16_t pkt_len;
    uint16_t offset;
    uint8_t checksum;
    uint8_t descr;
    uint8_t field_desc;
    uint8_t field_size;
    uint8_t field_count;
    uint8_t i;
    const uint8_t *response = _fn_parse_ctx.response;
    uint16_t resp_len = _fn_parse_ctx.resp_len;

    if (resp_len < FN_HEADER_SIZE) {
        return FN_ERR_INVALID;
    }

    pkt_len = FN_READ_LE16(response, 2);
    if (pkt_len != resp_len) {
        return FN_ERR_INVALID;
    }

    checksum = fn_calc_packet_checksum(response, resp_len);
    if (checksum != response[FN_CHECKSUM_OFFSET]) {
        return FN_ERR_IO;
    }

    descr = response[5];
    offset = FN_HEADER_SIZE;
    _fn_parse_ctx.status = FN_OK;

    if (descr == 0) {
        _fn_parse_ctx.data_offset = offset;
        _fn_parse_ctx.data_len = resp_len - offset;
        return FN_OK;
    }

    while (descr & 0x80U) {
        if (offset >= resp_len) {
            return FN_ERR_INVALID;
        }
        descr = response[offset++];
    }

    field_desc = descr & 0x07U;
    field_size = fn_field_size_table[field_desc];
    field_count = fn_field_count_table[field_desc];

    if (field_count > 0) {
        if ((uint16_t)(offset + field_size) > resp_len) {
            return FN_ERR_INVALID;
        }

        _fn_parse_ctx.status = 0;
        for (i = 0; i < field_size; ++i) {
            _fn_parse_ctx.status |= (uint8_t)(response[offset + i] << (8 * i));
        }

        offset += field_size;

        for (i = 1; i < field_count; ++i) {
            if ((uint16_t)(offset + field_size) > resp_len) {
                return FN_ERR_INVALID;
            }
            offset += field_size;
        }
    }

    _fn_parse_ctx.data_offset = offset;
    _fn_parse_ctx.data_len = resp_len - offset;

    return FN_OK;
}
