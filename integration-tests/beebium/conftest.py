from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

import pytest

_HERE = Path(__file__).resolve()
_LIB_ROOT = _HERE.parents[2]
_HOME = Path(os.path.expanduser("~"))


def _env_path(name: str, default: Path) -> Path:
    value = os.environ.get(name)
    return Path(value).expanduser() if value else default


BEEBIUM_HOME = _env_path("BEEBIUM_HOME", _HOME / "dev" / "bbc" / "beebium")
BEEBIUM_CLIENT_SRC = _env_path(
    "BEEBIUM_CLIENT_SRC", BEEBIUM_HOME / "clients" / "python" / "src"
)
FUJINET_TOOLS = _env_path(
    "FUJINET_TOOLS", _LIB_ROOT.parent.parent / "nio" / "repos" / "fujinet-nio" / "py"
)
FN_ROM_ROOT = _env_path("FN_ROM_ROOT", _LIB_ROOT.parent.parent / "nio" / "repos" / "fn-rom")

for _src in (BEEBIUM_CLIENT_SRC, FUJINET_TOOLS, _HERE):
    if _src.is_dir() and str(_src) not in sys.path:
        sys.path.insert(0, str(_src))


def pytest_addoption(parser):
    group = parser.getgroup("fujinet-nio-lib-beebium", "BBC integration scaffold for fujinet-nio-lib")
    group.addoption(
        "--fn-rom",
        action="store",
        default=os.environ.get("FN_ROM", str(FN_ROM_ROOT / "build" / "fujinet.rom")),
        help="fn-rom sideways ROM image to load",
    )
    group.addoption(
        "--fujinet-bin",
        action="store",
        default=os.environ.get(
            "FUJINET_BIN",
            str(FUJINET_TOOLS.parent / "build" / "fujibus-pty-debug" / "fujinet-nio"),
        ),
        help="path to the real posix fujinet-nio binary for future real tests",
    )


@pytest.fixture(scope="session")
def beebium_paths(pytestconfig):
    server = _env_path("BEEBIUM_SERVER", BEEBIUM_HOME / "build-release" / "src" / "server" / "beebium-model-b")
    mos = _env_path("BEEBIUM_MOS", BEEBIUM_HOME / "roms" / "acorn-mos_1_20.rom")
    basic = _env_path("BEEBIUM_BASIC", BEEBIUM_HOME / "roms" / "bbc-basic_2.rom")
    fn_rom = Path(pytestconfig.getoption("--fn-rom")).expanduser()

    if not BEEBIUM_CLIENT_SRC.is_dir():
        pytest.skip(f"Beebium python client not found at {BEEBIUM_CLIENT_SRC}")
    if not FUJINET_TOOLS.is_dir():
        pytest.skip(f"fujinet_tools not found at {FUJINET_TOOLS}")
    if not server.is_file() or not os.access(server, os.X_OK):
        pytest.skip(f"beebium-server not found/executable at {server}")
    if not mos.is_file():
        pytest.skip(f"MOS ROM not found at {mos}")
    if not basic.is_file():
        pytest.skip(f"BASIC ROM not found at {basic}")
    if not fn_rom.is_file():
        pytest.skip(f"fn-rom image not found at {fn_rom}")

    return {
        "server": server,
        "mos": mos,
        "basic": basic,
        "fn_rom": fn_rom,
        "slot": 12,
        "pty": os.environ.get("FN_PTY", "/tmp/fujinet-pty-e2e"),
    }


@pytest.fixture(scope="session")
def http_get_smoke_ssd(scaffold_info):
    cc65 = shutil.which("cl65")
    if not cc65:
        pytest.skip("cl65 not available")

    create_ssd = FN_ROM_ROOT / "scripts" / "create_ssd.py"
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
        cwd=str(FN_ROM_ROOT),
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
        "fn_rom": Path(pytestconfig.getoption("--fn-rom")).expanduser(),
        "fujinet_bin": Path(pytestconfig.getoption("--fujinet-bin")).expanduser(),
        "beebium_client_src": BEEBIUM_CLIENT_SRC,
        "fujinet_tools": FUJINET_TOOLS,
    }
