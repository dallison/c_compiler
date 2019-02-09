//
//  memcmp.c
//  c_compiler
//
//  Created by David Allison on 1/4/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stddef.h>

int memcmp(const void* s1, const void* s2, size_t n) {
  const unsigned char* p1 = s1;
  const unsigned char* p2 = s2;
  while (n-- > 0 && *p1 == *p2) {
    p1++;
    p2++;
  }
  return *p1 - *p2;
}
