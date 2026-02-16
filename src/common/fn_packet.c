/**
 * @file fn_packet.c
 * @brief FujiBus Packet Construction Implementation
 * 
 * Provides functions to build and parse FujiBus protocol packets.
 * 
 * @version 1.0.0
 */

#include "fn_protocol.h"
#include "fn_internal.h"
#include <string.h>

/* ============================================================================
 * Checksum Calculation
 * ============================================================================ */

/**
 * Calculate FujiBus checksum.
 * 
 * The checksum is a simple XOR of all bytes in the packet.
 * 
 * @param data    Packet data
 * @param len     Length of data
 * @return Checksum byte
 */
uint8_t fn_calc_checksum(const uint8_t *data, uint16_t len)
{
    uint8_t checksum;
    uint16_t i;
    
    checksum = 0;
    for (i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    
    return checksum;
}

/* ============================================================================
 * Packet Building Functions
 * ============================================================================ */

/**
 * Build a FujiBus packet header.
 * 
 * @param buffer       Output buffer
 * @param device_id    WireDeviceId
 * @param command      Command byte
 * @param param_count  Number of parameters
 * @param data_len     Payload length
 * @return Number of bytes written
 */
uint16_t fn_build_header(uint8_t *buffer,
                         uint8_t device_id,
                         uint8_t command,
                         uint8_t param_count,
                         uint16_t data_len)
{
    uint16_t offset;
    
    offset = 0;
    
    /* Device ID */
    buffer[offset++] = device_id;
    
    /* Command */
    buffer[offset++] = command;
    
    /* Parameter count */
    buffer[offset++] = param_count;
    
    /* Checksum placeholder (will be filled later) */
    buffer[offset++] = 0;
    
    /* Data length (little-endian) */
    buffer[offset++] = data_len & 0xFF;
    buffer[offset++] = (data_len >> 8) & 0xFF;
    
    return offset;
}

/**
 * Add a parameter to a packet.
 * 
 * @param buffer    Packet buffer (positioned at parameter area)
 * @param param     Parameter index (0-3)
 * @param value     Parameter value
 * @param size      Parameter size (1, 2, or 4)
 * @return Number of bytes written
 */
uint16_t fn_add_param(uint8_t *buffer,
                      uint8_t param,
                      uint32_t value,
                      uint8_t size)
{
    uint16_t offset;
    
    offset = param * FN_PARAM_DESC_SIZE;
    
    /* Size byte */
    buffer[offset] = size;
    
    /* Reserved */
    buffer[offset + 1] = 0;
    
    /* Value (little-endian) */
    buffer[offset + 2] = value & 0xFF;
    buffer[offset + 3] = (value >> 8) & 0xFF;
    
    /* For 32-bit values, we'd need more bytes, but FujiBus params are max 16-bit in practice */
    
    return FN_PARAM_DESC_SIZE;
}

/* ============================================================================
 * High-Level Packet Builders
 * ============================================================================ */

/**
 * Build an Open request packet.
 * 
 * @param buffer    Output buffer
 * @param method    HTTP method (FN_METHOD_*) or 0 for TCP
 * @param flags     Open flags
 * @param url       URL string (null-terminated)
 * @return Packet length, or 0 on error
 */
uint16_t fn_build_open_packet(uint8_t *buffer,
                               uint8_t method,
                               uint8_t flags,
                               const char *url)
{
    uint16_t offset;
    uint16_t url_len;
    uint16_t data_len;
    uint8_t checksum;
    
    url_len = 0;
    while (url[url_len] != '\0') {
        url_len++;
        if (url_len > FN_MAX_URL_LEN) {
            return 0;  /* URL too long */
        }
    }
    
    /* Calculate payload length */
    /* version(1) + method(1) + flags(1) + url_len(2) + url + header_count(2) + body_len(4) + resp_header_count(2) */
    data_len = 1 + 1 + 1 + 2 + url_len + 2 + 4 + 2;
    
    /* Build header */
    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_OPEN, 0, data_len);
    
    /* Build payload */
    /* Version */
    buffer[offset++] = FN_PROTOCOL_VERSION;
    
    /* Method */
    buffer[offset++] = method;
    
    /* Flags */
    buffer[offset++] = flags;
    
    /* URL length (little-endian) */
    buffer[offset++] = url_len & 0xFF;
    buffer[offset++] = (url_len >> 8) & 0xFF;
    
    /* URL */
    memcpy(buffer + offset, url, url_len);
    offset += url_len;
    
    /* Header count = 0 (no request headers in v1) */
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    
    /* Body length hint = 0 */
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    
    /* Response header count = 0 (no header capture in v1) */
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    
    /* Calculate and insert checksum */
    checksum = fn_calc_checksum(buffer, offset);
    buffer[3] = checksum;  /* Checksum is at offset 3 */
    
    return offset;
}

