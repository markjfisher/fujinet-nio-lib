/**
 * @file fn_clock.c
 * @brief FujiNet-NIO Clock API Implementation
 * 
 * Implementation of clock device functions.
 * Designed for CC65 compatibility (C99 with limited local variables).
 * 
 * @version 1.0.0
 */

#include "fujinet-nio.h"
#include "fn_protocol.h"
#include "fn_platform.h"
#include "fn_internal.h"

/* ============================================================================
 * Static Buffers (for cc65 compatibility - no large stack buffers)
 * ============================================================================ */

static uint8_t _clock_req_buf[FN_MAX_PACKET_SIZE];
static uint8_t _clock_resp_buf[FN_MAX_PACKET_SIZE];

/* ============================================================================
 * Clock Operations
 * ============================================================================ */

/**
 * @brief Get the current time from the FujiNet device.
 * 
 * Response payload format (v1):
 *   u8  version
 *   u8  flags (reserved, 0 for now)
 *   u16 reserved (LE, 0)
 *   u64 unix_seconds (LE)
 */
uint8_t fn_clock_get(FN_TIME_T *time)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t version;
    uint8_t i;
    
    if (time == NULL) {
        return FN_ERR_INVALID;
    }
    
    /* Build request packet header (no payload needed for GetTime) */
    req_len = fn_build_header(_clock_req_buf, FN_DEVICE_CLOCK, FN_CMD_CLOCK_GET, FN_HEADER_SIZE);
    if (req_len == 0) {
        return FN_ERR_INTERNAL;
    }
    
    /* Finalize checksum */
    _clock_req_buf[4] = fn_calc_checksum(_clock_req_buf, req_len);
    
    /* Send request and receive response */
    result = fn_transport_exchange(_clock_req_buf, req_len, _clock_resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }
    
    /* Parse response header */
    result = fn_parse_response_header(_clock_resp_buf, resp_len, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    
    if (status != FN_OK) {
        return status;
    }
    
    /* Check minimum payload size: version(1) + flags(1) + reserved(2) + time(8) = 12 bytes */
    if (data_len < 12) {
        return FN_ERR_INVALID;
    }
    
    /* Parse time payload */
    version = _clock_resp_buf[data_offset];
    if (version != FN_CLOCK_VERSION) {
        return FN_ERR_UNSUPPORTED;
    }
    
    /* Copy 8-byte timestamp to output (little-endian) */
#ifdef __CC65__
    for (i = 0; i < 8; i++) {
        time->b[i] = _clock_resp_buf[data_offset + 4 + i];
    }
#else
    *time = 0;
    for (i = 0; i < 8; i++) {
        *time |= ((uint64_t)_clock_resp_buf[data_offset + 4 + i]) << (8 * i);
    }
#endif
    
    return FN_OK;
}

/**
 * @brief Set the time on the FujiNet device.
 * 
 * Request payload format (v1):
 *   u8  version
 *   u64 unix_seconds (LE)
 * 
 * Response payload format (v1):
 *   Same as GetTime response
 */
