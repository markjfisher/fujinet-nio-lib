#include <string.h>

#include "fujinet-nio.h"
#include "fn_protocol.h"
#include "fn_raw.h"
#include "fn_legacy_appkey_internal.h"

static uint16_t get_u16le(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t get_u32le(const uint8_t *p)
{
    return (uint32_t)p[0]
        | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16)
        | ((uint32_t)p[3] << 24);
}

static void put_u16le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & 0xFF);
    p[1] = (uint8_t)((value >> 8) & 0xFF);
}

static void put_u32le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value & 0xFF);
    p[1] = (uint8_t)((value >> 8) & 0xFF);
    p[2] = (uint8_t)((value >> 16) & 0xFF);
    p[3] = (uint8_t)((value >> 24) & 0xFF);
}

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
    put_u32le(&_fn_legacy_appkey_transfer[off], 0);
    off = (uint16_t)(off + 4);
    put_u16le(&_fn_legacy_appkey_transfer[off], capacity);
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
        get_u32le(&_fn_legacy_appkey_transfer[4]) != 0) {
        *count = 0;
        return false;
    }

    data_len = get_u16le(&_fn_legacy_appkey_transfer[8]);
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
