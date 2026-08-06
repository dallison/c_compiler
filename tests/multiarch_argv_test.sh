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
INTERP_65="$ROOT/${12}"
ROM_65="$ROOT/${13}"
LIBC_65="$ROOT/${14}"

WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/multiarch-argv.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

cat > "$WORK/argv.c" <<'SRC'
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
  if (argc != 4 || argv == 0 || argv[4] != 0) {
    return 10;
  }
  if (argv[0] == 0 || argv[0][0] == '\0') {
    return 11;
  }
  if (strcmp(argv[1], "10") != 0 || strcmp(argv[2], "-d") != 0 ||
      strcmp(argv[3], "beta") != 0) {
    return 12;
  }
  if (atoi(argv[1]) != 10) {
    return 14;
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
  local linkage=$5
  local exe="$WORK/$target-$linkage.exe"
  local link_flags=()
  if [[ "$linkage" == static ]]; then
    link_flags=(-static)
  fi

  "$DAVECC" -target "$target" -O0 "${link_flags[@]}" \
    -isystem "$ROOT/libc/include" \
    "$WORK/argv.c" "$libc" -o "$exe"
  if [[ "$interpret" == yes ]]; then
    "$runner" -i "$exe" 10 -d beta
  else
    "$runner" "$exe" 10 -d beta
  fi
}

run_6502_case() {
  local target=$1
  local exe="$WORK/$target.exe"

  "$DAVECC" -target "$target" -O0 -static -isystem "$ROOT/libc/include" \
    "$WORK/argv.c" "$LIBC_65" -o "$exe"
  "$INTERP_65" -rom "$ROM_65" "$exe" 10 -d beta
}

cat > "$WORK/embedded_cpp.cpp" <<'SRC'
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  if (argc != 2 || atoi(argv[1]) != 2) {
    return 20;
  }
  std::vector<std::string> values;
  std::map<std::string, int> lookup;
  for (int i = 0; i < 2; ++i) {
    values.push_back("value " + std::to_string(i));
    lookup["key " + std::to_string(i)] = i + 3;
  }
  if (values.size() != 2 || values[0] != "value 0" ||
      values[1] != "value 1") {
    return 21;
  }
  return lookup["key 0"] == 3 && lookup["key 1"] == 4 ? 0 : 22;
}
SRC

run_6502_cpp_case() {
  local target=$1
  local exe="$WORK/$target-cpp.exe"

  "$DAVECC" -target "$target" -O0 -static -isystem "$ROOT/libc/include" \
    "$WORK/embedded_cpp.cpp" "$LIBC_65" -o "$exe"
  "$INTERP_65" -rom "$ROM_65" "$exe" 2
}

run_case x86_64 "$X86_64" "$LIBC_X86_64" yes static
run_case aarch64 "$AARCH64" "$LIBC_AARCH64" yes static
run_case arm "$ARM" "$LIBC_ARM" yes static
run_case riscv "$RISCV" "$LIBC_RISCV" no static
run_case pcode "$PCODE" "$LIBC_PCODE" no static

# Dynamic interpreters enter main directly, so they must construct the same
# guest argv representation as their static counterparts.
run_case x86_64 "$X86_64" "$LIBC_X86_64" yes dynamic
run_case aarch64 "$AARCH64" "$LIBC_AARCH64" yes dynamic
run_case arm "$ARM" "$LIBC_ARM" yes dynamic

run_6502_case 6502
run_6502_case 65c02
run_6502_cpp_case 6502
run_6502_cpp_case 65c02
