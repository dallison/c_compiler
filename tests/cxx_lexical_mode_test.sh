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
  'int and = 1; int main(void) { return and; }' \
  -std=c++11

expect_compile cxx11_alt_operator_expr \
  'int main(void) { return (1 and 1) && (1 not_eq 0); }' \
  -std=c++11

expect_fail cxx11_digit_separator \
  "int main(void) { return 1'000; }" \
  -std=c++11
expect_compile cxx14_digit_separator \
  "int main(void) { return 1'000 == 1000 ? 0 : 1; }" \
  -std=c++14
expect_fail cxx11_binary_literal \
  'int main(void) { return 0b1010; }' \
  -std=c++11
expect_compile cxx14_binary_literal \
  "int main(void) { return 0b1010'0101 == 165 ? 0 : 1; }" \
  -std=c++14
expect_compile cxx14_macro_pp_number \
  "#define N 0b1010'0101_suffix
int main(void) { return N == 165 ? 0 : 1; }" \
  -std=c++14

expect_fail c_mode_user_defined_literal \
  'int main(void) { return 123_km; }'
expect_compile cxx11_user_defined_literal \
  'int main(void) { return 123_km == 123 ? 0 : 1; }' \
  -std=c++11

expect_compile cxx11_prefixed_literals \
  'int main(void) { u8"text"; u"text"; U"text"; u8'"'"'x'"'"'; u'"'"'x'"'"'; U'"'"'x'"'"'; return 0; }' \
  -std=c++11
expect_compile cxx11_raw_literals \
  'int main(void) { R"delim(raw \ text)delim"; u8R"(raw)"; LR"(raw)"; return 0; }' \
  -std=c++11

multiline_raw="$WORK/cxx11_multiline_raw.c"
printf '%s\n' \
  'int main(void) { R"raw(first line' \
  'second line)raw"; return 0; }' > "$multiline_raw"
"$ROOT/$DAVECC" -target pcode -S -std=c++11 "$multiline_raw" \
    -o "$WORK/cxx11_multiline_raw.s" >"$WORK/cxx11_multiline_raw.out" 2>&1

macro_raw="$WORK/cxx11_macro_raw.c"
printf '%s\n' \
  '#define RAW R"raw(a"b)raw"' \
  'int main(void) { RAW; return 0; }' > "$macro_raw"
"$ROOT/$DAVECC" -target pcode -S -std=c++11 "$macro_raw" \
    -o "$WORK/cxx11_macro_raw.s" >"$WORK/cxx11_macro_raw.out" 2>&1

printf '%s\n' 'int pp_header_value(void) { return 0; }' > "$WORK/pp_header.h"
include_header="$WORK/cxx11_include_header.c"
printf '%s\n' \
  '#include <pp_header.h>' \
  'int main(void) { return pp_header_value(); }' > "$include_header"
"$ROOT/$DAVECC" -target pcode -S -std=c++11 -isystem "$WORK" "$include_header" \
    -o "$WORK/cxx11_include_header.s" >"$WORK/cxx11_include_header.out" 2>&1

expect_compile cxx17_module_identifiers \
  'int module; int import; int main(void) { module = 1; import = 2; return module + import; }' \
  -std=c++17
expect_fail cxx20_module_keywords \
  'int module; int import; int main(void) { return 0; }' \
  -std=c++20

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
