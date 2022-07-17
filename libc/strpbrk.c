//
//  strpbrk.c
//  c_compiler
//
//  Created by David Allison on 6/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <string.h>
#include <stdbool.h>

// Length of initial segment of s1 comprising chars in s2.
char *strpbrk(const char* s1, const char* s2) {
  while (*s1 != '\0') {
    for (const char* p = s2; *p != '\0'; p++) {
      if (*s1 == *p) {
        return s1;
      }
    }
    s1++;
  }
  return NULL;
}
