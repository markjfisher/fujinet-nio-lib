#include "fn_internal.h"

static uint8_t fn_clock_set_timezone_common(uint8_t command, const char *tz)
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
    if (_fn_resp_buf[data_offset] != FN_CLOCK_VERSION) {
        return FN_ERR_UNSUPPORTED;
    }

    tz_len = _fn_resp_buf[data_offset + 1];
    for (i = 0; i < tz_len && i < FN_MAX_TIMEZONE_LEN - 1; ++i) {
        tz[i] = (char)_fn_resp_buf[data_offset + 2 + i];
    }
    tz[i] = '\0';

    return FN_OK;
}

uint8_t fn_clock_set_timezone(const char *tz)
{
    return fn_clock_set_timezone_common(FN_CMD_CLOCK_SET_TZ, tz);
}

uint8_t fn_clock_set_timezone_save(const char *tz)
{
    return fn_clock_set_timezone_common(FN_CMD_CLOCK_SET_TZ_SAVE, tz);
}
