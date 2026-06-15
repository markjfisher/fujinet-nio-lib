#include <string.h>

#include "fn_bbc_internal.h"

static char _fn_bbc_long_sentinel[] = "://\r";

uint8_t fn_open_long(fn_handle_t *handle,
                     uint8_t method,
                     const char *url,
                     uint8_t flags)
{
    unsigned char channel;
    int mode;
    uint16_t url_len;

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

    if (url_len <= FN_BBC_DIRECT_URL_MAX) {
        return fn_open(handle, method, url, flags);
    }

    (void)flags;
    if (fn_bbc_arm_open_url(url, url_len) != FN_OK) {
        return FN_ERR_INVALID;
    }

    channel = osfind((unsigned char)mode, _fn_bbc_long_sentinel);
    if (channel == 0) {
        return FN_ERR_IO;
    }

    return fn_bbc_claim_channel(handle, channel);
}
