//
//  main.c
//  floating_point
//
//  Created by David Allison on 8/27/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

// Floating point from first principles, with no hardware multiplication or
// division.  We only use integer addition, subtraction and shifts.
//
// This only handles single precision IEEE754 numbers.  Double precision are
// just bigger numbers.
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

// This is an unpacked IEEE754 single precision floating point number.
typedef struct {
  uint8_t sign;         // Either 0x80 or 0.
  uint8_t exponent;     // Exponent with bias applied.
  uint32_t mantissa;    // Mantissa with implicit 1 in top bit.
} Unpacked;

typedef union Alias {
  uint32_t i;     // Unsigned integer.
  int32_t si;     // Signed integer.
  float f;        // Single precision float.
} Alias;

// Unpack a IEEE754 single precision number.
Unpacked Unpack(float f) {
  Alias a = {.f = f};
  Unpacked u;
  u.sign = (a.i & 0x80000000) >> 24;
  u.exponent = (a.i >> 23) & 0xff;      // Still has bias.
  u.mantissa = ((a.i & 0x007fffff) << 8) | 0x80000000;
  return u;
}

// Pack into a IEEE754 single precision number.
float Pack(Unpacked u) {
  Alias bits = {.i = 0};
  bits.i |= (u.mantissa >> 8) & 0x007fffff;      // Top 23 bits.
  bits.i |= (uint32_t)u.exponent << 23;                 // Already biased.
  bits.i |= (uint32_t)u.sign << 24;
  return bits.f;
}

// Is f zero.
bool FPIsZero(float f) {
  Alias a = {.f = f};
  return (a.i & 0x7fffffff) == 0;      // Both +ve and -ve 0.
}

bool FPIsNan(float f) {
  Alias a = {.f = f};
  return (a.i & 0x7fffffff) == 0x7fffffff;
}

bool FPIsInfinity(float f) {
  Alias a = {.f = f};
  return (a.i & 0x7f800000) == 0x7f800000;  // Both +ve and -ve
}

float FPZero(void) {
  Alias a = {.i = 0};
  return a.f;
}

float FPNaN(void) {
  Alias a = {.i = 0x7fffffff};
  return a.f;
}

float FPInfinity(void) {
  Alias a = {.i = 0x7f800000};
  return a.f;
}

// Normalize an unpacked number by shifting until we get a 1 in the MSB.  A
// normalized number always has a 1 to the left of the binary point.
Unpacked Normalize(Unpacked u) {
  if (u.mantissa == 0) {
    return u;
  }
  // Shift left until bit 31 of the mantissa is set and decrement exponent
  // with each shift.
  while ((u.mantissa & 0x80000000) == 0) {
    u.mantissa <<= 1;
    u.exponent--;
  }
  return u;
}


Unpacked Round(Unpacked v) {
  // The encoded mantissa is 24 bits long.  This means we discard the bottom
  // byte of the 32-bit mantissa in v.  We round to nearest, so if the top
  // bit of discarded is 1 we have a number 0.5 or larger and we add 1 to
  // the 24-bit mantissa.
  int discarded = v.mantissa & 0xff;
  
  // 0.5 or larger?
  if ((discarded & 0x80) != 0) {
    // Calculate rounded 24-bit mantissa.
    int32_t rounded = ((v.mantissa >> 8) + 1) << 8;
    // If the discarded byte is exactly 0x80 (0.5) we only round if
    // the resultant number is even (IEEE754 rounding rules).
    if (discarded == 0x80) {
      if ((rounded & 0x100) == 0) {
        // Even.
        v.mantissa = rounded;
      }
    } else {
      v.mantissa = rounded;
    }
  }
  return v;
}

// Quotient and Remainder.  This is the value of a division.
struct QR {
  uint64_t quotient;
  uint64_t remainder;
};

// Divide 64 bits a by b, producing a Quotient and Remainder (QR).
struct QR Divide64(uint64_t a, uint64_t b){
  struct QR qr = {0,0};
   for (int i = 0; i < 64; i++) {
    // Shift high bit of a into rem and shift a left by one.
    qr.remainder <<= 1;
    if ((a & 0x8000000000000000LL) != 0) {
      qr.remainder |= 1;
    }
    a <<= 1;
    qr.quotient <<= 1;
    if (qr.remainder >= b) {
      qr.quotient++;
      qr.remainder -= b;
    }
  }
  return qr;
}

