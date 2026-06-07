//
//  _fpfuncs.h
//  c_compiler
//
//  Created by David Allison on 12/1/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#ifndef _fpfuncs_h
#define _fpfuncs_h

#include <stdint.h>
#include <stdbool.h>

#if defined(__6502__)
// Double is single precision.  Need 128 bit for both integer and
// fractional part.  This is a total of 256 bits or 4X64.
#define FIXED_SIZE_WORDS 4
#define FIXED_SIZE_HALF 2
#define EXP_BIAS 127
typedef uint8_t Exponent;
typedef uint32_t Mantissa;
#define DOUBLE_IS_SINGLE 1
#elif defined(__aarch64__) || defined(__arm__)
// Match 6502-sized buffer until aarch64 codegen handles large stack frames.
#define FIXED_SIZE_WORDS 4
#define FIXED_SIZE_HALF 2
#define EXP_BIAS 1023
typedef uint16_t Exponent;
typedef uint64_t Mantissa;
#define DOUBLE_IS_SINGLE 0
#else
// Double is double precision.  The max power-of-2 exponent is 2^11, which
// is 2048 bits.  Twice that is 4096 bits so we need 4096/64 = 64 words.
#define FIXED_SIZE_WORDS 64
#define FIXED_SIZE_HALF 32
#define EXP_BIAS 1023
typedef uint16_t Exponent;
typedef uint64_t Mantissa;
#define DOUBLE_IS_SINGLE 0
#endif

#define FIXED_SIZE_WHOLE_PART FIXED_SIZE_HALF

void __IncrementHalf(uint64_t a[FIXED_SIZE_HALF]);
void __Increment(uint64_t a[FIXED_SIZE_WORDS]);
uint64_t __Add64(uint64_t* a, uint64_t* b, uint64_t carry_in);
void __Add(uint64_t a[FIXED_SIZE_WORDS], uint64_t b[FIXED_SIZE_WORDS]);
void __AddHalf(uint64_t a[FIXED_SIZE_HALF], uint64_t b[FIXED_SIZE_HALF]);
int64_t __ROL(uint64_t* a, uint64_t carry_in);
int64_t __ASL(uint64_t* a);
int64_t __ROR(uint64_t* a, uint64_t carry_in);
int64_t __LSR(uint64_t* a);
void __LShiftHalf(uint64_t a[FIXED_SIZE_HALF]);
void __LShift(uint64_t a[FIXED_SIZE_WORDS]);
void __RShift(uint64_t a[FIXED_SIZE_WORDS]);
void __MultiplyBy10Half(uint64_t a[FIXED_SIZE_HALF]);
void __MultiplyBy10(uint64_t a[FIXED_SIZE_WORDS]);
uint8_t __DivMod64By10(uint64_t a[1]);
bool __IsZeroUpper(uint64_t a[FIXED_SIZE_WORDS]);
uint8_t __DivModBy10Half(uint64_t a[FIXED_SIZE_HALF]);
uint8_t __DivideBy10(uint64_t a[FIXED_SIZE_WORDS]);
void __Round(uint64_t v[FIXED_SIZE_WORDS]);
bool __IsZeroHalf(uint64_t v[FIXED_SIZE_HALF]);
bool __IsZero(uint64_t v[FIXED_SIZE_WORDS]);
bool __IsLessThan10(uint64_t v[FIXED_SIZE_HALF]);
bool __IsOneHalf(uint64_t a[FIXED_SIZE_WORDS]);


// For 6502 we need to optimize the long calculations in assembly
// language.  The compiler output is much slower because it doesn't
// take advantage of the carry flag when shifting or doing multiple
// byte adds.
#if defined(__6502__)
extern void __add128(uint64_t* a, uint64_t* b);
extern void __add256(uint64_t* a, uint64_t* b);
extern void __lshift128(uint64_t a[2]);
extern void __lshift256(uint64_t a[4]);
extern void __rshift256(uint64_t a[4]);
extern void __inc128(uint64_t a[2]);
extern void __inc256(uint64_t a[4]);
extern void __unpackIEEE754(uint32_t* bits, struct FloatPrinter* f);
extern double __packIEEE754(int sign, int exp, uint64_t mantissa);
#endif

#endif /* _fpfuncs_h */
