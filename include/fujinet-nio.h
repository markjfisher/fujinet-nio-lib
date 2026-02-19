/**
 * @file fujinet-nio.h
 * @brief FujiNet-NIO Library for 6502 Applications
 * 
 * This library provides a clean interface for 6502 applications to communicate
 * with FujiNet-NIO devices using the FujiBus protocol. It supports both HTTP
 * and TCP network operations through a handle-based API.
 * 
 * @version 1.0.0
 * @license GPL v3, see LICENSE for details.
 */

#ifndef FUJINET_NIO_H
#define FUJINET_NIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Platform detection for standard types */
#ifdef _CMOC_VERSION_
    #include <cmoc.h>
#else
    #include <stdint.h>
    #include <stddef.h>
#endif

/* cc65 doesn't have uint64_t - use struct for 64-bit time values */
#ifdef __CC65__
    /* 64-bit time value as 8-byte array (little-endian) */
    typedef struct {
        uint8_t b[8];
    } fn_time_t;
    #define FN_TIME_T fn_time_t
#else
    #define FN_TIME_T uint64_t
#endif

/* ============================================================================
 * Constants and Configuration
 * ============================================================================ */

/** Maximum URL length supported */
#define FN_MAX_URL_LEN      256

/** Maximum concurrent network sessions */
#define FN_MAX_SESSIONS     4

/** Maximum read/write chunk size */
#define FN_MAX_CHUNK_SIZE   512

/* ============================================================================
 * Error Codes
 * ============================================================================ */

/** Operation completed successfully */
#define FN_OK               0x00

/** Device not found */
#define FN_ERR_NOT_FOUND    0x01

/** Invalid parameter or malformed request */
#define FN_ERR_INVALID      0x02

/** Device is busy, retry later */
#define FN_ERR_BUSY         0x03

/** Operation not ready, poll again */
#define FN_ERR_NOT_READY    0x04

/** I/O error during operation */
#define FN_ERR_IO           0x05

/** Device did not respond in time */
#define FN_ERR_TIMEOUT      0x06

/** Internal error */
#define FN_ERR_INTERNAL     0x07

/** Operation not supported */
#define FN_ERR_UNSUPPORTED  0x08

/** Transport layer error */
#define FN_ERR_TRANSPORT    0x10

/** URL too long */
#define FN_ERR_URL_TOO_LONG 0x11

/** No free handles available */
#define FN_ERR_NO_HANDLES   0x12

/** Unknown/unexpected error */
#define FN_ERR_UNKNOWN      0xFF

/* ============================================================================
 * HTTP Method Codes
 * ============================================================================ */

/** HTTP GET method */
#define FN_METHOD_GET       0x01

/** HTTP POST method */
#define FN_METHOD_POST      0x02

/** HTTP PUT method */
#define FN_METHOD_PUT       0x03

/** HTTP DELETE method */
#define FN_METHOD_DELETE    0x04

/** HTTP HEAD method */
#define FN_METHOD_HEAD      0x05

/* ============================================================================
 * Open Flags
 * ============================================================================ */

/** Use TLS/HTTPS for the connection */
#define FN_OPEN_TLS         0x01

/** Follow HTTP redirects automatically */
#define FN_OPEN_FOLLOW_REDIR 0x02

/** Allow handle eviction if no handles available */
#define FN_OPEN_ALLOW_EVICT 0x08

/* ============================================================================
 * Read Response Flags
 * ============================================================================ */

/** End of data reached (EOF) */
#define FN_READ_EOF         0x01

/** Response was truncated to fit buffer */
#define FN_READ_TRUNCATED   0x02

/* ============================================================================
 * Info Flags
 * ============================================================================ */

/** HTTP status code is available */
#define FN_INFO_HAS_STATUS  0x04

/** Content length is available */
#define FN_INFO_HAS_LENGTH  0x02

/** TCP: Socket is connected */
#define FN_INFO_CONNECTED   0x10

/** TCP: Peer has closed the connection */
#define FN_INFO_PEER_CLOSED 0x20

