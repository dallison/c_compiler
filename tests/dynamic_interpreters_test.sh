#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 8 ]]; then
  echo "usage: $0 davecc aarch64 arm riscv x86_64 pcode lib.c main.c" >&2
  exit 2
fi

DAVECC="$1"
AARCH64="$2"
ARM="$3"
RISCV="$4"
X86_64="$5"
PCODE="$6"
LIB_SOURCE="$7"
MAIN_SOURCE="$8"
WORK="${TEST_TMPDIR:-/tmp}/dynamic_interpreters"
mkdir -p "$WORK"

run_one() {
  local target="$1"
  local interpreter="$2"
  local interpret_flag="$3"
  local library="$WORK/libfunc-$target.so"
  local executable="$WORK/dynamic-$target.exe"

  "$DAVECC" -target "$target" -nostdinc -fpic -shared \
    "$LIB_SOURCE" -o "$library"
  "$DAVECC" -target "$target" -nostdinc -nostdlib -fpic \
    -Wl,-e -Wl,main -rpath "$WORK" \
    "$MAIN_SOURCE" "$library" -o "$executable"

  # LD_BIND_NOW makes the loader resolve every PLT slot up front instead of
  # leaving the lazy resolver to do it on the first call.  That is a separate
  # body of code writing the same slots, so run both ways.
  local bind_now
  for bind_now in "" 1; do
    local rc=0
    if [[ "$interpret_flag" == yes ]]; then
      LD_BIND_NOW="$bind_now" "$interpreter" -i "$executable" || rc=$?
    else
      LD_BIND_NOW="$bind_now" "$interpreter" "$executable" || rc=$?
    fi
    if [[ "$rc" -ne 99 ]]; then
      echo "$target dynamic execution returned $rc; expected 99" \
        "(LD_BIND_NOW='$bind_now')" >&2
      exit 1
    fi
  done
}

run_one aarch64 "$AARCH64" yes
run_one arm "$ARM" yes
run_one riscv "$RISCV" no
run_one x86_64 "$X86_64" yes
run_one pcode "$PCODE" no
