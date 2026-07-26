#include <string.h>

#include "fn_mount_resolve_internal.h"

uint8_t fn_mount_resolve_validate_io(fn_mount_resolve_io_t *io, uint16_t min_capacity)
{
    if (io == 0 || io->buffer == 0 || io->capacity < min_capacity) {
        return FN_ERR_INVALID;
    }
    return FN_OK;
}

uint8_t fn_mount_resolve_parse_response(const uint8_t *payload,
                                        uint16_t payload_len,
                                        char *canonical_uri,
                                        uint16_t canonical_cap,
                                        char *display_path,
                                        uint16_t display_cap,
                                        uint8_t *flags_out)
{
    uint16_t off;
    uint16_t raw_len;
    uint16_t copy_len;

    if (payload == 0 || payload_len < 8 || canonical_uri == 0 || canonical_cap == 0) {
        return FN_ERR_INVALID;
    }
    if (payload[0] != FN_FILEPROTO_VERSION) {
        return FN_ERR_IO;
    }
    if (flags_out != 0) {
        *flags_out = payload[1];
    }

    off = 4;
    raw_len = FN_GET_LE16(&payload[off]);
    off = (uint16_t)(off + 2);
    if ((uint16_t)(off + raw_len + 2) > payload_len) {
        return FN_ERR_IO;
    }
    copy_len = raw_len;
    if (copy_len >= canonical_cap) {
        copy_len = (uint16_t)(canonical_cap - 1);
    }
    memcpy(canonical_uri, &payload[off], copy_len);
    canonical_uri[copy_len] = 0;
    off = (uint16_t)(off + raw_len);

    raw_len = FN_GET_LE16(&payload[off]);
    off = (uint16_t)(off + 2);
    if ((uint16_t)(off + raw_len) > payload_len) {
        return FN_ERR_IO;
    }
    if (display_path != 0 && display_cap != 0) {
        copy_len = raw_len;
        if (copy_len >= display_cap) {
            copy_len = (uint16_t)(display_cap - 1);
        }
        memcpy(display_path, &payload[off], copy_len);
        display_path[copy_len] = 0;
    }
    return FN_OK;
}
