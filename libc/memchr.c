//
//  memchr.c
//  c_compiler
//
//  Created by David Allison on 6/17/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <string.h>

void *memchr(const void *s, int c, size_t n) {
  const unsigned char* p = s;
  unsigned char ch = (unsigned char)c;
  while (n-- > 0) {
    if (*p == ch) {
      return (void*)p;
    }
  }
  return NULL;
}
