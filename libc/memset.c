//
//  memset.c
//  c_compiler
//
//  Created by David Allison on 4/30/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stddef.h>
#include <stdint.h>


// Trivial, simple memory set.  In reality this would need to be
// optimized for the architecture using vector operations.
void *memset(void *s, int c, size_t n) {
#if 1
  char* p = s;
  while (n-- > 0) {
    *p++ = (char)c;
  }
  return s;
#else
  size_t n64 = n / sizeof(int64_t);
  size_t n32 = n / sizeof(int32_t);
  size_t n16 = n / sizeof(int16_t);
  char* p = s;
  // 64 bit.
  if (n64 > 0) {
    uint64_t c64 = 0;
    for (int i = 0; i < 8; i++) {
      c64 |= (c & 0xff) << (i * 8);
    }
    uint64_t* wp = p;
    while (n64-- > 0) {
      *wp++ = c64;
    }
    p = wp;
    n %= sizeof(int64_t);
  }
  
  // 32 bit.
  if (n32 > 0) {
    uint32_t c32 = 0;
    for (int i = 0; i < 4; i++) {
      c32 |= (c & 0xff) << (i * 8);
    }
    uint32_t* wp = p;
    while (n32-- > 0) {
      *wp++ = c32;
    }
    p = wp;
    n %= sizeof(int32_t);
  }
  
  // 16 bit.
  if (n16 > 0) {
    uint16_t c16 = 0;
    for (int i = 0; i < 2; i++) {
      c16 |= (c & 0xff) << (i * 8);
    }
    uint16_t* wp = p;
    while (n16-- > 0) {
      *wp++ = c16;
    }
    p = wp;
    n %= sizeof(int16_t);
  }
  
  // Any remaining bytes.
  while (n-- > 0) {
    *p++ = c;
  }
  return s;
#endif
}
