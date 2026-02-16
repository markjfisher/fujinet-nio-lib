/**
 * @file fn_protocol.h
 * @brief FujiBus Protocol Definitions
 * 
 * Low-level protocol constants and structures for FujiBus communication.
 * This header is used internally by the library but may also be useful
 * for advanced applications that need direct protocol access.
 * 
 * @version 1.0.0
 */

#ifndef FN_PROTOCOL_H
#define FN_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _CMOC_VERSION_
    #include <cmoc.h>
#else
    #include <stdint.h>
#endif

/* ============================================================================
 * SLIP Protocol Constants
 * ============================================================================ */

/** SLIP END byte - marks frame boundaries */
#define SLIP_END       0xC0

/** SLIP ESCAPE byte - escape prefix */
#define SLIP_ESCAPE    0xDB

/** Escaped END byte */
#define SLIP_ESC_END   0xDC

/** Escaped ESCAPE byte */
#define SLIP_ESC_ESC   0xDD

/* ============================================================================
 * Wire Device IDs
 * ============================================================================ */

/** FujiNet configuration device */
#define FN_DEVICE_FUJI       0x70

/** Network service device (HTTP/TCP) */
#define FN_DEVICE_NETWORK    0xFD

/** Disk service device */
#define FN_DEVICE_DISK       0xFC

/** File service device */
#define FN_DEVICE_FILE       0xFE

/* ============================================================================
 * Network Device Commands
 * ============================================================================ */

/** Open a network session */
#define FN_CMD_OPEN    0x01

/** Read data from session */
#define FN_CMD_READ    0x02

/** Write data to session */
#define FN_CMD_WRITE   0x03

/** Close a session */
#define FN_CMD_CLOSE   0x04

/** Get session information */
#define FN_CMD_INFO    0x05

/* ============================================================================
 * Protocol Version
 * ============================================================================ */

/** Current protocol version */
#define FN_PROTOCOL_VERSION  0x01

/* ============================================================================
 * Open Flags (Wire Format)
 * ============================================================================ */

/** Use TLS for connection */
#define FN_OPEN_FLAG_TLS           0x01

/** Follow HTTP redirects */
#define FN_OPEN_FLAG_FOLLOW_REDIR  0x02

/** Body length unknown (POST/PUT) */
#define FN_OPEN_FLAG_BODY_UNKNOWN  0x04

/** Allow handle eviction */
#define FN_OPEN_FLAG_ALLOW_EVICT   0x08

/* ============================================================================
 * Open Response Flags (Wire Format)
 * ============================================================================ */

/** Handle was allocated successfully */
#define FN_OPEN_RESP_ACCEPTED      0x01

/** Body write required (POST/PUT) */
#define FN_OPEN_RESP_NEEDS_BODY    0x02

/* ============================================================================
 * Read Response Flags (Wire Format)
 * ============================================================================ */

/** End of data reached */
#define FN_READ_RESP_EOF           0x01

/** Response truncated */
#define FN_READ_RESP_TRUNCATED     0x02

/* ============================================================================
 * Info Response Flags (Wire Format)
 * ============================================================================ */

/** Response headers included */
#define FN_INFO_RESP_HEADERS       0x01

/** Content length available */
#define FN_INFO_RESP_HAS_LENGTH    0x02

/** HTTP status available */
#define FN_INFO_RESP_HAS_STATUS    0x04

/* ============================================================================
 * FujiBus Packet Structure
 * ============================================================================ */

/**
 * FujiBus packet header structure.
 * 
 * Wire format:
 *   u8  device_id     - WireDeviceId
 *   u8  command       - Command byte
 *   u8  param_count   - Number of parameters (0-4)
 *   u8  checksum      - XOR checksum of all bytes including payload
 *   u16 data_len      - Length of payload data (little-endian)
 *   u8[] params[]     - Parameter descriptors (4 bytes each)
 *   u8[] data         - Payload data
 */

/** Maximum FujiBus packet size */
#define FN_MAX_PACKET_SIZE   1024

/** Maximum parameters per packet */
#define FN_MAX_PARAMS        4

/** Parameter descriptor size in bytes */
#define FN_PARAM_DESC_SIZE   4

/** FujiBus packet header size (before parameters and payload) */
#define FN_PACKET_HEADER_SIZE 6

/* ============================================================================
 * Parameter Descriptor Format
 * ============================================================================ */

/**
 * Parameter descriptor (4 bytes):
 *   u8  size      - Parameter size (1, 2, or 4 bytes)
 *   u8  reserved  - Must be 0
 *   u16 value     - Parameter value (little-endian, right-aligned)
 */

/** Parameter sizes */
#define FN_PARAM_SIZE_U8     1
#define FN_PARAM_SIZE_U16    2
#define FN_PARAM_SIZE_U32    4

/* ============================================================================
 * Internal Structures
 * ============================================================================ */

/**
 * Session state for tracking open handles.
 * Used internally by the library.
 */
typedef struct {
    uint8_t active;        /**< 1 if session is active */
    uint8_t is_tcp;        /**< 1 if TCP session, 0 if HTTP */
    uint8_t needs_body;    /**< 1 if body write required */
    uint8_t reserved;      /**< Padding */
    uint32_t write_offset; /**< Current write offset */
    uint32_t read_offset;  /**< Current read offset */
} fn_session_t;

/* ============================================================================
 * Low-Level Packet Functions (Internal Use)
 * ============================================================================ */

/**
 * Calculate FujiBus checksum.
 * 
 * @param data      Packet data
 * @param len       Length of data
 * @return Checksum byte
 */
uint8_t fn_calc_checksum(const uint8_t *data, uint16_t len);

/**
 * Build a FujiBus packet header.
 * 
 * @param buffer      Output buffer
 * @param device_id   WireDeviceId
 * @param command     Command byte
 * @param param_count Number of parameters
 * @param data_len    Payload length
 * @return Number of bytes written
 */
uint16_t fn_build_header(uint8_t *buffer,
                         uint8_t device_id,
                         uint8_t command,
                         uint8_t param_count,
                         uint16_t data_len);

/**
 * Add a parameter to a packet.
 * 
 * @param buffer    Packet buffer (positioned after header)
 * @param param     Parameter index (0-3)
 * @param value     Parameter value
 * @param size      Parameter size (1, 2, or 4)
 * @return Number of bytes written
 */
uint16_t fn_add_param(uint8_t *buffer,
                      uint8_t param,
                      uint32_t value,
                      uint8_t size);

#ifdef __cplusplus
}
#endif

#endif /* FN_PROTOCOL_H */
