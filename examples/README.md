# FujiNet-NIO Library Examples

This folder contains example applications demonstrating how to use the fujinet-nio-lib library.

## Build Structure

The build system organizes artifacts into separate directories:

```
examples/
  build/                    # Intermediate build files
    linux/                   # Linux target
      network/               # Network examples
        http_get.o
        tcp_get.o
    atari/                   # Atari target
      network/
        http_get.o
        tcp_get.o
  bin/                       # Final executables
    linux/
      http_get
      tcp_get
    atari/
      http_get.xex
      tcp_get.xex
```

## Building Examples

### Build all examples for a specific target:
```bash
make TARGET=linux    # Build for Linux (native testing)
make TARGET=atari    # Build for Atari 8-bit
```

### Build a specific example:
```bash
make http_get TARGET=linux
make tcp_get TARGET=linux
```

### Debug the build configuration:
```bash
make info TARGET=linux
```

### Clean build artifacts:
```bash
make clean           # Clean current target only
make clean-all       # Clean all targets
```

## Running Examples

### Linux Examples

Set the `FN_PORT` environment variable to specify the serial device:

```bash
# For ESP32 via USB-serial
export FN_PORT=/dev/ttyUSB0

# For POSIX fujinet-nio via PTY
export FN_PORT=/dev/pts/2

# Run the example
./bin/linux/http_get
./bin/linux/tcp_get
```

### Atari Examples

Copy the `.xex` files to your Atari storage medium (SD card, ATR disk, etc.) and run them from the Atari DOS menu.

## Example Categories

### Network Examples (`network/`)

- **http_get** - Simple HTTP GET request demonstrating:
  - Library initialization
  - Opening an HTTP connection
  - Getting response info (status, content-length)
  - Reading data in chunks
  - Handling EOF
  - Closing the connection

- **tcp_get** - TCP/TLS client demonstrating:
  - TCP socket connections
  - TLS encryption support
  - Sending data to server
  - Reading response data
  - Connection state monitoring
  - Peer close detection

### Clock Examples (`clock/`) - Planned

Examples for clock/time synchronization.

### Disk Examples (`disk/`) - Planned

Examples for disk device operations.

## Environment Variables

Examples can be configured via environment variables:

### HTTP Get Example
- `FN_TEST_URL` - URL to fetch (default: `http://localhost:8080/get`)

### TCP Get Example
- `FN_TCP_HOST` - Host to connect to (default: `localhost`)
- `FN_TCP_PORT` - Port to connect to (default: `8080`)
- `FN_TCP_TLS` - Set to `1` to enable TLS (default: disabled)
- `FN_TCP_REQUEST` - Custom request string to send

### Common
- `FN_PORT` - Serial port device (default: `/dev/ttyUSB0`)

## Adding New Examples

1. Create a new `.c` file in the appropriate category folder (e.g., `network/my_example.c`)
2. Add the example name to the appropriate list in the `Makefile`:
   ```makefile
   NETWORK_EXAMPLES := http_get tcp_get my_example
   ```
3. Build with `make my_example TARGET=<target>`

### Adding a New Category

1. Create a new folder (e.g., `clock/`)
2. Add your example files
3. Add a new example list in the Makefile:
   ```makefile
   CLOCK_EXAMPLES := clock_sync
   ```
4. Add to `ALL_EXAMPLES`:
   ```makefile
   ALL_EXAMPLES := $(NETWORK_EXAMPLES) $(CLOCK_EXAMPLES)
   ```
5. Add a build directory rule and pattern rule for the new category
