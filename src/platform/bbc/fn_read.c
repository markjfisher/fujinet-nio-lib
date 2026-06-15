#include "fn_bbc_internal.h"

uint8_t fn_read(fn_handle_t handle,
                uint32_t offset,
                uint8_t *buf,
                uint16_t max_len,
                uint16_t *bytes_read,
                uint8_t *flags)
{
    int8_t slot;
    int rc;
    uint16_t count;

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (handle == FN_INVALID_HANDLE || buf == 0 || bytes_read == 0) {
        return FN_ERR_INVALID;
    }

    slot = fn_find_session(handle);
    if (slot < 0) {
        return FN_ERR_NOT_FOUND;
    }

    if (offset != _fn_sessions[slot].read_offset) {
        return FN_ERR_INVALID;
    }

    count = 0;
    while (count < max_len) {
        rc = fn_bbc_osbget((unsigned char)handle);
        if (rc < 0) {
            break;
        }
        buf[count++] = (uint8_t)rc;
    }

    *bytes_read = count;
    if (flags != 0) {
        *flags = (count < max_len) ? FN_READ_EOF : 0;
    }

    _fn_sessions[slot].read_offset += count;
    return FN_OK;
}
