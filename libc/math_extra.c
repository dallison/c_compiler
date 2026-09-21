#include <math.h>

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../c_compiler/support/fp_extended.h"

#if defined(__DAVECC_LDBL_FORMAT__) && __DAVECC_LDBL_FORMAT__ >= 2
#define DAVECC_DISTINCT_LDOUBLE 1
#else
#define DAVECC_DISTINCT_LDOUBLE 0
#endif

#if DAVECC_DISTINCT_LDOUBLE
static int LdFormat(void) { return __DAVECC_LDBL_FORMAT__; }

static FPBits LdToBits(long double value) {
  FPBits bits;
  memcpy(&bits, &value, sizeof(bits));
  return bits;
}

static long double BitsToLd(FPBits bits) {
  long double value;
  memcpy(&value, &bits, sizeof(bits));
  return value;
}
#endif

int __davecc_fpclassify(double value) {
#if defined(__6502__)
  union {
    double value;
    uint32_t bits;
  } representation;
  uint32_t exponent;
  uint32_t fraction;
  representation.value = value;
  exponent = (representation.bits >> 23) & 0xff;
  fraction = representation.bits & 0x7fffff;
  if (exponent == 0xff) return fraction == 0 ? FP_INFINITE : FP_NAN;
#else
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
#endif
  if (exponent == 0) return fraction == 0 ? FP_ZERO : FP_SUBNORMAL;
  return FP_NORMAL;
}

int __davecc_signbit(double value) {
#if defined(__6502__)
  union {
    double value;
    uint32_t bits;
  } representation;
  representation.value = value;
  return (representation.bits >> 31) != 0;
#else
  union {
    double value;
    uint64_t bits;
  } representation;
  representation.value = value;
  return (representation.bits >> 63) != 0;
#endif
}

int __davecc_fpclassifyl(long double value) {
#if DAVECC_DISTINCT_LDOUBLE
  return FPClassify(LdToBits(value), LdFormat());
#else
  return __davecc_fpclassify((double)value);
#endif
}

int __davecc_signbitl(long double value) {
#if DAVECC_DISTINCT_LDOUBLE
  return FPSignBit(LdToBits(value), LdFormat());
#else
  return __davecc_signbit((double)value);
#endif
}

double sinh(double x) {
  double positive = exp(x);
  double negative = exp(-x);
  return (positive - negative) / 2.0;
}

double cosh(double x) {
  double positive = exp(x);
  double negative = exp(-x);
  return (positive + negative) / 2.0;
}

double tanh(double x) {
  double positive = exp(x);
  double negative = exp(-x);
  return (positive - negative) / (positive + negative);
}

double asinh(double x) {
  return log(x + sqrt(x * x + 1.0));
}

double acosh(double x) {
  if (x < 1.0) {
    errno = EDOM;
    return NAN;
  }
  return log(x + sqrt(x - 1.0) * sqrt(x + 1.0));
}

double atanh(double x) {
  if (x <= -1.0 || x >= 1.0) {
    errno = EDOM;
    return x == 1.0 ? HUGE_VAL : x == -1.0 ? -HUGE_VAL : NAN;
  }
  return 0.5 * log((1.0 + x) / (1.0 - x));
}

double exp2(double x) { return pow(2.0, x); }
double expm1(double x) { return exp(x) - 1.0; }
double log10(double x) { return log(x) / M_LN10; }
double log1p(double x) { return log(1.0 + x); }
double log2(double x) { return log(x) / M_LN2; }

int ilogb(double x) {
  int exponent;
  if (x == 0.0 || isnan(x) || isinf(x)) {
    errno = EDOM;
    return x == 0.0 ? INT_MIN : INT_MAX;
  }
  frexp(fabs(x), &exponent);
  return exponent - 1;
}

double logb(double x) {
  if (x == 0.0) {
    errno = ERANGE;
    return -HUGE_VAL;
  }
  if (isinf(x)) return HUGE_VAL;
  if (isnan(x)) return x;
  return (double)ilogb(x);
}

