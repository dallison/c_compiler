#!/bin/bash
set -euo pipefail

DAVECC="$1"
X86_64="$2"
LIBC="$3"
SRC="$4"
TYPEID_CASE="$5"
DYNAMIC_CAST_CASE="$6"

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/itanium-rtti.XXXXXX")"

EXE="$WORK/rtti.exe"
ASM="$WORK/rtti.s"

"$ROOT/$DAVECC" -target x86_64 -static -std=c++20 -isystem "$ROOT/libc/include" \
  -S "$ROOT/$SRC" -o "$ASM"

if ! grep -q '_ZTS' "$ASM"; then
  echo "missing _ZTS symbol in generated assembly" >&2
  exit 1
fi
if ! grep -q '_ZTI' "$ASM"; then
  echo "missing _ZTI symbol in generated assembly" >&2
  exit 1
fi
if grep -q '__davecc_ti_' "$ASM"; then
  echo "unexpected legacy __davecc_ti_ symbol on x86_64" >&2
  exit 1
fi

"$ROOT/$DAVECC" -target x86_64 -static -std=c++20 -isystem "$ROOT/libc/include" \
  -Wl,-e -Wl,main "$ROOT/$SRC" "$ROOT/$LIBC" -o "$EXE"

set +e
"$ROOT/$X86_64" -i "$EXE"
status=$?
set -e
if [[ "$status" -ne 0 ]]; then
  echo "runtime: expected exit 0, got $status" >&2
  exit 1
fi

for case in "$TYPEID_CASE" "$DYNAMIC_CAST_CASE"; do
  case_exe="$WORK/$(basename "$case" .cpp).exe"
  "$ROOT/$DAVECC" -target x86_64 -static -std=c++20 -isystem "$ROOT/libc/include" \
    -Wl,-e -Wl,main "$ROOT/$case" "$ROOT/$LIBC" -o "$case_exe"
  set +e
  "$ROOT/$X86_64" -i "$case_exe"
  case_status=$?
  set -e
  if [[ "$case_status" -ne 0 ]]; then
    echo "$case: expected exit 0, got $case_status" >&2
    exit 1
  fi
done

echo "itanium_rtti_test: ok"
