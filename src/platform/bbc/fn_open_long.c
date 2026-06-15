#include <string.h>

#include "fn_bbc_internal.h"

static char _fn_bbc_long_sentinel[] = "://\r";

const char *fn_bbc_prepare_long_open_name(const char *url,
                                          uint16_t url_len,
                                          uint8_t flags,
                                          uint8_t *result)
{
    if (fn_bbc_is_tls_rewrite(url, url_len, flags)) {
        *result = FN_ERR_URL_TOO_LONG;
        return 0;
    }

    if (fn_bbc_arm_open_url(url, url_len) != FN_OK) {
        *result = FN_ERR_INVALID;
        return 0;
    }

    *result = FN_OK;
    return _fn_bbc_long_sentinel;
}
