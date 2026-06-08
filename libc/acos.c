//
//  acos.c
//  c_compiler
//
//  Arccosine.  Uses the same rational core R as asin (asin(z) ~= z + z*R(z^2))
//  with three ranges:
//    |x| < 0.5 : acos(x) = pi/2 - (x + x*R(x^2))
//    x >= 0.5  : acos(x) = 2*asin(sqrt((1 - x)/2))
//    x <= -0.5 : acos(x) = pi - 2*asin(sqrt((1 + x)/2))
//  The reflections keep the rational argument inside [0, 0.5].
//

#include <math.h>

static const double
  kPio2 = 1.57079632679489661923,
  kPi   = 3.14159265358979323846,
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

// Evaluated with Horner's method using single accumulators to keep
// floating-point register pressure low (important on targets such as the 6502).
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

double acos(double x) {
  if (x != x) return x;                              // NaN
  if (x > 1.0 || x < -1.0) { double z = x - x; return z / z; }  // domain -> NaN
  return atan2(sqrt(1.0 - x * x), x);
}
