#!/bin/bash
set -euo pipefail

if [ "$#" -ne 3 ]; then
  echo "usage: $0 <davecc> <pcode_interpreter> <libc_pcode>" >&2
  exit 2
fi

davecc=$1
pcode=$2
libc=$3

if [ -f "libc/include/stdio.h" ]; then
  :
elif [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  cd "${TEST_SRCDIR}/${TEST_WORKSPACE}"
else
  cd "$(dirname "$0")/.."
fi

work=$(mktemp -d "${TMPDIR:-/tmp}/pcode_runtime_smoke.XXXXXX")
trap 'rm -rf "$work"' EXIT

cat > "$work/printf_smoke.c" <<'SRC'
#include <stdio.h>

int main(void) {
  printf("hello pcode\n");
  return 0;
}
SRC

"$davecc" -target pcode -static -Wl,-e -Wl,main \
  "$work/printf_smoke.c" -isystem libc/include \
  -o "$work/printf_smoke.exe" "$libc"

"$pcode" "$work/printf_smoke.exe" > "$work/output.txt"
printf 'hello pcode\n' > "$work/expected.txt"
cmp "$work/expected.txt" "$work/output.txt"

cat > "$work/setjmp_smoke.c" <<'SRC'
#include <setjmp.h>

static int secondary_completed;

static void jump_through_frames(jmp_buf env, int depth) {
  volatile int marker = depth + 1;
  if (depth != 0) {
    jump_through_frames(env, depth - 1);
  }
  if (marker == 1) {
    longjmp(env, 77);
  }
}

int main(void) {
  jmp_buf primary;
  jmp_buf secondary;
  int value = setjmp(primary);
  if (value == 0) {
    longjmp(primary, 0);
  }
  if (value != 1) return 1;

  value = setjmp(primary);
  if (value == 0) {
    jump_through_frames(primary, 4);
  }
  if (value != 77) return 2;

  value = setjmp(primary);
  if (value == 0) {
    int secondary_value = setjmp(secondary);
    if (secondary_value == 0) {
      longjmp(secondary, 19);
    }
    if (secondary_value != 19) return 3;
    secondary_completed = 1;
    longjmp(primary, -7);
  }
  if (value != -7) return 4;
  if (!secondary_completed) return 5;
  return 0;
}
SRC

"$davecc" -target pcode -static -Wl,-e -Wl,main \
  "$work/setjmp_smoke.c" -isystem libc/include \
  -o "$work/setjmp_smoke.exe" "$libc"

"$pcode" "$work/setjmp_smoke.exe"

cat > "$work/exception_cleanup.cpp" <<'SRC'
int destruction_order;

struct Guard {
  int id;
  ~Guard() { destruction_order = destruction_order * 10 + id; }
};

static void fail() {
  Guard first{1};
  Guard second{2};
  throw 42;
}

int main() {
  try {
    fail();
  } catch (const int& value) {
    return value == 42 && destruction_order == 21 ? 0 : 1;
  }
  return 2;
}
SRC

"$davecc" -target pcode -static -std=c++20 -Wl,-e -Wl,main \
  "$work/exception_cleanup.cpp" -isystem libc/include \
  -o "$work/exception_cleanup.exe" "$libc"

"$pcode" "$work/exception_cleanup.exe"
