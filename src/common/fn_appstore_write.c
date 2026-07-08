#include <string.h>

#include "fujinet-nio.h"
#include "fn_appstore_internal.h"

uint8_t fn_appstore_write(const char *namespace_name,
                          const char *key,
                          uint32_t offset,
                          const uint8_t *data,
                          uint16_t len,
                          fn_appstore_write_t *out)
{
    uint8_t *req;
    uint8_t *resp;
    uint16_t off;
    uint16_t response_len;
    uint8_t result;

    if (out == 0 || (len != 0 && data == 0)) {
        return FN_ERR_INVALID;
    }
    out->offset = offset;
    out->bytes_written = 0;

    result = fn_appstore_build_prefix(&off, namespace_name, key, 1);
    if (result != FN_OK) {
        return result;
    }
    if ((uint16_t)(off + 6 + len) > fn_appstore_request_capacity()) {
        return FN_ERR_INVALID;
    }

    req = fn_appstore_request_buffer();
    FN_PUT_LE32(&req[off], offset);
    off = (uint16_t)(off + 4);
    FN_PUT_LE16(&req[off], len);
    off = (uint16_t)(off + 2);
    if (len != 0) {
        memcpy(&req[off], data, len);
        off = (uint16_t)(off + len);
    }

    result = fn_appstore_call(FN_CMD_APPSTORE_WRITE, off, &response_len);
    if (result != FN_OK) {
        return result;
    }

    resp = fn_appstore_response_buffer();
    if (response_len < 10 || resp[0] != FN_FILEPROTO_VERSION) {
        return FN_ERR_IO;
    }

    out->offset = FN_GET_LE32(&resp[4]);
    out->bytes_written = FN_GET_LE16(&resp[8]);
    return FN_OK;
}
