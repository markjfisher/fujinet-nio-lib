#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "fn_internal.h"
#include "fn_platform.h"

#define FN_BBC_OSWORD78             0x78
#define FN_BBC_REASON_JSON_QUERY    0x00
#define FN_BBC_REASON_SET_BODY_LEN  0x01
#define FN_BBC_REASON_WRITE_DATA    0x02
#define FN_BBC_REASON_CONTENT_TYPE  0x03
#define FN_BBC_REASON_SET_OPEN_URL  0x04

#define FN_BBC_STATUS_OK            0x00
#define FN_BBC_STATUS_BAD_CALL      0x01
#define FN_BBC_STATUS_JSON_FAILED   0x02
#define FN_BBC_STATUS_BAD_CHANNEL   0x03

#define FN_BBC_DIRECT_URL_MAX       127u
#define FN_BBC_OSWORD_STR_MAX       512u
#define FN_BBC_SEEK_SET             0

static const char _fn_bbc_sentinel_url[] = "://";
static const char _fn_bbc_platform_name[] = "bbc";

uint8_t __fastcall__ fn_bbc_osword78(uint8_t *block);
unsigned char __fastcall__ fn_bbc_fd_getchannel(unsigned char fd);

static uint8_t fn_bbc_status_to_result(uint8_t status)
{
    switch (status) {
        case FN_BBC_STATUS_OK:
            return FN_OK;
        case FN_BBC_STATUS_BAD_CALL:
            return FN_ERR_INVALID;
        case FN_BBC_STATUS_JSON_FAILED:
            return FN_ERR_IO;
        case FN_BBC_STATUS_BAD_CHANNEL:
            return FN_ERR_NOT_FOUND;
        default:
            return FN_ERR_IO;
    }
}

static uint8_t fn_bbc_probe_rom(void)
{
    uint8_t block[16];

    memset(block, 0, sizeof(block));
    block[0] = FN_BBC_REASON_CONTENT_TYPE;
    block[1] = 0xFF;
    block[2] = 0;

    return fn_bbc_osword78(block) == FN_BBC_STATUS_OK;
}

static int fn_bbc_open_flags(uint8_t method)
{
    switch (method) {
        case 0:
        case FN_METHOD_POST:
            return O_RDWR;
        case FN_METHOD_GET:
            return O_RDONLY;
        case FN_METHOD_PUT:
            return O_WRONLY;
        default:
            return -1;
    }
}

static const char *fn_bbc_apply_tls_flag(const char *url, uint8_t flags)
{
    uint16_t len;

    if ((flags & FN_OPEN_TLS) == 0 || url == 0) {
        return url;
    }

    if (strncmp(url, "http://", 7) == 0) {
        len = (uint16_t)strlen(url + 7);
        if ((uint16_t)(len + 8) > FN_MAX_URL_LEN) {
            return 0;
        }
        memcpy(_fn_tcp_url, "https://", 8);
        memcpy(_fn_tcp_url + 8, url + 7, len + 1);
        return _fn_tcp_url;
    }

    if (strncmp(url, "tcp://", 6) == 0) {
        len = (uint16_t)strlen(url + 6);
        if ((uint16_t)(len + 6) > FN_MAX_URL_LEN) {
            return 0;
        }
        memcpy(_fn_tcp_url, "tls://", 6);
        memcpy(_fn_tcp_url + 6, url + 6, len + 1);
        return _fn_tcp_url;
    }

    return url;
}

static uint8_t fn_bbc_arm_open_url(const char *url, uint16_t len)
{
    uint8_t block[16];

    if (len > FN_BBC_OSWORD_STR_MAX) {
        return FN_ERR_URL_TOO_LONG;
    }

    memset(block, 0, sizeof(block));
    block[0] = FN_BBC_REASON_SET_OPEN_URL;
    block[2] = (uint8_t)((unsigned)url & 0xFFu);
    block[3] = (uint8_t)(((unsigned)url >> 8) & 0xFFu);
    block[4] = (uint8_t)(len & 0xFFu);
    block[5] = (uint8_t)(len >> 8);

    return fn_bbc_status_to_result(fn_bbc_osword78(block));
}

static uint8_t fn_bbc_channel_from_handle(fn_handle_t handle, uint8_t *channel)
{
    int8_t slot;

    slot = fn_find_session(handle);
    if (slot < 0 || channel == 0) {
        return FN_ERR_NOT_FOUND;
    }

    *channel = fn_bbc_fd_getchannel((unsigned char)handle);
    return FN_OK;
}

