//
//  exp.c
//  c_compiler
//
//  Natural exponential.  x is reduced to r in [-ln(2)/2, ln(2)/2] plus an
//  integer power of two, then exp(r) is evaluated with a minimax polynomial.
//

#include <math.h>

static const double kLn2Hi = 6.93147180369123816490e-01;
static const double kLn2Lo = 1.90821492927058770002e-10;
static const double kInvLn2 = 1.44269504088896340736;
static const double kExpP0 = 1.9875691500e-4;
static const double kExpP1 = 1.3981999507e-3;
static const double kExpP2 = 8.3334519073e-3;
static const double kExpP3 = 4.1665795894e-2;
static const double kExpP4 = 1.6666665459e-1;
static const double kExpP5 = 5.0000001201e-1;

double exp(double x) {
  if (x != x) {
    return x;
  }
  if (x == 0.0) {
    return 1.0;
  }
#if DBL_MAX_EXP > 200
  if (x > 709.782712893384) {
    return x * x;
  }
  if (x < -745.133219101941) {
    return 0.0;
  }
#else
  if (x > 88.0) {
    return x * x;
  }
  if (x < -103.0) {
    return 0.0;
  }
#endif

  int k = (int)floor(x * kInvLn2 + 0.5);
  double ln2_term = (double)k * kLn2Hi + (double)k * kLn2Lo;
  double r = x - ln2_term;
  double r2 = r * r;
  double p = kExpP0;
  p = p * r + kExpP1;
  p = p * r + kExpP2;
  p = p * r + kExpP3;
  p = p * r + kExpP4;
  p = p * r + kExpP5;
  p = p * r2 + r + 1.0;
  {
    double scale = p;
    int exponent = k;
    return ldexp(scale, exponent);
  }
}
