#!/usr/bin/env bash
# DSO program headers must not claim p_vaddr==0 for a writable/DYNAMIC load
# whose p_offset sits past the RX text image (loader uses bias+p_vaddr).
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 davecc" >&2
  exit 2
fi

DAVECC="$1"
WORK="${TEST_TMPDIR:-/tmp}/dso_phdr_layout"
mkdir -p "$WORK"

cat > "$WORK/lib.c" <<'SRC'
int exported_value = 42;
int Exported(int x) { return x + exported_value; }
SRC

# Script with no data MEMORY/PT_LOAD — previously left data_segment.alignment
# uninitialized/0 and collapsed dynamic_start to VA 0.
cat > "$WORK/one_region.ld" <<'EOF'
PHDRS
{
  text PT_LOAD FLAGS(5);
  dynamic PT_DYNAMIC FLAGS(4);
}
MEMORY
{
  vdso (rx) : ORIGIN = 0x80404000, LENGTH = 0x10000
}
SECTIONS
{
  .text : { *(.text* .rodata*) } > vdso :text
  .dynamic : { *(.dynamic) } > vdso :text :dynamic
  /DISCARD/ : { *(.eh_frame* .comment .note* .data* .bss* .got* .plt*) }
}
EOF

"$DAVECC" -target aarch64-unknown-linux-davecc -shared -fPIC -nostdlib -nostdinc -O2 \
  -c "$WORK/lib.c" -o "$WORK/lib.o"
"$DAVECC" -target aarch64-unknown-linux-davecc -shared -nostdlib -nostdinc \
  "$WORK/lib.o" -o "$WORK/one_region.so" \
  -Wl,-T -Wl,"$WORK/one_region.ld"

python3 - "$WORK/one_region.so" <<'PY'
import struct, sys
p = open(sys.argv[1], "rb").read()
def u16(o): return struct.unpack_from("<H", p, o)[0]
def u32(o): return struct.unpack_from("<I", p, o)[0]
def u64(o): return struct.unpack_from("<Q", p, o)[0]
phoff, phentsize, phnum = u64(32), u16(54), u16(56)
loads = []
for i in range(phnum):
  o = phoff + i * phentsize
  loads.append((u32(o), u32(o + 4), u64(o + 8), u64(o + 16), u64(o + 32)))
# Find RX text and any RW or DYNAMIC with non-zero offset.
text = None
for t, flags, off, va, filesz in loads:
  if t == 1 and (flags & 1) and off == 0:
    text = (off, va, filesz)
    break
if text is None:
  sys.exit("missing RX PT_LOAD at offset 0")
for t, flags, off, va, filesz in loads:
  if off == 0:
    continue
  if t in (1, 2) and va == 0:
    print("bad phdr type=%d flags=%d off=%#x vaddr=%#x filesz=%#x" %
          (t, flags, off, va, filesz), file=sys.stderr)
    sys.exit(1)
  # File offset past text must not map on top of text VA.
  if t == 1 and off >= text[2] and va < text[1] + text[2] and va == text[1]:
    print("RW load overlaps text VA", file=sys.stderr)
    sys.exit(1)
print("dso phdr layout ok")
PY
