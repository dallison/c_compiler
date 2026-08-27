// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

// A TLS symbol's value is its offset within the thread's block, which is what a
// local-exec relocation needs, so the TLS sections carry no load address and are
// skipped when the linker hands addresses out.  Their file content is still
// written just after the initialized data, and an executable stretches the
// writable PT_LOAD over it so a native loader can reach the initialization
// image, which means the loader copies the image to whatever address that file
// offset maps to.  Nothing had claimed that address and .bss began there, so
// loading the program wrote the TLS image over the first bytes of .bss.
//
// The image needs a strong enough alignment here that the writer pads the file
// before it, because reserving room for the image's size alone still left its
// tail on top of .bss.  `data_odd` ends the initialized data off a boundary to
// force that padding.

#include <stdint.h>

char data_odd[3] = {1, 2, 3};

thread_local int64_t tls_wide = 0x1122334455667788LL;
thread_local int64_t tls_narrow = 0x0a0b0c0dLL;
thread_local int tls_zero;

static int64_t bss_head;
static int64_t bss_tail[64];
static unsigned char bss_bytes[24];

int main() {
  if (data_odd[0] != 1 || data_odd[1] != 2 || data_odd[2] != 3) {
    return 3;
  }

  // The bytes the loader misplaced.  A zeroed .bss is the whole point.
  if (bss_head != 0) {
    return 4;
  }
  for (int i = 0; i < 64; i++) {
    if (bss_tail[i] != 0) {
      return 5;
    }
  }
  for (int i = 0; i < 24; i++) {
    if (bss_bytes[i] != 0) {
      return 6;
    }
  }

  // The image itself still has to arrive intact wherever it was put.
  if (tls_wide != 0x1122334455667788LL) {
    return 7;
  }
  if (tls_narrow != 0x0a0b0c0dLL) {
    return 8;
  }
  if (tls_zero != 0) {
    return 9;
  }

  // Checked one at a time on purpose: folding these into a single chained ||
  // trips an unrelated ARM -O2 defect that has nothing to do with where the
  // TLS image is placed.
  tls_wide = 1;
  tls_narrow = 2;
  tls_zero = 3;
  if (tls_wide != 1) {
    return 10;
  }
  if (tls_narrow != 2) {
    return 11;
  }
  if (tls_zero != 3) {
    return 12;
  }

  // Writing through the TLS block must not have reached .bss either.
  if (bss_head != 0 || bss_tail[0] != 0 || bss_bytes[0] != 0) {
    return 13;
  }
  return 0;
}
