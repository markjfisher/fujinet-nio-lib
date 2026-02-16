# fujinet-nio-lib Architecture Plan

## Overview

**fujinet-nio-lib** is a new 6502 library for communicating with FujiNet-NIO devices using the FujiBus protocol. It replaces the legacy fujinet-lib with a cleaner, more maintainable design that:

- Uses **handles** instead of device specs (per FujiBus design)
- Supports the **FujiBus binary protocol** with SLIP framing
- Provides a **clean platform abstraction layer**
- Builds with **multiple compilers** (cc65, CMOC, wcc, etc.)
- Minimizes code duplication through **layered architecture**

---

## Key Design Principles

### 1. Layered Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Application API                       │
│         fn_network_open, fn_network_read, etc.          │
└─────────────────────────────────────────────────────────┘
                           │
┌─────────────────────────────────────────────────────────┐
│                   Protocol Layer                         │
│      FujiBus packet construction, SLIP encoding         │
└─────────────────────────────────────────────────────────┘
                           │
┌─────────────────────────────────────────────────────────┐
│                   Transport Layer                        │
│    Platform-specific I/O: SIO, SmartPort, Drivewire     │
└─────────────────────────────────────────────────────────┘
```

### 2. Handle-Based API

Unlike the old library which used device specs like `N1:HTTPS://example.com/`, the new library uses:

- **Handles** (u16) returned from `fn_open()` 
- Handles represent sessions with the NetworkDevice (WireDeviceId 0xFD)
- All subsequent operations use the handle

### 3. Platform Abstraction

Platform-specific code is isolated to a thin transport layer:
- **Atari**: SIO via DCB (assembly)
- **Apple II**: SmartPort (assembly)
- **CoCo**: Drivewire (C/assembly)
- **MSDOS**: TCP/serial (C)

Common code handles:
- FujiBus packet construction
- SLIP encoding/decoding
- Protocol state machine
- API implementation

---

## Directory Structure

```
fujinet-nio-lib/
├── Makefile                    # Top-level build
├── makefiles/
│   ├── build.mk               # Core build logic (simplified)
│   ├── compiler-cc65.mk       # cc65 (atari, apple2, c64, etc.)
│   ├── compiler-cmoc.mk       # CMOC (coco)
│   ├── compiler-wcc.mk        # Watcom (msdos)
│   ├── compiler-zcc.mk        # z88dk (pmd85, z80 targets)
│   ├── os.mk                  # OS detection, platform mapping
│   └── targets.mk             # Target definitions
├── include/
│   ├── fujinet-nio.h          # Main public API header
│   ├── fn_protocol.h          # FujiBus protocol definitions
│   ├── fn_errors.h            # Error codes
│   └── fn_config.h            # Build configuration
├── src/
│   ├── common/
│   │   ├── fn_slip.c          # SLIP encoding/decoding
│   │   ├── fn_packet.c        # FujiBus packet construction
│   │   ├── fn_network.c       # Network API implementation
│   │   └── fn_utils.c         # Utility functions
│   └── platform/
│       ├── atari/
│       │   ├── fn_transport.s # SIO low-level (assembly)
│       │   └── fn_platform.h  # Atari-specific definitions
│       ├── apple2/
│       │   ├── fn_transport.s # SmartPort low-level
│       │   └── fn_platform.h
│       ├── coco/
│       │   ├── fn_transport.c # Drivewire low-level
│       │   └── fn_platform.h
│       ├── c64/
│       │   ├── fn_transport.s # IEC bus low-level
│       │   └── fn_platform.h
│       └── msdos/
│           ├── fn_transport.c # TCP/serial transport
│           └── fn_platform.h
├── examples/
│   ├── http_get.c             # Simple HTTP GET example
│   └── tcp_client.c           # TCP client example
└── README.md
```

---

## API Design

### Main Header: `fujinet-nio.h`

