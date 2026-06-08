//
//  asin.c
//  c_compiler
//
//  Arcsine.  For |x| <= 0.5 the series asin(x) = x + x^3*R(x^2) is used,
//  where R is a rational minimax approximation of (asin(x) - x) / x^3.  For
//  larger |x| the series converges too slowly, so the identity
//      asin(x) = pi/2 - 2*asin(sqrt((1 - x) / 2))
//  folds the argument back into [0, 0.5] where the same R applies.
//

#include <math.h>

static const double
  kPio2 = 1.57079632679489661923,
  pS0 =  1.66666666666666657415e-01,
  pS1 = -3.25565818622400915405e-01,
  pS2 =  2.01212532134862925881e-01,
  pS3 = -4.00555345006794114027e-02,
  pS4 =  7.91534994289814532176e-04,
  pS5 =  3.47933107596021167570e-05,
  qS1 = -2.40339491173441421878e+00,
  qS2 =  2.02094576023350569471e+00,
  qS3 = -6.88283971605453293030e-01,
  qS4 =  7.70381505559019352791e-02;

// R(t) where t = z^2; asin(z) ~= z + z*R(t) for small z.  Evaluated with
// Horner's method using single accumulators to keep floating-point register
// pressure low (important on targets such as the 6502).
static double __asin_r(double t) {
  volatile double vt = t;
  double p = pS5;
  p = p * vt + pS4;
  p = p * vt + pS3;
  p = p * vt + pS2;
  p = p * vt + pS1;
  p = p * vt + pS0;
  p = p * vt;
  double q = qS4;
  q = q * vt + qS3;
  q = q * vt + qS2;
  q = q * vt + qS1;
  q = q * vt + 1.0;
  return p / q;
}

double asin(double x) {
  if (x != x) return x;                         // NaN
  if (x > 1.0 || x < -1.0) { double z = x - x; return z / z; }
  return atan2(x, sqrt(1.0 - x * x));
}
