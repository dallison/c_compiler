// fpclassify / signbit for the hosted Darwin runtime.  math_extra.c also
// defines the libm entry points, which would clash with libSystem.

#include <math.h>
#include <stdint.h>

int __davecc_fpclassify(double value) {
  union {
    double value;
    uint64_t bits;
  } representation;
  uint64_t exponent;
  uint64_t fraction;
  representation.value = value;
  exponent = (representation.bits >> 52) & 0x7ff;
  fraction = representation.bits & 0xfffffffffffffULL;
  if (exponent == 0x7ff) return fraction == 0 ? FP_INFINITE : FP_NAN;
  if (exponent == 0) return fraction == 0 ? FP_ZERO : FP_SUBNORMAL;
  return FP_NORMAL;
}

int __davecc_signbit(double value) {
  union {
    double value;
    uint64_t bits;
  } representation;
  representation.value = value;
  return (representation.bits >> 63) != 0;
}

int __davecc_fpclassifyl(long double value) {
  return __davecc_fpclassify((double)value);
}

int __davecc_signbitl(long double value) {
  return __davecc_signbit((double)value);
}
