//
//  tan.c
//  c_compiler
//
//  Tangent computed from its own series rather than sin/cos.
//
//  x is reduced to x = k*(pi/2) + r with r in [-pi/4, pi/4].  pi/2 is carried
//  in two pieces (hi + lo) so the reduction stays accurate.  tan(r) is then
//  obtained from a half-angle: t = tan(r/2) is evaluated with the Taylor
//  series tan(u) = u + u^3/3 + 2u^5/15 + ... (whose argument lies in
//  [-pi/8, pi/8], giving ~1e-9 accuracy), and tan(r) = 2t / (1 - t^2).  For
//  odd k the angle sits in the next quadrant where tan(x) = -cot(r) = -1/tan(r).
//

#include <math.h>

static const double
  kPio2_hi = 1.57079632679489655800e+00,
  kPio2_lo = 6.12323399573676603587e-17,
  k2_pi    = 0.63661977236758134308;   // 2/pi

// tan(u) for |u| <= pi/8 via its Maclaurin series, evaluated with a running
// term to keep original-argument live ranges short.
static double __tan_small(double u) {
  double u2 = u * u;
  double term = u;
  double result = u;
  term *= u2; result += term / 3.0;
  term *= u2; result += (2.0 * term) / 15.0;
  term *= u2; result += (17.0 * term) / 315.0;
  term *= u2; result += (62.0 * term) / 2835.0;
  term *= u2; result += (1382.0 * term) / 155925.0;
  term *= u2; result += (21844.0 * term) / 6081075.0;
  return result;
}

double tan(double x) {
  int neg = 0;
  if (x != x) return x;                          // NaN
  if (x - x != 0.0) { double z = x - x; return z / z; }  // infinity -> NaN
  if (x < 0.0) { x = -x; neg = 1; }

  // Reduce to r in [-pi/4, pi/4].  Capture the quadrant parity as a small int
  // and finish using kd so the 64-bit k need not stay live across the kernel
  // call (keeps register pressure manageable on the 6502).
  long long k = (long long)(x * k2_pi + 0.5);
  int odd = (int)(k & 1);
  double kd = (double)k;
  double r = x - kd * kPio2_hi;
  r = r - kd * kPio2_lo;

  double t = __tan_small(r * 0.5);
  double denom = 1.0 - t * t;
  double tr = (t + t) / denom;                   // tan(r)

  double result = odd ? -1.0 / tr : tr;          // odd quadrant: -cot(r)
  return neg ? -result : result;
}
