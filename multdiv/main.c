//
//  main.c
//  multdiv
//
//  Created by David Allison on 11/3/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

extern char* __PrintFloatFormat(double f, int precision, char* buf, size_t size);
extern char* __PrintScientificFormat(double f, int precision, char* buf, size_t size);
extern char* __PrintGeneralFormat(double f,  int precision, char* buf, size_t size);

#if 0

struct FloatPrinter {
  uint8_t sign;
  uint8_t exp;
  uint32_t mantissa;
  uint8_t naninf;   // Bit 0: nan, 1: inf
  uint64_t fx[4];
};

// Divide a by b.
uint32_t divide(uint32_t a, uint32_t b){
  if (b == 0) {
    printf("division by zero\n");
    return 0;
  }
  uint32_t quotient = 0;
  uint32_t rem = 0;
  for (int i = 0; i < 32; i++) {
    // Shift high bit of a into rem and shift a left by one.
    rem <<= 1;
    if ((a & 0x80000000) != 0) {
      rem |= 1;
    }
    a <<= 1;
    quotient <<= 1;
    if (rem >= b) {
       quotient++;
       rem -= b;
    }
  }
  return quotient;
}

// Multiply a by b.
uint64_t multiply(uint32_t a, uint32_t b) {
  int64_t r = 0;
  while (a != 0) {
    if ((a & 1) == 1) {
      r += b;
    }
    a >>= 1;
    b <<= 1;
  }
  return r;
}

static void increment128(uint64_t a[2]) {
  ++a[0];
  if (a[0] != 0) {
    return;
  }
  ++a[1];
}

static void increment256(uint64_t a[4]) {
  ++a[0];
  if (a[0] != 0) {
    return;
  }
  ++a[1];
  if (a[1] != 0) {
    return;
  }
  ++a[2];
  if (a[2] != 0) {
    return;
  }
  ++a[3];
}

static uint64_t add64(uint64_t* a, uint64_t* b, uint64_t carry_in) {
  uint64_t na = *a + *b + carry_in;
  uint64_t carry_out = na < *a;
  *a = na;
  return carry_out;
}

static void add256(uint64_t a[4], uint64_t b[4]) {
  uint64_t carry = add64(&a[0], &b[0], 0);
  carry = add64(&a[1], &b[1], carry);
  carry = add64(&a[2], &b[2], carry);
  add64(&a[3], &b[3], carry);
}


static int64_t rol(uint64_t* a, uint64_t carry_in) {
  int64_t carry_out = (*a & (1LL << 63)) != 0;
  *a <<= 1;
  *a |= carry_in;
  return carry_out;
}

static int64_t asl(uint64_t* a) {
  int64_t carry = (*a & (1LL << 63)) != 0;
  *a <<= 1;
  return carry;
}

static int64_t ror(uint64_t* a, uint64_t carry_in) {
  int64_t carry_out = (*a & 1) != 0;
  *a >>= 1;
  *a |= carry_in << 63;
  return carry_out;
}

static int64_t lsr(uint64_t* a) {
  int64_t carry = (*a & 1) != 0;
  *a >>= 1;
  return carry;
}

static void lshift128(uint64_t a[2]) {
  int64_t carry = asl(&a[0]);
  carry = rol(&a[1], carry);
}

static void lshift256(uint64_t a[2]) {
  int64_t carry = asl(&a[0]);
  carry = rol(&a[1], carry);
  carry = rol(&a[2], carry);
  carry = rol(&a[3], carry);
}

static void rshift128(uint64_t a[2]) {
  int64_t carry = lsr(&a[1]);
  carry = ror(&a[0], carry);
}

static void rshift256(uint64_t a[4]) {
  int64_t carry = lsr(&a[3]);
  carry = ror(&a[2], carry);
  carry = ror(&a[1], carry);
  carry = ror(&a[0], carry);
}

// Multiply by 10 by x*8 + x*2.
// Puts result in a.
void multiply10(uint64_t a[4]) {
  // t = a * 8
  uint64_t t[4] = {a[0], a[1], a[2], a[3]};
  lshift256(t);
  lshift256(t);
  lshift256(t);

  // a = a * 2
  lshift256(a);
  
  // a = a + t
  add256(a, t);
}

