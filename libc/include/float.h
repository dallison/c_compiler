//
//  float.h
//  c_compiler
//
//  Characteristics of the floating types for DaveCC targets.
//

#ifndef float_h
#define float_h

#define FLT_RADIX 2
#define FLT_ROUNDS 1
#define FLT_EVAL_METHOD 0
#define FLT_HAS_SUBNORM 1
#define DBL_HAS_SUBNORM 1
#define LDBL_HAS_SUBNORM 1

#if defined(__6502__)
// 65C02 uses 32-bit IEEE-754 single precision for float, double, and
// long double.
#  define FLT_MANT_DIG   24
#  define FLT_DIG        6
#  define FLT_MIN_EXP    (-125)
#  define FLT_MIN_10_EXP (-37)
#  define FLT_MAX_EXP    128
#  define FLT_MAX_10_EXP 38
#  define FLT_MIN        1.17549435e-38F
#  define FLT_TRUE_MIN   1.40129846e-45F
#  define FLT_MAX        3.40282347e+38F
#  define FLT_EPSILON    1.19209290e-07F

#  define DBL_MANT_DIG   FLT_MANT_DIG
#  define DBL_DIG        FLT_DIG
#  define DBL_MIN_EXP    FLT_MIN_EXP
#  define DBL_MIN_10_EXP FLT_MIN_10_EXP
#  define DBL_MAX_EXP    FLT_MAX_EXP
#  define DBL_MAX_10_EXP FLT_MAX_10_EXP
#  define DBL_MIN        ((double)FLT_MIN)
#  define DBL_TRUE_MIN   ((double)FLT_TRUE_MIN)
#  define DBL_MAX        ((double)FLT_MAX)
#  define DBL_EPSILON    ((double)FLT_EPSILON)

#else
#  define FLT_MANT_DIG   24
#  define FLT_DIG        6
#  define FLT_MIN_EXP    (-125)
#  define FLT_MIN_10_EXP (-37)
#  define FLT_MAX_EXP    128
#  define FLT_MAX_10_EXP 38
#  define FLT_MIN        1.17549435e-38F
#  define FLT_TRUE_MIN   1.40129846e-45F
#  define FLT_MAX        3.40282347e+38F
#  define FLT_EPSILON    1.19209290e-07F

#  define DBL_MANT_DIG   53
#  define DBL_DIG        15
#  define DBL_MIN_EXP    (-1021)
#  define DBL_MIN_10_EXP (-307)
#  define DBL_MAX_EXP    1024
#  define DBL_MAX_10_EXP 308
#  define DBL_MIN        2.2250738585072014e-308
#  define DBL_TRUE_MIN   4.9406564584124654e-324
#  define DBL_MAX        1.7976931348623157e+308
#  define DBL_EPSILON    2.2204460492503131e-16
#endif

#if defined(__LDBL_MANT_DIG__)
#  define LDBL_MANT_DIG   __LDBL_MANT_DIG__
#  define LDBL_DIG        __LDBL_DIG__
#  define LDBL_MIN_EXP    __LDBL_MIN_EXP__
#  define LDBL_MIN_10_EXP __LDBL_MIN_10_EXP__
#  define LDBL_MAX_EXP    __LDBL_MAX_EXP__
#  define LDBL_MAX_10_EXP __LDBL_MAX_10_EXP__
#  define LDBL_DECIMAL_DIG __LDBL_DECIMAL_DIG__
#else
#  define LDBL_MANT_DIG   DBL_MANT_DIG
#  define LDBL_DIG        DBL_DIG
#  define LDBL_MIN_EXP    DBL_MIN_EXP
#  define LDBL_MIN_10_EXP DBL_MIN_10_EXP
#  define LDBL_MAX_EXP    DBL_MAX_EXP
#  define LDBL_MAX_10_EXP DBL_MAX_10_EXP
#  define LDBL_DECIMAL_DIG DBL_DECIMAL_DIG
#endif

#if defined(__DAVECC_LDBL_FORMAT__) && __DAVECC_LDBL_FORMAT__ == 3
#  define LDBL_MIN        3.36210314311209350626267781732175260e-4932L
#  define LDBL_TRUE_MIN   6.47517511943802511092443895822764655e-4966L
#  define LDBL_MAX        1.18973149535723176508575932662800702e+4932L
#  define LDBL_EPSILON    1.92592994438723585305597794258492732e-34L
#  define DECIMAL_DIG     36
#  define FLT_DECIMAL_DIG 9
#  define DBL_DECIMAL_DIG 17
#elif defined(__DAVECC_LDBL_FORMAT__) && __DAVECC_LDBL_FORMAT__ == 2
#  define LDBL_MIN        3.36210314311209350626e-4932L
#  define LDBL_TRUE_MIN   3.64519953188247460253e-4951L
#  define LDBL_MAX        1.18973149535723176502e+4932L
#  define LDBL_EPSILON    1.08420217248550443401e-19L
#  define DECIMAL_DIG     21
#  define FLT_DECIMAL_DIG 9
#  define DBL_DECIMAL_DIG 17
#elif defined(__6502__)
#  define LDBL_MIN        ((long double)FLT_MIN)
#  define LDBL_TRUE_MIN   ((long double)FLT_TRUE_MIN)
#  define LDBL_MAX        ((long double)FLT_MAX)
#  define LDBL_EPSILON    ((long double)FLT_EPSILON)
#  define DECIMAL_DIG     9
#  define FLT_DECIMAL_DIG 9
#  define DBL_DECIMAL_DIG 9
#else
#  define LDBL_MIN        ((long double)DBL_MIN)
#  define LDBL_TRUE_MIN   ((long double)DBL_TRUE_MIN)
#  define LDBL_MAX        ((long double)DBL_MAX)
#  define LDBL_EPSILON    ((long double)DBL_EPSILON)
#  define DECIMAL_DIG     17
#  define FLT_DECIMAL_DIG 9
#  define DBL_DECIMAL_DIG 17
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define FLT_NORM_MAX FLT_MAX
#define DBL_NORM_MAX DBL_MAX
#define LDBL_NORM_MAX LDBL_MAX
#define FLT_IS_IEC_60559 1
#define DBL_IS_IEC_60559 1
#define LDBL_IS_IEC_60559 1
#endif

#endif /* float_h */
