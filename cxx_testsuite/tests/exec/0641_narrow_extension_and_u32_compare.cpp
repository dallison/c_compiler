// RUN: -std=c++20
// EXPECT_EXIT: 0

// Zero/sign extension from 8/16/32 bits and 32-bit unsigned comparisons of
// values whose upper register bits differ (64-bit targets keep 32-bit values
// in wider registers).
__attribute__((noinline)) unsigned long long zext8(unsigned long long x) {
  return (unsigned char)x;
}
__attribute__((noinline)) unsigned long long zext16(unsigned long long x) {
  return (unsigned short)x;
}
__attribute__((noinline)) unsigned long long zext32(unsigned long long x) {
  return (unsigned int)x;
}
__attribute__((noinline)) long long sext8(long long x) { return (signed char)x; }
__attribute__((noinline)) long long sext16(long long x) { return (short)x; }
__attribute__((noinline)) long long sext32(long long x) { return (int)x; }
__attribute__((noinline)) bool ult32(unsigned a, unsigned b) { return a < b; }
__attribute__((noinline)) bool eq32(unsigned a, unsigned b) { return a == b; }

int main() {
  if (zext8(0x1234567890abcdefULL) != 0xefULL) return 1;
  if (zext16(0x1234567890abcdefULL) != 0xcdefULL) return 2;
  if (zext32(0x1234567890abcdefULL) != 0x90abcdefULL) return 3;
  if (sext8(0x1ff) != -1) return 4;
  if (sext16(0x18000) != -32768) return 5;
  if (sext32(0x180000000LL) != (long long)(int)0x80000000u) return 6;
  unsigned big = 0x80000000u;
  unsigned small = 1;
  if (!ult32(small, big)) return 7;
  if (ult32(big, small)) return 8;
  if (!eq32(big + big, 0)) return 9;
  unsigned x = 0xffffffffu;
  if (!ult32(x + 2, 2u)) return 10;
  return 0;
}
