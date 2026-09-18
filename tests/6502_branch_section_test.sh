#!/bin/bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 davecc elfdump" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
ELFDUMP="$ROOT/$2"
WORK="${TEST_TMPDIR:-/tmp}/6502-branch-section"
mkdir -p "$WORK"

# Two function sections with a branch at the same section offset.  The
# assembler used to key branches by offset only, so the second beq was
# emitted as the first section's bne.
cat >"$WORK/branches.s" <<'SRC'
.section ".text.first", "ax", @progbits
first:
	bne first_done
	lda #1
	lda #2
	lda #3
	lda #4
	lda #5
first_done:
	rts

.section ".text.second", "ax", @progbits
second:
	beq second_done
	lda #1
	lda #2
	lda #3
	lda #4
	lda #5
second_done:
	rts
SRC

"$DAVECC" -target 65c02 -nostdinc -c "$WORK/branches.s" -o "$WORK/branches.o"
first=$("$ELFDUMP" -x 1 "$WORK/branches.o" | head -1)
second=$("$ELFDUMP" -x 2 "$WORK/branches.o" | head -1)
# elfdump -x uses section numbers; find the two text sections.
python3 - "$ELFDUMP" "$WORK/branches.o" <<'PY'
import subprocess, sys, re
elfdump, obj = sys.argv[1], sys.argv[2]
listing = subprocess.check_output([elfdump, "-S", obj], text=True)
sections = {}
for line in listing.splitlines():
    m = re.search(r"^\s*(\d+):\s+(\S+)", line)
    if m:
        sections[m.group(2)] = int(m.group(1))
for name in (".text.first", ".text.second"):
    if name not in sections:
        raise SystemExit(f"missing {name}")
    dump = subprocess.check_output([elfdump, "-x", str(sections[name]), obj],
                                   text=True)
    hexbytes = re.findall(r"\b([0-9A-Fa-f]{2})\b", dump.splitlines()[0])
    if not hexbytes:
        # full dump line may put hex after the address
        hexbytes = re.findall(r"[0-9A-Fa-f]{2}", dump.replace("\n", " "))
    opcode = hexbytes[0].upper()
    expect = "D0" if name.endswith("first") else "F0"
    if opcode != expect:
        raise SystemExit(f"{name} starts with {opcode}, expected {expect}\n{dump}")
    print(f"{name}: {opcode}")
PY
