#include <string.h>

#include "fn_bbc_internal.h"

uint8_t fn_open(fn_handle_t *handle,
                uint8_t method,
                const char *url,
                uint8_t flags)
{
    unsigned char channel;
    int mode;
    int8_t slot;
    uint16_t url_len;
    const char *osfind_name;
    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (handle == 0 || url == 0) {
        return FN_ERR_INVALID;
    }

    mode = fn_bbc_open_flags(method);
    if (mode < 0) {
        return FN_ERR_UNSUPPORTED;
    }

    url_len = (uint16_t)strlen(url);
    if (url_len > FN_MAX_URL_LEN) {
        return FN_ERR_URL_TOO_LONG;
    }

    if (url_len > FN_BBC_DIRECT_URL_MAX) {
        return FN_ERR_URL_TOO_LONG;
    }

    (void)flags;
    osfind_name = fn_bbc_prepare_short_open_name(url, url_len);

    if (osfind_name == 0) {
        return FN_ERR_URL_TOO_LONG;
    }

    channel = osfind((unsigned char)mode, osfind_name);
    if (channel == 0) {
        return FN_ERR_IO;
    }

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
