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

// Scale x by 2^n.  The exponent is applied in steps so that very large or
// very small n still overflow to infinity / underflow to zero (or a
// subnormal) the way the hardware would, instead of building an invalid
// power of two.
double ldexp(double x, int n) {
  if (x == 0.0) return x;             // zero
  if (x != x) return x;               // NaN
  if (x - x != 0.0) return x;         // +/- infinity
  union { double f; uint64_t i; } pow2;
  while (n > 1023) {
    pow2.i = (uint64_t)(1023 + 1023) << 52;
    x *= pow2.f;
    n -= 1023;
  }
  while (n < -1022) {
    pow2.i = (uint64_t)(1023 - 1022) << 52;
    x *= pow2.f;
    n += 1022;
  }
  pow2.i = (uint64_t)(1023 + n) << 52;
  return x * pow2.f;
}
#endif

