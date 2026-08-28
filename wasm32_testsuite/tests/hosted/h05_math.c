#include <math.h>
#include <stdio.h>

int main(void) {
  printf("sqrt %.4f\n", sqrt(2.0));
  printf("pow %.4f\n", pow(2.0, 10.0));
  printf("fabs %.4f %.4f\n", fabs(-3.5), fabs(3.5));
  printf("floor %.1f ceil %.1f\n", floor(-2.5), ceil(-2.5));
  printf("exp %.4f log %.4f\n", exp(1.0), log(100.0));

  double whole = 0.0;
  double part = modf(7.25, &whole);
  printf("modf %.2f %.2f\n", whole, part);

  int exponent = 0;
  double mantissa = frexp(48.0, &exponent);
  printf("frexp %.4f %d\n", mantissa, exponent);
  printf("ldexp %.1f\n", ldexp(0.75, 6));
  return 0;
}
