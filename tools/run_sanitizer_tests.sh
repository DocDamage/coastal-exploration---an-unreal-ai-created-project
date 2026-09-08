#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; OUT="$(mktemp -d)"; trap 'rm -rf "$OUT"' EXIT
for SOURCE in "$ROOT"/tests/*_tests.cpp; do
 NAME="$(basename "$SOURCE" .cpp)"
 clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined \
  -I "$ROOT/Plugins/CoastalFoundation/Source/CoastalFoundation/Public" "$SOURCE" -o "$OUT/$NAME"
 ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1}" UBSAN_OPTIONS=halt_on_error=1 "$OUT/$NAME"
done
