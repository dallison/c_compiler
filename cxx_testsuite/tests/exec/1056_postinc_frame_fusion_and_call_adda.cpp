// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

// Exercises:
// 1. Prologue/epilogue sp-adjustment fusion and header-pad elision across
//    0, 1, 2, 3, and 4 callee-saved integer registers, including C++ exception
//    unwinding through fused odd/even frames.
// 2. Post-increment and post-decrement reuse of the adjacent pre-update memory
//    load across 8/16/32/64-bit integer, pointer, floating-point, and volatile
//    fields.
// 3. Branch-over-branch inversion (`b.cond`, `cbz`/`cbnz`, `tbz`/`tbnz`),
//    frameless/shrink-wrapped bare-`ret` threading, and dense jump-table switch
//    returns.
// 4. Call-free high-page `add` CSE alongside call-crossing GVN/LICM suppression
//    of constant-offset pointer additions (including loop back-edges with
//    post-use calls).

struct PostIncFields {
  unsigned char u8;
  signed char s8;
  short i16;
  unsigned short u16;
  int i32;
  unsigned int u32;
  long long i64;
  unsigned long long u64;
  float f32;
  double f64;
  const int* ptr;
  volatile int vol;
};

__attribute__((noinline)) static long long ExercisePostIncDec(PostIncFields* p) {
  unsigned char a = p->u8--;
  unsigned char b = p->u8++;
  signed char c = p->s8++;
  short d = p->i16--;
  unsigned short e = p->u16++;
  int f = p->i32--;
  unsigned int g = p->u32++;
  long long h = p->i64--;
  unsigned long long k = p->u64++;
  float fv = p->f32++;
  double dv = p->f64--;
  int pv = *(p->ptr++);
  int vv = p->vol++;
  return (long long)a + b + c + d + e + f + (long long)g + h + (long long)k +
         (long long)(fv * 2.0f) + (long long)(dv * 2.0) + pv + vv;
}

__attribute__((noinline)) static int LeafCall(int x) {
#if defined(__EXCEPTIONS)
  if (x == 999) {
    throw 77;
  }
#endif
  return x * 3 + 1;
}

// 0 callee-saved regs, no locals: frame header pad elided (`stack_frame_size == 0`).
__attribute__((noinline)) static int FrameZeroSaved(int x) {
  return LeafCall(x) + 5;
}

// 1 callee-saved reg (odd): `str x19, [sp, #-16]!` / `ldr x19, [sp], #16`.
__attribute__((noinline)) static int FrameOneSaved(int x, int y) {
  int r = LeafCall(x);
  return r + y;
}

// 2 callee-saved regs (even): `stp x20, x19, [sp, #-16]!` / `ldp x20, x19, [sp], #16`.
__attribute__((noinline)) static int FrameTwoSaved(int x, int y, int z) {
  int r = LeafCall(x);
  return r + y * 2 + z;
}

// 3 callee-saved regs (odd >= 3): `str` pre-index + `stp` pair.
__attribute__((noinline)) static int FrameThreeSaved(int a, int b, int c, int d) {
  int r = LeafCall(a);
  return r + b * 3 + c * 5 + d;
}

// 4 callee-saved regs (even >= 4): two `stp` pairs with fused bottom pair.
__attribute__((noinline)) static int FrameFourSaved(int a, int b, int c,
                                                    int d, int e) {
  int r = LeafCall(a);
  return r + b * 3 + c * 5 + d * 7 + e;
}

__attribute__((noinline)) static int UnwindCaller(int trigger) {
  int s1 = trigger + 11;
  int s2 = trigger + 22;
  int s3 = trigger + 33;
#if defined(__EXCEPTIONS)
  try {
    int a = FrameOneSaved(1, s1);
    int b = FrameTwoSaved(2, s1, s2);
    int c = FrameThreeSaved(trigger, s1, s2, s3);
    return a + b + c;
  } catch (int code) {
    return code + s1 * 2 + s2 * 3 + s3 * 5;
  }
#else
  int a = FrameOneSaved(1, s1);
  int b = FrameTwoSaved(2, s1, s2);
  int c = FrameThreeSaved(trigger, s1, s2, s3);
  return a + b + c;
#endif
}

__attribute__((noinline)) static int BranchAndRetPatterns(unsigned int flags,
                                                          int x) {
  if (x < 0) {
    return 10;
  }
  if ((flags & 1u) == 0) {
    return 20;
  }
  if ((flags & 8u) != 0) {
    return 30;
  }
  switch (x) {
    case 0:
      return 100;
    case 1:
      return 101;
    case 2:
      return LeafCall(x);
    case 3:
      return 103;
    case 4:
      return 104;
    default:
      return 105;
  }
}

struct LargeMachine {
  unsigned char low[0x4000];
  int page4_a;
  int page4_b;
  unsigned char mid1[0x2000 - 8];
  int page6_a;
  int page6_b;
  int page6_arr[8];
  unsigned char mid2[0x1000 - 40];
  int page7_a;
  int page7_b;
  unsigned char high[0x1500];
};

