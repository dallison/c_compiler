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
AARCH64_COROUTINE_SOURCE="$ROOT/tests/aarch64_coroutine_exchange_test.cc"

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

# Exercise coroutine lowering, mixed-size C++ calls, register pressure, branch
# relaxation, and dynamic relocation handling on every backend and at every
# supported optimization level.
run_coroutine_target() {
  local target=$1
  local interpreter=$2
  local interpreter_mode=$3
  local rom=${4:-}

  for opt in -O0 -O1 -O2 -O3 -Os; do
    local executable="$WORK/${target}_coroutine_${opt#-}.exe"
    local output="$WORK/${target}_coroutine_${opt#-}.out"
    "$DAVECC" -target "$target" "$opt" -std=c++20 \
      -isystem "$ROOT/libc/include" \
      "$AARCH64_COROUTINE_SOURCE" -o "$executable"

    local command=("$interpreter")
    if [[ -n "$rom" ]]; then
      command+=(-rom "$rom")
    fi
    if [[ "$interpreter_mode" == "integrated" ]]; then
      command+=(-i)
    fi
    command+=("$executable")
    "${command[@]}" >"$output"
    if [[ "$(cat "$output")" != "received: 42" ]]; then
      echo "$target coroutine $opt produced unexpected output:" >&2
      cat "$output" >&2
      exit 1
    fi
  done
}

run_coroutine_target 6502 "$INTERP_65" plain "$ROM_65"
run_coroutine_target 65c02 "$INTERP_65" plain "$ROM_65"
run_coroutine_target riscv "$INTERP_RV" plain ""
run_coroutine_target aarch64 "$INTERP_A64" integrated ""
run_coroutine_target arm "$INTERP_ARM" integrated ""
run_coroutine_target x86_64 "$INTERP_X86" integrated ""
run_coroutine_target pcode "$INTERP_PCODE" plain ""

# A lowering-created `this` pseudo used only to copy the co_await operand into
# its stack temporary is dead before the following call. It should use a
# caller-saved temporary rather than consume and spill a callee-saved register.
AARCH64_COROUTINE_ASM="$WORK/aarch64_coroutine.s"
"$DAVECC" -target aarch64 -O2 -std=c++20 \
  -isystem "$ROOT/libc/include" -S \
  "$AARCH64_COROUTINE_SOURCE" -o "$AARCH64_COROUTINE_ASM"
if ! grep -Eq \
    '[[:space:]]add[[:space:]]+x1[0-5],[[:space:]]*x10,[[:space:]]*#24' \
    "$AARCH64_COROUTINE_ASM"; then
  echo "AArch64 assigned the short-lived coroutine operand a saved register" >&2
  exit 1
fi

# An optimized ARM struct-return leaf should use caller-saved temporaries, keep
# reference arguments in registers, and omit all frame/save traffic.
ARM_COROUTINE_ASM="$WORK/arm_coroutine.s"
"$DAVECC" -target arm -O2 -std=c++20 \
  -isystem "$ROOT/libc/include" -S \
  "$AARCH64_COROUTINE_SOURCE" -o "$ARM_COROUTINE_ASM"
ARM_FROM_PROMISE=$(
  awk '/^_ZN3std16coroutine_handleI7PromiseE12from_promiseER7Promise:/{inside=1} \
       inside{print} \
       /^\.func_end__ZN3std16coroutine_handleI7PromiseE12from_promiseER7Promise:/{exit}' \
      "$ARM_COROUTINE_ASM"
)
if grep -Eq 'stmdb sp|ldmia sp|add fp|str[[:space:]]+r1,|mov[[:space:]]+r[0-3],[[:space:]]*r[0-3]' \
       <<<"$ARM_FROM_PROMISE" ||
   ! grep -Eq 'sub[[:space:]]+r[23],[[:space:]]*r[12],[[:space:]]*#16' \
       <<<"$ARM_FROM_PROMISE"; then
  echo "ARM generated saved-register traffic for from_promise leaf" >&2
  exit 1
fi

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

