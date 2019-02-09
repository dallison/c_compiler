//
//  strncat.c
//  c_compiler
//
//  Created by David Allison on 1/4/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stddef>

char* strncat(char* dest, const char* src, size_t n) {
  char* p = dest;
  while (*p != '\0') {
    p++;
  }
  while (n-- > 0 && *src != '\0') {
    *p++ = *src++;
  }
  *p = '\0';
  return dest;
}
