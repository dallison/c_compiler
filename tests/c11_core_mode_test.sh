#!/bin/bash
set -euo pipefail

DAVECC="$1"
ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/c11-core-mode.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

compile_source() {
  local name="$1"
  local target="$2"
  local source="$3"
  shift 3
  local src="$WORK/$name.c"
  printf '%s\n' "$source" > "$src"
  "$ROOT/$DAVECC" -target "$target" -std=c11 -S "$@" "$src" \
      -o "$WORK/$name.s" >"$WORK/$name.out" 2>&1
}

expect_compile() {
  local name="$1"
  local target="$2"
  local source="$3"
  shift 3
  if ! compile_source "$name" "$target" "$source" "$@"; then
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
  local target="$2"
  local pattern="$3"
  local source="$4"
  shift 4
  set +e
  compile_source "$name" "$target" "$source" "$@"
  local status=$?
  set -e
  local output
  output="$(<"$WORK/$name.out")"
  if [[ "$status" -eq 0 && "$output" != *"error:"* ]]; then
    echo "$name: expected compilation failure" >&2
    exit 1
  fi
  if [[ "$output" != *"$pattern"* ]]; then
    echo "$name: missing expected diagnostic: $pattern" >&2
    sed 's/^/  /' "$WORK/$name.out" >&2
    exit 1
  fi
}

expect_compile atomic_language x86_64 \
  '_Atomic int value = 1;
_Atomic(int *) pointer;
struct holder { _Atomic unsigned count; };
_Noreturn void stop(void) { for (;;) {} }
int test(void) {
  int old = value;
  value = old + 1;
  value++;
  ++value;
  value += 3;
  value -= 2;
  return value;
}'

expect_fail atomic_aarch64_profile aarch64 \
  "atomic types are unavailable on this target" \
  '#ifndef __STDC_NO_ATOMICS__
#error aarch64 must advertise incomplete C11 atomic support
#endif
_Atomic int value;'

expect_compile atomic_language_riscv riscv \
  '_Atomic int value;
int test(void) { value++; value += 2; return value; }'

expect_compile noreturn_redeclaration x86_64 \
  'void stop(void);
_Noreturn void stop(void) { for (;;) {} }
_Noreturn void again(void);
void again(void) { for (;;) {} }'

expect_compile atomic_header x86_64 \
  '#include <stdatomic.h>
#include <stdnoreturn.h>
atomic_int value = ATOMIC_VAR_INIT(1);
atomic_flag flag = ATOMIC_FLAG_INIT;
noreturn void stop(void) { for (;;) {} }
int test(void) {
  int expected = 1;
  atomic_store(&value, 2);
  if (!atomic_compare_exchange_strong(&value, &expected, 3)) {
    return 1;
  }
  if (atomic_fetch_add(&value, 2) != 3) {
    return 2;
  }
  if (atomic_fetch_or(&value, 2) != 5 ||
      atomic_fetch_and(&value, 6) != 7 ||
      atomic_fetch_xor(&value, 3) != 6) {
    return 5;
  }
  if (atomic_flag_test_and_set(&flag)) {
    return 3;
  }
  atomic_flag_clear(&flag);
  atomic_thread_fence(memory_order_seq_cst);
  return atomic_load(&value) == 5 ? 0 : 4;
}'

expect_compile c23_stdnoreturn x86_64 \
  '#include <stdnoreturn.h>
noreturn void stop(void) { for (;;) {} }
int test(void) { return 0; }' \
  -std=c23

expect_compile c23_fenv x86_64 \
  '#include <fenv.h>
int test(void) {
  femode_t mode;
  fexcept_t flags = FE_INVALID;
  return fegetmode(&mode) || fesetmode(FE_DFL_MODE) ||
         fesetexcept(FE_INVALID) ||
         fetestexceptflag(&flags, FE_INVALID) != FE_INVALID;
}' \
  -std=c23

expect_fail atomic_aggregate x86_64 \
  "_Atomic currently supports only scalar and pointer types" \
  'struct pair { int x; int y; };
_Atomic(struct pair) value;'

expect_fail atomic_bitfield x86_64 \
  "Atomic-qualified member cannot be a bitfield" \
  'struct bits { _Atomic unsigned value : 1; };'

expect_fail atomic_nested x86_64 \
  "_Atomic cannot be applied to an atomic type" \
  '_Atomic(_Atomic int) value;'

expect_fail atomic_redeclaration x86_64 \
  "redeclared with different type" \
  '_Atomic int value;
int value;'

expect_fail noreturn_object x86_64 \
  "'noreturn' attribute applies only to functions" \
  '_Noreturn int value;'

expect_fail noreturn_typedef x86_64 \
  "'noreturn' attribute applies only to functions" \
  'typedef _Noreturn void function_type(void);'

expect_fail atomic_unsupported_compound x86_64 \
  "atomic compound assignment is not supported" \
  '_Atomic int value;
int test(void) { return value *= 2; }'

expect_fail atomic_float_increment x86_64 \
  "atomic increment and decrement require integral or pointer type" \
  '_Atomic float value;
int test(void) { return value++ != 0; }'

expect_fail atomic_pcode pcode \
  "atomic types are unavailable on this target" \
  '#ifndef __STDC_NO_ATOMICS__
#error pcode must advertise unavailable atomics
#endif
_Atomic int value;'

expect_fail atomic_arm_profile arm \
  "atomic types are unavailable on this target" \
  '#ifndef __STDC_NO_ATOMICS__
