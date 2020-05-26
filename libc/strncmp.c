//
//  strncmp.c
//  c_compiler
//
//  Created by David Allison on 5/1/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stddef.h>

int strncmp(const char* a, const char* b, size_t n) {
  ssize_t len = n;
  while (len-- > 0 && *a != '\0') {
    if (*a != *b) {
      break;
    }
    a++;
    b++;
  }
  return *a - *b;
}
