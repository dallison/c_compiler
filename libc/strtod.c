//
//  strtod.c
//  c_compiler
//
//  Created by David Allison on 12/1/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include "_fpfuncs.h"
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <stddef.h>
#include <string.h>

#include "../c_compiler/support/fp_extended.h"

// The algorithm for this is:
// Build a fixed point binary number with the binary point at the
// half way point.  For single precision this number is 256 bits long
// and for double precision it's 4096 bits.
//
// Take the integral part of the string and convert it to binary in the
// upper half of the fixed point number (to the left of the binary point).
// Then take the fractional part (the bit after the decimal point) and
// keep adding to the integral part digit by digit, counting the number
// of digits added.  The result will be a large fixed point integer that
// contains both the integral and fractional parts.
//
// Say we are converting "1.5".  The fixed point number will be 15.0.
//
// Now we divide by 10 to the power of fraction_digits, resulting in the
// number 1.5 in the fixed point integer.
//
// Next we deal with the power-of-10 exponent.  For this we either divide
// or multiply by 10 to the power of the exponent.
//
// So if we had "1.5e3" the exponent is 3 so we multiply by 10 three times
// or multiply by 10^3.  This gives us the number 150.0 in the fixed point
// number.
//
// Now we need to float the binary point so that we have a 1 in the integral
// part.  We do this by shifting left or right, counting the number of
// shifts until we have only a 1 in the top half of the fixed point
// number.  The number of times we shift is the binary exponent.  The
// bits of the bottom half of the fixed point number is the binary mantissa.
//
// Finally we assemble the IEEE754 number (either single precision or double
// precision depending on the architecture).

// An exact zero keeps its sign and does not set errno. Overflow and total
// underflow go through RangeResult, which does set ERANGE.
static double SignedZero(uint8_t negative) {
  if (negative == 0) {
    return 0.0;
  }
#if DOUBLE_IS_SINGLE
  uint32_t packed = (uint32_t)negative << 24;
#else
  uint64_t packed = (uint64_t)negative << 56;
#endif
  return *(double*)&packed;
}

static double RangeResult(int overflow, uint8_t negative) {
  errno = ERANGE;
#if DOUBLE_IS_SINGLE
  uint32_t packed = (uint32_t)negative << 24;
  if (overflow) {
    packed |= 255u << 23;
  }
#else
  uint64_t packed = (uint64_t)negative << 56;
  if (overflow) {
    packed |= (uint64_t)0x7ff << 52;
  }
#endif
  return *(double*)&packed;
}

#if !DOUBLE_IS_SINGLE
// `exp` is the biased exponent of a normalized significand whose integer
// part is 1 and whose next 64 fraction bits are `top`. When that exponent
// is too small for a normal double, fold the significand into a subnormal
// or into a signed zero. `*fraction` receives the 52 stored mantissa bits.
static void PackSubnormal(int* exp, uint64_t* fraction, uint64_t top,
                          int has_lower) {
  int shift = 13 - *exp;
  uint64_t magnitude = 0;
  int round_up = 0;
  if (shift < 64) {
    uint64_t half = 1ULL << (shift - 1);
    uint64_t lost = top & ((1ULL << shift) - 1);
    magnitude = (top >> shift) | (1ULL << (64 - shift));
    int sticky = has_lower || (lost & (half - 1)) != 0;
    if (lost > half || (lost == half && (sticky || (magnitude & 1) != 0))) {
      round_up = 1;
    }
  } else if (shift == 64) {
    uint64_t half = 1ULL << 63;
    magnitude = 1;
    int sticky = has_lower || (top & (half - 1)) != 0;
    if (top > half || (top == half && (sticky || (magnitude & 1) != 0))) {
      round_up = 1;
    }
  } else if (shift == 65) {
    // The leading 1 sits exactly halfway below the smallest subnormal.
    magnitude = 0;
    if (has_lower || top != 0) {
      round_up = 1;
    }
  }
  if (round_up) {
    ++magnitude;
  }
  if (magnitude == 0) {
    errno = ERANGE;
    *fraction = 0;
    *exp = 0;
    return;
  }
  if (magnitude >= (1ULL << 52)) {
    *fraction = 0;
    *exp = 1;
    return;
  }
  *fraction = magnitude;
  *exp = 0;
}
#endif