// Multiply 32-bits a by b with 64 bit result.
uint64_t Multiply32(uint32_t a, uint32_t b) {
  uint64_t r = 0;
  uint64_t x = a;
  while (b != 0) {
    if ((b & 1) == 1) {
      r += x;
    }
    x <<= 1;
    b >>= 1;
  }
  return r;
}

// To multiply we add the exponents and multiply the mantissas.  The sign
// is the exclusive-OR of the two signs.
float FPMultiply(float a, float b) {
  if (FPIsZero(a) || FPIsZero(b)) {
    return 0;
  }
  if (FPIsInfinity(a) || FPIsInfinity(b)) {
    return FPInfinity();
  }
  if (FPIsNan(a) || FPIsNan(b)) {
    return FPNaN();
  }
  Unpacked ua = Unpack(a);
  Unpacked ub = Unpack(b);
  
  // Sign is EOR of both signs.
  uint8_t sign = ua.sign ^ ub.sign;
  
  // Remove bias from exponents, add them and apply bias.
  // (eA - 127) + (eB - 127) + 127
  // = eA + eB - 127 - 127 + 127
  // = eA + eB - 127
  int8_t exp = ua.exponent + ub.exponent - 127;

  // Multiply the mantissas.  These are both unsigned 32 bit numbers with the
  // high bits set.  The product will be an unsigned 64 bit number with the
  // high 32 bits containing the result we need.  Also, since both numbers
  // were fixed point with 1 bit to the left of the binary point, the result
  // will have 2 bits to the left of the binary point.  We increment the
  // exponent to take this into account.
  uint64_t m = Multiply32(ua.mantissa, ub.mantissa);
  
  Unpacked r;
  r.sign = sign;
  r.exponent = exp + 1;      // Exponent incremented.
  r.mantissa = (uint32_t)(m >> 32);
  return Pack(Round(Normalize(r)));
}

// Division is done by subtracting the exponents and dividing the mantissas.
float FPDivide(float a, float b) {
  if (FPIsZero(a)) {
    return 0;
  }
  if (FPIsZero(b)) {
    return FPInfinity();
  }
  if (FPIsInfinity(a) || FPIsInfinity(b)) {
    return FPInfinity();
  }
  if (FPIsNan(a) || FPIsNan(b)) {
    return FPNaN();
  }
  Unpacked ua = Unpack(a);
  Unpacked ub = Unpack(b);
  
  // Sign is EOR of both signs.
  uint8_t sign = ua.sign ^ ub.sign;
  
  // Remove bias from exponents, subtract them and apply bias.
  // (eA - 127) - (eB - 127) + 127
  // = eA - eB - 127 + 127 + 127
  // = eA - eB + 127
  uint8_t exp = ua.exponent - ub.exponent + 127;
 
  // Divide the mantissas.  The dividend is placed in the upper 32 bits of
  // the division and the divisor is in the lower 32 bits.
  struct QR qr = Divide64((uint64_t)ua.mantissa << 32, ub.mantissa);
 
  // If the quotient is greater than 32 bits, shift it right until it fits
  // into 32 bits and increment exponent for each shift.
  while ((qr.quotient & 0xffffffff00000000LL) != 0) {
    qr.quotient >>= 1;
    exp++;
  }
  Unpacked r;
  r.sign = sign;
  r.exponent = exp - 1;     // TODO: why subtract 1?
  r.mantissa = (uint32_t)qr.quotient;
  return Pack(Round(Normalize(r)));
}

