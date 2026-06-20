#include "test_framework.h"

#if defined(__x86_64__)
#include <eh_frame.h>
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
  DaveEHFrameRegisters regs;
  DaveEHFrameWalkResult walk;
  uintptr_t fake_stack[4];
  int count;

  if (!DaveEHFrameGetRange(&range)) {
    return 1;
  }
  if (!(range.start < range.end)) {
    return 2;
  }
  if (*(const unsigned int*)range.start != 28) {
    return 8;
  }
  if (*(const unsigned int*)(range.start + 32) == 0) {
    return 9;
  }
  count = DaveEHFrameCountFDEs();
  if (count == 0) {
    return 10;
  }
  cursor = (uintptr_t)range.start;
  if (!DaveEHFrameNextFDE(&cursor, (uintptr_t)range.end, &fde)) {
    return 11;
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
  if (!DaveEHFrameCFIAtPC(&fde, fde.pc_begin, &cfi)) {
    return 15;
  }
  if (cfi.cfa_reg != 7 || cfi.cfa_offset != 8 ||
      cfi.return_address_offset != -8) {
    return 16;
  }
  if (fde.has_frame) {
    if (!DaveEHFrameCFIAtPC(&fde, fde.pc_begin + 2, &cfi)) {
      return 20;
    }
    if (cfi.cfa_reg != 7 || cfi.cfa_offset != 16 ||
        !cfi.has_saved_rbp || cfi.saved_rbp_offset != -16) {
      return 21;
    }
    if (!DaveEHFrameCFIAtPC(&fde, fde.pc_begin + 8, &cfi)) {
      return 22;
    }
    if (cfi.cfa_reg != 6 || cfi.cfa_offset != 8) {
      return 23;
    }
  }

  fake_stack[0] = fde.pc_end;
  fake_stack[1] = 0x12345678;
  fake_stack[2] = 0;
  fake_stack[3] = 0;
  regs.pc = fde.pc_begin;
  regs.rsp = (uintptr_t)&fake_stack[0];
  regs.rbp = 0;
  if (!DaveEHFrameWalkFrame(&regs, &walk)) {
    return 17;
  }
  if (walk.caller_pc != fde.pc_end) {
    return 18;
  }
  if (walk.caller_rsp != (uintptr_t)&fake_stack[1]) {
    return 19;
  }
#endif
  return failures;
}
