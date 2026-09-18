#!/bin/bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 davecc elfdump" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
ELFDUMP="$ROOT/$2"
WORK="${TEST_TMPDIR:-/tmp}/asm-data-expr"
mkdir -p "$WORK"

cat >"$WORK/data.s" <<'SRC'
.section ".rodata", "a", @progbits
.global table
table:
	.byte 6*1, 12*2, 6*4, 3*8, 3*4
	.hword 3*10, 1+2*3
	.word 4*5
SRC

"$DAVECC" -target 65c02 -nostdinc -c "$WORK/data.s" -o "$WORK/data.o"
python3 - "$ELFDUMP" "$WORK/data.o" <<'PY'
import re
import subprocess
import sys

elfdump, obj = sys.argv[1], sys.argv[2]
listing = subprocess.check_output([elfdump, "-S", obj], text=True)
sections = {}
for line in listing.splitlines():
    m = re.search(r"^\s*(\d+):\s+(\S+)", line)
    if m:
        sections[m.group(2)] = int(m.group(1))
if ".rodata" not in sections:
    raise SystemExit("missing .rodata")
dump = subprocess.check_output(
    [elfdump, "-x", str(sections[".rodata"]), obj], text=True)
hexbytes = [b.upper() for b in re.findall(r"\b([0-9A-Fa-f]{2})\b", dump)]
# Expect: 06 18 18 18 0c | 1e 00 07 00 | 14 00 00 00
expect = ["06", "18", "18", "18", "0C", "1E", "00", "07", "00", "14", "00", "00", "00"]
if hexbytes[: len(expect)] != expect:
    raise SystemExit(f"bytes {hexbytes[:len(expect)]} != {expect}\n{dump}")
print("ok", " ".join(expect))
PY
