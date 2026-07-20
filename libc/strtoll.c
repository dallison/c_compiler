//
//  strtoll.c
//  c_compiler
//
//  Created by David Allison on 12/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include "strtox.h"

long long strtoll(const char* str, char** end, int base) {
  __StrtoxResult parsed =
      __Strtox(str, end, base, (unsigned long long)LLONG_MAX,
               (unsigned long long)LLONG_MAX + 1);
  if (!parsed.converted) {
    return 0;
  }
  if (parsed.overflow) {
    return parsed.negative ? LLONG_MIN : LLONG_MAX;
  }
  if (parsed.negative) {
    if (parsed.value == (unsigned long long)LLONG_MAX + 1) {
      return LLONG_MIN;
    }
    return -(long long)parsed.value;
  }
  return (long long)parsed.value;
}

long long atoll(const char* s) {
  return strtoll(s, NULL, 10);
}
