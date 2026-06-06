//
//  sqrt.c
//  c_compiler
//
//  Square root by Newton-Raphson iteration.
//
//  x is reduced to a value m near 1 (x = m * 2^e with e even), sqrt(m) is
//  found with a few Newton steps, then the result is scaled by 2^(e/2).
//  Newton's iteration g <- (g + m/g) / 2 doubles the number of correct
//  digits each step, so from a guess within a factor of two only a handful
//  of iterations reach full precision.
//

#include <math.h>

#if defined(__6502__)

// On the 6502 "double" is a 32-bit float, so we cannot pick the exponent out
// of a 64-bit representation.  Reduce the argument into [0.25, 4) with pure
// arithmetic (multiplying/dividing by 4 keeps the square root scaling exact
// as a power of two) and iterate there.
double sqrt(double x) {
  if (x != x) return x;
  if (x < 0.0) { double z = x - x; return z / z; }
  if (x == 0.0) return x;

  double scale = 1.0;
  while (x >= 4.0) { x *= 0.25; scale += scale; }
  while (x < 0.25) { x *= 4.0;  scale *= 0.5; }

  double g = 0.5 * (x + 1.0);
  for (int i = 0; i < 8; i++) {
    g = 0.5 * (g + x / g);
  }
  return g * scale;
}

#else

#include <stdint.h>

double sqrt(double x) {
  if (x != x) return x;                       // NaN
  if (x < 0.0) { double z = x - x; return z / z; }  // negative -> NaN
  if (x == 0.0) return x;                     // +/- 0

  union { double f; uint64_t i; } u;
  u.f = x;
  if ((u.i >> 52) == 0x7ffULL) return x;      // +infinity

  int extra = 0;
  if (((u.i >> 52) & 0x7ffULL) == 0) {
    // Subnormal: scale up by 2^54 so it becomes normal, and remember to
    // remove half of those 54 bits from the final exponent.
    u.f = x * 18014398509481984.0;            // 2^54
    extra = -54;
  }

  int e = (int)((u.i >> 52) & 0x7ffULL) - 1023 + extra;
  // Replace the exponent field with the bias so the value lies in [1, 2).
  u.i = (u.i & 0x000fffffffffffffULL) | (1023ULL << 52);
  double m = u.f;

  if (e & 1) {        // make the exponent even
    m += m;           // m now in [2, 4)
    e -= 1;
  }

  double g = 0.5 * (m + 1.0);                 // guess within a factor of 2
  for (int i = 0; i < 5; i++) {
    g = 0.5 * (g + m / g);
  }
  return ldexp(g, e / 2);
}

#endif
