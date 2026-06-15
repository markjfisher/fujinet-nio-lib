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

const char *fn_bbc_prepare_short_open_name(const char *url, uint16_t url_len)
{
    return fn_bbc_make_osfind_name(url, url_len);
}
