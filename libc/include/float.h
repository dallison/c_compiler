//
//  float.h
//  c_compiler
//
//  Characteristics of the floating types for DaveCC targets.
//

#ifndef float_h
#define float_h

#define FLT_RADIX 2

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

#  define LDBL_MANT_DIG   FLT_MANT_DIG
#  define LDBL_DIG        FLT_DIG
#  define LDBL_MIN_EXP    FLT_MIN_EXP
#  define LDBL_MIN_10_EXP FLT_MIN_10_EXP
#  define LDBL_MAX_EXP    FLT_MAX_EXP
#  define LDBL_MAX_10_EXP FLT_MAX_10_EXP
#  define LDBL_MIN        ((long double)FLT_MIN)
#  define LDBL_TRUE_MIN   ((long double)FLT_TRUE_MIN)
#  define LDBL_MAX        ((long double)FLT_MAX)
#  define LDBL_EPSILON    ((long double)FLT_EPSILON)

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

#  define LDBL_MANT_DIG   DBL_MANT_DIG
#  define LDBL_DIG        DBL_DIG
#  define LDBL_MIN_EXP    DBL_MIN_EXP
#  define LDBL_MIN_10_EXP DBL_MIN_10_EXP
#  define LDBL_MAX_EXP    DBL_MAX_EXP
#  define LDBL_MAX_10_EXP DBL_MAX_10_EXP
#  define LDBL_MIN        ((long double)DBL_MIN)
#  define LDBL_TRUE_MIN   ((long double)DBL_TRUE_MIN)
#  define LDBL_MAX        ((long double)DBL_MAX)
#  define LDBL_EPSILON    ((long double)DBL_EPSILON)
#endif

#endif /* float_h */