double strtod(const char* str, char** endptr) {
  uint64_t fx[FIXED_SIZE_WORDS] = {0};
  uint64_t n[FIXED_SIZE_HALF] = {0};

  uint8_t negative = 0;
  
  const char* p = str;
  while (isspace(*p)) {
    p++;
  }
  if (*p == '-') {
    negative = 0x80;
    p++;
  }
  if (*p == '+') {
    p++;
  }
  
  bool converted = false;
  bool nonzero_digit = false;

  // Convert integral part to binary.  This is put into the top
  // half of the fixed point number - the integral part.
  int fraction_digits = 0;
  while (isdigit(*p)) {
    converted = true;
    if (*p != '0') {
      nonzero_digit = true;
    }
    n[0] = *p - '0';
    __MultiplyBy10Half(fx + FIXED_SIZE_HALF);
    __AddHalf(fx + FIXED_SIZE_HALF, n);
    p++;
  }
  if (*p == '.') {
    p++;
    // We have a fractional part, continue accumulating and count the
    // number of fractional digits.
    while (isdigit(*p)) {
      converted = true;
      if (*p != '0') {
        nonzero_digit = true;
      }
      n[0] = *p - '0';
      __MultiplyBy10Half(fx + FIXED_SIZE_HALF);
      __AddHalf(fx + FIXED_SIZE_HALF, n);
      p++;
      fraction_digits++;
    }
    
    // Now shift the fractional part down to the lower half - to the right
    // of the binary point.
    for (int i = 0; i < fraction_digits; i++) {
      __DivideBy10(fx);
    }
  }
  
  if (!converted) {
    if (endptr != NULL) {
      *endptr = (char*)str;
    }
    return 0.0;
  }

  // Check for exponent. It is only consumed when at least one exponent digit
  // follows the optional sign.
  if (*p == 'e' || *p == 'E') {
    const char* exponent = p + 1;
    bool negative_exp = *exponent == '-';
    if (*exponent == '+' || *exponent == '-') {
      exponent++;
    }
    if (isdigit(*exponent)) {
      int decimal_exp = 0;
      do {
        // A few thousand powers of ten already exceed every binary64
        // magnitude. Saturate so a long exponent cannot overflow `int`.
        if (decimal_exp <= 1000000) {
          decimal_exp = decimal_exp * 10 + (*exponent - '0');
        }
        exponent++;
      } while (isdigit(*exponent));
      p = exponent;

      // Skip the digit-by-digit scale only when the coefficient's magnitude
      // plus this exponent is far outside binary64. A large exponent on a
      // tiny coefficient can still be finite, so the exponent alone is not
      // enough.
      if (nonzero_digit && decimal_exp > 300) {
        int top_bit = -1;
        for (int word = FIXED_SIZE_WORDS - 1; word >= 0; --word) {
          uint64_t bits = fx[word];
          if (bits == 0) {
            continue;
          }
          int bit = 63;
          while ((bits & (1ULL << bit)) == 0) {
            --bit;
          }
          top_bit = word * 64 + bit;
          break;
        }
        if (top_bit >= 0) {
          int binary_point = FIXED_SIZE_HALF * 64;
          // 3322/1000 approximates log2(10).
          long long log2_times_1000 =
              (long long)(top_bit - binary_point) * 1000;
          long long exp_times_1000 = (long long)decimal_exp * 3322;
          if (negative_exp) {
            exp_times_1000 = -exp_times_1000;
          }
          log2_times_1000 += exp_times_1000;
          if (log2_times_1000 > 1200LL * 1000 ||
              log2_times_1000 < -1200LL * 1000) {
            if (endptr != NULL) {
              *endptr = (char*)p;
            }
            return RangeResult(log2_times_1000 > 0 ? 1 : 0, negative);
          }
        }
      }

      if (negative_exp) {
        for (int i = 0; i < decimal_exp; i++) {
          __DivideBy10(fx);
        }
      } else {
        for (int i = 0; i < decimal_exp; i++) {
          __MultiplyBy10(fx);
        }
      }
    }
  }
  if (endptr != NULL) {
    *endptr = p;
  }
  
  // We now have a fixed point binary number. Float the binary point to normalize
  // the number.
  int exp = EXP_BIAS;
  if (__IsZero(fx)) {
    if (nonzero_digit) {
      errno = ERANGE;
    }
    return SignedZero(negative);
  }
  
  // Normalize by making the integer part 1.
  if (__IsZeroHalf(fx + FIXED_SIZE_HALF)) {
    // Number is less than one, shift left.
    while (!__IsOneHalf(fx + FIXED_SIZE_HALF)) {
      __LShift(fx);
      exp--;
    }
  } else {
    // Number is greater than one, shift right.
    while (!__IsOneHalf(fx + FIXED_SIZE_HALF)) {
      __RShift(fx);
      exp++;
    }
  }
  
  // The mantissa is the upper bits of the fractional part.
  uint64_t mantissa = fx[FIXED_SIZE_HALF-1];
#if DOUBLE_IS_SINGLE
  // Single precision - 23 bit mantissa. Exponent 255 is infinity and a
  // non-positive exponent does not fit in a normal value.
  if (exp >= 255) {
    return RangeResult(1, negative);
  }
  if (exp <= 0) {
    return RangeResult(0, negative);
  }
#if defined(__6502__)
  // 6502 is slow for multiple shifts.  This is in assembly language.
  return __packIEEE754(negative, exp, mantissa);
#else
  uint32_t packed = (mantissa >> (32 - 23)) | (exp << 23) | negative << 24;
  return *(double*)&packed;
#endif
#else
  // Double precision.
  bool sticky = false;
  for (int i = 0; i < FIXED_SIZE_HALF - 1; ++i) {
    if (fx[i] != 0) {
      sticky = true;
      break;
    }
  }
  uint64_t fraction = 0;
  if (exp >= 0x7ff) {
    return RangeResult(1, negative);
  }
  if (exp <= 0) {
    PackSubnormal(&exp, &fraction, mantissa, sticky ? 1 : 0);
  } else {
    const int discarded_bits = 64 - 52;
    fraction = mantissa >> discarded_bits;
    uint64_t discarded = mantissa & ((1ULL << discarded_bits) - 1);
    const uint64_t halfway = 1ULL << (discarded_bits - 1);
    if (discarded > halfway ||
        (discarded == halfway && (sticky || (fraction & 1) != 0))) {
      ++fraction;
      if (fraction == (1ULL << 52)) {
        fraction = 0;
        ++exp;
      }
    }
    if (exp >= 0x7ff) {
      return RangeResult(1, negative);
    }
  }
  uint64_t packed = fraction |
          ((uint64_t)exp << 52LL) |
          (uint64_t)negative << 56LL;
  return *(double*)&packed;
#endif
  
}

