#include <string.h>

#include "fn_bbc_internal.h"

uint8_t fn_write(fn_handle_t handle,
                 uint32_t offset,
                 const uint8_t *data,
                 uint16_t len,
                 uint16_t *written)
{
    int8_t slot;
    uint8_t block[16];
    uint16_t chunk;
    uint16_t total;

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (handle == FN_INVALID_HANDLE) {
        return FN_ERR_INVALID;
    }

    slot = fn_find_session(handle);
    if (slot < 0) {
        return FN_ERR_NOT_FOUND;
    }

    if (offset != _fn_sessions[slot].write_offset) {
        return FN_ERR_INVALID;
    }

    if (written != 0) {
        *written = 0;
    }

    if (len == 0) {
        return FN_OK;
    }

    if (data == 0) {
        return FN_ERR_INVALID;
    }

    total = 0;
    while (total < len) {
        chunk = (uint16_t)(len - total);
        if (chunk > FN_BBC_OSWORD_STR_MAX) {
            chunk = FN_BBC_OSWORD_STR_MAX;
        }

        memset(block, 0, sizeof(block));
        block[0] = FN_BBC_REASON_WRITE_DATA;
        block[2] = (uint8_t)(((unsigned)(data + total)) & 0xFFu);
        block[3] = (uint8_t)((((unsigned)(data + total)) >> 8) & 0xFFu);
        block[4] = (uint8_t)(chunk & 0xFFu);
        block[5] = (uint8_t)(chunk >> 8);
        block[6] = (uint8_t)handle;

        {
            uint8_t result;

            result = fn_bbc_status_to_result(fn_bbc_osword78(block));
            if (result != FN_OK) {
                return result;
            }
        }

        total += chunk;
    }

    _fn_sessions[slot].write_offset += total;
    if (written != 0) {
        *written = total;
    }
    return FN_OK;
}

uint8_t fn_set_body_length(uint16_t len)
{
    uint8_t block[16];

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    memset(block, 0, sizeof(block));
    block[0] = FN_BBC_REASON_SET_BODY_LEN;
    block[2] = (uint8_t)(len & 0xFFu);
    block[3] = (uint8_t)(len >> 8);

    return fn_bbc_status_to_result(fn_bbc_osword78(block));
}

uint8_t fn_set_content_profile(uint8_t profile)
{
    uint8_t block[16];

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    memset(block, 0, sizeof(block));
    block[0] = FN_BBC_REASON_CONTENT_TYPE;
    block[2] = profile;

    return fn_bbc_status_to_result(fn_bbc_osword78(block));
}
