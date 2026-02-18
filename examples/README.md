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
      clock/                 # Clock examples
        clock_test.o
    atari/                   # Atari target
      network/
        http_get.o
        tcp_get.o
      clock/
        clock_test.o
  bin/                       # Final executables
    linux/
      http_get
      tcp_get
      clock_test
    atari/
      http_get.xex
      tcp_get.xex
      clock_test.xex
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
make clock_test TARGET=linux
```

### Build Atari examples with custom configuration:
```bash
# Build with TLS enabled and custom host/port
make TARGET=atari FN_TCP_HOST=example.com FN_TCP_PORT=443 FN_TCP_TLS=1
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

# Run HTTP example
./bin/linux/http_get

# Run TCP example (connects to localhost:7777 by default)
./bin/linux/tcp_get

# Run TLS example with custom host
FN_TCP_HOST=127.0.0.1 FN_TCP_PORT=7778 FN_TCP_TLS=1 ./bin/linux/tcp_get

# Run with full URL override
FN_TEST_URL="tls://echo.fujinet.online:6001?testca=1" ./bin/linux/tcp_get

# Run clock example
./bin/linux/clock_test
```

### Atari Examples

Copy the `.xex` files to your Atari storage medium (SD card, ATR disk, etc.) and run them from the Atari DOS menu.

**Note:** Atari examples use compile-time configuration for TCP settings. To change the target server, rebuild with different parameters:

```bash
# Build for a specific server
make TARGET=atari FN_TCP_HOST=192.168.1.100 FN_TCP_PORT=7777

# Build with TLS enabled
make TARGET=atari FN_TCP_HOST=example.com FN_TCP_PORT=443 FN_TCP_TLS=1
```

The clock example works without any configuration - it simply reads and displays the current time from the FujiNet device.

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
  - TLS encryption support (via `tls://` URL scheme)
  - Sending data to server
  - Half-close (FIN) for echo servers
  - Idle timeout for response detection
  - Reading response data
  - Connection state monitoring

### Clock Examples (`clock/`)

- **clock_test** - Clock device demonstration showing:
  - Getting the current time from FujiNet
  - Setting the time on the device
  - Human-readable time formatting (year, month, day, hour, minute, second)
  - Raw time value display (Unix timestamp)
  - Note: Time setting may be disabled on some FujiNet configurations

### Disk Examples (`disk/`) - Planned

Examples for disk device operations.

## Configuration

### Linux (Runtime Environment Variables)

Examples can be configured via environment variables at runtime:

#### HTTP Get Example
- `FN_TEST_URL` - URL to fetch (default: `http://localhost:8080/get`)

#### TCP Get Example
- `FN_TEST_URL` - Full URL (e.g., `tcp://host:port` or `tls://host:port?testca=1`)
- `FN_TCP_HOST` - Host to connect to (default: `localhost`)
- `FN_TCP_PORT` - Port to connect to (default: `7777`)
- `FN_TCP_TLS` - Set to `1` to enable TLS (default: disabled)
- `FN_TCP_REQUEST` - Custom request string to send

#### Common
- `FN_PORT` - Serial port device (default: `/dev/ttyUSB0`)

### Atari (Compile-Time Defines)

Atari examples are configured at compile time via Makefile variables:

```bash
make TARGET=atari FN_TCP_HOST=192.168.1.100 FN_TCP_PORT=7778 FN_TCP_TLS=1
```

| Variable | Default | Description |
|----------|---------|-------------|
| `FN_TCP_HOST` | `localhost` | Target host |
| `FN_TCP_PORT` | `7777` | Target port |
| `FN_TCP_TLS` | `0` | Enable TLS (0=disabled, 1=enabled) |

## Adding New Examples

1. Create a new `.c` file in the appropriate category folder (e.g., `network/my_example.c`)
2. Add the example name to the appropriate list in the `Makefile`:
   ```makefile
   NETWORK_EXAMPLES := http_get tcp_get my_example
   ```
3. Build with `make my_example TARGET=<target>`

### Adding a New Category

1. Create a new folder (e.g., `disk/`)
2. Add your example files
3. Add a new example list in the Makefile:
    ```makefile
    DISK_EXAMPLES := disk_info
    ```
4. Add to `ALL_EXAMPLES`:
    ```makefile
    ALL_EXAMPLES := $(NETWORK_EXAMPLES) $(CLOCK_EXAMPLES) $(DISK_EXAMPLES)
    ```
5. Add a build directory rule and pattern rule for the new category

## Testing with Local Servers

For testing the TCP/TLS examples locally, you can use the test services from the fujinet-nio project:

```bash
# Start TCP echo server on port 7777
./scripts/start_test_services.sh tcp

# Start TLS echo server on port 7778
./scripts/start_test_services.sh tls

# Run the examples
FN_TCP_HOST=localhost FN_TCP_PORT=7777 ./bin/linux/tcp_get
FN_TCP_HOST=localhost FN_TCP_PORT=7778 FN_TCP_TLS=1 ./bin/linux/tcp_get
```
