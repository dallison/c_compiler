//
//  floor.c
//  c_compiler
//
//  Largest integral value not greater than x.
//

#include <math.h>

double floor(double x) {
  if (x != x) {
    return x;
  }
  if (x - x != 0.0) {
    return x;
  }

  double ipart;
  double fract = modf(x, &ipart);
  if (fract < 0.0) {
    return ipart - 1.0;
  }
  return ipart;
}
