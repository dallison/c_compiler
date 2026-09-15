#!/bin/bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 davecc elfdump" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
ELFDUMP="$ROOT/$2"
WORK="${TEST_TMPDIR:-/tmp}/function-sections-gc"
mkdir -p "$WORK"

cat >"$WORK/gc_functions.s" <<'SRC'
.section ".text._start", "ax", @progbits
.global _start
.type _start, @function
_start:
  jsr used
  rts
.func_end__start:
  .size _start, .func_end__start-_start

.section ".text.used", "ax", @progbits
.global used
.type used, @function
used:
  rts
.func_end_used:
  .size used, .func_end_used-used

.section ".text.unused_fn", "ax", @progbits
.global unused_fn
.type unused_fn, @function
unused_fn:
  rts
.func_end_unused_fn:
  .size unused_fn, .func_end_unused_fn-unused_fn
SRC

"$DAVECC" -target 65c02 -nostdinc -c "$WORK/gc_functions.s" -o "$WORK/gc_functions.o"

"$DAVECC" -target 65c02 -nostdinc -nostdlib --gc-sections -static -e _start \
  "$WORK/gc_functions.o" -o "$WORK/gc_yes.elf"
if "$ELFDUMP" -s "$WORK/gc_yes.elf" | grep -q unused_fn; then
  echo "unused_fn survived --gc-sections" >&2
  "$ELFDUMP" -s "$WORK/gc_yes.elf" >&2
  exit 1
fi
if ! "$ELFDUMP" -s "$WORK/gc_yes.elf" | grep -q used; then
  echo "used was removed by --gc-sections" >&2
  exit 1
fi
if ! "$ELFDUMP" -s "$WORK/gc_yes.elf" | grep -q _start; then
  echo "_start was removed by --gc-sections" >&2
  exit 1
fi

"$DAVECC" -target 65c02 -nostdinc -nostdlib --no-gc-sections -static -e _start \
  "$WORK/gc_functions.o" -o "$WORK/gc_no.elf"
if ! "$ELFDUMP" -s "$WORK/gc_no.elf" | grep -q unused_fn; then
  echo "unused_fn was removed without --gc-sections" >&2
  exit 1
fi

cat >"$WORK/two_funcs.c" <<'SRC'
int unused_helper(int a, int b) {
  return a + b;
}

int used_add(int x) {
  return x + 1;
}

int main(void) {
  return used_add(2);
}
SRC

"$DAVECC" -target 65c02 -ffunction-sections -S -nostdinc \
  "$WORK/two_funcs.c" -o "$WORK/two_funcs.s"
if ! grep -q '.section ".text.used_add"' "$WORK/two_funcs.s"; then
  echo "missing per-function section for used_add" >&2
  cat "$WORK/two_funcs.s" >&2
  exit 1
fi
if ! grep -q '.section ".text.unused_helper"' "$WORK/two_funcs.s"; then
  echo "missing per-function section for unused_helper" >&2
  exit 1
fi

"$DAVECC" -target 65c02 -fno-function-sections -S -nostdinc \
  "$WORK/two_funcs.c" -o "$WORK/two_funcs_shared.s"
if grep -q '.section ".text.used_add"' "$WORK/two_funcs_shared.s"; then
  echo "-fno-function-sections still emitted per-function sections" >&2
  exit 1
fi

"$DAVECC" -target 65c02 -ffunction-sections -c -nostdinc \
  "$WORK/two_funcs.c" -o "$WORK/two_funcs.o"
if ! "$ELFDUMP" -S "$WORK/two_funcs.o" | grep -q '\.text\.used_add'; then
  echo "object missing .text.used_add" >&2
  "$ELFDUMP" -S "$WORK/two_funcs.o" >&2
  exit 1
fi
if ! "$ELFDUMP" -S "$WORK/two_funcs.o" | grep -q '\.text\.unused_helper'; then
  echo "object missing .text.unused_helper" >&2
  exit 1
fi

echo "function-sections gc tests passed"
