# BBC ROM-Backed Migration Plan

## Goal

Refactor the BBC target in `fujinet-nio-lib` so it stops acting as a raw RS423/SLIP/FujiBus client and instead becomes a thin cc65 library layer over the BBC-native `fn-rom` network ABI.

That means:

- using standard MOS channel opens and reads for BBC network sessions
- using `OSWORD &78` for long URLs, JSON translation, and body/profile configuration
- removing BBC dependence on the large common packet builder/parser code
- moving BBC-specific behavior into platform code with a minimal footprint

## Why This Change Is Needed

The old BBC implementation in `src/platform/bbc/fn_transport.s` duplicates logic now already resident in `fn-rom`:

- RS423 setup and stream switching
- SLIP framing
- FujiBus packet construction and checksum usage
- retry and receive loops

At the same time the current library still pulls in the generic network stack for BBC:

- `src/common/fn_open.c`
- `src/common/fn_rw.c`
- `src/common/fn_info_close.c`
- packet builders/parsers under `src/common/fn_packet_*`
- `src/common/fn_state.c`

That split is wrong for BBC because `fn-rom` already owns the BBC-facing network ABI.

## BBC Target Architecture

The BBC target should become a ROM-backed client layer with this structure:

- public API remains in `include/fujinet-nio.h`
- Linux and Atari continue using the direct transport/common packet path
- BBC overrides the public network entry points with target-specific implementations
- BBC public functions call:
  - cc65 BBC file/channel APIs for open/read/seek/close
  - `OSWORD &78` for long URL, JSON translation, request body writes, and content profile setup

The BBC target should no longer:

- build FujiBus packets locally
- compute FujiBus checksums locally
- perform SLIP encode/decode locally
- own RS423 setup in the library

## ROM Interfaces To Use

The relevant `fn-rom` interfaces are:

- `OPENIN`, `OPENOUT`, `OPENUP` via the BBC MOS file vectors
- `BGET#`, `EOF#`, `BPUT#`, `CLOSE#`, `OSARGS` as normal channel operations
- `OSWORD &78` reasons:
  - `&00` JSON query configuration on an open handle
  - `&01` set one-shot POST/PUT body length
  - `&02` write request body bytes from RAM
  - `&03` set one-shot content profile
  - `&04` arm long URL for the next sentinel open

These are documented in:

- `fn-rom/docs/fnnet-api.md`
- `fn-rom/src/net/fnnet.s`
- `fn-rom/src/kernel/vectors/findv_entry.s`

## Public API Mapping

### `fn_init`

- use a lightweight `OSWORD &78` probe that is harmless if the ROM is present
- cache initialization state locally

### `fn_is_ready`

- report readiness based on the same ROM probe
- do not unconditionally return ready

### `fn_open`

- map method to BBC open mode:
  - GET -> open for input
  - PUT -> open for output
  - POST -> open for update
  - raw TCP/TLS -> open for update
- reject methods the current ROM ABI does not expose cleanly through MOS open semantics
- for URLs that exceed the cc65 BBC `open()` filename path limit, use `OSWORD &78` reason `&04` plus the sentinel `"://"`

### `fn_tcp_open`

- build `tcp://host:port`
- call `fn_open`

### `fn_read`

- use BBC file descriptor reads via cc65
- use `lseek()` before reads when the caller requests a new offset
- set `FN_READ_EOF` when the ROM reports end of stream

### `fn_write`

- use `OSWORD &78` reason `&02` to send write data directly from caller RAM
- require sequential write offsets on BBC because the ROM write-ext path advances an internal write cursor

### `fn_info`

- provide a reduced BBC implementation for now
- report `FN_INFO_CONNECTED` for an open handle
- leave HTTP status and content length unavailable until a future ROM-visible info ABI exists

### `fn_close`

- close the underlying BBC file descriptor
- free the local session slot

## BBC-Specific Extensions To Expose

To match the `fn-rom` network feature set more closely, add thin library wrappers for:

- `fn_set_body_length()` -> `OSWORD &78` reason `&01`
- `fn_set_content_profile()` -> `OSWORD &78` reason `&03`
- `fn_json_query()` -> `OSWORD &78` reason `&00`

These APIs are meaningful on BBC immediately and can return `FN_ERR_UNSUPPORTED` on other platforms until they gain equivalents.

## Build-System Refactor

For target `bbc`:

- exclude the large common network packet/path modules from the link
- exclude the common clock implementation for now
- compile BBC platform overrides instead

The BBC build should keep only the common pieces still genuinely shared, such as:

- `fn_util.c`
- small compatibility/stub files that do not drag in the direct transport stack

## Source Changes By Phase

### Phase 1: Planning And Build Split

- add this migration plan under `docs/`
- update build logic so BBC no longer links the common network packet/parsing stack
- add BBC compile-time platform detection in headers

### Phase 2: BBC ROM-Backed Network Path

- replace `src/platform/bbc/fn_transport.s` raw serial code
- add a tiny BBC `OSWORD &78` helper in assembly
- implement BBC public network functions against cc65 file APIs plus `OSWORD &78`
- add BBC-local session state instead of the heavyweight common transport state

### Phase 3: BBC Feature Extensions

- add wrappers for JSON query, body length, and content profile
- document the BBC-specific behavior and current method limitations

### Phase 4: Verification

- build the BBC library with `make bbc`
- compile at least one BBC example application against the new library path
- where possible, mirror the `fn-rom` transport/integration style for future automated tests

## Testing Strategy

### Immediate validation

- successful `make bbc`
- successful `make linux` and `make atari` regression build checks
- compile a BBC example linked against the library

### Follow-up automated testing

The long-term BBC test structure should follow the style already used in `fn-rom` and `cc65-clib`:

- soft65c02 unit tests for pure wrapper behavior
- beebium integration tests with `fn-rom` loaded
- optional real `fujinet-nio` interop via the beebium + PTY test path

The best reusable references are:

- `fn-rom/integration-tests/beebium/scripted/test_network_device.py`
- `fn-rom/integration-tests/beebium/real/test_real_fujinet_e2e.py`
- `fn-rom/bas/lib/fnnet.bas`
- `fn-rom/bas/iss/iss.bas`

## Known BBC Limitations After This Refactor

These are acceptable first-cut limitations unless the ROM ABI is extended:

- `fn_info()` cannot yet expose HTTP status/content length on BBC
- `HEAD` and `DELETE` are not cleanly representable through the current MOS open mapping and should return unsupported
- zero-length `fn_write()` is not a true half-close signal on BBC

## Expected Result

After the migration:

- BBC code size is reduced substantially because the generic FujiBus packet machinery is no longer linked
- the BBC library behavior matches the real platform architecture
- `fujinet-nio-lib` and `fn-rom` become easier to evolve together
- BBC applications can use the library while relying on the installed `fn-rom` for the actual network device implementation
