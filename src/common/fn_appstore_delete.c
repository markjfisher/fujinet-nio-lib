#include "fujinet-nio.h"
#include "fn_appstore_internal.h"

uint8_t fn_appstore_delete(const char *namespace_name,
                           const char *key,
                           fn_appstore_delete_t *out)
{
    uint8_t *resp;
    uint16_t off;
    uint16_t response_len;
    uint8_t result;

    if (out == 0) {
        return FN_ERR_INVALID;
    }
    out->deleted = 0;

    result = fn_appstore_build_prefix(&off, namespace_name, key, 1);
    if (result != FN_OK) {
        return result;
    }
    result = fn_appstore_call(FN_CMD_APPSTORE_DELETE, off, &response_len);
    if (result != FN_OK) {
        return result;
    }

    resp = fn_appstore_response_buffer();
    if (response_len < 4 || resp[0] != FN_FILEPROTO_VERSION) {
        return FN_ERR_IO;
    }

    out->deleted = (uint8_t)((resp[1] & 0x01) ? 1 : 0);
    return FN_OK;
}
