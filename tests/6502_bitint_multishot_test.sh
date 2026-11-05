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
WORK="${TEST_TMPDIR:-/tmp}/6502-bitint-multishot"
mkdir -p "$WORK"

# unsigned _BitInt(8) slot loops and fire-timer compares must allow several
# concurrent shots — same pattern as games/asteroids.cc.
cat >"$WORK/fire.cc" <<'SRC'
typedef unsigned _BitInt(8) u8;
typedef _BitInt(8) i8;
enum { AS_SHOTS = 3, FireGap = 6, ShotLife = 40, KeyFire = 8 };

u8 key_bits;
u8 ship_fire_t;
u8 shot_on[AS_SHOTS];
u8 shot_life[AS_SHOTS];
u8 fires;
u8 peak;

void fire_shot() {
  u8 i;
  for (i = 0; i < AS_SHOTS; i++) {
    if (!shot_on[i]) {
      shot_on[i] = 1;
      shot_life[i] = ShotLife;
      ship_fire_t = FireGap;
      fires = (u8)(fires + 1);
      return;
    }
  }
}

void beat_ship() {
  if (ship_fire_t > 0) {
    ship_fire_t--;
  } else if (key_bits & KeyFire) {
    fire_shot();
  }
}

void beat_shots() {
  u8 i;
  u8 n = 0;
  for (i = 0; i < AS_SHOTS; i++) {
    if (!shot_on[i]) {
      continue;
    }
    if (shot_life[i] > 0) {
      shot_life[i]--;
    }
    if (shot_life[i] == 0) {
      shot_on[i] = 0;
      continue;
    }
    n = (u8)(n + 1);
  }
  if (n > peak) {
    peak = n;
  }
}

int main() {
  u8 f;
  key_bits = KeyFire;
  for (f = 0; f < 30; f++) {
    beat_shots();
    beat_ship();
  }
  if (fires < 3) {
    return 1;
  }
  if (peak < 3) {
    return 2;
  }
  return 0;
}
SRC

for target in 6502 65c02; do
  for opt in -O0 -Os; do
    exe="$WORK/fire-$target$opt.exe"
    "$DAVECC" -target "$target" -std=c++20 -fno-exceptions "$opt" \
      "$WORK/fire.cc" -o "$exe"
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

echo "ok BitInt multi-shot fire timer and slot loop"
