#include <string.h>

#include "fn_bbc_internal.h"
#include "fn_protocol.h"
#include "fujinet-nio.h"

static uint8_t wifi_status(uint8_t s)
{
    if (s == 0) return FN_OK;
    if (s == 2) return FN_ERR_INVALID;
    if (s == 4) return FN_ERR_NOT_READY;
    if (s == 8) return FN_ERR_UNSUPPORTED;
    return FN_ERR_IO;
}

static uint8_t wifi_call(uint8_t command, const uint8_t *request, uint16_t request_len,
                         uint8_t *reply, uint16_t reply_capacity, uint16_t *reply_len)
{
    uint8_t status = FN_ERR_INTERNAL;
    uint8_t result = fn_bbc_device_call_raw(FN_DEVICE_WIFI, command, request, request_len,
                                            reply, reply_capacity, &status, reply_len);
    return result == FN_OK ? wifi_status(status) : result;
}

static uint8_t get_u8(const uint8_t *p, uint16_t len, uint16_t *at, uint8_t *v)
{
    if (*at >= len) return 0;
    *v = p[(*at)++];
    return 1;
}

static uint8_t get_string(const uint8_t *p, uint16_t len, uint16_t *at, char *out, uint8_t cap)
{
    uint8_t n;
    if (!get_u8(p, len, at, &n) || n >= cap || (uint16_t)(*at + n) > len) return 0;
    memcpy(out, p + *at, n);
    out[n] = 0;
    *at = (uint16_t)(*at + n);
    return 1;
}

uint8_t fn_wifi_get_status(fn_wifi_status_t *s)
{
    uint8_t req[] = {FN_WIFI_PROTOCOL_VERSION}, reply[128], result;
    uint16_t len = 0, at = 12;
    if (!s) return FN_ERR_INVALID;
    result = wifi_call(FN_WIFI_CMD_GET_STATUS, req, 1, reply, sizeof(reply), &len);
    if (result != FN_OK) return result;
    if (len < 12 || reply[0] != FN_WIFI_PROTOCOL_VERSION) return FN_ERR_IO;
    memset(s, 0, sizeof(*s));
    s->link_state = reply[1]; s->configured_enabled = reply[2];
    s->bssid_valid = reply[3]; s->scan_supported = reply[4]; s->rssi = (int8_t)reply[5];
    memcpy(s->bssid.bytes, reply + 6, 6); s->bssid.valid = s->bssid_valid;
    if (!get_string(reply, len, &at, s->ip, sizeof(s->ip)) ||
        !get_string(reply, len, &at, s->subnet, sizeof(s->subnet)) ||
        !get_string(reply, len, &at, s->gateway, sizeof(s->gateway)) ||
        !get_string(reply, len, &at, s->dns, sizeof(s->dns))) return FN_ERR_IO;
    if (at + 3 <= len) {
        s->capabilities = (uint16_t)reply[at] | ((uint16_t)reply[at + 1] << 8);
        s->backend_kind = reply[at + 2];
        at = (uint16_t)(at + 3);
    }
    return at == len ? FN_OK : FN_ERR_IO;
}

uint8_t fn_wifi_get_config(fn_wifi_config_t *c)
{
    uint8_t req[] = {FN_WIFI_PROTOCOL_VERSION}, reply[96], result;
    uint16_t len = 0, at = 3;
    if (!c) return FN_ERR_INVALID;
    result = wifi_call(FN_WIFI_CMD_GET_CONFIG, req, 1, reply, sizeof(reply), &len);
    if (result != FN_OK) return result;
    if (len < 3 || reply[0] != FN_WIFI_PROTOCOL_VERSION) return FN_ERR_IO;
    memset(c, 0, sizeof(*c)); c->enabled = reply[1]; c->password_present = reply[2];
    if (!get_string(reply, len, &at, c->ssid, sizeof(c->ssid)) ||
        !get_string(reply, len, &at, c->bssid, sizeof(c->bssid)) || at != len) return FN_ERR_IO;
    return FN_OK;
}

