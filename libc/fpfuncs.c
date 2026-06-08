//
//  fpfuncs.c
//  c_compiler
//
//  Created by David Allison on 12/1/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "_fpfuncs.h"

void __IncrementHalf(uint64_t a[FIXED_SIZE_HALF]) {
#if defined(__6502__)
  __inc128(a);
#else
  for (uint8_t i = 0; i < FIXED_SIZE_HALF; a++, i++) {
     ++(*a);
     if (*a != 0) {
       break;
     }
   }
#endif
}

void __Increment(uint64_t a[FIXED_SIZE_WORDS]) {
#if defined(__6502__)
  __inc256(a);
#else
  for (uint8_t i = 0; i < FIXED_SIZE_WORDS; a++, i++) {
    ++(*a);
    if (*a != 0) {
      break;
    }
  }
#endif
}

#if !defined(__6502__)
uint64_t __Add64(uint64_t* a, uint64_t* b, uint64_t carry_in) {
  uint64_t sum = *a + carry_in;
  uint64_t carry_out = sum < *a;
  uint64_t na = sum + *b;
  carry_out |= na < sum;
  *a = na;
  return carry_out;
}
#endif

void __Add(uint64_t a[FIXED_SIZE_WORDS], uint64_t b[FIXED_SIZE_WORDS]) {
#if defined(__6502__)
  __add256(a, b);
#else
  uint64_t carry = 0;
  for (int i = 0; i < FIXED_SIZE_WORDS; a++, b++, i++) {
     carry = __Add64(a, b, carry);
  }
#endif
}

void __AddHalf(uint64_t a[FIXED_SIZE_HALF], uint64_t b[FIXED_SIZE_HALF]) {
#if defined(__6502__)
  __add128(a, b);
#else
  uint64_t carry = 0;
  for (int i = 0; i < FIXED_SIZE_HALF; a++, b++, i++) {
     carry = __Add64(a, b, carry);
  }
#endif
}

#if !defined(__6502__)
int64_t __ROL(uint64_t* a, uint64_t carry_in) {
  int64_t carry_out = (*a & 0x8000000000000000ULL) != 0;
  *a <<= 1;
  *a |= carry_in;
  return carry_out;
}

int64_t __ASL(uint64_t* a) {
  int64_t carry = (*a & 0x8000000000000000ULL) != 0;
  *a <<= 1;
  return carry;
}

int64_t __ROR(uint64_t* a, uint64_t carry_in) {
  int64_t carry_out = (*a & 1) != 0;
  *a >>= 1;
  *a |= carry_in << 63;
  return carry_out;
}

int64_t __LSR(uint64_t* a) {
  int64_t carry = (*a & 1) != 0;
  *a >>= 1;
  return carry;
}
#endif

void __LShiftHalf(uint64_t a[FIXED_SIZE_HALF]) {
#if defined(__6502__)
  __lshift128(a);
#else
  int64_t carry = __ASL(a++);
  for (int i = 0; i < FIXED_SIZE_HALF - 1; i++) {
    carry = __ROL(a++, carry);
  }
#endif
}

void __LShift(uint64_t a[FIXED_SIZE_WORDS]) {
#if defined(__6502__)
  __lshift256(a);
#else
  int64_t carry = __ASL(a++);
  for (int i = 0; i < FIXED_SIZE_WORDS - 1; i++) {
    carry = __ROL(a++, carry);
  }
#endif
}

void __RShift(uint64_t a[FIXED_SIZE_WORDS]) {
#if defined(__6502__)
  __rshift256(a);
#else
  a += FIXED_SIZE_WORDS - 1;
  int64_t carry = __LSR(a--);
  for (int i = 0; i < FIXED_SIZE_WORDS - 1; i++) {
    carry = __ROR(a--, carry);
  }
#endif
}

// Multiply by 10 by x*8 + x*2.
// Puts result in a.
void __MultiplyBy10(uint64_t a[FIXED_SIZE_WORDS]) {
  uint64_t t[FIXED_SIZE_WORDS];
  memcpy(t, a, sizeof(t));
  
  // t = a * 8
  for (uint8_t i = 0; i < 3; i++) {
    __LShift(t);
  }

  // a = a * 2
  __LShift(a);
  
  // a = a + t
  __Add(a, t);
}

