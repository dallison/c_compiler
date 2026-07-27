#!/bin/bash
set -euo pipefail

DAVECC="$1"
AARCH64="$2"

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/aarch64-inline-asm.XXXXXX")"

run_case() {
  local name="$1"
  local expected="$2"
  local src="$WORK/$name.c"
  local exe="$WORK/$name.exe"
  shift 2
  printf '%s\n' "$@" > "$src"
  "$ROOT/$DAVECC" -target aarch64 -nostdinc -nostdlib -static \
    -Wl,-e -Wl,main \
    "$src" -o "$exe"
  set +e
  "$ROOT/$AARCH64" -i "$exe"
  local status=$?
  set -e
  if [[ "$status" -ne "$expected" ]]; then
    echo "$name: expected exit $expected, got $status" >&2
    exit 1
  fi
}

run_optimized_case() {
  local name="$1"
  local src="$WORK/$name.c"
  local exe="$WORK/$name.exe"
  shift
  printf '%s\n' "$@" > "$src"
  "$ROOT/$DAVECC" -target aarch64 -O2 -nostdinc -nostdlib -static \
    -Wl,-e -Wl,main \
    "$src" -o "$exe"
  "$ROOT/$AARCH64" -i "$exe"
}

run_case basic 0 \
  'int main(void) {' \
  '  int x = 41;' \
  '  int y;' \
  '  __asm__ volatile ("add %w[out], %w[in], #1" : [out] "=r" (y) : [in] "r" (x) : "cc");' \
  '  return y - 42;' \
  '}'

run_case readwrite_goto_decl 0 \
  'int renamed_value(void) asm("renamed_value_impl");' \
  'int renamed_value(void) { return 5; }' \
  'int main(void) {' \
  '  int x = renamed_value();' \
  '  __asm__ volatile ("add %w0, %w0, %1" : "+r" (x) : "i" (37) : "cc", "memory");' \
  '  __asm__ goto ("b %l0" ::: "memory" : done);' \
  '  return 99;' \
  'done:' \
  '  return x - 42;' \
  '}'

# A shift/add chain must continue shifting the original multiplicand. Register
# coalescing previously changed x * 15 into x * 63.
run_optimized_case multiply_by_15 \
  'int main(void) {' \
  '  volatile int x = 7;' \
  '  return x * 15 == 105 ? 0 : 1;' \
  '}'

bad="$WORK/bad.c"
printf '%s\n' \
  'int main(void) {' \
  '  int x = 0;' \
  '  __asm__ volatile ("nop" : "=q" (x));' \
  '  return x;' \
  '}' > "$bad"
if "$ROOT/$DAVECC" -target aarch64 -nostdinc -c "$bad" \
    -o "$WORK/bad.o" >"$WORK/bad.out" 2>&1; then
  echo "bad: expected unsupported constraint failure" >&2
  exit 1
fi
bad_output="$(<"$WORK/bad.out")"
if [[ "$bad_output" != *"Unsupported aarch64 asm output constraint"* ]]; then
  echo "bad: missing unsupported constraint diagnostic" >&2
  exit 1
fi
