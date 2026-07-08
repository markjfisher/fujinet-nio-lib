#include "fn_internal.h"

uint8_t fn_clock_copy_time(FN_TIME_T *time, uint16_t data_offset)
{
    uint8_t i;

    if (fn_clock_response_byte(data_offset) != FN_CLOCK_VERSION) {
        return FN_ERR_UNSUPPORTED;
    }

#ifdef __CC65__
    for (i = 0; i < 8; ++i) {
        time->b[i] = fn_clock_response_byte(data_offset + 4 + i);
    }
#else
    *time = 0;
    for (i = 0; i < 8; ++i) {
        *time |= ((uint64_t)fn_clock_response_byte(data_offset + 4 + i)) << (8 * i);
    }
#endif

    return FN_OK;
}
