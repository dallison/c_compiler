//
//  frexp.c
//  c_compiler
//
//  Decompose x into a normalized mantissa in [0.5, 1) and a power of two.
//

#include <math.h>
#include <stdint.h>
#include <float.h>

#if DBL_MANT_DIG == FLT_MANT_DIG

static double float_bits_to_double(uint32_t bits) {
  union {
    float f;
    uint32_t i;
  } u = {.i = bits};
  return (double)u.f;
}

double frexp(double x, int *exp) {
  union {
    float f;
    uint32_t i;
  } u = {(float)x};
  uint32_t bits = u.i;
  int e = (int)((bits >> 23) & 0xff);

  if ((bits & 0x7fffffffu) == 0) {
    *exp = 0;
    return x;
  }
  if (e == 255) {
    *exp = 0;
    return x;
  }

  if (e == 0) {
    uint32_t mant = bits & 0x007fffffu;
    while ((mant & 0x00800000u) == 0) {
      mant <<= 1;
      e -= 1;
    }
    e = 1;
    bits = (bits & 0x80000000u) | mant;
  }

  *exp = e - 126;
  bits = (bits & 0x807fffffu) | ((uint32_t)126 << 23);
  return float_bits_to_double(bits);
}

#else

double frexp(double x, int *exp) {
  union {
    double f;
    uint64_t i;
  } u = {x};

  if (u.i == 0 || (u.i << 1) == 0) {
    *exp = 0;
    return x;
  }
  if ((u.i >> 52) == 0x7ffULL) {
    *exp = 0;
    return x;
  }

  int e = (int)((u.i >> 52) & 0x7ffULL);
  if (e == 0) {
    u.f = x * 18014398509481984.0;
    e = (int)((u.i >> 52) & 0x7ffULL) - 1075;
  } else {
    e -= 1022;
    u.i = (u.i & 0x000fffffffffffffULL) | (1022ULL << 52);
  }

  *exp = e;
  return u.f;
}

#endif
