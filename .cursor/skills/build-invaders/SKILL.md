---
name: build-invaders
description: >-
  Build BBC Master Space Invaders with davecc, link at &E00, and pack
  INVADER onto ~/bbc_micro/games.ssd for the emulator. Use when rebuilding
  invaders, updating games.ssd, packing INVADER/SNAKE, or after editing
  games/invaders.cc, games/sprites.s, screen1.s, or bbc_mode2_e00.ld.
---

# Build Master Invaders → games.ssd

Work in the **git checkout** that has `bazel-bin/davecc` (usually
`/Users/dallison/c_compiler`), not a stale Google Drive mirror.

## Quick path

```bash
bazel build //:davecc          # if davecc missing/outdated
.cursor/skills/build-invaders/scripts/build_and_pack.sh
```

Override roots if needed:

```bash
ROOT=/Users/dallison/c_compiler SSD=$HOME/bbc_micro/games.ssd \
  .cursor/skills/build-invaders/scripts/build_and_pack.sh
```

Emulator: mount `~/bbc_micro/games.ssd`, then `*RUN INVADER` (or Shift-Break → `*CAT`).

## Manual steps (same as the script)

```bash
ROOT=/Users/dallison/c_compiler
SUP="$ROOT/6502 support"
WORK="$ROOT/games/build"
DAVECC="$ROOT/bazel-bin/davecc"
export DAVECC_LIB_DIR="$ROOT/bazel-bin/libc"
export DAVECC_INCLUDE_DIR="$ROOT/libc/include"
mkdir -p "$WORK"

"$DAVECC" -target 6502-acorn-bbc-davecc -c "$SUP/bbc_vdu.s" -o "$WORK/bbc_vdu.o"
"$DAVECC" -target 6502-acorn-bbc-davecc -c "$SUP/screen1.s" -o "$WORK/screen1.o"
"$DAVECC" -target 6502-acorn-bbc-davecc -c "$SUP/push.s" -o "$WORK/push.o"
"$DAVECC" -target 6502-acorn-bbc-davecc -c "$ROOT/games/sprites.s" -o "$WORK/sprites.o"

"$DAVECC" -target 6502-acorn-bbc-davecc -std=c++20 -fno-exceptions -Os \
  -ffunction-sections -Wl,--gc-sections \
  -Wl,-T -Wl,"$SUP/bbc_mode2_e00.ld" \
  "$ROOT/games/invaders.cc" \
  "$WORK/bbc_vdu.o" "$WORK/screen1.o" "$WORK/sprites.o" "$WORK/push.o" \
  -o "$WORK/invaders.elf"

python3 "$SUP/elf2ssd.py" "$HOME/bbc_micro/games.ssd" \
  "SNAKE=$WORK/snake.elf" "INVADER=$WORK/invaders.elf"
```

Omit `SNAKE=...` only if `snake.elf` is absent. Always prefer keeping both names on the disc.

## Verify before packing

| Check | Expect |
|-------|--------|
| ELF load | `0xE00` (`bbc_mode2_e00.ld`) |
| Image end | `end ≤ 0x7C00` (soft stack grows down from `&7C00`) |
| Headroom | Prefer `room_7c00 ≥ ~2K` |
| `k_cannon`, `k_squid0`, `batch_base` | Addresses `< 0x3000` (Mode 1 screen) |

```bash
python3 - <<'PY'
import sys
sys.path.insert(0, "/Users/dallison/c_compiler/6502 support")
from elf2ssd import read_elf
_, load, img = read_elf("/Users/dallison/c_compiler/games/build/invaders.elf")
end = load + len(img)
print(f"load=0x{load:x} end=0x{end:x} room_7c00={0x7c00-end}")
PY
nm games/build/invaders.elf | rg 'k_cannon|k_squid0|batch_base'
```

## Layout (do not “fix” casually)

- **Link**: `6502 support/bbc_mode2_e00.ld` — image at `&E00`; `elf2ssd` wraps a reloc stub loaded at DFS `&1F00` that copies down to `&E00`.
- **Sprites**: `games/sprites.s` in `.text.blitter` / low rodata — must stay under `&3000`. High `.rodata` sprites get shredded by the soft stack.
- **Blitter**: `6502 support/screen1.s` — `Screen::fill` / `stamp` / `close`; 256-byte batch auto-flushes when full.
- **Coros**: three `Play` jobs, **one** `co_await Frame` each; `play_pool[96]` only. A second await in `fleet_coro` overflowed the pool and smashed ZP (garbled text, quit to BASIC). Do not grow `play_pool` without checking high BSS vs `lives`/`score`.
- **Mode**: call `bbc::mode(129)` **once** in `main`. A second `mode()` re-parks the soft stack and returns to BASIC.
- **Laser**: `hop_shot` flushes each hop for a smooth climb; bombs move in `beat_hazards` before the fleet beat (bombs can pause briefly during the bolt).

## Snake (optional companion on the same SSD)

Snake uses `bbc_mode2.ld` (not `_e00`) historically; keep an existing `games/build/snake.elf` when packing. Rebuild snake only if that ELF is missing or the user asks.

## After packing

Tell the user to reload the disc / `*RUN INVADER`. Do not commit `games/build/` artifacts unless asked.
