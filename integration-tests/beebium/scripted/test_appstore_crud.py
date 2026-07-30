from __future__ import annotations

import time
from collections import OrderedDict
from fuji_device import (
    build_appstore_delete_response,
    build_appstore_list_response,
    build_appstore_read_response,
    build_appstore_stat_response,
    build_appstore_write_response,
    disk_image_responder,
)
from helpers import command, dump_screen_text, wait_for_screen_text
from fujinet_tools import fileproto as fp


def _u16(payload: bytes, off: int) -> tuple[int, int]:
    return payload[off] | (payload[off + 1] << 8), off + 2


def _u32(payload: bytes, off: int) -> tuple[int, int]:
    return int.from_bytes(payload[off:off + 4], "little"), off + 4


def _lp_string(payload: bytes, off: int) -> tuple[str, int]:
    n, off = _u16(payload, off)
    return payload[off:off + n].decode("utf-8"), off + n


def _decode_prefix(payload: bytes) -> tuple[str, str, int]:
    assert payload[0] == fp.FILEPROTO_VERSION
    off = 1
    namespace, off = _lp_string(payload, off)
    key, off = _lp_string(payload, off)
    return namespace, key, off


def _decode_read_req(payload: bytes) -> tuple[str, str, int, int]:
    namespace, key, off = _decode_prefix(payload)
    offset, off = _u32(payload, off)
    max_len, off = _u16(payload, off)
    assert off == len(payload)
    return namespace, key, offset, max_len


def _decode_write_req(payload: bytes) -> tuple[str, str, int, bytes]:
    namespace, key, off = _decode_prefix(payload)
    offset, off = _u32(payload, off)
    data_len, off = _u16(payload, off)
    data = payload[off:off + data_len]
    assert off + data_len == len(payload)
    return namespace, key, offset, data


def _decode_list_req(payload: bytes) -> tuple[str, str, int, int]:
    namespace, key, off = _decode_prefix(payload)
    start, off = _u16(payload, off)
    max_payload, off = _u16(payload, off)
    assert off == len(payload)
    return namespace, key, start, max_payload


