#!/bin/bash
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
X86_64="$ROOT/$2"
AARCH64="$ROOT/$3"
ARM="$ROOT/$4"
RISCV="$ROOT/$5"
PCODE="$ROOT/$6"
LIBC_X86_64="$ROOT/$7"
LIBC_AARCH64="$ROOT/$8"
LIBC_ARM="$ROOT/$9"
LIBC_RISCV="$ROOT/${10}"
LIBC_PCODE="$ROOT/${11}"

WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/multiarch-argv.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

cat > "$WORK/argv.c" <<'SRC'
#include <string.h>

int main(int argc, char** argv) {
  if (argc != 4 || argv == 0 || argv[4] != 0) {
    return 10;
  }
  if (argv[0] == 0 || argv[0][0] == '\0') {
    return 11;
  }
  if (strcmp(argv[1], "alpha") != 0 || strcmp(argv[2], "-d") != 0 ||
      strcmp(argv[3], "beta") != 0) {
    return 12;
  }
  argv[3][0] = 'B';
  return strcmp(argv[3], "Beta") == 0 ? 0 : 13;
}
SRC

run_case() {
  local target=$1
  local runner=$2
  local libc=$3
  local interpret=$4
  local exe="$WORK/$target.exe"

  "$DAVECC" -target "$target" -O0 -static -isystem "$ROOT/libc/include" \
    "$WORK/argv.c" "$libc" -o "$exe"
  if [[ "$interpret" == yes ]]; then
    "$runner" -i "$exe" alpha -d beta
  else
    "$runner" "$exe" alpha -d beta
  fi
}

run_case x86_64 "$X86_64" "$LIBC_X86_64" yes
run_case aarch64 "$AARCH64" "$LIBC_AARCH64" yes
run_case arm "$ARM" "$LIBC_ARM" yes
run_case riscv "$RISCV" "$LIBC_RISCV" no
run_case pcode "$PCODE" "$LIBC_PCODE" no
