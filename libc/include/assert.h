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
#define assert(e)  \
    ((void) ((e) ? ((void)0) : __assert (#e, __FILE__, __LINE__)))
#define __assert(e, file, line) \
    ((void)printf ("%s:%d: failed assertion `%s'\n", file, line, e), abort())

#else
#define assert(e)
#endif

#endif /* __DAVECC__ */
#endif /* assert_h */
