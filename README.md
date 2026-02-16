# fujinet-nio-lib

A clean, multi-platform 6502 library for communicating with FujiNet-NIO devices using the FujiBus protocol.

## Overview

**fujinet-nio-lib** is a modern C library designed for 8-bit applications that need to communicate with FujiNet-NIO devices. It provides:

- **Handle-based API** - Simple, intuitive session management
- **FujiBus protocol** - Binary protocol with SLIP framing
- **Multi-platform support** - Atari, Apple II, CoCo, C64, MS-DOS, and Linux
- **Multi-compiler support** - CC65, CMOC, Watcom, GCC
- **HTTP and TCP** - Both protocols supported from day one

## Features

- HTTP GET/POST/PUT/DELETE operations
- TCP socket connections
- TLS/HTTPS support (requires firmware TLS configuration)
- Automatic redirect following
- Streaming reads for large responses
- Caller-provided buffers (no hidden malloc)
- Minimal RAM footprint

## Current Status

**Working:**
- HTTP operations tested and working
- Linux native target for PC-based testing
- FujiBus protocol with SLIP framing
- Handle-based session management

**In Progress:**
- HTTPS/TLS requires ESP32 firmware configuration (see Known Issues)
- Atari SIO transport (assembly implementation complete, needs testing)
- Additional platform transports (Apple II, CoCo, etc.)

**Known Issues:**
- HTTPS connections fail on ESP32 firmware due to missing TLS certificate verification configuration. The ESP32 TLS stack needs `esp_tls_cfg_t` properly configured. HTTP works correctly.

## Requirements

### For Atari builds:
- CC65 compiler suite (cl65, ar65)
- GNU Make

### For CoCo builds:
- CMOC compiler
- LWTOOLS (lwar, lwasm)
- GNU Make

### For MS-DOS builds:
- Open Watcom (wcc, wlib)
- GNU Make

### For Linux builds (native testing):
- GCC compiler
- GNU Make

## Building

### Build all targets:
```bash
make
```

### Build for a specific platform:
```bash
make atari      # Atari 8-bit
make apple2     # Apple II
make coco       # Tandy CoCo
make msdos      # MS-DOS
make linux      # Native Linux (for testing)
```

### Clean build artifacts:
```bash
make clean
```

## Linux Native Testing

The Linux target allows you to build and test applications natively on your PC, communicating with a FujiNet-NIO device via serial port or PTY.

### Setting up the connection

Set the `FN_PORT` environment variable to specify the serial device:

```bash
# For ESP32 via USB-serial
export FN_PORT=/dev/ttyUSB0

# For POSIX fujinet-nio via PTY
export FN_PORT=/dev/pts/2

# Optionally set baud rate (default: 115200)
export FN_BAUD=115200
```

### Building a test application

```bash
# Build the library
make linux

# Compile your application
gcc -o my_test my_test.c -I include -L build -l:libfujinet-nio-linux.a

# Run with the serial port
FN_PORT=/dev/ttyUSB0 ./my_test
```

## Usage

### Basic HTTP GET

```c
#include "fujinet-nio.h"
#include <stdio.h>

int main(void) {
    uint8_t result;
    fn_handle_t handle;
    uint8_t buffer[512];
    uint16_t bytes_read;
    uint8_t flags;
    
    /* Initialize library */
    result = fn_init();
    if (result != FN_OK) {
        printf("Init failed: %s\n", fn_error_string(result));
        return 1;
    }
    
    /* Open HTTP connection */
    result = fn_open(&handle, FN_METHOD_GET, 
                     "https://example.com/api/data", 
                     FN_OPEN_TLS);
    if (result != FN_OK) {
        printf("Open failed: %s\n", fn_error_string(result));
        return 1;
    }
    
    /* Read response */
    uint32_t offset = 0;
    do {
        result = fn_read(handle, offset, buffer, sizeof(buffer) - 1,
                         &bytes_read, &flags);
        if (result == FN_OK && bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
            offset += bytes_read;
        }
    } while (result == FN_OK && !(flags & FN_READ_EOF));
    
    /* Close connection */
    fn_close(handle);
    return 0;
}
```

### TCP Connection

```c
#include "fujinet-nio.h"

int main(void) {
    fn_handle_t handle;
    uint8_t result;
    
    fn_init();
    
    /* Connect to TCP server */
    result = fn_tcp_open(&handle, "example.com", 8080);
    if (result != FN_OK) {
        return 1;
    }
    
    /* Send data */
    const char *request = "HELLO\n";
    uint16_t written;
    fn_write(handle, 0, (const uint8_t *)request, 6, &written);
    
    /* Receive response */
    uint8_t buffer[256];
    uint16_t bytes_read;
    uint8_t flags;
    fn_read(handle, 0, buffer, sizeof(buffer), &bytes_read, &flags);
    
    fn_close(handle);
    return 0;
}
```

## API Reference

### Initialization