// To add, we make the exponents the same by shifting the lower value to the
// right, then add the mantissas, taking sign into account.
float FPAdd(float a, float b) {
  if (FPIsZero(a)) {
    return b;
  }
  if (FPIsZero(b)) {
    return a;
  }
  if (FPIsInfinity(a) || FPIsInfinity(b)) {
    return FPInfinity();
  }
  if (FPIsNan(a) || FPIsNan(b)) {
    return FPNaN();
  }
  Unpacked ua = Unpack(a);
  Unpacked ub = Unpack(b);

  uint8_t exp = ua.exponent;
  if (ua.exponent > ub.exponent) {
    // Exponent of a is larger than b: shift b right by the difference,
    // stopping if we get to zero.
    int8_t diff = ua.exponent - ub.exponent;
    while (diff-- > 0 && ub.mantissa != 0) {
      ub.mantissa >>= 1;
    }
    if (ub.mantissa == 0) {
      return a;
    }
  } else {
    int8_t diff = ub.exponent - ua.exponent;
    while (diff-- > 0 && ua.mantissa != 0) {
      ua.mantissa >>= 1;
    }
    if (ua.mantissa == 0) {
      return b;
    }
    exp = ub.exponent;
  }
  uint64_t mA = (uint64_t)ua.mantissa;
  uint64_t mB = (uint64_t)ub.mantissa;
  
  // Negate mantissas as specified by sign.
  if (ua.sign == 0x80) {
    mA = -mA;
  }
  if (ub.sign == 0x80) {
    mB = -mB;
  }
  uint64_t mantissa = mA + mB;
  uint8_t sign = 0;
  
  // If result of mantissa addition is negative, set sign to 0x80 and make
  // mantissa positive.
  if ((mantissa & (1LL << 63)) != 0) {
    // Mantissa is negative, sign is 0x80.
    sign = 0x80;
    mantissa = -mantissa;
  }
  // If we have overflowed the 32 bits of mantissa, shift it right and increment
  // the exponent.
  while ((mantissa & 0xffffffff00000000LL) != 0) {
    mantissa >>= 1;
    exp++;
  }
  Unpacked r;
  r.sign = sign;
  r.exponent = exp;
  r.mantissa = (uint32_t)mantissa;
  return Pack(Round(Normalize(r)));
}

// Equality is just a strightforward bitwise comparison.
bool FPEqual(float a, float b) {
  Alias ba = {.f = a};
  Alias bb = {.f = b};
  return ba.i == bb.i;
}

bool FPNotEqual(float a, float b) {
  return !FPEqual(a, b);
}

// Less is more complex.  For a positive number we just do an integer
// comparison but that won't work for negative numbers since a negative
// number with a bigger exponent is less than a smaller exponent.
bool FPLess(float a, float b) {
  if (FPIsInfinity(b)) {
    return true;
  }
  if (FPIsInfinity(a)) {
    return false;
  }
  if (FPIsNan(a) || FPIsNan(b)) {
    return false;
  }
  Alias ba = {.f = a};
  Alias bb = {.f = b};
   if (ba.si < 0) {
    // If a is negative, it's less than a positive b.
    if (bb.si < 0) {
      return bb.si < ba.si;
    }
    return true;
  }
  if (bb.si < 0) {
    return false;
  }
  // Both positive, simple integer comparison.
  return ba.si < bb.si;
}

bool FPGreater(float a, float b) {
  return FPLess(b, a);
}

bool FPGreaterEqual(float a, float b) {
  return !FPLess(a, b);
}

bool FPLessEqual(float a, float b) {
  return !FPGreater(a, b);
}

// Convert a number from integer to floating point.
float ToFloat(int32_t v) {
  if (v == 0) {
    return 0;
  }
  Unpacked u = {0};
  if (v < 0) {
    u.sign = 0x80;
    v = -v;
  }
  u.exponent = 127;     // Start out at bias value for exponent.
  for (;;) {
    u.mantissa >>= 1;
    u.mantissa |= (v & 1) << 31;
    v >>= 1;
    if (v == 0) {
      break;
    }
    u.exponent++;
  }
  return Pack(Round(u));
}

int32_t FromFloat(float f) {
  if (FPIsZero(f) || FPIsNan(f) || FPIsInfinity(f)) {
    return 0;
  }
  Unpacked u = Unpack(f);
  if (u.exponent < 127 || u.exponent > (127+31)) {
    // Negative exponent is < 1 so this can't be an integer.
    // An exponent greater than 31 is too big.
    return 0;
  }
  // Remove bias from exponent and increment by 1 (2^0 == 1);
  int8_t exp = u.exponent - 127 + 1;
  int32_t v = 0;
  while (exp > 0) {
    // Shift the mantissa into the value, increment exp for each shift.
    v <<= 1;
    v |= (u.mantissa >> 31);
    u.mantissa <<= 1;
    exp--;
  }
  if (u.sign == 0x80) {
    v = -v;
  }
  return v;
}