/* ============================================================================
 * Types
 * ============================================================================ */

/** Network session handle */
typedef uint16_t fn_handle_t;

/** Invalid handle value */
#define FN_INVALID_HANDLE   0x0000

/* ============================================================================
 * Initialization
 * ============================================================================ */

/**
 * @brief Initialize the FujiNet-NIO library.
 * 
 * This must be called before any other library functions.
 * Performs platform-specific initialization and checks for device presence.
 * 
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_init(void);

/**
 * @brief Check if the FujiNet device is present and ready.
 * 
 * @return 1 if ready, 0 if not ready or not present
 */
uint8_t fn_is_ready(void);

/* ============================================================================
 * Network Operations
 * ============================================================================ */

/**
 * @brief Open a network session.
 * 
 * Creates a new network session and returns a handle for subsequent operations.
 * The URL scheme determines the protocol:
 *   - "http://" or "https://": HTTP protocol
 *   - "tcp://": Raw TCP socket
 * 
 * For HTTP:
 *   - Use FN_METHOD_* constants for the method parameter
 *   - GET/HEAD/DELETE typically complete immediately
 *   - POST/PUT may require fn_write() for body data
 * 
 * For TCP:
 *   - Use method = 0
 *   - URL format: "tcp://hostname:port"
 *   - Connection is established asynchronously
 * 
 * @param handle     Pointer to receive the session handle
 * @param method     HTTP method (FN_METHOD_*) or 0 for TCP
 * @param url        URL string (null-terminated)
 * @param flags      Open flags (FN_OPEN_*)
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_open(fn_handle_t *handle, 
                uint8_t method,
                const char *url,
                uint8_t flags);

/**
 * @brief Open a TCP connection (convenience wrapper).
 * 
 * @param handle     Pointer to receive the session handle
 * @param host       Hostname or IP address (null-terminated)
 * @param port       Port number
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_tcp_open(fn_handle_t *handle,
                    const char *host,
                    uint16_t port);

/**
 * @brief Write data to a session.
 * 
 * For HTTP POST/PUT: writes request body data.
 * For TCP: sends data on the socket.
 * 
 * Offsets must be sequential. For HTTP, the request is dispatched
 * automatically when bodyLenHint bytes have been written.
 * 
 * @param handle     Session handle
 * @param offset     Byte offset (must be sequential)
 * @param data       Data buffer to write
 * @param len        Length of data
 * @param written    Pointer to receive bytes actually written
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_write(fn_handle_t handle,
                 uint32_t offset,
                 const uint8_t *data,
                 uint16_t len,
                 uint16_t *written);

/**
 * @brief Read data from a session.
 * 
 * For HTTP: reads response body data.
 * For TCP: receives data from the socket.
 * 
 * Continue reading until FN_READ_EOF flag is set or bytes_read is 0.
 * 
 * @param handle      Session handle
 * @param offset      Byte offset (must be sequential for TCP)
 * @param buf         Buffer to receive data
 * @param max_len     Maximum bytes to read
 * @param bytes_read  Pointer to receive bytes actually read
 * @param flags       Pointer to receive read flags (FN_READ_*)
 * @return FN_OK on success, FN_ERR_NOT_READY if no data available
 */
uint8_t fn_read(fn_handle_t handle,
                uint32_t offset,
                uint8_t *buf,
                uint16_t max_len,
                uint16_t *bytes_read,
                uint8_t *flags);

/**
 * @brief Get session information.
 * 
 * For HTTP: returns HTTP status code and content length.
 * For TCP: returns connection state.
 * 
 * @param handle          Session handle
 * @param http_status     Pointer to receive HTTP status (or 0 if N/A)
 * @param content_length  Pointer to receive content length (or 0 if unknown)
 * @param flags           Pointer to receive info flags (FN_INFO_*)
 * @return FN_OK on success, FN_ERR_NOT_READY if info not yet available
 */
uint8_t fn_info(fn_handle_t handle,
                uint16_t *http_status,
                uint32_t *content_length,
                uint8_t *flags);

