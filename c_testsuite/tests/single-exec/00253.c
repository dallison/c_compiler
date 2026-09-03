#include <complex.h>
#include <stdio.h>

typedef double _Complex complex_double;
typedef long double _Complex complex_long_double;

static complex_double first_value(void) {
  return CMPLX(1.0, 2.0);
}

static complex_double second_value(void) {
  return CMPLX(3.0, 4.0);
}

int main(void) {
  complex_double call_sum = first_value() + second_value();
  if (creal(call_sum) != 4.0 || cimag(call_sum) != 6.0) return 1;

  complex_double call_product = first_value() * second_value();
  if (creal(call_product) != -5.0 || cimag(call_product) != 10.0)
    return 2;

  complex_double value = CMPLX(3.0, 4.0);
  complex_double conjugate = conj(value);
  if (creal(conjugate) != 3.0 || cimag(conjugate) != -4.0) return 3;
  if (cabs(value) != 5.0) return 4;

  complex_double exponential = cexp(CMPLX(0.0, 0.0));
  if (!(creal(exponential) > 0.99 && creal(exponential) < 1.01))
    return 5;
  if (!(cimag(exponential) > -0.001 && cimag(exponential) < 0.001))
    return 6;

  complex_double logarithm = clog(CMPLX(1.0, 0.0));
  if (creal(logarithm) != 0.0 || cimag(logarithm) != 0.0) return 7;

  complex_long_double long_value = CMPLXL(2.0L, 3.0L);
  complex_long_double long_sum = long_value + CMPLXL(4.0L, -1.0L);
  if (creall(long_sum) != 6.0L || cimagl(long_sum) != 2.0L) return 8;
  if (sizeof(complex_long_double) != 2 * sizeof(long double)) return 9;

  puts("ok");
  return 0;
}
