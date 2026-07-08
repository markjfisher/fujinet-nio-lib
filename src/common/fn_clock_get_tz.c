#include "fn_internal.h"

uint8_t fn_clock_get_tz(uint8_t *time_data, const char *tz, FnTimeFormat format)
{
    uint8_t payload[FN_MAX_TIMEZONE_LEN + 3];
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t result;
    uint8_t tz_len;
    uint16_t i;

    if (time_data == NULL || tz == NULL) {
        return FN_ERR_INVALID;
    }

    tz_len = 0;
    while (tz[tz_len] != '\0' && tz_len < FN_MAX_TIMEZONE_LEN) {
        ++tz_len;
    }

    payload[0] = FN_CLOCK_VERSION;
    payload[1] = (uint8_t)format;
    payload[2] = tz_len;
    for (i = 0; i < tz_len; ++i) {
        payload[i + 3] = (uint8_t)tz[i];
    }

    result = fn_clock_exchange(FN_CMD_CLOCK_GET_FORMAT, payload, (uint16_t)(tz_len + 3), &status, &data_offset, &data_len);
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

    for (i = 0; i < data_len - 2; ++i) {
        time_data[i] = fn_clock_response_byte(data_offset + 2 + i);
    }

    return FN_OK;
}
