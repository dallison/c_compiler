//
//  modf.c
//  c_compiler
//
//  Created by David Allison on 12/24/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <math.h>

// On the 6502 we have an assembly version of this.
#if !defined(__6502__)
#include <stdint.h>

// This comes from Sun's library.
double modf(double x, double *iptr) {
  union {double f; uint64_t i;} u = {x};
  uint64_t mask;
  int e = (int)(u.i>>52 & 0x7ff) - 0x3ff;

  /* no fractional part */
  if (e >= 52) {
    *iptr = x;
    if (e == 0x400 && u.i<<12 != 0) /* nan */
      return x;
    u.i &= 1ULL<<63;
    return u.f;
  }

  /* no integral part*/
  if (e < 0) {
    u.i &= 1ULL<<63;
    *iptr = u.f;
    return x;
  }

  mask = -1ULL>>12>>e;
  if ((u.i & mask) == 0) {
    *iptr = x;
    u.i &= 1ULL<<63;
    return u.f;
  }
  u.i &= ~mask;
  *iptr = u.f;
  return x - u.f;
}
#endif

