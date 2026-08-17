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
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/c23-type-system.XXXXXX")"
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

expect_compile c17_identifiers c17 \
  'int typeof = 1;
int typeof_unqual = 2;
int nullptr = 3;
int constexpr = 4;
int main(void) { return typeof + typeof_unqual + nullptr + constexpr - 10; }'

expect_compile typeof_forms c23 \
  'const int ci = 1;
int array[3];
int function(int x) { return x; }
typeof(ci) preserved;
typeof_unqual(ci) unqualified;
typeof(int *) pointer;
typeof(array) another_array;
typeof(function) *function_pointer = function;
static_assert(sizeof(another_array) == sizeof(array));
static_assert(_Generic(pointer, int *: 1, default: 0));
int main(void) { unqualified = preserved; return function_pointer(0); }'
expect_compile typeof_nested_uses c23 \
  'int value;
static_assert(_Generic((typeof(value))1, int: 1, default: 0));
static_assert(sizeof(typeof_unqual(const int)) == sizeof(int));
int main(void) {
  typeof((int){1}) local = 2;
  return _Generic(local, int: 0, default: 1);
}'
expect_fail typeof_bit_field c23 "bit-field" \
  'struct S { unsigned bit : 1; }; int main(void) { struct S s; typeof(s.bit) x; return 0; }'

expect_compile auto_inference c23 \
  'int array[2];
int function(void) { return 7; }
auto file_value = 3;
static auto file_static = 4;
int main(void) {
  const int source = 5;
  auto scalar = source;
  auto pointer = array;
  auto callable = function;
  const auto qualified = 6;
  static_assert(_Generic(scalar, int: 1, default: 0));
  static_assert(_Generic(pointer, int *: 1, default: 0));
  static_assert(_Generic(callable, int (*)(void): 1, default: 0));
  return file_value + file_static + scalar + qualified + callable() - 25;
}'
expect_fail auto_no_initializer c23 "requires an initializer" \
  'int main(void) { auto value; return 0; }'
expect_fail auto_pointer_declarator c23 "simple object declarator" \
  'int main(void) { int value; auto *pointer = &value; return 0; }'
expect_fail auto_array_declarator c23 "simple object declarator" \
  'int main(void) { auto values[2] = 1; return 0; }'
expect_fail auto_braced_initializer c23 "assignment-expression" \
  'int main(void) { auto value = { 1 }; return 0; }'
expect_fail auto_multiple_declarators c23 "one declarator" \
  'int main(void) { auto first = 1, second = 2; return 0; }'
expect_fail auto_redeclaration c23 "cannot redeclare" \
  'int main(void) { int value; auto value = 1; return 0; }'
expect_fail auto_self_reference c23 "cannot refer to itself" \
  'int main(void) { auto value = value; return 0; }'

expect_compile nullptr_semantics c23 \
  '#include <stddef.h>
static_assert(__STDC_VERSION_STDDEF_H__ == 202311L);
static_assert(_Generic(nullptr, nullptr_t: 1, default: 0));
static_assert(nullptr == nullptr);
int main(void) {
  int *pointer = nullptr;
  bool value = nullptr;
  if (nullptr) return 1;
  return pointer != nullptr || value;
}'
expect_fail nullptr_to_int c23 "cannot convert" \
  'int main(void) { int value = nullptr; return value; }'
expect_fail nullptr_integer_cast c23 "Illegal cast" \
  'int main(void) { return (int)nullptr; }'
expect_fail nullptr_not_integer_constant c23 "constant" \
  'static_assert(nullptr);'

expect_compile constexpr_objects c23 \
  'struct Pair { int x; int y; };
struct Holder { int value; int *pointer; };
constexpr int answer = 42;
constexpr float exact = 0.5;
constexpr struct Pair pair = { 2, 3 };
constexpr struct Holder holder = { .pointer = nullptr };
constexpr int *nil = nullptr;
static_assert(answer == 42);
static_assert(pair.x == 2);
int main(void) {
  constexpr int local = answer;
  return local + pair.y - 45 + (exact != 0.5) +
         (nil != nullptr) + (holder.pointer != nullptr);
}'
expect_fail constexpr_function c23 "constexpr functions" \
  'constexpr int function(void) { return 1; }'
expect_fail constexpr_no_initializer c23 "requires an initializer" \
  'constexpr int value;'
expect_fail constexpr_nonconstant c23 "constant expression" \
  'int function(void); constexpr int value = function();'
expect_fail ordinary_const_not_c23_constant c23 "constant" \
  'const int value = 1; static_assert(value == 1);'
expect_fail constexpr_not_representable c23 "exactly representable" \
  'constexpr unsigned char value = 128 + 128;'
expect_fail constexpr_float_not_representable c23 "exactly representable" \
  'constexpr float value = 0.1;'
expect_fail constexpr_volatile c23 "C constexpr object cannot" \
  'constexpr volatile int value = 1;'
expect_fail constexpr_atomic c23 "C constexpr object cannot" \
  'constexpr _Atomic(int) value = 1;'
expect_fail constexpr_nonnull_pointer c23 "initialized to null" \
  'int target; constexpr int *pointer = &target;'
expect_fail constexpr_nonnull_pointer_member c23 "initialized to null" \
  'int target; struct Holder { int *pointer; }; constexpr struct Holder value = { &target };'

runtime_source="$WORK/c23_type_runtime.c"
cat >"$runtime_source" <<'EOF'
#include <stddef.h>
#include <stdarg.h>

constexpr int base = 7;
constexpr struct Pair { int x; int y; } pair = { 2, 3 };
auto inferred_file = 5;

static int receives_null(int count, ...) {
  va_list ap;
  va_start(ap, count);
  nullptr_t value = va_arg(ap, nullptr_t);
  va_end(ap);
  return value == nullptr;
}

int main(void) {
  int array[2] = { 4, 6 };
  int bound = 1;
  auto pointer = array;
  typeof(int[++bound]) vla;
  typeof_unqual(const int) sum = pointer[0] + pointer[1];
  constexpr int local = base + 1;
  if (!receives_null(1, nullptr)) return 1;
  if (bound != 2) return 5;
  if (sizeof(vla) != 2 * sizeof(int)) return 6;
  if (sum != 10 || local != 8 || pair.x + pair.y != inferred_file) return 3;
  return nullptr ? 4 : 0;
}
EOF

"$DAVECC" -target x86_64 -std=c23 -O2 -static -isystem "$INCLUDE_DIR" \
    -Wl,-e -Wl,main "$runtime_source" "$LIBC" -o "$WORK/c23_type_runtime.exe"
"$INTERPRETER" -i "$WORK/c23_type_runtime.exe"
