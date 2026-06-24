"""pytest configuration for fujinet-nio-lib Beebium integration tests."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import pytest

from beebium_test_env import add_fujinet_tools_to_path, ensure_environment

_HERE = Path(__file__).resolve()
_LIB_ROOT = _HERE.parents[2]

ensure_environment()
add_fujinet_tools_to_path()
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))


def pytest_addoption(parser):
    group = parser.getgroup("fujinet-nio-lib-beebium", "BBC integration scaffold for fujinet-nio-lib")
    group.addoption(
        "--fujinet-bin",
        action="store",
        default=os.environ.get("FUJINET_BIN", ""),
        help="path to the real posix fujinet-nio binary for future real tests",
    )


def pytest_configure(config):
    ensure_environment()


@pytest.fixture(scope="session")
def beebium_paths():
    return {
        "server": Path(os.environ["BEEBIUM_SERVER"]),
        "mos": Path(os.environ["BEEBIUM_MOS"]),
        "basic": Path(os.environ["BEEBIUM_BASIC"]),
        "fn_rom": Path(os.environ["FN_ROM"]),
        "slot": 12,
        "pty": os.environ.get("FN_PTY", "/tmp/fujinet-pty-e2e"),
    }


@pytest.fixture(scope="session")
def http_get_smoke_ssd(scaffold_info):
    cc65 = shutil.which("cl65")
    if not cc65:
        pytest.skip("cl65 not available")

    create_ssd = _LIB_ROOT / "scripts" / "create_ssd.py"
    if not create_ssd.is_file():
        pytest.skip(f"create_ssd.py not found at {create_ssd}")

    dfstool = shutil.which("dfstool")
    if not dfstool:
        pytest.skip("dfstool not available")

    app_src = _HERE.parent / "apps" / "http_get_smoke.c"
    lib_file = _LIB_ROOT / "build" / "fujinet-nio-bbc.lib"
    initenv_obj = _LIB_ROOT / "obj" / "bbc" / "platform" / "bbc" / "initenv.o"
    if not lib_file.is_file() or not initenv_obj.is_file():
        pytest.skip("BBC library artifacts not built; run make bbc first")

    tmp = Path(tempfile.mkdtemp(prefix="fnlib-bbc-smoke-"))
    stage = tmp / "stage"
    stage.mkdir(parents=True, exist_ok=True)
    binary = stage / "HTGET"
    ssd = tmp / "htget.ssd"

    subprocess.run(
        [
            cc65,
            "-t",
            "bbc",
            "--start-addr",
            "0x1900",
            "-I",
            str(_LIB_ROOT / "include"),
            "-o",
            str(binary),
            str(app_src),
            str(lib_file),
            str(initenv_obj),
        ],
        check=True,
        cwd=str(_LIB_ROOT),
    )

    (stage / "HTGET.inf").write_text("$.HTGET 001900 001900\n")

    subprocess.run(
        ["python3", str(create_ssd), "-i", str(stage), "-o", str(ssd), "-t", "HTGET"],
        check=True,
        cwd=str(_LIB_ROOT),
    )

    return ssd


@pytest.fixture(scope="session")
def tcp_stream_partial_ssd(scaffold_info):
    cc65 = shutil.which("cl65")
    if not cc65:
        pytest.skip("cl65 not available")

    create_ssd = _LIB_ROOT / "scripts" / "create_ssd.py"
    if not create_ssd.is_file():
        pytest.skip(f"create_ssd.py not found at {create_ssd}")

    dfstool = shutil.which("dfstool")
    if not dfstool:
        pytest.skip("dfstool not available")

    app_src = _HERE.parent / "apps" / "tcp_stream_partial.c"
    lib_file = _LIB_ROOT / "build" / "fujinet-nio-bbc.lib"
    initenv_obj = _LIB_ROOT / "obj" / "bbc" / "platform" / "bbc" / "initenv.o"
    if not lib_file.is_file() or not initenv_obj.is_file():
        pytest.skip("BBC library artifacts not built; run make bbc first")

    tmp = Path(tempfile.mkdtemp(prefix="fnlib-bbc-tcp-stream-"))
    stage = tmp / "stage"
    stage.mkdir(parents=True, exist_ok=True)
    binary = stage / "TPSTRM"
    ssd = tmp / "tpstrm.ssd"

    subprocess.run(
        [
            cc65,
            "-t",
            "bbc",
            "--start-addr",
            "0x1900",
            "-I",
            str(_LIB_ROOT / "include"),
            "-o",
            str(binary),
            str(app_src),
            str(lib_file),
            str(initenv_obj),
        ],
        check=True,
        cwd=str(_LIB_ROOT),
    )

    (stage / "TPSTRM.inf").write_text("$.TPSTRM 001900 001900\n")

    subprocess.run(
        ["python3", str(create_ssd), "-i", str(stage), "-o", str(ssd), "-t", "TPSTRM"],
        check=True,
        cwd=str(_LIB_ROOT),
    )

    return ssd


@pytest.fixture(scope="session")
def tcp_stream_no_probe_ssd(scaffold_info):
    cc65 = shutil.which("cl65")
    if not cc65:
        pytest.skip("cl65 not available")

    create_ssd = _LIB_ROOT / "scripts" / "create_ssd.py"
    if not create_ssd.is_file():
        pytest.skip(f"create_ssd.py not found at {create_ssd}")

    dfstool = shutil.which("dfstool")
    if not dfstool:
        pytest.skip("dfstool not available")

    app_src = _HERE.parent / "apps" / "tcp_stream_no_probe.c"
    lib_file = _LIB_ROOT / "build" / "fujinet-nio-bbc.lib"
    initenv_obj = _LIB_ROOT / "obj" / "bbc" / "platform" / "bbc" / "initenv.o"
    if not lib_file.is_file() or not initenv_obj.is_file():
        pytest.skip("BBC library artifacts not built; run make bbc first")

    tmp = Path(tempfile.mkdtemp(prefix="fnlib-bbc-tcp-stream-np-"))
    stage = tmp / "stage"
    stage.mkdir(parents=True, exist_ok=True)
    binary = stage / "TPSTRN"
    ssd = tmp / "tpstrn.ssd"

    subprocess.run(
        [
            cc65,
            "-t",
            "bbc",
            "--start-addr",
            "0x1900",
            "-I",
            str(_LIB_ROOT / "include"),
            "-o",
            str(binary),
            str(app_src),
            str(lib_file),
            str(initenv_obj),
        ],
        check=True,
        cwd=str(_LIB_ROOT),
    )

    (stage / "TPSTRN.inf").write_text("$.TPSTRN 001900 001900\n")

    subprocess.run(
        ["python3", str(create_ssd), "-i", str(stage), "-o", str(ssd), "-t", "TPSTRN"],
        check=True,
        cwd=str(_LIB_ROOT),
    )

    return ssd


@pytest.fixture()
def beebium(beebium_paths):
    from beebium.client import Beebium

    extra_args = [
        "--sideways", f"{beebium_paths['slot']}:rom:{beebium_paths['fn_rom']}",
        "--host-serial", f"mode=pty:path={beebium_paths['pty']}",
    ]
    with Beebium.launch(
        mos_filepath=str(beebium_paths["mos"]),
        basic_filepath=str(beebium_paths["basic"]),
        server_filepath=str(beebium_paths["server"]),
        extra_args=extra_args,
    ) as bbc:
        if not bbc.system.wait_for_ready(timeout=5.0):
            raise RuntimeError("Beebium did not report READY within 5 seconds")
        bbc.system.set_speed_multiplier(0.0)
        yield bbc


@pytest.fixture()
def fuji_device(beebium_paths):
    from fuji_device import FujiDevice

    dev = FujiDevice(beebium_paths["pty"])
    dev.start()
    try:
        yield dev
    finally:
        dev.close()


@pytest.fixture(scope="session")
def scaffold_info(pytestconfig):
    return {
        "lib_root": _LIB_ROOT,
        "fn_rom": Path(os.environ["FN_ROM"]),
        "fujinet_bin": Path(pytestconfig.getoption("--fujinet-bin")).expanduser()
        if pytestconfig.getoption("--fujinet-bin")
        else None,
        "fujinet_tools": Path(os.environ["FUJINET_TOOLS"]),
    }
