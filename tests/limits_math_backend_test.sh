#!/bin/bash
set -euo pipefail

if [[ $# -ne 7 ]]; then
  echo "usage: $0 davecc target interpreter libc limits_source math_source rom-or-dash" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
TARGET="$2"
INTERPRETER="$ROOT/$3"
LIBC="$ROOT/$4"
LIMITS_SOURCE="$ROOT/$5"
MATH_SOURCE="$ROOT/$6"
ROM="$7"
WORK="${TEST_TMPDIR:-/tmp}/limits-math-$TARGET"
mkdir -p "$WORK"

ARGS=(-target "$TARGET" -static -isystem "$ROOT/libc/include")

if [[ "$TARGET" == "pcode" ]]; then
  ARGS+=(-Wl,-e -Wl,main)
fi

"$DAVECC" "${ARGS[@]}" -std=c++23 "$LIMITS_SOURCE" "$LIBC" -o "$WORK/limits.exe"
"$DAVECC" "${ARGS[@]}" "$MATH_SOURCE" "$LIBC" -o "$WORK/math.exe"

if [[ "$TARGET" == "65c02" ]]; then
  "$INTERPRETER" -rom "$ROOT/$ROM" "$WORK/limits.exe"
  "$INTERPRETER" -rom "$ROOT/$ROM" "$WORK/math.exe"
else
  "$INTERPRETER" "$WORK/limits.exe"
  "$INTERPRETER" "$WORK/math.exe"
fi
