#include <string.h>

#include "fujinet-nio.h"
#include "fn_internal.h"
#include "fn_protocol.h"
#include "fn_raw.h"
#include "fn_legacy_appkey_internal.h"

bool fuji_read_appkey(uint8_t key_id, uint16_t *count, uint8_t *data)
{
    char uri[FN_LEGACY_APPKEY_URI_MAX + 1];
    uint16_t uri_len;
    uint16_t off;
    uint16_t capacity;
    uint16_t data_len;
    fn_raw_response_t raw;

    if (count == 0 || data == 0 || _fn_legacy_appkey_creator_id == 0) {
        return false;
    }

    capacity = _fn_legacy_appkey_capacity();
    uri_len = _fn_legacy_appkey_build_uri(key_id, uri, sizeof(uri));
    if (uri_len == 0) {
        return false;
    }

    off = _fn_legacy_file_uri_prefix(_fn_legacy_appkey_transfer, uri, uri_len);
    FN_PUT_LE32(&_fn_legacy_appkey_transfer[off], 0);
    off = (uint16_t)(off + 4);
    FN_PUT_LE16(&_fn_legacy_appkey_transfer[off], capacity);
    off = (uint16_t)(off + 2);

    if (fn_raw_call(FN_DEVICE_FILE,
                    FN_CMD_FILE_READ,
                    _fn_legacy_appkey_transfer,
                    off,
                    _fn_legacy_appkey_transfer,
                    (uint16_t)(FN_LEGACY_APPKEY_READ_RESPONSE_HEADER + capacity),
                    &raw) != FN_OK ||
        raw.status != FN_OK ||
        raw.payload_length < FN_LEGACY_APPKEY_READ_RESPONSE_HEADER ||
        _fn_legacy_appkey_transfer[0] != 1 ||
        FN_GET_LE32(&_fn_legacy_appkey_transfer[4]) != 0) {
        *count = 0;
        return false;
    }

    data_len = FN_GET_LE16(&_fn_legacy_appkey_transfer[8]);
    if ((uint16_t)(FN_LEGACY_APPKEY_READ_RESPONSE_HEADER + data_len) > raw.payload_length ||
        data_len > capacity) {
        *count = 0;
        return false;
    }

    if (data_len != 0) {
        memcpy(data, &_fn_legacy_appkey_transfer[FN_LEGACY_APPKEY_READ_RESPONSE_HEADER], data_len);
    }
    *count = data_len;
    return true;
}
