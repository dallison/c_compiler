#!/bin/bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: $0 davecc interpreter libc rom" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
LIBC="$ROOT/$3"
ROM="$ROOT/$4"
REG="$ROOT/tests/regression"
WORK="${TEST_TMPDIR:-/tmp}/6502-math-regression"
mkdir -p "$WORK"

ARGS=(-target 65c02 -static -isystem "$ROOT/libc/include")

run_case() {
  local name="$1"
  local expected="$2"
  shift 2
  "$DAVECC" "${ARGS[@]}" "$@" -o "$WORK/$name.exe"
  set +e
  "$INTERPRETER" -rom "$ROM" "$WORK/$name.exe"
  local got=$?
  set -e
  if [[ "$got" -ne "$expected" ]]; then
    echo "FAIL $name expected exit $expected got $got" >&2
    exit 1
  fi
  echo "ok $name"
}

run_case shift_right 255 "$REG/shift_right_6502_test.c"
run_case shift_left 0 "$REG/shift_left_6502_test.c"
run_case fp_cmp_neg3 0 "$REG/fp_cmp_neg3_6502_test.c"
run_case floor_val 30 "$REG/floor_val_6502_test.c" "$ROOT/libc/floor.c" "$ROOT/libc/modf.c"
run_case modf_ipart 0 "$REG/modf_ipart_value_6502_test.c" "$ROOT/libc/modf.c"
run_case modf_37 0 "$REG/modf_37_6502_test.c" "$ROOT/libc/modf.c"
run_case modf_neg 0 "$REG/modf_neg_6502_test.c" "$ROOT/libc/modf.c"
run_case frexp_return 0 "$REG/frexp_return_6502_test.c" "$ROOT/libc/frexp.c"
run_case ldexp_basic 0 "$REG/ldexp_6502_test.c" "$ROOT/libc/ldexp.c"
run_case nested_fp_return 0 "$REG/nested_fp_return_6502_test.c"

echo "ok 6502 math regression suite"
