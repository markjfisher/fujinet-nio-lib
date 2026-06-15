#include <string.h>

#include "fn_bbc_internal.h"

static char _fn_bbc_open_name[FN_BBC_DIRECT_URL_MAX + 1];

static const char *fn_bbc_make_osfind_name(const char *src, uint16_t len)
{
    if (len >= sizeof(_fn_bbc_open_name)) {
        return 0;
    }

    memcpy(_fn_bbc_open_name, src, len);
    _fn_bbc_open_name[len] = '\r';
    return _fn_bbc_open_name;
}

static uint8_t fn_bbc_is_tls_rewrite(const char *url, uint16_t url_len, uint8_t flags)
{
    if ((flags & FN_OPEN_TLS) == 0) {
        return 0;
    }
    if (url_len >= 7 && strncmp(url, "http://", 7) == 0) {
        return 1;
    }
    if (url_len >= 6 && strncmp(url, "tcp://", 6) == 0) {
        return 1;
    }
    return 0;
}

static const char *fn_bbc_prepare_open_name(const char *url,
                                            uint16_t url_len,
                                            uint8_t flags)
{
    uint16_t tail_len;

    if (!fn_bbc_is_tls_rewrite(url, url_len, flags)) {
        return fn_bbc_make_osfind_name(url, url_len);
    }

    if (url[0] == 'h') {
        tail_len = (uint16_t)(url_len - 7);
        if ((uint16_t)(tail_len + 8) >= sizeof(_fn_bbc_open_name)) {
            return 0;
        }
        memcpy(_fn_bbc_open_name, "https://", 8);
        memcpy(_fn_bbc_open_name + 8, url + 7, tail_len);
        _fn_bbc_open_name[tail_len + 8] = '\r';
        return _fn_bbc_open_name;
    }

    tail_len = (uint16_t)(url_len - 6);
    if ((uint16_t)(tail_len + 6) >= sizeof(_fn_bbc_open_name)) {
        return 0;
    }
    memcpy(_fn_bbc_open_name, "tls://", 6);
    memcpy(_fn_bbc_open_name + 6, url + 6, tail_len);
    _fn_bbc_open_name[tail_len + 6] = '\r';
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
        if (fn_bbc_is_tls_rewrite(url, url_len, flags)) {
            return FN_ERR_URL_TOO_LONG;
        }

        if (fn_bbc_arm_open_url(url, url_len) != FN_OK) {
            return FN_ERR_INVALID;
        }
        osfind_name = fn_bbc_make_osfind_name("://", 3);
    } else {
        osfind_name = fn_bbc_prepare_open_name(url, url_len, flags);
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
