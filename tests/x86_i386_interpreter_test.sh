#!/usr/bin/env bash
# Compile a small i386 program with DaveCC libc and run it under the x86_64
# interpreter (ELF32 load + IA-32 decode).
set -euo pipefail

if [[ $# -lt 3 ]]; then
  echo "usage: $0 davecc libc interpreter" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-.}"
if [[ "$1" = /* ]]; then
  DAVECC="$1"
  LIBC="$2"
  INTERP="$3"
else
  DAVECC="$ROOT/$1"
  LIBC="$ROOT/$2"
  INTERP="$ROOT/$3"
fi
INCLUDE="${ROOT}/libc/include"
if [[ ! -d "$INCLUDE" ]]; then
  INCLUDE="$(pwd)/libc/include"
fi

WORK="${TEST_TMPDIR:-/tmp}/x86-i386-interpreter"
mkdir -p "$WORK"

cat >"$WORK/program.c" <<'EOF'
#include <unistd.h>

int add(int a, int b) { return a + b; }

int main(int argc, char** argv) {
  (void)argv;
  if (add(3, 4) != 7) {
    return 2;
  }
  if (argc < 1) {
    return 3;
  }
  if (write(1, "ok\n", 3) != 3) {
    return 4;
  }
  return 0;
}
EOF

"$DAVECC" -target x86 -static -nostdlib -nostdinc -isystem "$INCLUDE" \
  -Wl,-e -Wl,main "$WORK/program.c" "$LIBC" -o "$WORK/program"
file "$WORK/program" | grep -q 'ELF 32-bit.*Intel 80386'

set +e
"$INTERP" -i "$WORK/program"
status=$?
set -e
if [[ "$status" != "0" ]]; then
  echo "i386 interpreter run failed: exit=$status" >&2
  exit 1
fi

echo "x86 i386 interpreter test passed"
