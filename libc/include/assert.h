//
//  assert.h
//  c_compiler
//
//  Created by David Allison on 1/20/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef assert_h
#define assert_h
#ifdef __DAVECC__

#include <stdio.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdlib.h>

#ifndef NDEBUG
// Implemented in libc/assert_fail.c without printf so that assert() doesn't
// pull the printf formatting machinery into every program.
#ifdef __cplusplus
extern "C" void __davecc_assert_fail(const char* expression, const char* file,
                                     int line);
#else
void __davecc_assert_fail(const char* expression, const char* file, int line);
#endif
#define assert(e)  \
    ((void) ((e) ? ((void)0) : __davecc_assert_fail (#e, __FILE__, __LINE__)))

#else
#define assert(e)
#endif

#endif /* __DAVECC__ */
#endif /* assert_h */
