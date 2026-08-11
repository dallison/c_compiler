//
//  log.c
//  c_compiler
//
//  Natural logarithm.
//

#include <math.h>

static const double kLn2Hi = 6.93147180369123816490e-01;
static const double kLn2Lo = 1.90821492927058770002e-10;

static double __log1p(double f) {
  double z = f / (2.0 + f);
  double z2 = z * z;
  double term = z;
  double sum = z;
  for (int n = 3; n <= 21; n += 2) {
    term *= z2;
    sum += term / (double)n;
  }
  return 2.0 * sum;
}

double log(double x) {
  if (x != x) {
    return x;
  }
  if (x < 0.0) {
    double z = x - x;
    return z / z;
  }
  if (x == 0.0) {
    double z = 0.0;
    return -1.0 / z;
  }

  int e;
  double m = frexp(x, &e);
  double ln2_term = (double)e * kLn2Hi + (double)e * kLn2Lo;
  return ln2_term + __log1p(m - 1.0);
}
