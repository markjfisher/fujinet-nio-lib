#include <string.h>

#include "fn_bbc_internal.h"

uint8_t fn_open(fn_handle_t *handle,
                uint8_t method,
                const char *url,
                uint8_t flags)
{
    unsigned char channel;
    int mode;
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

    if (flags & FN_OPEN_STREAM_NO_PROBE) {
        uint8_t result = fn_bbc_arm_open_flags(FN_OPEN_FLAG_STREAM_NO_PROBE);
        if (result != FN_OK) {
            return result;
        }
    }

    osfind_name = fn_bbc_prepare_short_open_name(url, url_len);

    if (osfind_name == 0) {
        return FN_ERR_URL_TOO_LONG;
    }

    channel = osfind((unsigned char)mode, osfind_name);
    if (channel == 0) {
        return FN_ERR_IO;
    }

    return fn_bbc_claim_channel(handle, channel);
}
