//
//  modf.c
//  c_compiler
//
//  Split x into integral and fractional parts.
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

double modf(double x, double *iptr) {
  union {
    float f;
    uint32_t i;
  } u = {(float)x};
  uint32_t sign = u.i & 0x80000000u;

  if ((u.i & 0x7fffffffu) == 0) {
    *iptr = x;
    return x;
  }
  if (((u.i >> 23) & 0xffu) == 0xffu) {
    *iptr = x;
    return float_bits_to_double(sign);
  }

  int exponent = (int)((u.i >> 23) & 0xffu) - 127;
  if (exponent >= 23) {
    *iptr = x;
    return float_bits_to_double(sign);
  }
  if (exponent < 0) {
    *iptr = float_bits_to_double(sign);
    return x;
  }

  uint32_t mask = 0x007fffffu >> exponent;
  if ((u.i & mask) == 0) {
    *iptr = x;
    return float_bits_to_double(sign);
  }

  uint32_t truncated = u.i & ~mask;
  double ipart = float_bits_to_double(truncated);
  *iptr = ipart;
  return x - ipart;
}

#else

double modf(double x, double *iptr) {
  union {
    double f;
    uint64_t i;
  } u = {x};
  uint64_t mask;
  int e = (int)((u.i >> 52) & 0x7ffULL) - 0x3ff;

  if (e >= 52) {
    *iptr = x;
    if (e == 0x400 && (u.i << 12) != 0) {
      return x;
    }
    u.i &= 1ULL << 63;
    return u.f;
  }

  if (e < 0) {
    u.i &= 1ULL << 63;
    *iptr = u.f;
    return x;
  }

  mask = (~0ULL >> 12) >> e;
  if ((u.i & mask) == 0) {
    *iptr = x;
    u.i &= 1ULL << 63;
    return u.f;
  }
  u.i &= ~mask;
  *iptr = u.f;
  return x - u.f;
}

#endif
