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
WORK="${TEST_TMPDIR:-/tmp}/tgmath-$TARGET"
mkdir -p "$WORK"

"$DAVECC" -target "$TARGET" -static -std=c99 \
  -isystem "$ROOT/libc/include" "$SOURCE" "$LIBC" -o "$WORK/tgmath.exe"

if [[ "$TARGET" == "65c02" ]]; then
  "$INTERPRETER" -rom "$ROOT/$ROM" "$WORK/tgmath.exe"
else
  "$INTERPRETER" "$WORK/tgmath.exe"
fi
