#include "fn_internal.h"

uint8_t fn_parse_info_response(const uint8_t *response,
                               uint16_t resp_len,
                               fn_handle_t *handle,
                               uint16_t *http_status,
                               uint32_t *content_length,
                               uint8_t *flags)
{
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t result;

    result = fn_parse_response_header(response, resp_len, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    if (status != FN_OK) {
        return status;
    }

    if (data_len < 16) {
        *flags = 0;
        *http_status = 0;
        *content_length = 0;
        return FN_OK;
    }

    *flags = response[data_offset + 1];
    *handle = FN_READ_LE16(response, data_offset + 4);
    *http_status = FN_READ_LE16(response, data_offset + 6);
    *content_length = FN_READ_LE32(response, data_offset + 8);

    return FN_OK;
}
