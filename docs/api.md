# API Reference

This document provides a complete reference for the fujinet-nio-lib API.

## Initialization

### `fn_init()`

Initialize the library and transport layer.

```c
uint8_t fn_init(void);
```

**Returns:** `FN_OK` on success, error code on failure.

**Example:**
```c
uint8_t result = fn_init();
if (result != FN_OK) {
    printf("Init failed: %s\n", fn_error_string(result));
    return 1;
}
```

### `fn_is_ready()`

Check if the FujiNet device is ready for communication.

```c
uint8_t fn_is_ready(void);
```

**Returns:** Non-zero if device is ready, 0 if not.

## Network Operations

### Protocol Behavior

When you call `fn_open()`, the FujiNet device determines the protocol type from the URL scheme and returns **protocol capability flags**. The client library uses these flags internally to enforce correct offset behavior:

| Protocol | Flags | Offset Behavior |
|----------|-------|-----------------|
| HTTP/HTTPS | `0x00` | Random-access reads allowed (can request any offset) |
| TCP/TLS | `0x07` | Sequential offsets required (must track total bytes read/written) |

**Why this matters:**

- **HTTP**: You can request any offset (e.g., resume a download from byte 1000). The server may support range requests.
- **TCP/TLS**: Offsets must be strictly sequential. The library tracks `read_offset` and `write_offset` internally and validates that your offset matches the expected cursor position.

**Application code pattern:**

For all protocols, the recommended pattern is to track total bytes read/written and pass that as the offset:

```c
uint32_t total_read = 0;
do {
    result = fn_read(handle, total_read, buffer, sizeof(buffer), &bytes_read, &flags);
    if (result == FN_OK && bytes_read > 0) {
        // Process data...
        total_read += bytes_read;  // Track for next offset
    }
} while (result == FN_OK && !(flags & FN_READ_EOF));
```

This pattern works for both HTTP and TCP/TLS. For HTTP, the offset is informational; for TCP/TLS, it's enforced.

### `fn_open()`

Open a network connection to a URL.

```c
uint8_t fn_open(fn_handle_t *handle, 
                uint8_t method,
                const char *url,
                uint8_t flags);
```

**Parameters:**
- `handle` - Output pointer for the session handle
- `method` - HTTP method (`FN_METHOD_GET`, `FN_METHOD_POST`, etc.) or 0 for raw TCP/TLS
- `url` - URL to connect to (e.g., `http://example.com`, `tcp://host:port`, `tls://host:port`)
- `flags` - Optional flags (`FN_OPEN_TLS`, `FN_OPEN_FOLLOW_REDIR`)

**Returns:** `FN_OK` on success, error code on failure.

**URL Schemes:**
- `http://` - HTTP connection
- `https://` - HTTPS connection (TLS)
- `tcp://` - Raw TCP connection
- `tls://` - Raw TLS connection

**TLS Options (query parameters):**
- `?testca=1` - Use FujiNet Test CA for local testing
- `?insecure=1` - Skip certificate verification (not recommended)

**Example:**
```c
fn_handle_t handle;
uint8_t result = fn_open(&handle, FN_METHOD_GET, "https://example.com/api", 0);
```

### `fn_tcp_open()`

Open a TCP connection to a host and port (convenience function).

```c
uint8_t fn_tcp_open(fn_handle_t *handle,
                    const char *host,
                    uint16_t port);
```

**Parameters:**
- `handle` - Output pointer for the session handle
- `host` - Hostname or IP address
- `port` - Port number

**Returns:** `FN_OK` on success, error code on failure.

### `fn_read()`

Read data from an open connection.

```c
uint8_t fn_read(fn_handle_t handle,
                uint32_t offset,
                uint8_t *buf,
                uint16_t max_len,
                uint16_t *bytes_read,
                uint8_t *flags);
```

**Parameters:**
- `handle` - Session handle from `fn_open()`
- `offset` - Read offset (typically total bytes read so far)
- `buf` - Buffer to receive data
- `max_len` - Maximum bytes to read
- `bytes_read` - Output: actual bytes read
- `flags` - Output: read flags (`FN_READ_EOF`, etc.)

**Returns:** `FN_OK` on success, `FN_ERR_NOT_READY` if no data available, error code on failure.

**Read Flags:**
- `FN_READ_EOF` - End of stream reached

**Example:**
```c
uint8_t buffer[512];
uint16_t bytes_read;
uint8_t flags;
uint32_t total = 0;

do {
    result = fn_read(handle, total, buffer, sizeof(buffer) - 1, &bytes_read, &flags);
    if (result == FN_OK && bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
        total += bytes_read;
    }
} while (result == FN_OK && !(flags & FN_READ_EOF));
```

### `fn_write()`

Write data to an open connection.

```c
uint8_t fn_write(fn_handle_t handle,
                 uint32_t offset,
                 const uint8_t *data,
                 uint16_t len,
                 uint16_t *written);
```

