/**
 * @file fn_network.c
 * @brief FujiNet-NIO Network API Implementation
 * 
 * Main implementation of the network API functions.
 * Designed for CC65 compatibility (C99 with limited local variables).
 * 
 * @version 1.0.0
 */

#include "fujinet-nio.h"
#include "fn_protocol.h"
#include "fn_platform.h"
#include "fn_internal.h"
#include <string.h>

/* ============================================================================
 * Internal State
 * ============================================================================ */

/** Session tracking table */
static fn_session_t _sessions[FN_MAX_SESSIONS];

/** Library initialized flag */
static uint8_t _initialized = 0;

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

/**
 * Find a free session slot.
 */
static int8_t _find_free_slot(void)
{
    int8_t i;
    for (i = 0; i < FN_MAX_SESSIONS; i++) {
        if (!_sessions[i].active) {
            return i;
        }
    }
    return -1;
}

/**
 * Find session by handle.
 */
static int8_t _find_session(fn_handle_t handle)
{
    int8_t i;
    for (i = 0; i < FN_MAX_SESSIONS; i++) {
        if (_sessions[i].active && _sessions[i].handle == handle) {
            return i;
        }
    }
    return -1;
}

/**
 * Allocate a new handle.
 */
static fn_handle_t _alloc_handle(void)
{
    int8_t slot;
    
    slot = _find_free_slot();
    if (slot < 0) {
        return FN_INVALID_HANDLE;
    }
    
    _sessions[slot].active = 1;
    _sessions[slot].is_tcp = 0;
    _sessions[slot].needs_body = 0;
    _sessions[slot].write_offset = 0;
    _sessions[slot].read_offset = 0;
    
    return (fn_handle_t)(slot + 1);
}

/**
 * Free a handle.
 */
static void _free_handle(fn_handle_t handle)
{
    int8_t slot;
    
    if (handle == FN_INVALID_HANDLE) {
        return;
    }
    
    slot = _find_session(handle);
    if (slot >= 0) {
        _sessions[slot].active = 0;
    }
}

/* ============================================================================
 * Initialization
 * ============================================================================ */

uint8_t fn_init(void)
{
    uint8_t result;
    int8_t i;
    
    if (_initialized) {
        return FN_OK;
    }
    
    for (i = 0; i < FN_MAX_SESSIONS; i++) {
        _sessions[i].active = 0;
    }
    
    result = fn_transport_init();
    if (result != FN_OK) {
        return result;
    }
    
    _initialized = 1;
    return FN_OK;
}

uint8_t fn_is_ready(void)
{
    return fn_transport_ready();
}

/* ============================================================================
 * Network Operations
 * ============================================================================ */

/* Static buffers for CC65 compatibility (reduces stack usage) */
static uint8_t _req_buf[FN_MAX_PACKET_SIZE];
static uint8_t _resp_buf[FN_MAX_PACKET_SIZE];

uint8_t fn_open(fn_handle_t *handle, 
                uint8_t method,
                const char *url,
                uint8_t flags)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    uint8_t open_flags;
    int8_t slot;
    fn_handle_t resp_handle;
    uint8_t resp_flags;
    
    if (!_initialized) {
        return FN_ERR_INVALID;
    }
    
    if (handle == NULL || url == NULL) {
        return FN_ERR_INVALID;
    }
    
    if (strlen(url) > FN_MAX_URL_LEN) {
        return FN_ERR_URL_TOO_LONG;
    }
    
    open_flags = 0;
    if (flags & FN_OPEN_TLS) {
        open_flags |= FN_OPEN_FLAG_TLS;
    }
    if (flags & FN_OPEN_FOLLOW_REDIR) {
        open_flags |= FN_OPEN_FLAG_FOLLOW_REDIR;
    }
    if (flags & FN_OPEN_ALLOW_EVICT) {
        open_flags |= FN_OPEN_FLAG_ALLOW_EVICT;
    }
    
    req_len = fn_build_open_packet(_req_buf, method, open_flags, url);
    if (req_len == 0) {
        return FN_ERR_INVALID;
    }
    
    result = fn_transport_exchange(_req_buf, req_len, _resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }
    
    result = fn_parse_open_response(_resp_buf, resp_len, &resp_handle, &resp_flags);
    if (result != FN_OK) {
        return result;
    }
    
    /* Find a free session slot for tracking */
    slot = _find_free_slot();
    if (slot < 0) {
        /* No free slots - but device already allocated handle. Continue anyway. */
        *handle = resp_handle;
        return FN_OK;
    }
    
    /* Store the device-assigned handle and mark session active */
    *handle = resp_handle;
    _sessions[slot].active = 1;
    _sessions[slot].handle = resp_handle;
    _sessions[slot].is_tcp = 0;
    _sessions[slot].needs_body = 0;
    _sessions[slot].write_offset = 0;
    _sessions[slot].read_offset = 0;
    
    if (resp_flags & FN_OPEN_RESP_NEEDS_BODY) {
        _sessions[slot].needs_body = 1;
    }
    
    if (strncmp(url, "tcp://", 6) == 0) {
        _sessions[slot].is_tcp = 1;
    }
    
    return FN_OK;
}

/* Static buffer for TCP URL construction */
static char _tcp_url[FN_MAX_URL_LEN];

