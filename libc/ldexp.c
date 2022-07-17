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
double ldexp(double x, int exp) {
  return x;     // TODO
}
#endif

