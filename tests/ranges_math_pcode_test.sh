#!/bin/bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 davecc interpreter libc" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
LIBC="$ROOT/$3"
SOURCE="$ROOT/cxx_testsuite/tests/exec_ranges/0004_ranges_math.cpp"
EXE="${TEST_TMPDIR:-/tmp}/ranges-math-pcode.exe"

"$DAVECC" -target pcode -O2 -static -std=c++20 -isystem "$ROOT/libc/include" \
  -Wl,-e -Wl,main "$SOURCE" "$LIBC" -o "$EXE"
"$INTERPRETER" "$EXE"
