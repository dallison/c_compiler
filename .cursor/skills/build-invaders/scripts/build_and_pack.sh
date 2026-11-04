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

"$DAVECC" -target "$TARGET" -std=c++20 -fno-exceptions -Os \
  -ffunction-sections -Wl,--gc-sections \
  -Wl,-T -Wl,"$SUP/bbc_mode2_e00.ld" \
  "$ROOT/games/invaders.cc" \
  "$WORK/bbc_vdu.o" "$WORK/screen1.o" "$WORK/sprites.o" "$WORK/invaders_fx.o" \
  "$WORK/push.o" \
  -o "$WORK/invaders.elf"

python3 - <<PY
import sys
sys.path.insert(0, r"$SUP")
from elf2ssd import read_elf
_, load, img = read_elf(r"$WORK/invaders.elf")
end = load + len(img)
room = 0x7C00 - end
print(f"INVADER load=0x{load:x} end=0x{end:x} room_7c00={room}")
if load != 0xE00:
    raise SystemExit("expected load address 0xE00 (bbc_mode2_e00.ld)")
if end > 0x7C00:
    raise SystemExit("image overlaps soft stack at &7C00")
if room < 512:
    print(f"warning: only {room} bytes under &7C00", file=sys.stderr)
PY

# Everything screen1.s, sprites.s and invaders_fx.s define is read or run
# while the shadow screen is mapped at &3000, so it must sit below &3000.
if command -v nm >/dev/null 2>&1; then
  nm "$WORK/screen1.o" "$WORK/sprites.o" "$WORK/invaders_fx.o" |
    awk 'NF == 3 && $2 ~ /[TDB]/ {print $3}' | sort -u >"$WORK/low_syms.txt"
  nm "$WORK/invaders.elf" | python3 -c '
import sys
want = set(open(sys.argv[1]).read().split())
found = {}
for line in sys.stdin:
    parts = line.split()
    if len(parts) == 3 and parts[2] in want:
        found[parts[2]] = int(parts[0], 16)
missing = sorted(want - set(found))
if missing:
    print("missing low symbols: " + " ".join(missing), file=sys.stderr)
    sys.exit(1)
high = max(found, key=found.get)
print(f"highest low symbol {high}=0x{found[high]:x}")
over = sorted(n for n, a in found.items() if a >= 0x3000)
if over:
    print("at or above &3000: " + " ".join(over), file=sys.stderr)
    sys.exit(1)
' "$WORK/low_syms.txt"
fi

if [[ -f "$WORK/snake.elf" ]]; then
  python3 "$SUP/elf2ssd.py" "$SSD" \
    "SNAKE=$WORK/snake.elf" "INVADER=$WORK/invaders.elf"
else
  echo "warning: $WORK/snake.elf missing — packing INVADER only" >&2
  python3 "$SUP/elf2ssd.py" "$SSD" "INVADER=$WORK/invaders.elf"
fi

echo "packed $SSD"
echo "On the emulator: *CAT then *RUN INVADER (Shift-Break boots !BOOT)."
