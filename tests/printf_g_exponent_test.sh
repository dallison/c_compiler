#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: $0 davecc target interpreter libc" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
TARGET="$2"
INTERPRETER="$ROOT/$3"
LIBC="$ROOT/$4"
SOURCE="$ROOT/tests/regression/printf_g_exponent_test.c"
WORK="${TEST_TMPDIR:-/tmp}/printf-g-exponent-$TARGET"
mkdir -p "$WORK"

"$DAVECC" -target "$TARGET" -static -std=c99 \
  -isystem "$ROOT/libc/include" "$SOURCE" "$LIBC" \
  -o "$WORK/gexp.exe"

set +e
"$INTERPRETER" -i "$WORK/gexp.exe"
rc=$?
set -e
if [[ "$rc" -ne 0 ]]; then
  echo "printf %g exponent test ($TARGET) returned $rc; expected 0" >&2
  exit 1
fi
