#!/usr/bin/env bash
# .equ / .set names and macros that expand to #NAME must assemble as immediates.
set -euo pipefail

AARCH64ASM=${1:?aarch64asm}
AARCH64DASM=${2:?aarch64dasm}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

cat >"$tmp/equ.S" <<'EOF'
  .text
  .equ TF_SIZE, 16
  .set OFF, 8
  .macro SAVE_FRAME
  sub sp, sp, #TF_SIZE
  stp x0, x1, [sp, #0]
  ldr x0, [sp, #OFF]
  .endm
  .global bar
bar:
  SAVE_FRAME
  sub sp, sp, TF_SIZE
  ret
EOF

"$AARCH64ASM" -o "$tmp/equ.o" "$tmp/equ.S"
out=$("$AARCH64DASM" "$tmp/equ.o")
echo "$out" | grep -q 'sub sp, sp, #16' || {
  echo "expected sub sp, sp, #16 in:" >&2
  echo "$out" >&2
  exit 1
}
echo ok
