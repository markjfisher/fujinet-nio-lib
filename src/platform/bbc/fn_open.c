#include <string.h>

#include "fn_bbc_internal.h"

static char _fn_bbc_tls_url[FN_MAX_URL_LEN];
static char _fn_bbc_open_name[FN_BBC_DIRECT_URL_MAX + 1];

static const char *fn_bbc_apply_tls_flag(const char *url, uint8_t flags)
{
    uint16_t len;

    if ((flags & FN_OPEN_TLS) == 0 || url == 0) {
        return url;
    }

    if (strncmp(url, "http://", 7) == 0) {
        len = (uint16_t)strlen(url + 7);
        if ((uint16_t)(len + 8) > FN_MAX_URL_LEN) {
            return 0;
        }
        memcpy(_fn_bbc_tls_url, "https://", 8);
        memcpy(_fn_bbc_tls_url + 8, url + 7, len + 1);
        return _fn_bbc_tls_url;
    }

    if (strncmp(url, "tcp://", 6) == 0) {
        len = (uint16_t)strlen(url + 6);
        if ((uint16_t)(len + 6) > FN_MAX_URL_LEN) {
            return 0;
        }
        memcpy(_fn_bbc_tls_url, "tls://", 6);
        memcpy(_fn_bbc_tls_url + 6, url + 6, len + 1);
        return _fn_bbc_tls_url;
    }

    return url;
}

static const char *fn_bbc_make_osfind_name(const char *src, uint16_t len)
{
    if (len >= sizeof(_fn_bbc_open_name)) {
        return 0;
    }

    memcpy(_fn_bbc_open_name, src, len);
    _fn_bbc_open_name[len] = '\r';
    return _fn_bbc_open_name;
}

uint8_t fn_open(fn_handle_t *handle,
                uint8_t method,
                const char *url,
                uint8_t flags)
{
    unsigned char channel;
    int mode;
    int8_t slot;
    uint16_t url_len;
    const char *open_url;
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

    open_url = fn_bbc_apply_tls_flag(url, flags);
    if (open_url == 0) {
        return FN_ERR_URL_TOO_LONG;
    }

    url_len = (uint16_t)strlen(open_url);
    if (url_len > FN_MAX_URL_LEN) {
        return FN_ERR_URL_TOO_LONG;
    }

    if (url_len > FN_BBC_DIRECT_URL_MAX) {
        if (fn_bbc_arm_open_url(open_url, url_len) != FN_OK) {
            return FN_ERR_INVALID;
        }
        osfind_name = fn_bbc_make_osfind_name("://", 3);
    } else {
        osfind_name = fn_bbc_make_osfind_name(open_url, url_len);
    }

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
