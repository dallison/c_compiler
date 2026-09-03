#include <complex.h>
#include <stdio.h>

typedef float _Complex complex_float;
typedef double _Complex complex_double;

struct holder {
  unsigned char prefix;
  complex_double value;
  unsigned char suffix;
};

static struct holder global_holder = {
    17, CMPLX(6.0, -7.0), 23
};

static complex_float echo_float(complex_float value) {
  return value;
}

static complex_double rotate(complex_double value) {
  return value * I;
}

int main(void) {
  if (global_holder.prefix != 17 || global_holder.suffix != 23)
    return 1;
  if (creal(global_holder.value) != 6.0 ||
      cimag(global_holder.value) != -7.0)
    return 2;

  complex_float value = echo_float(CMPLXF(1.5f, -2.5f));
  if (crealf(value) != 1.5f || cimagf(value) != -2.5f) return 3;

  complex_double rotated = rotate(CMPLX(3.0, 4.0));
  if (creal(rotated) != -4.0 || cimag(rotated) != 3.0) return 4;

  puts("ok");
  return 0;
}
