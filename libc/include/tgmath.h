#ifndef _DAVECC_TGMATH_H
#define _DAVECC_TGMATH_H

#include <math.h>
#include <complex.h>

/*
 * Convert each generic argument's type to the type domain prescribed by
 * C99 7.22: integer and other non-floating arguments count as double.
 * These expressions only occur as controlling expressions of _Generic and
 * therefore do not evaluate the user's arguments.
 */
#define __DAVECC_TG_PROMOTE(value)                                      \
  _Generic((value),                                                     \
      float: (float)0,                                                  \
      long double: (long double)0,                                      \
      float complex: (float complex){0},                                \
      double complex: (double complex){0},                              \
      long double complex: (long double complex){0},                    \
      default: (double)0)

#define __DAVECC_TG_TYPE1(a) (__DAVECC_TG_PROMOTE(a))
#define __DAVECC_TG_TYPE2(a, b)                                        \
  (__DAVECC_TG_PROMOTE(a) + __DAVECC_TG_PROMOTE(b))
#define __DAVECC_TG_TYPE3(a, b, c)                                     \
  (__DAVECC_TG_PROMOTE(a) + __DAVECC_TG_PROMOTE(b) +                   \
   __DAVECC_TG_PROMOTE(c))

#define __DAVECC_TG_REAL_SELECT(type, f, d, l)                          \
  _Generic((type),                                                      \
      float: f,                                                         \
      long double: l,                                                   \
      float complex: f,                                                 \
      long double complex: l,                                           \
      default: d)

#define __DAVECC_TG_REAL_COMPLEX_SELECT(type, rf, rd, rl, cf, cd, cl)   \
  _Generic((type),                                                      \
      float: rf,                                                        \
      long double: rl,                                                  \
      float complex: cf,                                                \
      double complex: cd,                                               \
      long double complex: cl,                                          \
      default: rd)

#define __DAVECC_TG_COMPLEX_SELECT(type, f, d, l)                       \
  _Generic((type),                                                      \
      float: f,                                                         \
      long double: l,                                                   \
      float complex: f,                                                 \
      long double complex: l,                                           \
      default: d)

