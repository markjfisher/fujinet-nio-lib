from __future__ import annotations

import pytest


def test_real_scaffold_paths(scaffold_info):
    if not scaffold_info["fn_rom"].is_file():
        pytest.skip("fn-rom image not built yet")


@pytest.mark.skip(reason="real BBC beebium + fujinet-nio smoke tests not ported yet")
def test_real_http_get_placeholder():
    pass
