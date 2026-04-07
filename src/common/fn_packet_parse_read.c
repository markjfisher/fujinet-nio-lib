#include <string.h>

#include "fn_internal.h"

uint8_t fn_parse_read_response(const uint8_t *response,
                               uint16_t resp_len,
                               fn_handle_t *handle,
                               uint32_t *offset_echo,
                               uint8_t *flags,
                               uint8_t *data,
                               uint16_t data_max,
                               uint16_t *data_len)
{
    uint8_t status;
    uint16_t data_offset;
    uint16_t payload_len;
    uint8_t result;
    uint16_t actual_data_len;
    uint16_t copy_len;

    result = fn_parse_response_header(response, resp_len, &status, &data_offset, &payload_len);
    if (result != FN_OK) {
        return result;
    }
    if (status != FN_OK) {
        return status;
    }
    if (payload_len < 12) {
        return FN_ERR_INVALID;
    }

    *flags = response[data_offset + 1];
    *handle = FN_READ_LE16(response, data_offset + 4);
    *offset_echo = FN_READ_LE32(response, data_offset + 6);
    actual_data_len = FN_READ_LE16(response, data_offset + 10);

    copy_len = actual_data_len;
    if (copy_len > data_max) {
        copy_len = data_max;
    }

    if (copy_len > 0 && data != NULL) {
        memcpy(data, response + data_offset + 12, copy_len);
    }

    *data_len = actual_data_len;
    return FN_OK;
}