// Subtraction is an addition of a negative.
float FPSub(float a, float b) {
  return FPAdd(a, -b);
}

// Fixed point number manipulation.

void FixedIncrement128(uint64_t a[2]) {
  for (uint8_t i = 0; i < 2; a++, i++) {
     ++(*a);
     if (*a != 0) {
       break;
     }
   }
}

void FixedIncrement256(uint64_t a[4]) {
  for (uint8_t i = 0; i < 4; a++, i++) {
    ++(*a);
    if (*a != 0) {
      break;
    }
  }
}

uint64_t FixedAdd64(uint64_t* a, uint64_t* b, uint64_t carry_in) {
  uint64_t na = *a + *b + carry_in;
  uint64_t carry_out = na < *a;
  *a = na;
  return carry_out;
}

void FixedAdd256(uint64_t a[4], uint64_t b[4]) {
  uint64_t carry = 0;
  for (int i = 0; i < 4; a++, b++, i++) {
     carry = FixedAdd64(a, b, carry);
  }
}

void FixedAdd128(uint64_t a[2], uint64_t b[2]) {
  uint64_t carry = 0;
  for (int i = 0; i < 2; a++, b++, i++) {
     carry = FixedAdd64(a, b, carry);
  }
}

int64_t FixedROL64(uint64_t* a, uint64_t carry_in) {
  int64_t carry_out = (*a & (1LL << 63)) != 0;
  *a <<= 1;
  *a |= carry_in;
  return carry_out;
}

int64_t FixedASL64(uint64_t* a) {
  int64_t carry = (*a & (1LL << 63)) != 0;
  *a <<= 1;
  return carry;
}

int64_t FixedROR64(uint64_t* a, uint64_t carry_in) {
  int64_t carry_out = (*a & 1) != 0;
  *a >>= 1;
  *a |= carry_in << 63;
  return carry_out;
}

int64_t FixedLSR64(uint64_t* a) {
  int64_t carry = (*a & 1) != 0;
  *a >>= 1;
  return carry;
}

void FixedLShift128(uint64_t a[2]) {
  int64_t carry = FixedASL64(a++);
  for (int i = 0; i < 2 - 1; i++) {
    carry = FixedROL64(a++, carry);
  }
}

void FixedLShift256(uint64_t a[4]) {
  int64_t carry = FixedASL64(a++);
  for (int i = 0; i < 4 - 1; i++) {
    carry = FixedROL64(a++, carry);
  }
}

void FixedRShift256(uint64_t a[4]) {
  a += 4 - 1;
  int64_t carry = FixedLSR64(a--);
  for (int i = 0; i < 4 - 1; i++) {
    carry = FixedROR64(a--, carry);
  }
}

// Multiply by 10 by x*8 + x*2.
// Puts result in a.
void FixedMultiplyByTen256(uint64_t a[4]) {
  uint64_t t[4];
  memcpy(t, a, sizeof(t));
  
  // t = a * 8
  for (uint8_t i = 0; i < 3; i++) {
    FixedLShift256(t);
  }

  // a = a * 2
  FixedLShift256(a);
  
  // a = a + t
  FixedAdd256(a, t);
}

// Multiply by 10 by x*8 + x*2.
// Puts result in a.
void FixedMultiplyByTen128(uint64_t a[2]) {
  uint64_t t[2];
  memcpy(t, a, sizeof(t));
  
  // t = a * 8
  for (uint8_t i = 0; i < 3; i++) {
    FixedLShift128(t);
  }

  // a = a * 2
  FixedLShift128(a);
  
  // a = a + t
  FixedAdd128(a, t);
}

uint8_t FixedDivMod64ByTen64(uint64_t a[1]){
  uint8_t rem = *a % 10;
  *a /= 10;
  return rem;
}