# Observable checkpoints survive IR optimization, preserve source ordering, and
# lower to no target instruction.
checkpoint_body=$(
  awk '/IR for function checkpoint_order/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
checkpoint_store_line=$(grep -n -m1 '[[:space:]]store32(' <<<"$checkpoint_body" |
                        cut -d: -f1 || true)
checkpoint_line=$(grep -n -m1 '[[:space:]]observable_checkpoint(' \
                         <<<"$checkpoint_body" | cut -d: -f1 || true)
checkpoint_load_line=$(grep -n -m1 '[[:space:]]load32(' <<<"$checkpoint_body" |
                       cut -d: -f1 || true)
if [[ -z "$checkpoint_store_line" || -z "$checkpoint_line" ||
      -z "$checkpoint_load_line" ||
      "$checkpoint_store_line" -ge "$checkpoint_line" ||
      "$checkpoint_line" -ge "$checkpoint_load_line" ]]; then
  echo "observable checkpoint did not preserve optimized IR ordering" >&2
  exit 1
fi
checkpoint_asm=$(
  awk '/^checkpoint_order:/{inside=1} \
       inside{print} \
       /^\.func_end_checkpoint_order:/{exit}' "$WORK/optimizer.s"
)
if grep -Eq '^[[:space:]]*nop([[:space:]]|$)' <<<"$checkpoint_asm"; then
  echo "observable checkpoint emitted a machine instruction" >&2
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
nested_multiply_locations=$(
  awk '/^  Loop nesting: /{nesting=$3} \
       /^\$/ && /[[:space:]]muli\(/{print nesting}' \
      <<<"$nested_body"
)
if grep -Eq '^[1-9]' <<<"$nested_multiply_locations" ||
   ! grep -Eq '^0$' <<<"$nested_multiply_locations"; then
  echo "LICM did not hoist the nested-loop invariant multiply to a preheader" >&2
  exit 1
fi

# Store-to-load forwarding replaces a reload of a just-stored global with the
# stored value.  Dead-store elimination drops the overwritten store of 1.
store_forward_body=$(
  awk '/IR for function store_forward_global/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if grep -Eq 'load32\(.*REF memopt_global' <<<"$store_forward_body" ||
   ! grep -Eq 'const32\(\).* 11 ' <<<"$store_forward_body"; then
  echo "store-to-load did not forward the stored global constant" >&2
  exit 1
fi
dead_store_body=$(
  awk '/IR for function dead_store_global/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if grep -Eq 'store32\(.* 1 ' <<<"$dead_store_body"; then
  echo "dead-store elimination retained the overwritten store of 1" >&2
  exit 1
fi
load_cse_body=$(
  awk '/IR for function load_cse_global/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
load_cse_count=$(
  grep -c 'load32\(.*REF memopt_global' <<<"$load_cse_body" || true
)
if [[ "$load_cse_count" -ne 1 ]]; then
  echo "load CSE did not collapse the two global loads (found $load_cse_count)" >&2
  exit 1
fi
combine_add_body=$(
  awk '/IR for function combine_nested_add/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if grep -c '[[:space:]]addi(' <<<"$combine_add_body" | grep -qx 1 &&
   grep -Eq 'const32\(\).* 7 ' <<<"$combine_add_body"; then
  :
else
  echo "nested integer adds were not folded to a single +7" >&2
  exit 1
fi

# Constant division becomes a widening multiply and shifts.  The original
# divi must not remain; /10 uses the 0xcccccccd magic.
udiv10_body=$(
  awk '/IR for function udiv_const10/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if grep -Eq '[[:space:]]divi\(' <<<"$udiv10_body" ||
   ! grep -Eq '[[:space:]]muli\(' <<<"$udiv10_body"; then
  echo "unsigned /10 was not rewritten to a multiply" >&2
  exit 1
fi
sdiv10_body=$(
  awk '/IR for function sdiv_const10/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if grep -Eq '[[:space:]]divi\(' <<<"$sdiv10_body" ||
   ! grep -Eq '[[:space:]]muli\(' <<<"$sdiv10_body"; then
  echo "signed /10 was not rewritten to a multiply" >&2
  exit 1
fi
udiv7_body=$(
  awk '/IR for function udiv_const7/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if grep -Eq '[[:space:]]divi\(' <<<"$udiv7_body" ||
   ! grep -Eq '[[:space:]]muli\(' <<<"$udiv7_body"; then
  echo "unsigned /7 was not rewritten to a multiply" >&2
  exit 1
fi

# Complete unroll is -O3 only.  A 4-trip counted loop must still be a loop at
# -O2 and must become straight-line at -O3 (no back-edge compare/increment).
unroll_o2_body=$(
  awk '/IR for function unroll_sum4/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if ! grep -Eq '^  Loop nesting: 1$' <<<"$unroll_o2_body" ||
   ! grep -Eq '[[:space:]]inc32\(' <<<"$unroll_o2_body" ||
   ! grep -Eq '[[:space:]]btrue\(' <<<"$unroll_o2_body"; then
  echo "constant-trip loop was unrolled at -O2" >&2
  exit 1
fi
UNROLL_DUMP_SOURCE="$WORK/unroll_dump.c"
cp "$C_SOURCE" "$UNROLL_DUMP_SOURCE"
"$DAVECC" -target x86_64 -O3 -Xsave-ir -S \
  "$UNROLL_DUMP_SOURCE" -o "$WORK/unroll.s"
UNROLL_IR_FILE="${UNROLL_DUMP_SOURCE%.c}.ir"
if [[ ! -f "$UNROLL_IR_FILE" ]]; then
  echo "unroll IR dump was not produced" >&2
  exit 1
fi
unroll_o3_body=$(
  awk '/IR for function unroll_sum4/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$UNROLL_IR_FILE"
)
if grep -Eq '[[:space:]]inc32\(' <<<"$unroll_o3_body" ||
   grep -Eq '[[:space:]]btrue\(' <<<"$unroll_o3_body" ||
   grep -Eq '[[:space:]]cmplti\(' <<<"$unroll_o3_body" ||
   ! grep -Eq '[[:space:]]adda\(' <<<"$unroll_o3_body"; then
  echo "constant-trip loop was not completely unrolled at -O3" >&2
  exit 1
fi
adda_count=$(grep -c '[[:space:]]adda(' <<<"$unroll_o3_body" || true)
if [[ "$adda_count" -lt 4 ]]; then
  echo "unrolled body did not contain four address adds (found $adda_count)" >&2
  exit 1
fi

UNROLL_O0_EXE="$WORK/unroll_o0.exe"
UNROLL_O3_EXE="$WORK/unroll_o3.exe"
"$DAVECC" -target x86_64 -O0 -static -isystem "$ROOT/libc/include" \
  -Wl,-e -Wl,main "$C_SOURCE" "$LIBC_X86" -o "$UNROLL_O0_EXE"
"$DAVECC" -target x86_64 -O3 -static -isystem "$ROOT/libc/include" \
  -Wl,-e -Wl,main "$C_SOURCE" "$LIBC_X86" -o "$UNROLL_O3_EXE"
set +e
"$INTERP_X86" -i "$UNROLL_O0_EXE"
unroll_o0_rc=$?
"$INTERP_X86" -i "$UNROLL_O3_EXE"
unroll_o3_rc=$?
set -e
if [[ "$unroll_o3_rc" -ne "$unroll_o0_rc" ]]; then
  echo "x86_64 -O3 returned $unroll_o3_rc; -O0 returned $unroll_o0_rc" >&2
  exit 1
fi

# Small local structs/arrays used only at constant offsets become scalars.
# After SSA the member adda must be gone; the sum is just the two arguments.
sroa_point_body=$(
  awk '/IR for function sroa_point/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if grep -Eq '[[:space:]]adda\(' <<<"$sroa_point_body"; then
  echo "SROA left a constant-offset adda in sroa_point" >&2
  exit 1
fi
sroa_pair_body=$(
  awk '/IR for function sroa_pair/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if grep -Eq '[[:space:]]adda\(' <<<"$sroa_pair_body"; then
  echo "SROA left a constant-offset adda in sroa_pair" >&2
  exit 1
fi

# x86-64 lowers a dense switch to a RIP-relative indirect jump through
# 8-byte-aligned jmp slots.
dense_switch_ir=$(
  awk '/IR for function dense_switch/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$IR_FILE"
)
if ! grep -Eq '[[:space:]]cbra\(' <<<"$dense_switch_ir"; then
  echo "dense switch was not lowered to a computed branch" >&2
  exit 1
fi
dense_switch_asm=$(
  awk '/^dense_switch:/{inside=1} \
       inside{print} \
       /^\.func_end_dense_switch:/{exit}' "$WORK/optimizer.s"
)
if ! grep -Eq 'jmp[[:space:]]+\*' <<<"$dense_switch_asm"; then
  echo "x86_64 dense switch was not lowered to an indirect jump table" >&2
  exit 1
fi

# Direct `T x(args)` inlines the constructor into stores of `this`.  Copy
# initialization may still call the copy constructor (it constructs a
# temporary that codegen retargets).  Destructors are never inlined.
CXX_DUMP_SOURCE="$WORK/cxx_optimizer_dump.cc"
cp "$CXX_SOURCE" "$CXX_DUMP_SOURCE"
"$DAVECC" -target x86_64 -std=c++20 -O2 -Xsave-ir -S \
  -isystem "$ROOT/libc/include" \
  "$CXX_DUMP_SOURCE" -o "$WORK/cxx_optimizer.s"
CXX_IR_FILE="${CXX_DUMP_SOURCE%.cc}.ir"
if [[ ! -f "$CXX_IR_FILE" ]]; then
  echo "C++ optimizer IR dump was not produced" >&2
  exit 1
fi
cpp_loop_ir=$(
  awk '/IR for function cpp_loop/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$CXX_IR_FILE"
)
if grep -Eq '[[:space:]]calla\(' <<<"$cpp_loop_ir"; then
  echo "cpp_loop retained a call after constructor inlining" >&2
  exit 1
fi
cpp_loop_asm=$(
  awk '/^_Z8cpp_loopii:/{inside=1} \
       inside{print} \
       /^\.func_end__Z8cpp_loopii:/{exit}' "$WORK/cxx_optimizer.s"
)
if grep -Eq '[[:space:]]call[[:space:]]' <<<"$cpp_loop_asm"; then
  echo "cpp_loop x86_64 asm retained a call after constructor inlining" >&2
  exit 1
fi
if grep -Eq 'call[[:space:]]+_ZN11AccumulatorC1Ei' "$WORK/cxx_optimizer.s"; then
  echo "x86_64 still calls Accumulator(int) after constructor inlining" >&2
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
check_derived_induction_ir() {
  local ir_file=$1
  local target_name=$2
  local array_induction_body
  array_induction_body=$(
    awk '/IR for function induction_array_sum/{inside=1; after_ssa=0; next} \
         /IR for function /{if (inside) exit} \
         inside && /After SSA has been removed/{after_ssa=1; next} \
         inside && after_ssa' "$ir_file"
  )
  local array_scale_locations
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
    echo "$target_name indexed loop was not strength-reduced to a pointer recurrence" >&2
    exit 1
  fi

  # Non-unit integer updates are intentionally outside this first transform.
  local nonunit_induction_body
  nonunit_induction_body=$(
    awk '/IR for function induction_nonunit_sum/{inside=1; after_ssa=0; next} \
         /IR for function /{if (inside) exit} \
         inside && /After SSA has been removed/{after_ssa=1; next} \
         inside && after_ssa' "$ir_file"
  )
  if ! awk '
      /^  Loop nesting: / {nesting=$3}
      nesting == 1 && /^\$/ && /[[:space:]](muli|lsli)\(/ {found=1}
      END {exit found ? 0 : 1}
    ' <<<"$nonunit_induction_body"; then
    echo "$target_name non-unit induction loop was transformed outside the supported scope" >&2
    exit 1
  fi

  # A reverse loop can begin one element before the array when it executes zero
  # times.  Do not speculate that derived pointer in the preheader.
  local reverse_induction_body
  reverse_induction_body=$(
    awk '/IR for function induction_reverse_sum/{inside=1; after_ssa=0; next} \
         /IR for function /{if (inside) exit} \
         inside && /After SSA has been removed/{after_ssa=1; next} \
         inside && after_ssa' "$ir_file"
  )
  if grep -Eq '[[:space:]]deca\(.*DEF __invented__' \
      <<<"$reverse_induction_body"; then
    echo "$target_name reverse induction speculated a possibly invalid pointer" >&2
    exit 1
  fi
}

check_derived_induction_ir "$INDUCTION_IR_FILE" "ARM"

AARCH64_INDUCTION_DUMP_SOURCE="$WORK/aarch64_induction_dump.c"
cp "$C_SOURCE" "$AARCH64_INDUCTION_DUMP_SOURCE"
"$DAVECC" -target aarch64 -O2 -Xsave-ir -S \
  "$AARCH64_INDUCTION_DUMP_SOURCE" -o "$WORK/aarch64_induction.s"
AARCH64_INDUCTION_IR_FILE="${AARCH64_INDUCTION_DUMP_SOURCE%.c}.ir"
if [[ ! -f "$AARCH64_INDUCTION_IR_FILE" ]]; then
  echo "AArch64 induction IR dump was not produced" >&2
  exit 1
fi
check_derived_induction_ir "$AARCH64_INDUCTION_IR_FILE" "AArch64"

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

zero32_body=$(function_body store_zero32 "$AARCH64_PEEPHOLE_ASM")
zero64_body=$(function_body store_zero64 "$AARCH64_PEEPHOLE_ASM")
if ! grep -Eq '[[:space:]]str[[:space:]]+wzr,' <<<"$zero32_body" ||
   ! grep -Eq '[[:space:]]str[[:space:]]+xzr,' <<<"$zero64_body" ||
   grep -Eq '(^|[^[:alnum:]_])(x31|w31)([^[:alnum:]_]|$)' \
      "$AARCH64_PEEPHOLE_ASM"; then
  echo "AArch64 emitter did not use architectural zero-register names" >&2
  exit 1
fi

# A leaf returning a small aggregate receives its hidden result pointer in x8.
# Taking the address of a reference argument uses the pointer already in x0;
# neither value needs a stack home or a callee-saved register.
AARCH64_LEAF_SOURCE="$WORK/aarch64_leaf_frame.cc"
AARCH64_LEAF_ASM="$WORK/aarch64_leaf_frame.s"
AARCH64_LEAF_IR="${AARCH64_LEAF_SOURCE%.cc}.ir"
cat >"$AARCH64_LEAF_SOURCE" <<'EOF'
struct LeafResult {
  char* value;
};

extern "C" LeafResult leaf_reference(int& value) {
  char* address = reinterpret_cast<char*>(&value);
  return {address - 24};
}
EOF
"$DAVECC" -target aarch64 -std=c++20 -O2 -Xsave-ir -S \
  "$AARCH64_LEAF_SOURCE" -o "$AARCH64_LEAF_ASM"

leaf_reference_ir=$(
  awk '/IR for function leaf_reference/{inside=1; after_ssa=0; next} \
       /IR for function /{if (inside) exit} \
       inside && /After SSA has been removed/{after_ssa=1; next} \
       inside && after_ssa' "$AARCH64_LEAF_IR"
)
if grep -Eq 'memzero\(|loada\(.*REF address' \
    <<<"$leaf_reference_ir"; then
  echo "IR copy propagation retained a redundant aggregate zero/load" >&2
  exit 1
fi

leaf_reference_body=$(function_body leaf_reference "$AARCH64_LEAF_ASM")
if grep -Eq '(^|[^[:alnum:]_])(x29|x30|sp)([^[:alnum:]_]|$)|Saved (argument|integer|floating-point) registers' \
    <<<"$leaf_reference_body"; then
  echo "AArch64 leaf reference return unnecessarily created a stack frame" >&2
  exit 1
fi
if grep -Eq '[[:space:]]mov[[:space:]]|[[:space:]]str[[:space:]]+(x31|xzr)' \
    <<<"$leaf_reference_body" ||
   ! grep -Eq '[[:space:]]sub[[:space:]]+[^,]+,[[:space:]]*x0,[[:space:]]*#24' \
      <<<"$leaf_reference_body" ||
   ! grep -Eq '[[:space:]]str[[:space:]]+[^,]+,[[:space:]]*\[x8,[[:space:]]*#0\]' \
      <<<"$leaf_reference_body"; then
  echo "AArch64 leaf reference return retained redundant register copies" >&2
  exit 1
fi

# Passing a computed 64-bit ?: result (and related wide merge temps) into calls
# must preserve both halves across argument materialization.
ARM_I64_CALL_ARG_SOURCE="$ROOT/tests/arm_i64_call_arg_test.c"
ARM_I64_CALL_ARG_EXE="$WORK/arm_i64_call_arg.exe"
"$DAVECC" -target arm -O0 -nostdinc -nostdlib -static -Wl,-e -Wl,main \
  "$ARM_I64_CALL_ARG_SOURCE" -o "$ARM_I64_CALL_ARG_EXE"
if ! "$INTERP_ARM" -i "$ARM_I64_CALL_ARG_EXE"; then
  echo "ARM 64-bit call argument regression executable failed at -O0" >&2
  exit 1
fi

ARM_I64_CALL_ARG_O2_EXE="$WORK/arm_i64_call_arg_o2.exe"
"$DAVECC" -target arm -O2 -nostdinc -nostdlib -static -Wl,-e -Wl,main \
  "$ARM_I64_CALL_ARG_SOURCE" -o "$ARM_I64_CALL_ARG_O2_EXE"
if ! "$INTERP_ARM" -i "$ARM_I64_CALL_ARG_O2_EXE"; then
  echo "ARM 64-bit call argument regression executable failed at -O2" >&2
  exit 1
fi
