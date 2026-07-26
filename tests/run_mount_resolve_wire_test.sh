#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/tests/mount_resolve_wire_test"

mkdir -p "$(dirname "$OUT")"
gcc -std=c99 -Wall -Wextra -Werror \
    -I"$ROOT/include" \
    "$ROOT/tests/mount_resolve_wire_test.c" \
    "$ROOT/src/common/fn_mount_resolve_common.c" \
    "$ROOT/src/common/fn_mount_resolve_build.c" \
    "$ROOT/src/common/fn_mount_resolve_default.c" \
    "$ROOT/src/common/fn_resolve_mount_target.c" \
    "$ROOT/src/common/fn_format_mount_display.c" \
    -o "$OUT"
"$OUT"
