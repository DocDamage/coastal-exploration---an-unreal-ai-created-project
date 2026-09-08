#!/usr/bin/env bash
# Standalone helper tests only. No Unreal, vendor, or Windows runtime execution.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
CXX="${CXX:-clang++}"
export ASAN_OPTIONS="detect_leaks=1:halt_on_error=1"
export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1"
for SOURCE in "$ROOT"/tests/*_tests.cpp; do
  NAME="$(basename "$SOURCE" .cpp)"
  printf 'Sanitizer compile/run: %s\n' "$NAME"
  "$CXX" -std=c++17 -Wall -Wextra -Werror -pedantic -O1 -g \
    -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all \
    -I "$ROOT/Plugins/CoastalFoundation/Source/CoastalFoundation/Public" \
    "$SOURCE" -o "$OUT/$NAME"
  "$OUT/$NAME"
done
