/**
 * @file fn_internal.h
 * @brief Internal function declarations for fujinet-nio-lib
 * 
 * These functions are used internally by the library and should not
 * be called directly by applications.
 * 
 * @version 1.0.0
 */

#ifndef FN_INTERNAL_H
#define FN_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _CMOC_VERSION_
    #include <cmoc.h>
#else
    #include <stdint.h>
#endif

#include "fujinet-nio.h"

/* ============================================================================
 * SLIP Functions
 * ============================================================================ */

/**
 * Encode data with SLIP framing.
 */
uint16_t fn_slip_encode(const uint8_t *input, uint16_t in_len, uint8_t *output);

/**
 * Decode SLIP-framed data.
 */
uint16_t fn_slip_decode(const uint8_t *input, uint16_t in_len, uint8_t *output);

/* ============================================================================
 * Packet Building Functions
 * ============================================================================ */

/**
 * Build an Open request packet.
 */
uint16_t fn_build_open_packet(uint8_t *buffer,
                               uint8_t method,
                               uint8_t flags,
                               const char *url);

/**
 * Build a Read request packet.
 */
uint16_t fn_build_read_packet(uint8_t *buffer,
                               fn_handle_t handle,
                               uint32_t offset_val,
                               uint16_t max_bytes);

/**
 * Build a Write request packet.
 */
uint16_t fn_build_write_packet(uint8_t *buffer,
                                fn_handle_t handle,
                                uint32_t offset_val,
                                const uint8_t *data,
                                uint16_t data_len);

/**
 * Build a Close request packet.
 */
uint16_t fn_build_close_packet(uint8_t *buffer, fn_handle_t handle);

/**
 * Build an Info request packet.
 */
uint16_t fn_build_info_packet(uint8_t *buffer, fn_handle_t handle);

/* ============================================================================
 * Response Parsing Functions
 * ============================================================================ */

/**
 * Parse a response packet header.
 */
uint8_t fn_parse_response_header(const uint8_t *response,
                                  uint16_t resp_len,
                                  uint8_t *status,
                                  uint16_t *data_offset,
                                  uint16_t *data_len);

/**
 * Parse an Open response.
 */
uint8_t fn_parse_open_response(const uint8_t *response,
                                uint16_t resp_len,
                                fn_handle_t *handle,
                                uint8_t *flags);

/**
 * Parse a Read response.
 */
uint8_t fn_parse_read_response(const uint8_t *response,
                                uint16_t resp_len,
                                fn_handle_t *handle,
                                uint32_t *offset_echo,
                                uint8_t *flags,
                                uint8_t *data,
                                uint16_t data_max,
                                uint16_t *data_len);

/**
 * Parse an Info response.
 */
uint8_t fn_parse_info_response(const uint8_t *response,
                                uint16_t resp_len,
                                fn_handle_t *handle,
                                uint16_t *http_status,
                                uint32_t *content_length,
                                uint8_t *flags);

#ifdef __cplusplus
}
#endif

#endif /* FN_INTERNAL_H */
