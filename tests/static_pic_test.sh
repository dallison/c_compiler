#!/usr/bin/env bash
# Position-independent code loads a variable's address out of the GOT.  A fully
# static link has no loader to fill those slots, so the linker has to build the
# table and resolve it itself.  Before it did, the GOT was missing entirely and
# the displacement the code added to the PC resolved to zero, and on the targets
# whose relocation code reached for the PLT it crashed the linker outright.
set -euo pipefail

if [[ $# -ne 6 ]]; then
  echo "usage: $0 davecc aarch64 arm riscv x86_64 pcode" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
AARCH64="$ROOT/$2"
ARM="$ROOT/$3"
RISCV="$ROOT/$4"
X86_64="$ROOT/$5"
PCODE="$ROOT/$6"

WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/static-pic.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

# Several globals reached through pointers, plus a function pointer, so the GOT
# holds more than one entry and a wrong entry size or ordering shows up.
cat > pic.c <<'SRC'
#include <stdio.h>
int first = 1;
int second = 2;
int* first_pointer = &first;
int* second_pointer = &second;
static int Twice(int value) { return value * 2; }
int (*twice_pointer)(int) = Twice;
const char* message = "static-pic-ok";
int main(void) {
  printf("%s %d\n", message,
         *first_pointer + *second_pointer + twice_pointer(20));
  return 0;
}
SRC

# A hosted C++ program additionally runs static constructors from .init_array
# before main, whose entries are pointers the same GOT machinery produces.
cat > pic.cc <<'SRC'
#include <iostream>
#include <string>

struct Counter {
  Counter() : value(43) {}
  int value;
};

static Counter counter;
static Counter* counter_pointer = &counter;

int main() {
  std::string label = "static-pic-cc-ok";
  std::cout << label << " " << counter_pointer->value << "\n";
}
SRC

run_one() {
  local target="$1"
  local interpreter="$2"
  local output

  "$DAVECC" -target "$target" -fpic -static pic.c -o "pic-$target"
  output="$("$interpreter" "pic-$target")"
  if [[ "$output" != "static-pic-ok 43" ]]; then
    echo "unexpected $target -fpic -static output: $output" >&2
    exit 1
  fi

  "$DAVECC" -target "$target" -fpic -static pic.cc -o "pic-cc-$target"
  output="$("$interpreter" "pic-cc-$target")"
  if [[ "$output" != "static-pic-cc-ok 43" ]]; then
    echo "unexpected $target -fpic -static C++ output: $output" >&2
    exit 1
  fi

  # A static link without -fpic asks for no GOT entries, and must still be
  # linked without one rather than gaining an empty table.
  "$DAVECC" -target "$target" -static pic.c -o "plain-$target"
  output="$("$interpreter" "plain-$target")"
  if [[ "$output" != "static-pic-ok 43" ]]; then
    echo "unexpected $target -static output: $output" >&2
    exit 1
  fi
}

run_one aarch64 "$AARCH64"
run_one arm "$ARM"
run_one riscv "$RISCV"
run_one x86_64 "$X86_64"
run_one pcode "$PCODE"
