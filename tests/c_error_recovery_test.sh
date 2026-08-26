#!/bin/bash
# Error recovery has to terminate.  Each source below contains a construct the C
# parser cannot accept, and each once made the compiler report the same
# diagnostics forever because the rejecting parser consumed no input.  The cases
# use constructs davecc does not implement; if one becomes supported, replace it
# with another unsupported construct in the same position rather than dropping
# the case.
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
  python3 -c '
import os, signal, subprocess, sys
davecc, src, log = sys.argv[1:4]
with open(log, "wb") as output:
    proc = subprocess.Popen(
        [davecc, "-target", "pcode", "-std=c17", "-fsyntax-only",
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
' "$DAVECC" "$src" "$log"
}

expect_diagnosed() {
  local name="$1"
  local source="$2"
  local pattern="${3:-error:}"
  local src="$WORK/$name.c"
  local log="$WORK/$name.out"
  printf '%s\n' "$source" >"$src"
  local status=0
  run_davecc "$src" "$log" || status=$?
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
  if ! grep -Fq "$pattern" "$log"; then
    echo "$name: expected diagnostic not found: $pattern" >&2
    head -5 "$log" | sed 's/^/  /' >&2
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

echo "c error recovery ok"
