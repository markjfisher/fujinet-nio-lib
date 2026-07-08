#include "fn_internal.h"

uint8_t fn_clock_set_timezone_common(uint8_t command, const char *tz)
{
    uint8_t payload[FN_MAX_TIMEZONE_LEN + 2];
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t result;
    uint8_t tz_len;
    uint16_t i;

    if (tz == NULL) {
        return FN_ERR_INVALID;
    }

    tz_len = 0;
    while (tz[tz_len] != '\0' && tz_len < FN_MAX_TIMEZONE_LEN) {
        ++tz_len;
    }

    payload[0] = FN_CLOCK_VERSION;
    payload[1] = tz_len;
    for (i = 0; i < tz_len; ++i) {
        payload[i + 2] = (uint8_t)tz[i];
    }

    result = fn_clock_exchange(command, payload, (uint16_t)(tz_len + 2), &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }

    return status;
}
