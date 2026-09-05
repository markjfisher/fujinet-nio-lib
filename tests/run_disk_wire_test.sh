#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/tests/disk_wire_test"

mkdir -p "$(dirname "$OUT")"
gcc -std=c99 -Wall -Wextra -Werror \
    -I"$ROOT/include" \
    "$ROOT/tests/disk_wire_test.c" \
    "$ROOT/src/common/fn_disk.c" \
    "$ROOT/src/common/fn_packet_header.c" \
    "$ROOT/src/common/fn_checksum_fold.c" \
    "$ROOT/src/common/fn_packet_checksum.c" \
    "$ROOT/src/common/fn_packet_checksum_packet.c" \
    -o "$OUT"
"$OUT"