bool FixedIsZeroUpper128(uint64_t a[4]) {
  uint64_t* p = &a[2];
  for (uint8_t i = 0 ; i < 2; i++) {
    if (*p++ != 0) {
      return false;
    }
  }
  return true;
}

bool FixedIsOne128(uint64_t a[2]) {
  if (*a++ != 1) {
    return false;
  }
  for (uint8_t i = 1 ; i < 2; i++) {
    if (*a++ != 0) {
      return false;
    }
  }
  return true;
}

bool FixedIsZero128(uint64_t v[2]) {
  uint64_t *p = v;
  for (uint8_t i = 0; i < 2; i++) {
    if (*p++ != 0) {
      return false;
    }
  }
  return true;
}

bool FixedIsZero256(uint64_t v[4]) {
  uint64_t *p = v;
  for (uint8_t i = 0; i < 4; i++) {
    if (*p++ != 0) {
      return false;
    }
  }
  return true;
}

// Divide a by 10, where a is a big number.  Return
// the modulus.  A is modified to be a/10.
uint8_t FixedDivideByTen128(uint64_t a[2]){
  uint64_t quotient[2] = {0};
  uint8_t rem = 0;
  const int kHiWord = 2 - 1;
  for (uint16_t i = 0; i < 2 * 64; i++) {
    // Shift high bit of a into rem and shift a left by one.
    rem <<= 1;
    if ((a[kHiWord] & (1LL << 63)) != 0) {
      rem |= 1;
    }
    FixedLShift128(a);
    FixedLShift128(quotient);
    if (rem >= 10) {
       FixedIncrement128(quotient);
       rem -= 10;
    }
  }
  memcpy(a, quotient, sizeof(quotient));
  return rem;
}

// Divide a by 10, where a is a big number.
// A is modified to be a/10.
uint8_t FixedDivideByTen256(uint64_t a[4]) {
  // If top words are zero, use 128 division.
  if (FixedIsZero128(a + 2)) {
    return FixedDivideByTen128(a);
  }
  uint64_t quotient[4] = {0};
  uint8_t rem = 0;
  const int kHiWord = 4 - 1;
  for (uint16_t i = 0; i < 4 * 64; i++) {
    // Shift high bit of a into rem and shift a left by one.
    rem <<= 1;
    if ((a[kHiWord] & (1LL << 63)) != 0) {
      rem |= 1;
    }
    FixedLShift256(a);
    FixedLShift256(quotient);
    if (rem >= 10) {
       FixedIncrement256(quotient);
       rem -= 10;
    }
  }
  memcpy(a, quotient, sizeof(quotient));
  return rem;
}

// Round.  If top bit of the fraction part of v is set we have a
// number >= 0.5.  Round up.
void FixedRound(uint64_t v[4]) {
  if ((v[4/2 - 1] & (1LL << 63)) != 0) {
    FixedIncrement128(&v[4 / 2]);
  }
}

bool FixedIsLessThanTen128(uint64_t v[2]) {
  if (v[0] >= 10) {
    return false;
  }
  for (int i = 1; i < 2; i++) {
    if (v[i] != 0) {
      return false;
    }
  }
  return true;
}

// Collect the integer pointed to by p into the fixed point number fx.
// Returns the pointer to the character beyond the last one consumed.
const char* CollectInteger(const char* p, uint64_t fx[2]) {
  uint64_t n[2] = {0};
  while (isdigit(*p)) {
    n[0] = *p - '0';
    FixedMultiplyByTen128(fx);
    FixedAdd128(fx, n);
    p++;
  }
  return p;
}

const char* CollectPossibleSign(const char* p, bool* neg) {
  if (*p == '-') {
    *neg = true;
    p++;
  }
  if (*p == '+') {
    p++;
  }
  return p;
}

