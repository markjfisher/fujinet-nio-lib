from __future__ import annotations

import time

from beebium.screen import dump_screen


def command(bbc, text: str) -> None:
    with bbc.keyboard.text_input():
        bbc.keyboard.type(text)
        bbc.keyboard.press_return()


def run_basic_lines(bbc, lines: list[str], *, wait: float = 0.05) -> None:
    command(bbc, "NEW")
    for line in lines:
        command(bbc, line)
        time.sleep(wait)


def run_basic_program(bbc, lines: list[str], *, wait: float = 0.05) -> None:
    run_basic_lines(bbc, lines, wait=wait)
    command(bbc, "RUN")


def wait_for_screen_text(bbc, text: str, *, timeout: float = 8.0, case_sensitive: bool = True) -> None:
    deadline = time.monotonic() + timeout
    wanted = text if case_sensitive else text.upper()
    while time.monotonic() < deadline:
        screen = dump_screen(bbc)
        haystack = screen if case_sensitive else screen.upper()
        if wanted in haystack:
            return
        time.sleep(0.02)
    raise TimeoutError(f"Text {text!r} not found on screen within {timeout} seconds")
