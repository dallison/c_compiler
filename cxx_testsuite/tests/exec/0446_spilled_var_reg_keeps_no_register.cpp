// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

// A chained || read back a value that had just been written as though the write
// had not happened, on ARM at -O2.
//
// The ARM allocator reclaims a promoted variable's register at every block entry,
// because the approximate live-in sets can omit a variable that a loop back edge
// needs again.  It did so even for a variable that had since been spilled, whose
// value lives in its slot and whose every use reloads into a fresh register.  A
// spilled variable squatting on its old register is not harmless: the live-in
// loop that follows refuses to reserve a register another value already owns, so
// the value genuinely holding it across the block lost its reservation.  The
// spill-victim search then found the register owned by a spilled instruction,
// cleared that stale ownership as it is entitled to, and handed the register to
// the next value that needed one while the real owner was still live in it.
//
// Here the pointer walking `bss_bytes` is the spilled variable, and the value it
// displaces is the constant 3 -- materialized once for `g_zero = 3` and reused
// as the comparison operand of the third term of the chain, which is read two
// blocks later.  The middle term's block clobbered it, so the chain compared
// g_zero against a boolean and reported a difference that was not there.
//
// This wants everything below at once: with the loop gone, or the reads before
// the writes gone, or the chain shortened to two terms, the constant lands
// somewhere else and nothing goes wrong.

#include <stdint.h>

int64_t g_wide = 0x1122334455667788LL;
int64_t g_narrow = 0x0a0b0c0dLL;
int g_zero;

static unsigned char bss_bytes[4];

int main() {
  for (int i = 0; i < 4; i++) {
    if (bss_bytes[i] != 0) {
      return 3;
    }
  }

  if (g_wide != 0x1122334455667788LL) {
    return 4;
  }
  if (g_narrow != 0x0a0b0c0dLL) {
    return 5;
  }
  if (g_zero != 0) {
    return 6;
  }

  g_wide = 1;
  g_narrow = 2;
  g_zero = 3;

  // The chain is the point: each term on its own is fine.
  if (g_wide != 1 || g_narrow != 2 || g_zero != 3) {
    return 7;
  }

  // Reading them again separately must agree with the chain.
  if (g_wide != 1) {
    return 8;
  }
  if (g_narrow != 2) {
    return 9;
  }
  if (g_zero != 3) {
    return 10;
  }
  return 0;
}
