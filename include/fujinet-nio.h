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
#ifndef FN_MAX_URL_LEN
#ifdef __BBC__
#define FN_MAX_URL_LEN      512
#elif defined(__CC65__)
#define FN_MAX_URL_LEN      128
#else
#define FN_MAX_URL_LEN      256
#endif
#endif

/** Maximum concurrent network sessions */
#ifndef FN_MAX_SESSIONS
#ifdef __CC65__
#define FN_MAX_SESSIONS     3
#else
#define FN_MAX_SESSIONS     4
#endif
#endif

/** Maximum read/write chunk size */
#ifndef FN_MAX_CHUNK_SIZE
#define FN_MAX_CHUNK_SIZE   512
#endif

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

/**
 * Legacy TLS hint.
 *
 * URL schemes are authoritative across the library: use `https://` or `tls://`
 * in the URL itself. This flag is retained for compatibility but should not be
 * relied on for new code.
 */
#define FN_OPEN_TLS         0x01

/** Follow HTTP redirects automatically */
#define FN_OPEN_FOLLOW_REDIR 0x02

/** POST/PUT body length is unknown; commit with a zero-length write */
#define FN_OPEN_BODY_UNKNOWN 0x04

/** Allow handle eviction if no handles available */
#define FN_OPEN_ALLOW_EVICT 0x08

/** Streaming protocol is self-framed; avoid an extra probe read after each chunk */
#define FN_OPEN_STREAM_NO_PROBE 0x10

/* ============================================================================
 * Read Response Flags
 * ============================================================================ */

/** End of data reached (EOF) */
#define FN_READ_EOF         0x01

/** Response was truncated to fit buffer */
#define FN_READ_TRUNCATED   0x02

/** Additional bytes are already immediately available after this chunk */
#define FN_READ_MORE_AVAILABLE 0x04

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
 * Application Storage
 * ============================================================================ */

/** App-store read reached end of value */
#define FN_APPSTORE_READ_EOF     0x01

/** App-store read target key exists */
#define FN_APPSTORE_READ_EXISTS  0x02

/** App-store list has more keys after this page */
#define FN_APPSTORE_LIST_MORE    0x01

/* ============================================================================
 * BBC / fn-rom Network Extensions
 * ============================================================================ */

/** No explicit content profile */
#define FN_CONTENT_PROFILE_NONE 0x00

/** Send request body as application/json */
#define FN_CONTENT_PROFILE_JSON 0x01

/** Send request body as application/x-www-form-urlencoded */
#define FN_CONTENT_PROFILE_FORM 0x02

/** Send request body as text/plain */
#define FN_CONTENT_PROFILE_TEXT 0x03

/* ============================================================================
 * Types
 * ============================================================================ */

/** Network session handle */
typedef uint16_t fn_handle_t;

/** Invalid handle value */
#define FN_INVALID_HANDLE   0x0000

/** Application storage stat result */
typedef struct {
    uint8_t exists;          /**< Non-zero if the key exists */
    uint32_t size_bytes;     /**< Low 32 bits of value size */
    uint32_t size_bytes_high;/**< High 32 bits of value size */
    uint32_t mtime_unix;     /**< Low 32 bits of modified Unix time, or 0 */
    uint32_t mtime_unix_high;/**< High 32 bits of modified Unix time, or 0 */
} fn_appstore_stat_t;

/** Application storage read result */
typedef struct {
    uint8_t flags;           /**< FN_APPSTORE_READ_* flags */
    uint32_t offset;         /**< Echoed read offset */
    uint16_t bytes_read;     /**< Bytes copied into caller buffer */
} fn_appstore_read_t;

/** Application storage write result */
typedef struct {
    uint32_t offset;         /**< Echoed write offset */
    uint16_t bytes_written;  /**< Bytes accepted by device */
} fn_appstore_write_t;

/** Application storage delete result */
typedef struct {
    uint8_t deleted;         /**< Non-zero if an existing key was removed */
} fn_appstore_delete_t;

