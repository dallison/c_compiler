//
//  stdint.h
//  c_compiler
//
//  Created by David Allison on 1/20/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef stdint_h
#define stdint_h
#ifdef __DAVECC__

#include <limits.h>

#if defined(__6502__)
// On 6502, pointers and ints are 16 bits long.
typedef signed char int8_t;
typedef int int16_t;
typedef long int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned int uintptr_t;
typedef int intptr_t;
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;
typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;
typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;
#elif defined(__LP64__)
// LP64 (e.g. x86-64): long is 64 bits.
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef unsigned long uintptr_t;
typedef long intptr_t;
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;
typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;
typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;
#else
// ILP32 (e.g. 32-bit ARM): long is 32 bits, so 64-bit needs long long.
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned int uintptr_t;
typedef int intptr_t;
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;
typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;
typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;
#endif

typedef int8_t int_fast8_t;
typedef int16_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef int64_t int_fast64_t;
typedef uint8_t uint_fast8_t;
typedef uint16_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
typedef uint64_t uint_fast64_t;

#if defined(__LP64__)
#define INTPTR_MIN LONG_MIN
#define INTPTR_MAX LONG_MAX
#define UINTPTR_MAX ULONG_MAX
#define INT64_MIN LONG_MIN
#define INT64_MAX LONG_MAX
#define UINT64_MAX ULONG_MAX
#else
#define INTPTR_MIN INT_MIN
#define INTPTR_MAX INT_MAX
#define UINTPTR_MAX UINT_MAX
#define INT64_MIN LLONG_MIN
#define INT64_MAX LLONG_MAX
#define UINT64_MAX ULLONG_MAX
#endif

#define INT8_MIN SCHAR_MIN
#define INT8_MAX SCHAR_MAX
#define UINT8_MAX UCHAR_MAX
#define INT16_MIN SHRT_MIN
#define INT16_MAX SHRT_MAX
#define UINT16_MAX USHRT_MAX
#if defined(__6502__)
#define INT32_MIN LONG_MIN
#define INT32_MAX LONG_MAX
#define UINT32_MAX ULONG_MAX
#else
#define INT32_MIN INT_MIN
#define INT32_MAX INT_MAX
#define UINT32_MAX UINT_MAX
#endif

#ifndef SIZE_MAX
#if defined(__6502__)
#define SIZE_MAX UINT_MAX
#else
#define SIZE_MAX ULONG_MAX
#endif
#endif

#endif /* __DAVECC__ */
#endif /* stdint_h */
