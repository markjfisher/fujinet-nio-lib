#!/usr/bin/env python3
"""Analyze cl65/ld65 application map files with an application-first view.

This complements the ROM-focused scripts by highlighting:
- top module contributors
- app vs library vs runtime totals
- BSS/DATA reservations that are large for a small application
- differences between two builds, such as `bbc` vs `bbc-clib`
"""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass, field
from pathlib import Path

MODULE_HEADER_RE = re.compile(r"^(?P<name>[^\s:][^:]*)[:]\s*$")
SEGMENT_RE = re.compile(
    r"^\s+(?P<segment>\S+)\s+Offs=(?P<offset>[0-9A-F]{6})\s+"
    r"Size=(?P<size>[0-9A-F]{6})"
)

APP_SEGMENTS = {"CODE", "RODATA", "DATA", "BSS", "STARTUP", "ONCE", "ZEROPAGE"}


@dataclass
class ModuleSize:
    name: str
    segments: dict[str, int] = field(default_factory=dict)

    @property
    def total(self) -> int:
        return sum(self.segments.values())

    def seg(self, name: str) -> int:
        return self.segments.get(name, 0)


def parse_modules(map_path: Path) -> list[ModuleSize]:
    modules: list[ModuleSize] = []
    current: ModuleSize | None = None
    in_modules = False

    for raw_line in map_path.read_text().splitlines():
        line = raw_line.rstrip()
        if not in_modules:
            if line == "Modules list:":
                in_modules = True
            continue
        if line == "Segment list:":
            break

        match = MODULE_HEADER_RE.match(line)
        if match:
            current = ModuleSize(match.group("name"))
            modules.append(current)
            continue

        match = SEGMENT_RE.match(line)
        if match and current is not None:
            seg = match.group("segment")
            size = int(match.group("size"), 16)
            current.segments[seg] = current.segments.get(seg, 0) + size

    return [m for m in modules if m.total]


def classify(name: str, root_name: str) -> str:
    if name == root_name or name.endswith(f"/{root_name}"):
        return "app"
    if "fujinet-nio" in name or "(fn_" in name or "/fn_" in name:
        return "library"
    if "/cc65/lib/" in name or "bbc.lib(" in name or "bbc-clib.lib(" in name:
        return "runtime"
    return "other"


def summarize(modules: list[ModuleSize], root_name: str) -> dict[str, int]:
    totals = {"app": 0, "library": 0, "runtime": 0, "other": 0}
    for mod in modules:
        totals[classify(mod.name, root_name)] += mod.total
    return totals


def format_size(n: int) -> str:
    return f"{n:5d}  0x{n:04X}"


def print_report(map_path: Path) -> None:
    modules = parse_modules(map_path)
    root_name = map_path.stem + ".o"
    totals = summarize(modules, root_name)

    print(f"== {map_path.name} ==")
    print("totals:")
    for key in ("app", "library", "runtime", "other"):
        print(f"  {key:<8} {format_size(totals[key])}")

    print("top modules:")
    for mod in sorted(modules, key=lambda m: (-m.total, m.name))[:15]:
        print(f"  {format_size(mod.total)}  {mod.name}")

    print("largest BSS/DATA reservations:")
    ranked = sorted(
        [m for m in modules if m.seg("BSS") or m.seg("DATA")],
        key=lambda m: (-(m.seg("BSS") + m.seg("DATA")), m.name),
    )
    for mod in ranked[:12]:
        total = mod.seg("BSS") + mod.seg("DATA")
        pieces = []
        if mod.seg("BSS"):
            pieces.append(f"BSS={format_size(mod.seg('BSS')).strip()}")
        if mod.seg("DATA"):
            pieces.append(f"DATA={format_size(mod.seg('DATA')).strip()}")
        print(f"  {format_size(total)}  {mod.name}  [{' '.join(pieces)}]")
    print()


def compare(a_path: Path, b_path: Path) -> None:
    a_mods = {m.name: m for m in parse_modules(a_path)}
    b_mods = {m.name: m for m in parse_modules(b_path)}
    keys = sorted(set(a_mods) | set(b_mods), key=lambda k: -abs(b_mods.get(k, ModuleSize(k)).total - a_mods.get(k, ModuleSize(k)).total))

    print(f"== diff: {a_path.name} -> {b_path.name} ==")
    print("largest module deltas:")
    shown = 0
    for key in keys:
        a = a_mods.get(key, ModuleSize(key)).total
        b = b_mods.get(key, ModuleSize(key)).total
        delta = b - a
        if delta == 0:
            continue
        print(f"  {delta:+5d}  {key}")
        shown += 1
        if shown >= 20:
            break
    print()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("map", type=Path, nargs="+", help="application .map file(s)")
    parser.add_argument("--compare", type=Path, help="second map to diff against the first")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    for path in args.map:
        print_report(path)
    if args.compare:
        compare(args.map[0], args.compare)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
