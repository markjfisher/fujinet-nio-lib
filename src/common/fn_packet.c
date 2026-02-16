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
 * The checksum is a sum of all bytes with carry folding.
 * 
 * @param data    Packet data
 * @param len     Length of data
 * @return Checksum byte
 */
uint8_t fn_calc_checksum(const uint8_t *data, uint16_t len)
{
    uint16_t chk;
    uint16_t i;
    
    chk = 0;
    for (i = 0; i < len; i++) {
        chk += data[i];
        chk = ((chk >> 8) + (chk & 0xFF)) & 0xFFFF;
    }
    
    return (uint8_t)(chk & 0xFF);
}

/* ============================================================================
 * Packet Building Functions
 * ============================================================================ */

/**
 * Build a FujiBus packet header.
 * 
 * Header format: device(1) + command(1) + length(2) + checksum(1) + descr(1) = 6 bytes
 * 
 * @param buffer       Output buffer
 * @param device_id    WireDeviceId
 * @param command      Command byte
 * @param total_len    Total packet length (header + payload)
 * @return Number of bytes written
 */
uint16_t fn_build_header(uint8_t *buffer,
                         uint8_t device_id,
                         uint8_t command,
                         uint16_t total_len)
{
    uint16_t offset;
    
    offset = 0;
    
    /* Device ID */
    buffer[offset++] = device_id;
    
    /* Command */
    buffer[offset++] = command;
    
    /* Total length (little-endian) */
    buffer[offset++] = total_len & 0xFF;
    buffer[offset++] = (total_len >> 8) & 0xFF;
    
    /* Checksum placeholder (will be filled later) */
    buffer[offset++] = 0;
    
    /* Descriptor (0 for simple packets) */
    buffer[offset++] = 0;
    
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
    uint16_t payload_len;
    uint16_t total_len;
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
    payload_len = 1 + 1 + 1 + 2 + url_len + 2 + 4 + 2;
    
    /* Total packet length = header(6) + payload */
    total_len = FN_HEADER_SIZE + payload_len;
    
    /* Build header */
    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_OPEN, total_len);
    
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
    buffer[4] = checksum;  /* Checksum is at offset 4 in new header format */
    
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
    uint16_t payload_len;
    uint16_t total_len;
    uint8_t checksum;
    
    /* Payload: version(1) + handle(2) + offset(4) + max_bytes(2) = 9 bytes */
    payload_len = 9;
    total_len = FN_HEADER_SIZE + payload_len;
    
    /* Build header */
    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_READ, total_len);
    
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
    buffer[4] = checksum;  /* Checksum is at offset 4 */
    
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
    uint16_t total_len;
    uint8_t checksum;
    
    /* Payload: version(1) + handle(2) + offset(4) + data_len(2) + data */
    payload_len = 1 + 2 + 4 + 2 + data_len;
    total_len = FN_HEADER_SIZE + payload_len;
    
    /* Build header */
    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_WRITE, total_len);
    
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
    buffer[4] = checksum;  /* Checksum is at offset 4 */
    
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
    uint16_t payload_len;
    uint16_t total_len;
    uint8_t checksum;
    
    /* Payload: version(1) + handle(2) = 3 bytes */
    payload_len = 3;
    total_len = FN_HEADER_SIZE + payload_len;
    
    /* Build header */
    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_CLOSE, total_len);
    
    /* Version */
    buffer[offset++] = FN_PROTOCOL_VERSION;
    
    /* Handle (little-endian) */
    buffer[offset++] = handle & 0xFF;
    buffer[offset++] = (handle >> 8) & 0xFF;
    
    /* Calculate and insert checksum */
    checksum = fn_calc_checksum(buffer, offset);
    buffer[4] = checksum;  /* Checksum is at offset 4 */
    
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
    uint16_t payload_len;
    uint16_t total_len;
    uint8_t checksum;
    
    /* Payload: version(1) + handle(2) = 3 bytes */
    payload_len = 3;
    total_len = FN_HEADER_SIZE + payload_len;
    
    /* Build header */
    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_INFO, total_len);
    
    /* Version */
    buffer[offset++] = FN_PROTOCOL_VERSION;
    
    /* Handle (little-endian) */
    buffer[offset++] = handle & 0xFF;
    buffer[offset++] = (handle >> 8) & 0xFF;
    
    /* Calculate and insert checksum */
    checksum = fn_calc_checksum(buffer, offset);
    buffer[4] = checksum;  /* Checksum is at offset 4 */
    
    return offset;
}

