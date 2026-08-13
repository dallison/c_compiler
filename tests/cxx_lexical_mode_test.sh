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

expect_cpp20_extension() {
  local extension="$1"
  local src="$WORK/default_cpp20.$extension"
  printf '%s\n' \
    '#if __cplusplus != 202002L' \
    '#error expected C++20 mode' \
    '#endif' \
    'namespace inferred { int value = 0; }' \
    'int main(void) { return inferred::value; }' > "$src"
  "$ROOT/$DAVECC" -target pcode -S "$src" \
      -o "$WORK/default_cpp20_$extension.s" \
      >"$WORK/default_cpp20_$extension.out" 2>&1
}

expect_cpp20_extension cc
expect_cpp20_extension cpp

iostream_src="$WORK/default_iostream.cc"
printf '%s\n' \
  '#include <iostream>' \
  'int main(void) { std::cout << "hello world\n"; }' > "$iostream_src"
"$ROOT/$DAVECC" -target aarch64 -S -isystem "$ROOT/libc/include" \
    "$iostream_src" -o "$WORK/default_iostream.s" \
    >"$WORK/default_iostream.out" 2>&1
if [[ -s "$WORK/default_iostream.out" ]]; then
  echo "default_iostream: unexpected diagnostics" >&2
  sed 's/^/  /' "$WORK/default_iostream.out" >&2
  exit 1
fi

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
  "constexpr unsigned long long operator\"\"_suffix(unsigned long long value) { return value; }
#define N 0b1010'0101_suffix
int main(void) { return N == 165 ? 0 : 1; }" \
  -std=c++14

expect_fail c_mode_user_defined_literal \
  'int main(void) { return 123_km; }'
expect_compile cxx11_user_defined_literal \
  'constexpr unsigned long long operator""_km(unsigned long long value) { return value; }
int main(void) { return 123_km == 123 ? 0 : 1; }' \
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
expect_compile cxx20_module_keywords \
  'int module; int import; int main(void) { return 0; }' \
  -std=c++20

expect_compile cxx11_constexpr \
  'constexpr int value = 1; int main(void) { return value; }' \
  -std=c++11

expect_compile cxx26_mode \
  '#if __cplusplus != 202603L
#error expected C++26 mode
#endif
#if __cpp_pp_embed != 202502L
#error expected #embed feature macro
#endif
int main(void) { return 0; }' \
  -std=c++26
expect_compile cxx2c_mode_alias \
  '#if __cplusplus != 202603L
#error expected C++26 mode
#endif
int main(void) { return 0; }' \
  -std=c++2c

printf '\x00\x01\x7f\x80\xff' > "$WORK/embed.bin"
: > "$WORK/empty.bin"
mkdir "$WORK/unprocessable.bin"
expect_compile cxx26_embed \
  '#ifndef __has_embed
#error "__has_embed must be defined in C++26"
#endif
#if __has_embed("embed.bin") != __STDC_EMBED_FOUND__
#error "embed.bin must be found and non-empty"
#endif
#if __has_embed("empty.bin") != __STDC_EMBED_EMPTY__
#error "empty.bin must be found and empty"
#endif
#if __has_embed("missing.bin") != __STDC_EMBED_NOT_FOUND__
#error "missing resource must not be found"
#endif
#if __has_embed(<embed.bin>) != __STDC_EMBED_FOUND__
#error "angle resource search must use system paths"
#endif
#if __has_embed("embed.bin" davecc::unknown(1)) != __STDC_EMBED_NOT_FOUND__
#error "unsupported vendor parameters must report not found"
#endif
#if __has_embed("embed.bin" limit(0)) != __STDC_EMBED_EMPTY__
#error "limit(0) must make the resource empty"
#endif
static const unsigned char all[] = {
#embed "embed.bin"
};
static_assert(sizeof(all) == 5);
constexpr int first =
#embed "embed.bin" limit(1)
;
static_assert(first == 0);
constexpr int wrapped =
#embed "embed.bin" limit(1) prefix(1 +) suffix(+ 2)
;
static_assert(wrapped == 3);
static const unsigned char slice[] = {
#embed "embed.bin" limit(2) prefix(9,) suffix(,10)
};
static_assert(sizeof(slice) == 4);
static const unsigned char uintmax_limit[] = {
#embed "embed.bin" limit(18446744073709551615ULL)
};
static_assert(sizeof(uintmax_limit) == 5);
constexpr int empty_fallback =
#embed "empty.bin" if_empty(42)
;
static_assert(empty_fallback == 42);
#define EMBED_RESOURCE "embed.bin"
#define EMBED_LIMIT (1 + 1)
static const unsigned char macro_resource[] = {
#embed EMBED_RESOURCE limit(EMBED_LIMIT)
};
static_assert(sizeof(macro_resource) == 2);
int main(void) { return 0; }' \
  -std=c++26 -I"$WORK" -isystem "$WORK"

expect_fail cxx23_embed \
  'constexpr unsigned char data[] = {
#embed "embed.bin"
};' \
  -std=c++23 -I"$WORK"
expect_fail cxx26_embed_missing \
  'static const unsigned char data[] = {
#embed "missing.bin"
};' \
  -std=c++26 -I"$WORK"
expect_fail cxx26_embed_duplicate_parameter \
  'static const unsigned char data[] = {
#embed "embed.bin" limit(1) limit(2)
};' \
  -std=c++26 -I"$WORK"
expect_fail cxx26_embed_negative_limit \
  'static const unsigned char data[] = {
#embed "embed.bin" limit(-1)
};' \
  -std=c++26 -I"$WORK"
expect_fail cxx26_embed_unbalanced_parameter \
  '#if __has_embed("embed.bin" prefix({))
#endif
int main(void) { return 0; }' \
  -std=c++26 -I"$WORK"
expect_fail cxx26_has_embed_unprocessable \
  '#if __has_embed("unprocessable.bin")
#endif
int main(void) { return 0; }' \
  -std=c++26 -I"$WORK"
expect_fail cxx26_has_embed_unprocessable_unsupported_parameter \
  '#if __has_embed("unprocessable.bin" davecc::unknown)
#endif
int main(void) { return 0; }' \
  -std=c++26 -I"$WORK"

if "$ROOT/$DAVECC" -target pcode -std=c++29 -S "$WORK/no_such.c" \
    -o "$WORK/no_such.s" >"$WORK/bad_std.out" 2>&1; then
  echo "bad_std: expected invalid -std failure" >&2
  exit 1
fi
bad_std_output="$(<"$WORK/bad_std.out")"
if [[ "$bad_std_output" != *"Invalid language standard -std=c++29"* ]]; then
  echo "bad_std: missing invalid -std diagnostic" >&2
  exit 1
fi
