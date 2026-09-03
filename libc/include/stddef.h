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
#if defined(__6502__)
typedef unsigned int size_t;
#else
typedef unsigned long size_t;
#endif
#define __SIZE_T
#endif

#ifndef __SSIZE_T
#if defined(__6502__)
typedef int ssize_t;
#else
typedef long ssize_t;
#endif
#define __SSIZE_T
#endif

#ifndef __PTRIFF_T
typedef long ptrdiff_t;
#define __PTRDIFF_T
#endif

#define NULL ((void*)0)

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
#if defined(__6502__)
#define SIZE_MAX UINT_MAX
#else
#define SIZE_MAX ULONG_MAX
#endif
#endif
#endif /* __DAVECC__ */

#endif /* stddef_h */
