//
//  ceil.c
//  c_compiler
//
//  Smallest integral value not less than x.
//

#include <math.h>

double ceil(double x) {
  if (x != x) {
    return x;
  }
  if (x - x != 0.0) {
    return x;
  }

  double ipart;
  double fract = modf(x, &ipart);
  if (fract > 0.0) {
    return ipart + 1.0;
  }
  return ipart;
}