**Parameters:**
- `handle` - Session handle from `fn_open()`
- `offset` - Write offset (typically total bytes written so far)
- `data` - Data to write
- `len` - Length of data
- `written` - Output: actual bytes written

**Returns:** `FN_OK` on success, error code on failure.

**Half-Close:** To signal end of write (send FIN), call with `len=0` at current offset:
```c
// After writing all data, half-close the write side
fn_write(handle, total_written, NULL, 0, &dummy);
```

### `fn_info()`

Get information about an open connection.

```c
uint8_t fn_info(fn_handle_t handle,
                uint16_t *http_status,
                uint32_t *content_length,
                uint8_t *flags);
```

**Parameters:**
- `handle` - Session handle from `fn_open()`
- `http_status` - Output: HTTP status code (if applicable)
- `content_length` - Output: Content-Length (if available)
- `flags` - Output: info flags

**Info Flags:**
- `FN_INFO_HAS_STATUS` - HTTP status is valid
- `FN_INFO_HAS_LENGTH` - Content-Length is valid
- `FN_INFO_CONNECTED` - Connection is established
- `FN_INFO_PEER_CLOSED` - Peer has closed their side

### `fn_close()`

Close a connection and free the handle.

```c
uint8_t fn_close(fn_handle_t handle);
```

**Parameters:**
- `handle` - Session handle to close

**Returns:** `FN_OK` on success, error code on failure.

## Utilities

### `fn_error_string()`

Get a human-readable error string.

```c
const char *fn_error_string(uint8_t error);
```

**Parameters:**
- `error` - Error code from a failed operation

**Returns:** Static string describing the error.

### `fn_version()`

Get the library version string.

```c
const char *fn_version(void);
```

**Returns:** Version string (e.g., "1.0.0").

## Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0x00 | `FN_OK` | Success |
| 0x01 | `FN_ERR_NOT_FOUND` | Device not found |
| 0x02 | `FN_ERR_INVALID` | Invalid parameter or malformed request |
| 0x03 | `FN_ERR_BUSY` | Device is busy, retry later |
| 0x04 | `FN_ERR_NOT_READY` | Operation not ready, poll again |
| 0x05 | `FN_ERR_IO` | I/O error during operation |
| 0x06 | `FN_ERR_TIMEOUT` | Device did not respond in time |
| 0x07 | `FN_ERR_INTERNAL` | Internal error |
| 0x08 | `FN_ERR_UNSUPPORTED` | Operation not supported |
| 0x10 | `FN_ERR_TRANSPORT` | Transport layer error |
| 0x11 | `FN_ERR_URL_TOO_LONG` | URL exceeds maximum length |
| 0x12 | `FN_ERR_NO_HANDLES` | No free handles available |
| 0xFF | `FN_ERR_UNKNOWN` | Unknown/unexpected error |

## HTTP Methods

| Code | Name |
|------|------|
| 0x01 | `FN_METHOD_GET` |
| 0x02 | `FN_METHOD_POST` |
| 0x03 | `FN_METHOD_PUT` |
| 0x04 | `FN_METHOD_DELETE` |
| 0x05 | `FN_METHOD_HEAD` |

## Open Flags

| Flag | Value | Description |
|------|-------|-------------|
| `FN_OPEN_TLS` | 0x01 | Use TLS/HTTPS (for URLs without scheme) |
| `FN_OPEN_FOLLOW_REDIR` | 0x02 | Follow HTTP redirects |
| `FN_OPEN_ALLOW_EVICT` | 0x04 | Allow handle eviction under memory pressure |

## Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `FN_MAX_URL_LEN` | 256 | Maximum URL length |
| `FN_MAX_SESSIONS` | 4 | Maximum concurrent sessions |
| `FN_MAX_CHUNK_SIZE` | 512 | Maximum read/write chunk size |

## Protocol Capability Flags

These flags are returned by the server in the `fn_open()` response and used internally by the library. Applications typically don't need to check these directly, but understanding them helps explain offset behavior:

| Flag | Value | Description |
|------|-------|-------------|
| `FN_PROTO_FLAG_SEQUENTIAL_READ` | 0x01 | Reads must use sequential offsets (TCP/TLS) |
| `FN_PROTO_FLAG_SEQUENTIAL_WRITE` | 0x02 | Writes must use sequential offsets (TCP/TLS) |
| `FN_PROTO_FLAG_STREAMING` | 0x04 | Protocol is streaming, not request/response |

**Protocol flag values:**
- HTTP/HTTPS: `0x00` (random-access, no sequential requirement)
- TCP/TLS: `0x07` (streaming, sequential read/write required)

## Types

### `fn_handle_t`

Session handle type. Opaque handle returned by `fn_open()` and `fn_tcp_open()`.

```c
typedef uint8_t fn_handle_t;
```
