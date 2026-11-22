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

# Direct object emission (-c) shortens per-function local label stems to L0,
# L1, ... while -S keeps readable function names; rewrite those local stems
# before assembling so the resulting ELF objects remain byte-for-byte identical.
assemble_reference() {
  awk '
    /^\t\.type[ \t]+/ && $0 ~ /@function/ {
      fn = $2; sub(/,$/, "", fn); stem[fn] = "L" n++;
    }
    {
      for (f in stem) {
        gsub("\\.Leh_" f "_", ".Leh_" stem[f] "_")
        gsub("\\." f "_label_", "." stem[f] "_label_")
      }
      print
    }
  ' "$1" > "$1.stemmed"
  "$aarch64asm" "$1.stemmed" -o "$2"
}

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
assemble_reference "$work/reference.s" "$work/reference.o"

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
assemble_reference "$work/inline_reference.s" "$work/inline_reference.o"

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
assemble_reference "$work/data_reference.s" "$work/data_reference.o"

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
assemble_reference "$work/long_symbol_reference.s" \
  "$work/long_symbol_reference.o"

cmp "$work/long_symbol_reference.o" "$work/long_symbol_direct.o"

cat >"$work/O2.cpp" <<'EOF'
__attribute__((noinline)) int sum(int a, int b) { return a + b; }
__attribute__((noinline)) int odd_frame(int a, int b, int c) {
  int x = sum(a, b);
  int y = sum(b, c);
  return x + y + c;
}
int main() { return odd_frame(3, 4, 5) - 21; }
EOF

"$davecc" -target aarch64 -std=c++20 -O2 -nostdinc -c "$work/O2.cpp" \
  -o "$work/O2_direct.o"
"$davecc" -target aarch64 -std=c++20 -O2 -nostdinc -S "$work/O2.cpp" \
  -o "$work/O2_reference.s"
assemble_reference "$work/O2_reference.s" "$work/O2_reference.o"

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

# `adr` to a GNU local label encodes the distance from the instruction, as it
# does for a named label (a coroutine trampoline's return stub is `adr x1, 1b`).
cat >"$work/local_label_adr.S" <<'EOF'
  .text
  .p2align 2
1:
  mov x0, #42
  ret
  .global label_backward
label_backward:
  adr x0, 1b
  ret
  .global label_forward
label_forward:
  adr x0, 2f
  ret
2:
  mov x0, #7
  ret
EOF

cat >"$work/local_label_adr.c" <<'EOF'
typedef long (*Thunk)(void);
Thunk label_backward(void);
Thunk label_forward(void);

int main(void) {
  return label_backward()() == 42 && label_forward()() == 7 ? 0 : 1;
}
EOF

"$davecc" -target aarch64 -O0 -nostdinc -nostdlib -static -Wl,-e -Wl,main \
  "$work/local_label_adr.c" "$work/local_label_adr.S" \
  -o "$work/local_label_adr.bin"
"$aarch64" -i "$work/local_label_adr.bin"
