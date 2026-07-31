#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/tests/slot_catalog_wire_test"

mkdir -p "$(dirname "$OUT")"
gcc -std=c99 -Wall -Wextra -Werror \
    -I"$ROOT/include" \
    "$ROOT/tests/slot_catalog_wire_test.c" \
    "$ROOT/src/common/fn_slot_catalog_common.c" \
    "$ROOT/src/common/fn_slot_catalog_validate_default.c" \
    "$ROOT/src/common/fn_slot_catalog_get.c" \
    "$ROOT/src/common/fn_slot_catalog_put.c" \
    "$ROOT/src/common/fn_slot_catalog_delete.c" \
    "$ROOT/src/common/fn_slot_catalog_range.c" \
    "$ROOT/src/common/fn_slot_catalog_next_entry.c" \
    -o "$OUT"
"$OUT"
