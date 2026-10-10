// Converting a 32-bit integer to floating point must use only its 32-bit
// value, whatever arithmetic produced it.
unsigned A = 0xffffffffu, B = 0x80000000u;
int Q = 0x40000000;
long L = 0x1fffffffbL;

__attribute__((noinline)) double NegToDouble(unsigned a) {
  return (double)(-a);
}
__attribute__((noinline)) float NegToFloat(unsigned a) { return (float)(-a); }
__attribute__((noinline)) double NotToDouble(unsigned a) {
  return (double)(~a);
}
__attribute__((noinline)) double ShlToDouble(unsigned a) {
  return (double)(a << 1);
}
__attribute__((noinline)) double SignedShlToDouble(int q) {
  return (double)(q << 1);
}
__attribute__((noinline)) double NarrowToDouble(long l) {
  return (double)(int)l;
}
__attribute__((noinline)) double NarrowUnsignedToDouble(long l) {
  return (double)(unsigned)l;
}

int main() {
  if (NegToDouble(A) != 1.0) return 1;
  if (NegToFloat(A) != 1.0f) return 2;
  if (NotToDouble(B) != 2147483647.0) return 3;
  if (ShlToDouble(B) != 0.0) return 4;
  if (SignedShlToDouble(Q) != -2147483648.0) return 5;
  if (NarrowToDouble(L) != -5.0) return 6;
  if (NarrowUnsignedToDouble(L) != 4294967291.0) return 7;
  return 0;
}