/**
 * Build a Read request packet.
 * 
 * @param buffer     Output buffer
 * @param handle     Session handle
 * @param offset     Read offset
 * @param max_bytes  Maximum bytes to read
 * @return Packet length
 */
uint16_t fn_build_read_packet(uint8_t *buffer,
                               fn_handle_t handle,
                               uint32_t offset_val,
                               uint16_t max_bytes)
{
    uint16_t offset;
    uint16_t data_len;
    uint8_t checksum;
    
    /* Payload: version(1) + handle(2) + offset(4) + max_bytes(2) = 9 bytes */
    data_len = 9;
    
    /* Build header */
    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_READ, 0, data_len);
    
    /* Version */
    buffer[offset++] = FN_PROTOCOL_VERSION;
    
    /* Handle (little-endian) */
    buffer[offset++] = handle & 0xFF;
    buffer[offset++] = (handle >> 8) & 0xFF;
    
    /* Offset (little-endian) */
    buffer[offset++] = offset_val & 0xFF;
    buffer[offset++] = (offset_val >> 8) & 0xFF;
    buffer[offset++] = (offset_val >> 16) & 0xFF;
    buffer[offset++] = (offset_val >> 24) & 0xFF;
    
    /* Max bytes (little-endian) */
    buffer[offset++] = max_bytes & 0xFF;
    buffer[offset++] = (max_bytes >> 8) & 0xFF;
    
    /* Calculate and insert checksum */
    checksum = fn_calc_checksum(buffer, offset);
    buffer[3] = checksum;
    
    return offset;
}

/**
 * Build a Write request packet.
 * 
 * @param buffer     Output buffer
 * @param handle     Session handle
 * @param offset     Write offset
 * @param data       Data to write
 * @param data_len   Length of data
 * @return Packet length
 */
uint16_t fn_build_write_packet(uint8_t *buffer,
                                fn_handle_t handle,
                                uint32_t offset_val,
                                const uint8_t *data,
                                uint16_t data_len)
{
    uint16_t offset;
    uint16_t payload_len;
    uint8_t checksum;
    
    /* Payload: version(1) + handle(2) + offset(4) + data_len(2) + data */
    payload_len = 1 + 2 + 4 + 2 + data_len;
    
    /* Build header */
    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_WRITE, 0, payload_len);
    
    /* Version */
    buffer[offset++] = FN_PROTOCOL_VERSION;
    
    /* Handle (little-endian) */
    buffer[offset++] = handle & 0xFF;
    buffer[offset++] = (handle >> 8) & 0xFF;
    
    /* Offset (little-endian) */
    buffer[offset++] = offset_val & 0xFF;
    buffer[offset++] = (offset_val >> 8) & 0xFF;
    buffer[offset++] = (offset_val >> 16) & 0xFF;
    buffer[offset++] = (offset_val >> 24) & 0xFF;
    
    /* Data length (little-endian) */
    buffer[offset++] = data_len & 0xFF;
    buffer[offset++] = (data_len >> 8) & 0xFF;
    
    /* Data */
    if (data_len > 0 && data != NULL) {
        memcpy(buffer + offset, data, data_len);
        offset += data_len;
    }
    
    /* Calculate and insert checksum */
    checksum = fn_calc_checksum(buffer, offset);
    buffer[3] = checksum;
    
    return offset;
}

