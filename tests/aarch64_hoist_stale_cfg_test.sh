#!/usr/bin/env bash
# Post-RA loop-invariant hoist must not clobber a live pointer in a shrink-
# wrapped preheader when CFG edges are stale after the fast/slow path split.
#
# Symptom (fixed): after `add xN, xP, #off` (P holds the object pointer),
# hoist emitted `movz wP, #1` into the same preheader.
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/aarch64-hoist-stale.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

cat >"$WORK/scan.c" <<'SRC'
#include <stdint.h>

struct P {
  uint32_t pad0[17];
  uint32_t sig_pending; /* offset 68 in this layout; real Process uses 120 */
  uint64_t sig_handler[32];
};

static struct P *g_current;

int deliver(uint64_t *tf) {
  struct P *p = g_current;
  if (p == 0 || p->sig_pending == 0 || tf == 0) {
    return 0;
  }
  uint32_t pending = p->sig_pending;
  int chosen = 0;
  uint64_t handler = 0;
  if ((pending & (1u << 9)) != 0) {
    chosen = 9;
    handler = 0;
  } else {
    int sig = 1;
    while (sig < 32) {
      uint32_t bit = 1u << (unsigned)sig;
      if ((pending & bit) != 0) {
        uint64_t h = p->sig_handler[sig];
        if (h == 1 || (h == 0 && sig == 17)) {
          p->sig_pending = p->sig_pending & ~bit;
        } else {
          chosen = sig;
          handler = h;
          break;
        }
      }
      sig++;
    }
  }
  if (chosen == 0) {
    return 0;
  }
  p->sig_pending = p->sig_pending & ~(1u << (unsigned)chosen);
  tf[0] = (uint64_t)chosen;
  tf[1] = handler;
  return 2;
}
SRC

"$DAVECC" -target aarch64-unknown-daveos-davecc -std=c11 -O2 -S \
  "$WORK/scan.c" -o "$WORK/scan.s"

# Fail if any `add x*, xR, #imm` is followed within a few instructions by
# `movz wR, #1` (hoist overwriting the base pointer with the loop constant).
python3 - "$WORK/scan.s" <<'PY'
import re, sys
lines = open(sys.argv[1]).read().splitlines()
bugs = []
for i, line in enumerate(lines):
    m = re.search(r"\badd\s+x\d+,\s*x(\d+),\s*#", line)
    if not m:
        continue
    reg = m.group(1)
    for j in range(i + 1, min(i + 12, len(lines))):
        if re.search(rf"\bmovz?\s+w{reg},\s*#(?:0x)?1\b", lines[j]):
            bugs.append((i + 1, line.strip(), j + 1, lines[j].strip()))
            break
if bugs:
    print("hoist clobbered live base register:", file=sys.stderr)
    for a, al, b, bl in bugs:
        print(f"  L{a}: {al}", file=sys.stderr)
        print(f"  L{b}: {bl}", file=sys.stderr)
    sys.exit(1)
print("ok")
PY
