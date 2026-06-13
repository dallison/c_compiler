#!/bin/bash
set -euo pipefail

DAVECC="$1"

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/cxx-lexical-mode.XXXXXX")"

expect_compile() {
  local name="$1"
  local source="$2"
  shift 2
  local src="$WORK/$name.c"
  printf '%s\n' "$source" > "$src"
  "$ROOT/$DAVECC" -target pcode -S "$@" "$src" -o "$WORK/$name.s" \
      >"$WORK/$name.out" 2>&1
}

expect_fail() {
  local name="$1"
  local source="$2"
  shift 2
  local src="$WORK/$name.c"
  printf '%s\n' "$source" > "$src"
  set +e
  "$ROOT/$DAVECC" -target pcode -S "$@" "$src" -o "$WORK/$name.s" \
      >"$WORK/$name.out" 2>&1
  local status=$?
  set -e
  local output
  output="$(<"$WORK/$name.out")"
  if [[ "$status" -eq 0 && "$output" != *"error:"* ]]; then
    echo "$name: expected compilation failure" >&2
    exit 1
  fi
}

expect_compile c_mode_class \
  'int class; int main(void) { class = 3; return class; }'
expect_fail cxx11_class \
  'int class; int main(void) { return 0; }' \
  -std=c++11

expect_compile cxx17_concept \
  'int concept; int main(void) { concept = 4; return concept; }' \
  -std=c++17
expect_fail cxx20_concept \
  'int concept; int main(void) { return 0; }' \
  -std=c++20

expect_compile cxx17_char8_t \
  'int char8_t; int main(void) { char8_t = 5; return char8_t; }' \
  -std=c++17
expect_fail cxx20_char8_t \
  'int char8_t; int main(void) { return 0; }' \
  -std=c++20

expect_compile c_mode_alt_operator_word \
  'int and; int main(void) { and = 1; return and; }'
expect_fail cxx11_alt_operator_word \
  'int and; int main(void) { return 0; }' \
  -std=c++11

expect_compile cxx11_alt_operator_expr \
  'int main(void) { return (1 and 1) && (1 not_eq 0); }' \
  -std=c++11

expect_fail cxx11_constexpr \
  'int constexpr; int main(void) { return 0; }' \
  -std=c++11

if "$ROOT/$DAVECC" -target pcode -std=c++26 -S "$WORK/no_such.c" \
    -o "$WORK/no_such.s" >"$WORK/bad_std.out" 2>&1; then
  echo "bad_std: expected invalid -std failure" >&2
  exit 1
fi
bad_std_output="$(<"$WORK/bad_std.out")"
if [[ "$bad_std_output" != *"Invalid language standard -std=c++26"* ]]; then
  echo "bad_std: missing invalid -std diagnostic" >&2
  exit 1
fi
