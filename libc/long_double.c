//
//  Runtime helpers for distinct long double (IEEE binary128 / Intel 80-bit),
//  and for __int128 division.  The compiler lowers arithmetic and conversions
//  to these functions.
//

#include <stdint.h>

#include "../c_compiler/support/fp_extended.h"

#ifndef __DAVECC_LDBL_FORMAT__
#define __DAVECC_LDBL_FORMAT__ kFPExtFormatFloat64
#endif

typedef struct {
  uint64_t lo;
  uint64_t hi;
} __davecc_ld_bits;

static int LdFormat(void) { return __DAVECC_LDBL_FORMAT__; }

void __davecc_ld_add(__davecc_ld_bits* result, const __davecc_ld_bits* a,
                     const __davecc_ld_bits* b) {
  FPBits r = FPAdd((FPBits){a->lo, a->hi}, (FPBits){b->lo, b->hi}, LdFormat());
  result->lo = r.lo;
  result->hi = r.hi;
}

void __davecc_ld_sub(__davecc_ld_bits* result, const __davecc_ld_bits* a,
                     const __davecc_ld_bits* b) {
  FPBits r = FPSub((FPBits){a->lo, a->hi}, (FPBits){b->lo, b->hi}, LdFormat());
  result->lo = r.lo;
  result->hi = r.hi;
}

void __davecc_ld_mul(__davecc_ld_bits* result, const __davecc_ld_bits* a,
                     const __davecc_ld_bits* b) {
  FPBits r = FPMul((FPBits){a->lo, a->hi}, (FPBits){b->lo, b->hi}, LdFormat());
  result->lo = r.lo;
  result->hi = r.hi;
}

void __davecc_ld_div(__davecc_ld_bits* result, const __davecc_ld_bits* a,
                     const __davecc_ld_bits* b) {
  FPBits r = FPDiv((FPBits){a->lo, a->hi}, (FPBits){b->lo, b->hi}, LdFormat());
  result->lo = r.lo;
  result->hi = r.hi;
}

void __davecc_ld_neg(__davecc_ld_bits* result, const __davecc_ld_bits* a) {
  FPBits r = FPNeg((FPBits){a->lo, a->hi}, LdFormat());
  result->lo = r.lo;
  result->hi = r.hi;
}

int __davecc_ld_cmp(const __davecc_ld_bits* a, const __davecc_ld_bits* b) {
  return FPCompare((FPBits){a->lo, a->hi}, (FPBits){b->lo, b->hi}, LdFormat());
}

void __davecc_ld_from_f64(__davecc_ld_bits* result, double value) {
  FPBits r = FPBitsFromF64(value, LdFormat());
  result->lo = r.lo;
  result->hi = r.hi;
}

void __davecc_ld_from_f32(__davecc_ld_bits* result, float value) {
  FPBits r = FPBitsFromF32(value, LdFormat());
  result->lo = r.lo;
  result->hi = r.hi;
}

void __davecc_ld_from_i64(__davecc_ld_bits* result, long long value) {
  FPBits r = FPBitsFromI64(value, LdFormat());
  result->lo = r.lo;
  result->hi = r.hi;
}

void __davecc_ld_from_u64(__davecc_ld_bits* result, unsigned long long value) {
  FPBits r = FPBitsFromU64(value, LdFormat());
  result->lo = r.lo;
  result->hi = r.hi;
}

double __davecc_ld_to_f64(const __davecc_ld_bits* value) {
  return FPBitsToF64((FPBits){value->lo, value->hi}, LdFormat());
}

float __davecc_ld_to_f32(const __davecc_ld_bits* value) {
  return FPBitsToF32((FPBits){value->lo, value->hi}, LdFormat());
}

long long __davecc_ld_to_i64(const __davecc_ld_bits* value) {
  return FPBitsToI64((FPBits){value->lo, value->hi}, LdFormat());
}

unsigned long long __davecc_ld_to_u64(const __davecc_ld_bits* value) {
  return FPBitsToU64((FPBits){value->lo, value->hi}, LdFormat());
}

// __int128 division and remainder, on the same two-word objects.  Division by
// zero yields zero rather than trapping.
static void I128UnsignedDivide(__davecc_ld_bits n, __davecc_ld_bits d,
                               __davecc_ld_bits* quotient,
                               __davecc_ld_bits* remainder) {
  __davecc_ld_bits q = {0, 0};
  __davecc_ld_bits r = {0, 0};
  if (d.lo == 0 && d.hi == 0) {
    *quotient = q;
    *remainder = r;
    return;
  }
  if (n.hi == 0 && d.hi == 0) {
    q.lo = n.lo / d.lo;
    r.lo = n.lo % d.lo;
    *quotient = q;
    *remainder = r;
    return;
  }
  for (int i = 127; i >= 0; i--) {
    uint64_t carry = r.hi >> 63;
    uint64_t bit = i >= 64 ? (n.hi >> (i - 64)) & 1 : (n.lo >> i) & 1;
    r.hi = (r.hi << 1) | (r.lo >> 63);
    r.lo = (r.lo << 1) | bit;
    if (carry != 0 || r.hi > d.hi || (r.hi == d.hi && r.lo >= d.lo)) {
      uint64_t borrow = r.lo < d.lo;
      r.lo -= d.lo;
      r.hi -= d.hi + borrow;
      if (i >= 64) {
        q.hi |= (uint64_t)1 << (i - 64);
      } else {
        q.lo |= (uint64_t)1 << i;
      }
    }
  }
  *quotient = q;
  *remainder = r;
}

static __davecc_ld_bits I128Negate(__davecc_ld_bits v) {
  __davecc_ld_bits result;
  result.lo = ~v.lo + 1;
  result.hi = ~v.hi + (result.lo == 0);
  return result;
}

static __davecc_ld_bits I128Magnitude(__davecc_ld_bits v) {
  if ((v.hi >> 63) != 0) {
    return I128Negate(v);
  }
  return v;
}

void __davecc_i128_udiv(__davecc_ld_bits* result, const __davecc_ld_bits* a,
                        const __davecc_ld_bits* b) {
  __davecc_ld_bits remainder;
  I128UnsignedDivide(*a, *b, result, &remainder);
}

void __davecc_i128_umod(__davecc_ld_bits* result, const __davecc_ld_bits* a,
                        const __davecc_ld_bits* b) {
  __davecc_ld_bits quotient;
  I128UnsignedDivide(*a, *b, &quotient, result);
}

void __davecc_i128_sdiv(__davecc_ld_bits* result, const __davecc_ld_bits* a,
                        const __davecc_ld_bits* b) {
  __davecc_ld_bits quotient;
  __davecc_ld_bits remainder;
  I128UnsignedDivide(I128Magnitude(*a), I128Magnitude(*b), &quotient,
                     &remainder);
  if (((a->hi ^ b->hi) >> 63) != 0) {
    quotient = I128Negate(quotient);
  }
  *result = quotient;
}

void __davecc_i128_smod(__davecc_ld_bits* result, const __davecc_ld_bits* a,
                        const __davecc_ld_bits* b) {
  __davecc_ld_bits quotient;
  __davecc_ld_bits remainder;
  I128UnsignedDivide(I128Magnitude(*a), I128Magnitude(*b), &quotient,
                     &remainder);
  if ((a->hi >> 63) != 0) {
    remainder = I128Negate(remainder);
  }
  *result = remainder;
}

#include "../c_compiler/support/fp_extended.c"
