#!/bin/sh
set -eu

mkdir -p build/tests
gcc -std=c99 -Wall -Wextra -Werror -Iinclude \
    tests/session_wire_test.c \
    src/common/fn_session.c \
    src/common/fn_slip.c \
    -o build/tests/session_wire_test
build/tests/session_wire_test
