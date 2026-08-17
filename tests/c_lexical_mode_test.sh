#!/bin/bash
set -euo pipefail

DAVECC="$1"

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/c-lexical-mode.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

expect_compile() {
  local name="$1"
  local source="$2"
  shift 2
  local src="$WORK/$name.c"
  printf '%s\n' "$source" > "$src"
  if ! "$ROOT/$DAVECC" -target pcode -S "$@" "$src" -o "$WORK/$name.s" \
      >"$WORK/$name.out" 2>&1; then
    echo "$name: expected compilation success" >&2
    sed 's/^/  /' "$WORK/$name.out" >&2
    exit 1
  fi
  if [[ -s "$WORK/$name.out" ]]; then
    echo "$name: unexpected diagnostics" >&2
    sed 's/^/  /' "$WORK/$name.out" >&2
    exit 1
  fi
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

expect_fail_with() {
  local name="$1"
  local pattern="$2"
  local source="$3"
  shift 3
  expect_fail "$name" "$source" "$@"
  local output
  output="$(<"$WORK/$name.out")"
  if [[ "$output" != *"$pattern"* ]]; then
    echo "$name: missing expected diagnostic: $pattern" >&2
    sed 's/^/  /' "$WORK/$name.out" >&2
    exit 1
  fi
}

expect_compile c89_version \
  '#ifdef __STDC_VERSION__
#error C89 must not define __STDC_VERSION__
#endif
int main(void) { return 0; }' \
  -std=c89

expect_compile default_c99_version \
  '#if __STDC_VERSION__ != 199901L
#error default C mode must be C99
#endif
int main(void) { return 0; }'

expect_compile c11_version \
  '#if __STDC_VERSION__ != 201112L
#error expected C11
#endif
int main(void) { return 0; }' \
  -std=c11

expect_compile c17_version \
  '#if __STDC_VERSION__ != 201710L
#error expected C17
#endif
int main(void) { return 0; }' \
  -std=c17

c23_version_source='#if __STDC_VERSION__ != 202311L
#error expected C23
#endif
#ifdef __cplusplus
#error C mode must not define __cplusplus
#endif
int main(void) { return 0; }'
expect_compile c23_version "$c23_version_source" -std=c23
expect_compile c2x_alias "$c23_version_source" -std=c2x
expect_compile gnu23_alias "$c23_version_source" -std=gnu23
expect_compile gnu2x_alias "$c23_version_source" -std=gnu2x
expect_compile iso9899_2024_alias "$c23_version_source" -std=iso9899:2024

expect_compile c17_c23_words_are_identifiers \
  'int alignas;
int alignof;
int bool;
int false;
int static_assert;
int thread_local;
int true;
int main(void) { return 0; }' \
  -std=c17

expect_compile c99_c11_words_are_identifiers \
  'int _Alignas;
int _Alignof;
int _Atomic;
int _Noreturn;
int _Static_assert;
int _Thread_local;
int main(void) { return 0; }' \
  -std=c99

expect_compile c11_atomic_and_noreturn \
  '_Atomic int value;
_Noreturn void stop(void) { for (;;) {} }
int main(void) { value = 1; return value - 1; }' \
  -std=c11 -target x86_64

expect_compile c17_atomic_and_noreturn \
  '_Atomic(int *) pointer;
_Noreturn void stop(void) { for (;;) {} }
int main(void) { return pointer != 0; }' \
  -std=c17 -target x86_64

expect_compile c23_bool_constants \
  'bool enabled = true;
bool disabled = false;
static_assert(sizeof(bool) == sizeof(_Bool));
int main(void) { return enabled && !disabled ? 0 : 1; }' \
  -std=c23

expect_compile c11_static_assert \
  '_Static_assert(sizeof(int) >= 2, "int is too small");
struct holder {
  _Static_assert(sizeof(char) == 1, "char is one byte");
  int value;
};
_Static_assert(_Generic(1, int: 1, default: 0), "generic selection failed");
int main(void) { return 0; }' \
  -std=c11
expect_compile c99_generic_is_identifier \
  'static int _Generic(void) { return 0; }
int main(void) { return _Generic(); }' \
  -std=c99
expect_fail c11_static_assert_requires_message \
  '_Static_assert(sizeof(int) >= 2);
int main(void) { return 0; }' \
  -std=c11
expect_compile c23_static_assert_without_message \
  'static_assert(sizeof(int) >= 2);
_Static_assert(sizeof(char) == 1);
int main(void) { return 0; }' \
  -std=c23

expect_compile c11_alignment \
  '_Alignas(16) static char storage[16];
_Static_assert(_Alignof(int) >= 1, "invalid int alignment");
int main(void) { return _Alignof(storage) == 16 ? 0 : 1; }' \
  -std=c11
expect_compile c23_alignment \
  'alignas(16) static char storage[16];
struct aligned_member { alignas(16) char value; };
static_assert(alignof(int) >= 1);
static_assert(alignof(struct aligned_member) == 16);
int main(void) { return alignof(storage) == 16 ? 0 : 1; }' \
  -std=c23

expect_compile c11_thread_local \
  '_Thread_local int value;
int main(void) { return value; }' \
  -std=c11
expect_compile c23_thread_local \
  'thread_local int value;
int main(void) { return value; }' \
  -std=c23

expect_compile c23_attributes \
  '#ifndef __has_c_attribute
#error __has_c_attribute must be defined in C23
#endif
#if __has_c_attribute(deprecated) != 201904L
#error deprecated attribute probe has wrong value
#endif
#if __has_c_attribute(nodiscard) != 202003L
#error nodiscard attribute probe has wrong value
#endif
#if __has_c_attribute(noreturn) != 202202L
#error noreturn attribute probe has wrong value
#endif
#if __has_c_attribute(unsequenced) != 202207L
#error unsequenced attribute probe has wrong value
#endif
#if __has_c_attribute(davecc_unknown_attribute) != 0
#error unknown C attribute must report zero
#endif
[[maybe_unused]] static int unused_value;
[[nodiscard]] static int answer(void) { return 42; }
struct holder {
  [[maybe_unused]] int value;
  static_assert(sizeof(int) >= 2);
};
static int fallthrough_test(int value) {
  switch (value) {
    case 0:
      value++;
      [[fallthrough]];
    default:
      return value;
  }
}
int main(void) {
  return answer() == 42 && fallthrough_test(0) == 1 ? 0 : 1;
}' \
  -std=c23
expect_compile c17_has_c_attribute_undefined \
  '#ifdef __has_c_attribute
#error __has_c_attribute must not be defined before C23
#endif
int main(void) { return 0; }' \
  -std=c17
expect_fail c17_attributes \
  '[[maybe_unused]] static int unused_value;
int main(void) { return 0; }' \
  -std=c17

expect_compile c23_binary_and_digit_separators \
  '#if 0b1010 != 10
#error binary preprocessing constant has wrong value
#endif
#if 1'"'"'000 != 1000
#error separated preprocessing constant has wrong value
#endif
int main(void) { return 0b1010'"'"'0101 == 165 ? 0 : 1; }' \
  -std=c23
expect_fail c17_binary_literal \
  'int main(void) { return 0b1; }' \
  -std=c17
expect_fail c17_digit_separator \
  "int main(void) { return 1'000; }" \
  -std=c17

elifdef_source='#define PRESENT 1
#if 0
#elifdef PRESENT
static int present;
#else
#error elifdef did not select PRESENT
#endif
#if 0
#elifndef ABSENT
static int absent;
#else
#error elifndef did not select ABSENT
#endif
int main(void) { return present + absent; }'
expect_compile c23_elifdef "$elifdef_source" -std=c23
expect_fail_with c17_elifdef "Invalid preprocessor directive elifdef" \
  "$elifdef_source" -std=c17

printf '\x01\x02\xff' > "$WORK/embed.bin"
: > "$WORK/empty.bin"
embed_source='#ifndef __has_embed
#error __has_embed must be available in C23
#endif
#if __has_embed("embed.bin") != __STDC_EMBED_FOUND__
#error embed.bin must be found
#endif
#if __has_embed("empty.bin") != __STDC_EMBED_EMPTY__
#error empty.bin must be empty
#endif
#if __has_embed("missing.bin") != __STDC_EMBED_NOT_FOUND__
#error missing.bin must not be found
#endif
static const unsigned char bytes[] = {
#embed "embed.bin"
};
static_assert(sizeof(bytes) == 3);
int main(void) { return bytes[0] == 1 && bytes[2] == 255 ? 0 : 1; }'
expect_compile c23_embed "$embed_source" -std=c23 -I"$WORK"
expect_fail_with c17_embed "Invalid preprocessor directive embed" \
  'static const unsigned char bytes[] = {
#embed "embed.bin"
};
int main(void) { return 0; }' \
  -std=c17 -I"$WORK"

expect_compile c23_va_opt \
  '#define VALUE(...) 1 __VA_OPT__(+ (__VA_ARGS__))
#define COMMA(...) __VA_OPT__(,)
#define PASTE(prefix, ...) prefix ## __VA_OPT__(__VA_ARGS__)
#define STRINGIFY(...) __VA_OPT__(#__VA_ARGS__)
#define HAS_TOKENS(...) 0 __VA_OPT__(+ 1)
static_assert(VALUE() == 1);
static_assert(VALUE(2) == 3);
static_assert(HAS_TOKENS() == 0);
static_assert(HAS_TOKENS(,) == 1);
static_assert(sizeof((int[]){10 COMMA()}) == sizeof(int));
static_assert(sizeof((int[]){10 COMMA(ignored) 20}) == 2 * sizeof(int));
static_assert(sizeof(STRINGIFY(alpha,beta)) == sizeof("alpha,beta"));
int PASTE(value,) = 4;
int PASTE(value, 2) = 5;
int main(void) { return value + value2 == 9 ? 0 : 1; }' \
  -std=c23
expect_fail_with c17_va_opt "__VA_OPT__ requires C23 or C++20" \
  '#define VALUE(...) 1 __VA_OPT__(+ (__VA_ARGS__))
int main(void) { return VALUE(2); }' \
  -std=c17
expect_fail_with c23_va_opt_non_variadic \
  "variadic function-like macro" \
  '#define BAD(x) __VA_OPT__(x)
int main(void) { return BAD(0); }' \
  -std=c23
expect_fail_with c23_va_opt_unbalanced "Missing ')' in __VA_OPT__" \
  '#define BAD(...) __VA_OPT__((__VA_ARGS__)
int main(void) { return 0; }' \
  -std=c23
expect_fail_with c23_va_opt_nested "cannot be nested" \
  '#define BAD(...) __VA_OPT__(__VA_OPT__(__VA_ARGS__))
int main(void) { return BAD(1); }' \
  -std=c23

expect_compile c11_utf8_string \
  '_Static_assert(_Generic(u8"text"[0], char: 1, default: 0),
               "C11 UTF-8 strings have char elements");
