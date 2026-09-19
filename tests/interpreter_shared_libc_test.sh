#!/usr/bin/env bash
# Link a small program against the interpreter-profile shared libc and run it.
set -euo pipefail

if [[ $# -ne 7 ]]; then
  echo "usage: $0 davecc aarch64 arm riscv x86_64 pcode libcx86_64.so" >&2
  exit 2
fi

DAVECC="$1"
X86_64="$5"
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

exe="$WORK/dynamic-x86_64.exe"
"$DAVECC" -target x86_64 -dynamic "$WORK/main.c" -o "$exe"
cp "$LIBDIR/libcx86_64.so" "$WORK/"
rc=0
"$X86_64" -i "$exe" || rc=$?
if [[ "$rc" -ne 11 ]]; then
  echo "x86_64 -dynamic execution returned $rc; expected 11" >&2
  exit 1
fi
