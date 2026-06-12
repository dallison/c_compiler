//
//  atan2.c
//  c_compiler
//
//  Two-argument arctangent.  atan2 is inherently the quadrant-aware
//  combination of atan(y/x) with the signs of x and y, so it is built on
//  atan() plus a +/- pi adjustment for the left half plane.
//

#include <math.h>

static const double
  kPi   = 3.14159265358979323846,
  kPio2 = 1.57079632679489661923;

double atan2(double y, double x) {
  if (x != x) return x + y;             // propagate NaN
  if (y != y) return x + y;

  if (x == 0.0) {
    if (y > 0.0) return kPio2;
    if (y < 0.0) return -kPio2;
    return 0.0;                          // atan2(0, 0)
  }

  double a = atan(y / x);
  if (x > 0.0) return a;                 // right half plane
  if (y >= 0.0) return a + kPi;          // second quadrant
  return a - kPi;                        // third quadrant
}
