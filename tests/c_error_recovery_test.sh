#!/bin/bash
# Error recovery has to terminate normally.  Each source below contains a
# construct the compiler rejects, and each once made it spin or crash after the
# diagnostic.  The cases use constructs davecc does not implement; if one becomes
# supported, replace it with another unsupported construct in the same position
# rather than dropping the case.
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <davecc>" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/c-error-recovery.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

# The error limit would otherwise stop a spinning parser and hide the defect, so
# it is disabled here and a spin is caught by the timeout instead.  The compiler
# writes straight to the log: a spin produces output far faster than it can be
# usefully buffered.
run_davecc() {
  local src="$1"
  local log="$2"
  local std="$3"
  python3 -c '
import os, signal, subprocess, sys
davecc, src, log, std = sys.argv[1:5]
with open(log, "wb") as output:
    proc = subprocess.Popen(
        [davecc, "-target", "pcode", "-std=" + std, "-fsyntax-only",
         "-error-limit=0", src],
        stdout=output,
        stderr=subprocess.STDOUT,
        start_new_session=True,
    )
    try:
        raise SystemExit(proc.wait(timeout=10))
    except subprocess.TimeoutExpired:
        os.killpg(proc.pid, signal.SIGKILL)
        proc.wait()
        raise SystemExit(124)
' "$DAVECC" "$src" "$log" "$std"
}

# Compile and require an ordinary exit: no hang, no signal.  A rejected
# construct leaves the compiler holding partly built state, so what follows it
# must still be processed without crashing.
expect_terminates() {
  local name="$1"
  local source="$2"
  local std="${3:-c17}"
  local src="$WORK/$name.c"
  local log="$WORK/$name.out"
  printf '%s\n' "$source" >"$src"
  local status=0
  run_davecc "$src" "$log" "$std" || status=$?
  if [[ "$status" -eq 124 ]]; then
    echo "$name: compiler did not terminate" >&2
    head -5 "$log" | sed 's/^/  /' >&2
    exit 1
  fi
  if [[ "$status" -lt 0 || "$status" -gt 1 ]]; then
    echo "$name: compiler exited abnormally with status $status" >&2
    head -5 "$log" | sed 's/^/  /' >&2
    exit 1
  fi
}

expect_diagnosed() {
  local name="$1"
  local source="$2"
  local pattern="${3:-error:}"
  local std="${4:-c17}"
  expect_terminates "$name" "$source" "$std"
  if ! grep -Fq "$pattern" "$WORK/$name.out"; then
    echo "$name: expected diagnostic not found: $pattern" >&2
    head -5 "$WORK/$name.out" | sed 's/^/  /' >&2
    exit 1
  fi
}

# _Complex and _Imaginary are classified as type tokens but name no implemented
# type.  Unless the type parser consumes them, recovery that stops at a type
# token cannot move past one, and the enclosing parse repeats forever.  The
# positions below spun in the translation-unit loop, the struct member loop and
# the old-style argument declaration list respectively.
complex_unsupported="'_Complex' types are not supported"
expect_diagnosed complex_declaration_specifier \
  'double _Complex f(void);
int main(void) { return 0; }' \
  "$complex_unsupported"

expect_diagnosed complex_struct_member \
  'struct S { _Complex float d; };' \
  "$complex_unsupported"

expect_diagnosed imaginary_declaration_specifier \
  '_Imaginary double x;' \
  "'_Imaginary' types are not supported"

# The GNU spelling of the same specifier.
expect_diagnosed gnu_complex_declaration_specifier \
  '__complex__ double foo (__complex__ double x, __complex__ double y)
{
  return x / y;
}' \
  "$complex_unsupported"

# An old-style argument declaration list that reaches end of input before the
# function body: that loop only stopped at the '{' of the body.
expect_diagnosed old_style_arguments_at_end_of_input \
  'int old(a)
int a;'

# Reading a member of an aggregate constexpr object materializes the subobject
# for that member and stores it in the containing object's slot.  The containing
# object here belongs to the symbol and outlives the evaluation, so a subobject
# owned by the evaluation context left the slot dangling and the second read
# followed it.  Two reads are needed: the first frees the subobject, the second
# reuses the slot.  Whether the initializer is accepted does not matter, only
# that the reads do not crash.
expect_terminates constexpr_subobject_read_twice \
  'struct inner { void *p; };
struct outer { struct inner x; };
constexpr struct outer v = { };
static_assert (v.x.p == 0);
static_assert (v.x.p == 0);' \
  c23

# A file-scope assignment is parsed as an implicit-int declaration, which here
# redeclares an object of a different type.  The initializer is encoded for the
# storage of the object being initialized, and the invalid expression beside it
# has an unrelated type, which used to fail an assertion in the encoder.
expect_diagnosed static_initializer_after_type_mismatch \
  'double res;
res = .;'

echo "c error recovery ok"