def test_appstore_crud_app_validates_wire_and_screen_output(beebium, fuji_device, appstore_crud_ssd):
    seen: list[tuple[int, bytes]] = []
    store: dict[str, OrderedDict[str, bytes]] = {
        "test.app": OrderedDict(),
        "empty.app": OrderedDict(),
    }
    expected_writes = [
        ("test.app", "alpha", b"one"),
        ("test.app", "beta", b"two"),
        ("test.app", "gamma", b"three"),
        ("test.app", "beta", b"two-up"),
    ]
    writes_seen: list[tuple[str, str, bytes]] = []

    def keys_for(namespace: str) -> list[str]:
        return list(store.setdefault(namespace, OrderedDict()).keys())

    def inner(pkt):
        if pkt.device != fp.FILE_DEVICE_ID:
            return None

        if pkt.command == fp.CMD_APPSTORE_DELETE:
            seen.append((pkt.command, pkt.payload))
            namespace, key, off = _decode_prefix(pkt.payload)
            assert off == len(pkt.payload)
            assert namespace == "test.app"
            assert key in ("gamma", "missing")
            deleted = key in store[namespace]
            store[namespace].pop(key, None)
            return build_appstore_delete_response(deleted=deleted)

        if pkt.command == fp.CMD_APPSTORE_STAT:
            seen.append((pkt.command, pkt.payload))
            namespace, key, off = _decode_prefix(pkt.payload)
            assert off == len(pkt.payload)
            assert namespace == "test.app"
            value = store[namespace].get(key)
            return build_appstore_stat_response(
                exists=value is not None,
                size=0 if value is None else len(value),
                mtime=123456 if value is not None else 0,
            )

        if pkt.command == fp.CMD_APPSTORE_WRITE:
            seen.append((pkt.command, pkt.payload))
            namespace, key, offset, data = _decode_write_req(pkt.payload)
            assert namespace == "test.app"
            assert offset == 0
            writes_seen.append((namespace, key, data))
            if key not in store[namespace]:
                store[namespace][key] = data
            else:
                store[namespace][key] = data
            return build_appstore_write_response(offset=0, written=len(data))

        if pkt.command == fp.CMD_APPSTORE_READ:
            seen.append((pkt.command, pkt.payload))
            namespace, key, offset, max_len = _decode_read_req(pkt.payload)
            assert namespace == "test.app"
            assert offset == 0
            value = store[namespace].get(key)
            if value is None:
                return build_appstore_read_response(offset=0, data=b"", exists=False)
            assert max_len >= len(value)
            return build_appstore_read_response(offset=0, data=value)

        if pkt.command == fp.CMD_APPSTORE_LIST:
            seen.append((pkt.command, pkt.payload))
            namespace, key, start, max_payload = _decode_list_req(pkt.payload)
            assert key == ""
            assert start == 0
            keys = keys_for(namespace)
            assert max_payload >= sum(2 + len(k.encode("utf-8")) for k in keys)
            return build_appstore_list_response(keys=keys, start_index=0)

        return None

    fuji_device.set_responder(
        disk_image_responder(
            image_path=str(appstore_crud_ssd),
            catalog_slot=7,
            drive_slot=4,
            uri="sd0:/astore.ssd",
            inner=inner,
        )
    )

    command(beebium, "*FHOST sd0:/")
    time.sleep(0.2)
    command(beebium, "*FIN 7 astore.ssd")
    time.sleep(0.2)
    command(beebium, "*FMOUNT 7 0")
    time.sleep(0.2)
    fuji_device.clear()
    command(beebium, "*RUN ASTORE")

    wait_for_screen_text(beebium, "ASTORE OK", timeout=10.0)
    screen = dump_screen_text(beebium)
    for expected in (
        "ASTORE START",
        "EMPTY OK",
        "PUT3 OK",
        "LIST3 OK",
        "GET3 OK",
        "UPD beta two-up",
        "POSTUP OK",
        "DEL gamma OK",
        "LIST2 OK",
        "LEFT alpha one",
        "LEFT beta two-up",
        "DELMISS OK",
        "ASTORE OK",
    ):
        assert expected in screen

    assert writes_seen == expected_writes
    assert store == {
        "test.app": OrderedDict([("alpha", b"one"), ("beta", b"two-up")]),
        "empty.app": OrderedDict(),
    }
    assert [cmd for cmd, _payload in seen] == [
        fp.CMD_APPSTORE_LIST,
        fp.CMD_APPSTORE_WRITE,
        fp.CMD_APPSTORE_WRITE,
        fp.CMD_APPSTORE_WRITE,
        fp.CMD_APPSTORE_LIST,
        fp.CMD_APPSTORE_READ,
        fp.CMD_APPSTORE_STAT,
        fp.CMD_APPSTORE_READ,
        fp.CMD_APPSTORE_STAT,
        fp.CMD_APPSTORE_READ,
        fp.CMD_APPSTORE_STAT,
        fp.CMD_APPSTORE_WRITE,
        fp.CMD_APPSTORE_READ,
        fp.CMD_APPSTORE_STAT,
        fp.CMD_APPSTORE_READ,
        fp.CMD_APPSTORE_STAT,
        fp.CMD_APPSTORE_READ,
        fp.CMD_APPSTORE_STAT,
        fp.CMD_APPSTORE_DELETE,
        fp.CMD_APPSTORE_READ,
        fp.CMD_APPSTORE_STAT,
        fp.CMD_APPSTORE_LIST,
        fp.CMD_APPSTORE_READ,
        fp.CMD_APPSTORE_STAT,
        fp.CMD_APPSTORE_READ,
        fp.CMD_APPSTORE_STAT,
        fp.CMD_APPSTORE_DELETE,
    ]
