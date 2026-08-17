#!/bin/bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 <davecc> <interpreter> <libc>" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
LIBC="$ROOT/$3"
INCLUDE_DIR="$ROOT/libc/include"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/c23-numeric-headers.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

compile_source() {
  local name="$1"
  local standard="$2"
  local source="$3"
  local src="$WORK/$name.c"
  printf '%s\n' "$source" >"$src"
  "$DAVECC" -target pcode -std="$standard" -S -isystem "$INCLUDE_DIR" \
      "$src" -o "$WORK/$name.s" >"$WORK/$name.out" 2>&1
}

expect_compile() {
  local name="$1"
  local standard="$2"
  local source="$3"
  if ! compile_source "$name" "$standard" "$source"; then
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
  local standard="$2"
  local pattern="$3"
  local source="$4"
  set +e
  compile_source "$name" "$standard" "$source"
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

expect_compile stdbit_surface c23 \
  '#include <stdbit.h>
static_assert(__STDC_VERSION_STDBIT_H__ == 202311L);
static_assert(__STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_LITTLE__);
static_assert(UINT_WIDTH == sizeof(unsigned int) * CHAR_BIT);
static_assert(_Generic(stdc_bit_floor((unsigned char)3),
                       unsigned char: 1, default: 0));
static_assert(_Generic(stdc_bit_ceil((unsigned long long)3),
                       unsigned long long: 1, default: 0));
static_assert(_Generic(stdc_bit_floor((unsigned _BitInt(32))3),
                       unsigned _BitInt(32): 1, default: 0));
int main(void) { return 0; }'

expect_fail stdbit_requires_c23 c17 "requires C23" \
  '#include <stdbit.h>'
expect_fail stdbit_rejects_signed c23 "No matching association" \
  '#include <stdbit.h>
int main(void) { return (int)stdc_count_ones(1); }'
expect_fail stdbit_rejects_unmatched_bitint c23 "No matching association" \
  '#include <stdbit.h>
int main(void) {
  unsigned _BitInt(33) value = 1;
  return (int)stdc_count_ones(value);
}'

expect_compile stdckdint_surface c23 \
  '#include <stdckdint.h>
static_assert(__STDC_VERSION_STDCKDINT_H__ == 202311L);
int main(void) {
  unsigned int result;
  return ckd_add(&result, 1U, 2U);
}'

expect_fail stdckdint_requires_c23 c17 "requires C23" \
  '#include <stdckdint.h>'
expect_fail stdckdint_rejects_bitint c23 "No matching association" \
  '#include <stdckdint.h>
int main(void) {
  unsigned int result;
  return ckd_add(&result, (unsigned _BitInt(8))1, 2U);
}'
expect_fail stdckdint_rejects_char c23 "No matching association" \
  '#include <stdckdint.h>
int main(void) {
  int result;
  char value = 1;
  return ckd_add(&result, value, 2);
}'
expect_fail stdckdint_rejects_bool c23 "No matching association" \
  '#include <stdckdint.h>
int main(void) {
  int result;
  return ckd_add(&result, true, 2);
}'
expect_fail stdckdint_rejects_enum c23 "No matching association" \
  '#include <stdckdint.h>
enum number { one = 1 };
int main(void) {
  int result;
  enum number value = one;
  return ckd_add(&result, value, 2);
}'

runtime_source="$WORK/c23_stdbit_runtime.c"
cat >"$runtime_source" <<'EOF'
#include <stdbit.h>
#include <stdckdint.h>
#include <limits.h>
#include <stdint.h>

#define U64_MAX (~(uint64_t)0)
#define I64_MAX ((int64_t)(U64_MAX >> 1))
#define I64_MIN (-I64_MAX - 1)

static unsigned int evaluations;

static unsigned int once(unsigned int value) {
  ++evaluations;
  return value;
}

int main(void) {
  if (stdc_leading_zeros_uc(0) != UCHAR_WIDTH) return 1;
  if (stdc_leading_ones_uc(0xf0) != 4) return 2;
  if (stdc_trailing_zeros_ui(0x40U) != 6) return 3;
  if (stdc_trailing_ones_ui(7U) != 3) return 4;
  if (stdc_first_leading_zero_uc(0xff) != 0) return 5;
  if (stdc_first_leading_one_uc(0x10) != 4) return 6;
  if (stdc_first_trailing_zero_uc(0xff) != 0) return 7;
  if (stdc_first_trailing_one_uc(0x10) != 5) return 8;
  if (stdc_count_zeros_us(0) != USHRT_WIDTH) return 9;
  if (stdc_count_ones_ull(0xf0f0ULL) != 8) return 10;
  if (!stdc_has_single_bit(0x80000000U)) return 11;
  if (stdc_has_single_bit(3U)) return 12;
  if (stdc_bit_width(0U) != 0 || stdc_bit_width(9U) != 4) return 13;
  if (stdc_bit_floor(9U) != 8U || stdc_bit_ceil(9U) != 16U) return 14;
  if (stdc_count_ones(once(0xfU)) != 4 || evaluations != 1) return 15;

  unsigned _BitInt(32) precise = 0x80000001U;
  if (stdc_count_ones(precise) != 2) return 16;
  if (stdc_bit_floor(precise) != (unsigned _BitInt(32))0x80000000U) return 17;

  unsigned short us;
  if (ckd_add(&us, 65535U, 1U) != true || us != 0) return 18;
  if (ckd_sub(&us, 1, 2) != true || us != 65535U) return 19;
  if (ckd_mul(&us, 256U, 257U) != true || us != 256U) return 20;

  int si;
  if (ckd_add(&si, INT_MAX, 1) != true || si != INT_MIN) return 21;
  if (ckd_sub(&si, INT_MIN, 1) != true || si != INT_MAX) return 22;
  if (ckd_mul(&si, INT_MAX, 2) != true || si != -2) return 23;

  uint64_t u64;
  if (ckd_add(&u64, U64_MAX, (int64_t)-1) != false ||
      u64 != U64_MAX - 1) return 24;
  if (ckd_mul(&u64, U64_MAX, (uint64_t)2) != true ||
      u64 != U64_MAX - 1) return 25;
  if (ckd_mul(&u64, U64_MAX, U64_MAX) != true || u64 != 1) return 26;

  int64_t i64;
  if (ckd_mul(&i64, I64_MIN, (int64_t)-1) != true ||
      i64 != I64_MIN) return 27;
  if (ckd_add(&i64, I64_MIN, I64_MAX) != false || i64 != -1) return 28;

  int outputs[2] = {0, 0};
  int* output = outputs;
  int left = 1;
  int right = 2;
  if (ckd_add(output++, left++, right++) || output != outputs + 1 ||
      left != 2 || right != 3 || outputs[0] != 3) return 29;

  unsigned _BitInt(5) narrow = 0x10uwb;
  if (__davecc_clz(narrow, 5) != 0 || __davecc_ctz(narrow, 5) != 4 ||
      __davecc_popcount(narrow, 5) != 1) return 30;
  if (__davecc_clz(0x20U, 5) != 5 ||
      __davecc_popcount(0xffU, 5) != 5) return 31;

  signed char small;
  if (ckd_add(&small, (signed char)100, (signed char)27) ||
      small != 127) return 32;
  if (!ckd_add(&small, (signed char)100, (signed char)28) ||
      small != -128) return 33;
  return 0;
}
EOF

for target in pcode x86_64 aarch64 arm riscv 6502; do
  "$DAVECC" -target "$target" -std=c23 -O0 -S -isystem "$INCLUDE_DIR" \
      "$runtime_source" -o "$WORK/c23_stdbit_runtime.$target.s"
done

"$DAVECC" -target x86_64 -std=c23 -O2 -static -isystem "$INCLUDE_DIR" \
    -Wl,-e -Wl,main "$runtime_source" "$LIBC" -o "$WORK/c23_stdbit_runtime.exe"
"$INTERPRETER" -i "$WORK/c23_stdbit_runtime.exe"
