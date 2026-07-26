#include <string.h>

#include "fn_mount_resolve_internal.h"

static uint8_t check_len(const char *s, uint16_t *len, uint8_t allow_empty)
{
    size_t n;

    if (s == 0) {
        return FN_ERR_INVALID;
    }
    n = strlen(s);
    if ((!allow_empty && n == 0) || n > 255u) {
        return FN_ERR_INVALID;
    }
    *len = (uint16_t)n;
    return FN_OK;
}

static uint8_t has_prefix(const char *host)
{
    const char *p;

    for (p = host; *p && *p != '/' && *p != '\\'; ++p) {
        if (*p == ':') {
            return 1;
        }
    }
    return 0;
}

uint8_t fn_mount_resolve_build_resolve(fn_mount_resolve_io_t *io,
                                       const char *host,
                                       const char *browse_path,
                                       const char *leaf,
                                       uint16_t *request_len)
{
    static const char prefix[] = "tnfs://";
    uint16_t host_len;
    uint16_t path_len;
    uint16_t leaf_len;
    uint16_t base_len;
    uint16_t off;
    uint8_t prefixed;

    if (request_len == 0 ||
        check_len(host, &host_len, 0) != FN_OK ||
        check_len(browse_path ? browse_path : "", &path_len, 1) != FN_OK ||
        check_len(leaf ? leaf : "", &leaf_len, 1) != FN_OK) {
        return FN_ERR_INVALID;
    }

    prefixed = has_prefix(host);
    base_len = host_len + path_len + 3;
    if (!prefixed) {
        base_len = (uint16_t)(base_len + sizeof(prefix) - 1);
    }
    if (fn_mount_resolve_validate_io(io, (uint16_t)(1 + 2 + base_len + 2 + leaf_len)) != FN_OK) {
        return FN_ERR_INVALID;
    }

    off = 0;
    io->buffer[off++] = FN_FILEPROTO_VERSION;
    FN_PUT_LE16(&io->buffer[off], 0);
    off = (uint16_t)(off + 2);
    if (!prefixed) {
        memcpy(&io->buffer[off], prefix, sizeof(prefix) - 1);
        off = (uint16_t)(off + sizeof(prefix) - 1);
    }
    memcpy(&io->buffer[off], host, host_len);
    off = (uint16_t)(off + host_len);
    if (path_len != 0) {
        if (io->buffer[off - 1] != '/' && browse_path[0] != '/') {
            io->buffer[off++] = '/';
        }
        memcpy(&io->buffer[off], browse_path, path_len);
        off = (uint16_t)(off + path_len);
    } else if (!prefixed && io->buffer[off - 1] != '/') {
        io->buffer[off++] = '/';
    }
    base_len = (uint16_t)(off - 3);
    FN_PUT_LE16(&io->buffer[1], base_len);
    FN_PUT_LE16(&io->buffer[off], leaf_len);
    off = (uint16_t)(off + 2);
    if (leaf_len != 0) {
        memcpy(&io->buffer[off], leaf, leaf_len);
        off = (uint16_t)(off + leaf_len);
    }
    *request_len = off;
    return FN_OK;
}

uint8_t fn_mount_resolve_build_format(fn_mount_resolve_io_t *io,
                                      const char *canonical_uri,
                                      uint16_t *request_len)
{
    uint16_t uri_len;

    if (request_len == 0 || check_len(canonical_uri, &uri_len, 0) != FN_OK) {
        return FN_ERR_INVALID;
    }
    if (fn_mount_resolve_validate_io(io, (uint16_t)(1 + 2 + uri_len + 2)) != FN_OK) {
        return FN_ERR_INVALID;
    }

    io->buffer[0] = FN_FILEPROTO_VERSION;
    FN_PUT_LE16(&io->buffer[1], uri_len);
    memcpy(&io->buffer[3], canonical_uri, uri_len);
    FN_PUT_LE16(&io->buffer[3 + uri_len], 0);
    *request_len = (uint16_t)(5 + uri_len);
    return FN_OK;
}
