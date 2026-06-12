//
//  strchr.c
//  c_compiler
//
//  Created by David Allison on 1/4/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stddef.h>

char* strchr(const char* s, char c) {
  while (*s != '\0' && *s != c) {
    s++;
  }
  if (*s == '\0') {
    return NULL;
  }
  return (char*)s;
}
