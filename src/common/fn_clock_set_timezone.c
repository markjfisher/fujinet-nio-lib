#include "fn_internal.h"

uint8_t fn_clock_set_timezone_common(uint8_t command, const char *tz);

uint8_t fn_clock_set_timezone(const char *tz)
{
    return fn_clock_set_timezone_common(FN_CMD_CLOCK_SET_TZ, tz);
}
