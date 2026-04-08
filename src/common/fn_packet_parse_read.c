#include <string.h>

#include "fn_internal.h"

uint8_t fn_parse_read_response(void)
{
    uint8_t result;
    uint16_t actual_data_len;
    uint16_t copy_len;
    const uint8_t *response = _fn_parse_ctx.response;

    result = fn_parse_response_header();
    if (result != FN_OK) {
        return result;
    }
    if (_fn_parse_ctx.status != FN_OK) {
        return _fn_parse_ctx.status;
    }
    if (_fn_parse_ctx.data_len < 12) {
        return FN_ERR_INVALID;
    }

    _fn_parse_ctx.flags = response[_fn_parse_ctx.data_offset + 1];
    _fn_parse_ctx.handle = FN_READ_LE16(response, _fn_parse_ctx.data_offset + 4);
    _fn_parse_ctx.offset_echo = FN_READ_LE32(response, _fn_parse_ctx.data_offset + 6);
    actual_data_len = FN_READ_LE16(response, _fn_parse_ctx.data_offset + 10);

    copy_len = actual_data_len;
    if (copy_len > _fn_parse_ctx.data_max) {
        copy_len = _fn_parse_ctx.data_max;
    }

    if (copy_len > 0 && _fn_parse_ctx.data != NULL) {
        memcpy(_fn_parse_ctx.data, response + _fn_parse_ctx.data_offset + 12, copy_len);
    }

    _fn_parse_ctx.data_len = actual_data_len;
    return FN_OK;
}
