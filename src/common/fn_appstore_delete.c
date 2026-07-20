#include "fujinet-nio.h"
#include "fn_appstore_internal.h"

uint8_t fn_appstore_delete(fn_appstore_io_t *io,
                           const char *namespace_name,
                           const char *key,
                           fn_appstore_delete_t *out)
{
    uint8_t *resp;
    uint16_t off;
    uint16_t response_len;
    uint8_t result;

    if (out == 0 || fn_appstore_validate_io(io, 4) != FN_OK) {
        return FN_ERR_INVALID;
    }
    out->deleted = 0;

    result = fn_appstore_build_prefix(io, &off, namespace_name, key, 1);
    if (result != FN_OK) {
        return result;
    }
    result = fn_appstore_call(io, FN_CMD_APPSTORE_DELETE, off, &response_len);
    if (result != FN_OK) {
        return result;
    }

    resp = io->buffer;
    if (response_len < 4 || resp[0] != FN_FILEPROTO_VERSION) {
        return FN_ERR_IO;
    }

    out->deleted = (uint8_t)((resp[1] & 0x01) ? 1 : 0);
    return FN_OK;
}
