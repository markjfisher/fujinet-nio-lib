#include "fn_internal.h"

uint8_t fn_clock_get_timezone(char *tz)
{
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t result;
    uint8_t tz_len;
    uint16_t i;

    if (tz == NULL) {
        return FN_ERR_INVALID;
    }

    result = fn_clock_exchange(FN_CMD_CLOCK_GET_TZ, NULL, 0, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    if (status != FN_OK) {
        return status;
    }
    if (data_len < 2) {
        return FN_ERR_INVALID;
    }
    if (fn_clock_response_byte(data_offset) != FN_CLOCK_VERSION) {
        return FN_ERR_UNSUPPORTED;
    }

    tz_len = fn_clock_response_byte(data_offset + 1);
    for (i = 0; i < tz_len && i < FN_MAX_TIMEZONE_LEN - 1; ++i) {
        tz[i] = (char)fn_clock_response_byte(data_offset + 2 + i);
    }
    tz[i] = '\0';

    return FN_OK;
}
