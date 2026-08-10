#!/bin/bash
set -euo pipefail

if [[ $# -ne 6 ]]; then
  echo "usage: $0 davecc target interpreter libc source rom-or-dash" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
TARGET="$2"
INTERPRETER="$ROOT/$3"
LIBC="$ROOT/$4"
SOURCE="$ROOT/$5"
ROM="$6"
WORK="${TEST_TMPDIR:-/tmp}/filesystem-syscall-$TARGET"
LANGUAGE_FLAG=""
if [[ "$SOURCE" == *.cpp ]]; then
  LANGUAGE_FLAG="-std=c++23"
fi
mkdir -p "$WORK"

cd "$ROOT"
if [[ "$TARGET" == "pcode" ]]; then
  "$DAVECC" -target pcode -static -Wl,-e -Wl,main \
    ${LANGUAGE_FLAG:+"$LANGUAGE_FLAG"} -isystem libc/include \
    "$SOURCE" "$LIBC" -o "$WORK/test.exe"
  "$INTERPRETER" "$WORK/test.exe"
elif [[ "$TARGET" == "65c02" ]]; then
  "$DAVECC" -target 65c02 -static ${LANGUAGE_FLAG:+"$LANGUAGE_FLAG"} \
    -isystem libc/include \
    "$SOURCE" "$LIBC" -o "$WORK/test.exe"
  "$INTERPRETER" -rom "$ROOT/$ROM" "$WORK/test.exe"
else
  echo "unsupported backend: $TARGET" >&2
  exit 2
fi
