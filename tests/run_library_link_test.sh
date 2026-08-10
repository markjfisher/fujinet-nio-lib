#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIBRARY="$ROOT/build/fujinet-nio-linux.a"
OUT="$ROOT/build/tests/library_link_test"

test -f "$LIBRARY"
mkdir -p "$(dirname "$OUT")"
gcc -std=c99 -Wall -Wextra -Werror \
    -I"$ROOT/include" \
    "$ROOT/tests/library_link_test.c" \
    "$LIBRARY" \
    -o "$OUT"
"$OUT"
