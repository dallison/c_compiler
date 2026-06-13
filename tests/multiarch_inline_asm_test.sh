#!/bin/bash
set -euo pipefail

DAVECC="$1"
X86_64="$2"
ARM="$3"
RISCV="$4"

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/multiarch-inline-asm.XXXXXX")"

run_exec_case() {
  local target="$1"
  local runner="$2"
  local name="$3"
  local expected="$4"
  shift 4
  local src="$WORK/$name.c"
  local exe="$WORK/$name.exe"
  printf '%s\n' "$@" > "$src"
  "$ROOT/$DAVECC" -target "$target" -static -Wl,-e -Wl,main "$src" -o "$exe"
  set +e
  "$ROOT/$runner" "$exe"
  local status=$?
  set -e
  if [[ "$status" -ne "$expected" ]]; then
    echo "$name: expected exit $expected, got $status" >&2
    exit 1
  fi
}

run_exec_case x86_64 "$X86_64" x86_64_inline 0 \
  'int renamed_value(void) asm("renamed_value_impl");' \
  'int renamed_value(void) { return 5; }' \
  'int main(void) {' \
  '  long y;' \
  '  __asm__ volatile ("movq %1, %0" : "=r" (y) : "r" ((long)renamed_value()));' \
  '  __asm__ volatile ("addq %1, %0" : "+r" (y) : "i" (37) : "cc", "memory");' \
  '  __asm__ goto ("jmp %l0" ::: "memory" : done);' \
  '  return 99;' \
  'done:' \
  '  return y - 42;' \
  '}'

run_exec_case arm "$ARM" arm_inline 0 \
  'int main(void) {' \
  '  int y;' \
  '  __asm__ volatile ("mov %0, %1" : "=r" (y) : "r" (5));' \
  '  __asm__ volatile ("add %0, %0, %1" : "+r" (y) : "i" (37) : "cc", "memory");' \
  '  __asm__ goto ("b %l0" ::: "memory" : done);' \
  '  return 99;' \
  'done:' \
  '  return y - 42;' \
  '}'

run_exec_case riscv "$RISCV" riscv_inline 0 \
  'int main(void) {' \
  '  long y;' \
  '  __asm__ volatile ("mv %0, %1" : "=r" (y) : "r" (5L));' \
  '  __asm__ volatile ("addi %0, %0, %1" : "+r" (y) : "i" (37) : "memory");' \
  '  __asm__ goto ("j %l0" ::: "memory" : done);' \
  '  return 99;' \
  'done:' \
  '  return y - 42;' \
  '}'

c02="$WORK/6502.c"
c02_s="$WORK/6502.s"
printf '%s\n' \
  'int main(void) {' \
  '  int x = 5;' \
  '  int y;' \
  '  __asm__ volatile ("lda %1\nsta %0" : "=r" (y) : "r" (x));' \
  '  __asm__ volatile ("lda #%0" :: "i" (7));' \
  '  return 0;' \
  '}' > "$c02"
"$ROOT/$DAVECC" -target 6502 -S -O0 "$c02" -o "$c02_s"
c02_asm="$(<"$c02_s")"
if [[ "$c02_asm" != *"lda #7"* ]]; then
  echo "6502: immediate operand was not substituted" >&2
  exit 1
fi
if [[ "$c02_asm" == *"%0"* || "$c02_asm" == *"%1"* || "$c02_asm" != *"sta __"* ]]; then
  echo "6502: register operands were not substituted" >&2
  exit 1
fi
"$ROOT/$DAVECC" -target 6502 -c "$c02" -o "$WORK/6502.o"

bad="$WORK/6502_bad.c"
printf '%s\n' \
  'int main(void) {' \
  '  int x = 0;' \
  '  __asm__ volatile ("nop" : "=q" (x));' \
  '  return x;' \
  '}' > "$bad"
if "$ROOT/$DAVECC" -target 6502 -c "$bad" -o "$WORK/6502_bad.o" >"$WORK/6502_bad.out" 2>&1; then
  echo "6502: expected unsupported output constraint failure" >&2
  exit 1
fi
bad_output="$(<"$WORK/6502_bad.out")"
if [[ "$bad_output" != *"Unsupported 6502 asm output constraint"* ]]; then
  echo "6502: missing unsupported constraint diagnostic" >&2
  exit 1
fi
