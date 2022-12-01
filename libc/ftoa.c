//
//  ftoa.c
//  c_compiler
//
//  Created by David Allison on 11/13/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "_fpfuncs.h"

#if 1
#define STATIC 
#else
#define STATIC static
#endif

struct FloatPrinter {       // 6502 offset
  uint8_t sign;             // 0
  Exponent exp;             // 1
  Mantissa mantissa;        // 2
  uint8_t naninf;           // 6 Bit 0: nan, 1: inf
  uint64_t fx[FIXED_SIZE_WORDS];           // 7
};


// Convert a float to a 256 bit fixed point representation.
// The single precision fixed point representation is:
//
// 255                     127                        0
// +-----------------------+--------------------------+
// |    whole part         |    fractional part       |
// +-----------------------+--------------------------+
//
// That is, 128 bits of whole number followed by 128 bits of
// the fraction.
STATIC void FixFloat(struct FloatPrinter* printer, double f) {
  // Extract the IEEE754 single precision to sign, exponent, and mantissa.
#if defined(__6502__)
  uint32_t* bits = (uint32_t*)&f;
  // Multiple bit shifts are slow on 6502.
  __unpackIEEE754(bits, printer);
#elif DOUBLE_IS_SINGLE
  // Single Precision
  printer->sign = bits >> 31;
  printer->exp = (bits >> 23) & 0xff;
  printer->mantissa = ((bits & 0x7fffff) | 0x800000) << 8;
#else
  // Double precision.
  uint64_t* bits = (uint64_t*)&f;
  printer->sign = (*bits & (1LL << 63)) != 0;
  printer->exp = (*bits >> 52) & 0x7ff;
  printer->mantissa = ((*bits & ((1LL << 53) - 1)) | (1LL << 52)) << 11;
#endif
  printer->naninf = 0;
#if defined(__6502__)
  if (printer->exp == 0xff) {
    if (printer->mantissa == 0xffffff00) {
      printer->naninf = 1;            // NaN
    }
    if (printer->mantissa == 0) {
      printer->naninf = 2;            // Infinity
    }
    return;
  }
#else
  if (printer->exp == 0x7ff) {
    if (printer->mantissa == 0xffffffffffffff00LL) {
      printer->naninf = 1;            // NaN
    }
    if (printer->mantissa == 0) {
      printer->naninf = 2;            // Infinity
    }
    return;
  }

#endif
  if (printer->exp == 0) {
    return;
  }
  // Put mantissa in the high end of the fractional part.
#if defined(__6502__)
  printer->fx[1] = (uint64_t)printer->mantissa << 32;
#else
  printer->fx[31] = printer->mantissa;
#endif
  // Shift the mantissa left or right by the exponent.
  // In IEE754 single precision, the exponent has a bias
  // of 127, so 127 == 0, 128=0, 126=-1
  // For double precision the bias is 1023
  if (printer->exp < EXP_BIAS) {
    // Negative exponent - shift right until exp is 127.
    while (printer->exp < EXP_BIAS - 1) {
      __RShift(printer->fx);
      ++printer->exp;
    }
  } else {
    // Positive exponent, shift left.
    while (printer->exp >= EXP_BIAS) {
      __LShift(printer->fx);
      --printer->exp;
    }
  }
}


// Write 0.00000 with precision number of digits after the point.
STATIC char* WriteZero(int precision, char* buf, size_t size) {
  char* end = buf + size - 1;
  char* p = end;
  *p-- = '\0';
  while (precision > 0) {
     *p-- = '0';
     --precision;
   }
   *p-- = '.';
   *p-- = '0';
   return p + 1;
}

// Write nan or inf.
STATIC char* WriteNanInf(int8_t sign, int v, char* buf, size_t size) {
  char* end = buf + size - 4;
  char* p = end;
  if (sign) {
    *p++ = '-';
  }
  strcpy(p, v == 1 ? "nan" : "inf");
  return end;
}

