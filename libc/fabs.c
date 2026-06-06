//
//  fabs.c
//  c_compiler
//
//  Absolute value of a double.
//

#include <math.h>

#if defined(__6502__)
double fabs(double x) {
  return x < 0.0 ? -x : x;
}
#else
#include <stdint.h>

// Clearing the sign bit is exact and also normalises -0.0 to +0.0.
double fabs(double x) {
  union { double f; uint64_t i; } u = {x};
  u.i &= ~(1ULL << 63);
  return u.f;
}
#endif