float strtof(const char* str, char** endptr) {
  int saved = errno;
  errno = 0;
  double value = strtod(str, endptr);
  int range = errno == ERANGE;
  float narrowed = (float)value;
  // A finite double can still be outside the float range. The cast then
  // becomes an infinity or a zero, which is an overflow or an underflow.
  if (!range && ((narrowed > FLT_MAX) || (narrowed < -FLT_MAX) ||
                 (narrowed == 0.0f && value != 0.0))) {
    range = 1;
  }
  errno = range ? ERANGE : saved;
  return narrowed;
}

#if defined(__DAVECC_LDBL_FORMAT__) && __DAVECC_LDBL_FORMAT__ >= 2
static int StrtoldFormat(void) { return __DAVECC_LDBL_FORMAT__; }

static long double BitsToLongDouble(FPBits bits) {
  long double value;
  memcpy(&value, &bits, sizeof(value));
  return value;
}

static int StartsWithIgnoreCase(const char* text, const char* prefix) {
  while (*prefix != '\0') {
    char a = *text++;
    char b = *prefix++;
    if (a >= 'A' && a <= 'Z') {
      a = (char)(a - 'A' + 'a');
    }
    if (b >= 'A' && b <= 'Z') {
      b = (char)(b - 'A' + 'a');
    }
    if (a != b) {
      return 0;
    }
  }
  return 1;
}

static FPBits ScaleByPow10(FPBits value, int exponent, int format) {
  if (exponent == 0) {
    return value;
  }
  int n = exponent < 0 ? -exponent : exponent;
  FPBits power = FPBitsFromI64(10, format);
  FPBits scale = FPBitsFromI64(1, format);
  while (n > 0) {
    if (n & 1) {
      scale = FPMul(scale, power, format);
    }
    n >>= 1;
    if (n > 0) {
      power = FPMul(power, power, format);
    }
  }
  if (exponent < 0) {
    return FPDiv(value, scale, format);
  }
  return FPMul(value, scale, format);
}