// Multiply by 10 by x*8 + x*2.
// Puts result in a.
void __MultiplyBy10Half(uint64_t a[FIXED_SIZE_HALF]) {
  uint64_t t[FIXED_SIZE_HALF];
  memcpy(t, a, sizeof(t));
  
  // t = a * 8
  for (uint8_t i = 0; i < 3; i++) {
    __LShiftHalf(t);
  }

  // a = a * 2
  __LShiftHalf(a);
  
  // a = a + t
  __AddHalf(a, t);
}

uint8_t __DivMod64By10(uint64_t a[1]){
  uint8_t rem = *a % 10;
  *a /= 10;
  return rem;
}

bool __IsZeroUpper(uint64_t a[FIXED_SIZE_WORDS]) {
  uint64_t* p = &a[FIXED_SIZE_HALF];
  for (uint8_t i = 0 ; i < FIXED_SIZE_HALF; i++) {
    if (*p++ != 0) {
      return false;
    }
  }
  return true;
}

bool __IsOneHalf(uint64_t a[FIXED_SIZE_WORDS]) {
  if (*a++ != 1) {
    return false;
  }
  for (uint8_t i = 1 ; i < FIXED_SIZE_HALF; i++) {
    if (*a++ != 0) {
      return false;
    }
  }
  return true;
}

// Divide a by 10, where a is a big number.  Return
// the modulus.  A is modified to be a/10.
uint8_t __DivModBy10Half(uint64_t a[FIXED_SIZE_HALF]){
  // If the upper words are clear, use native 64-bit division instead of the
  // multiword long division path.
  bool upper_zero = true;
  for (uint8_t i = 1; i < FIXED_SIZE_HALF; i++) {
    if (a[i] != 0) {
      upper_zero = false;
      break;
    }
  }
  if (upper_zero) {
    return __DivMod64By10(a);
  }
  uint64_t quotient[FIXED_SIZE_HALF] = {0};
  uint8_t rem = 0;
  const int kHiWord = FIXED_SIZE_HALF - 1;
  for (uint16_t i = 0; i < FIXED_SIZE_HALF * 64; i++) {
    // Shift high bit of a into rem and shift a left by one.
    rem <<= 1;
    if ((a[kHiWord] & (1LL << 63)) != 0) {
      rem |= 1;
    }
    __LShiftHalf(a);
    __LShiftHalf(quotient);
    if (rem >= 10) {
       __IncrementHalf(quotient);
       rem -= 10;
    }
  }
  memcpy(a, quotient, sizeof(quotient));
  return rem;
}

// Divide a by 10, where a is a big number.
// A is modified to be a/10.
uint8_t __DivideBy10(uint64_t a[FIXED_SIZE_WORDS]) {
  // If top words are zero, use half division.
  if (__IsZeroUpper(a)) {
    return __DivModBy10Half(a);
  }
  uint64_t quotient[FIXED_SIZE_WORDS] = {0};
  uint8_t rem = 0;
  const int kHiWord = FIXED_SIZE_WORDS - 1;
  for (uint16_t i = 0; i < FIXED_SIZE_WORDS * 64; i++) {
    // Shift high bit of a into rem and shift a left by one.
    rem <<= 1;
    if ((a[kHiWord] & (1LL << 63)) != 0) {
      rem |= 1;
    }
    __LShift(a);
    __LShift(quotient);
    if (rem >= 10) {
       __Increment(quotient);
       rem -= 10;
    }
  }
  memcpy(a, quotient, sizeof(quotient));
  return rem;
}

// Round.  If top bit of the fraction part of v is set we have a
// number >= 0.5.  Round up.
void __Round(uint64_t v[FIXED_SIZE_WORDS]) {
  if ((v[FIXED_SIZE_WORDS/2 - 1] & (1LL << 63)) != 0) {
    __IncrementHalf(&v[FIXED_SIZE_WORDS / 2]);
  }
}

bool __IsZeroHalf(uint64_t v[FIXED_SIZE_HALF]) {
  uint64_t *p = v;
  for (uint8_t i = 0; i < FIXED_SIZE_HALF; i++) {
    if (*p++ != 0) {
      return false;
    }
  }
  return true;
}

bool __IsZero(uint64_t v[FIXED_SIZE_WORDS]) {
  uint64_t *p = v;
  for (uint8_t i = 0; i < FIXED_SIZE_WORDS; i++) {
    if (*p++ != 0) {
      return false;
    }
  }
  return true;
}

bool __IsLessThan10(uint64_t v[FIXED_SIZE_HALF]) {
  if (v[0] >= 10) {
    return false;
  }
  for (int i = 1; i < FIXED_SIZE_HALF; i++) {
    if (v[i] != 0) {
      return false;
    }
  }
  return true;
}
