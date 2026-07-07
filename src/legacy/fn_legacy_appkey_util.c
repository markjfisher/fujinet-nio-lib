#include <string.h>

#include "fujinet-nio.h"
#include "fn_protocol.h"
#include "fn_raw.h"
#include "fn_legacy_appkey_internal.h"

#define FN_FILEPROTO_VERSION 1

static const char k_persist_prefix[] = "persist:///FujiNet/";
static const char k_fuji_dir[] = "persist:///FujiNet";
static const char k_hex[] = "0123456789abcdef";

static void put_u16le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & 0xFF);
    p[1] = (uint8_t)((value >> 8) & 0xFF);
}

static void append_hex8(char *out, uint16_t *offset, uint8_t value)
{
    out[(*offset)++] = k_hex[(value >> 4) & 0x0F];
    out[(*offset)++] = k_hex[value & 0x0F];
}

uint16_t _fn_legacy_appkey_capacity(void)
{
    return _fn_legacy_appkey_size == SIZE_256
        ? FN_LEGACY_APPKEY_SIZE_256
        : FN_LEGACY_APPKEY_DEFAULT_SIZE;
}

uint16_t _fn_legacy_appkey_build_uri(uint8_t key_id, char *uri, uint16_t uri_capacity)
{
    uint16_t off;
    uint16_t prefix_len;

    prefix_len = (uint16_t)(sizeof(k_persist_prefix) - 1);
    if (uri_capacity < (uint16_t)(prefix_len + 12 + 1)) {
        return 0;
    }

    memcpy(uri, k_persist_prefix, prefix_len);
    off = prefix_len;
    append_hex8(uri, &off, (uint8_t)((_fn_legacy_appkey_creator_id >> 8) & 0xFF));
    append_hex8(uri, &off, (uint8_t)(_fn_legacy_appkey_creator_id & 0xFF));
    append_hex8(uri, &off, _fn_legacy_appkey_app_id);
    append_hex8(uri, &off, key_id);
    uri[off++] = '.';
    uri[off++] = 'k';
    uri[off++] = 'e';
    uri[off++] = 'y';
    uri[off] = 0;
    return off;
}

uint16_t _fn_legacy_file_uri_prefix(uint8_t *buf, const char *uri, uint16_t uri_len)
{
    buf[0] = FN_FILEPROTO_VERSION;
    put_u16le(&buf[1], uri_len);
    memcpy(&buf[3], uri, uri_len);
    return (uint16_t)(3 + uri_len);
}

uint8_t _fn_legacy_file_mkdir_fuji(void)
{
    uint16_t off;
    fn_raw_response_t raw;

    off = _fn_legacy_file_uri_prefix(
        _fn_legacy_appkey_transfer,
        k_fuji_dir,
        (uint16_t)(sizeof(k_fuji_dir) - 1));
    _fn_legacy_appkey_transfer[off++] = 0x03; /* parents + exist_ok */

    if (fn_raw_call(FN_DEVICE_FILE,
                    FN_CMD_FILE_MKDIR,
                    _fn_legacy_appkey_transfer,
                    off,
                    _fn_legacy_appkey_transfer,
                    4,
                    &raw) != FN_OK) {
        return FN_ERR_IO;
    }
    return raw.status;
}