double cbrt(double x) {
  return x < 0.0 ? -pow(-x, 1.0 / 3.0) : pow(x, 1.0 / 3.0);
}

double hypot(double x, double y) {
  double larger = fabs(x);
  double smaller = fabs(y);
  double ratio;
  if (larger < smaller) {
    double temporary = larger;
    larger = smaller;
    smaller = temporary;
  }
  if (isinf(larger)) return HUGE_VAL;
  if (larger == 0.0) return 0.0;
  ratio = smaller / larger;
  return larger * sqrt(1.0 + ratio * ratio);
}

double erf(double x) {
  double sign = x < 0.0 ? -1.0 : 1.0;
  double absolute = fabs(x);
  double t = 1.0 / (1.0 + 0.3275911 * absolute);
  double polynomial =
      (((((1.061405429 * t - 1.453152027) * t) + 1.421413741) * t -
         0.284496736) * t + 0.254829592) * t;
  return sign * (1.0 - polynomial * exp(-absolute * absolute));
}

double erfc(double x) { return 1.0 - erf(x); }

static double GammaPositive(double x) {
  static const double coefficients[] = {
      676.5203681218851, -1259.1392167224028, 771.32342877765313,
      -176.61502916214059, 12.507343278686905, -0.13857109526572012,
      0.0000099843695780195716, 0.00000015056327351493116};
  double sum = 0.99999999999980993;
  double power;
  int i;
  x -= 1.0;
  for (i = 0; i < 8; i++) sum += coefficients[i] / (x + i + 1.0);
  power = x + 7.5;
  return 2.5066282746310005 * pow(power, x + 0.5) * exp(-power) * sum;
}

double tgamma(double x) {
  if (x <= 0.0 && x == floor(x)) {
    errno = EDOM;
    return NAN;
  }
  if (x < 0.5) return M_PI / (sin(M_PI * x) * GammaPositive(1.0 - x));
  return GammaPositive(x);
}

double lgamma(double x) {
  double gamma = tgamma(x);
  return log(fabs(gamma));
}

double trunc(double x) {
  if (isnan(x) || isinf(x) || x == 0.0) return x;
  return x < 0.0 ? ceil(x) : floor(x);
}

double fmod(double x, double y) {
  if (y == 0.0 || isinf(x) || isnan(x) || isnan(y)) {
    errno = EDOM;
    return NAN;
  }
  if (isinf(y)) return x;
  return x - trunc(x / y) * y;
}

double round(double x) {
  if (x < 0.0) return ceil(x - 0.5);
  return floor(x + 0.5);
}

double rint(double x) {
  double lower = floor(x);
  double fraction = x - lower;
  if (fraction < 0.5) return lower;
  if (fraction > 0.5) return lower + 1.0;
  return fmod(lower, 2.0) == 0.0 ? lower : lower + 1.0;
}

double nearbyint(double x) { return rint(x); }
long lround(double x) { return (long)round(x); }
long long llround(double x) { return (long long)round(x); }
long lrint(double x) { return (long)rint(x); }
long long llrint(double x) { return (long long)rint(x); }

double remainder(double x, double y) {
  if (y == 0.0 || isinf(x) || isnan(x) || isnan(y)) {
    errno = EDOM;
    return NAN;
  }
  return x - rint(x / y) * y;
}

double remquo(double x, double y, int* quotient) {
  double rounded;
  if (y == 0.0 || quotient == NULL) {
    errno = EDOM;
    return NAN;
  }
  rounded = rint(x / y);
  *quotient = ((int)rounded) & 0x7f;
  if (rounded < 0.0) *quotient = -*quotient;
  return x - rounded * y;
}

double copysign(double magnitude, double sign) {
#if defined(__6502__)
  union {
    double value;
    uint32_t bits;
  } left, right;
  left.value = magnitude;
  right.value = sign;
  left.bits = (left.bits & 0x7fffffffU) | (right.bits & 0x80000000U);
#else
  union {
    double value;
    uint64_t bits;
  } left, right;
  left.value = magnitude;
  right.value = sign;
  left.bits = (left.bits & 0x7fffffffffffffffULL) |
              (right.bits & 0x8000000000000000ULL);
#endif
  return left.value;
}

