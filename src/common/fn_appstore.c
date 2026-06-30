#include <string.h>

#include "fujinet-nio.h"
#include "fn_protocol.h"
#include "fn_raw.h"

enum {
    FN_FILEPROTO_VERSION = 1,
    FN_APPSTORE_PREFIX_MAX = 1 + 2 + 255 + 2 + 255
};

static uint8_t app_req_buf[FN_MAX_PACKET_SIZE - FN_HEADER_SIZE];
static uint8_t app_resp_buf[FN_MAX_PACKET_SIZE];

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

static uint8_t map_status(uint8_t status)
{
    switch (status) {
        case 0: return FN_OK;
        case 1: return FN_ERR_NOT_FOUND;
        case 2: return FN_ERR_INVALID;
        case 3: return FN_ERR_BUSY;
        case 4: return FN_ERR_NOT_READY;
        case 5: return FN_ERR_IO;
        case 6: return FN_ERR_TIMEOUT;
        case 7: return FN_ERR_INTERNAL;
        case 8: return FN_ERR_UNSUPPORTED;
        default: return FN_ERR_UNKNOWN;
    }
}

static uint8_t check_name_len(const char *s, uint16_t *len, uint8_t allow_empty)
{
    size_t n;

    if (s == 0) {
        return FN_ERR_INVALID;
    }

    n = strlen(s);
    if ((!allow_empty && n == 0) || n > 255) {
        return FN_ERR_INVALID;
    }

    *len = (uint16_t)n;
    return FN_OK;
}

static uint8_t build_prefix(uint8_t *buf,
                            uint16_t *off,
                            const char *namespace_name,
                            const char *key,
                            uint8_t key_required)
{
    uint16_t ns_len;
    uint16_t key_len;
    uint8_t result;

    result = check_name_len(namespace_name, &ns_len, 0);
    if (result != FN_OK) {
        return result;
    }
    result = check_name_len(key ? key : "", &key_len, !key_required);
    if (result != FN_OK) {
        return result;
    }
    if (key_required && key_len == 0) {
        return FN_ERR_INVALID;
    }

    *off = 0;
    buf[(*off)++] = FN_FILEPROTO_VERSION;
    put_u16le(&buf[*off], ns_len);
    *off = (uint16_t)(*off + 2);
    memcpy(&buf[*off], namespace_name, ns_len);
    *off = (uint16_t)(*off + ns_len);
    put_u16le(&buf[*off], key_len);
    *off = (uint16_t)(*off + 2);
    if (key_len != 0) {
        memcpy(&buf[*off], key, key_len);
        *off = (uint16_t)(*off + key_len);
    }
    return FN_OK;
}

static uint8_t appstore_call(uint8_t command,
                             const uint8_t *request,
                             uint16_t request_len,
                             uint8_t *response_buf,
                             uint16_t response_capacity,
                             fn_raw_response_t *raw)
{
    uint8_t result;

    result = fn_raw_call(FN_DEVICE_FILE,
                         command,
                         request,
                         request_len,
                         response_buf,
                         response_capacity,
                         raw);
    if (result != FN_OK) {
        return result;
    }
    return map_status(raw->status);
}

uint8_t fn_appstore_stat(const char *namespace_name,
                         const char *key,
                         fn_appstore_stat_t *out)
{
    uint16_t off;
    fn_raw_response_t raw;
    uint8_t result;

    if (out == 0) {
        return FN_ERR_INVALID;
    }
    out->exists = 0;
    out->size_bytes = 0;
    out->size_bytes_high = 0;
    out->mtime_unix = 0;
    out->mtime_unix_high = 0;

    result = build_prefix(app_req_buf, &off, namespace_name, key, 1);
    if (result != FN_OK) {
        return result;
    }

    result = appstore_call(FN_CMD_APPSTORE_STAT, app_req_buf, off, app_resp_buf, sizeof(app_resp_buf), &raw);
    if (result != FN_OK) {
        return result;
    }
    if (raw.payload_length < 20 || app_resp_buf[0] != FN_FILEPROTO_VERSION) {
        return FN_ERR_IO;
    }

    out->exists = (uint8_t)((app_resp_buf[1] & 0x01) ? 1 : 0);
    out->size_bytes = get_u32le(&app_resp_buf[4]);
    out->size_bytes_high = get_u32le(&app_resp_buf[8]);
    out->mtime_unix = get_u32le(&app_resp_buf[12]);
    out->mtime_unix_high = get_u32le(&app_resp_buf[16]);
    return FN_OK;
}

