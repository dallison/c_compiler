#!/bin/sh
set -eu

root="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
davecc="$root/$1"
aarch64asm="$root/$2"
aarch64="$root/$3"
work="$(mktemp -d "${TMPDIR:-/tmp}/aarch64-direct-object.XXXXXX")"
if [ -z "${DAVECC_KEEP_TEST_WORK:-}" ]; then
  trap 'rm -rf "$work"' EXIT
else
  echo "test work directory: $work"
fi

cat >"$work/input.cpp" <<'EOF'
template <int N>
__attribute__((noinline)) int value() {
  return N;
}

int main() {
  return value<0>() + value<1>() + value<65535>();
}
EOF

"$davecc" -target aarch64 -std=c++20 -O0 -nostdinc -c "$work/input.cpp" \
  -o "$work/direct.o"
"$davecc" -target aarch64 -std=c++20 -O0 -nostdinc -S "$work/input.cpp" \
  -o "$work/reference.s"
"$aarch64asm" "$work/reference.s" -o "$work/reference.o"

grep -q '// \*\*\* Basic block' "$work/reference.s"
cmp "$work/reference.o" "$work/direct.o"

cat >"$work/inline_asm.cpp" <<'EOF'
int add_one(int x) {
  int result;
  __asm__("add %0, %1, #1" : "=r"(result) : "r"(x));
  return result;
}

int main() {
  return add_one(41);
}
EOF

"$davecc" -target aarch64 -std=c++20 -O0 -nostdinc -c "$work/inline_asm.cpp" \
  -o "$work/inline_direct.o"
"$davecc" -target aarch64 -std=c++20 -O0 -nostdinc -S "$work/inline_asm.cpp" \
  -o "$work/inline_reference.s"
"$aarch64asm" "$work/inline_reference.s" -o "$work/inline_reference.o"

cmp "$work/inline_reference.o" "$work/inline_direct.o"

cat >"$work/data.cpp" <<'EOF'
const int kValue = 42;
const char* kText = "hello";
extern int external;
int* external_pointer = &external;
int main() {
  return kValue + kText[0];
}
EOF

"$davecc" -target aarch64 -std=c++20 -O0 -nostdinc -c "$work/data.cpp" \
  -o "$work/data_direct.o"
"$davecc" -target aarch64 -std=c++20 -O0 -nostdinc -S "$work/data.cpp" \
  -o "$work/data_reference.s"
"$aarch64asm" "$work/data_reference.s" -o "$work/data_reference.o"

cmp "$work/data_reference.o" "$work/data_direct.o"

long_name="long_symbol_"
i=0
while [ "$i" -lt 600 ]; do
  long_name="${long_name}x"
  i=$((i + 1))
done
{
  printf 'int %s(int value) {\n' "$long_name"
  printf '  return value ? value + 1 : value - 1;\n'
  printf '}\n'
  printf 'int main() { return %s(41) == 42 ? 0 : 1; }\n' "$long_name"
} >"$work/long_symbol.cpp"

"$davecc" -target aarch64 -std=c++20 -O0 -nostdinc -c \
  "$work/long_symbol.cpp" -o "$work/long_symbol_direct.o"
"$davecc" -target aarch64 -std=c++20 -O0 -nostdinc -S \
  "$work/long_symbol.cpp" -o "$work/long_symbol_reference.s"
"$aarch64asm" "$work/long_symbol_reference.s" \
  -o "$work/long_symbol_reference.o"

cmp "$work/long_symbol_reference.o" "$work/long_symbol_direct.o"

cat >"$work/O2.cpp" <<'EOF'
int sum(int a, int b) { return a + b; }
int main() { return sum(3, 4); }
EOF

"$davecc" -target aarch64 -std=c++20 -O2 -nostdinc -c "$work/O2.cpp" \
  -o "$work/O2_direct.o"
"$davecc" -target aarch64 -std=c++20 -O2 -nostdinc -S "$work/O2.cpp" \
  -o "$work/O2_reference.s"
"$aarch64asm" "$work/O2_reference.s" -o "$work/O2_reference.o"

cmp "$work/O2_reference.o" "$work/O2_direct.o"

cat >"$work/observable_store.cpp" <<'EOF'
struct Box {
  int value;
  __attribute__((noinline)) explicit Box(int input) : value(input) {}
};

volatile int input = 42;

int main() {
  Box box(input);
  return box.value - 42;
}
EOF

"$davecc" -target aarch64 -std=c++20 -O2 -nostdinc -nostdlib -static \
  -Wl,-e -Wl,main "$work/observable_store.cpp" \
  -o "$work/observable_store.bin"
"$aarch64" -i "$work/observable_store.bin"

cat >"$work/shifted_switch.cpp" <<'EOF'
volatile int selector = 11;

int selected_value() {
  switch (selector) {
    case 0: return 17;
    case 1: return 19;
    case 2: return 23;
    case 3: return 29;
    case 4: return 31;
    case 5: return 37;
    case 6: return 41;
    case 7: return 43;
    case 8: return 47;
    case 9: return 53;
    case 10: return 59;
    case 11: return 61;
    case 12: return 67;
    default: return 71;
  }
}

int main() {
  return selected_value() == 61 ? 0 : 1;
}
EOF

"$davecc" -target aarch64 -std=c++20 -O0 -nostdinc -nostdlib -static \
  -Wl,-e -Wl,main "$work/shifted_switch.cpp" \
  -o "$work/shifted_switch.bin"
"$aarch64" -i "$work/shifted_switch.bin"
