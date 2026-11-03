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
WORK="${TEST_TMPDIR:-/tmp}/6502-byte-index"
mkdir -p "$WORK"

# An 8-bit index added to a pointer must not read the next zero-page
# register as its high byte.  A signed index of -1 addresses the previous
# element.
cat >"$WORK/index.c" <<'SRC'
unsigned char global_buf[8];

int fill_global(void) {
  unsigned char i;
  for (i = 0; i < 5; i++) {
    global_buf[i] = (unsigned char)(4 + i);
  }
  if (global_buf[0] != 4) return 1;
  if (global_buf[1] != 5) return 2;
  if (global_buf[4] != 8) return 3;
  if (global_buf[5] != 0) return 4;
  return 0;
}

int store_signed(signed char* base, signed char index) {
  base[index] = 12;
  return base[index];
}

int main(void) {
  signed char sbuf[8];
  unsigned char i;
  int err = fill_global();
  if (err != 0) return err;
  for (i = 0; i < 8; i++) {
    sbuf[i] = 0;
  }
  if (store_signed(sbuf + 3, -1) != 12) return 5;
  if (sbuf[2] != 12) return 6;
  if (sbuf[3] != 0) return 7;
  return 0;
}
SRC

for target in 6502 65c02; do
  for opt in -O0 -O1 -Os; do
    exe="$WORK/index-$target$opt.exe"
    "$DAVECC" -target "$target" "$opt" "$WORK/index.c" -o "$exe"
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

echo "ok 8-bit pointer indexes stay inside the addressed array"
