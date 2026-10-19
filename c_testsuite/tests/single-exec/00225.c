/* #pragma once: a file that is included more than once is read only once.

   This file includes itself.  With a working #pragma once the self-include is
   skipped, so main is defined once and the program prints normally.  If
   #pragma once regressed the self-include would recurse without bound and the
   compile would fail.  A quoted include is found beside the including file. */
#pragma once
#include <stdio.h>

#include "00225.c"

int main(void) {
  printf("pragma once works\n");
  return 0;
}
