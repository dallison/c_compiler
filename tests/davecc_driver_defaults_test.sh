#!/bin/bash
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
LIBC="$ROOT/$3"
ARM_INTERPRETER="$ROOT/$4"

WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/davecc-defaults.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

cat > hello.cc <<'SRC'
#include <iostream>

int main() {
  std::cout << "driver-defaults-ok\n";
}
SRC

# A bare hosted link must find DaveCC's headers and target libc, and must use
# main rather than the linker's _start default.
"$DAVECC" -target aarch64 hello.cc -o hello
output="$("$INTERPRETER" -i hello)"
if [[ "$output" != "driver-defaults-ok" ]]; then
  echo "unexpected dynamic program output: $output" >&2
  exit 1
fi

# ARM uses translated guest addresses and lazy PLT binding. Exercise a hosted
# C++ program with enough imports to verify GOT-slot selection and argument
# preservation in its resolver.
"$DAVECC" -target arm hello.cc -o hello.arm
output="$("$ARM_INTERPRETER" hello.arm)"
if [[ "$output" != "driver-defaults-ok" ]]; then
  echo "unexpected ARM dynamic program output: $output" >&2
  exit 1
fi

# Static linkage remains an explicit policy choice. It produces an executable
# that can be run by the in-tree interpreter.
"$DAVECC" -target aarch64 -static hello.cc -o hello.static
output="$("$INTERPRETER" -i hello.static)"
if [[ "$output" != "driver-defaults-ok" ]]; then
  echo "unexpected static program output: $output" >&2
  exit 1
fi

# Passing the target libc explicitly must not cause the driver to add it twice.
"$DAVECC" -target aarch64 -static hello.cc "$LIBC" -o hello.explicit-libc

# Host compiler environment variables must not affect DaveCC's system headers.
mkdir fake-host-includes
printf '%s\n' '#error host iostream must not be used' \
  > fake-host-includes/iostream
CPATH="$WORK/fake-host-includes" \
CPLUS_INCLUDE_PATH="$WORK/fake-host-includes" \
SDKROOT="$WORK/fake-host-includes" \
  "$DAVECC" -target aarch64 -S hello.cc -o hello.host-isolated.s

# Compile-only and -nostdlib modes must not require a target archive.
mkdir empty-lib
DAVECC_LIB_DIR="$WORK/empty-lib" \
  "$DAVECC" -target aarch64 -S hello.cc -o hello.compile-only.s

# Without -o, -S must report success and use the source basename with .s.
cat > assembly-default.cc <<'SRC'
int answer() {
  return 42;
}
SRC
"$DAVECC" -target 65c02 -S assembly-default.cc
if [[ ! -s assembly-default.s ]]; then
  echo "-S did not create its default assembly output" >&2
  exit 1
fi

cat > freestanding.c <<'SRC'
int main(void) {
  return 0;
}
SRC
DAVECC_LIB_DIR="$WORK/empty-lib" \
  "$DAVECC" -target aarch64 -static -nostdlib \
    -Wl,-e -Wl,main freestanding.c -o freestanding

# -nostdinc must remove both the resolved DaveCC path and the compiled-in
# CWD-relative fallback.
if "$DAVECC" -target aarch64 -nostdinc -S hello.cc \
    -o hello.nostdinc.s >nostdinc.out 2>&1; then
  echo "-nostdinc unexpectedly found <iostream>" >&2
  exit 1
fi
