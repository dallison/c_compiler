#!/bin/bash
set -euo pipefail

if [[ $# -ne 10 ]]; then
  echo "usage: $0 run davecc libc65 libc-aarch64 libc-arm libc-pcode libc-riscv libc-x86_64 libc-xtensa start-xtensa" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
RUN="$ROOT/$1"
DAVECC="$ROOT/$2"
LIBC_65="$ROOT/$3"
LIBC_AARCH64="$ROOT/$4"
LIBC_ARM="$ROOT/$5"
LIBC_PCODE="$ROOT/$6"
LIBC_RISCV="$ROOT/$7"
LIBC_X86_64="$ROOT/$8"
LIBC_XTENSA="$ROOT/$9"
START_XTENSA="$ROOT/${10}"

WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/run-dispatch.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

cat >"$WORK/program.c" <<'SRC'
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
  if (argc != 3 || argv == 0 || argv[3] != 0) {
    return 1;
  }
  if (argv[0] == 0 || argv[0][0] == '\0') {
    return 2;
  }
  if (atoi(argv[1]) != 10 || strcmp(argv[2], "-d") != 0) {
    return 3;
  }
  return 0;
}
SRC

run_case() {
  local target=$1
  local libc=$2
  local executable="$WORK/$target.exe"

  "$DAVECC" -target "$target" -O0 -static -isystem "$ROOT/libc/include" \
    "$WORK/program.c" "$libc" -o "$executable"
  "$RUN" "$executable" 10 -d
}

run_case 65c02 "$LIBC_65"
run_case aarch64 "$LIBC_AARCH64"
run_case arm "$LIBC_ARM"
run_case pcode "$LIBC_PCODE"
run_case riscv "$LIBC_RISCV"
run_case x86_64 "$LIBC_X86_64"

cat >"$WORK/esp32.c" <<'SRC'
int main(void) { return 0; }
SRC
"$DAVECC" -target esp32 -O0 -nostdlib -static -isystem "$ROOT/libc/include" \
  "$START_XTENSA" "$WORK/esp32.c" "$LIBC_XTENSA" -e _start -o "$WORK/esp32.exe"
"$RUN" "$WORK/esp32.exe"
