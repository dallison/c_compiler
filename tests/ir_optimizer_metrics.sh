#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 davecc [output.csv]" >&2
  exit 2
fi

DAVECC=$1
OUTPUT=${2:-/dev/stdout}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=$(mktemp -d "${TMPDIR:-/tmp}/ir_optimizer_metrics.XXXXXX")
trap 'rm -rf "$WORK"' EXIT
cp "$ROOT/tests/ir_optimizer_cases.c" "$WORK/cases.c"

printf 'target,opt,ir_nodes,target_instructions,loop_instructions,induction_address_instructions,sccp_ir_nodes,sccp_instructions,alias_instructions,object_bytes,spill_helpers,loop_spill_helpers,induction_address_spill_helpers\n' >"$OUTPUT"
for target in 65c02 riscv aarch64 arm x86_64 pcode; do
  for opt in -O0 -O2 -Os; do
    tag="${target}_${opt#-}"
    assembly="$WORK/$tag.s"
    object="$WORK/$tag.o"

    "$DAVECC" -target "$target" "$opt" -nostdinc -nostdlib \
      -Xsave-ir -S "$WORK/cases.c" -o "$assembly" >/dev/null
    cp "$WORK/cases.ir" "$WORK/$tag.ir"
    "$DAVECC" -target "$target" "$opt" -nostdinc -nostdlib \
      -c "$WORK/cases.c" -o "$object"

    ir_nodes=$(
      awk '/IR for function dead_expression/{inside=1; after_ssa=0; next}
           /IR for function /{if (inside) exit}
           inside && /After SSA has been removed/{after_ssa=1; next}
           inside && after_ssa && /^\$/ {count++}
           END {print count+0}' "$WORK/$tag.ir"
    )
    target_instructions=$(
      awk '/^dead_expression:/{inside=1; next}
           /^\.func_end_dead_expression:/{inside=0}
           inside {
             line=$0
             sub(/^[[:space:]]*\/\* @[0-9]+ \*\/[[:space:]]*/, "", line)
             if (line ~ /^[[:space:]]*[[:alpha:]_][[:alnum:]_.]*[[:space:]]/) count++
           }
           END {print count+0}' "$assembly"
    )
    loop_instructions=$(
      awk '/^(licm_invariant|induction_reload):/{inside=1; next}
           /^\.func_end_(licm_invariant|induction_reload):/{inside=0}
           inside {
             line=$0
             sub(/^[[:space:]]*\/\* @[0-9]+ \*\/[[:space:]]*/, "", line)
             if (line ~ /^[[:space:]]*[[:alpha:]_][[:alnum:]_.]*[[:space:]]/) count++
           }
           END {print count+0}' "$assembly"
    )
    induction_address_instructions=$(
      awk '/^(induction_array_sum|induction_array_store|induction_reverse_sum):/{inside=1; next}
           /^\.func_end_(induction_array_sum|induction_array_store|induction_reverse_sum):/{inside=0}
           inside {
             line=$0
             sub(/^[[:space:]]*\/\* @[0-9]+ \*\/[[:space:]]*/, "", line)
             if (line ~ /^[[:space:]]*[[:alpha:]_][[:alnum:]_.]*[[:space:]]/) count++
           }
           END {print count+0}' "$assembly"
    )
    sccp_ir_nodes=$(
      awk '/IR for function sccp_executable_phi/{inside=1; after_ssa=0; next}
           /IR for function /{if (inside) exit}
           inside && /After SSA has been removed/{after_ssa=1; next}
           inside && after_ssa && /^\$/ {count++}
           END {print count+0}' "$WORK/$tag.ir"
    )
    sccp_instructions=$(
      awk '/^(sccp_executable_phi|sccp_equal_phi):/{inside=1; next}
           /^\.func_end_(sccp_executable_phi|sccp_equal_phi):/{inside=0}
           inside {
             line=$0
             sub(/^[[:space:]]*\/\* @[0-9]+ \*\/[[:space:]]*/, "", line)
             if (line ~ /^[[:space:]]*[[:alpha:]_][[:alnum:]_.]*[[:space:]]/) count++
           }
           END {print count+0}' "$assembly"
    )
    alias_instructions=$(
      awk '/^(alias_distinct_globals|alias_may_alias):/{inside=1; next}
           /^\.func_end_(alias_distinct_globals|alias_may_alias):/{inside=0}
           inside {
             line=$0
             sub(/^[[:space:]]*\/\* @[0-9]+ \*\/[[:space:]]*/, "", line)
             if (line ~ /^[[:space:]]*[[:alpha:]_][[:alnum:]_.]*[[:space:]]/) count++
           }
           END {print count+0}' "$assembly"
    )
    object_bytes=$(wc -c <"$object" | tr -d ' ')
    spill_helpers=$(
      awk '/^dead_expression:/{inside=1; next}
           /^\.func_end_dead_expression:/{inside=0}
           inside && /__spill|__push|__load_result/ {count++}
           END {print count+0}' "$assembly"
    )
    loop_spill_helpers=$(
      awk '/^(licm_invariant|induction_reload):/{inside=1; next}
           /^\.func_end_(licm_invariant|induction_reload):/{inside=0}
           inside && /__spill|__push|__load_result/ {count++}
           END {print count+0}' "$assembly"
    )
    induction_address_spill_helpers=$(
      awk '/^(induction_array_sum|induction_array_store|induction_reverse_sum):/{inside=1; next}
           /^\.func_end_(induction_array_sum|induction_array_store|induction_reverse_sum):/{inside=0}
           inside && /__spill|__push|__load_result/ {count++}
           END {print count+0}' "$assembly"
    )
    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
      "$target" "$opt" "$ir_nodes" "$target_instructions" \
      "$loop_instructions" "$induction_address_instructions" \
      "$sccp_ir_nodes" "$sccp_instructions" "$alias_instructions" \
      "$object_bytes" "$spill_helpers" "$loop_spill_helpers" \
      "$induction_address_spill_helpers" >>"$OUTPUT"
  done
done
