//
//  limits.h
//  c_compiler
//
//  Created by David Allison on 5/9/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef limits_h
#define limits_h

#  define CHAR_BIT  8

/* Minimum and maximum values a `signed char' can hold.  */
#  define SCHAR_MIN  (-128)
#  define SCHAR_MAX  127

/* Maximum value an `unsigned char' can hold.  (Minimum is 0.)  */
#  define UCHAR_MAX  255

/* Minimum and maximum values a `char' can hold.  */
#  ifdef __CHAR_UNSIGNED__
#   define CHAR_MIN  0
#   define CHAR_MAX  UCHAR_MAX
#  else
#   define CHAR_MIN  SCHAR_MIN
#   define CHAR_MAX  SCHAR_MAX
#  endif

/* Minimum and maximum values a `signed short int' can hold.  */
#  define SHRT_MIN  (-32768)
#  define SHRT_MAX  32767

/* Maximum value an `unsigned short int' can hold.  (Minimum is 0.)  */
#  define USHRT_MAX  65535

#if defined(__6502__)
// 6502 uses 16-bit ints.  Longs are 32-bit.  Long long is 64 bit.
/* Minimum and maximum values a `signed int' can hold.  */
#  define INT_MIN  (-INT_MAX - 1)
#  define INT_MAX  32767

/* Maximum value an `unsigned int' can hold.  (Minimum is 0.)  */
#  define UINT_MAX  65535U

/* Minimum and maximum values a `signed long int' can hold.  */
#   define LONG_MAX  2147483647L
#  define LONG_MIN  (-LONG_MAX - 1L)

/* Maximum value an `unsigned long int' can hold.  (Minimum is 0.)  */
#   define ULONG_MAX  4294967295UL


#else
#  define INT_MIN  (-INT_MAX - 1)
#  define INT_MAX  2147483647
/* Maximum value an `unsigned int' can hold.  (Minimum is 0.)  */
#  define UINT_MAX  4294967295U

/* Minimum and maximum values a `signed long int' can hold.  */
#  if __WORDSIZE == 64
#   define LONG_MAX  9223372036854775807L
#  else
#   define LONG_MAX  2147483647L
#  endif
#  define LONG_MIN  (-LONG_MAX - 1L)

/* Maximum value an `unsigned long int' can hold.  (Minimum is 0.)  */
#  if __WORDSIZE == 64
#   define ULONG_MAX  18446744073709551615UL
#  else
#   define ULONG_MAX  4294967295UL
#  endif
#endif


/* Minimum and maximum values a `signed long long int' can hold.  */
#   define LLONG_MAX  9223372036854775807LL
#   define LLONG_MIN  (-LLONG_MAX - 1LL)

/* Maximum value an `unsigned long long int' can hold.  (Minimum is 0.)  */
#   define ULLONG_MAX  18446744073709551615ULL

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#if defined(__6502__) || defined(__W65C02__)
#define WCHAR_MIN (-32767 - 1)
#define WCHAR_MAX 32767
#else
#define WCHAR_MIN (-2147483647 - 1)
#define WCHAR_MAX 2147483647
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#  define BOOL_WIDTH 1
#  define CHAR_WIDTH CHAR_BIT
#  define SCHAR_WIDTH CHAR_BIT
#  define UCHAR_WIDTH CHAR_BIT
#  define SHRT_WIDTH 16
#  define USHRT_WIDTH 16
#  if defined(__6502__)
#    define INT_WIDTH 16
#    define UINT_WIDTH 16
#    define LONG_WIDTH 32
#    define ULONG_WIDTH 32
#  else
#    define INT_WIDTH 32
#    define UINT_WIDTH 32
#    if __WORDSIZE == 64
#      define LONG_WIDTH 64
#      define ULONG_WIDTH 64
#    else
#      define LONG_WIDTH 32
#      define ULONG_WIDTH 32
#    endif
#  endif
#  define LLONG_WIDTH 64
#  define ULLONG_WIDTH 64
#  define BITINT_MAXWIDTH 64
#endif

#endif /* limits_h */