double nan(const char* tag) {
  (void)tag;
  return NAN;
}

double nextafter(double from, double to) {
  if (isnan(from) || isnan(to)) return from + to;
  if (from == to) return to;
#if defined(__6502__)
  {
    union {
      double value;
      uint32_t bits;
    } representation;
    representation.value = from;
    if (from == 0.0) {
      representation.bits = __davecc_signbit(to) ? 0x80000001U : 1U;
    } else if ((from < to) == (from > 0.0)) {
      representation.bits++;
    } else {
      representation.bits--;
    }
    return representation.value;
  }
#else
  {
    union {
      double value;
      uint64_t bits;
    } representation;
    representation.value = from;
    if (from == 0.0) {
      representation.bits =
          __davecc_signbit(to) ? 0x8000000000000001ULL : 1ULL;
    } else if ((from < to) == (from > 0.0)) {
      representation.bits++;
    } else {
      representation.bits--;
    }
    return representation.value;
  }
#endif
}

double nexttoward(double from, long double to) {
  return nextafter(from, (double)to);
}

double fdim(double x, double y) {
  return isnan(x) || isnan(y) ? x + y : x > y ? x - y : 0.0;
}
double fmax(double x, double y) {
  if (isnan(x)) return y;
  if (isnan(y)) return x;
  return x > y ? x : y;
}
double fmin(double x, double y) {
  if (isnan(x)) return y;
  if (isnan(y)) return x;
  return x < y ? x : y;
}
double fma(double x, double y, double z) { return x * y + z; }
double scalbn(double x, int exponent) { return ldexp(x, exponent); }
double scalbln(double x, long exponent) {
  if (exponent > INT_MAX) exponent = INT_MAX;
  if (exponent < INT_MIN) exponent = INT_MIN;
  return ldexp(x, (int)exponent);
}

#if DAVECC_DISTINCT_LDOUBLE
static FPBits LdC(uint64_t hi, uint64_t lo) {
  return FPBitsFromF128(hi, lo, LdFormat());
}

static FPBits LdExpBits(FPBits x) {
  int format = LdFormat();
  if (FPIsNaN(x, format)) {
    return x;
  }
  if (FPIsZero(x, format)) {
    return FPBitsFromF64(1.0, format);
  }
  if (FPIsInf(x, format)) {
    if (FPSignBit(x, format)) {
      return FPBitsFromF64(0.0, format);
    }
    return x;
  }
  if (FPCompare(x, FPBitsFromF64(11356.5, format), format) > 0) {
    return FPDiv(x, FPBitsFromF64(0.0, format), format);
  }
  if (FPCompare(x, FPBitsFromF64(-11433.0, format), format) < 0) {
    return FPBitsFromF64(0.0, format);
  }

  FPBits inv_ln2 = LdC(0x3fff71547652b82fULL, 0xe1777d0ffda0d23aULL);
  FPBits ln2 = LdC(0x3ffe62e42fefa39eULL, 0xf35793c7673007e6ULL);
  FPBits scaled = FPMul(x, inv_ln2, format);
  int64_t n = FPBitsToI64(scaled, format);
  FPBits n_bits = FPBitsFromI64(n, format);
  if (FPSignBit(scaled, format) && FPCompare(n_bits, scaled, format) != 0) {
    n -= 1;
    n_bits = FPBitsFromI64(n, format);
  }
  FPBits remainder = FPSub(x, FPMul(n_bits, ln2, format), format);
  FPBits term = FPBitsFromF64(1.0, format);
  FPBits sum = term;
  int i;
  for (i = 1; i <= 48; i++) {
    term = FPDiv(FPMul(term, remainder, format), FPBitsFromI64(i, format),
                 format);
    FPBits next = FPAdd(sum, term, format);
    if (FPCompare(next, sum, format) == 0) {
      break;
    }
    sum = next;
  }
  if (n > 200000) {
    n = 200000;
  }
  if (n < -200000) {
    n = -200000;
  }
  return FPLdexp(sum, (int)n, format);
}

