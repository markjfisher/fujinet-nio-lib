from __future__ import annotations

import os
import select
import struct
import threading
import time
import tty
from typing import Callable, Optional

from fujinet_tools import fujibus as fb
from fujinet_tools import diskproto as dp
from fujinet_tools import fileproto as fp
from fujinet_tools import fujiproto as fuji
from fujinet_tools import netproto as netp

FujiPacket = fb.FujiPacket
Responder = Callable[[FujiPacket], Optional[bytes]]


def default_success_responder(pkt: FujiPacket) -> bytes:
    return fb.build_fuji_response_wire(pkt.device, pkt.command, 0, b"")


class FujiDevice:
    def __init__(self, pty_path: str, *, responder: Optional[Responder] = None):
        self.pty_path = pty_path
        self._fd = -1
        self._rx = bytearray()
        self._requests: list[FujiPacket] = []
        self._lock = threading.Lock()
        self._stop = False
        self._thread: Optional[threading.Thread] = None
        self._responder: Responder = responder or default_success_responder

    def start(self, open_timeout: float = 5.0) -> "FujiDevice":
        deadline = time.monotonic() + open_timeout
        last_err: Optional[OSError] = None
        while time.monotonic() < deadline:
            try:
                self._fd = os.open(self.pty_path, os.O_RDWR | os.O_NOCTTY)
                break
            except OSError as exc:
                last_err = exc
                time.sleep(0.1)
        if self._fd < 0:
            raise RuntimeError(f"could not open pty {self.pty_path!r}: {last_err}")
        tty.setraw(self._fd)
        self._stop = False
        self._thread = threading.Thread(target=self._loop, daemon=True)
        self._thread.start()
        return self

    def close(self) -> None:
        self._stop = True
        if self._thread is not None:
            self._thread.join(timeout=1.0)
            self._thread = None
        if self._fd >= 0:
            try:
                os.close(self._fd)
            except OSError:
                pass
            self._fd = -1

    def _loop(self) -> None:
        while not self._stop:
            readable, _, _ = select.select([self._fd], [], [], 0.1)
            if not readable:
                continue
            try:
                chunk = os.read(self._fd, 512)
            except OSError:
                break
            if not chunk:
                continue
            self._rx.extend(chunk)
            while True:
                frame = fb._extract_frame_from_rx(self._rx)
                if frame is None:
                    break
                pkt = fb.parse_fuji_packet(fb.slip_decode(frame))
                if pkt is None:
                    continue
                with self._lock:
                    self._requests.append(pkt)
                try:
                    reply = self._responder(pkt)
                except Exception:
                    reply = None
                if reply:
                    try:
                        os.write(self._fd, reply)
                    except OSError:
                        pass

    def set_responder(self, fn: Responder) -> None:
        self._responder = fn

    @property
    def requests(self) -> list[FujiPacket]:
        with self._lock:
            return list(self._requests)

    def clear(self) -> None:
        with self._lock:
            self._requests.clear()

    def wait_for(self, predicate: Callable[[FujiPacket], bool], timeout: float = 5.0):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            for pkt in self.requests:
                if predicate(pkt):
                    return pkt
            time.sleep(0.02)
        return None

    def wait_for_command(self, device: int, command: int, timeout: float = 5.0):
        return self.wait_for(lambda p: p.device == device and p.command == command, timeout)


def build_resolve_path_response(resolved_uri: str, display_path: str, status: int = 0) -> bytes:
    rb = resolved_uri.encode("utf-8")
    db = display_path.encode("utf-8")
    body = (
        bytes([fp.FILEPROTO_VERSION, 0])
        + struct.pack("<H", 0)
        + struct.pack("<H", len(rb)) + rb
        + struct.pack("<H", len(db)) + db
    )
    return fb.build_fuji_response_wire(fp.FILE_DEVICE_ID, fp.CMD_RESOLVE_PATH, status, body)


def build_list_response(formatted_text: str = "FILE\n", status: int = 0) -> bytes:
    flags = fp.LIST_RESP_FLAG_FORMATTED
    text_bytes = formatted_text.encode("utf-8")
    entry_count = formatted_text.count("\n") + (1 if formatted_text and not formatted_text.endswith("\n") else 0)
    body = (
        bytes([fp.FILEPROTO_VERSION, flags])
        + struct.pack("<H", 0)
        + struct.pack("<H", 0)
        + struct.pack("<H", entry_count)
        + struct.pack("<H", len(text_bytes))
        + text_bytes
    )
    return fb.build_fuji_response_wire(fp.FILE_DEVICE_ID, fp.CMD_LIST, status, body)


def build_get_mounts_response(text: str, status: int = 0) -> bytes:
    text_b = text.encode("utf-8")
    entry_count = text.count("\n") + (1 if text and not text.endswith("\n") else 0)
    body = (
        bytes([fuji.GET_MOUNTS_VERSION, fuji.GET_MOUNTS_RESP_FLAG_FORMATTED])
        + struct.pack("<H", 0)
        + struct.pack("<H", 0)
        + struct.pack("<H", entry_count)
        + struct.pack("<H", len(text_b))
        + text_b
    )
    return fb.build_fuji_response_wire(fuji.FUJI_DEVICE_ID, fuji.CMD_GET_MOUNTS, status, body)


