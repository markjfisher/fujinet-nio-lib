#include "fn_mount_resolve_internal.h"

uint8_t fn_format_mount_display(fn_mount_resolve_io_t *io,
                                const char *canonical_uri,
                                char *display_path,
                                uint16_t display_cap,
                                uint8_t *flags_out)
{
    uint16_t request_len;
    uint16_t response_len;
    char canonical[2];
    uint8_t result;

    result = fn_mount_resolve_build_format(io, canonical_uri, &request_len);
    if (result != FN_OK) {
        return result;
    }
    result = fn_mount_resolve_call(io, FN_CMD_FILE_RESOLVE_PATH, request_len, &response_len);
    if (result != FN_OK) {
        return result;
    }
    return fn_mount_resolve_parse_response(io->buffer,
                                           response_len,
                                           canonical,
                                           sizeof(canonical),
                                           display_path,
                                           display_cap,
                                           flags_out);
}
