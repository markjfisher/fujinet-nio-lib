from __future__ import annotations

import time
from fujinet_tools import fujibus as fb
from fuji_device import (
    build_network_close_response,
    build_network_open_response,
    build_network_read_response,
    disk_image_responder,
)
from fujinet_tools import netproto as netp
from helpers import command, dump_screen_text, wait_for_screen_text


def test_tcp_stream_single_fn_read_returns_partial_data_without_waiting_for_eof(
    beebium, fuji_device, tcp_stream_partial_ssd
):
    handle = 0x1234
    read_requests = []

    def inner(pkt):
        if pkt.device != netp.NETWORK_DEVICE_ID:
            return None
        if pkt.command == netp.CMD_OPEN:
            return build_network_open_response(
                handle=handle,
                proto_flags=(
                    netp.PROTO_FLAG_SEQUENTIAL_READ
                    | netp.PROTO_FLAG_SEQUENTIAL_WRITE
                    | netp.PROTO_FLAG_STREAMING
                ),
            )
        if pkt.command == netp.CMD_READ:
            read_requests.append(pkt)
            req_offset = int.from_bytes(pkt.payload[3:7], "little")
            if req_offset == 0:
                return build_network_read_response(handle=handle, offset=0, data=b"OK", eof=False)
            return fb.build_fuji_response_wire(netp.NETWORK_DEVICE_ID, netp.CMD_READ, 4, b"")
        if pkt.command == netp.CMD_CLOSE:
            return build_network_close_response()
        return None

    fuji_device.set_responder(
        disk_image_responder(
            image_path=str(tcp_stream_partial_ssd),
            fuji_slot=7,
            drive_slot=4,
            uri="sd0:/tpstrm.ssd",
            inner=inner,
        )
    )

    command(beebium, "*FHOST sd0:/")
    time.sleep(0.2)
    command(beebium, "*FIN 7 tpstrm.ssd")
    time.sleep(0.2)
    command(beebium, "*FMOUNT 7 0")
    time.sleep(0.2)
    fuji_device.clear()
    command(beebium, "*RUN TPSTRM")

    try:
        wait_for_screen_text(beebium, "[OK 2 0]", timeout=8.0)
    except TimeoutError:
        print("SCREEN AFTER *RUN TPSTRN:\n" + dump_screen_text(beebium))
        print("SEEN REQUESTS:", [(pkt.device, pkt.command, pkt.payload.hex()) for pkt in fuji_device.requests])
        raise

    open_pkt = fuji_device.wait_for_command(netp.NETWORK_DEVICE_ID, netp.CMD_OPEN, timeout=8.0)
    assert open_pkt is not None and open_pkt.checksum_ok

    deadline = time.monotonic() + 8.0
    while time.monotonic() < deadline:
        close_pkt = fuji_device.wait_for_command(netp.NETWORK_DEVICE_ID, netp.CMD_CLOSE, timeout=0.1)
        if close_pkt is not None:
            break
    else:
        print("SCREEN AFTER *RUN TPSTRM:\n" + dump_screen_text(beebium))
        raise AssertionError("close packet not observed")

    assert len(read_requests) == 2
    assert int.from_bytes(read_requests[0].payload[3:7], "little") == 0
    assert int.from_bytes(read_requests[1].payload[3:7], "little") == 2


def test_tcp_stream_no_probe_avoids_followup_read_when_chunk_is_self_framed(
    beebium, fuji_device, tcp_stream_no_probe_ssd
):
    handle = 0x1234
    read_requests = []

    def inner(pkt):
        if pkt.device != netp.NETWORK_DEVICE_ID:
            return None
        if pkt.command == netp.CMD_OPEN:
            assert pkt.payload[2] & netp.FLAG_STREAM_NO_PROBE
            return build_network_open_response(
                handle=handle,
                proto_flags=(
                    netp.PROTO_FLAG_SEQUENTIAL_READ
                    | netp.PROTO_FLAG_SEQUENTIAL_WRITE
                    | netp.PROTO_FLAG_STREAMING
                ),
            )
        if pkt.command == netp.CMD_READ:
            read_requests.append(pkt)
            req_offset = int.from_bytes(pkt.payload[3:7], "little")
            if req_offset == 0:
                return build_network_read_response(
                    handle=handle,
                    offset=0,
                    data=b"OK",
                    eof=False,
                    more_available=False,
                )
            return fb.build_fuji_response_wire(netp.NETWORK_DEVICE_ID, netp.CMD_READ, 4, b"")
        if pkt.command == netp.CMD_CLOSE:
            return build_network_close_response()
        return None

    fuji_device.set_responder(
        disk_image_responder(
            image_path=str(tcp_stream_no_probe_ssd),
            fuji_slot=7,
            drive_slot=4,
            uri="sd0:/tpstrn.ssd",
            inner=inner,
        )
    )

    command(beebium, "*FHOST sd0:/")
    time.sleep(0.2)
    command(beebium, "*FIN 7 tpstrn.ssd")
    time.sleep(0.2)
    command(beebium, "*FMOUNT 7 0")
    time.sleep(0.2)
    fuji_device.clear()
    command(beebium, "*RUN TPSTRN")

    try:
        wait_for_screen_text(beebium, "[OK 2 0]", timeout=8.0)
    except TimeoutError:
        print("SCREEN AFTER *RUN TPSTRN:\n" + dump_screen_text(beebium))
        print("SEEN REQUESTS:", [(pkt.device, pkt.command, pkt.payload.hex()) for pkt in fuji_device.requests])
        raise

    open_pkt = fuji_device.wait_for_command(netp.NETWORK_DEVICE_ID, netp.CMD_OPEN, timeout=8.0)
    assert open_pkt is not None and open_pkt.checksum_ok

    close_pkt = fuji_device.wait_for_command(netp.NETWORK_DEVICE_ID, netp.CMD_CLOSE, timeout=8.0)
    assert close_pkt is not None and close_pkt.checksum_ok

    assert len(read_requests) == 1
    assert int.from_bytes(read_requests[0].payload[3:7], "little") == 0
