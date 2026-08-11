#include <math.h>

int main(void) {
  double r = ldexp(1.0, 0);
  if (r == 1.0) return 0;
  if (r == 0.0) return 1;
  if (r == 2.0) return 2;
  return 3;
}
