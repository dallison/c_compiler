/* Conditional (?:) expression whose operands are string literals.

   Regression test: the literalref lowering used to ignore the "-> tmp"
   destination that ?: / && / || use to merge a value into one location, so the
   result tmp was never written and the selected string pointer was read
   uninitialized (typically NULL -> crash in printf). */
#include <stdio.h>

static const char* pick(int c) { return c ? "yes" : "no"; }

int main(void) {
  int t = 1;
  int f = 0;

  /* Directly as a call argument. */
  printf("%s\n", t ? "PASS" : "FAIL");
  printf("%s\n", f ? "FAIL" : "PASS");

  /* Assigned to a pointer, then used. */
  const char* p = t ? "alpha" : "beta";
  printf("%s\n", p);
  const char* q = f ? "alpha" : "beta";
  printf("%s\n", q);

  /* Returned from a function (both branches exercised). */
  printf("%s\n", pick(1));
  printf("%s\n", pick(0));
  return 0;
}
