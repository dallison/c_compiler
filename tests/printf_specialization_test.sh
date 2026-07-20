#!/bin/bash
set -euo pipefail

if [[ $# -ne 5 ]]; then
  echo "usage: $0 davecc 6502-interpreter rom elfdump x86-interpreter" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
ROM="$ROOT/$3"
ELFDUMP="$ROOT/$4"
X86_INTERPRETER="$ROOT/$5"
WORK="${TEST_TMPDIR:-/tmp}/printf-specialization"
mkdir -p "$WORK"

cat >"$WORK/literal.c" <<'SRC'
#include <stdio.h>
int main(void) {
  printf("literal-output\n");
  return 0;
}
SRC

cat >"$WORK/integer.c" <<'SRC'
#include <stdio.h>
int main(void) {
  printf("int=%d hex=%#x string=%s\n", -23, 42, "ok");
  return 0;
}
SRC

cat >"$WORK/profiles.c" <<'SRC'
#include <stdio.h>
void profiles(FILE* f, char* out, long v, double d) {
  fprintf(f, "%d", 1);
  sprintf(out, "%ld", v);
  snprintf(out, 32, "%f", d);
}
SRC

cat >"$WORK/long.c" <<'SRC'
#include <stdio.h>
int main(void) {
  printf("long=%ld\n", 123456L);
  return 0;
}
SRC

cat >"$WORK/ostream.cc" <<'SRC'
#include <ostream>
void output(std::ostream& stream) {
  stream << 42;
}
SRC

"$DAVECC" -target 65c02 -S "$WORK/literal.c" -o "$WORK/literal.s"
"$DAVECC" -target 65c02 -S "$WORK/integer.c" -o "$WORK/integer.s"
"$DAVECC" -target 65c02 -S "$WORK/profiles.c" -o "$WORK/profiles.s"
"$DAVECC" -target 65c02 -S "$WORK/long.c" -o "$WORK/long.s"
"$DAVECC" -target 65c02 -S "$WORK/ostream.cc" -o "$WORK/ostream.s"

grep -q 'jsr[[:space:]]*__printf_literal' "$WORK/literal.s"
grep -q 'jsr[[:space:]]*__printf_int' "$WORK/integer.s"
grep -q 'jsr[[:space:]]*__fprintf_int' "$WORK/profiles.s"
grep -q 'jsr[[:space:]]*__sprintf_long' "$WORK/profiles.s"
grep -q 'jsr[[:space:]]*__snprintf_fp' "$WORK/profiles.s"
grep -q 'jsr[[:space:]]*__printf_long' "$WORK/long.s"
grep -q 'jsr[[:space:]]*__snprintf_int' "$WORK/ostream.s"
if grep -q 'jsr[[:space:]]*snprintf' "$WORK/ostream.s"; then
  echo "integer ostream still calls generic snprintf" >&2
  exit 1
fi

"$DAVECC" -target 65c02 -fno-printf-specialize -S "$WORK/integer.c" \
  -o "$WORK/disabled.s"
grep -q 'jsr[[:space:]]*printf' "$WORK/disabled.s"

"$DAVECC" -target aarch64 -S "$WORK/integer.c" -o "$WORK/aarch64-default.s"
if grep -q '__printf_int' "$WORK/aarch64-default.s"; then
  echo "aarch64 enabled printf specialization by default" >&2
  exit 1
fi
"$DAVECC" -target aarch64 -fprintf-specialize -S "$WORK/integer.c" \
  -o "$WORK/aarch64-enabled.s"
grep -q '__printf_int' "$WORK/aarch64-enabled.s"

"$DAVECC" -target 65c02 "$WORK/literal.c" -o "$WORK/literal.exe"
"$DAVECC" -target 65c02 "$WORK/integer.c" -o "$WORK/integer.exe"
"$DAVECC" -target 65c02 "$WORK/long.c" -o "$WORK/long.exe"
"$DAVECC" -target x86_64 -static -fprintf-specialize "$WORK/integer.c" \
  -o "$WORK/integer.x86"

# The interpreter should find the support ROM next to its Bazel runfiles.
literal_output="$("$INTERPRETER" "$WORK/literal.exe")"
integer_output="$("$INTERPRETER" -rom "$ROM" "$WORK/integer.exe")"
long_output="$("$INTERPRETER" -rom "$ROM" "$WORK/long.exe")"
x86_output="$("$X86_INTERPRETER" -i "$WORK/integer.x86")"
if [[ "$literal_output" != "literal-output" ]]; then
  echo "unexpected literal printf output: $literal_output" >&2
  exit 1
fi
if [[ "$long_output" != "long=123456" ]]; then
  echo "unexpected long printf output: $long_output" >&2
  exit 1
fi
if [[ "$x86_output" != "int=-23 hex=0x2a string=ok" ]]; then
  echo "unexpected x86 specialized printf output: $x86_output" >&2
  exit 1
fi
if [[ "$integer_output" != "int=-23 hex=0x2a string=ok" ]]; then
  echo "unexpected integer printf output: $integer_output" >&2
  exit 1
fi

# Ensure the specialized integer image does not pull in the full printf object.
"$ELFDUMP" -s "$WORK/integer.exe" >"$WORK/integer.symbols"
if grep -q '__PrintFloatFormat' "$WORK/integer.symbols"; then
  echo "integer-only printf unexpectedly linked floating-point formatting" >&2
  exit 1
fi
"$ELFDUMP" -s "$WORK/long.exe" >"$WORK/long.symbols"
if grep -q '__PrintFloatFormat' "$WORK/long.symbols"; then
  echo "long-only printf unexpectedly linked floating-point formatting" >&2
  exit 1
fi
