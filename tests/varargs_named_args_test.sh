#!/usr/bin/env bash
# A variadic function with more than one named parameter, at every optimization
# level.
#
# aarch64 and riscv reserve a save area for the argument registers the named
# parameters did not use, and both the prologue that fills it and the va_list
# va_start builds describe it by the count of named parameters in registers.
# That count was only raised for a named parameter that ended up on the stack.
# When optimizing, a named parameter stays in a register unless something takes
# its address, and the only one whose address is taken is the last, the one
# va_start names.  So the count came out as 1 however many there were, the save
# area was placed over the earlier named argument registers, and the first
# va_arg returned a named argument instead of the first variadic one.
#
# riscv's libc is built with -O1, which made fprintf print its format string as
# the first conversion's argument.
#
# x86_64 chose between the save area and the caller's stack with a comparison
# whose flags a following branch read.  Dead-code elimination, which runs at -O2
# and tracks register results, saw nothing using it and removed it.
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

WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/varargs-named.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

# Nothing here needs a library, so the failure is a return code rather than
# output, and the builtins stand in for <stdarg.h> because -nostdinc hides it.
# One, two and three named parameters, because the error grew with the count:
# the save area overlapped one more named register for each.
cat > named.c <<'SRC'
#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end

static int One(int count, ...) {
  va_list ap;
  va_start(ap, count);
  int total = 0;
  for (int i = 0; i < count; i++) total += va_arg(ap, int);
  va_end(ap);
  return total;
}

static int Two(int tag, int count, ...) {
  va_list ap;
  va_start(ap, count);
  int total = tag;
  for (int i = 0; i < count; i++) total += va_arg(ap, int);
  va_end(ap);
  return total;
}

static int Three(int tag, int other, int count, ...) {
  va_list ap;
  va_start(ap, count);
  int total = tag + other;
  for (int i = 0; i < count; i++) total += va_arg(ap, int);
  va_end(ap);
  return total;
}

// From four named parameters on, x86_64 -O2 dropped the comparison that decides
// whether va_arg reads the register save area or the caller's stack, because a
// comparison's result is the condition flags and no operand names those, so it
// looked unused.  The branch then went whichever way the last instruction to
// touch the flags implied.
static int Four(int tag, int other, int third, int count, ...) {
  va_list ap;
  va_start(ap, count);
  int total = tag + other + third;
  for (int i = 0; i < count; i++) total += va_arg(ap, int);
  va_end(ap);
  return total;
}

// Named floating-point parameters are counted separately and were undercounted
// the same way.
static int Floats(double first, double second, int count, ...) {
  va_list ap;
  va_start(ap, count);
  double total = first + second;
  for (int i = 0; i < count; i++) total += va_arg(ap, double);
  va_end(ap);
  return (int)total;
}

int main(void) {
  if (One(3, 10, 20, 30) != 60) return 1;
  if (Two(1, 3, 10, 20, 30) != 61) return 2;
  if (Three(1, 2, 3, 10, 20, 30) != 63) return 3;
  if (Four(1, 2, 3, 3, 10, 20, 30) != 66) return 4;
  if (Floats(1.0, 2.0, 3, 10.0, 20.0, 30.0) != 63) return 5;
  return 99;
}
SRC

# More named parameters than there are argument registers, so the last of them
# arrive on the stack.  ARM addressed those without allowing for the register
# save area its variadic prologue puts between the frame record and the caller's
# arguments, so the last named parameter read as the first one, and va_start
# began at the last named parameter rather than after it.
cat > spilled.c <<'SRC'
#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end

static int Five(int a, int b, int c, int d, int count, ...) {
  va_list ap;
  va_start(ap, count);
  int total = a + b + c + d;
  for (int i = 0; i < count; i++) total += va_arg(ap, int);
  va_end(ap);
  return total;
}

static int Eight(int a, int b, int c, int d, int e, int f, int g, int count,
                 ...) {
  va_list ap;
  va_start(ap, count);
  int total = a + b + c + d + e + f + g;
  for (int i = 0; i < count; i++) total += va_arg(ap, int);
  va_end(ap);
  return total;
}

int main(void) {
  if (Five(1, 2, 3, 4, 3, 10, 20, 30) != 70) return 1;
  if (Eight(1, 2, 3, 4, 5, 6, 7, 3, 10, 20, 30) != 88) return 2;
  return 99;
}
SRC

run_one() {
  local target="$1"
  local interpreter="$2"
  local source="$3"
  shift 3
  local level

  for level in "$@"; do
    "$DAVECC" -target "$target" -nostdinc -nostdlib "$level" \
      -Wl,-e -Wl,main "$source" -o "${source%.c}-$target"
    local rc=0
    "$interpreter" "${source%.c}-$target" || rc=$?
    if [[ "$rc" -ne 99 ]]; then
      echo "$target $level $source returned $rc; expected 99" >&2
      exit 1
    fi
  done
}

run_target() {
  local target="$1"
  local interpreter="$2"

  run_one "$target" "$interpreter" named.c -O0 -O1 -O2
  run_one "$target" "$interpreter" spilled.c -O0 -O1 -O2
}

run_target aarch64 "$AARCH64"
run_target arm "$ARM"
run_target riscv "$RISCV"
run_target x86_64 "$X86_64"
run_target pcode "$PCODE"
