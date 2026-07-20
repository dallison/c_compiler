//
//  strtol.c
//  c_compiler
//
//  Created by David Allison on 1/4/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include "strtox.h"

long strtol(const char* str, char** end, int base) {
  __StrtoxResult parsed =
      __Strtox(str, end, base, (unsigned long long)LONG_MAX,
               (unsigned long long)LONG_MAX + 1);
  if (!parsed.converted) {
    return 0;
  }
  if (parsed.overflow) {
    return parsed.negative ? LONG_MIN : LONG_MAX;
  }
  if (parsed.negative) {
    if (parsed.value == (unsigned long long)LONG_MAX + 1) {
      return LONG_MIN;
    }
    return -(long)parsed.value;
  }
  return (long)parsed.value;
}

int atoi(const char* s) {
  return (int)strtol(s, NULL, 10);
}

long atol(const char* s) {
  return strtol(s, NULL, 10);
}
