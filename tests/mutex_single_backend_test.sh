#!/bin/bash
set -euo pipefail

if [[ $# -lt 4 || $# -gt 5 ]]; then
  echo "usage: $0 <davecc> <target> <interpreter> <libc> [rom]" >&2
  exit 2
fi

DAVECC=$1
TARGET=$2
INTERPRETER=$3
LIBC=$4
ROM=${5:-}
ROOT=$(pwd)
WORK="${TEST_TMPDIR:-/tmp}/mutex-${TARGET}"
rm -rf "$WORK"
mkdir -p "$WORK"

SOURCE="$ROOT/cxx_testsuite/tests/exec/0282_standard_mutex.cpp"
EXE="$WORK/mutex.exe"
ARGS=(
  -target "$TARGET"
  -static
  -fno-exceptions
  -std=c++20
  -isystem "$ROOT/libc/include"
)

if [[ "$TARGET" == "pcode" ]]; then
  ARGS+=(-Wl,-e -Wl,main)
fi

"$DAVECC" "${ARGS[@]}" "$SOURCE" "$LIBC" -o "$EXE"

COMMAND=("$INTERPRETER")
if [[ -n "$ROM" ]]; then
  COMMAND+=(-rom "$ROM")
fi
COMMAND+=("$EXE")
"${COMMAND[@]}"