uint8_t fn_wifi_set_config(const fn_wifi_config_update_t *u)
{
    uint8_t req[2 + 1 + FN_WIFI_MAX_SSID + 1 + FN_WIFI_MAX_BSSID + 1 + FN_WIFI_MAX_PASSWORD];
    const char *v[3]; const uint8_t f[3] = {FN_WIFI_SET_SSID, FN_WIFI_SET_BSSID, FN_WIFI_SET_PASSWORD};
    uint16_t at = 2, n; uint8_t i;
    if (!u || (u->fields & (uint8_t)~0x3Fu)) return FN_ERR_INVALID;
    req[0] = FN_WIFI_PROTOCOL_VERSION; req[1] = u->fields;
    if (u->fields & FN_WIFI_SET_ENABLED) req[at++] = u->enabled ? 1 : 0;
    v[0] = u->ssid; v[1] = u->bssid; v[2] = u->password;
    for (i = 0; i < 3; ++i) if (u->fields & f[i]) {
        if (!v[i]) return FN_ERR_INVALID;
        n = (uint16_t)strlen(v[i]);
        if ((i == 0 && n > FN_WIFI_MAX_SSID) || (i == 1 && n != 0 && n != FN_WIFI_MAX_BSSID) ||
            (i == 2 && n > FN_WIFI_MAX_PASSWORD) || n > 255) return FN_ERR_INVALID;
        req[at++] = (uint8_t)n; memcpy(req + at, v[i], n); at = (uint16_t)(at + n);
    }
    return wifi_call(FN_WIFI_CMD_SET_CONFIG, req, at, req, sizeof(req), 0);
}

uint8_t fn_wifi_scan(uint16_t offset, uint8_t limit, fn_wifi_scan_record_t *r,
                     uint8_t cap, uint8_t *count, uint8_t *more,
                     uint8_t *reply, uint16_t reply_capacity)
{
    uint8_t req[4], result, n, i, page_limit;
    uint16_t len, at, max_records;
    if (!r || !count || !more || !cap || !limit ||
        !reply || reply_capacity < (uint16_t)(3 + 1 + FN_WIFI_MAX_SSID + 9) ||
        limit > FN_WIFI_MAX_SCAN_RECORDS)
        return FN_ERR_INVALID;
    max_records = (uint16_t)((reply_capacity - 3) / (1 + FN_WIFI_MAX_SSID + 9));
    if (max_records > FN_WIFI_MAX_SCAN_RECORDS) max_records = FN_WIFI_MAX_SCAN_RECORDS;
    page_limit = limit;
    if ((uint16_t)page_limit > max_records) page_limit = (uint8_t)max_records;
    if (!page_limit) return FN_ERR_INVALID;
    req[0] = FN_WIFI_PROTOCOL_VERSION;
    req[1] = (uint8_t)offset;
    req[2] = (uint8_t)(offset >> 8);
    req[3] = page_limit;
    len = 0; at = 3;
    result = wifi_call(FN_WIFI_CMD_SCAN, req, 4, reply, reply_capacity, &len);
    if (result != FN_OK) return result;
    if (len < 3 || reply[0] != FN_WIFI_PROTOCOL_VERSION) return FN_ERR_IO;
    *more = reply[1]; n = reply[2]; if (n > cap || n > page_limit) return FN_ERR_INVALID;
    for (i = 0; i < n; ++i) {
        memset(&r[i], 0, sizeof(r[i]));
        if (!get_string(reply, len, &at, r[i].ssid, sizeof(r[i].ssid)) || at + 9 > len) return FN_ERR_IO;
        memcpy(r[i].bssid.bytes, reply + at, 6); r[i].bssid.valid = 1; at += 6;
        r[i].rssi = (int8_t)reply[at++]; r[i].channel = reply[at++]; r[i].auth = reply[at++];
    }
    *count = n; return at == len ? FN_OK : FN_ERR_IO;
}