#define __DAVECC_TG_REAL1(name, value)                                  \
  __DAVECC_TG_REAL_SELECT(__DAVECC_TG_TYPE1(value),                     \
                          name##f, name, name##l)(value)
#define __DAVECC_TG_REAL2(name, left, right)                            \
  __DAVECC_TG_REAL_SELECT(__DAVECC_TG_TYPE2(left, right),               \
                          name##f, name, name##l)(left, right)
#define __DAVECC_TG_REAL3(name, a, b, c)                                \
  __DAVECC_TG_REAL_SELECT(__DAVECC_TG_TYPE3(a, b, c),                   \
                          name##f, name, name##l)(a, b, c)
#define __DAVECC_TG_RC1(name, value)                                    \
  __DAVECC_TG_REAL_COMPLEX_SELECT(                                      \
      __DAVECC_TG_TYPE1(value), name##f, name, name##l,                 \
      c##name##f, c##name, c##name##l)(value)
#define __DAVECC_TG_RC2(name, left, right)                              \
  __DAVECC_TG_REAL_COMPLEX_SELECT(                                      \
      __DAVECC_TG_TYPE2(left, right), name##f, name, name##l,           \
      c##name##f, c##name, c##name##l)(left, right)
#define __DAVECC_TG_COMPLEX1(name, value)                               \
  __DAVECC_TG_COMPLEX_SELECT(__DAVECC_TG_TYPE1(value),                  \
                             name##f, name, name##l)(value)

#undef acos
#undef asin
#undef atan
#undef acosh
#undef asinh
#undef atanh
#undef cos
#undef sin
#undef tan
#undef cosh
#undef sinh
#undef tanh
#undef exp
#undef log
#undef pow
#undef sqrt
#undef fabs

#define acos(value) __DAVECC_TG_RC1(acos, value)
#define asin(value) __DAVECC_TG_RC1(asin, value)
#define atan(value) __DAVECC_TG_RC1(atan, value)
#define acosh(value) __DAVECC_TG_RC1(acosh, value)
#define asinh(value) __DAVECC_TG_RC1(asinh, value)
#define atanh(value) __DAVECC_TG_RC1(atanh, value)
#define cos(value) __DAVECC_TG_RC1(cos, value)
#define sin(value) __DAVECC_TG_RC1(sin, value)
#define tan(value) __DAVECC_TG_RC1(tan, value)
#define cosh(value) __DAVECC_TG_RC1(cosh, value)
#define sinh(value) __DAVECC_TG_RC1(sinh, value)
#define tanh(value) __DAVECC_TG_RC1(tanh, value)
#define exp(value) __DAVECC_TG_RC1(exp, value)
#define log(value) __DAVECC_TG_RC1(log, value)
#define pow(left, right) __DAVECC_TG_RC2(pow, left, right)
#define sqrt(value) __DAVECC_TG_RC1(sqrt, value)
#define fabs(value)                                                      \
  __DAVECC_TG_REAL_COMPLEX_SELECT(                                      \
      __DAVECC_TG_TYPE1(value), fabsf, fabs, fabsl,                     \
      cabsf, cabs, cabsl)(value)

#undef atan2
#undef cbrt
#undef ceil
#undef copysign
#undef erf
#undef erfc
#undef exp2
#undef expm1
#undef fdim
#undef floor
#undef fma
#undef fmax
#undef fmin
#undef fmod
#undef frexp
#undef hypot
#undef ilogb
#undef ldexp
#undef lgamma
#undef llrint
#undef llround
#undef log10
#undef log1p
#undef log2
#undef logb
#undef lrint
#undef lround
#undef modf
#undef nearbyint
#undef nextafter
#undef nexttoward
#undef remainder
#undef remquo
#undef rint
#undef round
#undef scalbln
#undef scalbn
#undef tgamma
#undef trunc

#define atan2(y, x) __DAVECC_TG_REAL2(atan2, y, x)
#define cbrt(value) __DAVECC_TG_REAL1(cbrt, value)
#define ceil(value) __DAVECC_TG_REAL1(ceil, value)
#define copysign(magnitude, sign)                                       \
  __DAVECC_TG_REAL2(copysign, magnitude, sign)
#define erf(value) __DAVECC_TG_REAL1(erf, value)
#define erfc(value) __DAVECC_TG_REAL1(erfc, value)
#define exp2(value) __DAVECC_TG_REAL1(exp2, value)
#define expm1(value) __DAVECC_TG_REAL1(expm1, value)
#define fdim(left, right) __DAVECC_TG_REAL2(fdim, left, right)
#define floor(value) __DAVECC_TG_REAL1(floor, value)
#define fma(a, b, c) __DAVECC_TG_REAL3(fma, a, b, c)
#define fmax(left, right) __DAVECC_TG_REAL2(fmax, left, right)
#define fmin(left, right) __DAVECC_TG_REAL2(fmin, left, right)
#define fmod(left, right) __DAVECC_TG_REAL2(fmod, left, right)
#define frexp(value, exponent)                                         \
  __DAVECC_TG_REAL_SELECT(__DAVECC_TG_TYPE1(value),                    \
                          frexpf, frexp, frexpl)(value, exponent)
#define hypot(left, right) __DAVECC_TG_REAL2(hypot, left, right)
#define ilogb(value) __DAVECC_TG_REAL1(ilogb, value)
#define ldexp(value, exponent)                                         \
  __DAVECC_TG_REAL_SELECT(__DAVECC_TG_TYPE1(value),                    \
                          ldexpf, ldexp, ldexpl)(value, exponent)
#define lgamma(value) __DAVECC_TG_REAL1(lgamma, value)
#define llrint(value) __DAVECC_TG_REAL1(llrint, value)
#define llround(value) __DAVECC_TG_REAL1(llround, value)
#define log10(value) __DAVECC_TG_REAL1(log10, value)
#define log1p(value) __DAVECC_TG_REAL1(log1p, value)
#define log2(value) __DAVECC_TG_REAL1(log2, value)
#define logb(value) __DAVECC_TG_REAL1(logb, value)
#define lrint(value) __DAVECC_TG_REAL1(lrint, value)
#define lround(value) __DAVECC_TG_REAL1(lround, value)
#define modf(value, integer)                                           \
  __DAVECC_TG_REAL_SELECT(__DAVECC_TG_TYPE1(value),                    \
                          modff, modf, modfl)(value, integer)
#define nearbyint(value) __DAVECC_TG_REAL1(nearbyint, value)
#define nextafter(from, to) __DAVECC_TG_REAL2(nextafter, from, to)
#define nexttoward(from, to)                                           \
  __DAVECC_TG_REAL_SELECT(__DAVECC_TG_TYPE1(from),                     \
                          nexttowardf, nexttoward, nexttowardl)(from, to)
#define remainder(left, right) __DAVECC_TG_REAL2(remainder, left, right)
#define remquo(left, right, quotient)                                  \
  __DAVECC_TG_REAL_SELECT(__DAVECC_TG_TYPE2(left, right),              \
                          remquof, remquo, remquol)(left, right, quotient)
#define rint(value) __DAVECC_TG_REAL1(rint, value)
#define round(value) __DAVECC_TG_REAL1(round, value)
#define scalbln(value, exponent)                                       \
  __DAVECC_TG_REAL_SELECT(__DAVECC_TG_TYPE1(value),                    \
                          scalblnf, scalbln, scalblnl)(value, exponent)
#define scalbn(value, exponent)                                        \
  __DAVECC_TG_REAL_SELECT(__DAVECC_TG_TYPE1(value),                    \
                          scalbnf, scalbn, scalbnl)(value, exponent)
#define tgamma(value) __DAVECC_TG_REAL1(tgamma, value)
#define trunc(value) __DAVECC_TG_REAL1(trunc, value)

#undef carg
#undef cimag
#undef conj
#undef cproj
#undef creal

#define carg(value) __DAVECC_TG_COMPLEX1(carg, value)
#define conj(value) __DAVECC_TG_COMPLEX1(conj, value)
#define cproj(value) __DAVECC_TG_COMPLEX1(cproj, value)
#define cimag(z)                                                        \
  _Generic((z),                                                        \
      float: cimagf,                                                   \
      long double: cimagl,                                             \
      float complex: cimagf,                                           \
      long double complex: cimagl,                                     \
      default: cimag)(z)
#define creal(z)                                                        \
  _Generic((z),                                                        \
      float: crealf,                                                   \
      long double: creall,                                             \
      float complex: crealf,                                           \
      long double complex: creall,                                     \
      default: creal)(z)

#endif /* _DAVECC_TGMATH_H */
