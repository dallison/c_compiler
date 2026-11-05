---
name: build-invaders
description: >-
  Build BBC Master Space Invaders with davecc, link at &E00, and pack
  INVADER onto ~/bbc_micro/games.ssd for the emulator. Use when rebuilding
  invaders, updating games.ssd, packing INVADER/SNAKE, or after editing
  games/invaders.cc, games/invaders_fx.s, games/sprites.py, screen1.s, or
  bbc_mode2_e00.ld.
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
"$DAVECC" -target 6502-acorn-bbc-davecc -c "$ROOT/games/invaders_fx.s" -o "$WORK/invaders_fx.o"

"$DAVECC" -target 6502-acorn-bbc-davecc -std=c++20 -fno-exceptions -Os \
  -ffunction-sections -Wl,--gc-sections \
  -Wl,-T -Wl,"$SUP/bbc_mode2_e00.ld" \
  "$ROOT/games/invaders.cc" \
  "$WORK/bbc_vdu.o" "$WORK/screen1.o" "$WORK/sprites.o" "$WORK/invaders_fx.o" \
  "$WORK/push.o" -o "$WORK/invaders.elf"

python3 "$SUP/elf2ssd.py" "$HOME/bbc_micro/games.ssd" \
  "SNAKE=$WORK/snake.elf" "INVADER=$WORK/invaders.elf"
```

Omit `SNAKE=...` only if `snake.elf` is absent. Always prefer keeping both names on the disc. To rebuild Snake:

```bash
"$DAVECC" -target 6502-acorn-bbc-davecc -std=c++20 -fno-exceptions \
  -Wl,-T -Wl,"$SUP/bbc_mode2.ld" "$ROOT/games/snake.cc" \
  "$WORK/bbc_vdu.o" "$WORK/push.o" -o "$WORK/snake.elf"
```

`games/sprites.s` is generated: edit `games/sprites.py` and run
`python3 games/sprites.py > games/sprites.s`.

## Verify before packing

| Check | Expect |
|-------|--------|
| ELF load | `0xE00` (`bbc_mode2_e00.ld`) |
| Image end | `end ≤ 0x7C00` (soft stack grows down from `&7C00`) |
| Headroom | Prefer `room_7c00 ≥ ~2K` |
| Global symbols of `screen1.o`, `sprites.o`, `invaders_fx.o` | All `< 0x3000` (the script prints the highest) |

The script does these checks; it fails if any of those symbols lands at or above `&3000`.

## Layout (do not “fix” casually)

- **Link**: `6502 support/bbc_mode2_e00.ld` — image at `&E00`; `elf2ssd` wraps a reloc stub loaded at DFS `&1F00` that copies down to `&E00`.
- **Shadow screen**: Mode 129 draws by setting ACCCON bit X (`&FE34`) with interrupts off, which maps shadow RAM over `&3000–&7FFF`. Only the `fx_*` routines set it, so the code and data they touch must live below `&3000`: all of `screen1.s`, `sprites.s` and `invaders_fx.s` (including `shot_pat`, the laser's strip). davecc ignores section attributes, so none of that can move to C++.
- **Shared constants**: `games/invaders.h` holds the `#define`s both sides use (layout, speeds, key bits, HUD columns). The assembler preprocesses it, so it holds nothing else.
- **Engine**: `games/invaders_fx.s` does what can happen in a frame of play: `frame_wait`, direct keyboard scan (`keys_read`), cannon and fire latch, firing, laser and collision (O(1) cell tests, no fleet scan), bomb drops and bombs, fleet march and bounds, UFO, HUD digits. Sprites in `sprites.s` are column-major and pre-shifted so a draw is straight stores.
- **C++ side**: `games/invaders.cc` owns the engine's state and tables (`extern "C"` globals), keeps game flow (phases, waves, scoring, deaths), and does what happens outside a frame of play: wave setup (`Fleet`), clearing the laser and bombs (`Volley`), the death images, launching the UFO. davecc's C++ runs 10–25x slower than the asm: firing, a bomb drop, the fleet bounds and the HUD digits each took 4K–12K cycles in C++ and dropped frames on kills, so they stay in asm. It calls the engine through no-argument functions on globals. davecc spends about 1K cycles entering a function that loads `this` or arguments, and `coroutine_handle::operator bool` costs about 1.2K, so hot paths avoid both.
- **Sound**: SOUND blocks (`Note`) and ENVELOPE blocks are in `invaders.cc`; `Game::run` defines envelopes 1–4 once, and the `sound_*` entries in `invaders_fx.s` call OSWORD 7 (about 750 cycles). Channel 0 is noise (explosions), 1 the march, 2 the laser, 3 the saucer. Every block sets the flush bit (`&10` on the channel): without it OSWORD 7 waits when a channel's queue is full, which stalls the frame. The saucer's warble lasts until `sound_ufo_off` (in `ufo_erase`, on death, on a cleared wave, on quit). Envelope target levels run 0–126; levels of 0 are silent.
- **Coros**: four `Play` jobs (player, laser, hazards, fleet), each with **one** `co_await Frame` per vsync, resumed in that order every tick; `play_pool[96]` only. A second await in a job overflows the pool and smashes ZP (garbled text, quit to BASIC). Do not grow `play_pool` without checking high BSS vs `lives`/`score`. If the four frames don't fit, `play_frame` runs the same beats as plain calls.
- **Budget**: a tick is one vsync (about 40K cycles); play uses about 16–23K including the MOS. Deploying a wave takes about 9 vsyncs (CLS + full redraw).
- **Assembler**: in davecc `.s` files `.word` is **4 bytes**; use `.hword` for 16-bit pointers and values.
- **Mode**: call `bbc::mode(129)` **once** in `main`. A second `mode()` re-parks the soft stack and returns to BASIC.

## Snake and Asteroids (same SSD, same link)

Snake and Asteroids match Invaders: shadow mode 1, `screen1.s` blitter,
sprites in `sprites.s`, play helpers in `snake_fx.s` / `asteroids_fx.s`,
C++20 coroutines, linked at `&E00` with `bbc_mode2_e00.ld`. The pack
script builds all three (`*RUN SNAKE`, `*RUN INVADER`, `*RUN ASTERO`).

## After packing

Tell the user to reload the disc / `*RUN INVADER`. Do not commit `games/build/` artifacts unless asked.
