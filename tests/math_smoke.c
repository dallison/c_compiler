//
//  Return-code smoke test for libc math used by <random>.
//

#include <math.h>

static int approx(double actual, double expected, double tolerance) {
  double delta = actual - expected;
  if (delta < 0.0) {
    delta = -delta;
  }
  return delta <= tolerance ? 0 : 1;
}

int main(void) {
#if defined(__6502__)
  const double tolerance = 2e-2;
#else
  const double tolerance = 5e-3;
#endif

  if (approx(floor(3.7), 3.0, tolerance) != 0) return 10;
  if (approx(ceil(3.2), 4.0, tolerance) != 0) return 11;
  if (approx(floor(-3.2), -4.0, tolerance) != 0) return 12;
  if (approx(ceil(-3.7), -3.0, tolerance) != 0) return 13;

  if (approx(exp(0.0), 1.0, tolerance) != 0) return 20;
  if (approx(log(1.0), 0.0, tolerance) != 0) return 21;
  if (approx(log(2.0), 0.69314718055994530942, tolerance) != 0) return 22;

  if (approx(pow(2.0, 3.0), 8.0, tolerance) != 0) return 30;
  if (approx(sqrt(4.0), 2.0, tolerance) != 0) return 31;

  int exponent = 0;
  if (approx(frexp(8.0, &exponent), 0.5, tolerance) != 0) return 40;
  if (exponent != 4) return 41;

  return 0;
}