static FPBits LdLog1pBits(FPBits f, int format) {
  FPBits two = FPBitsFromF64(2.0, format);
  FPBits z = FPDiv(f, FPAdd(two, f, format), format);
  FPBits z2 = FPMul(z, z, format);
  FPBits term = z;
  FPBits sum = z;
  int n;
  for (n = 3; n <= 81; n += 2) {
    term = FPMul(term, z2, format);
    FPBits next = FPAdd(sum, FPDiv(term, FPBitsFromI64(n, format), format),
                        format);
    if (FPCompare(next, sum, format) == 0) {
      break;
    }
    sum = next;
  }
  return FPMul(two, sum, format);
}

static FPBits LdLogBits(FPBits x) {
  int format = LdFormat();
  if (FPIsNaN(x, format)) {
    return x;
  }
  if (FPSignBit(x, format) && !FPIsZero(x, format)) {
    return FPDiv(FPSub(x, x, format), FPSub(x, x, format), format);
  }
  if (FPIsZero(x, format)) {
    return FPNeg(FPDiv(FPBitsFromF64(1.0, format), FPBitsFromF64(0.0, format),
                       format),
                 format);
  }
  if (FPIsInf(x, format)) {
    return x;
  }
  FPBits one = FPBitsFromF64(1.0, format);
  if (FPCompare(x, one, format) == 0) {
    return FPBitsFromF64(0.0, format);
  }

  // Values near 1 would cancel e*ln2 against log(mantissa).  Use log1p
  // on x-1 so LDBL_EPSILON stays visible.
  if (FPCompare(x, FPBitsFromF64(0.75, format), format) > 0 &&
      FPCompare(x, FPBitsFromF64(1.5, format), format) < 0) {
    return LdLog1pBits(FPSub(x, one, format), format);
  }

  int exponent = 0;
  FPBits mantissa = FPFrexp(x, &exponent, format);
  FPBits ln2 = LdC(0x3ffe62e42fefa39eULL, 0xf35793c7673007e6ULL);
  return FPAdd(FPMul(FPBitsFromI64(exponent, format), ln2, format),
               LdLog1pBits(FPSub(mantissa, one, format), format), format);
}

static FPBits LdSinBits(FPBits x, int quadrant) {
  int format = LdFormat();
  if (FPIsNaN(x, format)) {
    return x;
  }
  if (FPIsZero(x, format)) {
    if (quadrant & 1) {
      return FPBitsFromF64(1.0, format);
    }
    return x;
  }
  if (FPIsInf(x, format)) {
    return FPDiv(FPSub(x, x, format), FPSub(x, x, format), format);
  }
  if (FPSignBit(x, format)) {
    x = FPAbs(x, format);
    quadrant += 2;
  }
  FPBits two_over_pi = LdC(0x3ffe45f306dc9c88ULL, 0x2a53f84eafa3ea6aULL);
  FPBits half_pi = LdC(0x3fff921fb54442d1ULL, 0x8469898cc51701b8ULL);
  FPBits scaled = FPMul(x, two_over_pi, format);
  FPBits integer;
  FPBits fraction = FPModf(scaled, &integer, format);
  int64_t n = FPBitsToI64(integer, format);
  if (n < 0) {
    n = -n;
  }
  quadrant = (quadrant + (int)(n & 3)) & 3;
  if ((quadrant & 1) != 0) {
    fraction = FPSub(FPBitsFromF64(1.0, format), fraction, format);
  }
  FPBits angle = FPMul(fraction, half_pi, format);
  FPBits angle2 = FPMul(angle, angle, format);
  int use_cos = quadrant & 1;
  FPBits term;
  if (use_cos) {
    term = FPBitsFromF64(1.0, format);
  } else {
    term = angle;
  }
  FPBits sum = term;
  int k;
  int limit = use_cos ? 40 : 41;
  for (k = use_cos ? 2 : 3; k <= limit; k += 2) {
    term = FPNeg(FPDiv(FPMul(term, angle2, format),
                       FPMul(FPBitsFromI64(k - 1, format),
                             FPBitsFromI64(k, format), format),
                       format),
                 format);
    FPBits next = FPAdd(sum, term, format);
    if (FPCompare(next, sum, format) == 0) {
      break;
    }
    sum = next;
  }
  if (quadrant > 1) {
    sum = FPNeg(sum, format);
  }
  return sum;
}

