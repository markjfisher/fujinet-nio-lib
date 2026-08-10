#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/tests/disk_context_test"

mkdir -p "$(dirname "$OUT")"
gcc -std=c99 -Wall -Wextra -Werror \
    -I"$ROOT/include" \
    "$ROOT/tests/disk_context_test.c" \
    "$ROOT/src/common/fn_disk.c" \
    "$ROOT/src/common/fn_packet_header.c" \
    "$ROOT/src/common/fn_packet_checksum.c" \
    -o "$OUT"
"$OUT"
