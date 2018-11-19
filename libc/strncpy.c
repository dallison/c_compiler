//
//  strncpy.c
//  c_compiler
//
//  Created by David Allison on 4/29/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stddef.h>

char* strncpy(char* dest, const char* src, size_t len) {
  char* p = dest;
  while (len-- > 0 && *src != '\0') {
    *p++ = *src++;
  }
  if (len > 0) {
    *p = '\0';
  }
  return dest;
}
