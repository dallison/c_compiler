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
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/c23-bitint.XXXXXX")"
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

expect_compile c17_identifier c17 \
  'int _BitInt = 3; int main(void) { return _BitInt - 3; }'

expect_compile declarations c23 \
  '#include <limits.h>
signed _BitInt(2) s2;
unsigned _BitInt(1) u1;
_BitInt(7) s7;
unsigned _BitInt(33) u33;
static_assert(BITINT_MAXWIDTH == 64);
static_assert(sizeof(s2) == 1);
static_assert(sizeof(s7) == 1);
static_assert(sizeof(u33) == 8);
static_assert(_Generic(s7, _BitInt(7): 1, default: 0));
static_assert(_Generic(u33, unsigned _BitInt(33): 1, default: 0));
int main(void) { return 0; }'

expect_fail signed_width_one c23 "at least 2" \
  '_BitInt(1) value;'
expect_fail zero_width c23 "between 1 and BITINT_MAXWIDTH" \
  '_BitInt(0) value;'
expect_fail excessive_width c23 "between 1 and BITINT_MAXWIDTH" \
  '_BitInt(65) value;'
expect_fail nonconstant_width c23 "integer constant expression" \
  'int n; _BitInt(n) value;'
expect_fail invalid_specifier_combination c23 "Invalid type combination" \
  'long _BitInt(8) value;'
expect_fail constexpr_not_representable c23 "exactly representable" \
  'constexpr _BitInt(5) value = 16;'

expect_compile literal_types c23 \
  '#if 3uwb != 3
#error wb suffix failed in preprocessor expression
#endif
static_assert(_Generic(0wb, _BitInt(2): 1, default: 0));
static_assert(_Generic(1wb, _BitInt(2): 1, default: 0));
static_assert(_Generic(3wb, _BitInt(3): 1, default: 0));
static_assert(_Generic(0uwb, unsigned _BitInt(1): 1, default: 0));
static_assert(_Generic(3Uwb, unsigned _BitInt(2): 1, default: 0));
static_assert(_Generic(7uWB, unsigned _BitInt(3): 1, default: 0));
static_assert((_BitInt(5))16 == -16);
int main(void) { return 0; }'

expect_compile standard_type_rank c23 \
  'static_assert(_Generic((_BitInt(7))1 + 1, int: 1, default: 0) == 1);'
expect_compile wider_bitint_rank c23 \
  'static_assert(_Generic((_BitInt(33))1 + 1,
                          _BitInt(33): 1, default: 0) == 1);'
expect_compile no_integer_promotion c23 \
  'static_assert(_Generic((_BitInt(7))1 << 1,
                          _BitInt(7): 1, default: 0) == 1);'
expect_compile signed_unsigned_rank c23 \
  'static_assert(_Generic((unsigned _BitInt(9))1 + (_BitInt(8))1,
                          unsigned _BitInt(9): 1, default: 0) == 1);'
expect_compile enum_underlying c23 \
  'enum E : _BitInt(4) { E0 = -8, E1 = 7 };
static_assert(sizeof(enum E) == sizeof(_BitInt(4)));
int main(void) { return 0; }'

expect_fail enum_overflow c23 "not representable" \
  'enum E : _BitInt(4) { E0 = 8 };'

runtime_source="$WORK/c23_bitint_runtime.c"
cat >"$runtime_source" <<'EOF'
struct Bits {
  unsigned _BitInt(5) low : 5;
  _BitInt(6) signed_value : 6;
};

static _BitInt(13) add13(_BitInt(13) a, _BitInt(13) b) {
  return a + b;
}

int main(void) {
  _BitInt(5) signed_value = 15;
  unsigned _BitInt(5) unsigned_value = 31;
  signed_value += 1;
  unsigned_value += 1;
  if (signed_value != -16 || unsigned_value != 0) return 1;

  _BitInt(3) signed_literal_sum = 3wb + 1wb;
  unsigned _BitInt(3) unsigned_literal_sum = 7uwb + 1uwb;
  if (signed_literal_sum != -4 || unsigned_literal_sum != 0) return 2;

  _BitInt(13) large = add13(4095, 1);
  if (large != -4096) return 3;

  _BitInt(32) same_size = -123;
  int ordinary = same_size;
  same_size = ordinary;
  if (same_size != -123 || ordinary != -123) return 5;

  _BitInt(7) floating_source = 42;
  double floating = floating_source;
  _BitInt(7) floating_result = floating;
  if (floating_result != 42) return 6;

  struct Bits bits = { .low = 31, .signed_value = -17 };
  if (bits.low != 31 || bits.signed_value != -17) return 4;
  return 0;
}
EOF

for target in pcode x86_64 aarch64 arm riscv 6502; do
  "$DAVECC" -target "$target" -std=c23 -O0 -S -isystem "$INCLUDE_DIR" \
      "$runtime_source" -o "$WORK/c23_bitint_runtime.$target.s"
done

"$DAVECC" -target x86_64 -std=c23 -O2 -static -isystem "$INCLUDE_DIR" \
    -Wl,-e -Wl,main "$runtime_source" "$LIBC" -o "$WORK/c23_bitint_runtime.exe"
"$INTERPRETER" -i "$WORK/c23_bitint_runtime.exe"
