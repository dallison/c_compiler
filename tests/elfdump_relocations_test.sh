#!/usr/bin/env bash
# elfdump read relocation and symbol tables by casting the mapped bytes to the
# canonical (ELF64) structures, and took its file base from the decoded header
# rather than from the mapping.  Neither holds for an ELF32 file, so every
# relocation in an ARM object or executable printed as R_ARM_NONE at offset 0
# with no symbol name.
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 davecc elfdump" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
ELFDUMP="$ROOT/$2"

WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/elfdump-relocations.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

cat > reloc.c <<'SRC'
#include <stdio.h>
int pic_value = 7;
int* pic_pointer = &pic_value;
int main(void) {
  printf("%d\n", *pic_pointer);
  return 0;
}
SRC

expect_contains() {
  local description="$1"
  local dump="$2"
  local pattern="$3"
  if ! grep -q -- "$pattern" "$dump"; then
    echo "$description: expected '$pattern' in relocation dump" >&2
    cat "$dump" >&2
    exit 1
  fi
}

expect_absent() {
  local description="$1"
  local dump="$2"
  local pattern="$3"
  if grep -q -- "$pattern" "$dump"; then
    echo "$description: unexpected '$pattern' in relocation dump" >&2
    cat "$dump" >&2
    exit 1
  fi
}

# An ELF32 relocatable object.  Its relocations name symbols from a symbol
# table whose on-disk entries are narrower than the canonical ones.
"$DAVECC" -target arm -c reloc.c -o reloc-arm.o
"$ELFDUMP" -r reloc-arm.o > object.dump
expect_contains "ARM object" object.dump "pic_value"
expect_contains "ARM object" object.dump "printf"
expect_absent "ARM object" object.dump "R_ARM_NONE"

# An ELF32 executable, whose dynamic relocations live in a SHT_REL section
# with no addend field, so its entries are narrower again.
"$DAVECC" -target arm -fpic reloc.c -o reloc-arm
"$ELFDUMP" -r reloc-arm > executable.dump
expect_contains "ARM executable" executable.dump ".rel.dyn"
expect_contains "ARM executable" executable.dump "R_ARM_RELATIVE"
expect_absent "ARM executable" executable.dump "R_ARM_NONE"

# The ELF64 path shares this code, so check it still resolves symbol names.
"$DAVECC" -target aarch64 -c reloc.c -o reloc-aarch64.o
"$ELFDUMP" -r reloc-aarch64.o > object64.dump
expect_contains "aarch64 object" object64.dump "pic_value"
expect_contains "aarch64 object" object64.dump "printf"
