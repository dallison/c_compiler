//
//  strrchr.c
//  c_compiler
//
//  Created by David Allison on 1/4/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stddef.h>

char* strrchr(const char* s, char c) {
  const char* start = s;
  while (*s != '\0') {
    s++;
  }
  while (s >= start && *s != c) {
    s--;
  }
  return s < start ? NULL : (char*)s;
}
