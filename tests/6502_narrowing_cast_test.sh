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
WORK="${TEST_TMPDIR:-/tmp}/6502-narrowing-cast"
mkdir -p "$WORK"

# A narrowing conversion keeps the low bytes of its operand (the value
# modulo the destination width), for signed and unsigned destinations
# alike.  The 6502 LowerSignExtend shorten path used to OR the *source*
# top byte's sign bit into the destination's top byte instead, so e.g.
# (u8)(0 - 255) produced 0x81 rather than 0x01 and from_int(-255) did
# not produce 1.
cat >"$WORK/narrow.c" <<'SRC'
typedef unsigned _BitInt(8) u8;
typedef signed _BitInt(8) s8;
typedef signed _BitInt(16) s16;
typedef unsigned _BitInt(16) u16;
typedef signed _BitInt(32) s32;

__attribute__((noinline)) u8 negmag(u8 uv) { return (u8)(0 - uv); }
__attribute__((noinline)) u8 u8_from_int(int v) { return (u8)v; }
__attribute__((noinline)) s8 s8_from_int(int v) { return (s8)v; }
__attribute__((noinline)) s8 s8_from_long(long v) { return (s8)v; }
__attribute__((noinline)) s16 s16_from_int(int v) { return (s16)v; }
__attribute__((noinline)) s16 s16_from_long(long v) { return (s16)v; }
__attribute__((noinline)) u16 u16_from_long(long v) { return (u16)v; }
__attribute__((noinline)) s8 s8_from_s32(s32 v) { return (s8)v; }
__attribute__((noinline)) int widen_s8(s8 v) { return v; }
__attribute__((noinline)) long widen_s16(s16 v) { return v; }

typedef unsigned _BitInt(16) gu16;
typedef signed _BitInt(8) gs8;
static gu16 ship_x;
static gs8 ship_vx;

__attribute__((noinline)) void move_ship(void) {
  if ((u8)ship_vx != 0) {
    u8 uv = (u8)ship_vx;
    if (uv < 128) {
      ship_x = (gu16)(ship_x + uv);
      if (ship_x >= 304) {
        ship_x = (gu16)(ship_x - 304);
      }
    } else if (ship_x >= (u8)(0 - uv)) {
      ship_x = (gu16)(ship_x - (u8)(0 - uv));
    } else {
      ship_x = (gu16)(ship_x + (304 - (u8)(0 - uv)));
    }
  }
}

static volatile int minus_255 = -255, minus_256 = -256, minus_129 = -129;
static volatile int minus_1 = -1, val_255 = 255, val_256 = 256;
static volatile long minus_65535 = -65535L, minus_32769 = -32769L;
static volatile long val_4660 = 0x1234L, minus_70000 = -70000L;

int main(void) {
  if (negmag(1) != 255) return 1;
  if (negmag(2) != 254) return 2;
  if (negmag(127) != 129) return 3;
  if (negmag(128) != 128) return 4;
  if (negmag(255) != 1) return 5;
  if (u8_from_int(minus_1) != 255) return 6;
  if (u8_from_int(minus_255) != 1) return 7;
  if (u8_from_int(minus_256) != 0) return 8;
  if (u8_from_int(val_255) != 255) return 9;
  if (u8_from_int(val_256) != 0) return 10;
  if (s8_from_int(minus_255) != 1) return 11;
  if (s8_from_int(minus_129) != 127) return 12;
  if (s8_from_int(minus_1) != -1) return 13;
  if (s8_from_int(val_255) != -1) return 14;
  if (s8_from_long(minus_65535) != 1) return 15;
  if (s16_from_int(minus_65535) != 1) return 16;
  if (s16_from_long(minus_32769) != 32767) return 17;
  if (s16_from_long(val_4660) != 0x1234) return 18;
  if (u16_from_long(minus_70000) != 61072) return 19;
  if (s8_from_s32(minus_255) != 1) return 20;
  if (widen_s8(-128) != -128) return 21;
  if (widen_s8(127) != 127) return 22;
  if (widen_s16(-32768) != -32768L) return 23;
  if (widen_s16(32767) != 32767L) return 24;
  // The descending unsigned compare in move_ship accesses the global
  // through a pointer; SetIndexRegDown used to load Y with the top
  // byte's index on every iteration, so ship_x >= mag compared the
  // high byte twice and took the wrap branch at x=156, mag=3.
  ship_x = 156;
  ship_vx = -3;
  move_ship();
  if (ship_x != 153) return 25;
  move_ship();
  if (ship_x != 150) return 26;
  ship_x = 2;
  move_ship();
  if (ship_x != 303) return 27;
  ship_x = 300;
  ship_vx = 3;
  move_ship();
  if (ship_x != 303) return 28;
  move_ship();
  if (ship_x != 2) return 29;
  return 0;
}
SRC

for target in 6502 65c02; do
  for opt in -O0 -O1 -Os; do
    exe="$WORK/narrow-$target$opt.exe"
    "$DAVECC" -target "$target" -std=c23 "$opt" "$WORK/narrow.c" -o "$exe"
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

echo "ok narrowing casts keep the low bytes of their operand"
