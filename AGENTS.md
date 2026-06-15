# AGENTS

## Project principles

`fujinet-nio-lib` targets small 6502 systems, so code size and RAM use are part of
correctness, especially on BBC Micro targets.

Agents working in this repo should follow these rules.

## BBC size and RAM rules

1. Prefer application-owned buffers over library-owned scratch space.
2. Avoid persistent global buffers unless they are required on every call path.
3. Avoid `malloc` and `free` in library code. Hidden heap usage is usually a size bug.
4. Treat BSS usage as seriously as CODE size. A small program can still fail if RAM
   reservations push it into screen memory or workspace.
5. Do not add convenience layers that drag in heavy runtime support for BBC unless
   the user explicitly wants them.

## Archive/linking rules

`cl65`/`ar65` only pull in archive members that satisfy unresolved symbols. That
means file boundaries matter.

1. When size selectivity matters, keep one externally-useful function per source
   file where practical.
2. Separate rarely-used helpers from hot/common paths so small apps do not link
   unused features.
3. Keep state/data owners separate from public entry points if that prevents large
   unused blocks from linking.
4. When a function needs optional helpers such as JSON, write-path support, or TLS
   rewriting, prefer isolating those helpers into their own files or making the
   calling path explicit.

## BBC implementation strategy

The BBC target is ROM-backed.

1. Prefer calling `fn-rom`/MOS interfaces over reimplementing transport logic.
2. Prefer BBC platform-specific code in `src/platform/bbc/` over generic common C
   when the common code increases footprint.
3. If a BBC path becomes hot or still too large after C-level simplification,
   consider moving it to ASM in a later phase.

## Example/test program rules

1. Example programs should not accidentally dominate map analysis.
2. On BBC, prefer `conio`/simple output over `printf` when possible.
3. Avoid `putenv`/`getenv` emulation on BBC examples if compile-time configuration
   is sufficient, because it can pull in heap support.
4. Keep BBC example buffers intentionally small unless the example is explicitly a
   throughput test.

## Analysis workflow

When changing BBC code, use the map files and app analysis tooling.

1. Build with `.map` and `.lbl` outputs enabled.
2. Use `scripts/app_map_analysis.py` to inspect:
   - app vs library vs runtime totals
   - top modules
   - largest BSS/DATA reservations
3. Use `bbc` and `bbc-clib` builds together to distinguish:
   - library cost
   - generic cc65 runtime cost
   - ROM-backed runtime savings

## Preferred optimization order

1. Remove unused linked objects by improving file granularity.
2. Remove persistent RAM reservations.
3. Simplify example/runtime usage that pulls in heavy support code.
4. Only then consider lower-level C rewrites or ASM conversions.