void CollectAndHandleExponent(const char* p, uint64_t fx[4]) {
  // Exponent can have a sign.
  bool negative = false;
  p = CollectPossibleSign(p, &negative);
  
  // Collect exponent and convert to binary.  This is a simple integer
  // as it can have a max value of 127.
  int exp = 0;
  while (isdigit(*p)) {
    exp = exp * 10 + *p++ - '0';
  }
  
  // Multiply or divide by the 10 to the power of exponent.
  if (negative) {
    // Exponent is negative, divide the fixed point number by 10^exp
    // iteratively.
    for (int i = 0; i < exp; i++) {
      FixedDivideByTen256(fx);
    }
  } else {
    // Exponent is positive, multiply by 10^exp.
    for (int i = 0; i < exp; i++) {
      FixedMultiplyByTen256(fx);
    }
  }
}

// Calculate the binary exponent is zero (127 with bias).
// During the normalization loops we increment or decrement the binary
// exponent until we have a binary number of 1.x.
//
// Returns the binary exponent and modifies fx.
int NormalizeAndGetExponent(uint64_t fx[4]) {
  int exp = 127;
  if (FixedIsZero128(fx + 2)) {
    // Number is less than one, shift left.
    while (!FixedIsOne128(fx + 2)) {
      FixedLShift256(fx);
      exp--;
    }
  } else {
    // Number is greater than one, shift right.
    while (!FixedIsOne128(fx + 2)) {
      FixedRShift256(fx);
      exp++;
    }
  }
  return exp;
}

float ASCIIToFloat (const char* p) {
  // Fixed point 256-bit number.
  uint64_t fx[4] = {0};
  
  // Can have a + or - prefix.
  bool negative = false;
  p = CollectPossibleSign(p, &negative);
  
  // Convert integral part to binary.  This is put into the top
  // 128 of the fixed point number - the integral part.
  p = CollectInteger(p, fx + 2);
  
  // Any decimal point?
  if (*p == '.') {
    p++;
    
    // We have a fractional part, continue accumulating into the top 128.
    const char* fract_start = p;
    p = CollectInteger(p, fx + 2);
    
    // Calculate number of digits in the fraction.
    int fraction_digits = (int)(p - fract_start);
    
    // Now shift the fractional part down to the lower half - to the right
    // of the binary point.  This is a base-10 division.
    for (int i = 0; i < fraction_digits; i++) {
      FixedDivideByTen256(fx);
    }
  }
  
  // Check for exponent.  If it's present handle it, modifying the fixed
  // point number.
  if (*p == 'e' || *p == 'E') {
    CollectAndHandleExponent(p + 1, fx);
  }
  
  // A zero is special case.
  if (FixedIsZero256(fx)) {
    return 0;
  }
  
  // We now have a fixed point binary number. Float the binary point to normalize
  // the number by shifting left or right until the integer part has the
  // value 1.
  int exp = NormalizeAndGetExponent(fx);

  // The mantissa is the upper bits of the fractional part shifted right by
  // 1 bit and the top bit set.
  uint32_t mantissa = ((uint32_t)(fx[1] >> 32) >> 1) | 0x80000000;

  Unpacked u;
  u.sign = negative ? 0x80 : 0;
  u.exponent = exp;
  u.mantissa = mantissa;
  return Pack(Round(Normalize(u)));
}



struct FloatPrinter {
  Unpacked u;
  uint8_t naninf;
  uint64_t fx[4];          
};
#define FP_NAN 1      // Non a Number
#define FP_INF 2      // Infinity.

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
void FixFloat(struct FloatPrinter* printer, float f) {
  // Check for special cases.
  if (FPIsZero(f)) {
    return;
  }
  if (FPIsNan(f)) {
    printer->naninf = FP_NAN;            // NaN
    return;
  }
  if (FPIsInfinity(f)) {
    printer->naninf = FP_INF;            // Infinity
    return;
  }
  
  // Extract the IEEE754 single precision to sign, exponent, and mantissa.
  printer->u = Unpack(f);

  // Put mantissa in the high end of the fractional part.  The is one less
  // than the exponent says.
  printer->fx[1] = (uint64_t)printer->u.mantissa << 32;
  
  // Shift the mantissa left or right by the exponent.
  // In IEE754 single precision, the exponent has a bias
  // of 127, so 127 == 0, 128=0, 126=-1
  if (printer->u.exponent < 127) {
    // Negative exponent - shift right until exp is 127.
    while (printer->u.exponent < 126) {
      FixedRShift256(printer->fx);
      ++printer->u.exponent;
    }
  } else {
    // Positive exponent, shift left.
    while (printer->u.exponent >= 127) {
      FixedLShift256(printer->fx);
      --printer->u.exponent;
    }
  }
}

