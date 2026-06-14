#include "fn_internal.h"

fn_session_t _fn_sessions[FN_MAX_SESSIONS];
uint8_t _fn_initialized = 0;
char _fn_tcp_url[FN_MAX_URL_LEN];

int8_t fn_find_free_slot(void)
{
    int8_t i;

    for (i = 0; i < FN_MAX_SESSIONS; ++i) {
        if (!_fn_sessions[i].active) {
            return i;
        }
    }

    return -1;
}

int8_t fn_find_session(fn_handle_t handle)
{
    int8_t i;

    for (i = 0; i < FN_MAX_SESSIONS; ++i) {
        if (_fn_sessions[i].active && _fn_sessions[i].handle == handle) {
            return i;
        }
    }

    return -1;
}

void fn_free_handle(fn_handle_t handle)
{
    int8_t slot;

    if (handle == FN_INVALID_HANDLE) {
        return;
    }

    slot = fn_find_session(handle);
    if (slot >= 0) {
        _fn_sessions[slot].active = 0;
        _fn_sessions[slot].handle = FN_INVALID_HANDLE;
        _fn_sessions[slot].read_offset = 0;
        _fn_sessions[slot].write_offset = 0;
    }
}
