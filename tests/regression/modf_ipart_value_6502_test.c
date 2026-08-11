#include <math.h>

int main(void) {
  double ipart = 99.0;
  (void)modf(3.0, &ipart);
  if (ipart > 2.9 && ipart < 3.1) return 0;
  if (ipart > 98.9 && ipart < 99.1) return 99;
  return 10;
}