// Write 0.00000 with precision number of digits after the point.
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

// Write nan or inf.
char* WriteNanInf(int8_t sign, int v, char* buf, size_t size) {
  char* end = buf + size - 4;
  char* p = end;
  if (sign != 0) {
    *p++ = '-';
  }
  strcpy(p, v == FP_NAN ? "nan" : "inf");
  return end;
}


// Write the exponent backwards starting at buf.  Return the start of
// the result.  Always writes 3 or 4 chars:
// +/-[X]XX
char* WriteExponent(int exp, char* buf) {
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
int CalculateExponent(uint64_t v[4]) {
  uint64_t t[4];
  memcpy(t, v, sizeof(t));
  int exp = 0;

  if (FixedIsZero128(&t[2])) {
    // Less than 1, multiply by 10 until >= 1
    while (FixedIsZero128(&t[2])) {
      FixedMultiplyByTen256(t);
      --exp;
    }
  } else {
    while (!FixedIsLessThanTen128(&t[2])) {
      FixedDivideByTen256(t);
      ++exp;
    }
  }
  return exp;
}

// Print the 256 bit fixed point number in the printer in scientific form
// to the buffer.  Return the address of the printed text.
char* PrintFixedPointScientific(struct FloatPrinter* printer, int precision, char* buf, size_t size) {
  if (printer->naninf != 0) {
    return WriteNanInf(printer->u.sign, printer->naninf, buf, size);
  }
  if (FixedIsZero256(printer->fx)) {
    char* end = buf + size - 1;
    *end-- = '\0';
    char* p = WriteExponent(0, end);
    *p-- = 'e';
    return WriteZero(precision, buf, p - buf + 1);
  }

  int exp = CalculateExponent(printer->fx);

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
      FixedDivideByTen256(printer->fx);
    }
  } else {
    for (int i = 0; i < shifts; i++) {
      FixedMultiplyByTen256(printer->fx);
    }
  }
  FixedRound(printer->fx);
  
  char* end = buf + size - 1;
  *end-- = '\0';
  char* p = WriteExponent(exp, end);
  *p-- = 'e';
  while (!FixedIsZero128(&printer->fx[2])) {
    if (precision == 0) {
      *p-- = '.';
    }
    uint8_t r = FixedDivideByTen128(&printer->fx[2]);
    *p-- = r + '0';
    --precision;
  }
  if (printer->u.sign) {
    *p-- = '-';
  }
  return p+1;
}


