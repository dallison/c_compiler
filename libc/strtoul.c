//
//  strtoul.c
//  c_compiler
//
//  Created by David Allison on 10/18/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include "strtox_long.h"

unsigned long strtoul(const char* str, char** end, int base) {
  __StrtoxLongResult parsed =
      __StrtoxLong(str, end, base, ULONG_MAX, ULONG_MAX);
  if (!parsed.converted) {
    return 0;
  }
  if (parsed.overflow) {
    return ULONG_MAX;
  }
  unsigned long value = (unsigned long)parsed.value;
  return parsed.negative ? (unsigned long)(0 - value) : value;
}

