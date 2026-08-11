//
//  ldexp.c
//  c_compiler
//
//  Scale x by 2^n.
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

double ldexp(double x, int n) {
  union {
    float f;
    uint32_t i;
  } u = {(float)x};
  uint32_t bits = u.i;

  if ((bits & 0x7fffffffu) == 0 || n == 0) {
    return x;
  }
  if (((bits >> 23) & 0xffu) == 0xffu) {
    return x;
  }

  int exponent = (int)((bits >> 23) & 0xffu);
  if (exponent == 0) {
    while ((bits & 0x007fffffu) != 0 && exponent > -126) {
      bits <<= 1;
      exponent -= 1;
    }
    exponent += 1;
  }

  exponent += n;
  if (exponent >= 255) {
    return float_bits_to_double((bits & 0x80000000u) | 0x7f800000u);
  }
  if (exponent <= 0) {
    if (exponent <= -24) {
      return float_bits_to_double(bits & 0x80000000u);
    }
    bits &= 0x807fffffu;
    while (exponent < 1) {
      bits >>= 1;
      exponent += 1;
    }
    bits |= (uint32_t)exponent << 23;
    return float_bits_to_double(bits);
  }

  bits = (bits & 0x807fffffu) | ((uint32_t)exponent << 23);
  return float_bits_to_double(bits);
}

#else

double ldexp(double x, int n) {
  if (x == 0.0) {
    return x;
  }
  if (x != x) {
    return x;
  }
  if (x - x != 0.0) {
    return x;
  }
  union {
    double f;
    uint64_t i;
  } pow2;
  while (n > 1023) {
    pow2.i = (uint64_t)(1023 + 1023) << 52;
    x *= pow2.f;
    n -= 1023;
  }
  while (n < -1022) {
    pow2.i = (uint64_t)(1023 - 1022) << 52;
    x *= pow2.f;
    n += 1022;
  }
  pow2.i = (uint64_t)(1023 + n) << 52;
  return x * pow2.f;
}

#endif
