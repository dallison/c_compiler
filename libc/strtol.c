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

#include "strtox_long.h"

long strtol(const char* str, char** end, int base) {
  __StrtoxLongResult parsed =
      __StrtoxLong(str, end, base, (unsigned long)LONG_MAX,
                   (unsigned long)LONG_MAX + 1);
  if (!parsed.converted) {
    return 0;
  }
  if (parsed.overflow) {
    return parsed.negative ? LONG_MIN : LONG_MAX;
  }
  if (parsed.negative) {
    if (parsed.value == (unsigned long)LONG_MAX + 1) {
      return LONG_MIN;
    }
    return -(long)parsed.value;
  }
  return (long)parsed.value;
}

// atoi and atol live in their own translation units (atoi.c, atol.c) so
// that linking them doesn't force this file (and the wide arithmetic
// strtol needs) into the image.
