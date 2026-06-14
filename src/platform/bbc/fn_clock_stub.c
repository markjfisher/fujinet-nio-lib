#include "fujinet-nio.h"

uint8_t fn_clock_get(FN_TIME_T *time)
{
    (void)time;
    return FN_ERR_UNSUPPORTED;
}

uint8_t fn_clock_set(const FN_TIME_T *time)
{
    (void)time;
    return FN_ERR_UNSUPPORTED;
}

uint8_t fn_clock_get_format(uint8_t *time_data, FnTimeFormat format)
{
    (void)time_data;
    (void)format;
    return FN_ERR_UNSUPPORTED;
}

uint8_t fn_clock_get_tz(uint8_t *time_data, const char *tz, FnTimeFormat format)
{
    (void)time_data;
    (void)tz;
    (void)format;
    return FN_ERR_UNSUPPORTED;
}

uint8_t fn_clock_get_timezone(char *tz)
{
    (void)tz;
    return FN_ERR_UNSUPPORTED;
}

uint8_t fn_clock_set_timezone(const char *tz)
{
    (void)tz;
    return FN_ERR_UNSUPPORTED;
}

uint8_t fn_clock_set_timezone_save(const char *tz)
{
    (void)tz;
    return FN_ERR_UNSUPPORTED;
}

uint8_t fn_clock_sync_network_time(FN_TIME_T *time)
{
    (void)time;
    return FN_ERR_UNSUPPORTED;
}
