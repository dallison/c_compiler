#!/bin/bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 davecc 6502-interpreter rom" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
ROM="$ROOT/$3"
WORK="${TEST_TMPDIR:-/tmp}/6502-nmos-flags"
mkdir -p "$WORK"

# NMOS stores a byte through a pointer with LDY #0 / STA (zp),Y.  A reload
# that only re-established N/Z for the following branch must survive.
cat >"$WORK/flags.c" <<'SRC'
__attribute__((noinline)) int is_decimal(char c) {
  return c == 'd' || c == 'i' || c == 'u';
}

int main(void) {
  static volatile char probe[] = "diuq";
  if (!is_decimal(probe[0])) return 1;
  if (!is_decimal(probe[1])) return 2;
  if (!is_decimal(probe[2])) return 3;
  if (is_decimal(probe[3])) return 4;
  return 0;
}
SRC

for target in 6502 65c02; do
  for opt in -O0 -O1 -Os; do
    exe="$WORK/flags-$target$opt.exe"
    "$DAVECC" -target "$target" "$opt" "$WORK/flags.c" -o "$exe"
    set +e
    "$INTERPRETER" -rom "$ROM" "$exe"
    got=$?
    set -e
    if [[ "$got" -ne 0 ]]; then
      echo "FAIL $target $opt: exit $got" >&2
      exit 1
    fi
  done
done

echo "ok byte compares keep their N/Z flags across NMOS indirect stores"