/* ============================================================================
 * Response Parsing Functions
 * ============================================================================ */

/**
 * Parse a response packet header.
 * 
 * Header format: device(1) + command(1) + length(2) + checksum(1) + descr(1) = 6 bytes
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
    uint16_t pkt_len;
    uint8_t descr;
    uint8_t checksum;
    uint16_t offset;
    uint8_t tmp_buffer[1024];
    
    /* Minimum response: header(6) */
    if (resp_len < FN_HEADER_SIZE) {
        return FN_ERR_INVALID;
    }
    
    /* Extract packet length */
    pkt_len = response[2] | (response[3] << 8);
    
    /* Verify packet length matches */
    if (pkt_len != resp_len) {
        return FN_ERR_INVALID;
    }
    
    /* Verify checksum - copy to temp buffer and zero checksum byte */
    if (resp_len > sizeof(tmp_buffer)) {
        return FN_ERR_INVALID;
    }
    memcpy(tmp_buffer, response, resp_len);
    tmp_buffer[4] = 0;  /* Zero checksum for calculation */
    checksum = fn_calc_checksum(tmp_buffer, resp_len);
    if (checksum != response[4]) {
        return FN_ERR_IO;
    }
    
    /* Extract descriptor */
    descr = response[5];
    
    /* For simple packets (descr=0), payload starts immediately after header */
    if (descr == 0) {
        *data_offset = FN_HEADER_SIZE;
        *data_len = resp_len - FN_HEADER_SIZE;
        *status = FN_OK;  /* No params means status is in payload */
        return FN_OK;
    }
    
    /* Parse descriptor to find params and payload offset */
    /* Descriptor format: low 3 bits encode field type */
    /* For now, handle simple case: descr encodes param count in low nibble */
    offset = FN_HEADER_SIZE;
    
    /* Handle continuation bit (0x80) - varint-like descriptor */
    while (descr & 0x80) {
        if (offset >= resp_len) {
            return FN_ERR_INVALID;
        }
        descr = response[offset++];
    }
    
    /* Parse params based on descriptor */
    /* Field size table: 0->0, 1->1, 2->1, 3->1, 4->1, 5->2, 6->2, 7->4 */
    /* Num fields table: 0->0, 1->1, 2->2, 3->3, 4->4, 5->1, 6->2, 7->1 */
    {
        uint8_t field_desc;
        uint8_t field_count;
        uint8_t field_size;
        uint8_t i;
        
        field_desc = descr & 0x07;
        
        /* Field size table */
        static const uint8_t field_size_table[8] = {0, 1, 1, 1, 1, 2, 2, 4};
        /* Field count table */
        static const uint8_t field_count_table[8] = {0, 1, 2, 3, 4, 1, 2, 1};
        
        field_size = field_size_table[field_desc];
        field_count = field_count_table[field_desc];
        
        /* Extract first param as status (if present) */
        if (field_count > 0 && offset + field_size <= resp_len) {
            *status = 0;
            for (i = 0; i < field_size; i++) {
                *status |= response[offset + i] << (8 * i);
            }
            offset += field_size;
            
            /* Skip remaining params */
            for (i = 1; i < field_count; i++) {
                offset += field_size;
            }
        } else {
            *status = FN_OK;
        }
    }
    
    *data_offset = offset;
    *data_len = resp_len - offset;
    
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
