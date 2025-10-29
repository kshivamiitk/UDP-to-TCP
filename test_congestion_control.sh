#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build/tests"

mkdir -p "$BUILD_DIR"

printf 'Compiling congestion control test...\n'

g++ -std=c++17 -Wall -Wextra -pedantic -I"$SCRIPT_DIR/include" \
    "$SCRIPT_DIR/tests/congestion_test.cpp" -o "$BUILD_DIR/congestion_test"

printf 'Running congestion control test...\n'
"$BUILD_DIR/congestion_test"