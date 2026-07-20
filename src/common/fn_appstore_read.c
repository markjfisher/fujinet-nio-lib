#include <string.h>

#include "fujinet-nio.h"
#include "fn_appstore_internal.h"

uint8_t fn_appstore_read(fn_appstore_io_t *io,
                         const char *namespace_name,
                         const char *key,
                         uint32_t offset,
                         uint8_t *buf,
                         uint16_t max_len,
                         fn_appstore_read_t *out)
{
    uint8_t *req;
    uint8_t *resp;
    uint16_t off;
    uint16_t data_len;
    uint16_t response_len;
    uint8_t result;

    if (out == 0 || (max_len != 0 && buf == 0) ||
        fn_appstore_validate_io(io, 10) != FN_OK ||
        max_len == 0 || max_len > (uint16_t)(io->capacity - 10)) {
        return FN_ERR_INVALID;
    }
    out->flags = 0;
    out->offset = offset;
    out->bytes_read = 0;

    result = fn_appstore_build_prefix(io, &off, namespace_name, key, 1);
    if (result != FN_OK) {
        return result;
    }
    if ((uint16_t)(off + 6) > io->capacity) {
        return FN_ERR_INVALID;
    }
    req = io->buffer;
    FN_PUT_LE32(&req[off], offset);
    off = (uint16_t)(off + 4);
    FN_PUT_LE16(&req[off], max_len);
    off = (uint16_t)(off + 2);

    result = fn_appstore_call(io, FN_CMD_APPSTORE_READ, off, &response_len);
    if (result != FN_OK) {
        return result;
    }

    resp = io->buffer;
    if (response_len < 10 || resp[0] != FN_FILEPROTO_VERSION) {
        return FN_ERR_IO;
    }

    data_len = FN_GET_LE16(&resp[8]);
    if ((uint16_t)(10 + data_len) > response_len || data_len > max_len) {
        return FN_ERR_IO;
    }
    if (data_len != 0) {
        memmove(buf, &resp[10], data_len);
    }

    out->flags = resp[1];
    out->offset = FN_GET_LE32(&resp[4]);
    out->bytes_read = data_len;
    return FN_OK;
}
