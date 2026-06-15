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

uint8_t fn_bbc_is_tls_rewrite(const char *url, uint16_t url_len, uint8_t flags)
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

const char *fn_bbc_prepare_short_open_name(const char *url, uint16_t url_len, uint8_t flags)
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
