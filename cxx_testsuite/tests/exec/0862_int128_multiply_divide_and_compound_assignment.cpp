// __int128 arithmetic that abseil's hashing and uint128 rely on.  Mix() in
// absl/hash computes `uint128 m = lhs; m *= rhs;`, which is a compound multiply
// by a 64-bit value; its result must not depend on whatever the registers held.
// The full product must carry between the 32-bit partial products, and
// division and remainder must be computed rather than returning the dividend,
// and a negative narrower value widens with a high word of all ones.
#include <stdint.h>

typedef unsigned __int128 u128;
typedef __int128 s128;

__attribute__((noinline)) uint64_t Opaque(uint64_t v) { return v; }

static u128 Make(uint64_t hi, uint64_t lo) { return ((u128)hi << 64) | lo; }

__attribute__((noinline)) uint64_t Mix(uint64_t lhs, uint64_t rhs) {
  u128 m = lhs;
  m *= rhs;
  return (uint64_t)(m >> 64) ^ (uint64_t)m;
}

int main() {
  uint64_t a = Opaque(0x123456789abcdef0ull);
  uint64_t b = Opaque(0xdcb22ca68cb134edull);
  uint64_t ones = Opaque(~0ull);
  u128 big = ((u128)a << 64) | b;
  u128 big2 = ((u128)b << 64) | a;

  if ((u128)ones * ones != Make(0xfffffffffffffffeull, 0x0000000000000001ull)) return 1;
  if ((u128)a * b != Make(0x0fb1a5e09ba3ecf3ull, 0x49b0a33702132430ull)) return 2;
  if (big * big2 != Make(0x547c165b00f4315cull, 0x49b0a33702132430ull)) return 3;
  u128 m = a;
  m *= b;
  if (m != Make(0x0fb1a5e09ba3ecf3ull, 0x49b0a33702132430ull)) return 4;
  u128 full = big;
  full *= big2;
  if (full != Make(0x547c165b00f4315cull, 0x49b0a33702132430ull)) return 5;
  if (Mix(a, b) != (0x0fb1a5e09ba3ecf3ull ^ 0x49b0a33702132430ull)) return 6;
  u128 sum = big;
  sum += b;
  sum -= a;
  if (sum != Make(0x123456789abcdef1ull, 0xa73002d47ea58aeaull)) return 7;
  u128 shifted = big;
  shifted <<= 77;
  shifted >>= 19;
  if (shifted != Make(0x000008b29a32c4d3ull, 0xb400000000000000ull)) return 8;
  s128 negative = -(s128)big;
  negative >>= 3;
  if ((u128)negative != Make(0xfdb97530eca86421ull, 0xe469ba6b2e69d962ull)) return 9;
  uint64_t narrow = a;
  narrow *= (u128)b;
  narrow += big2;
  if (narrow != 0x5be4f9af9cd00320ull) return 10;

  if (big2 / big != Make(0x0000000000000000ull, 0x000000000000000cull)) return 11;
  if (big2 % big != Make(0x023e1eff4bd6c1a2ull, 0xb9da3eaa026e63d4ull)) return 12;
  if (big / b != Make(0x0000000000000000ull, 0x151dd62887e8444bull)) return 13;
  if (big % b != Make(0x0000000000000000ull, 0x07ba83f350efbf7eull)) return 14;
  s128 divisor = (s128)Opaque(1000003);
  if ((u128)(-(s128)big / divisor) != Make(0xfffffece94bdb435ull, 0xce62730eeca110fdull)) return 15;
  if ((u128)(-(s128)big % divisor) != Make(0xffffffffffffffffull, 0xfffffffffff31edcull)) return 16;
  if ((u128)((s128)big / -7) != Make(0xfd663cca33099702ull, 0x299d6755ebe6af4cull)) return 17;
  if ((u128)((s128)big % -7) != Make(0x0000000000000000ull, 0x0000000000000001ull)) return 18;
  u128 quotient = big;
  quotient /= (b | 3);
  quotient %= (a | 5);
  s128 constant = -7;
  if ((uint64_t)(constant >> 64) != ~0ull) return 20;
  long negative_long = (long)Opaque((uint64_t)-3);
  short negative_short = (short)Opaque((uint64_t)-5);
  signed char negative_char = (signed char)Opaque((uint64_t)-6);
  s128 from_long = negative_long, from_short = negative_short,
       from_char = negative_char;
  if (from_long != -3 || (uint64_t)(from_long >> 64) != ~0ull) return 21;
  if (from_short != -5 || (uint64_t)(from_short >> 64) != ~0ull) return 22;
  if (from_char != -6 || (uint64_t)(from_char >> 64) != ~0ull) return 23;
  if (quotient != Make(0x0000000000000000ull, 0x02e97fafed2b6555ull)) return 19;
  return 0;
}
