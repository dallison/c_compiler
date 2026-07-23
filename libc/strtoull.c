//
//  strtoull.c
//  c_compiler
//
//  Created by David Allison on 12/2/21.
//  Copyright © 2021 David Allison. All rights reserved.
//
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include "strtox.h"

unsigned long long strtoull(const char* str, char** end, int base) {
  __StrtoxResult parsed =
      __Strtox(str, end, base, ULLONG_MAX, ULLONG_MAX);
  if (!parsed.converted) {
    return 0;
  }
  if (parsed.overflow) {
    return ULLONG_MAX;
  }
  if (parsed.negative) {
    return 0ULL - parsed.value;
  }
  return parsed.value;
}
