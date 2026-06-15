from __future__ import annotations

import os
import sys
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
def scaffold_info(pytestconfig):
    return {
        "lib_root": _LIB_ROOT,
        "fn_rom": Path(pytestconfig.getoption("--fn-rom")).expanduser(),
        "fujinet_bin": Path(pytestconfig.getoption("--fujinet-bin")).expanduser(),
        "beebium_client_src": BEEBIUM_CLIENT_SRC,
        "fujinet_tools": FUJINET_TOOLS,
    }