static FPBits LdSqrtBits(FPBits x) {
  int format = LdFormat();
  if (FPIsNaN(x, format) || FPIsZero(x, format) ||
      (FPIsInf(x, format) && !FPSignBit(x, format))) {
    return x;
  }
  if (FPSignBit(x, format)) {
    return FPDiv(FPSub(x, x, format), FPSub(x, x, format), format);
  }
  int exponent = 0;
  FPBits mantissa = FPFrexp(x, &exponent, format);
  if ((exponent & 1) != 0) {
    mantissa = FPMul(mantissa, FPBitsFromF64(2.0, format), format);
    exponent -= 1;
  }
  FPBits y = FPBitsFromF64(sqrt(FPBitsToF64(mantissa, format)), format);
  FPBits half = FPBitsFromF64(0.5, format);
  int i;
  for (i = 0; i < 8; i++) {
    y = FPMul(FPAdd(y, FPDiv(mantissa, y, format), format), half, format);
  }
  return FPLdexp(y, exponent / 2, format);
}
#endif

#define DEFINE_UNARY_VARIANTS(name)                 \
  float name##f(float x) { return (float)name(x); } \
  long double name##l(long double x) {              \
    return (long double)name((double)x);             \
  }

#if DAVECC_DISTINCT_LDOUBLE
float sinf(float x) { return (float)sin(x); }
long double sinl(long double x) { return BitsToLd(LdSinBits(LdToBits(x), 0)); }
float cosf(float x) { return (float)cos(x); }
long double cosl(long double x) {
  return BitsToLd(LdSinBits(FPAbs(LdToBits(x), LdFormat()), 1));
}
float tanf(float x) { return (float)tan(x); }
long double tanl(long double x) { return sinl(x) / cosl(x); }
float expf(float x) { return (float)exp(x); }
long double expl(long double x) { return BitsToLd(LdExpBits(LdToBits(x))); }
float exp2f(float x) { return (float)exp2(x); }
long double exp2l(long double x) {
  return BitsToLd(LdExpBits(FPMul(
      LdToBits(x), LdC(0x3ffe62e42fefa39eULL, 0xf35793c7673007e6ULL),
      LdFormat())));
}
float expm1f(float x) { return (float)expm1(x); }
long double expm1l(long double x) { return expl(x) - 1.0L; }
float logf(float x) { return (float)log(x); }
long double logl(long double x) { return BitsToLd(LdLogBits(LdToBits(x))); }
float log10f(float x) { return (float)log10(x); }
long double log10l(long double x) {
  return BitsToLd(FPMul(LdLogBits(LdToBits(x)),
                        LdC(0x3ffdbcb7b1526e50ULL, 0xe32a6ab7555f5a68ULL),
                        LdFormat()));
}
float log1pf(float x) { return (float)log1p(x); }
long double log1pl(long double x) { return logl(1.0L + x); }
float log2f(float x) { return (float)log2(x); }
long double log2l(long double x) {
  return BitsToLd(FPMul(LdLogBits(LdToBits(x)),
                        LdC(0x3fff71547652b82fULL, 0xe1777d0ffda0d23aULL),
                        LdFormat()));
}
float sqrtf(float x) { return (float)sqrt(x); }
long double sqrtl(long double x) { return BitsToLd(LdSqrtBits(LdToBits(x))); }
#else
DEFINE_UNARY_VARIANTS(sin)
DEFINE_UNARY_VARIANTS(cos)
DEFINE_UNARY_VARIANTS(tan)
DEFINE_UNARY_VARIANTS(exp)
DEFINE_UNARY_VARIANTS(exp2)
DEFINE_UNARY_VARIANTS(expm1)
DEFINE_UNARY_VARIANTS(log)
DEFINE_UNARY_VARIANTS(log10)
DEFINE_UNARY_VARIANTS(log1p)
DEFINE_UNARY_VARIANTS(log2)
DEFINE_UNARY_VARIANTS(sqrt)
#endif

