//
//  div.c
//  c_compiler
//
//  Created by David Allison on 6/17/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <stdlib.h>

#if 0
// Integer division as a single operation.

div_t div(int numer, int denom) {
#if defined(__6502__)
  extern div_t __cdivmod2(int, int);
  return __cdivmod2(numer, denom);
#else
  // Default, non-optimized case.
  div_t r;
  r.quot = numer / denom;
  r.rem = numer % denom;
  return r;
#endif
}

ldiv_t ldiv(long int numer, long int denom) {
#if defined(__6502__)
  extern ldiv_t __cdivmod4(long int, long int);
  return __cdivmod4(numer, denom);
#else
  ldiv_t r;
  // Default, non-optimized case.
  r.quot = numer / denom;
  r.rem = numer % denom;
  return r;
#endif
}
#endif

lldiv_t lldiv(long long int numer, long long int denom) {
#if defined(__6502__)
  extern lldiv_t __cdivmod8(long long int, long long int);
  return __cdivmod8(numer, denom);
#else
  lldiv_t r;
  // Default, non-optimized case.
  r.quot = numer / denom;
  r.rem = numer % denom;
  return r;
#endif
}

#if defined(__arm__) || (defined(__risc_v__) && defined(__ILP32__))
// 32-bit ARM has no hardware 64-bit multiply, divide or modulo.  The code
// generator lowers those operations into calls to the libgcc-style runtime
// helpers below.  These implementations deliberately use only 64-bit add,
// subtract, shift, compare and bitwise operations (all of which the backend
// expands into 32-bit instruction sequences); they never use the C `*`, `/`
// or `%` operators on 64-bit values, which would recurse back into these very
// helpers.
#include <stdint.h>

unsigned long long __muldi3(unsigned long long a, unsigned long long b) {
  unsigned long long result = 0;
  while (b != 0) {
    if (b & 1ULL) {
      result += a;
    }
    a <<= 1;
    b >>= 1;
  }
  return result;
}

// Unsigned 64-bit division returning the quotient and, optionally, the
// remainder.  Classic restoring shift/subtract long division.
unsigned long long __udivmoddi4(unsigned long long n, unsigned long long d,
                                unsigned long long* rem) {
  unsigned long long q = 0;
  unsigned long long r = 0;
  for (int i = 63; i >= 0; i--) {
    r <<= 1;
    r |= (n >> i) & 1ULL;
    if (r >= d) {
      r -= d;
      q |= (1ULL << i);
    }
  }
  if (rem != 0) {
    *rem = r;
  }
  return q;
}

unsigned long long __udivdi3(unsigned long long a, unsigned long long b) {
  return __udivmoddi4(a, b, 0);
}

unsigned long long __umoddi3(unsigned long long a, unsigned long long b) {
  unsigned long long r;
  __udivmoddi4(a, b, &r);
  return r;
}

long long __divdi3(long long a, long long b) {
  int negate = 0;
  unsigned long long ua, ub;
  if (a < 0) {
    ua = (unsigned long long)(-a);
    negate ^= 1;
  } else {
    ua = (unsigned long long)a;
  }
  if (b < 0) {
    ub = (unsigned long long)(-b);
    negate ^= 1;
  } else {
    ub = (unsigned long long)b;
  }
  unsigned long long q = __udivmoddi4(ua, ub, 0);
  return negate ? -(long long)q : (long long)q;
}

long long __moddi3(long long a, long long b) {
  int negate = 0;
  unsigned long long ua, ub;
  if (a < 0) {
    ua = (unsigned long long)(-a);
    negate = 1;
  } else {
    ua = (unsigned long long)a;
  }
  if (b < 0) {
    ub = (unsigned long long)(-b);
  } else {
    ub = (unsigned long long)b;
  }
  unsigned long long r;
  __udivmoddi4(ua, ub, &r);
  return negate ? -(long long)r : (long long)r;
}

#if defined(__risc_v__) && defined(__ILP32__)
unsigned long long __ashldi3(unsigned long long a, int n) {
  unsigned alo = (unsigned)a;
  unsigned ahi = (unsigned)(a >> 32);
  n &= 63;
  unsigned lo, hi;
  if (n == 0) {
    return a;
  }
  if (n >= 32) {
    lo = 0;
    hi = alo << (n - 32);
  } else {
    lo = alo << n;
    hi = (ahi << n) | (alo >> (32 - n));
  }
  return ((unsigned long long)hi << 32) | lo;
}

unsigned long long __lshrdi3(unsigned long long a, int n) {
  unsigned alo = (unsigned)a;
  unsigned ahi = (unsigned)(a >> 32);
  n &= 63;
  unsigned lo, hi;
  if (n == 0) {
    return a;
  }
  if (n >= 32) {
    hi = 0;
    lo = ahi >> (n - 32);
  } else {
    hi = ahi >> n;
    lo = (alo >> n) | (ahi << (32 - n));
  }
  return ((unsigned long long)hi << 32) | lo;
}

long long __ashrdi3(long long a, int n) {
  unsigned alo = (unsigned)a;
  int ahi = (int)(unsigned)((unsigned long long)a >> 32);
  n &= 63;
  unsigned lo;
  int hi;
  if (n == 0) {
    return a;
  }
  if (n >= 32) {
    hi = ahi >> 31;
    lo = (unsigned)(ahi >> (n - 32));
  } else {
    hi = ahi >> n;
    lo = (alo >> n) | ((unsigned)ahi << (32 - n));
  }
  return (long long)(((unsigned long long)(unsigned)hi << 32) | lo);
}

long long __davecc_double_to_i64(double value) {
  union {
    double value;
    unsigned long long bits;
  } input;
  input.value = value;
  unsigned sign = (unsigned)(input.bits >> 63);
  unsigned exponent = (unsigned)((input.bits >> 52) & 0x7ff);
  if (exponent == 0x7ff) {
    return sign ? (long long)0x8000000000000000ULL
                : (long long)0x7fffffffffffffffULL;
  }
  int e = (int)exponent - 1023;
  if (e < 0) {
    return 0;
  }
  if (e > 63 || (e == 63 && sign == 0)) {
    return sign ? (long long)0x8000000000000000ULL
                : (long long)0x7fffffffffffffffULL;
  }
  unsigned long long magnitude =
      (input.bits & ((1ULL << 52) - 1)) | (1ULL << 52);
  if (e >= 52) {
    magnitude <<= e - 52;
  } else {
    magnitude >>= 52 - e;
  }
  return sign ? (long long)(0ULL - magnitude) : (long long)magnitude;
}

unsigned long long __davecc_double_to_u64(double value) {
  union {
    double value;
    unsigned long long bits;
  } input;
  input.value = value;
  unsigned sign = (unsigned)(input.bits >> 63);
  unsigned exponent = (unsigned)((input.bits >> 52) & 0x7ff);
  if (sign != 0 || (int)exponent - 1023 < 0) {
    return 0;
  }
  int e = (int)exponent - 1023;
  if (exponent == 0x7ff || e > 63) {
    return 0xffffffffffffffffULL;
  }
  unsigned long long magnitude =
      (input.bits & ((1ULL << 52) - 1)) | (1ULL << 52);
  if (e >= 52) {
    magnitude <<= e - 52;
  } else {
    magnitude >>= 52 - e;
  }
  return magnitude;
}
#endif
#endif  // __arm__ || RV32 ILP32

