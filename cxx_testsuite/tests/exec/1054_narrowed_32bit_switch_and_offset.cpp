// RUN: -std=c++20
// EXPECT_EXIT: 0
// Narrowed 32-bit integers may carry non-zero upper bits in 64-bit registers;
// dense jump-table switches and unsigned pointer offsets must ignore bits
// [63:32], while decayed 4-byte array pointers in pointer subtraction must not
// be truncated.

enum Op { OP0, OP1, OP2, OP3, OP4, OP5, OP6, OP7 };

__attribute__((noinline)) static unsigned long long Dirty64(unsigned low) {
  return 0x1234567800000000ULL | low;
}

__attribute__((noinline)) static int DenseSwitchInt(int x) {
  switch (x) {
    case 0: return 10;
    case 1: return 20;
    case 2: return 30;
    case 3: return 40;
    case 4: return 50;
    case 5: return 60;
    case 6: return 70;
    case 7: return 80;
    default: return -1;
  }
}

__attribute__((noinline)) static int DenseSwitchEnum(Op x) {
  switch (x) {
    case OP0: return 11;
    case OP1: return 21;
    case OP2: return 31;
    case OP3: return 41;
    case OP4: return 51;
    case OP5: return 61;
    case OP6: return 71;
    case OP7: return 81;
    default: return -1;
  }
}

__attribute__((noinline)) static int DenseSwitchTrunc(unsigned long long x) {
  switch (static_cast<int>(x)) {
    case 0: return 12;
    case 1: return 22;
    case 2: return 32;
    case 3: return 42;
    case 4: return 52;
    case 5: return 62;
    case 6: return 72;
    case 7: return 82;
    default: return -1;
  }
}

__attribute__((noinline)) static char IndexFormal(const char* p, unsigned u) {
  return *(p + u);
}

__attribute__((noinline)) static char IndexCall(const char* p) {
  unsigned u = static_cast<unsigned>(Dirty64(3));
  return *(p + u);
}

__attribute__((noinline)) static char IndexCast(const char* p,
                                                unsigned long long x) {
  int s = static_cast<int>(x);
  unsigned u = static_cast<unsigned>(s);
  return *(p + u);
}

__attribute__((noinline)) static char* AdvanceOne(char* p) {
  return p + 1;
}

__attribute__((noinline)) static float FloatFmaLike(float a, float b, float c) {
  float prod = a * b;
  return prod + c;
}

__attribute__((noinline)) static unsigned long ConstBaseSigned(int off) {
  return reinterpret_cast<unsigned long>(reinterpret_cast<char*>(0x100) + off);
}

__attribute__((noinline)) static unsigned long ConstBaseUnsigned(unsigned off) {
  return reinterpret_cast<unsigned long>(reinterpret_cast<char*>(0x100) + off);
}

__attribute__((noinline)) static char SignedFromZeroExt32(const char* mid,
                                                          unsigned long long x) {
  unsigned long long z = static_cast<unsigned>(x);
  int neg = static_cast<int>(z);
  return *(mid + neg);
}

int main() {
  const char buf[8] = "abcdefg";

  if (DenseSwitchInt(static_cast<int>(Dirty64(2))) != 30) return 1;
  if (DenseSwitchInt(static_cast<int>(Dirty64(9))) != -1) return 2;
  if (DenseSwitchEnum(static_cast<Op>(Dirty64(5))) != 61) return 3;
  if (DenseSwitchTrunc(Dirty64(4)) != 52) return 4;
  if (IndexFormal(buf, static_cast<unsigned>(Dirty64(3))) != 'd') return 5;
  if (IndexCall(buf) != 'd') return 6;
  if (IndexCast(buf, Dirty64(5)) != 'f') return 7;

  char extra[4] = {'A', 'B', 'C', 'D'};
  char* extra_next = AdvanceOne(extra);
  if (extra_next - extra != 1 || extra[0] != 'A') return 8;

  if (FloatFmaLike(2.0f, 3.5f, 1.25f) != 8.25f) return 9;
  if (ConstBaseSigned(-4) != 0xfcUL) return 10;
  if (ConstBaseUnsigned(static_cast<unsigned>(Dirty64(8))) != 0x108UL) {
    return 11;
  }
  if (SignedFromZeroExt32(buf + 4, Dirty64(0xfffffffeU)) != 'c') return 12;
  return 0;
}
