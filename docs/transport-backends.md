# Transport Backends

`fujinet-nio` firmware can expose the FujiBus/NIO protocol through multiple
transports and channels. `fujinet-nio-lib` is narrower by design: one library
build selects one channel at compile time.

That channel can be any physical path that can carry the selected transport's
framing. The public library API is not serial-specific, and the common stream
transport is not COM-port-specific or POSIX-specific. The currently implemented
MS-DOS channel is COM serial; the currently implemented Linux channel is POSIX
serial/PTY.

## Library Layers

The library has four practical layers:

1. Public API: `fn_open`, `fn_read`, `fn_write`, `fn_close`, clock helpers, and
   platform extension helpers.
2. Common protocol core: packet construction, response parsing, session state,
   and checksum handling.
3. Transport core: frame exchange over a class of channels. The current common
   stream transport owns SLIP framing and calls byte-channel functions.
4. Channel backend: the target-specific implementation of
   `fn_channel_init`, `fn_channel_ready`, `fn_channel_read_byte`,
   `fn_channel_write_byte`, `fn_channel_drain_rx`, and `fn_channel_close`.

For direct transports that cannot sensibly share `fn_transport_stream.c`, a
platform can still implement
   `fn_transport_init`, `fn_transport_ready`, `fn_transport_exchange`, and
   `fn_platform_name` directly.

The channel backend owns physical I/O only. It must not build device packets,
calculate checksums, parse responses, or know about FujiNet devices.

## Current Backends

| Target | Backend | Notes |
|--------|---------|-------|
| `linux` | common stream transport over POSIX serial/PTY channel | Runtime device path through `FN_PORT`. |
| `msdos` | common stream transport over COM serial channel | Direct 8250-compatible UART access. Defaults to COM1 at 115200 baud. |
| `bbc`, `bbc-clib` | `fn-rom`/MOS channels | Does not use the common direct transport stack. BBC delegates transport details to `fn-rom`. |
| `atari` | SIO | Direct target-specific transport. |

## Is MS-DOS Serial-Only?

The `msdos` target currently selects the COM serial channel because
`src/platform/msdos/fn_channel_serial.c` implements 8250-compatible byte I/O.

That is a build-target/channel choice, not a public API or transport-core
limitation. The same common protocol and stream transport code can support a
future MS-DOS parallel-port channel if that channel implements the byte-channel
interface and presents the same ordered byte-stream semantics.

## Why Not Runtime Transport Selection?

Runtime channel selection is possible on larger targets, but it should not be the
default model for this repository.

Most supported machines are small. A runtime channel table, multiple compiled
channels, extra configuration parsing, and channel-specific persistent buffers
would increase code size and RAM pressure. On BBC in particular, archive member
boundaries and BSS usage are part of correctness because unused code and hidden
buffers can make otherwise small applications fail.

For that reason, the preferred model is compile-time channel selection.

## Recommended Future Shape

Add new physical channels as named build targets, each linking one channel
backend:

| Future target | Platform directory | Backend |
|---------------|--------------------|---------|
| `msdos` or `msdos-serial` | `src/platform/msdos/` or `src/platform/msdos_serial/` | stream transport over COM/RS-232 |
| `msdos-parallel` | `src/platform/msdos_parallel/` | stream transport over PC parallel port |
| `bbc`, `bbc-clib` | `src/platform/bbc/` | `fn-rom`/MOS channel facade |
| `bbc-userbus` | `src/platform/bbc_userbus/` | Direct BBC user-bus backend |
| `bbc-1mhzbus` | `src/platform/bbc_1mhzbus/` | Direct BBC 1 MHz bus backend |

The target name should describe the host platform and the physical channel when
there is more than one plausible backend for a platform.

## Adding An MS-DOS Parallel Backend

The least disruptive route is:

1. Add a target mapping in `makefiles/targets.mk`, for example:

   ```make
   PLATFORM_msdos-parallel := msdos_parallel
   COMPILER_FAMILY_msdos-parallel := wcc
   TRANSPORT_FAMILY_msdos-parallel := stream
   TARGET_CFLAGS_msdos-parallel :=
   TARGET_ASFLAGS_msdos-parallel :=
   ```

2. Add `msdos-parallel` to `TARGETS` only when it is ready to build by default.
3. Create `src/platform/msdos_parallel/fn_channel_parallel.c`.
4. Implement the byte-channel entry points:

   ```c
   uint8_t fn_channel_init(void);
   uint8_t fn_channel_ready(void);
   uint8_t fn_channel_write_byte(uint8_t value, uint16_t timeout_ms);
   uint8_t fn_channel_read_byte(uint8_t *value, uint16_t timeout_ms);
   void fn_channel_drain_rx(void);
   void fn_channel_close(void);
   ```

5. Implement `fn_platform_name()` for the target.
6. Keep packet construction, checksums, SLIP framing, response parsing, sessions,
   clock, and network API behavior in `src/common/`.
7. Keep any parallel-port handshaking, timing, IRQ, status-register, or BIOS
   fallback logic inside `src/platform/msdos_parallel/`.

If the parallel channel can provide ordered byte read/write operations, it can
reuse `fn_transport_stream.c` unchanged. If the channel is packet-oriented and
does not need SLIP on the host side, add a different transport family rather than
putting packet protocol knowledge into the channel implementation.

## POSIX Channel Examples

USB serial on a Raspberry Pi is not a different channel in this model. It is the
existing POSIX serial/PTY channel with a different device path, such as
`/dev/ttyUSB0` or `/dev/ttyACM0`.

Useful POSIX channel variants would be:

- `linux-tcp`: a TCP socket byte channel for emulator, CI, or a local
  `fujinet-nio` process exposing a byte-stream endpoint.
- `linux-gpio` or `linux-spi`: a Raspberry Pi channel for an adapter connected
  through GPIO or SPI rather than USB serial.
- `linux-libusb`: a userspace USB endpoint channel if a future adapter exposes a
  vendor-specific USB protocol instead of USB CDC serial.

All of these can reuse `fn_transport_stream.c` if they provide ordered byte
read/write semantics. Only the channel implementation should change.

## Adding BBC User-Bus Or 1 MHz Bus Backends

BBC is different from MS-DOS because the current library target intentionally
delegates transport and channel handling to `fn-rom`. A direct user-bus or 1 MHz
bus backend should therefore be a separate target rather than a hidden mode
inside the existing `bbc` target.

The recommended approach is:

1. Keep `bbc` and `bbc-clib` as ROM-backed targets.
2. Add explicit targets such as `bbc-userbus` or `bbc-1mhzbus`.
3. Put direct bus integration under a separate platform directory such as
   `src/platform/bbc_userbus/`.
4. Decide whether the new BBC target can afford the common protocol and stream
   transport stack. If it cannot, mirror the existing BBC approach and provide
   small platform-specific implementations of the public APIs that call a
   resident ROM or small bus shim.
5. Use map analysis before accepting the backend as practical for real BBC
   applications.

This keeps the existing BBC target small and predictable while leaving room for
NIO firmware channels beyond the current ROM/MOS integration.

## Design Rule

`fujinet-nio` decides which transports and channels exist. `fujinet-nio-lib`
decides which one channel a particular application binary is built to speak.

When a platform needs more than one physical channel, prefer another target and
another channel directory over runtime selection. Reuse the common transport
family where the channel semantics match; write only the channel layer for new
byte-stream channels.
