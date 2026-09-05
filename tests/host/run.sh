#!/usr/bin/env bash
set -euo pipefail
ANLAND_TEST_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ANLAND_TEST_TMP="$(mktemp -d)"
trap 'rm -rf "$ANLAND_TEST_TMP"' EXIT
cc -std=c11 -D__ANDROID__ -Wall -Wextra -Werror \
    -I"$ANLAND_TEST_ROOT/tests/host/stubs" \
    "$ANLAND_TEST_ROOT/tests/host/api29_native_test.c" \
    "$ANLAND_TEST_ROOT/app/src/main/jni/anland_core/common/socket_utils.c" \
    -pthread -ldl -o "$ANLAND_TEST_TMP/api29_native_test"
"$ANLAND_TEST_TMP/api29_native_test"
