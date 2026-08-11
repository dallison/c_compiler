#!/bin/bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 davecc interpreter" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
WORK="${TEST_TMPDIR:-/tmp}/x86-constexpr-bool"
mkdir -p "$WORK"

"$DAVECC" -target x86_64 -static -isystem "$ROOT/libc/include" -std=c++23 \
  "$ROOT/tests/regression/constexpr_bool_x86_test.cpp" \
  "$ROOT/libc_x86_64" -o "$WORK/test.exe"
"$INTERPRETER" "$WORK/test.exe"

echo "ok x86 constexpr bool / limits codegen"