/**
 * Build a Close request packet.
 * 
 * @param buffer    Output buffer
 * @param handle    Session handle
 * @return Packet length
 */
uint16_t fn_build_close_packet(uint8_t *buffer, fn_handle_t handle)
{
    uint16_t offset;
    uint16_t data_len;
    uint8_t checksum;
    
    /* Payload: version(1) + handle(2) = 3 bytes */
    data_len = 3;
    
    /* Build header */
    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_CLOSE, 0, data_len);
    
    /* Version */
    buffer[offset++] = FN_PROTOCOL_VERSION;
    
    /* Handle (little-endian) */
    buffer[offset++] = handle & 0xFF;
    buffer[offset++] = (handle >> 8) & 0xFF;
    
    /* Calculate and insert checksum */
    checksum = fn_calc_checksum(buffer, offset);
    buffer[3] = checksum;
    
    return offset;
}

/**
 * Build an Info request packet.
 * 
 * @param buffer    Output buffer
 * @param handle    Session handle
 * @return Packet length
 */
uint16_t fn_build_info_packet(uint8_t *buffer, fn_handle_t handle)
{
    uint16_t offset;
    uint16_t data_len;
    uint8_t checksum;
    
    /* Payload: version(1) + handle(2) = 3 bytes */
    data_len = 3;
    
    /* Build header */
    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_INFO, 0, data_len);
    
    /* Version */
    buffer[offset++] = FN_PROTOCOL_VERSION;
    
    /* Handle (little-endian) */
    buffer[offset++] = handle & 0xFF;
    buffer[offset++] = (handle >> 8) & 0xFF;
    
    /* Calculate and insert checksum */
    checksum = fn_calc_checksum(buffer, offset);
    buffer[3] = checksum;
    
    return offset;
}

/* ============================================================================
 * Response Parsing Functions
 * ============================================================================ */

/**
 * Parse a response packet header.
 * 
 * @param response      Response packet buffer
 * @param resp_len      Response length
 * @param status        Pointer to receive status code
 * @param data_offset   Pointer to receive offset of payload data
 * @param data_len      Pointer to receive payload length
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_parse_response_header(const uint8_t *response,
                                  uint16_t resp_len,
                                  uint8_t *status,
                                  uint16_t *data_offset,
                                  uint16_t *data_len)
{
    uint8_t checksum;
    
    /* Minimum response: device(1) + command(1) + param_count(1) + checksum(1) + data_len(2) = 6 bytes */
    if (resp_len < 6) {
        return FN_ERR_INVALID;
    }
    
    /* Verify checksum */
    checksum = fn_calc_checksum(response, resp_len);
    if (checksum != 0) {
        return FN_ERR_IO;
    }
    
    /* Extract status from first parameter (if present) */
    /* In FujiBus, status is typically in param[0] */
    if (response[2] > 0 && resp_len >= 10) {
        /* Parameter 0 is at offset 6 */
        *status = response[9];  /* Low byte of param value */
    } else {
        *status = FN_OK;
    }
    
    /* Extract data length */
    *data_len = response[4] | (response[5] << 8);
    
    /* Calculate data offset (after header and parameters) */
    *data_offset = 6 + (response[2] * FN_PARAM_DESC_SIZE);
    
    return FN_OK;
}

/**
 * Parse an Open response.
 * 
 * @param response    Response packet
 * @param resp_len    Response length
 * @param handle      Pointer to receive handle
 * @param flags       Pointer to receive flags
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_parse_open_response(const uint8_t *response,
                                uint16_t resp_len,
                                fn_handle_t *handle,
                                uint8_t *flags)
{
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t result;
    
    result = fn_parse_response_header(response, resp_len, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    
    if (status != FN_OK) {
        return status;
    }
    
    /* Open response payload: version(1) + flags(1) + reserved(2) + handle(2) */
    if (data_len < 6) {
        return FN_ERR_INVALID;
    }
    
    *flags = response[data_offset + 1];
    *handle = response[data_offset + 4] | (response[data_offset + 5] << 8);
    
    return FN_OK;
}