DEFINE_UNARY_VARIANTS(asin)
DEFINE_UNARY_VARIANTS(acos)
DEFINE_UNARY_VARIANTS(atan)
DEFINE_UNARY_VARIANTS(sinh)
DEFINE_UNARY_VARIANTS(cosh)
DEFINE_UNARY_VARIANTS(tanh)
DEFINE_UNARY_VARIANTS(asinh)
DEFINE_UNARY_VARIANTS(acosh)
DEFINE_UNARY_VARIANTS(atanh)
DEFINE_UNARY_VARIANTS(logb)
DEFINE_UNARY_VARIANTS(cbrt)
DEFINE_UNARY_VARIANTS(erf)
DEFINE_UNARY_VARIANTS(erfc)
DEFINE_UNARY_VARIANTS(tgamma)
DEFINE_UNARY_VARIANTS(lgamma)
DEFINE_UNARY_VARIANTS(round)
DEFINE_UNARY_VARIANTS(rint)
DEFINE_UNARY_VARIANTS(nearbyint)

#if DAVECC_DISTINCT_LDOUBLE
float ceilf(float x) { return (float)ceil(x); }
long double ceill(long double x) { return BitsToLd(FPCeil(LdToBits(x), LdFormat())); }
float floorf(float x) { return (float)floor(x); }
long double floorl(long double x) { return BitsToLd(FPFloor(LdToBits(x), LdFormat())); }
float truncf(float x) { return (float)trunc(x); }
long double truncl(long double x) { return BitsToLd(FPTrunc(LdToBits(x), LdFormat())); }
float fabsf(float x) { return (float)fabs(x); }
long double fabsl(long double x) { return BitsToLd(FPAbs(LdToBits(x), LdFormat())); }
#else
DEFINE_UNARY_VARIANTS(ceil)
DEFINE_UNARY_VARIANTS(floor)
DEFINE_UNARY_VARIANTS(trunc)
DEFINE_UNARY_VARIANTS(fabs)
#endif

#undef DEFINE_UNARY_VARIANTS

#define DEFINE_BINARY_VARIANTS(name)                         \
  float name##f(float x, float y) { return (float)name(x, y); } \
  long double name##l(long double x, long double y) {         \
    return (long double)name((double)x, (double)y);            \
  }

DEFINE_BINARY_VARIANTS(atan2)
DEFINE_BINARY_VARIANTS(hypot)
DEFINE_BINARY_VARIANTS(fmod)
DEFINE_BINARY_VARIANTS(remainder)
DEFINE_BINARY_VARIANTS(fdim)
DEFINE_BINARY_VARIANTS(fmax)
DEFINE_BINARY_VARIANTS(fmin)
#if DAVECC_DISTINCT_LDOUBLE
float powf(float x, float y) { return (float)pow(x, y); }
long double powl(long double x, long double y) {
  if (x == 1.0L || y == 0.0L) {
    return 1.0L;
  }
  if (x == 0.0L) {
    return y > 0.0L ? 0.0L : expl(y * logl(0.0L));
  }
  if (x < 0.0L) {
    long double integer;
    long double fraction = modfl(y, &integer);
    if (fraction != 0.0L) {
      return (long double)(0.0L / 0.0L);
    }
    long double magnitude = expl(y * logl(-x));
    return ((int)integer & 1) ? -magnitude : magnitude;
  }
  return expl(y * logl(x));
}
#else
DEFINE_BINARY_VARIANTS(pow)
#endif

#undef DEFINE_BINARY_VARIANTS

float copysignf(float magnitude, float sign) {
  return (float)copysign(magnitude, sign);
}
long double copysignl(long double magnitude, long double sign) {
#if DAVECC_DISTINCT_LDOUBLE
  return BitsToLd(FPCopySign(LdToBits(magnitude), LdToBits(sign), LdFormat()));
#else
  return (long double)copysign((double)magnitude, (double)sign);
#endif
}
float nanf(const char* tag) { return (float)nan(tag); }
long double nanl(const char* tag) { return (long double)nan(tag); }