uint8_t divmod10_64(uint64_t a[1]){
  uint64_t quotient = 0;
  uint8_t rem = 0;
  for (int i = 0; i < 64; i++) {
    // Shift high bit of a into rem and shift a left by one.
    rem <<= 1;
    if ((a[0] & (1LL << 63)) != 0) {
      rem |= 1;
    }
    *a <<= 1;
    quotient <<= 1;
    if (rem >= 10) {
       ++quotient;
       rem -= 10;
    }
  }
  *a = quotient;
  return rem;
}


// Divide a by 10, where a is a 128 bit number.  Return
// the modulus.  A is modified to be a/10.
uint8_t divmod10_128(uint64_t a[2]){
  if (a[1] == 0) {
    return divmod10_64(a);
  }
  uint64_t quotient[2] = {0};
  uint8_t rem = 0;
  for (int i = 0; i < 128; i++) {
    // Shift high bit of a into rem and shift a left by one.
    rem <<= 1;
    if ((a[1] & (1LL << 63)) != 0) {
      rem |= 1;
    }
    lshift128(a);
    lshift128(quotient);
    if (rem >= 10) {
       increment128(quotient);
       rem -= 10;
    }
  }
  a[0] = quotient[0];
  a[1] = quotient[1];
  return rem;
}

// Divide a by 10, where a is a 256 bit number.
// A is modified to be a/10.
uint8_t div10(uint64_t a[4]) {
  // If top words are zero, use 128 bit division.
  if (a[2] == 0 && a[3] == 0) {
    return divmod10_128(a);
  }
  uint64_t quotient[4] = {0};
  uint8_t rem = 0;
  for (int i = 0; i < 256; i++) {
    // Shift high bit of a into rem and shift a left by one.
    rem <<= 1;
    if ((a[3] & (1LL << 63)) != 0) {
      rem |= 1;
    }
    lshift256(a);
    lshift256(quotient);
    if (rem >= 10) {
       increment256(quotient);
       rem -= 10;
    }
  }
  a[0] = quotient[0];
  a[1] = quotient[1];
  a[2] = quotient[2];
  a[3] = quotient[3];
  return rem;
}

// Convert a float to a 256 bit fixed point representation.
void FixFloat(struct FloatPrinter* printer, float f) {
  uint32_t bits = *(uint32_t*)&f;
  printer->sign = bits >> 31;
  printer->exp = (bits >> 23) & 0xff;
  printer->mantissa = ((bits & 0x7fffff) | 0x800000) << 8;
  printer->naninf = 0;
  if (printer->exp == 0xff) {
    if (printer->mantissa == 0xffffff) {
      printer->naninf = 1;
    }
    if (printer->mantissa == 0) {
      printer->naninf = 2;
    }
    return;
  }
  memset(printer->fx, 0, sizeof(printer->fx));
  if (printer->exp == 0) {
    return;
  }
  printer->fx[1] = (uint64_t)printer->mantissa << 32;
  if (printer->exp < 127) {
    // Negative exponent - shift right until exp is 127.
    while (printer->exp < 126) {     // Why 126?  Seems to be half if we use 127.
      rshift256(printer->fx);
      ++printer->exp;
    }
  } else {
    // Positive exponent, shift left.
    while (printer->exp >= 127) {
      lshift256(printer->fx);
      --printer->exp;
    }
  }
}

// Round.  If top bit of the fraction part of v is set we have a
// number >= 0.5.  Round up.
void Round(uint64_t v[4]) {
  if ((v[1] & (1LL << 63)) != 0) {
    increment128(&v[2]);
  }
}

bool IsZero(uint64_t v[4]) {
  return (v[0] | v[1] | v[2] | v[3]) == 0;
}

