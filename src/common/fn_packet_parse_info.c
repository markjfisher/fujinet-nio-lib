#include "fn_internal.h"

uint8_t fn_parse_info_response(void)
{
    uint8_t result;
    const uint8_t *response = _fn_parse_ctx.response;

    result = fn_parse_response_header();
    if (result != FN_OK) {
        return result;
    }
    if (_fn_parse_ctx.status != FN_OK) {
        return _fn_parse_ctx.status;
    }

    if (_fn_parse_ctx.data_len < 16) {
        _fn_parse_ctx.flags = 0;
        _fn_parse_ctx.http_status = 0;
        _fn_parse_ctx.content_length = 0;
        return FN_OK;
    }

    _fn_parse_ctx.flags = response[_fn_parse_ctx.data_offset + 1];
    _fn_parse_ctx.handle = FN_READ_LE16(response, _fn_parse_ctx.data_offset + 4);
    _fn_parse_ctx.http_status = FN_READ_LE16(response, _fn_parse_ctx.data_offset + 6);
    _fn_parse_ctx.content_length = FN_READ_LE32(response, _fn_parse_ctx.data_offset + 8);

    return FN_OK;
}
