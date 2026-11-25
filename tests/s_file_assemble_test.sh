#!/bin/bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 davecc" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
WORK="${TEST_TMPDIR:-/tmp}/s-file-assemble"
mkdir -p "$WORK"

cat >"$WORK/align.S" <<'SRC'
#if defined(__APPLE__)
.section __TEXT,__text,regular,pure_instructions
#define NAME _align_mask
#else
.text
#define NAME align_mask
#endif
.global NAME
.p2align 2
NAME:
  bic x0, x0, #15
  add x1, x1, #(32 + 16)
  cbz x0, 2f
1:
  adr x1, 1b
  stp q8, q9, [x0, #(160 + 32)]
  ldp q8, q9, [x0, #(160 + 32)]
  mrs x2, fpsr
  msr fpcr, x2
  mrs x3, nzcv
  msr nzcv, x3
  mrs x4, tpidr_el0
  msr tpidr_el0, x4
  mrs x5, s3_3_c13_c0_2
  msr s3_3_c13_c0_2, x5
  mov fp, sp
  mov lr, x30
  ret
2:
  ret
SRC

"$DAVECC" -nostdinc -c "$WORK/align.S" -o "$WORK/align.o"
test -s "$WORK/align.o"

cat >"$WORK/arm_special.S" <<'SRC'
.text
.global arm_special
.p2align 2
arm_special:
  mrs r0, cpsr
  mrs r1, apsr
  mrs r2, spsr
  msr cpsr_f, r0
  msr apsr_nzcvq, r1
  vmrs apsr_nzcv, fpscr
  vmrs r3, fpscr
  vmsr fpscr, r3
  bx lr
SRC

"$DAVECC" -nostdinc -target arm -c "$WORK/arm_special.S" -o "$WORK/arm_special.o"
test -s "$WORK/arm_special.o"
echo ok
