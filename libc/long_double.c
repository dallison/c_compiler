//
//  Runtime helpers for distinct long double (IEEE binary128 / Intel 80-bit).
//  The compiler lowers arithmetic and conversions to these functions.
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

double __davecc_ld_to_f64(const __davecc_ld_bits* value) {
  return FPBitsToF64((FPBits){value->lo, value->hi}, LdFormat());
}

float __davecc_ld_to_f32(const __davecc_ld_bits* value) {
  return FPBitsToF32((FPBits){value->lo, value->hi}, LdFormat());
}

long long __davecc_ld_to_i64(const __davecc_ld_bits* value) {
  return FPBitsToI64((FPBits){value->lo, value->hi}, LdFormat());
}

#include "../c_compiler/support/fp_extended.c"
