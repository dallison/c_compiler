//
//  stddef.h
//  c_compiler
//
//  Created by David Allison on 1/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef stddef_h
#define stddef_h
#ifdef __DAVECC__

#include <limits.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define __STDC_VERSION_STDDEF_H__ 202311L
#endif

#ifndef __SIZE_T
#if defined(__LP64__)
typedef unsigned long size_t;
#else
typedef unsigned int size_t;
#endif
#define __SIZE_T
#endif

#ifndef __SSIZE_T
#if defined(__LP64__)
typedef long ssize_t;
#else
typedef int ssize_t;
#endif
#define __SSIZE_T
#endif

#ifndef __PTRDIFF_T
#if defined(__LP64__)
typedef long ptrdiff_t;
#else
typedef int ptrdiff_t;
#endif
#define __PTRDIFF_T
#endif

#ifndef __MAX_ALIGN_T
typedef struct {
  long long __max_align_ll;
  long double __max_align_ld;
} max_align_t;
#define __MAX_ALIGN_T
#endif

#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif

#if !defined(__WCHAR_T) && !defined(__cplusplus)
typedef int wchar_t;
#define __WCHAR_T
#endif

#if defined(__cplusplus) && __cplusplus >= 201103L
namespace std {
using nullptr_t = decltype(nullptr);
}
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
typedef typeof(nullptr) nullptr_t;
#define unreachable() __builtin_unreachable()
#endif

#define offsetof(type, member) ((size_t)(&((type*)0)->member))

#ifndef SIZE_MAX
#if defined(__LP64__)
#define SIZE_MAX ULONG_MAX
#else
#define SIZE_MAX UINT_MAX
#endif
#endif
#endif /* __DAVECC__ */

#endif /* stddef_h */
