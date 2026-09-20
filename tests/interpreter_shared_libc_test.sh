#!/usr/bin/env bash
# Link a small program against the interpreter-profile shared libc and run it
# on every PIC-capable architecture that the interpreter can load today.
set -euo pipefail

if [[ $# -lt 7 ]]; then
  echo "usage: $0 davecc aarch64 arm riscv x86_64 pcode libcx86_64.so" >&2
  exit 2
fi

DAVECC="$1"
AARCH64="$2"
ARM="$3"
RISCV="$4"
X86_64="$5"
PCODE="$6"
LIBDIR="$(cd "$(dirname "$7")" && pwd)"
export DAVECC_LIB_DIR="$LIBDIR"
if [[ -d "$LIBDIR/include" ]]; then
  export DAVECC_INCLUDE_DIR="$LIBDIR/include"
elif [[ -d "$(dirname "$DAVECC")/libc/include" ]]; then
  export DAVECC_INCLUDE_DIR="$(dirname "$DAVECC")/libc/include"
fi

WORK="${TEST_TMPDIR:-/tmp}/interpreter_shared_libc"
mkdir -p "$WORK"

cat >"$WORK/main.c" <<'SRC'
#include <stdio.h>
int main(void) {
  puts("ok");
  return 11;
}
SRC

run_one() {
  local arch="$1"
  local interp="$2"
  local so_name="$3"
  shift 3
  local interp_args=("$@")
  local exe="$WORK/dynamic-${arch}.exe"
  "$DAVECC" -target "$arch" -dynamic "$WORK/main.c" -o "$exe"
  if [[ -f "$LIBDIR/$so_name" ]]; then
    cp "$LIBDIR/$so_name" "$WORK/"
  fi
  local rc=0
  perl -e 'alarm 20; exec @ARGV' "$interp" ${interp_args[@]+"${interp_args[@]}"} "$exe" \
    >"$WORK/${arch}.out" 2>"$WORK/${arch}.err" || rc=$?
  if [[ "$rc" -ne 11 ]]; then
    echo "$arch -dynamic execution returned $rc; expected 11" >&2
    if [[ -s "$WORK/${arch}.err" ]]; then
      cat "$WORK/${arch}.err" >&2
    fi
    exit 1
  fi
}

run_one aarch64 "$AARCH64" libcaarch64.so -i
run_one arm "$ARM" libcarm.so
run_one riscv "$RISCV" libcriscv.so
run_one x86_64 "$X86_64" libcx86_64.so -i
run_one x86 "$X86_64" libcx86.so
run_one pcode "$PCODE" libcpcode.so

if [[ -f "$LIBDIR/libcriscv32.so" ]]; then
  "$DAVECC" -target riscv32 -dynamic "$WORK/main.c" -o "$WORK/dynamic-riscv32.exe"
fi