uint8_t fn_init(void)
{
    int8_t i;

    if (_fn_initialized) {
        return FN_OK;
    }

    if (!fn_bbc_probe_rom()) {
        return FN_ERR_NOT_FOUND;
    }

    for (i = 0; i < FN_MAX_SESSIONS; ++i) {
        _fn_sessions[i].active = 0;
        _fn_sessions[i].handle = FN_INVALID_HANDLE;
        _fn_sessions[i].read_offset = 0;
        _fn_sessions[i].write_offset = 0;
        _fn_sessions[i].proto_flags = 0;
        _fn_sessions[i].needs_body = 0;
        _fn_sessions[i].reserved = 0;
    }

    _fn_initialized = 1;
    return FN_OK;
}

uint8_t fn_is_ready(void)
{
    return fn_bbc_probe_rom();
}

uint8_t fn_open(fn_handle_t *handle,
                uint8_t method,
                const char *url,
                uint8_t flags)
{
    int fd;
    int mode;
    int8_t slot;
    uint16_t url_len;
    const char *open_url;

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (handle == 0 || url == 0) {
        return FN_ERR_INVALID;
    }

    mode = fn_bbc_open_flags(method);
    if (mode < 0) {
        return FN_ERR_UNSUPPORTED;
    }

    open_url = fn_bbc_apply_tls_flag(url, flags);
    if (open_url == 0) {
        return FN_ERR_URL_TOO_LONG;
    }

    url_len = (uint16_t)strlen(open_url);
    if (url_len > FN_MAX_URL_LEN) {
        return FN_ERR_URL_TOO_LONG;
    }

    if (url_len > FN_BBC_DIRECT_URL_MAX) {
        if (fn_bbc_arm_open_url(open_url, url_len) != FN_OK) {
            return FN_ERR_INVALID;
        }
        fd = open(_fn_bbc_sentinel_url, mode);
    } else {
        fd = open(open_url, mode);
    }

    if (fd < 0) {
        return FN_ERR_IO;
    }

    slot = fn_find_free_slot();
    if (slot < 0) {
        close(fd);
        return FN_ERR_NO_HANDLES;
    }

    *handle = (fn_handle_t)fd;
    _fn_sessions[slot].active = 1;
    _fn_sessions[slot].handle = (fn_handle_t)fd;
    _fn_sessions[slot].read_offset = 0;
    _fn_sessions[slot].write_offset = 0;
    _fn_sessions[slot].proto_flags = 0;
    _fn_sessions[slot].needs_body = 0;
    _fn_sessions[slot].reserved = 0;
    return FN_OK;
}

uint8_t fn_tcp_open(fn_handle_t *handle,
                    const char *host,
                    uint16_t port)
{
    uint8_t offset;
    uint16_t p;

    if (handle == 0 || host == 0) {
        return FN_ERR_INVALID;
    }

    strcpy(_fn_tcp_url, "tcp://");
    offset = 6;

    if ((uint16_t)(offset + strlen(host)) > FN_MAX_URL_LEN - 10) {
        return FN_ERR_URL_TOO_LONG;
    }

    strcpy(_fn_tcp_url + offset, host);
    offset += (uint8_t)strlen(host);
    _fn_tcp_url[offset++] = ':';

    p = port;
    if (p >= 10000) {
        _fn_tcp_url[offset++] = (char)('0' + (p / 10000));
        p %= 10000;
    }
    if (p >= 1000) {
        _fn_tcp_url[offset++] = (char)('0' + (p / 1000));
        p %= 1000;
    }
    if (p >= 100) {
        _fn_tcp_url[offset++] = (char)('0' + (p / 100));
        p %= 100;
    }
    if (p >= 10) {
        _fn_tcp_url[offset++] = (char)('0' + (p / 10));
        p %= 10;
    }
    _fn_tcp_url[offset++] = (char)('0' + p);
    _fn_tcp_url[offset] = '\0';

    return fn_open(handle, 0, _fn_tcp_url, 0);
}

uint8_t fn_read(fn_handle_t handle,
                uint32_t offset,
                uint8_t *buf,
                uint16_t max_len,
                uint16_t *bytes_read,
                uint8_t *flags)
{
    int8_t slot;
    int rc;

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (handle == FN_INVALID_HANDLE || buf == 0 || bytes_read == 0) {
        return FN_ERR_INVALID;
    }

    slot = fn_find_session(handle);
    if (slot < 0) {
        return FN_ERR_NOT_FOUND;
    }

    if (_fn_sessions[slot].read_offset != offset) {
        if (lseek((int)handle, (long)offset, FN_BBC_SEEK_SET) < 0) {
            return FN_ERR_INVALID;
        }
        _fn_sessions[slot].read_offset = offset;
    }

    rc = read((int)handle, buf, max_len);
    if (rc < 0) {
        return FN_ERR_IO;
    }

    *bytes_read = (uint16_t)rc;
    if (flags != 0) {
        *flags = (rc == 0) ? FN_READ_EOF : 0;
    }

    _fn_sessions[slot].read_offset += (uint32_t)(uint16_t)rc;
    return FN_OK;
}

