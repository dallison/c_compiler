//
//  strtod.c
//  c_compiler
//
//  Created by David Allison on 12/1/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include "_fpfuncs.h"
#include <ctype.h>
#include <string.h>

#ifndef __6502__
#define strtod Strtod
#endif

extern void Break();

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
// to bits of the bottom half of the fixed point number is the binary mantissa.
//
// Finally we assemble the IEEE754 number (either single precision or double
// precision depending on the architecture).
double strtod (const char* str, char** endptr) {
  uint64_t fx[FIXED_SIZE_WORDS] = {0};
  uint64_t n[FIXED_SIZE_HALF] = {0};

  uint8_t negative = 0;
  
  const char* p = str;
  if (*p == '-') {
    negative = 0x80;
    p++;
  }
  if (*p == '+') {
    p++;
  }
  
  // Convert integral part to binary.  This is put into the top
  // half of the fixed point number - the integral part.
  int fraction_digits = 0;
  while (isdigit(*p) && *p != '.' && *p != 'e' && *p != 'E') {
    n[0] = *p - '0';
    __MultiplyBy10Half(fx + FIXED_SIZE_HALF);
    __AddHalf(fx + FIXED_SIZE_HALF, n);
    p++;
  }
  Break();
  
  if (*p == '.') {
    p++;
    // We have a fractional part, continue accumulating and count the
    // number of fractional digits.
    while (isdigit(*p) && *p != 'e' && *p != 'E') {
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
  
  // Check for exponent.
  if (*p == 'e' || *p == 'E') {
    p++;
    bool negative_exp = *p == '-';
    if (*p == '+' || *p =='-') {
      p++;
    }
    int exp = 0;
    while (isdigit(*p)) {
      exp = exp * 10 + *p++ - '0';
    }
    
    // Multiply or divide by the exponent.
    if (negative_exp) {
      for (int i = 0; i < exp; i++) {
        __DivideBy10(fx);
      }
    } else {
      for (int i = 0; i < exp; i++) {
        __MultiplyBy10(fx);
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
    return 0;
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
  // Single precision - 23 bit mantissa.
#if defined(__6502__)
  // 6502 is slow for multiple shifts.  This is in assembly language.
  return __packIEEE754(negative, exp, mantissa);
#else
  uint32_t packed = (mantissa >> (32 - 23)) | (exp << 23) | negative << 24;
  return *(double*)&packed;
#endif
#else
  // Double precision.
  uint64_t packed = (mantissa >> (64LL - 52LL)) |
          ((uint64_t)exp << 52LL) |
          (uint64_t)negative << 56LL;
  return *(double*)&packed;
#endif
  
}