__attribute__((noinline)) static void MutateMachine(LargeMachine* m, int delta) {
  m->page4_b += delta;
  m->page6_b += delta * 2;
  m->page7_b += delta * 3;
}

__attribute__((noinline)) static int StepLargeMachine(LargeMachine* m,
                                                      int steps,
                                                      int mode) {
  // Call-free multi-block high-page accesses: eligible for page-add CSE.
  int fast_sum = 0;
  if (mode > 0) {
    if (m->page7_a == 10) {
      fast_sum += m->page7_b;
    }
    if (m->page6_a == 20) {
      fast_sum += m->page6_b;
    }
  }
  if (mode == 1) {
    return fast_sum + m->page7_a + m->page6_a;
  }

  // Loop with a post-use call on the back-edge: `m + 0x4000` / `m + 0x6000`
  // must remain valid across iterations when clobbered by caller-saved calls.
  int acc = fast_sum;
  for (int i = 0; i < steps; i++) {
    acc += m->page4_a + m->page6_arr[i & 7] + m->page7_b;
    MutateMachine(m, i + 1);
  }
  return acc + m->page4_b + m->page6_b;
}

static LargeMachine g_machine;

int main() {
  int arr[3] = {40, 50, 60};
  PostIncFields pf = {
      10, -3, 100, 200, 1000, 2000u, 5000LL, 6000ULL, 1.5f, 2.5, &arr[0], 7,
  };
  long long sum = ExercisePostIncDec(&pf);
  long long expected_sum = 10 + 9 + (-3) + 100 + 200 + 1000 + 2000 + 5000 +
                           6000 + 3 + 5 + 40 + 7;
  if (sum != expected_sum) {
    return 1;
  }
  if (pf.u8 != 10 || pf.s8 != -2 || pf.i16 != 99 || pf.u16 != 201 ||
      pf.i32 != 999 || pf.u32 != 2001u || pf.i64 != 4999LL ||
      pf.u64 != 6001ULL || pf.f32 != 2.5f || pf.f64 != 1.5 ||
      pf.ptr != &arr[1] || pf.vol != 8) {
    return 2;
  }

  if (FrameZeroSaved(4) != 18) {
    return 3;
  }
  if (FrameOneSaved(4, 9) != 22) {
    return 4;
  }
  if (FrameTwoSaved(4, 9, 5) != 36) {
    return 5;
  }
  if (FrameThreeSaved(4, 2, 3, 4) != 38) {
    return 6;
  }
  if (FrameFourSaved(4, 2, 3, 4, 5) != 67) {
    return 7;
  }

  // Normal path and exception unwind path through fused odd/even frames.
  int ok_unwind = UnwindCaller(5);
  if (ok_unwind != FrameOneSaved(1, 16) + FrameTwoSaved(2, 16, 27) +
                       FrameThreeSaved(5, 16, 27, 38)) {
    return 8;
  }
#if defined(__EXCEPTIONS)
  int thrown_unwind = UnwindCaller(999);
  if (thrown_unwind != 77 + 1010 * 2 + 1021 * 3 + 1032 * 5) {
    return 9;
  }
#endif

  if (BranchAndRetPatterns(0u, 2) != 20 ||
      BranchAndRetPatterns(9u, 2) != 30 ||
      BranchAndRetPatterns(1u, -1) != 10 ||
      BranchAndRetPatterns(1u, 0) != 100 ||
      BranchAndRetPatterns(1u, 2) != 7 ||
      BranchAndRetPatterns(1u, 4) != 104 ||
      BranchAndRetPatterns(1u, 9) != 105) {
    return 10;
  }

  g_machine.page4_a = 3;
  g_machine.page4_b = 4;
  g_machine.page6_a = 20;
  g_machine.page6_b = 5;
  g_machine.page7_a = 10;
  g_machine.page7_b = 6;
  for (int i = 0; i < 8; i++) {
    g_machine.page6_arr[i] = i * 2 + 1;
  }
  if (StepLargeMachine(&g_machine, 0, 1) != (6 + 5) + 10 + 20) {
    return 11;
  }
  int stepped = StepLargeMachine(&g_machine, 4, 2);
  // Initial fast_sum = 11.
  // i=0: acc += 3 + 1 + 6 = 10 (acc=21); page4_b=5, page6_b=7, page7_b=9
  // i=1: acc += 3 + 3 + 9 = 15 (acc=36); page4_b=7, page6_b=11, page7_b=15
  // i=2: acc += 3 + 5 + 15 = 23 (acc=59); page4_b=10, page6_b=17, page7_b=24
  // i=3: acc += 3 + 7 + 24 = 34 (acc=93); page4_b=14, page6_b=25, page7_b=36
  // return 93 + 14 + 25 = 132.
  if (stepped != 132) {
    return 12;
  }
  return 0;
}
