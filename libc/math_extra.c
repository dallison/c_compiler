#include <math.h>

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

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

#define DEFINE_UNARY_VARIANTS(name)                 \
  float name##f(float x) { return (float)name(x); } \
  long double name##l(long double x) {              \
    return (long double)name((double)x);             \
  }

DEFINE_UNARY_VARIANTS(sin)
DEFINE_UNARY_VARIANTS(cos)
DEFINE_UNARY_VARIANTS(tan)
DEFINE_UNARY_VARIANTS(asin)
DEFINE_UNARY_VARIANTS(acos)
DEFINE_UNARY_VARIANTS(atan)
DEFINE_UNARY_VARIANTS(sinh)
DEFINE_UNARY_VARIANTS(cosh)
DEFINE_UNARY_VARIANTS(tanh)
DEFINE_UNARY_VARIANTS(asinh)
DEFINE_UNARY_VARIANTS(acosh)
DEFINE_UNARY_VARIANTS(atanh)
DEFINE_UNARY_VARIANTS(exp)
DEFINE_UNARY_VARIANTS(exp2)
DEFINE_UNARY_VARIANTS(expm1)
DEFINE_UNARY_VARIANTS(log)
DEFINE_UNARY_VARIANTS(log10)
DEFINE_UNARY_VARIANTS(log1p)
DEFINE_UNARY_VARIANTS(log2)
DEFINE_UNARY_VARIANTS(logb)
DEFINE_UNARY_VARIANTS(sqrt)
DEFINE_UNARY_VARIANTS(cbrt)
DEFINE_UNARY_VARIANTS(erf)
DEFINE_UNARY_VARIANTS(erfc)
DEFINE_UNARY_VARIANTS(tgamma)
DEFINE_UNARY_VARIANTS(lgamma)
DEFINE_UNARY_VARIANTS(ceil)
DEFINE_UNARY_VARIANTS(floor)
DEFINE_UNARY_VARIANTS(trunc)
DEFINE_UNARY_VARIANTS(round)
DEFINE_UNARY_VARIANTS(rint)
DEFINE_UNARY_VARIANTS(nearbyint)
DEFINE_UNARY_VARIANTS(fabs)

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
DEFINE_BINARY_VARIANTS(pow)

#undef DEFINE_BINARY_VARIANTS

float copysignf(float magnitude, float sign) {
  return (float)copysign(magnitude, sign);
}
long double copysignl(long double magnitude, long double sign) {
  return (long double)copysign((double)magnitude, (double)sign);
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
  return (long double)nextafter((double)from, (double)to);
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
int ilogbl(long double x) { return ilogb((double)x); }
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
  return (long double)frexp((double)x, exponent);
}
float ldexpf(float x, int exponent) { return (float)ldexp(x, exponent); }
long double ldexpl(long double x, int exponent) {
  return (long double)ldexp((double)x, exponent);
}
float modff(float x, float* integer) {
  double integral;
  double result = modf(x, &integral);
  *integer = (float)integral;
  return (float)result;
}
long double modfl(long double x, long double* integer) {
  double integral;
  double result = modf((double)x, &integral);
  *integer = (long double)integral;
  return (long double)result;
}
float scalbnf(float x, int exponent) { return (float)scalbn(x, exponent); }
long double scalbnl(long double x, int exponent) {
  return (long double)scalbn((double)x, exponent);
}
float scalblnf(float x, long exponent) { return (float)scalbln(x, exponent); }
long double scalblnl(long double x, long exponent) {
  return (long double)scalbln((double)x, exponent);
}
