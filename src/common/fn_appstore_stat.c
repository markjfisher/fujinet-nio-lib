#include "fujinet-nio.h"
#include "fn_appstore_internal.h"

uint8_t fn_appstore_stat(fn_appstore_io_t *io,
                         const char *namespace_name,
                         const char *key,
                         fn_appstore_stat_t *out)
{
    uint8_t *resp;
    uint16_t off;
    uint16_t response_len;
    uint8_t result;

    if (out == 0 || fn_appstore_validate_io(io, 20) != FN_OK) {
        return FN_ERR_INVALID;
    }

    off = fn_appstore_build_prefix(io, namespace_name, key, 1);
    if (off == 0) {
        return FN_ERR_INVALID;
    }
    result = fn_appstore_call(io, FN_CMD_APPSTORE_STAT, off, &response_len);
    if (result != FN_OK) {
        return result;
    }

    resp = io->buffer;
    if (response_len < 20 || resp[0] != FN_FILEPROTO_VERSION) {
        return FN_ERR_IO;
    }

    out->exists = (uint8_t)((resp[1] & 0x01) ? 1 : 0);
    out->size_bytes = FN_GET_LE32(&resp[4]);
    out->size_bytes_high = FN_GET_LE32(&resp[8]);
    out->mtime_unix = FN_GET_LE32(&resp[12]);
    out->mtime_unix_high = FN_GET_LE32(&resp[16]);
    return FN_OK;
}