#error arm must advertise incomplete C11 atomic support
#endif
_Atomic int value;'

expect_compile unsupported_profile_macros pcode \
  '#if __STDC_NO_ATOMICS__ != 1
#error unsupported target must define __STDC_NO_ATOMICS__
#endif
#if __STDC_NO_THREADS__ != 1
#error unsupported target must advertise unavailable threads
#endif
#if __STDC_NO_COMPLEX__ != 1
#error unavailable complex arithmetic must be advertised
#endif
#ifdef __STDC_NO_VLA__
#error VLA support must not be disabled
#endif
int test(void) { return 0; }'

expect_compile optional_macros x86_64 \
  '#ifdef __STDC_NO_ATOMICS__
#error native target must support milestone atomics
#endif
#ifdef __STDC_NO_THREADS__
#error native target must support C11 threads
#endif
#if __STDC_NO_COMPLEX__ != 1
#error unavailable complex arithmetic must be advertised
#endif
#ifdef __STDC_NO_VLA__
#error VLA support must not be disabled
#endif
int test(int n) { int values[n]; return sizeof(values) != 0; }'

expect_compile character_type_macros x86_64 \
  'typedef __CHAR16_TYPE__ compiler_char16_t;
typedef __CHAR32_TYPE__ compiler_char32_t;
typedef __WCHAR_TYPE__ compiler_wchar_t;
_Static_assert(sizeof(compiler_char16_t) == 2, "char16 type width");
_Static_assert(sizeof(compiler_char32_t) == 4, "char32 type width");
_Static_assert(sizeof(compiler_wchar_t) == 4, "wchar type width");
int test(void) { return 0; }'

expect_compile character_type_macros_65c02 65c02 \
  'typedef __CHAR16_TYPE__ compiler_char16_t;
typedef __CHAR32_TYPE__ compiler_char32_t;
typedef __WCHAR_TYPE__ compiler_wchar_t;
_Static_assert(sizeof(compiler_char16_t) == 2, "char16 type width");
_Static_assert(sizeof(compiler_char32_t) == 4, "char32 type width");
_Static_assert(sizeof(compiler_wchar_t) == 2, "wchar type width");
int test(void) { return 0; }'

expect_compile pre_c11_identifiers x86_64 \
  '#ifdef __STDC_NO_COMPLEX__
#error C99 has no optional-complex feature macro
#endif
int _Atomic;
int _Noreturn;
int test(void) { return 0; }' \
  -std=c99

# A variable bound is an expression, so the two declarations below hold different
# expressions for the same parameter type and are still the same declaration.  The
# bound must not be compared as a number: it shares storage with the pointer to
# that expression, which would make the comparison read pointer bits and reject
# these, and would put those bits in the diagnostic.
expect_compile vla_parameter_redeclared x86_64 \
  'int outer(int n, char m[1][n]);
int outer(int n, char m[1][n]) { return n + (m != 0); }
void inner(int n, int m[n][n]);
void inner(int n, int m[n][n]) { (void)m; }
void mixed(int n, int m[3][n]);
void mixed(int n, int m[4][n]) { (void)m; }
int test(void) { return 0; }'

# The element type still has to agree, and a bound that is a constant on both
# sides still has to match.
expect_fail vla_parameter_element_mismatch x86_64 \
  "redeclared with different type" \
  'int f(int n, char m[1][n]);
int f(int n, short m[1][n]) { return 0; }'

expect_fail array_bound_mismatch x86_64 \
  "redeclared with different type" \
  'extern int a[5];
extern int a[6];'

# A variable bound has no number to report, so the diagnostic spells it "[*]"
# rather than printing whatever the bound expression's pointer happens to be.
vla_msg_src="$WORK/vla_bound_spelling.c"
printf '%s\n' 'int f(int n, char m[1][n]);
int f(int n, short m[1][n]) { return 0; }' > "$vla_msg_src"
set +e
"$ROOT/$DAVECC" -target x86_64 -std=c11 -S "$vla_msg_src" \
    -o "$WORK/vla_bound_spelling.s" >"$WORK/vla_bound_spelling.out" 2>&1
set -e
vla_msg="$(<"$WORK/vla_bound_spelling.out")"
if [[ "$vla_msg" != *"char[*]"* ]]; then
  echo "vla_bound_spelling: variable bound not reported as [*]" >&2
  sed 's/^/  /' "$WORK/vla_bound_spelling.out" >&2
  exit 1
fi
if [[ "$vla_msg" =~ char\[[0-9-]+\] ]]; then
  echo "vla_bound_spelling: variable bound reported as a number" >&2
  sed 's/^/  /' "$WORK/vla_bound_spelling.out" >&2
  exit 1
fi

c23_src="$WORK/c23_noreturn.c"
printf '%s\n' '_Noreturn void stop(void) { for (;;) {} }' > "$c23_src"
"$ROOT/$DAVECC" -target x86_64 -std=c23 -Wdeprecated-declarations -S \
    "$c23_src" -o "$WORK/c23_noreturn.s" >"$WORK/c23_noreturn.out" 2>&1
c23_output="$(<"$WORK/c23_noreturn.out")"
if [[ "$c23_output" != *"'_Noreturn' is deprecated in C23"* ]]; then
  echo "c23_noreturn: missing deprecation warning" >&2
  sed 's/^/  /' "$WORK/c23_noreturn.out" >&2
  exit 1
fi