uint8_t fn_appstore_read(const char *namespace_name,
                         const char *key,
                         uint32_t offset,
                         uint8_t *buf,
                         uint16_t max_len,
                         fn_appstore_read_t *out)
{
    uint16_t off;
    uint16_t data_len;
    fn_raw_response_t raw;
    uint8_t result;

    if (out == 0 || (max_len != 0 && buf == 0)) {
        return FN_ERR_INVALID;
    }
    if (max_len == 0 || max_len > (uint16_t)(FN_MAX_PACKET_SIZE - 10)) {
        return FN_ERR_INVALID;
    }
    out->flags = 0;
    out->offset = offset;
    out->bytes_read = 0;

    result = build_prefix(app_req_buf, &off, namespace_name, key, 1);
    if (result != FN_OK) {
        return result;
    }
    put_u32le(&app_req_buf[off], offset);
    off = (uint16_t)(off + 4);
    put_u16le(&app_req_buf[off], max_len);
    off = (uint16_t)(off + 2);

    result = appstore_call(FN_CMD_APPSTORE_READ, app_req_buf, off, app_resp_buf, sizeof(app_resp_buf), &raw);
    if (result != FN_OK) {
        return result;
    }
    if (raw.payload_length < 10 || app_resp_buf[0] != FN_FILEPROTO_VERSION) {
        return FN_ERR_IO;
    }

    data_len = get_u16le(&app_resp_buf[8]);
    if ((uint16_t)(10 + data_len) > raw.payload_length || data_len > max_len) {
        return FN_ERR_IO;
    }

    if (data_len != 0) {
        memcpy(buf, &app_resp_buf[10], data_len);
    }
    out->flags = app_resp_buf[1];
    out->offset = get_u32le(&app_resp_buf[4]);
    out->bytes_read = data_len;
    return FN_OK;
}

uint8_t fn_appstore_write(const char *namespace_name,
                          const char *key,
                          uint32_t offset,
                          const uint8_t *data,
                          uint16_t len,
                          fn_appstore_write_t *out)
{
    uint16_t off;
    fn_raw_response_t raw;
    uint8_t result;

    if (out == 0 || (len != 0 && data == 0)) {
        return FN_ERR_INVALID;
    }
    out->offset = offset;
    out->bytes_written = 0;

    result = build_prefix(app_req_buf, &off, namespace_name, key, 1);
    if (result != FN_OK) {
        return result;
    }
    if ((uint16_t)(off + 6 + len) > sizeof(app_req_buf)) {
        return FN_ERR_INVALID;
    }
    put_u32le(&app_req_buf[off], offset);
    off = (uint16_t)(off + 4);
    put_u16le(&app_req_buf[off], len);
    off = (uint16_t)(off + 2);
    if (len != 0) {
        memcpy(&app_req_buf[off], data, len);
        off = (uint16_t)(off + len);
    }

    result = appstore_call(FN_CMD_APPSTORE_WRITE, app_req_buf, off, app_resp_buf, sizeof(app_resp_buf), &raw);
    if (result != FN_OK) {
        return result;
    }
    if (raw.payload_length < 10 || app_resp_buf[0] != FN_FILEPROTO_VERSION) {
        return FN_ERR_IO;
    }

    out->offset = get_u32le(&app_resp_buf[4]);
    out->bytes_written = get_u16le(&app_resp_buf[8]);
    return FN_OK;
}