// v is a 256 bit fixed point number with the binary point at 128 bits.
STATIC char* PrintFixedPoint(struct FloatPrinter* printer, int precision, char* buf, size_t size) {
  if (printer->naninf != 0) {
    return WriteNanInf(printer->sign, printer->naninf, buf, size);
  }
  if (__IsZero(printer->fx)) {
    return WriteZero(precision, buf, size);
  }
  char* end = buf + size - 1;
  char* p = end;
  *p-- = '\0';
  
  // Shift the number left (base 10) by the precision.  This puts the
  // value to be printed into the whole number part of the fixed point
  // number.  The first bit in the fraction is the rounding bit because
  // it represents 0.5.
  int8_t leading_zeroes = 0;
  for (int8_t i = 0; i < precision; i++) {
    __MultiplyBy10(printer->fx);
    if (printer->fx[FIXED_SIZE_WHOLE_PART] == 0) {
      leading_zeroes++;
    }
  }
  // Round the whole part up if the fraction is >- 0.5.
  __Round(printer->fx);
  
  // Write the whole number in decimal, putting a decimal point
  // in the appropriate place.
  bool point_printed = false;
  while (!__IsZeroHalf(&printer->fx[FIXED_SIZE_WHOLE_PART])) {
    if (precision == 0) {
      *p-- = '.';
      point_printed = true;
    }
    uint8_t r = __DivModBy10Half(&printer->fx[FIXED_SIZE_WHOLE_PART]);
    *p-- = r + '0';
    --precision;
  }
  // Write any leading zeroes.
  for (uint8_t i = 0; i < leading_zeroes; i++) {
    *p-- = '0';
  }
  if (!point_printed) {
    *p-- = '.';
    *p-- = '0';
  }
  if (printer->sign) {
    *p-- = '-';
  }
  return p + 1;
}

// Write the exponent backwards starting at buf.  Return the start of
// the result.  Always writes 3 or 4 chars:
// +/-[X]XX
STATIC char* WriteExponent(int exp, char* buf) {
  bool negative = false;
  if (exp < 0) {
    exp = -exp;
    negative = true;
  }
  int ndigits = 2;
  while (exp != 0 || ndigits > 0) {
    uint8_t d = exp % 10;
    *buf-- = d + '0';
    exp /= 10;
    --ndigits;
  }
  *buf-- = negative ? '-' : '+';
  return buf;
}

// Given a 256 bit fixed point number, calculate the base-10 exponent
// needed to normalize it to a number between 1 and 10.  Doesn't modify
// the input argument.
STATIC int CalculateExponent(uint64_t v[FIXED_SIZE_WORDS]) {
  uint64_t t[FIXED_SIZE_WORDS];
  memcpy(t, v, sizeof(t));
  int exp = 0;

  if (__IsZeroHalf(&t[FIXED_SIZE_WHOLE_PART])) {
    // Less than 1, multiply by 10 until >= 1
    while (__IsZeroHalf(&t[FIXED_SIZE_WHOLE_PART])) {
      __MultiplyBy10(t);
      --exp;
    }
  } else {
    while (!__IsLessThan10(&t[FIXED_SIZE_WHOLE_PART])) {
      __DivideBy10(t);
      ++exp;
    }
  }
  return exp;
}