```c
uint8_t fn_init(void);           // Initialize library
uint8_t fn_is_ready(void);       // Check if device is ready
```

### Network Operations

```c
uint8_t fn_open(fn_handle_t *handle, 
                uint8_t method,
                const char *url,
                uint8_t flags);

uint8_t fn_tcp_open(fn_handle_t *handle,
                    const char *host,
                    uint16_t port);

uint8_t fn_read(fn_handle_t handle,
                uint32_t offset,
                uint8_t *buf,
                uint16_t max_len,
                uint16_t *bytes_read,
                uint8_t *flags);

uint8_t fn_write(fn_handle_t handle,
                 uint32_t offset,
                 const uint8_t *data,
                 uint16_t len,
                 uint16_t *written);

uint8_t fn_info(fn_handle_t handle,
                uint16_t *http_status,
                uint32_t *content_length,
                uint8_t *flags);

uint8_t fn_close(fn_handle_t handle);
```

### Utilities

```c
const char *fn_error_string(uint8_t error);
const char *fn_version(void);
```

## Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0x00 | `FN_OK` | Success |
| 0x01 | `FN_ERR_INVALID` | Invalid parameter |
| 0x02 | `FN_ERR_BUSY` | Device busy |
| 0x03 | `FN_ERR_NOT_READY` | Not ready |
| 0x04 | `FN_ERR_IO` | I/O error |
| 0x05 | `FN_ERR_NO_MEMORY` | Out of memory |
| 0x06 | `FN_ERR_NOT_FOUND` | Not found |
| 0x07 | `FN_ERR_TIMEOUT` | Timeout |
| 0x08 | `FN_ERR_TRANSPORT` | Transport error |
| 0x09 | `FN_ERR_URL_TOO_LONG` | URL too long |
| 0x0A | `FN_ERR_NO_HANDLES` | No free handles |

## HTTP Methods

| Code | Name |
|------|------|
| 0x01 | `FN_METHOD_GET` |
| 0x02 | `FN_METHOD_POST` |
| 0x03 | `FN_METHOD_PUT` |
| 0x04 | `FN_METHOD_DELETE` |
| 0x05 | `FN_METHOD_HEAD` |

## Open Flags

| Flag | Description |
|------|-------------|
| `FN_OPEN_TLS` | Use TLS/HTTPS |
| `FN_OPEN_FOLLOW_REDIR` | Follow redirects |
| `FN_OPEN_ALLOW_EVICT` | Allow handle eviction |

## Directory Structure

```
fujinet-nio-lib/
├── Makefile              # Top-level build
├── makefiles/
│   ├── build.mk          # Core build logic
│   ├── targets.mk        # Platform definitions
│   ├── compiler.mk       # Compiler selection
│   ├── compiler-cc65.mk  # CC65 config
│   ├── compiler-cmoc.mk  # CMOC config
│   └── compiler-wcc.mk   # Watcom config
├── include/
│   ├── fujinet-nio.h     # Main API header
│   ├── fn_protocol.h     # Protocol definitions
│   └── fn_platform.h     # Platform interface
├── src/
│   ├── common/
│   │   ├── fn_slip.c     # SLIP encoding
│   │   ├── fn_packet.c   # Packet building
│   │   └── fn_network.c  # API implementation
│   └── platform/
│       ├── atari/        # Atari SIO transport
│       ├── apple2/       # Apple II SmartPort
│       ├── coco/         # CoCo Drivewire
│       └── msdos/        # MS-DOS TCP/serial
└── examples/
    └── http_get.c        # Example program
```

## Platform Support

| Platform | Transport | Compiler | Status |
|----------|-----------|----------|--------|
| Atari 8-bit | SIO | CC65 | ✅ Implemented |
| Apple II | SmartPort | CC65 | 🚧 Planned |
| Commodore 64 | IEC | CC65 | 🚧 Planned |
| Tandy CoCo | Drivewire | CMOC | 🚧 Planned |
| MS-DOS | TCP/Serial | Watcom | 🚧 Planned |

## Differences from fujinet-lib

| Feature | fujinet-lib (old) | fujinet-nio-lib (new) |
|---------|-------------------|----------------------|
| Protocol | Legacy SIO | FujiBus binary |
| Addressing | Device specs | Handles |
| Framing | Platform-specific | SLIP (common) |
| Build | Complex recursive | Simplified explicit |
| Platform code | Mixed with common | Clean separation |

## License

GPL v3 - See LICENSE file for details.

## Contributing

Contributions are welcome! Please ensure:

1. Code follows the existing style
2. Platform-specific code goes in `src/platform/<platform>/`
3. Common code goes in `src/common/`
4. All functions are documented with Doxygen-style comments

## Related Projects

- [fujinet-nio](https://github.com/FujiNetWIFI/fujinet-nio) - FujiNet firmware rewrite
- [fujinet-lib](https://github.com/FujiNetWIFI/fujinet-lib) - Legacy library (reference)
- [cc65](https://github.com/cc65/cc65) - 6502 C compiler
