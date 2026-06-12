//
//  memcpy.c
//  c_compiler
//
//  Created by David Allison on 4/30/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stddef.h>
#include <stdint.h>

// Simple, non-optimal memory copy.  In reality this needs to be really fast
// using vector operations.
void *memcpy(void *dest, const void *src, size_t n) {
  char* p = dest;
  const char* s = src;
  while (n-- > 0) {
    *p++ = *s++;
  }
  return dest;
}

