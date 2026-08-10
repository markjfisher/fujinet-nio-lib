#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/tests/disk_wire_test"

mkdir -p "$(dirname "$OUT")"
gcc -std=c99 -Wall -Wextra -Werror \
    -I"$ROOT/include" \
    "$ROOT/tests/disk_wire_test.c" \
    "$ROOT/src/common/fn_disk.c" \
    -o "$OUT"
"$OUT"