```c
#ifndef FUJINET_NIO_H
#define FUJINET_NIO_H

#include <stdint.h>

/* Handle type for network sessions */
typedef uint16_t fn_handle_t;

/* Error codes */
#define FN_OK              0x00
#define FN_ERR_INVALID     0x01
#define FN_ERR_BUSY        0x02
#define FN_ERR_NOT_READY   0x03
#define FN_ERR_IO          0x04
#define FN_ERR_NO_MEMORY   0x05
#define FN_ERR_NOT_FOUND   0x06

/* HTTP methods */
#define FN_METHOD_GET      0x01
#define FN_METHOD_POST     0x02
#define FN_METHOD_PUT      0x03
#define FN_METHOD_DELETE   0x04
#define FN_METHOD_HEAD     0x05

/* Open flags */
#define FN_FLAG_TLS           0x01
#define FN_FLAG_FOLLOW_REDIR  0x02

/* Read flags (returned in flags param) */
#define FN_READ_EOF           0x01
#define FN_READ_TRUNCATED     0x02

/* Info flags */
#define FN_INFO_HAS_STATUS    0x04
#define FN_INFO_HAS_LENGTH    0x02
#define FN_INFO_CONNECTED     0x10  /* TCP: socket connected */
#define FN_INFO_PEER_CLOSED   0x20  /* TCP: peer closed connection */

/* Initialize the library (platform-specific) */
uint8_t fn_init(void);

/* Open a network session 
 * Returns handle in *handle, returns error code
 * For TCP: use method=0 and url="tcp://host:port"
 * For HTTP: use FN_METHOD_* and url="http://..." or "https://..."
 */
uint8_t fn_open(fn_handle_t *handle, 
                uint8_t method,
                const char *url,
                uint8_t flags);

/* TCP-specific open (convenience wrapper) */
uint8_t fn_tcp_open(fn_handle_t *handle,
                    const char *host,
                    uint16_t port);

/* Write data to session (for POST/PUT) */
uint8_t fn_write(fn_handle_t handle,
                 uint32_t offset,
                 const uint8_t *data,
                 uint16_t len,
                 uint16_t *written);

/* Read data from session */
uint8_t fn_read(fn_handle_t handle,
                uint32_t offset,
                uint8_t *buf,
                uint16_t max_len,
                uint16_t *bytes_read,
                uint8_t *flags);

/* Get session info (HTTP status, content length) */
uint8_t fn_info(fn_handle_t handle,
                uint16_t *http_status,
                uint64_t *content_length,
                uint8_t *flags);

/* Close a session */
uint8_t fn_close(fn_handle_t handle);

/* Get error description string */
const char *fn_error_string(uint8_t error);

#endif /* FUJINET_NIO_H */
```

### Protocol Header: `fn_protocol.h`

```c
#ifndef FN_PROTOCOL_H
#define FN_PROTOCOL_H

#include <stdint.h>

/* Wire device IDs */
#define FN_DEVICE_NETWORK    0xFD
#define FN_DEVICE_DISK       0xFC
#define FN_DEVICE_FILE       0xFE
#define FN_DEVICE_FUJI       0x70

/* Network commands */
#define FN_CMD_OPEN    0x01
#define FN_CMD_READ    0x02
#define FN_CMD_WRITE   0x03
#define FN_CMD_CLOSE   0x04
#define FN_CMD_INFO    0x05

/* SLIP special bytes */
#define SLIP_END       0xC0
#define SLIP_ESCAPE    0xDB
#define SLIP_ESC_END   0xDC
#define SLIP_ESC_ESC   0xDD

/* Protocol version */
#define FN_PROTOCOL_VERSION  0x01

/* FujiBus packet structure (for reference) */
typedef struct {
    uint8_t  device;      /* WireDeviceId */
    uint8_t  command;     /* Command byte */
    uint8_t  param_count; /* Number of parameters */
    uint16_t data_len;    /* Payload length */
} fn_packet_header_t;

#endif /* FN_PROTOCOL_H */
```

---

## Build System Design

### Top-Level Makefile (Simplified)

```makefile
# fujinet-nio-lib Makefile
TARGETS = atari apple2 apple2enh c64 coco msdos
PROGRAM := fujinet-nio

.PHONY: all clean $(TARGETS)

all:
	@for target in $(TARGETS); do \
		echo "Building $$target"; \
		$(MAKE) -f makefiles/build.mk TARGET=$$target; \
	done

$(TARGETS):
	$(MAKE) -f makefiles/build.mk TARGET=$@

clean:
	rm -rf build/ obj/ dist/
```

### Core Build Logic: `makefiles/build.mk`

Key improvements over fujinet-lib:

