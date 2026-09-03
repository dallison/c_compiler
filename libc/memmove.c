//
//  memmove.c
//  c_compiler
//
//  Created by David Allison on 1/4/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stddef.h>
#include <stdint.h>
#include <string.h>

void* memmove(void* dest, const void* src, size_t n) {
  char* p = dest;
  const char* s = src;
  uintptr_t destination_address;
  uintptr_t source_address;
  if (n == 0 || dest == src) {
    return dest;
  }
  destination_address = (uintptr_t)dest;
  source_address = (uintptr_t)src;
  if (destination_address > source_address &&
      destination_address - source_address < n) {
    // Dest is inside src buffer, copy from end.
    p += n;
    s += n;
    while (n != 0) {
      --n;
      *--p = *--s;
    }
  } else {
    memcpy(dest, src, n);
  }
  return dest;
}

