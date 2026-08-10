# DiskDevice client codec contract

`fn_disk_*` in `fujinet-nio-lib` is the typed client codec for the generic
FujiNet-NIO DiskDevice. It does not implement an operating-system disk driver,
filesystem logic, caching, or channel/session policy.

The codec sends FujiBus requests to wire device `0xFC` using DiskDevice v1.
All slot numbers are one-based. Multi-byte integers are little-endian. Device
status values are mapped to the library's `FN_*` result codes; malformed
successful responses return `FN_ERR_IO`.

`Info` deliberately accepts two response sizes. The original DiskDevice
response contains the fixed fields through `sectorCount` and is 12 bytes. A
newer response may append the optional `lastError` byte and is 13 bytes. This
preserves interoperability with existing BBC/MS-DOS clients and older NIO
peers while exposing `last_error=0` when the optional byte is absent.

| API | DiskDevice command | Contract |
|---|---|---|
| `fn_disk_mount` | `Mount (0x01)` | Mount a full URI with mode/type/hint and decode effective geometry. |
| `fn_disk_unmount` | `Unmount (0x02)` | Unmount one slot and validate the echoed slot. |
| `fn_disk_read_sector` | `ReadSector (0x03)` | Read one LBA into caller storage, honoring its capacity. |
| `fn_disk_write_sector` | `WriteSector (0x04)` | Send one complete sector; the server validates geometry. |
| `fn_disk_info` | `Info (0x05)` | Decode slot flags, geometry, and last error. |
| `fn_disk_clear_changed` | `ClearChanged (0x06)` | Clear one slot's changed flag. |

`fn_disk_read_sector` returns the actual response length because DiskDevice
supports variable-size sectors. Callers should obtain geometry with `Mount` or
`Info` before issuing reads and must provide storage large enough for the
requested transfer. `fn_disk_write_sector` does not assume a sector size;
callers must obtain geometry first and provide at least one complete sector.

The codec uses the library's existing synchronous `fn_raw_call` primitive. It
does not retry requests, maintain a session, or permit concurrent operations.
Those policies belong to the channel/session layer and the native driver.

The first Amiga driver uses `Mount`, `Info`, `ReadSector`, `Unmount`, and
`ClearChanged` in read-only mode. Write support is available in the codec but
is deferred until the driver defines cache, flush, and failure semantics.
