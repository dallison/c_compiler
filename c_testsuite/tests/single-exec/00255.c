#include <complex.h>
#include <stdio.h>

typedef double _Complex complex_double;

static complex_double global_array[2] = {1.0, 2.0};
static complex_double compound_target[1] = {CMPLX(10.0, 20.0)};
static complex_double arithmetic_constant = 1.0 + 2.0 * I;
static complex_double summed_constant =
    CMPLX(1.0, 2.0) + CMPLX(3.0, 4.0);

static complex_double first_value(void) {
  return CMPLX(1.0, 2.0);
}

static complex_double discard_value(void) {
  return CMPLX(100.0, 200.0);
}

int main(void) {
  if (creal(global_array[0]) != 1.0 || cimag(global_array[0]) != 0.0)
    return 1;
  if (creal(global_array[1]) != 2.0 || cimag(global_array[1]) != 0.0)
    return 2;
  if (creal(arithmetic_constant) != 1.0 ||
      cimag(arithmetic_constant) != 2.0)
    return 6;
  if (creal(summed_constant) != 4.0 || cimag(summed_constant) != 6.0)
    return 7;

  complex_double left = CMPLX(3.0, 4.0);
  complex_double right = CMPLX(5.0, 6.0);
  complex_double selected = 1 ? left : right;
  if (creal(selected) != 3.0 || cimag(selected) != 4.0) return 3;
  selected = 0 ? left : right;
  if (creal(selected) != 5.0 || cimag(selected) != 6.0) return 4;

  compound_target[(discard_value(), 0)] += first_value();
  if (creal(compound_target[0]) != 11.0 ||
      cimag(compound_target[0]) != 22.0)
    return 5;

  puts("ok");
  return 0;
}
