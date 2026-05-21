//
//  main.c
//  fdiv
//
//  Created by David Allison on 12/6/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Divide a by b.
void divide(uint32_t a, uint32_t b, uint32_t* quotient, uint32_t* rem){
  if (b == 0) {
    printf("division by zero\n");
    return;
  }
  *quotient = 0;
  *rem = 0;
  for (int i = 0; i < 32; i++) {
    // Shift high bit of a into rem and shift a left by one.
    *rem <<= 1;
    if ((a & 0x80000000) != 0) {
      *rem |= 1;
    }
    a <<= 1;
    *quotient <<= 1;
    if (*rem >= b) {
       (*quotient)++;
       *rem -= b;
    }
  }
}

// Divide 48-bit numbers.  For mantissa division we need to divide a
// 48 bit number with the upper 24 bits populated by another with the
// lower 24 bits populated.
//
// +--------------------+------------------+
// |           a        |         0        |
// +--------------------+------------------+
// Divided by
// +--------------------+------------------+
// |           0        |         b        |
// +--------------------+------------------+
//
// We can shortcut this a little by noticing that for the first 24 shifts
// we will be putting 'a' into 'rem' because 'b' cannot be less than 'rem'
// until we get the top bit set (both numbers are normalized with a 1 in the
// top bit).  So we put 'a' into 'rem' initially and shift 24 times.  We
// don't need 'a' any more since we've put it in rem.  We also need some 'rem'
// to be 32 bits wide since we will be shifting it left once per
void udiv6(uint32_t a, uint32_t b, uint32_t* quotient, uint32_t* rem){
  if (b == 0) {
    printf("division by zero\n");
    return;
  }
  *quotient = 0;
  // Since we are dividing by a 24 bit number the top 24 bits of a
  // will always be shifted into rem.  So lets start with rem = a
  // and only do 24 iterations,
  *rem = a;
  for (int i = 0; i < 24; i++) {
    // For the first iteration we already have 'rem' containing 'a'
    // so we don't shift it or the quotient.
    if (i != 0) {
      // Shift high bit of a into rem and shift a left by one.
      *rem <<= 1;
      *quotient <<= 1;
    }
    if (*rem >= b) {
       (*quotient)++;
       *rem -= b;
    }
  }
}

uint32_t rshift(uint32_t x) {
  while ((x & 1) == 0) {
    x >>= 1;
  }
  return x;
}

uint32_t Normalize(uint32_t x, uint8_t* exp) {
  while ((x & (1 << 23)) == 0) {
    x <<= 1;
    (*exp)--;
  }
  return x;
}

// 24 bit round.  If bits 23 or 22 of remainder r is set (0.5 or above)
// increment q
uint32_t Round(uint32_t q, uint32_t r) {
  if ((r & (3 << 22)) != 0) {
    ++q;
  }
  return q;
}

int main(int argc, const char * argv[]) {
  char buf[32];
  printf("Number 1:");
  fflush(stdout);
  fgets(buf, sizeof(buf), stdin);
  float f1 = strtod(buf, NULL);
  printf("Number 2:");
  fflush(stdout);
  fgets(buf, sizeof(buf), stdin);
  float f2 = strtod(buf, NULL);

  uint32_t bits = *(uint32_t*)&f1;
  uint8_t sign1 = bits >> 31;
  uint8_t exp1 = (bits >> 23) & 0xff;
  uint32_t mantissa1 = ((bits & 0x7fffff) | 0x800000) << 0;

  bits = *(uint32_t*)&f2;
  uint8_t sign2 = bits >> 31;
  uint8_t exp2 = (bits >> 23) & 0xff;
  uint32_t mantissa2 = ((bits & 0x7fffff) | 0x800000) << 0;
  
  uint8_t sign = sign1 ^ sign2;
  
  float f = f1 / f2;
  bits = *(uint32_t*)&f;
  uint8_t sign3 = bits >> 31;
  uint8_t exp3 = (bits >> 23) & 0xff;
  uint32_t mantissa3 = ((bits & 0x7fffff) | 0x800000) << 0;

  printf("f: %08x m: %08x f:%f\n", *(uint32_t*)&f, mantissa3, f);
  printf("m1: %08x m2:%08x\n", mantissa1, mantissa2);
  uint32_t q, r;
  udiv6(mantissa1, mantissa2, &q, &r);
  
  uint8_t exp = exp1 - exp2 + 0x7f;
  q = Normalize(Round(q, r), &exp);
  // Clear top bit of mantissa.
  q &= ~(1 << 23);

  
  // Pack to IEEE754
  uint32_t packed = q | (exp << 23) | (sign << 31);
  printf("%08x %08x exp %d: %08x %f\n", q, r, exp, packed, *(float*)&packed);

}
