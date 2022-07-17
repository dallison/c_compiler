//
//  div.c
//  c_compiler
//
//  Created by David Allison on 6/17/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <stdlib.h>

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