/** Application storage list page result */
typedef struct {
    uint8_t flags;           /**< FN_APPSTORE_LIST_* flags */
    uint16_t start_index;    /**< Echoed start index */
    uint16_t key_count;      /**< Keys encoded in key_data */
    uint16_t key_data_len;   /**< Bytes written into key_data */
} fn_appstore_list_t;

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
 * @brief Open a network session using an explicit long-URL path.
 *
 * This is intended for platforms where the common open path should stay small and
 * long URL support is an opt-in feature. On platforms without a special long-URL
 * path, this reduces to `fn_open()`.
 *
 * @param handle     Pointer to receive the session handle
 * @param method     HTTP method (FN_METHOD_*) or 0 for TCP
 * @param url        URL string (null-terminated)
 * @param flags      Open flags (FN_OPEN_*)
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_open_long(fn_handle_t *handle,
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
 * The library retries transient FN_ERR_NOT_READY/FN_ERR_BUSY responses and
 * continues partial writes until the requested byte count has been accepted or
 * a non-transient error occurs.
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

/**
 * @brief Set the one-shot request body length for the next BBC fn-rom network open.
 *
 * This maps to `OSWORD &78` reason `&01` on BBC and returns
 * `FN_ERR_UNSUPPORTED` on platforms that do not expose an equivalent API.
 */
uint8_t fn_set_body_length(uint16_t len);

/**
 * @brief Set the one-shot request content profile for the next BBC fn-rom network open.
 *
 * This maps to `OSWORD &78` reason `&03` on BBC and returns
 * `FN_ERR_UNSUPPORTED` on platforms that do not expose an equivalent API.
 */
uint8_t fn_set_content_profile(uint8_t profile);

/**
 * @brief Configure JSON translation for an already-open BBC fn-rom network channel.
 *
 * This maps to `OSWORD &78` reason `&00` on BBC and returns
 * `FN_ERR_UNSUPPORTED` on platforms that do not expose an equivalent API.
 */
uint8_t fn_json_query(fn_handle_t handle, const char *path);

/* ============================================================================
 * Application Storage Operations
 * ============================================================================ */

/**
 * @brief Query metadata for a namespaced application storage key.
 *
 * Missing keys return FN_OK with out->exists set to 0.
 */
uint8_t fn_appstore_stat(const char *namespace_name,
                         const char *key,
                         fn_appstore_stat_t *out);

/**
 * @brief Read bytes from a namespaced application storage key.
 *
 * Missing keys return FN_OK with FN_APPSTORE_READ_EXISTS clear, EOF set, and
 * bytes_read set to 0.
 */
uint8_t fn_appstore_read(const char *namespace_name,
                         const char *key,
                         uint32_t offset,
                         uint8_t *buf,
                         uint16_t max_len,
                         fn_appstore_read_t *out);

/**
 * @brief Write bytes to a namespaced application storage key.
 *
 * offset==0 creates or replaces the value. Later calls can append or overwrite
 * by using the desired byte offset.
 */
uint8_t fn_appstore_write(const char *namespace_name,
                          const char *key,
                          uint32_t offset,
                          const uint8_t *data,
                          uint16_t len,
                          fn_appstore_write_t *out);

/**
 * @brief Delete a namespaced application storage key.
 *
 * Missing keys return FN_OK with out->deleted set to 0.
 */
uint8_t fn_appstore_delete(const char *namespace_name,
                           const char *key,
                           fn_appstore_delete_t *out);

/**
 * @brief List keys in an application storage namespace.
 *
 * key_data receives the raw FileDevice list blob: repeated u16 little-endian
 * key length followed by key bytes. Use fn_appstore_list_next_key() to iterate.
 */
uint8_t fn_appstore_list(const char *namespace_name,
                         uint16_t start_index,
                         uint8_t *key_data,
                         uint16_t key_data_capacity,
                         fn_appstore_list_t *out);

/**
 * @brief Decode the next key from an app-store list key_data blob.
 *
 * Pass offset=0 for the first key. On success, key_out is null-terminated and
 * offset advances to the next encoded key.
 */
uint8_t fn_appstore_list_next_key(const uint8_t *key_data,
                                  uint16_t key_data_len,
                                  uint16_t *offset,
                                  char *key_out,
                                  uint16_t key_out_capacity);

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
#ifndef FN_MAX_TIME_STRING
#define FN_MAX_TIME_STRING  32
#endif

/** Maximum timezone string length */
#ifndef FN_MAX_TIMEZONE_LEN
#ifdef __CC65__
#define FN_MAX_TIMEZONE_LEN 32
#else
#define FN_MAX_TIMEZONE_LEN 64
#endif
#endif

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

/**
 * @brief Synchronize time from network (NTP).
 * 
 * Requests the FujiNet device to fetch the current time from NTP servers.
 * Useful after setting time manually to restore correct network time.
 * 
 * @param time       Pointer to receive the new Unix timestamp after sync (8 bytes)
 * @return FN_OK on success, error code on failure
 */
uint8_t fn_clock_sync_network_time(FN_TIME_T *time);

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

#ifdef __ATARI__
/**
 * @brief Return the last Atari SIO DSTATS value observed by the transport.
 */
uint8_t fn_atari_last_sio_status(void);
#endif

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
