from __future__ import annotations

import pytest


def test_scripted_scaffold_paths(scaffold_info):
    assert scaffold_info["lib_root"].name == "fujinet-nio-lib"
    if not scaffold_info["beebium_client_src"].is_dir():
        pytest.skip("beebium client source not present")
    if not scaffold_info["fujinet_tools"].is_dir():
        pytest.skip("fujinet_tools source not present")


@pytest.mark.skip(reason="scripted BBC packet tests not ported yet")
def test_http_get_scripted_placeholder():
    pass