uint8_t fn_clock_set(const FN_TIME_T *time)
{
    uint16_t offset;
    uint16_t payload_len;
    uint16_t total_len;
    uint16_t resp_len;
    uint8_t result;
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t checksum;
    uint8_t i;
    
    if (time == NULL) {
        return FN_ERR_INVALID;
    }
    
    /* Payload: version(1) + time(8) = 9 bytes */
    payload_len = 9;
    total_len = FN_HEADER_SIZE + payload_len;
    
    /* Build header */
    offset = fn_build_header(_clock_req_buf, FN_DEVICE_CLOCK, FN_CMD_CLOCK_SET, total_len);
    
    /* Version */
    _clock_req_buf[offset++] = FN_CLOCK_VERSION;
    
    /* Unix timestamp (little-endian) */
#ifdef __CC65__
    for (i = 0; i < 8; i++) {
        _clock_req_buf[offset++] = time->b[i];
    }
#else
    for (i = 0; i < 8; i++) {
        _clock_req_buf[offset++] = (uint8_t)((*time >> (8 * i)) & 0xFF);
    }
#endif
    
    /* Calculate and insert checksum */
    checksum = fn_calc_checksum(_clock_req_buf, offset);
    _clock_req_buf[4] = checksum;
    
    /* Send request and receive response */
    result = fn_transport_exchange(_clock_req_buf, offset, _clock_resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }
    
    /* Parse response header */
    result = fn_parse_response_header(_clock_resp_buf, resp_len, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    
    return status;
}

/**
 * @brief Get the current time in a specific format.
 * 
 * Request payload format:
 *   u8  version
 *   u8  format
 *   [optional: u8 tz_len + char[] timezone]
 * 
 * Response payload format:
 *   u8  version
 *   u8  format (echo)
 *   u8[] formatted_time
 */
uint8_t fn_clock_get_format(uint8_t *time_data, FnTimeFormat format)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t version;
    uint8_t resp_format;
    uint16_t i;
    
    if (time_data == NULL) {
        return FN_ERR_INVALID;
    }
    
    /* Build request packet: version(1) + format(1) = 2 bytes payload */
    req_len = fn_build_header(_clock_req_buf, FN_DEVICE_CLOCK, FN_CMD_CLOCK_GET_FORMAT, FN_HEADER_SIZE + 2);
    if (req_len == 0) {
        return FN_ERR_INTERNAL;
    }
    
    /* Add payload */
    _clock_req_buf[req_len++] = FN_CLOCK_VERSION;
    _clock_req_buf[req_len++] = (uint8_t)format;
    
    /* Finalize checksum */
    _clock_req_buf[4] = fn_calc_checksum(_clock_req_buf, req_len);
    
    /* Send request and receive response */
    result = fn_transport_exchange(_clock_req_buf, req_len, _clock_resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }
    
    /* Parse response header */
    result = fn_parse_response_header(_clock_resp_buf, resp_len, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    
    if (status != FN_OK) {
        return status;
    }
    
    /* Check minimum payload size: version(1) + format(1) = 2 bytes */
    if (data_len < 2) {
        return FN_ERR_INVALID;
    }
    
    /* Parse response */
    version = _clock_resp_buf[data_offset];
    resp_format = _clock_resp_buf[data_offset + 1];  /* Echo of requested format */
    (void)resp_format;  /* Unused but kept for protocol documentation */
    
    if (version != FN_CLOCK_VERSION) {
        return FN_ERR_UNSUPPORTED;
    }
    
    /* Copy formatted time to output (skip version and format bytes) */
    for (i = 0; i < data_len - 2; i++) {
        time_data[i] = _clock_resp_buf[data_offset + 2 + i];
    }
    
    return FN_OK;
}

/**
 * @brief Get the current time for a specific timezone.
 */
uint8_t fn_clock_get_tz(uint8_t *time_data, const char *tz, FnTimeFormat format)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t version;
    uint8_t tz_len;
    uint16_t i;
    
    if (time_data == NULL || tz == NULL) {
        return FN_ERR_INVALID;
    }
    
    /* Calculate timezone length */
    tz_len = 0;
    while (tz[tz_len] != '\0' && tz_len < FN_MAX_TIMEZONE_LEN) {
        tz_len++;
    }
    
    /* Build request packet: version(1) + format(1) + tz_len(1) + tz(n) */
    req_len = fn_build_header(_clock_req_buf, FN_DEVICE_CLOCK, FN_CMD_CLOCK_GET_FORMAT, FN_HEADER_SIZE + 3 + tz_len);
    if (req_len == 0) {
        return FN_ERR_INTERNAL;
    }
    
    /* Add payload */
    _clock_req_buf[req_len++] = FN_CLOCK_VERSION;
    _clock_req_buf[req_len++] = (uint8_t)format;
    _clock_req_buf[req_len++] = tz_len;
    for (i = 0; i < tz_len; i++) {
        _clock_req_buf[req_len++] = (uint8_t)tz[i];
    }
    
    /* Finalize checksum */
    _clock_req_buf[4] = fn_calc_checksum(_clock_req_buf, req_len);
    
    /* Send request and receive response */
    result = fn_transport_exchange(_clock_req_buf, req_len, _clock_resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }
    
    /* Parse response header */
    result = fn_parse_response_header(_clock_resp_buf, resp_len, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    
    if (status != FN_OK) {
        return status;
    }
    
    /* Check minimum payload size */
    if (data_len < 2) {
        return FN_ERR_INVALID;
    }
    
    /* Parse response */
    version = _clock_resp_buf[data_offset];
    if (version != FN_CLOCK_VERSION) {
        return FN_ERR_UNSUPPORTED;
    }
    
    /* Copy formatted time to output (skip version and format bytes) */
    for (i = 0; i < data_len - 2; i++) {
        time_data[i] = _clock_resp_buf[data_offset + 2 + i];
    }
    
    return FN_OK;
}

/**
 * @brief Get the current timezone string.
 * 
 * Response payload format:
 *   u8  version
 *   u8  length
 *   char[] timezone_string (null-terminated)
 */
uint8_t fn_clock_get_timezone(char *tz)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t version;
    uint8_t tz_len;
    uint16_t i;
    
    if (tz == NULL) {
        return FN_ERR_INVALID;
    }
    
    /* Build request packet header (no payload needed) */
    req_len = fn_build_header(_clock_req_buf, FN_DEVICE_CLOCK, FN_CMD_CLOCK_GET_TZ, FN_HEADER_SIZE);
    if (req_len == 0) {
        return FN_ERR_INTERNAL;
    }
    
    /* Finalize checksum */
    _clock_req_buf[4] = fn_calc_checksum(_clock_req_buf, req_len);
    
    /* Send request and receive response */
    result = fn_transport_exchange(_clock_req_buf, req_len, _clock_resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }
    
    /* Parse response header */
    result = fn_parse_response_header(_clock_resp_buf, resp_len, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    
    if (status != FN_OK) {
        return status;
    }
    
    /* Check minimum payload size: version(1) + length(1) = 2 bytes */
    if (data_len < 2) {
        return FN_ERR_INVALID;
    }
    
    /* Parse response */
    version = _clock_resp_buf[data_offset];
    tz_len = _clock_resp_buf[data_offset + 1];
    
    if (version != FN_CLOCK_VERSION) {
        return FN_ERR_UNSUPPORTED;
    }
    
    /* Copy timezone string to output */
    for (i = 0; i < tz_len && i < FN_MAX_TIMEZONE_LEN - 1; i++) {
        tz[i] = (char)_clock_resp_buf[data_offset + 2 + i];
    }
    tz[i] = '\0';
    
    return FN_OK;
}

/**
 * @brief Set the timezone (non-persistent).
 * 
 * Request payload format:
 *   u8  version
 *   u8  length
 *   char[] timezone_string
 */
uint8_t fn_clock_set_timezone(const char *tz)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t tz_len;
    uint16_t i;
    
    if (tz == NULL) {
        return FN_ERR_INVALID;
    }
    
    /* Calculate timezone length */
    tz_len = 0;
    while (tz[tz_len] != '\0' && tz_len < FN_MAX_TIMEZONE_LEN) {
        tz_len++;
    }
    
    /* Build request packet: version(1) + length(1) + tz(n) */
    req_len = fn_build_header(_clock_req_buf, FN_DEVICE_CLOCK, FN_CMD_CLOCK_SET_TZ, FN_HEADER_SIZE + 2 + tz_len);
    if (req_len == 0) {
        return FN_ERR_INTERNAL;
    }
    
    /* Add payload */
    _clock_req_buf[req_len++] = FN_CLOCK_VERSION;
    _clock_req_buf[req_len++] = tz_len;
    for (i = 0; i < tz_len; i++) {
        _clock_req_buf[req_len++] = (uint8_t)tz[i];
    }
    
    /* Finalize checksum */
    _clock_req_buf[4] = fn_calc_checksum(_clock_req_buf, req_len);
    
    /* Send request and receive response */
    result = fn_transport_exchange(_clock_req_buf, req_len, _clock_resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }
    
    /* Parse response header */
    result = fn_parse_response_header(_clock_resp_buf, resp_len, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    
    return status;
}

/**
 * @brief Set the timezone and persist to configuration.
 */
uint8_t fn_clock_set_timezone_save(const char *tz)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t tz_len;
    uint16_t i;
    
    if (tz == NULL) {
        return FN_ERR_INVALID;
    }
    
    /* Calculate timezone length */
    tz_len = 0;
    while (tz[tz_len] != '\0' && tz_len < FN_MAX_TIMEZONE_LEN) {
        tz_len++;
    }
    
    /* Build request packet: version(1) + length(1) + tz(n) */
    req_len = fn_build_header(_clock_req_buf, FN_DEVICE_CLOCK, FN_CMD_CLOCK_SET_TZ_SAVE, FN_HEADER_SIZE + 2 + tz_len);
    if (req_len == 0) {
        return FN_ERR_INTERNAL;
    }
    
    /* Add payload */
    _clock_req_buf[req_len++] = FN_CLOCK_VERSION;
    _clock_req_buf[req_len++] = tz_len;
    for (i = 0; i < tz_len; i++) {
        _clock_req_buf[req_len++] = (uint8_t)tz[i];
    }
    
    /* Finalize checksum */
    _clock_req_buf[4] = fn_calc_checksum(_clock_req_buf, req_len);
    
    /* Send request and receive response */
    result = fn_transport_exchange(_clock_req_buf, req_len, _clock_resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }
    
    /* Parse response header */
    result = fn_parse_response_header(_clock_resp_buf, resp_len, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    
    return status;
}

/**
 * @brief Synchronize time from network (NTP).
 * 
 * Request payload format:
 *   u8  version
 * 
 * Response payload format (same as GetTime):
 *   u8  version
 *   u8  flags
 *   u16 reserved
 *   u64 unix_seconds
 */
uint8_t fn_clock_sync_network_time(FN_TIME_T *time)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t version;
    uint8_t i;
    
    if (time == NULL) {
        return FN_ERR_INVALID;
    }
    
    /* Build request packet: version(1) = 1 byte payload */
    req_len = fn_build_header(_clock_req_buf, FN_DEVICE_CLOCK, FN_CMD_CLOCK_SYNC_NETWORK_TIME, FN_HEADER_SIZE + 1);
    if (req_len == 0) {
        return FN_ERR_INTERNAL;
    }
    
    /* Add payload */
    _clock_req_buf[req_len++] = FN_CLOCK_VERSION;
    
    /* Finalize checksum */
    _clock_req_buf[4] = fn_calc_checksum(_clock_req_buf, req_len);
    
    /* Send request and receive response */
    result = fn_transport_exchange(_clock_req_buf, req_len, _clock_resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }
    
    /* Parse response header */
    result = fn_parse_response_header(_clock_resp_buf, resp_len, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    
    if (status != FN_OK) {
        return status;
    }
    
    /* Check minimum payload size: version(1) + flags(1) + reserved(2) + time(8) = 12 bytes */
    if (data_len < 12) {
        return FN_ERR_INVALID;
    }
    
    /* Parse response */
    version = _clock_resp_buf[data_offset];
    if (version != FN_CLOCK_VERSION) {
        return FN_ERR_UNSUPPORTED;
    }
    
    /* Copy 8-byte timestamp to output (little-endian) */
#ifdef __CC65__
    for (i = 0; i < 8; i++) {
        time->b[i] = _clock_resp_buf[data_offset + 4 + i];
    }
#else
    *time = 0;
    for (i = 0; i < 8; i++) {
        *time |= ((uint64_t)_clock_resp_buf[data_offset + 4 + i]) << (8 * i);
    }
#endif
    
    return FN_OK;
}
