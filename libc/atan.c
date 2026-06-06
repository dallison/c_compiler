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

  // Horner evaluation of x - x^3/3 + x^5/5 - ... using a single accumulator
  // (kept as sequential statements so targets with very few floating-point
  // registers, e.g. the 6502, don't run out).
  double x2 = x * x;
  double a = 1.0 / 13.0;
  a = a * x2 - 1.0 / 11.0;
  a = a * x2 + 1.0 / 9.0;
  a = a * x2 - 1.0 / 7.0;
  a = a * x2 + 1.0 / 5.0;
  a = a * x2 - 1.0 / 3.0;
  a = a * x2 + 1.0;
  double r = x * a;
  r += offset;
  if (inv) r = kPio2 - r;
  if (neg) r = -r;
  return r;
}