char* WriteZero(int precision, char* buf, size_t size) {
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

char* WriteNanInf(int8_t sign, int v, char* buf, size_t size) {
  char* end = buf + size - 4;
  char* p = end;
  if (sign) {
    *p++ = '-';
  }
  strcpy(p, v == 1 ? "nan" : "inf");
  return end;
}

// v is a 256 bit fixed point number with the binary point at 128 bits.
char* PrintFixedPoint(struct FloatPrinter* printer, int precision, char* buf, size_t size) {
  if (printer->naninf != 0) {
    return WriteNanInf(printer->sign, printer->naninf, buf, size);
  }
  if (IsZero(printer->fx)) {
    return WriteZero(precision, buf, size);
  }
  char* end = buf + size - 1;
  char* p = end;
  *p-- = '\0';
  int leading_zeroes = 0;
  for (int i = 0; i < precision; i++) {
    multiply10(printer->fx);
    if (printer->fx[2] == 0) {
      leading_zeroes++;
    }
  }
  Round(printer->fx);
  
  while (printer->fx[2] != 0 || printer->fx[3] != 0) {
    if (precision == 0) {
      *p-- = '.';
    }
    int r = divmod10_128(&printer->fx[2]);
    *p-- = r + '0';
    --precision;
  }
  for (int i = 0; i < leading_zeroes; i++) {
    *p-- = '0';
  }
  if (leading_zeroes > 0) {
    *p-- = '.';
    *p-- = '0';
  }
  if (printer->sign) {
    *p-- = '-';
  }
  return p + 1;
}

// Write the exponent backwards starting at buf.  Return the start of
// the result.  Always writes 3 chars:
// +/-XX
char* WriteExponent(int exp, char* buf) {
  bool negative = false;
  if (exp < 0) {
    exp = -exp;
    negative = true;
  }
  int ndigits = 2;
  while (exp != 0 || ndigits > 0) {
    int d = exp % 10;
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
int CalculateExponent(uint64_t v[4]) {
  uint64_t t[4];
  memcpy(t, v, sizeof(t));
  int exp = 0;
  if (t[3] == 0 && t[2] == 0) {
    // Less than 1, multiply by 10 until >= 1
    while (t[2] == 0) {
      multiply10(t);
      --exp;
    }
  } else {
    while (t[3] != 0 || t[2] >= 10) {
      div10(t);
      ++exp;
    }
  }
  return exp;
}

// Print the 256 bit fixed point number in the printer in scientific form
// to the buffer.  Return the address of the printed text.
char* PrintFixedPointScientific(struct FloatPrinter* printer, int precision, int exp, char* buf, size_t size) {
  if (printer->naninf != 0) {
    return WriteNanInf(printer->sign, printer->naninf, buf, size);
  }
  if (IsZero(printer->fx)) {
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
      div10(printer->fx);
    }
  } else {
    for (int i = 0; i < shifts; i++) {
      multiply10(printer->fx);
    }
  }
  Round(printer->fx);
  
  char* end = buf + size - 1;
  *end-- = '\0';
  char* p = WriteExponent(exp, end);
  *p-- = 'e';
  while (printer->fx[2] != 0 || printer->fx[3] != 0) {
    if (precision == 0) {
      *p-- = '.';
    }
    int r = divmod10_128(&printer->fx[2]);
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
char* PrintFixedPointGeneral(struct FloatPrinter* printer, int precision, char* buf, size_t size) {
  if (printer->naninf != 0) {
    return WriteNanInf(printer->sign, printer->naninf, buf, size);
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
    char* e = buf + size - 5;     // e+XX
    char* s = e - 1;              // Last digit in fraction.
    while (*s == '0') {
      *s = '\0';
      --s;
    }
    if (*s == '.') {
      *s = '\0';
    }
    s++;
    if (s < e - 1) {
      while (*e != '\0') {
        *s++ = *e++;
      }
    }
    *s = '\0';
  }
  return p;
}


char* PrintFloatFormat(float f, int precision, char* buf, size_t size) {
  struct FloatPrinter printer;
  printf("%.6f\n", f);
  FixFloat(&printer, f);
  return PrintFixedPoint(&printer, precision, buf, size);
}

char* PrintScientificFormat(float f, int precision, char* buf, size_t size) {
  printf("%.7e\n", f);
  struct FloatPrinter printer;
  FixFloat(&printer, f);
  
  // Calculate the exponent.
  int exp = CalculateExponent(printer.fx);
  return PrintFixedPointScientific(&printer, precision, exp, buf, size);
}

// %g format.
char* PrintGeneralFormat(float f,  int precision, char* buf, size_t size) {
   printf("%g\n", f);
  struct FloatPrinter printer;
  FixFloat(&printer, f);
  return PrintFixedPointGeneral(&printer, precision, buf, size);
}

#endif

int main(int argc, char** argv) {
  double v = 9.7;
  
  char buf[256];
  char* p = __PrintFloatFormat(v, 6, buf, sizeof(buf));
  fputs(p, stdout);
  fputc('\n', stdout);
  fflush(stdout);
  
  p = __PrintScientificFormat(v, 7, buf, sizeof(buf));
  fputs(p, stdout);
  fputc('\n', stdout);
  fflush(stdout);
 
  p = __PrintGeneralFormat(v, 6, buf, sizeof(buf));
  fputs(p, stdout);
  fputc('\n', stdout);
  fflush(stdout);

}