/**
 * Parse a Read response.
 * 
 * @param response     Response packet
 * @param resp_len     Response length
 * @param handle       Pointer to receive handle
 * @param offset_echo  Pointer to receive offset echo
 * @param flags        Pointer to receive flags
 * @param data         Buffer to receive data
 * @param data_max     Maximum data buffer size
 * @param data_len     Pointer to receive actual data length
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_parse_read_response(const uint8_t *response,
                                uint16_t resp_len,
                                fn_handle_t *handle,
                                uint32_t *offset_echo,
                                uint8_t *flags,
                                uint8_t *data,
                                uint16_t data_max,
                                uint16_t *data_len)
{
    uint8_t status;
    uint16_t data_offset;
    uint16_t payload_len;
    uint8_t result;
    uint16_t actual_data_len;
    uint16_t copy_len;
    
    result = fn_parse_response_header(response, resp_len, &status, &data_offset, &payload_len);
    if (result != FN_OK) {
        return result;
    }
    
    if (status != FN_OK) {
        return status;
    }
    
    /* Read response payload: version(1) + flags(1) + reserved(2) + handle(2) + offset(4) + data_len(2) + data */
    if (payload_len < 12) {
        return FN_ERR_INVALID;
    }
    
    *flags = response[data_offset + 1];
    *handle = response[data_offset + 4] | (response[data_offset + 5] << 8);
    *offset_echo = ((uint32_t)response[data_offset + 6]) |
                   ((uint32_t)response[data_offset + 7] << 8) |
                   ((uint32_t)response[data_offset + 8] << 16) |
                   ((uint32_t)response[data_offset + 9] << 24);
    
    actual_data_len = response[data_offset + 10] | (response[data_offset + 11] << 8);
    
    /* Copy data */
    copy_len = actual_data_len;
    if (copy_len > data_max) {
        copy_len = data_max;
    }
    
    if (copy_len > 0 && data != NULL) {
        memcpy(data, response + data_offset + 12, copy_len);
    }
    
    *data_len = actual_data_len;
    
    return FN_OK;
}

/**
 * Parse an Info response.
 * 
 * @param response         Response packet
 * @param resp_len         Response length
 * @param handle           Pointer to receive handle
 * @param http_status      Pointer to receive HTTP status
 * @param content_length   Pointer to receive content length
 * @param flags            Pointer to receive flags
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_parse_info_response(const uint8_t *response,
                                uint16_t resp_len,
                                fn_handle_t *handle,
                                uint16_t *http_status,
                                uint32_t *content_length,
                                uint8_t *flags)
{
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;
    uint8_t result;
    
    result = fn_parse_response_header(response, resp_len, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }
    
    if (status != FN_OK) {
        return status;
    }
    
    /* Info response payload: version(1) + flags(1) + reserved(2) + handle(2) + http_status(2) + content_length(8) + ... */
    if (data_len < 16) {
        /* Minimal response */
        *flags = 0;
        *http_status = 0;
        *content_length = 0;
        return FN_OK;
    }
    
    *flags = response[data_offset + 1];
    *handle = response[data_offset + 4] | (response[data_offset + 5] << 8);
    
    /* HTTP status (only valid if FN_INFO_HAS_STATUS flag is set) */
    *http_status = response[data_offset + 6] | (response[data_offset + 7] << 8);
    
    /* Content length (only valid if FN_INFO_HAS_LENGTH flag is set) */
    /* Note: protocol uses 64-bit, but we only return 32-bit */
    *content_length = ((uint32_t)response[data_offset + 8]) |
                      ((uint32_t)response[data_offset + 9] << 8) |
                      ((uint32_t)response[data_offset + 10] << 16) |
                      ((uint32_t)response[data_offset + 11] << 24);
    
    return FN_OK;
}
