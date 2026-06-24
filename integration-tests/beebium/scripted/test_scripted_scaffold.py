from __future__ import annotations

import pytest


def test_scripted_scaffold_paths(scaffold_info):
    assert scaffold_info["lib_root"].name == "fujinet-nio-lib"
    assert scaffold_info["fujinet_tools"].is_dir()


@pytest.mark.skip(reason="scripted BBC packet tests not ported yet")
def test_http_get_scripted_placeholder():
    pass
