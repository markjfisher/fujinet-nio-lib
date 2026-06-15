#include "fn_bbc_internal.h"

uint8_t fn_init(void)
{
    int8_t i;

    if (_fn_initialized) {
        return FN_OK;
    }

    if (!fn_bbc_probe_rom()) {
        return FN_ERR_NOT_FOUND;
    }

    for (i = 0; i < FN_MAX_SESSIONS; ++i) {
        _fn_sessions[i].active = 0;
        _fn_sessions[i].handle = FN_INVALID_HANDLE;
        _fn_sessions[i].read_offset = 0;
        _fn_sessions[i].write_offset = 0;
        _fn_sessions[i].proto_flags = 0;
        _fn_sessions[i].needs_body = 0;
        _fn_sessions[i].reserved = 0;
    }

    _fn_initialized = 1;
    return FN_OK;
}

uint8_t fn_is_ready(void)
{
    return fn_bbc_probe_rom();
}
