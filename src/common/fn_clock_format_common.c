#include "fn_internal.h"

uint8_t fn_clock_copy_format_response(uint8_t *time_data,
                                      uint16_t data_offset,
                                      uint16_t data_len)
{
    uint16_t i;

    if (data_len < 2) {
        return FN_ERR_INVALID;
    }
    if (fn_clock_response_byte(data_offset) != FN_CLOCK_VERSION) {
        return FN_ERR_UNSUPPORTED;
    }

    for (i = 0; i < data_len - 2; ++i) {
        time_data[i] = fn_clock_response_byte(data_offset + 2 + i);
    }

    return FN_OK;
}
