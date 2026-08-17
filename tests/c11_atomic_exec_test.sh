#!/bin/bash
set -euo pipefail

if [[ $# -ne 5 ]]; then
  echo "usage: $0 <davecc> <target> <interpreter> <libc> <source>" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
TARGET="$2"
INTERPRETER="$ROOT/$3"
LIBC="$ROOT/$4"
SOURCE="$ROOT/$5"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/c11-atomic-exec.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

EXTRA_COMPILE=()
EXTRA_INTERPRETER=()
if [[ "$TARGET" != "riscv" ]]; then
  EXTRA_COMPILE=(-Wl,-e -Wl,main)
  EXTRA_INTERPRETER=(-i)
fi

"$DAVECC" -target "$TARGET" -std=c11 -O2 -static -isystem "$ROOT/libc/include" \
    "${EXTRA_COMPILE[@]}" "$SOURCE" "$LIBC" -o "$WORK/c11_atomic.exe"
"$INTERPRETER" "${EXTRA_INTERPRETER[@]}" "$WORK/c11_atomic.exe"
