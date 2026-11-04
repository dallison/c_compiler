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
WORK="${TEST_TMPDIR:-/tmp}/6502-variable-shift"
mkdir -p "$WORK"

# A shift by a count held in a variable must still copy its operand to the
# result when the count turns out to be zero.  Each call follows one that
# leaves a different value in the result registers, so a skipped copy shows.
cat >"$WORK/shift.c" <<'SRC'
typedef unsigned _BitInt(8) u8;
typedef signed _BitInt(8) s8;

__attribute__((noinline)) u8 lsr8(u8 v, u8 n) { return (u8)(v >> n); }
__attribute__((noinline)) u8 asl8(u8 v, u8 n) { return (u8)(v << n); }
__attribute__((noinline)) s8 asr8(s8 v, u8 n) { return (s8)(v >> n); }
__attribute__((noinline)) unsigned lsr16(unsigned v, unsigned char n) { return v >> n; }
__attribute__((noinline)) unsigned asl16(unsigned v, unsigned char n) { return v << n; }
__attribute__((noinline)) int asr16(int v, unsigned char n) { return v >> n; }
__attribute__((noinline)) unsigned long lsr32(unsigned long v, unsigned char n) { return v >> n; }
__attribute__((noinline)) unsigned long asl32(unsigned long v, unsigned char n) { return v << n; }
__attribute__((noinline)) long asr32(long v, unsigned char n) { return v >> n; }

// Bit 3 - p of a byte, as a sprite decoder picks a pixel's low colour bit.
__attribute__((noinline)) u8 low_bit(u8 v, u8 p) { return (u8)((v >> (3 - p)) & 1); }

static volatile unsigned char zero = 0, one = 1, three = 3, four = 4, seven = 7;

int main(void) {
  if (lsr8(0xf0, four) != 0x0f) return 1;
  if (lsr8(0xa5, zero) != 0xa5) return 2;
  if (asl8(0x0f, four) != 0xf0) return 3;
  if (asl8(0xa5, zero) != 0xa5) return 4;
  if (asr8(-128, seven) != -1) return 5;
  if (asr8(-91, zero) != -91) return 6;
  if (asr8(-91, one) != -46) return 7;
  if (lsr16(0xf000u, four) != 0x0f00u) return 8;
  if (lsr16(0xa55au, zero) != 0xa55au) return 9;
  if (asl16(0x00f0u, four) != 0x0f00u) return 10;
  if (asl16(0xa55au, zero) != 0xa55au) return 11;
  if (asr16(-32768, seven) != -256) return 12;
  if (asr16(-23206, zero) != -23206) return 13;
  if (lsr32(0xf0000000ul, four) != 0x0f000000ul) return 14;
  if (lsr32(0xa55a1234ul, zero) != 0xa55a1234ul) return 15;
  if (asl32(0x0000f000ul, four) != 0x000f0000ul) return 16;
  if (asl32(0xa55a1234ul, zero) != 0xa55a1234ul) return 17;
  if (asr32(-2147483647l - 1, seven) != -16777216l) return 18;
  if (asr32(-1520823756l, zero) != -1520823756l) return 19;
  if (low_bit(0x08, zero) != 1) return 20;
  if (low_bit(0x01, three) != 1) return 21;
  if (low_bit(0xfe, three) != 0) return 22;
  return 0;
}
SRC

for target in 6502 65c02; do
  for opt in -O0 -O1 -Os; do
    exe="$WORK/shift-$target$opt.exe"
    "$DAVECC" -target "$target" -std=c23 "$opt" "$WORK/shift.c" -o "$exe"
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

echo "ok shifts by a variable count copy their operand when the count is zero"
