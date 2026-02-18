# Building fujinet-nio-lib

This document describes how to build the fujinet-nio-lib library for various platforms.

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

## Build Output

The build produces the following outputs:

```
fujinet-nio-lib/
  build/
    fujinet-nio-atari.lib    # Atari library
    fujinet-nio-linux.a      # Linux static library
    fujinet-nio-apple2.lib   # Apple II library (planned)
    fujinet-nio-coco.lib     # CoCo library (planned)
    fujinet-nio-msdos.lib    # MS-DOS library (planned)
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

## Cross-Compilation Notes

### Atari (CC65)

The library uses the CC65 compiler's `-t atari` target. Key considerations:

- No floating point support
- No large stack allocations (use static buffers)
- No mixed declarations and code (C89 style)
- Limited standard library

### Platform-Specific Code

Platform-specific transport code is located in:
- `src/platform/atari/` - Atari SIO transport
- `src/platform/apple2/` - Apple II SmartPort (planned)
- `src/platform/coco/` - CoCo Drivewire (planned)
- `src/platform/msdos/` - MS-DOS TCP/serial (planned)
- `src/platform/posix/` - Linux/POSIX transport

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
    ├── Makefile          # Examples build
    └── network/          # Network examples
```
