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
#if defined(__6502__)
  const char* s = src;
  while (n-- > 0) {
    *p++ = *s++;
  }
  return dest;
#else
  // 64 bit.  Each chunk count must be derived from the *remaining* byte count,
  // so it is computed after the previous (wider) step has reduced n.
  size_t n64 = n / sizeof(int64_t);
  if (n64 > 0) {
    uint64_t* wp = p;
    const uint64_t* ws = src;
    while (n64-- > 0) {
      *wp++ = *ws++;
    }
    p = wp;
    src = ws;
    n %= sizeof(int64_t);
  }

  // 32 bit.
  size_t n32 = n / sizeof(int32_t);
  if (n32 > 0) {
    uint32_t* wp = p;
    const uint32_t* ws = src;
    while (n32-- > 0) {
      *wp++ = *ws++;
    }
    p = wp;
    src = ws;
    n %= sizeof(int32_t);
  }
  // 16 bit.
  size_t n16 = n / sizeof(int16_t);
  if (n16 > 0) {
    uint16_t* wp = p;
    const uint16_t* ws = src;
    while (n16-- > 0) {
      *wp++ = *ws++;
    }
    p = wp;
    src = ws;
    n %= sizeof(int16_t);
  }
  
  // Any remaining bytes.
  const char* s = src;
  while (n-- > 0) {
    *p++ = *s++;
  }
  return dest;
#endif
}

