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

typedef int wchar_t;
#define WHAR_MAX INT_MAX

#ifndef __SIZE_T
#if defined(__W65C02__)
typedef unsigned int size_t;
#else
typedef unsigned long size_t;
#endif
#define __SIZE_T
#endif

#define NULL ((void*)0)

#endif /* whar_h */
