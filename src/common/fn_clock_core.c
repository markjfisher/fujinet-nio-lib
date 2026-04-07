#include "fn_internal.h"

static uint8_t fn_clock_copy_time(FN_TIME_T *time, uint16_t data_offset)
{
    uint8_t i;

    if (_fn_resp_buf[data_offset] != FN_CLOCK_VERSION) {
        return FN_ERR_UNSUPPORTED;
    }

#ifdef __CC65__
    for (i = 0; i < 8; ++i) {
        time->b[i] = _fn_resp_buf[data_offset + 4 + i];
    }
#else
    *time = 0;
    for (i = 0; i < 8; ++i) {
        *time |= ((uint64_t)_fn_resp_buf[data_offset + 4 + i]) << (8 * i);
    }
#endif

    return FN_OK;
}

uint8_t fn_clock_get(FN_TIME_T *time)
{
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t result;

    if (time == NULL) {
        return FN_ERR_INVALID;
    }

    result = fn_clock_exchange(FN_CMD_CLOCK_GET, NULL, 0, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    if (status != FN_OK) {
        return status;
    }
    if (data_len < 12) {
        return FN_ERR_INVALID;
    }

    return fn_clock_copy_time(time, data_offset);
}

uint8_t fn_clock_set(const FN_TIME_T *time)
{
    uint8_t payload[9];
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t result;
    uint8_t i;

    if (time == NULL) {
        return FN_ERR_INVALID;
    }

    payload[0] = FN_CLOCK_VERSION;
#ifdef __CC65__
    for (i = 0; i < 8; ++i) {
        payload[i + 1] = time->b[i];
    }
#else
    for (i = 0; i < 8; ++i) {
        payload[i + 1] = (uint8_t)((*time >> (8 * i)) & 0xFF);
    }
#endif

    result = fn_clock_exchange(FN_CMD_CLOCK_SET, payload, sizeof(payload), &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }

    return status;
}

uint8_t fn_clock_sync_network_time(FN_TIME_T *time)
{
    uint8_t payload[1];
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t result;

    if (time == NULL) {
        return FN_ERR_INVALID;
    }

    payload[0] = FN_CLOCK_VERSION;
    result = fn_clock_exchange(FN_CMD_CLOCK_SYNC_NETWORK_TIME, payload, sizeof(payload), &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    if (status != FN_OK) {
        return status;
    }
    if (data_len < 12) {
        return FN_ERR_INVALID;
    }

    return fn_clock_copy_time(time, data_offset);
}
