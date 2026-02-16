# FujiNet-NIO Library Examples

This folder contains example applications demonstrating how to use the fujinet-nio-lib library.

## Building Examples

### Build all examples for a specific target:
```bash
make TARGET=linux    # Build for Linux (native testing)
make TARGET=atari    # Build for Atari 8-bit
```

### Build a specific example:
```bash
make http_get TARGET=linux
```

### Clean build artifacts:
```bash
make clean
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
```

### Atari Examples

Copy the `.xex` files to your Atari storage medium (SD card, ATR disk, etc.) and run them from the Atari DOS menu.

## Example Categories

### Network Examples (`network/`)

- **http_get** - Simple HTTP GET request demonstrating:
  - Library initialization
  - Opening an HTTP connection with TLS
  - Getting response info (status, content-length)
  - Reading data in chunks
  - Handling EOF
  - Closing the connection

## Adding New Examples

1. Create a new `.c` file in the appropriate category folder (e.g., `network/my_example.c`)
2. Add the example name to the appropriate list in the `Makefile`:
   ```makefile
   NETWORK_EXAMPLES := http_get my_example
   ```
3. Build with `make my_example TARGET=<target>`
