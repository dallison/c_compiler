#!/bin/bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 davecc elfdump" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
ELFDUMP="$ROOT/$2"
WORK="${TEST_TMPDIR:-/tmp}/6502-index-y"
mkdir -p "$WORK"

# LDX and STX take their Y index in the slots other group-2 opcodes use for
# X. The assembler used group 1's abs,Y bits, so LDX abs,Y came out as BA
# (TSX) followed by LDX zp.
cat >"$WORK/index_y.s" <<'SRC'
.set ZP 0x80
.text
start:
  ldx $1234,y
  ldx $20,y
  stx $20,y
  ldx ZP, Y
  stx ZP, Y
  ldx table, Y
  lda table, Y
table:
  rts
SRC

"$DAVECC" -target 65c02 -nostdinc -c "$WORK/index_y.s" -o "$WORK/index_y.o"
python3 - "$ELFDUMP" "$WORK/index_y.o" <<'PY'
import re, subprocess, sys
elfdump, obj = sys.argv[1], sys.argv[2]
listing = subprocess.check_output([elfdump, "-S", obj], text=True)
number = None
for line in listing.splitlines():
    m = re.search(r"^\s*(\d+):\s+(\S+)", line)
    if m and m.group(2) == ".text":
        number = m.group(1)
if number is None:
    raise SystemExit("missing .text")
dump = subprocess.check_output([elfdump, "-x", number, obj], text=True)
got = " ".join(" ".join(re.findall(r"\b[0-9A-F]{2}\b", line[:48]))
               for line in dump.splitlines()).split()
expect = ("BE 34 12 B6 20 96 20 B6 80 96 80 BE 00 00 B9 00 00 60").split()
if got != expect:
    raise SystemExit(f"got    {' '.join(got)}\nexpect {' '.join(expect)}")
print("ok")
PY

# STX has no abs,Y form.
cat >"$WORK/stx_abs_y.s" <<'SRC'
.text
  stx $1234,y
SRC
if "$DAVECC" -target 65c02 -nostdinc -c "$WORK/stx_abs_y.s" \
    -o "$WORK/stx_abs_y.o" 2>"$WORK/stx_abs_y.err"; then
  echo "stx abs,Y assembled" >&2
  exit 1
fi
grep -q "STX has no abs,Y mode" "$WORK/stx_abs_y.err"
