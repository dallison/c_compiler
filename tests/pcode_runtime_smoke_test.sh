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
