from __future__ import annotations

import time

from fuji_device import (
    build_network_close_response,
    build_network_open_response,
    build_network_read_response,
    disk_image_responder,
)
from helpers import command, dump_screen_text, wait_for_screen_text
from fujinet_tools import netproto as netp


def _decode_open_payload(payload: bytes):
    off = 0
    version = payload[off]
    off += 1
    method = payload[off]
    off += 1
    flags = payload[off]
    off += 1
    url_len = payload[off] | (payload[off + 1] << 8)
    off += 2
    url = payload[off:off + url_len].decode("utf-8")
    return version, method, flags, url


def test_fn_open_http_get_emits_open_read_close(beebium, fuji_device, http_get_smoke_ssd):
    handle = 0x1234

    def inner(pkt):
        if pkt.device != netp.NETWORK_DEVICE_ID:
            return None
        if pkt.command == netp.CMD_OPEN:
            return build_network_open_response(handle=handle, proto_flags=0)
        if pkt.command == netp.CMD_READ:
            return build_network_read_response(handle=handle, offset=0, data=b"OK", eof=True)
        if pkt.command == netp.CMD_CLOSE:
            return build_network_close_response()
        return None

    fuji_device.set_responder(
        disk_image_responder(
            image_path=str(http_get_smoke_ssd),
            catalog_slot=7,
            drive_slot=4,
            uri="sd0:/htget.ssd",
            inner=inner,
        )
    )

    command(beebium, "*FHOST sd0:/")
    time.sleep(0.2)
    command(beebium, "*FIN 7 htget.ssd")
    time.sleep(0.2)
    command(beebium, "*FMOUNT 7 0")
    time.sleep(0.2)
    fuji_device.clear()
    command(beebium, "*RUN HTGET")

    open_pkt = fuji_device.wait_for_command(netp.NETWORK_DEVICE_ID, netp.CMD_OPEN, timeout=8.0)
    if open_pkt is None:
        print("SCREEN AFTER *RUN HTGET:\n" + dump_screen_text(beebium))
        print("SEEN REQUESTS:", [(pkt.device, pkt.command) for pkt in fuji_device.requests])
    assert open_pkt is not None and open_pkt.checksum_ok
    version, method, flags, url = _decode_open_payload(open_pkt.payload)
    assert version == netp.NETPROTO_VERSION
    assert method == 1
    assert flags == 0x08
    assert url == "http://example.com/data.txt"

    read_pkt = fuji_device.wait_for_command(netp.NETWORK_DEVICE_ID, netp.CMD_READ, timeout=8.0)
    if read_pkt is None:
        print("SCREEN AFTER OPEN:\n" + dump_screen_text(beebium))
        print("SEEN REQUESTS:", [(pkt.device, pkt.command) for pkt in fuji_device.requests])
    assert read_pkt is not None and read_pkt.checksum_ok
    assert int.from_bytes(read_pkt.payload[1:3], "little") == handle
    assert int.from_bytes(read_pkt.payload[3:7], "little") == 0

    close_pkt = fuji_device.wait_for_command(netp.NETWORK_DEVICE_ID, netp.CMD_CLOSE, timeout=8.0)
    assert close_pkt is not None and close_pkt.checksum_ok

    wait_for_screen_text(beebium, "[ OK", timeout=8.0)


def test_http_get_long_arms_full_url_and_emits_open_read_close(beebium, fuji_device, http_get_long_ssd):
    handle = 0x2345
    expected_short_url = "http://example.com/short.txt"
    expected_url = "http://example.com/" + ("0123456789abcdef" * 8)
    assert len(expected_url) > 127

    def inner(pkt):
        if pkt.device != netp.NETWORK_DEVICE_ID:
            return None
        if pkt.command == netp.CMD_OPEN:
            return build_network_open_response(handle=handle, proto_flags=0)
        if pkt.command == netp.CMD_READ:
            return build_network_read_response(handle=handle, offset=0, data=b"OK", eof=True)
        if pkt.command == netp.CMD_CLOSE:
            return build_network_close_response()
        return None

    fuji_device.set_responder(
        disk_image_responder(
            image_path=str(http_get_long_ssd),
            catalog_slot=7,
            drive_slot=4,
            uri="sd0:/htlong.ssd",
            inner=inner,
        )
    )

    command(beebium, "*FHOST sd0:/")
    time.sleep(0.2)
    command(beebium, "*FIN 7 htlong.ssd")
    time.sleep(0.2)
    command(beebium, "*FMOUNT 7 0")
    time.sleep(0.2)
    fuji_device.clear()
    command(beebium, "*RUN HTLONG")

    wait_for_screen_text(beebium, "LONG OK", timeout=8.0)

    packets = [pkt for pkt in fuji_device.requests if pkt.device == netp.NETWORK_DEVICE_ID]
    assert [pkt.command for pkt in packets] == [
        netp.CMD_OPEN, netp.CMD_READ, netp.CMD_CLOSE,
        netp.CMD_OPEN, netp.CMD_READ, netp.CMD_CLOSE,
    ]
    assert all(pkt.checksum_ok for pkt in packets)

    first_open = _decode_open_payload(packets[0].payload)
    second_open = _decode_open_payload(packets[3].payload)
    assert first_open == (netp.NETPROTO_VERSION, 1, 0x08, expected_short_url)
    assert second_open == (netp.NETPROTO_VERSION, 1, 0x08, expected_url)

    for read_pkt in (packets[1], packets[4]):
        assert int.from_bytes(read_pkt.payload[1:3], "little") == handle
        assert int.from_bytes(read_pkt.payload[3:7], "little") == 0
