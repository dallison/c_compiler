//
//  whar.h
//  c_compiler
//
//  Created by David Allison on 11/28/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#ifndef whar_h
#define whar_h

#include <limits.h>

#if !defined(__WCHAR_T) && !defined(__cplusplus)
typedef int wchar_t;
#define __WCHAR_T
#endif
#define WHAR_MAX INT_MAX

#ifndef __SIZE_T
#if defined(__6502__)
typedef unsigned int size_t;
#else
typedef unsigned long size_t;
#endif
#define __SIZE_T
#endif

#define NULL ((void*)0)

#endif /* whar_h */