uint8_t fn_write(fn_handle_t handle,
                 uint32_t offset,
                 const uint8_t *data,
                 uint16_t len,
                 uint16_t *written)
{
    int8_t slot;
    uint8_t block[16];
    uint8_t channel;
    uint16_t chunk;
    uint16_t total;
    uint8_t result;

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (handle == FN_INVALID_HANDLE) {
        return FN_ERR_INVALID;
    }

    slot = fn_find_session(handle);
    if (slot < 0) {
        return FN_ERR_NOT_FOUND;
    }

    if (offset != _fn_sessions[slot].write_offset) {
        return FN_ERR_INVALID;
    }

    if (written != 0) {
        *written = 0;
    }

    if (len == 0) {
        return FN_OK;
    }

    if (data == 0) {
        return FN_ERR_INVALID;
    }

    result = fn_bbc_channel_from_handle(handle, &channel);
    if (result != FN_OK) {
        return result;
    }

    total = 0;
    while (total < len) {
        chunk = (uint16_t)(len - total);
        if (chunk > FN_BBC_OSWORD_STR_MAX) {
            chunk = FN_BBC_OSWORD_STR_MAX;
        }

        memset(block, 0, sizeof(block));
        block[0] = FN_BBC_REASON_WRITE_DATA;
        block[2] = (uint8_t)(((unsigned)(data + total)) & 0xFFu);
        block[3] = (uint8_t)((((unsigned)(data + total)) >> 8) & 0xFFu);
        block[4] = (uint8_t)(chunk & 0xFFu);
        block[5] = (uint8_t)(chunk >> 8);
        block[6] = channel;

        result = fn_bbc_status_to_result(fn_bbc_osword78(block));
        if (result != FN_OK) {
            return result;
        }

        total += chunk;
    }

    _fn_sessions[slot].write_offset += total;
    if (written != 0) {
        *written = total;
    }
    return FN_OK;
}

uint8_t fn_info(fn_handle_t handle,
                uint16_t *http_status,
                uint32_t *content_length,
                uint8_t *flags)
{
    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (fn_find_session(handle) < 0) {
        return FN_ERR_NOT_FOUND;
    }

    if (http_status != 0) {
        *http_status = 0;
    }
    if (content_length != 0) {
        *content_length = 0;
    }
    if (flags != 0) {
        *flags = FN_INFO_CONNECTED;
    }

    return FN_OK;
}

uint8_t fn_close(fn_handle_t handle)
{
    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (handle == FN_INVALID_HANDLE) {
        return FN_ERR_INVALID;
    }

    if (fn_find_session(handle) < 0) {
        return FN_ERR_NOT_FOUND;
    }

    if (close((int)handle) != 0) {
        return FN_ERR_IO;
    }

    fn_free_handle(handle);
    return FN_OK;
}

uint8_t fn_set_body_length(uint16_t len)
{
    uint8_t block[16];

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    memset(block, 0, sizeof(block));
    block[0] = FN_BBC_REASON_SET_BODY_LEN;
    block[2] = (uint8_t)(len & 0xFFu);
    block[3] = (uint8_t)(len >> 8);

    return fn_bbc_status_to_result(fn_bbc_osword78(block));
}

uint8_t fn_set_content_profile(uint8_t profile)
{
    uint8_t block[16];

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    memset(block, 0, sizeof(block));
    block[0] = FN_BBC_REASON_CONTENT_TYPE;
    block[2] = profile;

    return fn_bbc_status_to_result(fn_bbc_osword78(block));
}

uint8_t fn_json_query(fn_handle_t handle, const char *path)
{
    uint8_t block[16];
    uint8_t channel;
    uint16_t len;
    uint8_t result;

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (path == 0) {
        return FN_ERR_INVALID;
    }

    len = (uint16_t)strlen(path);
    if (len > FN_BBC_OSWORD_STR_MAX) {
        return FN_ERR_URL_TOO_LONG;
    }

    result = fn_bbc_channel_from_handle(handle, &channel);
    if (result != FN_OK) {
        return result;
    }

    memset(block, 0, sizeof(block));
    block[0] = FN_BBC_REASON_JSON_QUERY;
    block[2] = (uint8_t)((unsigned)path & 0xFFu);
    block[3] = (uint8_t)(((unsigned)path >> 8) & 0xFFu);
    block[4] = (uint8_t)(len & 0xFFu);
    block[5] = (uint8_t)(len >> 8);
    block[6] = channel;

    return fn_bbc_status_to_result(fn_bbc_osword78(block));
}

const char *fn_platform_name(void)
{
    return _fn_bbc_platform_name;
}
