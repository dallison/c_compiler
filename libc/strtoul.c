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

#include "strtox.h"

unsigned long strtoul(const char* str, char** end, int base) {
  __StrtoxResult parsed =
      __Strtox(str, end, base, (unsigned long long)ULONG_MAX,
               (unsigned long long)ULONG_MAX);
  if (!parsed.converted) {
    return 0;
  }
  if (parsed.overflow) {
    return ULONG_MAX;
  }
  unsigned long value = (unsigned long)parsed.value;
  return parsed.negative ? (unsigned long)(0 - value) : value;
}

