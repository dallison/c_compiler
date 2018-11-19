//
//  memset.c
//  c_compiler
//
//  Created by David Allison on 4/30/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stddef.h>

// Trivial, simple memory set.  In reality this would need to be
// optimized for the architecture using vector operations.
void *memset(void *s, int c, size_t n) {
  char* p = s;
  while (n-- > 0) {
    *p++ = c;
  }
  return s;
}