def build_get_mount_response(*, slot: int, enabled: bool, uri: str, mode: str = "auto", status: int = 0) -> bytes:
    uri_b = uri.encode("utf-8")
    mode_b = mode.encode("utf-8")
    body = bytes([
        slot & 0xFF,
        0x01 if enabled else 0x00,
        len(uri_b) & 0xFF,
    ]) + uri_b + bytes([len(mode_b) & 0xFF]) + mode_b
    return fb.build_fuji_response_wire(fuji.FUJI_DEVICE_ID, fuji.CMD_GET_MOUNT, status, body)


def build_set_mount_response(status: int = 0) -> bytes:
    return fb.build_fuji_response_wire(fuji.FUJI_DEVICE_ID, fuji.CMD_SET_MOUNT, status, b"")


def build_disk_mount_response(*, slot: int, sector_count: int, status: int = 0) -> bytes:
    body = (
        bytes([dp.DISKPROTO_VERSION, 0x01])
        + struct.pack("<H", 0)
        + bytes([slot & 0xFF, 2])
        + struct.pack("<H", 256)
        + struct.pack("<I", sector_count)
    )
    return fb.build_fuji_response_wire(dp.DISK_DEVICE_ID, dp.CMD_MOUNT, status, body)


def build_disk_info_response(*, slot: int, sector_count: int, status: int = 0) -> bytes:
    body = (
        bytes([dp.DISKPROTO_VERSION, 0x01])
        + struct.pack("<H", 0)
        + bytes([slot & 0xFF, 2])
        + struct.pack("<H", 256)
        + struct.pack("<I", sector_count)
        + bytes([0])
    )
    return fb.build_fuji_response_wire(dp.DISK_DEVICE_ID, dp.CMD_INFO, status, body)


def build_disk_read_sector_response(*, slot: int, lba: int, data: bytes, status: int = 0) -> bytes:
    body = (
        bytes([dp.DISKPROTO_VERSION, 0])
        + struct.pack("<H", 0)
        + bytes([slot & 0xFF])
        + struct.pack("<I", lba)
        + struct.pack("<H", len(data))
        + data
    )
    return fb.build_fuji_response_wire(dp.DISK_DEVICE_ID, dp.CMD_READ_SECTOR, status, body)


def build_network_open_response(*, handle: int, proto_flags: int = 0x00, status: int = 0) -> bytes:
    body = (
        bytes([netp.NETPROTO_VERSION, 0x01])
        + struct.pack("<H", 0)
        + struct.pack("<H", handle)
        + bytes([proto_flags & 0xFF])
    )
    return fb.build_fuji_response_wire(netp.NETWORK_DEVICE_ID, netp.CMD_OPEN, status, body)


def build_network_read_response(
    *,
    handle: int,
    offset: int,
    data: bytes,
    eof: bool = False,
    more_available: bool = False,
    status: int = 0,
) -> bytes:
    flags = 0x01 if eof else 0x00
    if more_available:
        flags |= 0x04
    body = (
        bytes([netp.NETPROTO_VERSION, flags])
        + struct.pack("<H", 0)
        + struct.pack("<H", handle)
        + struct.pack("<I", offset)
        + struct.pack("<H", len(data))
        + data
    )
    return fb.build_fuji_response_wire(netp.NETWORK_DEVICE_ID, netp.CMD_READ, status, body)


def build_network_close_response(status: int = 0) -> bytes:
    return fb.build_fuji_response_wire(netp.NETWORK_DEVICE_ID, netp.CMD_CLOSE, status, b"")


def disk_image_responder(*, image_path, fuji_slot: int, drive_slot: int, uri: str, formatted_mounts: str = "0: AUTO\n", inner: Responder | None = None):
    with open(image_path, "rb") as fh:
        image = fh.read()
    nsec = max(1, len(image) // 256)

    def _resp(pkt: FujiPacket):
        if inner is not None:
            r = inner(pkt)
            if r is not None:
                return r
        if pkt.device == fp.FILE_DEVICE_ID:
            if pkt.command == fp.CMD_RESOLVE_PATH:
                return build_resolve_path_response(uri, uri)
            if pkt.command == fp.CMD_LIST:
                return build_list_response("HTGET\n")
        if pkt.device == fuji.FUJI_DEVICE_ID:
            if pkt.command == fuji.CMD_GET_MOUNT:
                return build_get_mount_response(slot=fuji_slot, enabled=True, uri=uri)
            if pkt.command == fuji.CMD_SET_MOUNT:
                return build_set_mount_response()
            if pkt.command == fuji.CMD_GET_MOUNTS:
                return build_get_mounts_response(formatted_mounts)
        if pkt.device == dp.DISK_DEVICE_ID:
            if pkt.command == dp.CMD_MOUNT:
                return build_disk_mount_response(slot=drive_slot, sector_count=nsec)
            if pkt.command == dp.CMD_INFO:
                return build_disk_info_response(slot=drive_slot, sector_count=nsec)
            if pkt.command == dp.CMD_READ_SECTOR:
                lba = int.from_bytes(pkt.payload[2:6], "little")
                maxb = int.from_bytes(pkt.payload[6:8], "little") or 256
                start = lba * 256
                data = image[start:start + min(maxb, 256)]
                if len(data) < 256:
                    data = data + bytes(256 - len(data))
                return build_disk_read_sector_response(slot=drive_slot, lba=lba, data=data)
        return default_success_responder(pkt)

    return _resp
