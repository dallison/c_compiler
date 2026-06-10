/* Multiple function calls used as sibling arguments, where the results are
   floating-point.  A call returns its result in a fixed return register, so an
   earlier-evaluated call's result must be stashed into a temporary or a later
   argument's call clobbers it.

   Regression test: floating-point results were previously not stashed (they
   collapsed to a single value, e.g. printing "1.0 1.0"), and the backends'
   call dest-routing copied a floating-point return with an integer move
   (corrupting it to 0.0) and gave the float temporary an integer register. */
#include <stdio.h>

static double fd(double x) { return x; }
static float ff(float x) { return x; }
static int fi(int x) { return x; }

struct P { double a, b; };
static struct P mkp(double v) { struct P p; p.a = v; p.b = v * 2; return p; }

int main(void) {
  printf("%.1f %.1f\n", fd(1.0), fd(2.0));
  printf("%.1f %.1f %.1f\n", fd(7.0), fd(8.0), fd(9.0));
  printf("%.1f %.1f\n", (double)ff(3.0f), (double)ff(4.0f));
  printf("%d %.1f\n", fi(5), fd(6.0));
  printf("%.1f %.1f\n", mkp(10.0).a, mkp(20.0).a);
  return 0;
}
