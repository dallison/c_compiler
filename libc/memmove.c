//
//  memmove.c
//  c_compiler
//
//  Created by David Allison on 1/4/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stddef.h>
#include <string.h>

void* memmove(void* dest, const void* src, size_t n) {
  char* p = dest;
  const char* s = src;
  if (p >= s && (p < s + n)) {
    // Dest is inside src buffer, copy from end.
    p += n - 1;
    s += n - 1;
    while (s >= (const char*)src) {
      *p-- = *s--;
    }
  } else {
    memcpy(dest, src, n);
  }
  return dest;
}

