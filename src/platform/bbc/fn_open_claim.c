#include "fn_bbc_internal.h"

uint8_t fn_bbc_claim_channel(fn_handle_t *handle, unsigned char channel)
{
    int8_t slot;

    slot = fn_find_free_slot();
    if (slot < 0) {
        close_file(channel);
        return FN_ERR_NO_HANDLES;
    }

    *handle = (fn_handle_t)channel;
    _fn_sessions[slot].active = 1;
    _fn_sessions[slot].handle = (fn_handle_t)channel;
    _fn_sessions[slot].read_offset = 0;
    _fn_sessions[slot].write_offset = 0;
    _fn_sessions[slot].proto_flags = FN_PROTO_FLAG_SEQUENTIAL_READ | FN_PROTO_FLAG_SEQUENTIAL_WRITE;
    _fn_sessions[slot].needs_body = 0;
    _fn_sessions[slot].reserved = 0;
    return FN_OK;
}