int main(void) { return u8"text"[0] == '"'"'t'"'"' ? 0 : 1; }' \
  -std=c11
expect_compile c23_utf8_literals \
  '#if __STDC_UTF_8__ != 1
#error C23 must advertise UTF-8 encoding
#endif
static_assert(_Generic(u8"text"[0], unsigned char: 1, default: 0));
static_assert(_Generic(u8'"'"'a'"'"', unsigned char: 1, default: 0));
int main(void) { return u8'"'"'a'"'"' == 97 ? 0 : 1; }' \
  -std=c23
expect_fail c17_utf8_character \
  'int main(void) { return u8'"'"'a'"'"'; }' \
  -std=c17

expect_compile c17_c23_type_keyword_identifiers \
  'int typeof = 1;
int typeof_unqual = 2;
int nullptr = 3;
int constexpr = 4;
int main(void) { return typeof + typeof_unqual + nullptr + constexpr - 10; }' \
  -std=c17
expect_compile c23_type_keywords \
  'constexpr int value = 1;
static_assert(_Generic(nullptr, typeof(nullptr): 1, default: 0));
typeof_unqual(const int) plain = 2;
int main(void) { return value + plain - 3; }' \
  -std=c23

invalid_src="$WORK/invalid_std.c"
printf '%s\n' 'int main(void) { return 0; }' > "$invalid_src"
set +e
"$ROOT/$DAVECC" -target pcode -std=c24 -S "$invalid_src" \
    -o "$WORK/invalid_std.s" >"$WORK/invalid_std.out" 2>&1
invalid_status=$?
set -e
if [[ "$invalid_status" -eq 0 ]]; then
  echo "invalid_std: expected invalid -std failure" >&2
  exit 1
fi
invalid_output="$(<"$WORK/invalid_std.out")"
if [[ "$invalid_output" != *"Invalid language standard -std=c24"* ]]; then
  echo "invalid_std: missing invalid -std diagnostic" >&2
  exit 1
fi
