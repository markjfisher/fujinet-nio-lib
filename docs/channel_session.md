# FujiBus channel/session contract

`fn_stream_session_t` is the shared session boundary for legacy byte-stream
channels. It is used by the current MS-DOS RS-232, POSIX serial/PTY, Amiga
serial, and TCP-style stream paths through their byte-channel adapters.

The channel implementation supplies only:

- `open` and `close`;
- one-byte write with a timeout;
- one-byte read with a timeout;
- input flushing.

The session owns the protocol behavior:

- SLIP framing and escaping;
- complete FujiBus packet writes and reads;
- response-buffer bounds;
- total request timeout;
- stale-input flushing before a request;
- one outstanding request at a time.

The public operation is:

```text
request(fujibus_packet, timeout) -> fujibus_packet
```

It never retries implicitly. The caller owns retry decisions and may retry
only operations whose semantics make that safe. A missing response is
`FN_ERR_TIMEOUT`; a failed byte write is returned by the channel; an invalid
or oversized decoded frame is `FN_ERR_IO`; a second simultaneous request is
`FN_ERR_BUSY`.

The session reports `packet_native=0` and `max_outstanding=1` for this SLIP
stream implementation. Packet-native channels will implement the same
packet-level contract with their own framing and can report different
capabilities without changing DiskDevice codecs or OS-driver logic.

RS-232 is the first concrete use because it exercises the complete persistent
session path with an existing byte-channel implementation while remaining a
correctness/smoke-test transport rather than the target full-disk performance
path.
