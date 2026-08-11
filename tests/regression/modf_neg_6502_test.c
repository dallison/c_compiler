#include <math.h>

int main(void) {
  double ipart = 99.0;
  double fract = modf(-3.2, &ipart);
  if (ipart != -3.0) return 20;
  if (fract >= 0.0) return 21;
  if (fract <= -0.3 || fract >= -0.1) return 22;
  return 0;
}