// Print the 256 bit fixed point number in the printer in scientific form
// to the buffer.  Return the address of the printed text.
STATIC char* PrintFixedPointScientific(struct FloatPrinter* printer, int precision, int exp, char* buf, size_t size) {
  if (printer->naninf != 0) {
    return WriteNanInf(printer->sign, printer->naninf, buf, size);
  }
  if (__IsZero(printer->fx)) {
    char* end = buf + size - 1;
    *end-- = '\0';
    char* p = WriteExponent(0, end);
    *p-- = 'e';
    return WriteZero(precision, buf, p - buf + 1);
  }

  // We need to avoid division as much as possible to keep the accuracy
  // up.  The naive algorithm is to normalize the number to 1.xeEE
  // and then multiply it up by 10^precision, but if the number
  // is greater than 1 this will involve divsion by 10 then multiplication
  // by 10.
  //
  // We already have the power-of-10 exponent but we want the result
  // to be this multiplied by 10^precision.
  //
  // Let the fixed point number be N, exponent be 'e' and precision be P.
  //
  // The result we want is 10^-e * 10^P
  // Which is 10^(P-e)
  //
  // The number of base-10 shifts is the precision minus the exponent.
  // We shift right by division and left by multiplication.
  int shifts = precision - exp;
  if (shifts < 0) {
    for (int i = shifts; i < 0; i++) {
      __DivideBy10(printer->fx);
    }
  } else {
    for (int i = 0; i < shifts; i++) {
      __MultiplyBy10(printer->fx);
    }
  }
  __Round(printer->fx);
  
  char* end = buf + size - 1;
  *end-- = '\0';
  char* p = WriteExponent(exp, end);
  *p-- = 'e';
  while (!__IsZeroHalf(&printer->fx[FIXED_SIZE_WHOLE_PART])) {
    if (precision == 0) {
      *p-- = '.';
    }
    uint8_t r = __DivModBy10Half(&printer->fx[FIXED_SIZE_WHOLE_PART]);
    *p-- = r + '0';
    --precision;
  }
  if (printer->sign) {
    *p-- = '-';
  }
  return p+1;
}

// According to C99:
// Let P = precision
// Let X = the exponent calculated for the %e conversion.
// If P > X and X >= -4 then use %f with precision = P - (X + 1)
// Otherwise use %e with precision P - 1
// Trailing zeroes are removed and also the dot if it's traiing.
STATIC char* PrintFixedPointGeneral(struct FloatPrinter* printer, int precision, char* buf, size_t size) {
  if (printer->naninf != 0) {
    return WriteNanInf(printer->sign, printer->naninf, buf, size);
  }
  if (__IsZero(printer->fx)) {
    char* p = buf + size - 1;
    *p-- = '\0';
    *p = '0';
    return p;
  }

  // Need to work out the base10 exponent but we can't do that if we
  // are going to use %f output.
  int exp = CalculateExponent(printer->fx);
  bool f_format = false;
  char* p;
  if (precision > exp && exp >= -4) {
    f_format = true;
    p = PrintFixedPoint(printer, precision - (exp + 1), buf, size);
  } else {
    // We've already normalized the fixed point number so we can use that.
    p = PrintFixedPointScientific(printer, precision - 1, exp, buf, size);
  }
  
  // The C99 spec says that we strip trailing zeroes and the decimal
  // point.  The string returned in p is terminated by a NUL.
  if (f_format) {
    char* s = buf + size - 2;
    while (*s == '0') {
      *s = '\0';
      --s;
    }
    if (*s == '.') {
      *s = '\0';
    }
  } else {
    char* e = buf + size - 5;     // e+[X]XX
    if (*e != 'e') {
      e--;
    }
    char* s = e - 1;              // Last digit in fraction.
    
    // Remove trailing zeroes.
    while (*s == '0') {
      *s = '\0';
      --s;
    }
    // Remove trailing point.
    if (*s == '.') {
      *s = '\0';
    }
    s++;
    // Copy the exponent down.
    if (s < e - 1) {
      while (*e != '\0') {
        *s++ = *e++;
      }
    }
    *s = '\0';
  }
  return p;
}


char* __PrintFloatFormat(double f, int precision, char* buf, size_t size) {
  struct FloatPrinter printer = {0};
  FixFloat(&printer, f);
  return PrintFixedPoint(&printer, precision, buf, size);
}

char* __PrintScientificFormat(double f, int precision, char* buf, size_t size) {
  struct FloatPrinter printer = {0};;
  FixFloat(&printer, f);
  
  // Calculate the exponent.
  int exp = CalculateExponent(printer.fx);
  return PrintFixedPointScientific(&printer, precision, exp, buf, size);
}

// %g format.
char* __PrintGeneralFormat(double f,  int precision, char* buf, size_t size) {
  struct FloatPrinter printer = {0};
  FixFloat(&printer, f);
  return PrintFixedPointGeneral(&printer, precision, buf, size);
}
