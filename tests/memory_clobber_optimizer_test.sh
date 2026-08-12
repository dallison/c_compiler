#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 davecc source" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
SOURCE="$ROOT/$2"
WORK="${TEST_TMPDIR:-/tmp}/memory-clobber-optimizer"
mkdir -p "$WORK"

for target in 6502 65c02 riscv aarch64 arm x86_64 pcode; do
  "$DAVECC" -target "$target" -O2 -nostdinc -nostdlib -S \
    "$SOURCE" -o "$WORK/$target.s"
done

DUMP_SOURCE="$WORK/memory_clobber_optimizer_cases.c"
cp "$SOURCE" "$DUMP_SOURCE"
"$DAVECC" -target x86_64 -O2 -nostdinc -nostdlib -Xsave-ir -S \
  "$DUMP_SOURCE" -o "$WORK/memory_clobber_optimizer.s"
IR_FILE="${DUMP_SOURCE%.c}.ir"
if [[ ! -f "$IR_FILE" ]]; then
  echo "memory-clobber optimizer IR dump was not produced" >&2
  exit 1
fi

loop_body=$(
  awk '/IR for function memory_clobber_loop/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if ! grep -Eq '[[:space:]]asm\(.*\{asmmemoryclobber\}' <<<"$loop_body" ||
   ! awk '
      /^  Loop nesting: / {nesting=$3}
      /^\$/ && /load32\(.*REF barrier_value/ && nesting ~ /^[1-9]/ {safe=1}
      END {exit safe ? 0 : 1}
    ' <<<"$loop_body"; then
  echo "inline asm memory clobber did not retain the loop memory load" >&2
  exit 1
fi

reload_body=$(
  awk '/IR for function memory_clobber_reload/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if ! grep -Eq '[[:space:]]asm\(.*\{asmmemoryclobber\}' <<<"$reload_body" ||
   ! grep -Eq '[[:space:]]load32\(.*REF value' <<<"$reload_body"; then
  echo "constant propagation reused memory across an asm memory clobber" >&2
  exit 1
fi