uint8_t fn_tcp_open(fn_handle_t *handle,
                    const char *host,
                    uint16_t port)
{
    uint8_t offset;
    uint16_t p;
    
    strcpy(_tcp_url, "tcp://");
    offset = 6;
    
    if (offset + strlen(host) > FN_MAX_URL_LEN - 10) {
        return FN_ERR_URL_TOO_LONG;
    }
    strcpy(_tcp_url + offset, host);
    offset += strlen(host);
    
    _tcp_url[offset++] = ':';
    
    /* Convert port to string */
    p = port;
    if (p >= 10000) {
        _tcp_url[offset++] = '0' + (p / 10000);
        p %= 10000;
    }
    if (p >= 1000) {
        _tcp_url[offset++] = '0' + (p / 1000);
        p %= 1000;
    }
    if (p >= 100) {
        _tcp_url[offset++] = '0' + (p / 100);
        p %= 100;
    }
    if (p >= 10) {
        _tcp_url[offset++] = '0' + (p / 10);
        p %= 10;
    }
    _tcp_url[offset++] = '0' + p;
    _tcp_url[offset] = '\0';
    
    return fn_open(handle, 0, _tcp_url, 0);
}

uint8_t fn_write(fn_handle_t handle,
                 uint32_t offset,
                 const uint8_t *data,
                 uint16_t len,
                 uint16_t *written)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    int8_t slot;
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    
    if (!_initialized) {
        return FN_ERR_INVALID;
    }
    
    if (handle == FN_INVALID_HANDLE) {
        return FN_ERR_INVALID;
    }
    
    slot = _find_session(handle);
    if (slot < 0) {
        return FN_ERR_NOT_FOUND;
    }
    
    if (offset != _sessions[slot].write_offset) {
        return FN_ERR_INVALID;
    }
    
    req_len = fn_build_write_packet(_req_buf, handle, offset, data, len);
    if (req_len == 0) {
        return FN_ERR_INVALID;
    }
    
    result = fn_transport_exchange(_req_buf, req_len, _resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }
    
    result = fn_parse_response_header(_resp_buf, resp_len, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    
    if (status != FN_OK) {
        return status;
    }
    
    if (data_len >= 12 && written != NULL) {
        *written = _resp_buf[data_offset + 10] | (_resp_buf[data_offset + 11] << 8);
        _sessions[slot].write_offset += *written;
    } else if (written != NULL) {
        *written = 0;
    }
    
    return FN_OK;
}

uint8_t fn_read(fn_handle_t handle,
                uint32_t offset,
                uint8_t *buf,
                uint16_t max_len,
                uint16_t *bytes_read,
                uint8_t *flags)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    int8_t slot;
    fn_handle_t resp_handle;
    uint32_t offset_echo;
    
    if (!_initialized) {
        return FN_ERR_INVALID;
    }
    
    if (handle == FN_INVALID_HANDLE || buf == NULL || bytes_read == NULL) {
        return FN_ERR_INVALID;
    }
    
    slot = _find_session(handle);
    if (slot < 0) {
        return FN_ERR_NOT_FOUND;
    }
    
    req_len = fn_build_read_packet(_req_buf, handle, offset, max_len);
    if (req_len == 0) {
        return FN_ERR_INVALID;
    }
    
    result = fn_transport_exchange(_req_buf, req_len, _resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }
    
    result = fn_parse_read_response(_resp_buf, resp_len, &resp_handle, &offset_echo, flags, buf, max_len, bytes_read);
    if (result != FN_OK) {
        return result;
    }
    
    if (_sessions[slot].is_tcp && *bytes_read > 0) {
        _sessions[slot].read_offset += *bytes_read;
    }
    
    return FN_OK;
}

uint8_t fn_info(fn_handle_t handle,
                uint16_t *http_status,
                uint32_t *content_length,
                uint8_t *flags)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    int8_t slot;
    fn_handle_t resp_handle;
    
    if (!_initialized) {
        return FN_ERR_INVALID;
    }
    
    if (handle == FN_INVALID_HANDLE) {
        return FN_ERR_INVALID;
    }
    
    slot = _find_session(handle);
    if (slot < 0) {
        return FN_ERR_NOT_FOUND;
    }
    
    req_len = fn_build_info_packet(_req_buf, handle);
    if (req_len == 0) {
        return FN_ERR_INVALID;
    }
    
    result = fn_transport_exchange(_req_buf, req_len, _resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }
    
    result = fn_parse_info_response(_resp_buf, resp_len, &resp_handle, http_status, content_length, flags);
    
    return result;
}

uint8_t fn_close(fn_handle_t handle)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    
    if (!_initialized) {
        return FN_ERR_INVALID;
    }
    
    if (handle == FN_INVALID_HANDLE) {
        return FN_ERR_INVALID;
    }
    
    req_len = fn_build_close_packet(_req_buf, handle);
    if (req_len == 0) {
        return FN_ERR_INVALID;
    }
    
    result = fn_transport_exchange(_req_buf, req_len, _resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    
    _free_handle(handle);
    
    return result;
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

const char *fn_error_string(uint8_t error)
{
    switch (error) {
        case FN_OK:               return "OK";
        case FN_ERR_INVALID:      return "Invalid parameter";
        case FN_ERR_BUSY:         return "Device busy";
        case FN_ERR_NOT_READY:    return "Not ready";
        case FN_ERR_IO:           return "I/O error";
        case FN_ERR_NO_MEMORY:    return "Out of memory";
        case FN_ERR_NOT_FOUND:    return "Not found";
        case FN_ERR_TIMEOUT:      return "Timeout";
        case FN_ERR_TRANSPORT:    return "Transport error";
        case FN_ERR_URL_TOO_LONG: return "URL too long";
        case FN_ERR_NO_HANDLES:   return "No free handles";
        default:                  return "Unknown error";
    }
}

const char *fn_version(void)
{
    return "1.0.0";
}
