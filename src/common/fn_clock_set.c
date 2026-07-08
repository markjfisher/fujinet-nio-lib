#include "fn_internal.h"

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
        payload[i + 1] = (uint8_t)((*time >> (8 * i)) & 0xFFu);
    }
#endif

    result = fn_clock_exchange(FN_CMD_CLOCK_SET, payload, sizeof(payload), &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }

    return status;
}
