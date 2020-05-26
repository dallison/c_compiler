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

#ifndef NDEBUG
#define assert(e) do { if (!(e)) \
  { printf("Assertion failed: " #e); abort(); } \
} while (0)
#else
#define assert(e)
#endif

#endif /* __DAVECC__ */
#endif /* assert_h */
