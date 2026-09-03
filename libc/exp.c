//
//  exp.c
//  c_compiler
//
//  Natural exponential.  x is reduced to r in [-ln(2)/2, ln(2)/2] plus an
//  integer power of two, then exp(r) is evaluated with a minimax polynomial.
//

#include <math.h>

#define kLn2Hi 6.93147180369123816490e-01
#define kLn2Lo 1.90821492927058770002e-10
#define kInvLn2 1.44269504088896340736

double exp(double x) {
  double scaled;
  double remainder;
  double term;
  double result;
  int k;
  int i;
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

  scaled = x * kInvLn2;
  k = (int)scaled;
  if (scaled < 0.0 && (double)k != scaled) k--;
  remainder = x - (double)k * kLn2Hi - (double)k * kLn2Lo;
  term = 1.0;
  result = 1.0;
  for (i = 1; i <= 18; i++) {
    term *= remainder / (double)i;
    result += term;
  }
  while (k > 0) {
    result *= 2.0;
    k--;
  }
  while (k < 0) {
    result *= 0.5;
    k++;
  }
  return result;
}
