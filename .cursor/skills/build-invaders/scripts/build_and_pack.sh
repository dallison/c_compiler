#!/usr/bin/env bash
# Build Master Invaders (+ keep Snake) and pack ~/bbc_micro/games.ssd.
set -euo pipefail

ROOT="${ROOT:-$(cd "$(dirname "$0")/../../../.." && pwd)}"
SUP="$ROOT/6502 support"
WORK="$ROOT/games/build"
DAVECC="${DAVECC:-$ROOT/bazel-bin/davecc}"
SSD="${SSD:-$HOME/bbc_micro/games.ssd}"
TARGET=6502-acorn-bbc-davecc

if [[ ! -x "$DAVECC" ]]; then
  echo "missing davecc at $DAVECC — run: cd \"$ROOT\" && bazel build //:davecc" >&2
  exit 1
fi

mkdir -p "$WORK"
export DAVECC_LIB_DIR="${DAVECC_LIB_DIR:-$ROOT/bazel-bin/libc}"
export DAVECC_INCLUDE_DIR="${DAVECC_INCLUDE_DIR:-$ROOT/libc/include}"

asm() {
  "$DAVECC" -target "$TARGET" -c "$1" -o "$2"
}

asm "$SUP/bbc_vdu.s" "$WORK/bbc_vdu.o"
asm "$SUP/screen1.s" "$WORK/screen1.o"
asm "$SUP/push.s" "$WORK/push.o"
asm "$ROOT/games/sprites.s" "$WORK/sprites.o"
asm "$ROOT/games/invaders_fx.s" "$WORK/invaders_fx.o"
asm "$ROOT/games/snake_fx.s" "$WORK/snake_fx.o"
asm "$ROOT/games/asteroids_fx.s" "$WORK/asteroids_fx.o"

build_game() {
  local src="$1" fx="$2" out="$3"
  "$DAVECC" -target "$TARGET" -std=c++20 -fno-exceptions -Os \
    -ffunction-sections -Wl,--gc-sections \
    -Wl,-T -Wl,"$SUP/bbc_mode2_e00.ld" \
    "$ROOT/games/$src" \
    "$WORK/bbc_vdu.o" "$WORK/screen1.o" "$WORK/sprites.o" "$WORK/$fx" \
    "$WORK/push.o" \
    -o "$WORK/$out"
}

build_game invaders.cc invaders_fx.o invaders.elf
build_game snake.cc snake_fx.o snake.elf
build_game asteroids.cc asteroids_fx.o asteroids.elf

check_elf() {
  local elf="$1" name="$2"
  python3 - <<PY
import sys
sys.path.insert(0, r"$SUP")
from elf2ssd import read_elf
_, load, img = read_elf(r"$elf")
end = load + len(img)
room = 0x7C00 - end
print(f"$name load=0x{load:x} end=0x{end:x} room_7c00={room}")
if load != 0xE00:
    raise SystemExit("expected load address 0xE00 (bbc_mode2_e00.ld)")
if end > 0x7C00:
    raise SystemExit("$name overlaps soft stack at &7C00")
if room < 512:
    print(f"warning: $name only {room} bytes under &7C00", file=sys.stderr)
PY
}

check_elf "$WORK/invaders.elf" INVADER
check_elf "$WORK/snake.elf" SNAKE
check_elf "$WORK/asteroids.elf" ASTERO

# Blitter/sprite symbols that survived --gc-sections must stay below &3000
# (readable while the shadow screen is mapped). Unused per-sprite sections
# are dropped, so do not require every sprites.o symbol to appear.
if command -v nm >/dev/null 2>&1; then
  for elf in invaders snake asteroids; do
    case $elf in
      invaders) objs=("$WORK/screen1.o" "$WORK/sprites.o" "$WORK/invaders_fx.o") ;;
      snake) objs=("$WORK/screen1.o" "$WORK/sprites.o" "$WORK/snake_fx.o") ;;
      asteroids) objs=("$WORK/screen1.o" "$WORK/sprites.o" "$WORK/asteroids_fx.o") ;;
    esac
    nm "${objs[@]}" |
      awk 'NF == 3 && $2 ~ /[TDB]/ {print $3}' | sort -u >"$WORK/low_syms_$elf.txt"
    nm "$WORK/$elf.elf" | python3 -c '
import sys
want = set(open(sys.argv[1]).read().split())
found = {}
for line in sys.stdin:
    parts = line.split()
    if len(parts) == 3 and parts[2] in want:
        found[parts[2]] = int(parts[0], 16)
if not found:
    print(sys.argv[2] + " has no low blitter/sprite symbols", file=sys.stderr)
    sys.exit(1)
high = max(found, key=found.get)
print(f"{sys.argv[2]} highest low symbol {high}=0x{found[high]:x} ({len(found)} kept)")
over = sorted(n for n, a in found.items() if a >= 0x3000)
if over:
    print(sys.argv[2] + " at or above &3000: " + " ".join(over), file=sys.stderr)
    sys.exit(1)
' "$WORK/low_syms_$elf.txt" "$elf"
  done
fi

python3 "$SUP/elf2ssd.py" "$SSD" \
  "SNAKE=$WORK/snake.elf" "INVADER=$WORK/invaders.elf" \
  "ASTERO=$WORK/asteroids.elf"

# DFS *RUN loads the reloc wrapper at PAGE (&1F00). Mode-7 HIMEM is &7C00;
# past that MOS returns "Bad program" before the reloc stub can run.

python3 - <<PY
# Confirm catalog entries fit under mode-7 HIMEM (&7C00).
ssd = open(r"$SSD", "rb").read()
himem = 0x7C00
nfiles = ssd[0x105] // 8
bad = []
for i in range(nfiles):
    raw = ssd[8 + i * 8 : 16 + i * 8]
    name = raw[:7].decode("ascii", "replace").rstrip()
    if name.startswith("!"):
        continue
    meta = ssd[0x108 + i * 8 : 0x110 + i * 8]
    load = meta[0] | (meta[1] << 8) | ((meta[6] & 0x0C) << 14)
    length = meta[4] | (meta[5] << 8) | ((meta[6] & 0x30) << 12)
    end = load + length
    print(f"{name} DFS load={load:#x} len={length} end={end:#x} room_7c00={himem - end}")
    if end > himem:
        bad.append(name)
if bad:
    raise SystemExit("DFS image past HIMEM &7C00 (Bad program): " + ", ".join(bad))
PY

echo "packed $SSD"
echo "On the emulator: *CAT then *RUN INVADER / SNAKE / ASTERO."
