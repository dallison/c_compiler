#include <tgmath.h>

static int near(double left, double right) {
  double difference = left - right;
  if (difference < 0.0) difference = -difference;
  return difference < 0.02;
}

int main(void) {
  float float_value = sqrt(4.0f);
  if (float_value != 2.0f) return 1;

  double _Complex complex_value = sqrt(CMPLX(-4.0, 0.0));
  if (!near(creal(complex_value), 0.0)) return 2;
  if (!near(cimag(complex_value), 2.0)) return 3;

  complex_value = exp(CMPLX(0.0, 0.0));
  if (!near(creal(complex_value), 1.0)) return 4;
  if (!near(cimag(complex_value), 0.0)) return 5;

  int evaluations = 0;
  if (sin(evaluations++) != 0.0 || evaluations != 1) return 6;

  float integer_part = 0.0f;
  float fractional_part = modf(2.5f, &integer_part);
  if (integer_part != 2.0f || fractional_part != 0.5f) return 7;

  return 0;
}
