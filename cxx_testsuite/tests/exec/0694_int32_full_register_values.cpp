// 32-bit integer results that a 64-bit register can hold in more than one
// form: negation, complement and left shift of unsigned values, signed left
// shift into the sign bit, conversions that narrow or change signedness, and
// constants converted to unsigned.  Each feeds an operation that reads the
// whole register: comparison, division, right shift or widening.
unsigned A = 0xffffffffu, H = 0xf0000001u, Z = 0, U77 = 0xffffffb3u;
int I78 = -78, Q = 0x40000000, NEG = -5;
long L5 = 0x100000005L, LM5 = 0x1fffffffbL, L6 = 0x100000006L;

__attribute__((noinline)) int EqNegConst(unsigned u) { return u == -77; }
__attribute__((noinline)) int EqCastConst(unsigned u) {
  return u == (unsigned)-77;
}
__attribute__((noinline)) int EqMixed(unsigned u, int i) { return u == i; }
__attribute__((noinline)) int NegU(unsigned a) { return -a == 1u; }
__attribute__((noinline)) int NotU(unsigned a) { return ~a == 0u; }
__attribute__((noinline)) int ShlU(unsigned a) { return (a << 4) == 0x10u; }
__attribute__((noinline)) int ShlS(int i) { return (i << 1) < 0; }
__attribute__((noinline)) int SubWrap(unsigned a) {
  return a - 1u == 0xffffffffu;
}
__attribute__((noinline)) int CastSum(unsigned u, int i) {
  return u == (unsigned)(i + 1);
}
__attribute__((noinline)) int CastToInt(unsigned u, int i) {
  return (int)u == i + 1;
}
__attribute__((noinline)) int NarrowU(long l) { return (unsigned)l == 5u; }
__attribute__((noinline)) int NarrowS(long l) { return (int)l == -5; }
__attribute__((noinline)) unsigned DivNeg(unsigned a) { return (-a) / 1u; }
__attribute__((noinline)) unsigned ShrNot(unsigned a) { return (~a | 0u) >> 1; }
__attribute__((noinline)) unsigned DivNarrow(long l) {
  return ((unsigned)l) / 3u;
}
__attribute__((noinline)) long WidenNeg(unsigned a) { return (long)(-a); }
__attribute__((noinline)) int HalveShl(int q) { return (q << 1) / 2; }

int main() {
  unsigned u = U77;
  if (!(u == -77)) return 1;
  if (!EqNegConst(U77)) return 2;
  if (!EqCastConst(U77)) return 3;
  if (!EqMixed(U77, -77)) return 4;
  if (!NegU(A)) return 5;
  if (!NotU(A)) return 6;
  if (!ShlU(H)) return 7;
  if (!ShlS(Q)) return 8;
  if (!SubWrap(Z)) return 9;
  if (!CastSum(U77, I78)) return 10;
  if (!CastToInt(U77, I78)) return 11;
  if (!NarrowU(L5)) return 12;
  if (!NarrowS(LM5)) return 13;
  if (DivNeg(A) != 1u) return 14;
  if (ShrNot(A) != 0u) return 15;
  if (DivNarrow(L6) != 2u) return 16;
  if (WidenNeg(A) != 1L) return 17;
  if (HalveShl(Q) != -0x40000000) return 18;
  if (-NEG != 5) return 19;
  return 0;
}
