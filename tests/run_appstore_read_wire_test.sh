#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/tests/appstore_read_wire_test"

mkdir -p "$(dirname "$OUT")"
gcc -std=c99 -Wall -Wextra -Werror \
    -I"$ROOT/include" \
    "$ROOT/tests/appstore_read_wire_test.c" \
    "$ROOT/src/common/fn_appstore_common.c" \
    "$ROOT/src/common/fn_appstore_read.c" \
    -o "$OUT"
"$OUT"
