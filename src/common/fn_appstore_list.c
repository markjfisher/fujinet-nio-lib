#include <string.h>

#include "fujinet-nio.h"
#include "fn_appstore_internal.h"

uint8_t fn_appstore_list(fn_appstore_io_t *io,
                         const char *namespace_name,
                         uint16_t start_index,
                         uint8_t *key_data,
                         uint16_t key_data_capacity,
                         fn_appstore_list_t *out)
{
    uint8_t *req;
    uint8_t *resp;
    uint16_t off;
    uint16_t key_data_len;
    uint16_t response_len;
    uint8_t result;

    if (out == 0 || key_data == 0 ||
        fn_appstore_validate_io(io, 10) != FN_OK ||
        key_data_capacity == 0 ||
        key_data_capacity > (uint16_t)(io->capacity - 10)) {
        return FN_ERR_INVALID;
    }

    off = fn_appstore_build_prefix(io, namespace_name, "", 0);
    if (off == 0) {
        return FN_ERR_INVALID;
    }

    if ((uint16_t)(off + 4) > io->capacity) {
        return FN_ERR_INVALID;
    }
    req = io->buffer;
    FN_PUT_LE16(&req[off], start_index);
    off = (uint16_t)(off + 2);
    FN_PUT_LE16(&req[off], key_data_capacity);
    off = (uint16_t)(off + 2);

    result = fn_appstore_call(io, FN_CMD_APPSTORE_LIST, off, &response_len);
    if (result != FN_OK) {
        return result;
    }

    resp = io->buffer;
    if (response_len < 10 || resp[0] != FN_APPSTORE_PROTOCOL_VERSION) {
        return FN_ERR_IO;
    }

    key_data_len = FN_GET_LE16(&resp[8]);
    if ((uint16_t)(10 + key_data_len) > response_len || key_data_len > key_data_capacity) {
        return FN_ERR_IO;
    }
    if (key_data_len != 0) {
        memcpy(key_data, &resp[10], key_data_len);
    }

    out->flags = resp[1];
    out->start_index = FN_GET_LE16(&resp[4]);
    out->key_count = FN_GET_LE16(&resp[6]);
    out->key_data_len = key_data_len;
    return FN_OK;
}
