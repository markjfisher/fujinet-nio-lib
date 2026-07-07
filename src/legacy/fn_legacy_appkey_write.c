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

bool fuji_write_appkey(uint8_t key_id, uint16_t count, uint8_t *data)
{
    char uri[FN_LEGACY_APPKEY_URI_MAX + 1];
    uint16_t uri_len;
    uint16_t off;
    fn_raw_response_t raw;

    if ((count != 0 && data == 0) ||
        _fn_legacy_appkey_creator_id == 0 ||
        count > _fn_legacy_appkey_capacity()) {
        return false;
    }

    if (_fn_legacy_file_mkdir_fuji() != FN_OK) {
        return false;
    }

    uri_len = _fn_legacy_appkey_build_uri(key_id, uri, sizeof(uri));
    if (uri_len == 0) {
        return false;
    }

    off = _fn_legacy_file_uri_prefix(_fn_legacy_appkey_transfer, uri, uri_len);
    put_u32le(&_fn_legacy_appkey_transfer[off], 0);
    off = (uint16_t)(off + 4);
    put_u16le(&_fn_legacy_appkey_transfer[off], count);
    off = (uint16_t)(off + 2);
    if (count != 0) {
        memcpy(&_fn_legacy_appkey_transfer[off], data, count);
        off = (uint16_t)(off + count);
    }

    if (fn_raw_call(FN_DEVICE_FILE,
                    FN_CMD_FILE_WRITE,
                    _fn_legacy_appkey_transfer,
                    off,
                    _fn_legacy_appkey_transfer,
                    10,
                    &raw) != FN_OK ||
        raw.status != FN_OK ||
        raw.payload_length < 10 ||
        _fn_legacy_appkey_transfer[0] != 1 ||
        get_u32le(&_fn_legacy_appkey_transfer[4]) != 0 ||
        get_u16le(&_fn_legacy_appkey_transfer[8]) != count) {
        return false;
    }

    return true;
}
