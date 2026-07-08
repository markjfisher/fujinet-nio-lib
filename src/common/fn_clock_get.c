#include "fn_internal.h"

uint8_t fn_clock_copy_time(FN_TIME_T *time, uint16_t data_offset);

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
