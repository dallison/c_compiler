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
  ret
2:
  ret
SRC

"$DAVECC" -c "$WORK/align.S" -o "$WORK/align.o"
test -s "$WORK/align.o"
echo ok