/**
 * @brief Close a network session.
 * 
 * Releases the session handle and any associated resources.
 * 
 * @param handle     Session handle to close
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_close(fn_handle_t handle);

/* ============================================================================
 * Clock Operations
 * ============================================================================ */

/**
 * @brief Time format codes for fn_clock_get_format().
 * 
 * These codes match the server-side TimeFormat enum and the legacy
 * fujinet-lib TimeFormat enum for compatibility.
 */
typedef enum {
    /** 7 bytes: [century, year, month, day, hour, min, sec] */
    FN_TIME_FORMAT_SIMPLE     = 0x00,
    
    /** 4 bytes: Apple ProDOS format */
    FN_TIME_FORMAT_PRODOS     = 0x01,
    
    /** 6 bytes: [day, month, year, hour, min, sec] */
    FN_TIME_FORMAT_APETIME    = 0x02,
    
    /** ISO string with TZ offset: "YYYY-MM-DDTHH:MM:SS+HHMM" */
    FN_TIME_FORMAT_TZ_ISO     = 0x03,
    
    /** ISO string UTC: "YYYY-MM-DDTHH:MM:SS+0000" */
    FN_TIME_FORMAT_UTC_ISO    = 0x04,
    
    /** 16 bytes: "YYYYMMDD0HHMMSS000" */
    FN_TIME_FORMAT_APPLE3_SOS = 0x05,
} FnTimeFormat;

/** Maximum formatted time string length (for string formats) */
#define FN_MAX_TIME_STRING  32

/** Maximum timezone string length */
#define FN_MAX_TIMEZONE_LEN 64

/**
 * @brief Get the current time from the FujiNet device.
 * 
 * Returns the current Unix timestamp (seconds since 1970-01-01).
 * 
 * @param time       Pointer to receive the Unix timestamp (8 bytes, little-endian)
 * @return FN_OK on success, FN_ERR_NOT_READY if time not available
 */
uint8_t fn_clock_get(FN_TIME_T *time);

/**
 * @brief Set the time on the FujiNet device.
 * 
 * Sets the device's real-time clock to the specified Unix timestamp.
 * 
 * @param time       Pointer to Unix timestamp to set (8 bytes, little-endian)
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_clock_set(const FN_TIME_T *time);

/**
 * @brief Get the current time in a specific format.
 * 
 * Returns the time pre-formatted by the FujiNet device, offloading
 * complex time conversion from the host.
 * 
 * @param time_data   Buffer to receive formatted time (size depends on format)
 * @param format      Desired time format (FnTimeFormat enum)
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_clock_get_format(uint8_t *time_data, FnTimeFormat format);

/**
 * @brief Get the current time for a specific timezone without affecting system TZ.
 * 
 * @param time_data   Buffer to receive formatted time
 * @param tz          Timezone string (POSIX format, e.g., "CET-1CEST,M3.5.0,M10.5.0/3")
 * @param format      Desired time format
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_clock_get_tz(uint8_t *time_data, const char *tz, FnTimeFormat format);

/**
 * @brief Get the current timezone string.
 * 
 * @param tz          Buffer to receive timezone string (min FN_MAX_TIMEZONE_LEN bytes)
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_clock_get_timezone(char *tz);

/**
 * @brief Set the timezone (non-persistent, runtime only).
 * 
 * @param tz          Timezone string (POSIX format)
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_clock_set_timezone(const char *tz);

/**
 * @brief Set the timezone and persist to configuration.
 * 
 * @param tz          Timezone string (POSIX format)
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_clock_set_timezone_save(const char *tz);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Get a human-readable error string.
 * 
 * @param error      Error code
 * @return Pointer to static error string
 */
const char *fn_error_string(uint8_t error);

/**
 * @brief Get the library version string.
 * 
 * @return Pointer to version string (e.g., "1.0.0")
 */
const char *fn_version(void);

#ifdef __cplusplus
}
#endif

#endif /* FUJINET_NIO_H */
