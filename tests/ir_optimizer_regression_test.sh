#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 14 ]]; then
  echo "usage: $0 davecc 6502 rom libc65 riscv libcrv aarch64 libca64 arm libcarm x86_64 libcx86 pcode libcpcode" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERP_65="$ROOT/$2"
ROM_65="$ROOT/$3"
LIBC_65="$ROOT/$4"
INTERP_RV="$ROOT/$5"
LIBC_RV="$ROOT/$6"
INTERP_A64="$ROOT/$7"
LIBC_A64="$ROOT/$8"
INTERP_ARM="$ROOT/$9"
LIBC_ARM="$ROOT/${10}"
INTERP_X86="$ROOT/${11}"
LIBC_X86="$ROOT/${12}"
INTERP_PCODE="$ROOT/${13}"
LIBC_PCODE="$ROOT/${14}"
WORK="${TEST_TMPDIR:-/tmp}/ir-optimizer-regression"
mkdir -p "$WORK"

C_SOURCE="$ROOT/tests/ir_optimizer_cases.c"
SCCP_ALIAS_SOURCE="$ROOT/tests/sccp_alias_execution_cases.c"
CXX_SOURCE="$ROOT/tests/ir_optimizer_cases.cc"

run_target() {
  local target=$1
  local interpreter=$2
  local libc=$3
  local interpreter_mode=$4
  local rom=${5:-}
  shift 5 || true

  for source in "$C_SOURCE" "$SCCP_ALIAS_SOURCE" "$CXX_SOURCE"; do
    local source_kind=c
    if [[ "$source" == *.cc ]]; then
      source_kind=cxx
    fi
    local expected_rc=-1
    for opt in -O0 -O2 -Os; do
      local executable="$WORK/${target}_${source_kind}_${opt#-}.exe"
      local compile=(
        "$DAVECC" -target "$target" "$opt" -static
        -isystem "$ROOT/libc/include"
      )
      if [[ "$source_kind" == cxx ]]; then
        compile+=(-std=c++20)
      fi
      if [[ $# -gt 0 ]]; then
        compile+=("$@")
      fi
      compile+=("$source" "$libc" -o "$executable")
      "${compile[@]}"

      local command=("$interpreter")
      if [[ -n "$rom" ]]; then
        command+=(-rom "$rom")
      fi
      if [[ "$interpreter_mode" == "integrated" ]]; then
        command+=(-i)
      fi
      command+=("$executable")
      set +e
      "${command[@]}"
      local rc=$?
      set -e
      if [[ "$opt" == "-O0" ]]; then
        expected_rc=$rc
      elif [[ "$rc" -ne "$expected_rc" ]]; then
        echo "$target $source_kind $opt returned $rc; -O0 returned $expected_rc" >&2
        exit 1
      fi
    done
  done
}

run_target 65c02 "$INTERP_65" "$LIBC_65" plain "$ROM_65"
run_target riscv "$INTERP_RV" "$LIBC_RV" plain ""
run_target aarch64 "$INTERP_A64" "$LIBC_A64" integrated "" -Wl,-e -Wl,main
run_target arm "$INTERP_ARM" "$LIBC_ARM" integrated "" -Wl,-e -Wl,main
run_target x86_64 "$INTERP_X86" "$LIBC_X86" integrated "" -Wl,-e -Wl,main
run_target pcode "$INTERP_PCODE" "$LIBC_PCODE" plain "" -Wl,-e -Wl,main

# Keep one shape assertion architecture-neutral at the IR level.  The final
# optimized dump must not retain the deliberately dead multiply.
DUMP_SOURCE="$WORK/optimizer_dump.c"
cp "$C_SOURCE" "$DUMP_SOURCE"
"$DAVECC" -target x86_64 -O2 -Xsave-ir -S \
  "$DUMP_SOURCE" -o "$WORK/optimizer.s"
IR_FILE="${DUMP_SOURCE%.c}.ir"
if [[ ! -f "$IR_FILE" ]]; then
  echo "optimizer IR dump was not produced" >&2
  exit 1
fi
INDUCTION_DUMP_SOURCE="$WORK/induction_dump.c"
cp "$C_SOURCE" "$INDUCTION_DUMP_SOURCE"
"$DAVECC" -target arm -O2 -Xsave-ir -S \
  "$INDUCTION_DUMP_SOURCE" -o "$WORK/induction.s"
INDUCTION_IR_FILE="${INDUCTION_DUMP_SOURCE%.c}.ir"
if [[ ! -f "$INDUCTION_IR_FILE" ]]; then
  echo "induction IR dump was not produced" >&2
  exit 1
fi
dead_body=$(
  awk '/IR for function dead_expression/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if grep -Eq '[[:space:]]muli\(' <<<"$dead_body"; then
  echo "IR DCE retained the dead multiply in dead_expression" >&2
  exit 1
fi

# SCCP must use executable edges when meeting phi inputs.  The infeasible
# multiply is removed and the surviving value is folded through the join.
sccp_body=$(
  awk '/IR for function sccp_executable_phi/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if grep -Eq '[[:space:]]muli\(' <<<"$sccp_body" ||
   ! grep -Eq 'const32\(\).* 16 ' <<<"$sccp_body"; then
  echo "SCCP did not fold the executable phi to its constant result" >&2
  exit 1
fi

# Equal constants from two executable predecessors also collapse a phi even
# when the controlling condition is not known.
equal_phi_body=$(
  awk '/IR for function sccp_equal_phi/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if grep -Eq 'load32\(.*REF value' <<<"$equal_phi_body" ||
   ! grep -Eq 'const32\(\).* 11 ' <<<"$equal_phi_body"; then
  echo "SCCP did not collapse an equal-valued phi" >&2
  exit 1
fi

# LICM must place the invariant multiply in the dedicated preheader, not in a
# loop block.  The preheader is visible as a nesting-zero block after SSA
# removal.
licm_multiply_locations=$(
  awk '/IR for function licm_invariant/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa && /^  Loop nesting: /{nesting=$3} \
       inside && after_ssa && /^\$/ && /[[:space:]]muli\(/{print nesting}' \
      "$IR_FILE"
)
if grep -Eq '^[1-9]' <<<"$licm_multiply_locations" ||
   ! grep -Eq '^0$' <<<"$licm_multiply_locations"; then
  echo "LICM did not move the invariant multiply to a loop preheader" >&2
  exit 1
fi

# Object-based alias analysis can prove that direct accesses to two distinct
# globals do not overlap, so the invariant read may leave the loop.
alias_read_locations=$(
  awk '/IR for function alias_distinct_globals/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa && /^  Loop nesting: /{nesting=$3} \
       inside && after_ssa && /^\$/ && /load32\(.*REF alias_read/{print nesting}' \
      "$IR_FILE"
)
if grep -Eq '^[1-9]' <<<"$alias_read_locations" ||
   ! grep -Eq '^0$' <<<"$alias_read_locations"; then
  echo "alias-aware LICM did not hoist a disjoint global load" >&2
  exit 1
fi

# A load through a possibly-aliasing pointer must remain in the loop.  Address
# values may be hoisted, but the unannotated pointee load itself may not.
alias_may_body=$(
  awk '/IR for function alias_may_alias/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if ! awk '
    /^  Loop nesting: / {nesting=$3}
    /^\$/ && /load32\(/ && !/REF/ && nesting ~ /^[1-9]/ {safe=1}
    END {exit safe ? 0 : 1}
  ' <<<"$alias_may_body"; then
  echo "alias-aware LICM hoisted a may-alias pointee load" >&2
  exit 1
fi

# Nested natural loops must retain distinct depths after preheader insertion.
nested_body=$(
  awk '/IR for function nested_licm/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if ! grep -Eq '^  Loop nesting: 2$' <<<"$nested_body"; then
  echo "nested loop depth was not preserved" >&2
  exit 1
fi

# A canonical induction update already computes the value consumed by the
# latch comparison.  There must not be a reload of i in the same block.
induction_body=$(
  awk '/IR for function induction_reload/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if ! awk '
    /^\*\*\* Basic block #/ {
      if (has_inc && has_reload) bad=1
      has_inc=0
      has_reload=0
    }
    /[[:space:]]inc32\(.*DEF i/ {has_inc=1}
    /[[:space:]]load32\(.*REF i/ {has_reload=1}
    END {
      if (has_inc && has_reload) bad=1
      exit bad ? 1 : (saw_inc ? 0 : 1)
    }
    /[[:space:]]inc32\(.*DEF i/ {saw_inc=1}
  ' <<<"$induction_body"; then
  echo "induction latch retained a redundant reload" >&2
  exit 1
fi

# A canonical zero-based indexed loop is represented by a derived pointer
# induction variable: scaling disappears and the latch advances the pointer by
# the element size.
array_induction_body=$(
  awk '/IR for function induction_array_sum/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$INDUCTION_IR_FILE"
)
array_scale_locations=$(
  awk '/^  Loop nesting: /{nesting=$3} \
       /^\$/ && /[[:space:]](muli|lsli)\(/{print nesting}' \
      <<<"$array_induction_body"
)
if [[ -n "$array_scale_locations" ]] ||
   ! grep -Eq '[[:space:]]inca\(.*DEF __invented__' \
      <<<"$array_induction_body" ||
   ! grep -Eq '[[:space:]]loada\(.*REF __invented__' \
      <<<"$array_induction_body"; then
  echo "indexed loop was not strength-reduced to a pointer recurrence" >&2
  exit 1
fi

# Non-unit integer updates are intentionally outside this first transform.
nonunit_induction_body=$(
  awk '/IR for function induction_nonunit_sum/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$INDUCTION_IR_FILE"
)
if ! awk '
    /^  Loop nesting: / {nesting=$3}
    nesting == 1 && /^\$/ && /[[:space:]](muli|lsli)\(/ {found=1}
    END {exit found ? 0 : 1}
  ' <<<"$nonunit_induction_body"; then
  echo "non-unit induction loop was transformed outside the supported scope" >&2
  exit 1
fi

# A reverse loop can begin one element before the array when it executes zero
# times.  Do not speculate that derived pointer in the preheader.
reverse_induction_body=$(
  awk '/IR for function induction_reverse_sum/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$INDUCTION_IR_FILE"
)
if grep -Eq '[[:space:]]deca\(.*DEF __invented__' \
    <<<"$reverse_induction_body"; then
  echo "reverse induction speculated a possibly invalid pointer" >&2
  exit 1
fi

# AArch64 target peepholes run after IR lowering and before register
# allocation.  Check their emitted instruction shapes independently of the IR
# assertions above.
AARCH64_PEEPHOLE_SOURCE="$ROOT/tests/aarch64_peephole_cases.c"
AARCH64_PEEPHOLE_ASM="$WORK/aarch64_peepholes.s"
"$DAVECC" -target aarch64 -O2 -nostdinc -nostdlib -S \
  "$AARCH64_PEEPHOLE_SOURCE" -o "$AARCH64_PEEPHOLE_ASM"

function_body() {
  local function_name=$1
  local assembly=$2
  awk "/^${function_name}:$/{inside=1; next} \
       /^\\.func_end_${function_name}:$/{inside=0} \
       inside" "$assembly"
}

move_body=$(function_body move_chain "$AARCH64_PEEPHOLE_ASM")
if grep -Eq '[[:space:]]mov[[:space:]]' <<<"$move_body"; then
  echo "AArch64 peephole retained a redundant move chain" >&2
  exit 1
fi

branch_body=$(function_body branch_zero "$AARCH64_PEEPHOLE_ASM")
if ! grep -Eq '[[:space:]]cb(n?z)[[:space:]]' <<<"$branch_body" ||
   grep -Eq '[[:space:]]cmp[[:space:]].*#0' <<<"$branch_body"; then
  echo "AArch64 zero branch was not fused to cbz/cbnz" >&2
  exit 1
fi

extend_body=$(function_body zero_extend_byte "$AARCH64_PEEPHOLE_ASM")
if ! grep -Eq '[[:space:]]ubfx[[:space:]].*#0, #8' <<<"$extend_body" ||
   grep -Eq '[[:space:]]and[[:space:]]' <<<"$extend_body"; then
  echo "AArch64 zero extension was not reduced to one instruction" >&2
  exit 1
fi

shift_body=$(function_body shifted_add "$AARCH64_PEEPHOLE_ASM")
if ! grep -Eq '[[:space:]]add[[:space:]].*, lsl #1' <<<"$shift_body" ||
   grep -Eq '^[[:space:]]*(/\* @[0-9]+ \*/[[:space:]]*)?lsl[[:space:]]' \
      <<<"$shift_body"; then
  echo "AArch64 shifted add was not fused" >&2
  exit 1
fi

small_offset_body=$(function_body load_small_offset "$AARCH64_PEEPHOLE_ASM")
if ! grep -Eq '[[:space:]]ldr[[:space:]].*\[[^]]+, #12\]' \
    <<<"$small_offset_body" ||
   grep -Eq '[[:space:]]add[[:space:]].*#12' <<<"$small_offset_body"; then
  echo "AArch64 small address offset was not folded into the load" >&2
  exit 1
fi

large_offset_body=$(function_body load_large_offset "$AARCH64_PEEPHOLE_ASM")
if ! grep -Eq '[[:space:]]add[[:space:]].*#400' <<<"$large_offset_body" ||
   ! grep -Eq '[[:space:]]ldr[[:space:]].*\[[^]]+, #0\]' \
      <<<"$large_offset_body" ||
   grep -Eq '[[:space:]]ldr[[:space:]].*\[[^]]+, #400\]' \
      <<<"$large_offset_body"; then
  echo "AArch64 out-of-range load offset was folded unsafely" >&2
  exit 1
fi
