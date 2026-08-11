//
//  pow.c
//  c_compiler
//
//  x raised to the power y.
//

#include <math.h>

double pow(double x, double y) {
  if (y == 0.0) {
    return 1.0;
  }
  if (x != x || y != y) {
    return x + y;
  }
  if (x == 0.0) {
    if (y > 0.0) {
      return 0.0;
    }
    double z = 0.0;
    return 1.0 / z;
  }
  if (x < 0.0) {
    double ipart;
    if (modf(y, &ipart) != 0.0) {
      double z = 0.0;
      return z / z;
    }
    double ax = -x;
    double result = exp(y * log(ax));
    if (((int)ipart) & 1) {
      return -result;
    }
    return result;
  }
  return exp(y * log(x));
}
