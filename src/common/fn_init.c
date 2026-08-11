#include "fn_internal.h"
#include "fn_platform.h"

uint8_t fn_init(void)
{
    uint8_t result;
    int8_t i;

    if (_fn_initialized)
        return fn_transport_ready() ? FN_OK : fn_transport_init();

    for (i = 0; i < FN_MAX_SESSIONS; ++i) {
        _fn_sessions[i].active = 0;
    }

    result = fn_transport_init();
    if (result != FN_OK) {
        return result;
    }

    _fn_initialized = 1;
    return FN_OK;
}

uint8_t fn_is_ready(void)
{
    return fn_transport_ready();
}
