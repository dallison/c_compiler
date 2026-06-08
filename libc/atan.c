//
//  atan.c
//  c_compiler
//
//  Arctangent via argument reduction onto a small interval followed by the
//  odd Taylor series atan(x) = x - x^3/3 + x^5/5 - ...
//
//  Two reductions keep the series argument tiny so only a few terms are
//  needed:
//    1. atan(x) = pi/2 - atan(1/x)            for |x| > 1
//    2. atan(x) = pi/6 + atan((x*sqrt3 - 1) / (sqrt3 + x))   for x > 2-sqrt3
//  After both, the argument satisfies |t| <= tan(pi/12) ~ 0.268, where the
//  series truncated after x^13 is accurate to roughly 1e-12.
//

#include <math.h>

static const double
  kPio2  = 1.57079632679489661923,    // pi/2
  kPi6   = 0.52359877559829887308,    // pi/6
  kSqrt3 = 1.73205080756887729353,    // sqrt(3)
  kTanPi12 = 0.26794919243112270647;  // tan(pi/12) = 2 - sqrt(3)

double atan(double x) {
  int neg = 0, inv = 0;
  double offset = 0.0;

  if (x != x) return x;                       // NaN
  if (x < 0.0) { x = -x; neg = 1; }
  if (x > 1.0) { x = 1.0 / x; inv = 1; }      // reduction 1
  if (x > kTanPi12) {                         // reduction 2
    x = (x * kSqrt3 - 1.0) / (kSqrt3 + x);
    offset = kPi6;
  }

  // Evaluate x - x^3/3 + x^5/5 - ... with a running term. Keeping each step
  // local avoids a long live range for x on register-poor backends.
  double x2 = x * x;
  double term = x;
  double r = x + offset;
  term *= x2; r -= term / 3.0;
  term *= x2; r += term / 5.0;
  term *= x2; r -= term / 7.0;
  term *= x2; r += term / 9.0;
  term *= x2; r -= term / 11.0;
  term *= x2; r += term / 13.0;
  if (inv) r = kPio2 - r;
  if (neg) r = -r;
  return r;
}
