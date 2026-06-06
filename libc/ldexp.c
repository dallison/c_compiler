//
//  ldexp.c
//  c_compiler
//
//  Created by David Allison on 12/26/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <math.h>

// On the 6502 we have an assembly version of this.
#if !defined(__6502__)
#include <stdint.h>

// Construct 2^n as a double by writing the IEEE-754 biased exponent field
// directly.  Only valid for a normal exponent: 1 <= 1023 + n <= 2046.
static double __pow2(int n) {
  union { double f; uint64_t i; } u;
  u.i = (uint64_t)(1023 + n) << 52;
  return u.f;
}

// Scale x by 2^n.  The exponent is applied in steps so that very large or
// very small n still overflow to infinity / underflow to zero (or a
// subnormal) the way the hardware would, instead of building an invalid
// power of two.
double ldexp(double x, int n) {
  if (x == 0.0 || x != x) return x;   // zero or NaN
  if (x - x != 0.0) return x;         // +/- infinity
  while (n > 1023) {
    x *= __pow2(1023);
    n -= 1023;
  }
  while (n < -1022) {
    x *= __pow2(-1022);
    n += 1022;
  }
  return x * __pow2(n);
}
#endif