float nextafterf(float from, float to) {
  union {
    float value;
    uint32_t bits;
  } representation;
  if (isnan(from) || isnan(to)) return from + to;
  if (from == to) return to;
  representation.value = from;
  if (from == 0.0f) {
    representation.bits = __davecc_signbit(to) ? 0x80000001U : 1U;
  } else if ((from < to) == (from > 0.0f)) {
    representation.bits++;
  } else {
    representation.bits--;
  }
  return representation.value;
}
long double nextafterl(long double from, long double to) {
#if DAVECC_DISTINCT_LDOUBLE
  return BitsToLd(FPNextAfter(LdToBits(from), LdToBits(to), LdFormat()));
#else
  return (long double)nextafter((double)from, (double)to);
#endif
}
float nexttowardf(float from, long double to) {
  return nextafterf(from, (float)to);
}
long double nexttowardl(long double from, long double to) {
  return nextafterl(from, to);
}

float fmaf(float x, float y, float z) { return x * y + z; }
long double fmal(long double x, long double y, long double z) {
  return x * y + z;
}

int ilogbf(float x) { return ilogb(x); }
int ilogbl(long double x) {
#if DAVECC_DISTINCT_LDOUBLE
  return FPIlogb(LdToBits(x), LdFormat());
#else
  return ilogb((double)x);
#endif
}
long lroundf(float x) { return lround(x); }
long lroundl(long double x) { return lround((double)x); }
long long llroundf(float x) { return llround(x); }
long long llroundl(long double x) { return llround((double)x); }
long lrintf(float x) { return lrint(x); }
long lrintl(long double x) { return lrint((double)x); }
long long llrintf(float x) { return llrint(x); }
long long llrintl(long double x) { return llrint((double)x); }

float remquof(float x, float y, int* quotient) {
  return (float)remquo(x, y, quotient);
}
long double remquol(long double x, long double y, int* quotient) {
  return (long double)remquo((double)x, (double)y, quotient);
}

float frexpf(float x, int* exponent) {
  return (float)frexp(x, exponent);
}
long double frexpl(long double x, int* exponent) {
#if DAVECC_DISTINCT_LDOUBLE
  return BitsToLd(FPFrexp(LdToBits(x), exponent, LdFormat()));
#else
  return (long double)frexp((double)x, exponent);
#endif
}
float ldexpf(float x, int exponent) { return (float)ldexp(x, exponent); }
long double ldexpl(long double x, int exponent) {
#if DAVECC_DISTINCT_LDOUBLE
  return BitsToLd(FPLdexp(LdToBits(x), exponent, LdFormat()));
#else
  return (long double)ldexp((double)x, exponent);
#endif
}
float modff(float x, float* integer) {
  double integral;
  double result = modf(x, &integral);
  *integer = (float)integral;
  return (float)result;
}
long double modfl(long double x, long double* integer) {
#if DAVECC_DISTINCT_LDOUBLE
  FPBits whole;
  FPBits frac = FPModf(LdToBits(x), &whole, LdFormat());
  *integer = BitsToLd(whole);
  return BitsToLd(frac);
#else
  double integral;
  double result = modf((double)x, &integral);
  *integer = (long double)integral;
  return (long double)result;
#endif
}
float scalbnf(float x, int exponent) { return (float)scalbn(x, exponent); }
long double scalbnl(long double x, int exponent) {
#if DAVECC_DISTINCT_LDOUBLE
  return ldexpl(x, exponent);
#else
  return (long double)scalbn((double)x, exponent);
#endif
}
float scalblnf(float x, long exponent) { return (float)scalbln(x, exponent); }
long double scalblnl(long double x, long exponent) {
#if DAVECC_DISTINCT_LDOUBLE
  if (exponent > INT_MAX) exponent = INT_MAX;
  if (exponent < INT_MIN) exponent = INT_MIN;
  return ldexpl(x, (int)exponent);
#else
  return (long double)scalbln((double)x, exponent);
#endif
}
