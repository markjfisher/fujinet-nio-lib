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

uint8_t fn_appstore_validate_io(fn_appstore_io_t *io, uint16_t min_capacity)
{
    if (io == 0 || io->buffer == 0 || io->capacity < min_capacity) {
        return FN_ERR_INVALID;
    }
    return FN_OK;
}

uint16_t fn_appstore_build_prefix(fn_appstore_io_t *io,
                                  const char *namespace_name,
                                  const char *key,
                                  uint8_t key_required)
{
    uint8_t *buf;
    uint16_t ns_len;
    uint16_t key_len;
    uint16_t off;
    uint8_t result;

    result = check_name_len(namespace_name, &ns_len, 0);
    if (result != FN_OK) {
        return 0;
    }
    result = check_name_len(key ? key : "", &key_len, !key_required);
    if (result != FN_OK || (key_required && key_len == 0)) {
        return 0;
    }
    if (fn_appstore_validate_io(io, (uint16_t)(1 + 2 + ns_len + 2 + key_len)) != FN_OK) {
        return 0;
    }

    buf = io->buffer;
    off = 0;
    buf[off++] = FN_FILEPROTO_VERSION;
    FN_PUT_LE16(&buf[off], ns_len);
    off = (uint16_t)(off + 2);
    memcpy(&buf[off], namespace_name, ns_len);
    off = (uint16_t)(off + ns_len);
    FN_PUT_LE16(&buf[off], key_len);
    off = (uint16_t)(off + 2);
    if (key_len != 0) {
        memcpy(&buf[off], key, key_len);
        off = (uint16_t)(off + key_len);
    }
    return off;
}
