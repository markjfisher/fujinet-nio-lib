#include <string.h>

#include "fn_appstore_internal.h"

static uint8_t check_name_len(const char *s, uint16_t *len, uint8_t allow_empty)
{
    size_t n;

    if (s == 0) {
        return FN_ERR_INVALID;
    }

    n = strlen(s);
    if ((!allow_empty && n == 0) || n > 255) {
        return FN_ERR_INVALID;
    }

    *len = (uint16_t)n;
    return FN_OK;
}

uint8_t fn_appstore_build_prefix(uint16_t *off,
                                 const char *namespace_name,
                                 const char *key,
                                 uint8_t key_required)
{
    uint8_t *buf;
    uint16_t ns_len;
    uint16_t key_len;
    uint8_t result;

    result = check_name_len(namespace_name, &ns_len, 0);
    if (result != FN_OK) {
        return result;
    }
    result = check_name_len(key ? key : "", &key_len, !key_required);
    if (result != FN_OK || (key_required && key_len == 0)) {
        return FN_ERR_INVALID;
    }

    buf = fn_appstore_request_buffer();
    *off = 0;
    buf[(*off)++] = FN_FILEPROTO_VERSION;
    FN_PUT_LE16(&buf[*off], ns_len);
    *off = (uint16_t)(*off + 2);
    memcpy(&buf[*off], namespace_name, ns_len);
    *off = (uint16_t)(*off + ns_len);
    FN_PUT_LE16(&buf[*off], key_len);
    *off = (uint16_t)(*off + 2);
    if (key_len != 0) {
        memcpy(&buf[*off], key, key_len);
        *off = (uint16_t)(*off + key_len);
    }
    return FN_OK;
}