long double strtold(const char* str, char** endptr) {
  const char* p = str;
  while (isspace((unsigned char)*p)) {
    p++;
  }
  int negative = 0;
  if (*p == '-' || *p == '+') {
    negative = *p == '-';
    p++;
  }
  int format = StrtoldFormat();
  if (StartsWithIgnoreCase(p, "nan")) {
    if (endptr != NULL) {
      *endptr = (char*)(p + 3);
    }
    FPBits nan_bits;
    nan_bits.lo = 0;
    nan_bits.hi = 0;
    if (format == kFPExtFormatIntel80) {
      nan_bits.lo = 0xc000000000000000ULL;
      nan_bits.hi = (negative ? (1ULL << 15) : 0) | 0x7fffULL;
    } else {
      nan_bits.hi = (negative ? (1ULL << 63) : 0) | (0x7fffULL << 48) |
                    0x0000800000000000ULL;
    }
    return BitsToLongDouble(nan_bits);
  }
  if (StartsWithIgnoreCase(p, "inf")) {
    p += 3;
    if (StartsWithIgnoreCase(p, "inity")) {
      p += 5;
    }
    if (endptr != NULL) {
      *endptr = (char*)p;
    }
    FPBits inf;
    inf.lo = 0;
    inf.hi = 0;
    if (format == kFPExtFormatIntel80) {
      inf.lo = 0x8000000000000000ULL;
      inf.hi = (negative ? (1ULL << 15) : 0) | 0x7fffULL;
    } else {
      inf.hi = (negative ? (1ULL << 63) : 0) | (0x7fffULL << 48);
    }
    return BitsToLongDouble(inf);
  }

  FPBits ten = FPBitsFromI64(10, format);
  FPBits value = FPBitsFromI64(0, format);
  int fraction_digits = 0;
  int converted = 0;
  int nonzero_digit = 0;
  while (isdigit((unsigned char)*p)) {
    if (*p != '0') {
      nonzero_digit = 1;
    }
    value = FPAdd(FPMul(value, ten, format),
                  FPBitsFromI64(*p - '0', format), format);
    p++;
    converted = 1;
  }
  if (*p == '.') {
    p++;
    while (isdigit((unsigned char)*p)) {
      if (*p != '0') {
        nonzero_digit = 1;
      }
      value = FPAdd(FPMul(value, ten, format),
                    FPBitsFromI64(*p - '0', format), format);
      p++;
      fraction_digits++;
      converted = 1;
    }
  }
  if (!converted) {
    if (endptr != NULL) {
      *endptr = (char*)str;
    }
    return 0.0L;
  }

  int exponent = -fraction_digits;
  if (*p == 'e' || *p == 'E') {
    const char* exp_start = p + 1;
    int exp_negative = *exp_start == '-';
    if (*exp_start == '+' || *exp_start == '-') {
      exp_start++;
    }
    if (isdigit((unsigned char)*exp_start)) {
      int decimal_exp = 0;
      do {
        if (decimal_exp <= 1000000) {
          decimal_exp = decimal_exp * 10 + (*exp_start - '0');
        }
        exp_start++;
      } while (isdigit((unsigned char)*exp_start));
      p = exp_start;
      exponent += exp_negative ? -decimal_exp : decimal_exp;
    }
  }
  if (endptr != NULL) {
    *endptr = (char*)p;
  }
  // 0e10000 is an exact zero. Scaling a zero by an overflowing power of ten
  // is a 0*inf NaN, so skip the scale entirely.
  if (!nonzero_digit) {
    value = FPBitsFromI64(0, format);
    if (negative) {
      value = FPNeg(value, format);
    }
    return BitsToLongDouble(value);
  }
  value = ScaleByPow10(value, exponent, format);
  if (negative) {
    value = FPNeg(value, format);
  }
  // A nonzero field that overflowed or underflowed is not a finite result.
  // An exact zero, including 0e99999, stays zero and does not set errno.
  if (nonzero_digit &&
      (FPIsInf(value, format) || FPIsZero(value, format))) {
    errno = ERANGE;
  }
  return BitsToLongDouble(value);
}
#else
long double strtold(const char* str, char** endptr) {
  return (long double)strtod(str, endptr);
}
#endif

double atof(const char* str) {
  return strtod(str, NULL);
}

#ifndef __6502__
// Backward-compatible alias for the original internal name (see strtod_test).
double Strtod(const char* str, char** endptr) {
  return strtod(str, endptr);
}
#endif