uint8_t fn_appstore_delete(const char *namespace_name,
                           const char *key,
                           fn_appstore_delete_t *out)
{
    uint16_t off;
    fn_raw_response_t raw;
    uint8_t result;

    if (out == 0) {
        return FN_ERR_INVALID;
    }
    out->deleted = 0;

    result = build_prefix(app_req_buf, &off, namespace_name, key, 1);
    if (result != FN_OK) {
        return result;
    }

    result = appstore_call(FN_CMD_APPSTORE_DELETE, app_req_buf, off, app_resp_buf, sizeof(app_resp_buf), &raw);
    if (result != FN_OK) {
        return result;
    }
    if (raw.payload_length < 4 || app_resp_buf[0] != FN_FILEPROTO_VERSION) {
        return FN_ERR_IO;
    }

    out->deleted = (uint8_t)((app_resp_buf[1] & 0x01) ? 1 : 0);
    return FN_OK;
}

uint8_t fn_appstore_list(const char *namespace_name,
                         uint16_t start_index,
                         uint8_t *key_data,
                         uint16_t key_data_capacity,
                         fn_appstore_list_t *out)
{
    uint16_t off;
    uint16_t key_data_len;
    fn_raw_response_t raw;
    uint8_t result;

    if (out == 0 || key_data == 0 || key_data_capacity == 0 ||
        key_data_capacity > (uint16_t)(FN_MAX_PACKET_SIZE - 10)) {
        return FN_ERR_INVALID;
    }
    out->flags = 0;
    out->start_index = start_index;
    out->key_count = 0;
    out->key_data_len = 0;

    result = build_prefix(app_req_buf, &off, namespace_name, "", 0);
    if (result != FN_OK) {
        return result;
    }
    put_u16le(&app_req_buf[off], start_index);
    off = (uint16_t)(off + 2);
    put_u16le(&app_req_buf[off], key_data_capacity);
    off = (uint16_t)(off + 2);

    result = appstore_call(FN_CMD_APPSTORE_LIST, app_req_buf, off, app_resp_buf, sizeof(app_resp_buf), &raw);
    if (result != FN_OK) {
        return result;
    }
    if (raw.payload_length < 10 || app_resp_buf[0] != FN_FILEPROTO_VERSION) {
        return FN_ERR_IO;
    }

    key_data_len = get_u16le(&app_resp_buf[8]);
    if ((uint16_t)(10 + key_data_len) > raw.payload_length || key_data_len > key_data_capacity) {
        return FN_ERR_IO;
    }
    if (key_data_len != 0) {
        memcpy(key_data, &app_resp_buf[10], key_data_len);
    }

    out->flags = app_resp_buf[1];
    out->start_index = get_u16le(&app_resp_buf[4]);
    out->key_count = get_u16le(&app_resp_buf[6]);
    out->key_data_len = key_data_len;
    return FN_OK;
}

uint8_t fn_appstore_list_next_key(const uint8_t *key_data,
                                  uint16_t key_data_len,
                                  uint16_t *offset,
                                  char *key_out,
                                  uint16_t key_out_capacity)
{
    uint16_t off;
    uint16_t key_len;

    if (key_data == 0 || offset == 0 || key_out == 0 || key_out_capacity == 0) {
        return FN_ERR_INVALID;
    }
    off = *offset;
    if (off == key_data_len) {
        return FN_ERR_NOT_READY;
    }
    if ((uint16_t)(off + 2) > key_data_len) {
        return FN_ERR_IO;
    }
    key_len = get_u16le(&key_data[off]);
    off = (uint16_t)(off + 2);
    if ((uint16_t)(off + key_len) > key_data_len || key_len >= key_out_capacity) {
        return FN_ERR_IO;
    }

    memcpy(key_out, &key_data[off], key_len);
    key_out[key_len] = 0;
    off = (uint16_t)(off + key_len);
    *offset = off;
    return FN_OK;
}
