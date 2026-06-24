# fujinet-nio-lib

A clean, multi-platform 6502 library for communicating with FujiNet-NIO devices using the FujiBus protocol.

## Overview

**fujinet-nio-lib** is a modern C library designed for 8-bit applications that need to communicate with FujiNet-NIO devices. It provides:

- **Handle-based API** - Simple, intuitive session management
- **FujiBus protocol** - Binary protocol with SLIP framing
- **Multi-platform support** - Atari, Apple II, CoCo, C64, MS-DOS, and Linux
- **Multi-compiler support** - CC65, CMOC, Watcom, GCC
- **HTTP and TCP/TLS** - All protocols supported

## Features

- HTTP GET/POST/PUT/DELETE operations
- TCP socket connections
- TLS/HTTPS support (via `tls://` URL scheme)
- Automatic redirect following
- Streaming reads for large responses
- Caller-provided buffers (no hidden malloc)
- Minimal RAM footprint

## Documentation

- **[Developer setup](docs/DEVELOPMENT.md)** — Beebium BBC tests (three env vars)
- **[API Reference](docs/api.md)** - Complete API documentation
- **[Building](docs/building.md)** - Build instructions for all platforms
- **[Transport Backends](docs/transport-backends.md)** - How platform library targets map to NIO channels
- **[Examples](examples/README.md)** - Example applications and usage

## Quick Start

### Building

```bash
# Build for Linux (native testing)
make linux

# Build for MS-DOS
make msdos

# Build for Atari
make atari
```

### Basic HTTP GET

```c
#include "fujinet-nio.h"
#include <stdio.h>

int main(void) {
    fn_handle_t handle;
    uint8_t buffer[512];
    uint16_t bytes_read;
    uint8_t flags;
    
    fn_init();
    
    // Open HTTP connection
    fn_open(&handle, FN_METHOD_GET, "https://example.com/api", 0);
    
    // Read response
    uint32_t offset = 0;
    do {
        fn_read(handle, offset, buffer, sizeof(buffer) - 1, &bytes_read, &flags);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
            offset += bytes_read;
        }
    } while (!(flags & FN_READ_EOF));
    
    fn_close(handle);
    return 0;
}
```

### TCP/TLS Connection

```c
#include "fujinet-nio.h"

int main(void) {
    fn_handle_t handle;
    uint8_t buffer[256];
    uint16_t bytes_read, written;
    uint8_t flags;
    
    fn_init();
    
    // Connect using URL scheme (tcp:// or tls://)
    fn_open(&handle, 0, "tls://example.com:443", 0);
    
    // Send data
    const char *request = "HELLO\n";
    fn_write(handle, 0, (const uint8_t *)request, 6, &written);
    
    // Receive response
    fn_read(handle, 0, buffer, sizeof(buffer), &bytes_read, &flags);
    
    fn_close(handle);
    return 0;
}
```

## Current Status

**Working:**
- HTTP operations tested and working
- TCP/TLS operations tested and working
- Linux native target for PC-based testing using the common FujiBus/NIO stream transport over a POSIX serial or PTY channel
- MS-DOS target builds with Open Watcom and uses the common FujiBus/NIO stream transport over a COM serial channel
- BBC target now uses the installed `fn-rom` network ROM through MOS channel opens and `OSWORD &78`
- FujiBus protocol with SLIP framing
- Handle-based session management

**In Progress:**
- Atari SIO transport (assembly implementation complete, needs testing)
- Additional platform transports (Apple II, CoCo, etc.)

**Known Issues:**
- BBC `fn_info()` currently reports connectivity only and does not expose HTTP status/content length yet.
- BBC currently supports GET, PUT, POST, and raw TCP/TLS opens. HEAD and DELETE are not yet mapped through the `fn-rom` MOS-facing open ABI.
- Atari hardware needs testing with a build of fujinet-nio for Atari SIO.

## Platform Support

| Platform | Transport | Compiler | Status |
|----------|-----------|----------|--------|
| Atari 8-bit | SIO | CC65 | ✅ Implemented |
| BBC Micro | `fn-rom` MOS + `OSWORD &78` | CC65 | ✅ Implemented |
| Apple II | SmartPort | CC65 | 🚧 Planned |
| Commodore 64 | IEC | CC65 | 🚧 Planned |
| Tandy CoCo | Drivewire | CMOC | 🚧 Planned |
| MS-DOS | COM serial | Watcom | ✅ Implemented |
| Linux | stream over POSIX serial/PTY | GCC | ✅ Working |

## Differences from fujinet-lib

| Feature | fujinet-lib (old) | fujinet-nio-lib (new) |
|---------|-------------------|----------------------|
| Protocol | Legacy SIO | FujiBus binary |
| Addressing | Device specs | Handles |
| Framing | Platform-specific | SLIP (common) |
| Build | Complex recursive | Simplified explicit |
| Platform code | Mixed with common | Clean separation |
| TLS | HTTP only | HTTP and raw TCP |

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
