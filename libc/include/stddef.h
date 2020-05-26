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

#ifndef __SIZE_T
typedef unsigned long size_t;
#define __SIZE_T
#endif

#ifndef __SSIZE_T
typedef long ssize_t;
#define __SSIZE_T
#endif

#ifndef __PTRIFF_T
typedef long ptrdiff_t;
#define __PTRDIFF_T
#endif

#define NULL ((void*)0)

#ifndef __WCHAR_T
typedef int wchar_t;
#define __WCHAR_T
#endif

#define offsetof(type, member) ((size_t)(&((type*)0)->member))
#endif /* __DAVECC__ */

#endif /* stddef_h */
