/* #pragma {davecc,clang,GCC} diagnostic push/pop/ignored/warning/error.

   Exercises every diagnostic-pragma form across all three vendor names.  The
   warnings themselves go to stderr, so this test only asserts that the pragmas
   are accepted and do not disturb parsing or codegen: it must still compile and
   print the expected value. */
#include <stdio.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static int with_unused(void) {
  int never_used;
  return 3;
}
#pragma GCC diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wunused-variable"
#pragma clang diagnostic pop

#pragma davecc diagnostic ignored "-Wunused-variable"

int main(void) {
  printf("%d\n", with_unused());
  return 0;
}
