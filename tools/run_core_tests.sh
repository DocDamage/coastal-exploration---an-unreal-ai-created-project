#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
CXX="${CXX:-g++}"
for SOURCE in "$ROOT"/tests/*_tests.cpp; do
  NAME="$(basename "$SOURCE" .cpp)"
  "$CXX" -std=c++17 -Wall -Wextra -Werror -pedantic \
    -I "$ROOT/Plugins/CoastalFoundation/Source/CoastalFoundation/Public" \
    "$SOURCE" -o "$OUT/$NAME"
  "$OUT/$NAME"
done
