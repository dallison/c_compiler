//
//  math.h
//  c_compiler
//
//  Created by David Allison on 12/24/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#ifndef math_h
#define math_h

#ifdef __cplusplus
extern "C" {
#endif

typedef float float_t;
typedef double double_t;

#define MATH_ERRNO 1
#define MATH_ERREXCEPT 2
#define math_errhandling MATH_ERRNO

#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4

#define HUGE_VAL (1.0 / 0.0)
#define HUGE_VALF (1.0F / 0.0F)
#define HUGE_VALL (1.0L / 0.0L)
#define INFINITY HUGE_VALF
#define NAN (0.0F / 0.0F)

int __davecc_fpclassify(double value);
int __davecc_signbit(double value);

#define fpclassify(value) __davecc_fpclassify((double)(value))
#define isfinite(value) (fpclassify(value) > FP_INFINITE)
#define isinf(value) (fpclassify(value) == FP_INFINITE)
#define isnan(value) (fpclassify(value) == FP_NAN)
#define isnormal(value) (fpclassify(value) == FP_NORMAL)
#define signbit(value) __davecc_signbit((double)(value))

// Generally useful contants.
#define M_E        2.7182818284590452354   // e
#define M_LOG2E    1.4426950408889634074   // log_2 e
#define M_LOG10E   0.43429448190325182765  // log_10 e
#define M_LN2      0.69314718055994530942  // log_e 2
#define M_LN10     2.30258509299404568402  // log_e 10
#define M_PI       3.14159265358979323846  // pi
#define M_PI_2     1.57079632679489661923  // pi/2
#define M_PI_4     0.78539816339744830962  // pi/4
#define M_1_PI     0.31830988618379067154  // 1/pi
#define M_2_PI     0.63661977236758134308  // 2/pi
#define M_2_SQRTPI 1.12837916709551257390  // 2/sqrt(pi)
#define M_SQRT2    1.41421356237309504880  // sqrt(2)
#define M_SQRT1_2  0.70710678118654752440  // 1/sqrt(2)


double sin(double x);
double cos(double x);
double tan(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double atan2(double y, double x);

double sqrt(double x);
double fabs(double x);

double floor(double x);
double ceil(double x);
double frexp(double x, int* exp);
double exp(double x);
double log(double x);
double pow(double x, double y);

double modf(double x, double* p);
double ldexp(double mantissa, int exp);

double sinh(double x);
double cosh(double x);
double tanh(double x);
double asinh(double x);
double acosh(double x);
double atanh(double x);
double exp2(double x);
double expm1(double x);
double log10(double x);
double log1p(double x);
double log2(double x);
double logb(double x);
int ilogb(double x);
double cbrt(double x);
double hypot(double x, double y);
double erf(double x);
double erfc(double x);
double tgamma(double x);
double lgamma(double x);
double fmod(double x, double y);
double remainder(double x, double y);
double remquo(double x, double y, int* quotient);
double copysign(double magnitude, double sign);
double nan(const char* tag);
double nextafter(double from, double to);
double nexttoward(double from, long double to);
double fdim(double x, double y);
double fmax(double x, double y);
double fmin(double x, double y);
double fma(double x, double y, double z);
double trunc(double x);
double round(double x);
long lround(double x);
long long llround(double x);
double rint(double x);
long lrint(double x);
long long llrint(double x);
double nearbyint(double x);
double scalbn(double x, int exponent);
double scalbln(double x, long exponent);

#define __DAVECC_DECLARE_MATH_VARIANTS(name) \
  float name##f(float x);                    \
  long double name##l(long double x);

__DAVECC_DECLARE_MATH_VARIANTS(sin)
__DAVECC_DECLARE_MATH_VARIANTS(cos)
__DAVECC_DECLARE_MATH_VARIANTS(tan)
__DAVECC_DECLARE_MATH_VARIANTS(asin)
__DAVECC_DECLARE_MATH_VARIANTS(acos)
__DAVECC_DECLARE_MATH_VARIANTS(atan)
float atan2f(float y, float x);
long double atan2l(long double y, long double x);
__DAVECC_DECLARE_MATH_VARIANTS(sinh)
__DAVECC_DECLARE_MATH_VARIANTS(cosh)
__DAVECC_DECLARE_MATH_VARIANTS(tanh)
__DAVECC_DECLARE_MATH_VARIANTS(asinh)
__DAVECC_DECLARE_MATH_VARIANTS(acosh)
__DAVECC_DECLARE_MATH_VARIANTS(atanh)
__DAVECC_DECLARE_MATH_VARIANTS(exp)
__DAVECC_DECLARE_MATH_VARIANTS(exp2)
__DAVECC_DECLARE_MATH_VARIANTS(expm1)
__DAVECC_DECLARE_MATH_VARIANTS(log)
__DAVECC_DECLARE_MATH_VARIANTS(log10)
__DAVECC_DECLARE_MATH_VARIANTS(log1p)
__DAVECC_DECLARE_MATH_VARIANTS(log2)
__DAVECC_DECLARE_MATH_VARIANTS(logb)
int ilogbf(float x);
int ilogbl(long double x);
__DAVECC_DECLARE_MATH_VARIANTS(sqrt)
__DAVECC_DECLARE_MATH_VARIANTS(cbrt)
float hypotf(float x, float y);
long double hypotl(long double x, long double y);
__DAVECC_DECLARE_MATH_VARIANTS(erf)
__DAVECC_DECLARE_MATH_VARIANTS(erfc)
__DAVECC_DECLARE_MATH_VARIANTS(tgamma)
__DAVECC_DECLARE_MATH_VARIANTS(lgamma)
__DAVECC_DECLARE_MATH_VARIANTS(ceil)
__DAVECC_DECLARE_MATH_VARIANTS(floor)
__DAVECC_DECLARE_MATH_VARIANTS(trunc)
__DAVECC_DECLARE_MATH_VARIANTS(round)
__DAVECC_DECLARE_MATH_VARIANTS(rint)
__DAVECC_DECLARE_MATH_VARIANTS(nearbyint)
long lroundf(float x);
long lroundl(long double x);
long long llroundf(float x);
long long llroundl(long double x);
long lrintf(float x);
long lrintl(long double x);
long long llrintf(float x);
long long llrintl(long double x);
__DAVECC_DECLARE_MATH_VARIANTS(fabs)
float fmodf(float x, float y);
long double fmodl(long double x, long double y);
float remainderf(float x, float y);
long double remainderl(long double x, long double y);
float remquof(float x, float y, int* quotient);
long double remquol(long double x, long double y, int* quotient);
float copysignf(float magnitude, float sign);
long double copysignl(long double magnitude, long double sign);
float nanf(const char* tag);
long double nanl(const char* tag);
float nextafterf(float from, float to);
long double nextafterl(long double from, long double to);
float nexttowardf(float from, long double to);
long double nexttowardl(long double from, long double to);
float fdimf(float x, float y);
long double fdiml(long double x, long double y);
float fmaxf(float x, float y);
long double fmaxl(long double x, long double y);
float fminf(float x, float y);
long double fminl(long double x, long double y);
float fmaf(float x, float y, float z);
long double fmal(long double x, long double y, long double z);
float frexpf(float x, int* exponent);
long double frexpl(long double x, int* exponent);
float ldexpf(float x, int exponent);
long double ldexpl(long double x, int exponent);
float modff(float x, float* integer);
long double modfl(long double x, long double* integer);
float scalbnf(float x, int exponent);
long double scalbnl(long double x, int exponent);
float scalblnf(float x, long exponent);
long double scalblnl(long double x, long exponent);
float powf(float x, float y);
long double powl(long double x, long double y);

#undef __DAVECC_DECLARE_MATH_VARIANTS

#ifdef __cplusplus
}
#endif

#endif /* math_h */
