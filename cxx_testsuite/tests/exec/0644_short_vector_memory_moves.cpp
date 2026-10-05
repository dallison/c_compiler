// RUN: -std=c++20
// EXPECT_EXIT: 0

// 4- and 8-byte vectors held in SIMD registers are stored and spilled at
// their own width, so nothing next to them is overwritten; a 16-byte spill
// slot stays inside the spill area.  A vector compound literal assigned to a
// struct member is copied into it.
typedef unsigned char u8x8 __attribute__((vector_size(8)));
typedef unsigned char u8x4 __attribute__((vector_size(4)));
typedef int i32x4 __attribute__((vector_size(16)));

#define INL static inline __attribute__((always_inline))
INL u8x8 dup_inline(unsigned char v) {
  u8x8 r;
  for (int i = 0; i < 8; ++i) r[i] = v;
  return r;
}
INL u8x8 eq_inline(u8x8 a, u8x8 b) { return a == b; }

__attribute__((noinline)) u8x8 dup8(unsigned char v) { return dup_inline(v); }
__attribute__((noinline)) u8x4 dup4(unsigned char v) {
  u8x4 r;
  for (int i = 0; i < 4; ++i) r[i] = v;
  return r;
}
__attribute__((noinline)) i32x4 dup16(int v) {
  i32x4 r = {v, v, v, v};
  return r;
}
__attribute__((noinline)) u8x8 eq8(u8x8 a, u8x8 b) { return a == b; }
__attribute__((noinline)) u8x4 add4(u8x4 a, u8x4 b) { return a + b; }
__attribute__((noinline)) i32x4 add16(i32x4 a, i32x4 b) { return a + b; }

struct Framed {
  unsigned char before[8];
  u8x8 c;
  unsigned char after[8];
};

int main() {
  u8x8 c = {3, 1, 3, 1, 3, 1, 3, 1};
  u8x8 m = eq_inline(c, dup_inline(3));
  if (m[0] != 0xff || m[1] != 0 || m[2] != 0xff) return 1;

  Framed s;
  for (int i = 0; i < 8; ++i) s.before[i] = s.after[i] = 0x55;
  s.c = (u8x8){3, 1, 3, 1, 3, 1, 3, 1};
  if (s.c[0] != 3 || s.c[1] != 1) return 2;
  u8x8 m2 = eq8(s.c, dup8(3));
  if (m2[0] != 0xff || m2[1] != 0) return 3;

  u8x4 g4 = {9, 9, 9, 9};
  unsigned char guard4[4] = {7, 7, 7, 7};
  u8x4 a4 = add4(dup4(1), dup4(2));
  if (a4[3] != 3 || g4[0] != 9 || guard4[0] != 7) return 4;

  i32x4 k = {1, 2, 3, 4};
  long guard16 = 0x1234;
  i32x4 sum = add16(dup16(10), add16(k, dup16(1)));
  if (sum[0] != 12 || sum[3] != 15 || guard16 != 0x1234 || k[2] != 3) return 5;

  for (int i = 0; i < 8; ++i)
    if (s.before[i] != 0x55 || s.after[i] != 0x55) return 6;
  return 0;
}