// v is a 256 bit fixed point number with the binary point at 128 bits.
char* PrintFixedPoint(struct FloatPrinter* printer, int precision, char* buf, size_t size) {
  if (printer->naninf != 0) {
    return WriteNanInf(printer->u.sign, printer->naninf, buf, size);
  }
  if (FixedIsZero256(printer->fx)) {
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
    FixedMultiplyByTen256(printer->fx);
    if (FixedIsZero128(&printer->fx[2])) {
      leading_zeroes++;
    }
  }
  // Round the whole part up if the fraction is >- 0.5.
  FixedRound(printer->fx);
  
  // Write the whole number in decimal, putting a decimal point
  // in the appropriate place.
  bool point_printed = false;
  while (!FixedIsZero128(&printer->fx[2])) {
    if (precision == 0) {
      *p-- = '.';
      point_printed = true;
    }
    uint8_t r = FixedDivideByTen128(&printer->fx[2]);
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
  if (printer->u.sign != 0) {
    *p-- = '-';
  }
  return p + 1;
}

char* FloatToASCII(float f, int precision, char* buf, size_t size) {
  struct FloatPrinter printer = {0};
  FixFloat(&printer, f);
  return PrintFixedPoint(&printer, precision, buf, size);
}

char* FloatToASCIIScientific(float f, int precision, char* buf, size_t size) {
  struct FloatPrinter printer = {0};
  FixFloat(&printer, f);
  return PrintFixedPointScientific(&printer, precision, buf, size);
}

int main(int argc, const char * argv[]) {
  bool ok = true;
  
  float a = -5758;
  float b = 70060;
  float  c = a / b;
  float  d = FPDivide(a, b);
  int32_t e = *(int32_t*)&c;
  int32_t f = *(int32_t*)&d;
  int32_t g = *(int32_t*)&a;
  int32_t h = *(int32_t*)&b;
  printf("%g(%" PRIx32 ") and %g(%" PRIx32 "): right: %f(%" PRIx32 "), wrong: %f(%" PRIx32 ")\n", a, g, b, h, c, e, d, f);

  for (int i = 0; i < 1; i++) {
    float a = (float)rand() + 1/(float)rand();
    float b = (float)rand() + 1/(float)rand();
    int32_t v = rand();
    
    if (rand() & 1) {
      a = -a;
    }
    if (rand() & 1) {
      b = -b;
    }
    // Multiplication.,
    printf("multiply %d\n", i);
    float c = a * b;
    float d = FPMultiply(a, b);
    if (c != d) {
      int32_t e = *(int32_t*)&c;
      int32_t f = *(int32_t*)&d;
      printf("Multiply wrong %g and %g: right: %f(%" PRIx32 "), wrong: %f(%" PRIx32 ")\n", a, b, c, e, d, f);
      ok = false;
    }
    
    // Division.
    printf("divide %d\n", i);
    c = a / b;
    d = FPDivide(a, b);
    if (c != d) {
      int32_t e = *(int32_t*)&c;
      int32_t f = *(int32_t*)&d;
      printf("Divide wrong %g and %g: right: %f(%" PRIx32 "), wrong: %f(%" PRIx32 ")\n", a, b, c, e, d, f);
      ok = false;
   }
    
    // Addition.
    printf("add %d\n", i);
    c = a + b;
    d = FPAdd(a, b);
    if (c != d) {
      int32_t e = *(int32_t*)&c;
      int32_t f = *(int32_t*)&d;
      printf("Add wrong %g and %g: right: %f(%" PRIx32 "), wrong: %f(%" PRIx32 ")\n", a, b, c, e, d, f);
      ok = false;
   }
    
    // Subtraction.
    printf("sub %d\n", i);
    c = a - b;
    d = FPSub(a, b);
    if (c != d) {
      int32_t e = *(int32_t*)&c;
      int32_t f = *(int32_t*)&d;
      printf("Subtract wrong %g and %g: right: %f(%" PRIx32 "), wrong: %f(%" PRIx32 ")\n", a, b, c, e, d, f);
      ok = false;
   }
    
    // Less
    printf("less %d\n", i);
    bool g = a < b;
    bool h = FPLess(a, b);
    if (g != h) {
      printf("Less wrong: %g and %g: right %d, wrong %d\n", a, b, g, h);
      ok = false;
    }
    
    // To float
    printf("to float %d\n", i);
    c = v;
    d = ToFloat(v);
    if (c != d) {
      int32_t e = *(int32_t*)&c;
      int32_t f = *(int32_t*)&d;
      printf("ToFloat wrong %d: right: %f(%" PRIx32 "), wrong: %f(%" PRIx32 ")\n", v, c, e, d, f);
      ok = false;
    }
    
    // From float
    printf("from float %d\n", i);
    v = a;
    int32_t w = FromFloat(a);
    if (v != w) {
      int32_t e = *(int32_t*)&v;
      int32_t f = *(int32_t*)&w;
      printf("FromFloat wrong %g: right: %d(%" PRIx32 "), wrong: %d(%" PRIx32 ")\n", a, v, e, w, f);
      ok = false;
    }
 }
  if (ok) {
    printf("OK\n");
  }
  
  {
    float f = ASCIIToFloat("1.525");
    printf("%f\n", f);
    
    char buf[256];
    char *p = FloatToASCII(f, 6, buf, sizeof(buf));
    printf("%s\n", p);
    
    p = FloatToASCII(0.001, 6, buf, sizeof(buf));
    printf("%s\n", p);
    
    p = FloatToASCIIScientific(12.75e34, 6, buf, sizeof(buf));
    printf("%s\n", p);
 }
}

