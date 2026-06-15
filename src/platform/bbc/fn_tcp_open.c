#include <string.h>

#include "fn_bbc_internal.h"

static char _fn_bbc_tcp_url[FN_MAX_URL_LEN];

uint8_t fn_tcp_open(fn_handle_t *handle,
                    const char *host,
                    uint16_t port)
{
    uint8_t offset;
    uint16_t p;

    if (handle == 0 || host == 0) {
        return FN_ERR_INVALID;
    }

    strcpy(_fn_bbc_tcp_url, "tcp://");
    offset = 6;

    if ((uint16_t)(offset + strlen(host)) > FN_MAX_URL_LEN - 10) {
        return FN_ERR_URL_TOO_LONG;
    }

    strcpy(_fn_bbc_tcp_url + offset, host);
    offset += (uint8_t)strlen(host);
    _fn_bbc_tcp_url[offset++] = ':';

    p = port;
    if (p >= 10000) {
        _fn_bbc_tcp_url[offset++] = (char)('0' + (p / 10000));
        p %= 10000;
    }
    if (p >= 1000) {
        _fn_bbc_tcp_url[offset++] = (char)('0' + (p / 1000));
        p %= 1000;
    }
    if (p >= 100) {
        _fn_bbc_tcp_url[offset++] = (char)('0' + (p / 100));
        p %= 100;
    }
    if (p >= 10) {
        _fn_bbc_tcp_url[offset++] = (char)('0' + (p / 10));
        p %= 10;
    }
    _fn_bbc_tcp_url[offset++] = (char)('0' + p);
    _fn_bbc_tcp_url[offset] = '\0';

    return fn_open(handle, 0, _fn_bbc_tcp_url, 0);
}