1. **Simpler structure**: No recursive wildcard, explicit source lists
2. **Clear platform mapping**: One file per platform
3. **Compiler abstraction**: Clean interface for different toolchains
4. **Dependency generation**: Automatic header dependencies

```makefile
# Core build logic
TARGET ?= atari
-include makefiles/os.mk
-include makefiles/compiler-$(COMPILER_FAMILY).mk

# Platform sources
PLATFORM_DIR := src/platform/$(PLATFORM)
COMMON_DIR := src/common

# Explicit source lists (cleaner than wildcards)
COMMON_SRCS := $(COMMON_DIR)/fn_slip.c \
               $(COMMON_DIR)/fn_packet.c \
               $(COMMON_DIR)/fn_network.c

PLATFORM_SRCS := $(wildcard $(PLATFORM_DIR)/*.c $(PLATFORM_DIR)/*.s)

SRCS := $(COMMON_SRCS) $(PLATFORM_SRCS)
OBJS := $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
OBJS := $(patsubst %.s,$(OBJDIR)/%.o,$(OBJS))

# Build rules...
```

### Compiler Abstraction

Each compiler file defines:

```makefile
# compiler-cc65.mk
CC := cl65
AR := ar65
CFLAGS := -Osir -I include
ASFLAGS := -I include
OBJEXT := .o

# Compile rule
define COMPILE_C
$(CC) -t $(TARGET) -c $(CFLAGS) -o $@ $<
endef
```

---

## Platform Transport Layer

### Interface

Each platform must implement:

```c
/* fn_platform.h - Platform transport interface */

/* Send a FujiBus packet (SLIP-encoded) and receive response */
uint8_t fn_transport_exchange(const uint8_t *request, 
                               uint16_t req_len,
                               uint8_t *response, 
                               uint16_t *resp_len);

/* Check if transport is ready */
uint8_t fn_transport_ready(void);

/* Initialize transport */
uint8_t fn_transport_init(void);
```

### Atari Implementation (SIO)

The Atari uses SIO via the DCB. The assembly code:
1. Sets up DCB with device ID (0xFD for Network)
2. Constructs SIO command frame
3. Sends packet data
4. Receives response

```asm
; fn_transport.s - Atari SIO transport
; 
; uint8_t fn_transport_exchange(const uint8_t *request, 
;                                uint16_t req_len,
;                                uint8_t *response, 
;                                uint16_t *resp_len);

.export _fn_transport_exchange

.include "atari.inc"

_fn_transport_exchange:
    ; Setup DCB for SIO operation
    ; Send command frame
    ; Transfer data
    ; Receive response
    ; Return status
```

### Apple II Implementation (SmartPort)

Similar approach using SmartPort calls.

### CoCo Implementation (Drivewire)

Uses Drivewire protocol over serial.

---

## FujiBus Packet Construction

### Common Code: `fn_packet.c`

```c
#include "fn_protocol.h"
#include "fn_slip.h"

/* Build an Open request packet */
uint16_t fn_build_open_packet(uint8_t *buffer,
                               uint8_t method,
                               uint8_t flags,
                               const char *url)
{
    uint8_t *p = buffer;
    uint16_t url_len = strlen(url);
    
    /* FujiBus header */
    *p++ = FN_DEVICE_NETWORK;   /* device */
    *p++ = FN_CMD_OPEN;          /* command */
    *p++ = 0;                    /* param count placeholder */
    
    /* Open payload */
    *p++ = FN_PROTOCOL_VERSION;
    *p++ = method;
    *p++ = flags;
    
    /* URL (length-prefixed) */
    *p++ = url_len & 0xFF;
    *p++ = (url_len >> 8) & 0xFF;
    memcpy(p, url, url_len);
    p += url_len;
    
    /* No headers for simple open */
    *p++ = 0; *p++ = 0;  /* header count = 0 */
    *p++ = 0; *p++ = 0;  /* body len hint = 0 */
    *p++ = 0; *p++ = 0;  /* resp header count = 0 */
    
    return p - buffer;
}

/* Build a Read request packet */
uint16_t fn_build_read_packet(uint8_t *buffer,
                               fn_handle_t handle,
                               uint32_t offset,
                               uint16_t max_bytes)
{
    uint8_t *p = buffer;
    
    *p++ = FN_DEVICE_NETWORK;
    *p++ = FN_CMD_READ;
    *p++ = 4;  /* param count */
    
    *p++ = FN_PROTOCOL_VERSION;
    *p++ = handle & 0xFF;
    *p++ = (handle >> 8) & 0xFF;
    *p++ = offset & 0xFF;
    *p++ = (offset >> 8) & 0xFF;
    *p++ = (offset >> 16) & 0xFF;
    *p++ = (offset >> 24) & 0xFF;
    *p++ = max_bytes & 0xFF;
    *p++ = (max_bytes >> 8) & 0xFF;
    
    return p - buffer;
}
```

