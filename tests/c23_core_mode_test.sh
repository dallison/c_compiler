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
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/c23-core-mode.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

compile_source() {
  local name="$1"
  local standard="$2"
  local source="$3"
  local src="$WORK/$name.c"
  printf '%s\n' "$source" > "$src"
  "$DAVECC" -target pcode -std="$standard" -S "$src" \
      -o "$WORK/$name.s" >"$WORK/$name.out" 2>&1
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

expect_compile fixed_enums c23 \
  'enum Small : unsigned char { SMALL_ZERO, SMALL_MAX = 255 };
enum Signed : signed char { SIGNED_NEGATIVE = -1 };
enum Wide : long long { WIDE_VALUE = 4294967296LL };
typedef unsigned short U16;
enum Forward : U16;
enum Forward : U16 { FORWARD_VALUE = 65535 };
static_assert(sizeof(enum Small) == sizeof(unsigned char));
static_assert(sizeof(enum Signed) == sizeof(signed char));
static_assert(sizeof(enum Forward) == sizeof(unsigned short));
static_assert(SIGNED_NEGATIVE < 0);
static_assert(WIDE_VALUE == 4294967296LL);
static_assert(_Generic(SMALL_MAX, enum Small: 1, default: 0));
int main(void) { return 0; }'
expect_fail fixed_enum_c17 c17 "Expected" \
  'enum E : int { E0 };'
expect_fail fixed_enum_overflow c23 "not representable" \
  'enum E : unsigned char { E0 = 256 };'
expect_fail fixed_enum_implicit_overflow c23 "not representable" \
  'enum E : unsigned char { E0 = 255, E1 };'
expect_fail fixed_enum_mismatch c23 "different underlying type" \
  'enum E : int; enum E : short { E0 };'
expect_fail fixed_enum_added_late c23 "cannot be redeclared" \
  'enum E; enum E : int { E0 };'
expect_fail fixed_enum_bool c23 "must be integral" \
  'enum E : bool { E0 };'

expect_compile empty_initializers c23 \
  'struct Pair { int x; int y; };
union Value { int x; long y; };
int global_scalar = {};
int global_array[2] = {};
struct Pair global_pair = {};
union Value global_value = {};
int main(void) {
  int scalar = {};
  int array[2] = {};
  struct Pair pair = {};
  union Value value = {};
  return scalar + array[0] + pair.x + value.x + (int){};
}'
expect_fail empty_initializer_c17 c17 "empty initializer requires C23" \
  'int main(void) { int value = {}; return value; }'
expect_fail empty_aggregate_initializer_c17 c17 "empty initializer requires C23" \
  'struct Pair { int x; int y; };
int main(void) { struct Pair value = {}; return value.x; }'

expect_fail implicit_int_c23 c23 "type specifier missing" \
  'extern value; int main(void) { return 0; }'
expect_fail implicit_function_c23 c23 "Calling undeclared function" \
  'int main(void) { return missing(1); }'
expect_fail knr_definition_c23 c23 "identifier-list function declarators" \
  'int add(a, b) int a; int b; { return a + b; }'

expect_compile empty_function_prototype c23 \
  'int no_args();
int no_args() { return 0; }
int main(void) { return no_args(); }'
expect_fail empty_function_call_with_arg c23 "argument" \
  'int no_args();
int no_args() { return 0; }
int main(void) { return no_args(1); }'
expect_compile leading_ellipsis_c23 c23 \
  'int variadic(...);
int main(void) { return 0; }'
expect_fail leading_ellipsis_c17 c17 "requires C23" \
  'int variadic(...);'

expect_compile c23_labels c23 \
  'int test(int x) {
  goto declaration;
declaration:
  int value = x;
  return value;
}
void trailing(void) {
end:
}
int choose(int x) {
  switch (x) {
    case 0:
      int value = 3;
      return value;
    default:
  }
  return 0;
}
int main(void) { return test(0) + choose(1); }'
expect_fail trailing_label_c17 c17 "requires C23" \
  'void trailing(void) { end: }'

runtime_source="$WORK/c23_runtime.c"
cat >"$runtime_source" <<'EOF'
#define ADD(base, ...) base __VA_OPT__(+ (__VA_ARGS__))
enum Byte : unsigned char { BYTE_VALUE = 255 };
struct Pair { int x; int y; };

static int no_args() {
  return 0;
}

static void trailing_label(void) {
done:
}

int main(void) {
  int scalar = {};
  int array[3] = {};
  struct Pair pair = {};
  enum Byte value = BYTE_VALUE;
  const unsigned char *text = u8"ok";
  trailing_label();
  if (scalar != 0 || array[2] != 0 || pair.x != 0 || pair.y != 0) return 1;
  if (sizeof(enum Byte) != 1 || value != 255) return 2;
  if (text[0] != 'o' || text[1] != 'k') return 3;
  if (ADD(4) != 4 || ADD(4, 5) != 9 || no_args() != 0) return 4;
  goto declaration;
declaration:
  int after_label = {};
  return after_label;
}
EOF

"$DAVECC" -target x86_64 -std=c23 -O2 -static -isystem "$INCLUDE_DIR" \
    -Wl,-e -Wl,main "$runtime_source" "$LIBC" -o "$WORK/c23_runtime.exe"
"$INTERPRETER" -i "$WORK/c23_runtime.exe"
