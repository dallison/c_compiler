#include <math.h>

int main(void) {
  double ipart = 99.0;
  (void)modf(3.7, &ipart);
  if (ipart > 2.95 && ipart < 3.05) return 0;
  return (int)(ipart * 10.0);
}
