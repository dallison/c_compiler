/* Multiple function calls used as arguments to another call.

   Regression test: arguments are evaluated right-to-left, and on targets where
   a call returns its value in a fixed register, an earlier-evaluated call's
   result was clobbered by a later argument's call before reaching its own
   argument register.  This collapsed e.g. f(g(1), g(2)) to f(<g(2)>, <g(2)>). */
#include <stdio.h>

static int id(int c) { return c; }
static const char* pick(int c) { return c ? "yes" : "no"; }
static int add(int a, int b) { return a + b; }

int main(void) {
  /* Two call arguments. */
  printf("%d %d\n", id(1), id(2));
  /* Three call arguments. */
  printf("%d %d %d\n", id(7), id(8), id(9));
  /* Pointer-returning calls. */
  printf("%s %s\n", pick(1), pick(0));
  /* Nested: call result feeding another call's arguments. */
  printf("%d\n", add(id(3), id(4)));
  /* Mixed: an expression containing a call alongside a bare call. */
  printf("%d %d\n", id(5) + 1, id(6));
  return 0;
}
