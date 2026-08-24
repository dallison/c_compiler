#include "test_framework.h"

#if defined(__x86_64__)
#include <eh_frame.h>
#include <lsda.h>
#endif
#include <setjmp.h>
#include <stdlib.h>

int TestAbs(void) {
  int failures = 0;

  CHECK_EQ(abs(-7), 7, failures);
  CHECK_EQ(abs(0), 0, failures);
  CHECK_EQ(abs(11), 11, failures);
  CHECK_EQ(labs(-9L), 9L, failures);
  CHECK_EQ(llabs(-13LL), 13LL, failures);
  return failures;
}

int TestStrtol(void) {
  int failures = 0;
  char* end = NULL;
  long signed_value;
  unsigned long unsigned_value;

  signed_value = strtol("123", &end, 10);
  CHECK_EQ(signed_value, 123, failures);
  CHECK(*end == '\0', failures);
  signed_value = strtol("-42tail", &end, 10);
  CHECK_EQ(signed_value, -42, failures);
  CHECK(*end == 't', failures);
  signed_value = strtol("0x2a", &end, 0);
  CHECK_EQ(signed_value, 42, failures);
  CHECK(*end == '\0', failures);
  unsigned_value = strtoul("-1", &end, 10);
  CHECK_EQ(unsigned_value, (unsigned long)-1, failures);
  CHECK(*end == '\0', failures);
  CHECK_EQ(atoi("5"), 5, failures);
  return failures;
}

static void JumpWithValue(jmp_buf env, int value) {
  longjmp(env, value);
}

static void JumpThroughFrames(jmp_buf env, int depth) {
  volatile int marker = depth + 11;
  if (depth > 0) {
    JumpThroughFrames(env, depth - 1);
  }
  if (marker == 11) {
    longjmp(env, 77);
  }
  longjmp(env, 78);
}

static int StackStillUsable(int value) {
  volatile int local = value + 5;
  return local + 2;
}

int TestSetjmp(void) {
  jmp_buf env;
  int failures = 0;

  int val = setjmp(env);
  if (val == 0) {
    JumpWithValue(env, 42);
  }
  CHECK_EQ(val, 42, failures);

  val = setjmp(env);
  if (val == 0) {
    JumpWithValue(env, 0);
  }
  CHECK_EQ(val, 1, failures);

  val = setjmp(env);
  if (val == 0) {
    JumpThroughFrames(env, 4);
  }
  CHECK_EQ(val, 77, failures);
  CHECK_EQ(StackStillUsable(5), 12, failures);

  return failures;
}

int TestEHFrame(void) {
  int failures = 0;
#if defined(__x86_64__)
  DaveEHFrameRange range;
  uintptr_t cursor;
  DaveEHFDE fde;
  DaveEHFDE found;
  DaveEHFrameCFI cfi;
  int count;

  if (!DaveEHFrameGetRange(&range)) {
    return 1;
  }
  if (!(range.start < range.end)) {
    return 2;
  }
  // The CIE has a "zR" augmentation: length 26 (18 bytes content + 8-byte align).
  if (*(const unsigned int*)range.start != 26) {
    return 8;
  }
  if (*(const unsigned int*)(range.start + 30) == 0) {
    return 9;
  }
  count = DaveEHFrameCountFDEs();
  if (count == 0) {
    return 10;
  }
  cursor = (uintptr_t)range.start;
  fde.pc_begin = 0;
  while (DaveEHFrameNextFDE(&cursor, (uintptr_t)range.end, &fde)) {
    if (!DaveEHFrameCFIAtPC(&fde, fde.pc_begin, &cfi)) {
      continue;
    }
    if (cfi.cfa_reg == 7 && cfi.cfa_offset == 8 && cfi.ra_reg == 16 &&
        cfi.regs[16].rule == DAVE_CFI_REG_OFFSET &&
        cfi.regs[16].offset == -8) {
      break;
    }
    fde.pc_begin = 0;
  }
  if (fde.pc_begin == 0) {
    return 16;
  }
  if (!(fde.pc_begin < fde.pc_end)) {
    return 12;
  }
  if (!DaveEHFrameFindFDE(fde.pc_begin, &found)) {
    return 13;
  }
  if (found.pc_begin != fde.pc_begin || found.pc_end != fde.pc_end) {
    return 14;
  }

#endif
  return failures;
}

int TestLSDAParser(void) {
  int failures = 0;
#if defined(__x86_64__)
  static const uint8_t kLsda[] = {
      0xff, 0xff, 0x01, 0x04, 0x00, 0x10, 0x20, 0x01, 0x00, 0x00,
  };
  DaveLSDAAction action;
  DaveLSDAQuery query = {
      .pc = 0x1008,
      .scope_start = 0x1008,
      .scope_end = 0x1008,
      .thrown = 0,
      .search_phase = 0,
      .handler_frame = 0,
  };
  if (!DaveLSDAFindAction(kLsda, 0x1000, &query, &action)) {
    return 1;
  }
  if (!action.is_cleanup || action.is_catch) {
    return 2;
  }
  if (action.landing_pad != 0x1020) {
    return 3;
  }

  static const uint8_t kCatchAllLsda[] = {
      0xff, 0x1b, 0x0c, 0x01, 0x04, 0x08, 0x18, 0x30,
      0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  query.pc = 0x1020;
  query.scope_start = 0x1020;
  query.scope_end = 0x1020;
  query.search_phase = 1;
  if (!DaveLSDAFindAction(kCatchAllLsda, 0x1000, &query, &action)) {
    return 4;
  }
  if (!action.is_catch || action.is_cleanup) {
    return 5;
  }
  if (action.landing_pad != 0x1030) {
    return 6;
  }
#endif
  return failures;
}
