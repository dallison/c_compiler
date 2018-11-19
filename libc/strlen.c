//
//  strlen.c
//  c_compiler
//
//  Created by David Allison on 4/29/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stddef.h>

size_t strlen(const char* s) {
  size_t len = 0;
  while (*s++ != '\0') {
    ++len;
  }
  return len;
}
