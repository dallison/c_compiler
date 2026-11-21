// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

// Exercises:
// 1. Fast-path register splitting + shrink-wrapping when the slow path enters a
//    call-containing loop with loop-invariant hoisted values (dead-end LICM
//    preheader must stay off the fast path).
// 2. Fast-path register splitting across a dense switch jump table whose entries
//    mix fast call-free returns, dedicated slow call arms, and shared default
//    slow arms without perturbing jump-table entry stride.
// 3. Forward-declared two-use internal static helper inlining and dead internal
//    static function elimination at -O2 (while keeping address-taken and
//    attribute((used)) static functions alive).
// 4. Address-add / operand forwarding safety when an inlined helper dereferences
//    a post-decremented pointer (`p += 3; LsrPostDec(p--);`).

extern "C" void UnreferencedDeadStaticTargetMustBeEliminated();

static int DeadStaticLeaf(int x) {
  UnreferencedDeadStaticTargetMustBeEliminated();
  return x + 1;
}

static int DeadStaticCaller(int x) {
  return DeadStaticLeaf(x) * 3;
}

static int g_used_attr_sink = 0;

__attribute__((used)) static void KeptByUsedAttribute() {
  g_used_attr_sink = 42;
}

static int AddressTakenStatic(int x, int y) {
  return x * 7 + y;
}

static int ForwardTwoUseHelper(int a, int b, int c);

__attribute__((noinline)) static int FirstCallerOfTwoUse(int x) {
  return ForwardTwoUseHelper(x, x + 3, x ^ 5);
}

static int ForwardTwoUseHelper(int a, int b, int c) {
  int acc = a * 11 + b * 5 - c;
  for (int i = 0; i < 4; i++) {
    acc = (acc ^ (b + i)) + (a & (i + 1)) + c;
  }
  return acc;
}

__attribute__((noinline)) static int SecondCallerOfTwoUse(int x) {
  return ForwardTwoUseHelper(x + 1, x * 2, x + 9);
}

__attribute__((noinline)) static int SlowSink(int a, int b, int c, int d) {
  return (a * 3) ^ (b + 7) ^ (c * 5) ^ d;
}

struct FrameState {
  int regs[16];
  int mode;
  int limit;
};

__attribute__((noinline)) static int FastReturnOrSlowLoop(FrameState* s,
                                                          int gate) {
  if (gate == 0) {
    return s->regs[0] + s->regs[1];
  }
  int r0 = s->regs[0];
  int r1 = s->regs[1];
  int r2 = s->regs[2];
  int r3 = s->regs[3];
  int r4 = s->regs[4];
  int r5 = s->regs[5];
  int r6 = s->regs[6];
  int r7 = s->regs[7];
  int total = 0;
  for (int i = 0; i < s->limit; i++) {
    int inv = (r0 + r1) ^ (r2 + r3) ^ (r4 + r5) ^ (r6 + r7) ^ s->mode;
    total += SlowSink(inv, i + r0, r3 + s->regs[i & 15], r7 - i);
  }
  return total;
}

__attribute__((noinline)) static int SwitchFastAndSlowArms(const int* v,
                                                           int op) {
  int a = v[0];
  int b = v[1];
  int c = v[2];
  int d = v[3];
  int e = v[4];
  int f = v[5];
  switch (op) {
    case 0:
      return a + b;
    case 1:
      return SlowSink(a, b, c, d) + e + f;
    case 2:
      return b * 2 + c;
    case 3:
      return c ^ d;
    case 4:
      return SlowSink(f, e, d, c) - a;
    case 5:
      return d + e + f;
    case 6:
      return a * 4 - f;
    case 7:
    default:
      return SlowSink(a + b, c + d, e + f, op);
  }
}

static inline unsigned long long LsrPostDec(unsigned long long* p) {
  unsigned long long v = *p;
  unsigned long long carry = v & 1ull;
  *p = v >> 1;
  return carry;
}

static inline int ReadPostDec(const int* p) {
  return *p;
}

__attribute__((noinline)) static int SumFromTailPostDec(const int* a) {
  a += 3;
  int first = ReadPostDec(a--);
  return first * 10 + a[0];
}

__attribute__((noinline)) static void RightShift4Words(unsigned long long* a) {
  a += 3;
  unsigned long long carry = LsrPostDec(a--);
  for (int i = 0; i < 3; i++) {
    unsigned long long next_carry = *a & 1ull;
    *a = (*a >> 1) | (carry << 63);
    carry = next_carry;
    a--;
  }
}

int main() {
  int (*fn)(int, int) = &AddressTakenStatic;
  if (fn(6, 5) != 47) {
    return 1;
  }

  auto ref = [](int a, int b, int c) {
    int acc = a * 11 + b * 5 - c;
    for (int i = 0; i < 4; i++) {
      acc = (acc ^ (b + i)) + (a & (i + 1)) + c;
    }
    return acc;
  };
  if (FirstCallerOfTwoUse(4) != ref(4, 7, 1)) {
    return 2;
  }
  if (SecondCallerOfTwoUse(4) != ref(5, 8, 13)) {
    return 3;
  }

  FrameState st{};
  for (int i = 0; i < 16; i++) {
    st.regs[i] = i * 3 + 1;
  }
  st.mode = 9;
  st.limit = 5;

  if (FastReturnOrSlowLoop(&st, 0) != 5) {
    return 4;
  }
  int slow_expected = 0;
  {
    int inv = (st.regs[0] + st.regs[1]) ^ (st.regs[2] + st.regs[3]) ^
              (st.regs[4] + st.regs[5]) ^ (st.regs[6] + st.regs[7]) ^ st.mode;
    for (int i = 0; i < st.limit; i++) {
      slow_expected +=
          SlowSink(inv, i + st.regs[0], st.regs[3] + st.regs[i & 15],
                   st.regs[7] - i);
    }
  }
  if (FastReturnOrSlowLoop(&st, 1) != slow_expected) {
    return 5;
  }

  const int vals[6] = {3, 5, 7, 11, 13, 17};
  if (SwitchFastAndSlowArms(vals, 0) != 8) return 10;
  if (SwitchFastAndSlowArms(vals, 1) != SlowSink(3, 5, 7, 11) + 30) return 11;
  if (SwitchFastAndSlowArms(vals, 2) != 17) return 12;
  if (SwitchFastAndSlowArms(vals, 3) != (7 ^ 11)) return 13;
  if (SwitchFastAndSlowArms(vals, 4) != SlowSink(17, 13, 11, 7) - 3) return 14;
  if (SwitchFastAndSlowArms(vals, 5) != 41) return 15;
  if (SwitchFastAndSlowArms(vals, 6) != -5) return 16;
  if (SwitchFastAndSlowArms(vals, 7) != SlowSink(8, 18, 30, 7)) return 17;
  if (SwitchFastAndSlowArms(vals, 99) != SlowSink(8, 18, 30, 99)) return 18;

  KeptByUsedAttribute();
  if (g_used_attr_sink != 42) {
    return 19;
  }

  unsigned long long words[4] = {0x0ull, 0x0ull, 0x0ull, 0x3ull};
  RightShift4Words(words);
  if (words[3] != 0x1ull || words[2] != (1ull << 63) || words[1] != 0x0ull ||
      words[0] != 0x0ull) {
    return 20;
  }

  const int tail_vals[4] = {1, 2, 3, 4};
  if (SumFromTailPostDec(tail_vals) != 43) {
    return 21;
  }
  return 0;
}