### SLIP Encoding: `fn_slip.c`

```c
#include "fn_protocol.h"

/* Encode data with SLIP framing */
uint16_t fn_slip_encode(const uint8_t *input, uint16_t in_len,
                        uint8_t *output)
{
    uint16_t out_len = 0;
    
    output[out_len++] = SLIP_END;
    
    for (uint16_t i = 0; i < in_len; i++) {
        uint8_t b = input[i];
        if (b == SLIP_END) {
            output[out_len++] = SLIP_ESCAPE;
            output[out_len++] = SLIP_ESC_END;
        } else if (b == SLIP_ESCAPE) {
            output[out_len++] = SLIP_ESCAPE;
            output[out_len++] = SLIP_ESC_ESC;
        } else {
            output[out_len++] = b;
        }
    }
    
    output[out_len++] = SLIP_END;
    return out_len;
}

/* Decode SLIP-framed data */
uint16_t fn_slip_decode(const uint8_t *input, uint16_t in_len,
                        uint8_t *output)
{
    uint16_t out_len = 0;
    uint16_t i = 0;
    
    /* Skip leading END */
    if (in_len > 0 && input[0] == SLIP_END) i++;
    
    for (; i < in_len; i++) {
        uint8_t b = input[i];
        if (b == SLIP_END) break;
        if (b == SLIP_ESCAPE) {
            i++;
            if (i >= in_len) break;
            b = input[i];
            if (b == SLIP_ESC_END) output[out_len++] = SLIP_END;
            else if (b == SLIP_ESC_ESC) output[out_len++] = SLIP_ESCAPE;
        } else {
            output[out_len++] = b;
        }
    }
    
    return out_len;
}
```

---

## Implementation Strategy

### Phase 1: Core Infrastructure

1. Create directory structure
2. Implement build system (Makefiles)
3. Create protocol headers (`fn_protocol.h`, `fn_errors.h`)
4. Implement SLIP encoding/decoding
5. Implement FujiBus packet construction

### Phase 2: Platform Layer

1. Implement Atari SIO transport (assembly)
2. Test with fujinet-nio device
3. Implement Apple II SmartPort transport
4. Implement CoCo Drivewire transport
5. Implement MSDOS transport

### Phase 3: API Layer

1. Implement `fn_init()` / `fn_open()`
2. Implement `fn_read()` / `fn_write()`
3. Implement `fn_info()` / `fn_close()`
4. Add error handling and status codes

### Phase 4: Testing & Documentation

1. Create example programs
2. Write README with usage examples
3. Test on real hardware
4. Test with emulators

---

## Comparison: Old vs New

| Aspect | fujinet-lib (old) | fujinet-nio-lib (new) |
|--------|-------------------|----------------------|
| Protocol | Legacy SIO commands | FujiBus binary |
| Addressing | Device specs | Handles |
| Framing | Platform-specific | SLIP (common) |
| Build | Complex recursive make | Simplified explicit make |
| Platform code | Mixed with common | Clean separation |
| API style | Device-centric | Session-centric |

---

## Design Decisions

Based on discussion, the following decisions have been made:

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Header capture | Defer to v2 | Keep v1 minimal, add complexity later |
| TCP support | Include from start | Many applications need raw sockets |
| Buffer management | Caller-provided | More flexible, reentrant, no hidden malloc |
| Maximum URL length | 256 bytes | Reasonable for 8-bit systems |
| Concurrent sessions | 4 sessions | Matches device limit, conserves RAM |

---

## Next Steps

1. ✅ Review this plan with user
2. ✅ Refine based on feedback
3. Switch to Code mode for implementation
