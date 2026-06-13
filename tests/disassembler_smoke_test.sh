#!/usr/bin/env bash

set -euo pipefail

davecc=$1
riscvdasm=$2
aarch64dasm=$3
armdasm=$4
x86_64dasm=$5
elfdump=$6

work=${TEST_TMPDIR:-$(mktemp -d)}
mkdir -p "$work"
if [[ -z "${TEST_TMPDIR:-}" ]]; then
  trap 'rm -rf "$work"' EXIT
fi

cat >"$work/smoke.c" <<'EOF'
extern int printf(const char*, ...);

int add(int a, int b) {
  return a + b;
}

const char* message(void) {
  return "hello";
}

int main(void) {
  printf("hello");
  return add(1, 2);
}
EOF

check_target() {
  local target=$1
  local tool=$2
  shift 2
  local obj="$work/$target.o"
  local out="$work/$target.dis"

  "$davecc" -target "$target" -O0 -c "$work/smoke.c" -o "$obj"
  "$tool" "$obj" >"$out"
  "$elfdump" -c "$obj" >"$work/$target.elfdump"
  for expected in "$@"; do
    if ! grep -E "$expected" "$out" >/dev/null; then
      echo "missing expected disassembly for $target: $expected" >&2
      sed -n '1,80p' "$out" >&2
      return 1
    fi
    if ! grep -E "$expected" "$work/$target.elfdump" >/dev/null; then
      echo "missing expected elfdump disassembly for $target: $expected" >&2
      sed -n '1,80p' "$work/$target.elfdump" >&2
      return 1
    fi
  done
}

check_target riscv "$riscvdasm" '^add:' 'add|addi'
check_target aarch64 "$aarch64dasm" '^add:' 'stp' 'adr' 'printf' ' ret$'
check_target arm "$armdasm" '^add:' 'add|bl|bx'
check_target x86_64 "$x86_64dasm" '^add:' 'mov|add|call|ret'
