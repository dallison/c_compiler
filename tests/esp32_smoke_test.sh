#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 8 ]]; then
  echo "usage: $0 davecc esp32 elfdump xtensadasm libc start exec-case libc-case" >&2
  exit 2
fi

resolve_runfile() {
  if [[ "$1" = /* ]]; then
    printf '%s\n' "$1"
  else
    printf '%s/%s\n' "${TEST_SRCDIR}/${TEST_WORKSPACE}" "$1"
  fi
}

DAVECC=$(resolve_runfile "$1")
ESP32=$(resolve_runfile "$2")
ELFDUMP=$(resolve_runfile "$3")
XTENSADASM=$(resolve_runfile "$4")
LIBC=$(resolve_runfile "$5")
START=$(resolve_runfile "$6")
EXEC_CASE=$(resolve_runfile "$7")
LIBC_CASE=$(resolve_runfile "$8")
ROOT="${TEST_SRCDIR}/${TEST_WORKSPACE}"
WORK="${TEST_TMPDIR}/esp32-smoke"
mkdir -p "$WORK"

"$DAVECC" -target esp32 -nostdinc -S "$EXEC_CASE" -o "$WORK/abi.s"
grep -q 'entry a1' "$WORK/abi.s"
grep -q 'call8' "$WORK/abi.s"
grep -q 'callx8' "$WORK/abi.s"
grep -q 'retw' "$WORK/abi.s"

"$DAVECC" -target esp32 -nostdinc -c "$EXEC_CASE" -o "$WORK/abi.o"
"$ELFDUMP" "$WORK/abi.o" >"$WORK/abi.elfdump"
grep -q $'Machine:\tXtensa' "$WORK/abi.elfdump"
grep -q '\.xtensa\.info' "$WORK/abi.elfdump"
"$XTENSADASM" "$WORK/abi.o" >"$WORK/abi.disassembly"
grep -q 'entry' "$WORK/abi.disassembly"
grep -q 'call8' "$WORK/abi.disassembly"

cat >"$WORK/external-call.c" <<'EOF'
extern int external_function(int);
int use_external(int value) { return external_function(value); }
EOF
"$DAVECC" -target esp32 -nostdinc -c "$WORK/external-call.c" \
  -o "$WORK/external-call.o"
"$ELFDUMP" -S "$WORK/external-call.o" >"$WORK/external-call.sections"
"$ELFDUMP" -r "$WORK/external-call.o" >"$WORK/external-call.relocations"
grep -q '\.rela\.text' "$WORK/external-call.sections"
grep -q '00000014.*external_function' "$WORK/external-call.relocations"

cat >"$WORK/riscv-only.s" <<'EOF'
.text
lui a2, 1
EOF
if "$DAVECC" -target esp32 -nostdinc -c "$WORK/riscv-only.s" \
    -o "$WORK/riscv-only.o" >"$WORK/riscv-only.log" 2>&1; then
  echo "ESP32 assembler accepted a RISC-V-only instruction" >&2
  exit 1
fi
grep -q 'unknown instruction: lui' "$WORK/riscv-only.log"

compile_and_run() {
  local source=$1 output=$2
  "$DAVECC" -target esp32 -nostdlib -static \
    -isystem "$ROOT/libc/include" \
    "$START" "$source" "$LIBC" -e _start -o "$output"
  "$ELFDUMP" -S "$output" >"$output.sections"
  grep -Eq '\.bss.*3ff[bcdef][0-9a-f]{4}' "$output.sections"
  "$ESP32" "$output"
}

compile_and_run "$EXEC_CASE" "$WORK/abi.elf"
output=$(compile_and_run "$LIBC_CASE" "$WORK/libc.elf")
[[ "$output" = "esp32:42" ]]

echo "ESP32 Xtensa smoke test passed"
