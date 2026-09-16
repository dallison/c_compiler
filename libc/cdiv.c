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

#if defined(__arm__) || defined(__i386__)
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

unsigned long long __ashldi3(unsigned long long value, int count) {
  count &= 63;
  while (count-- != 0) {
    value <<= 1;
  }
  return value;
}

unsigned long long __lshrdi3(unsigned long long value, int count) {
  count &= 63;
  while (count-- != 0) {
    value >>= 1;
  }
  return value;
}

long long __ashrdi3(long long value, int count) {
  count &= 63;
  while (count-- != 0) {
    value >>= 1;
  }
  return value;
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
#endif  // __arm__ || __i386__

