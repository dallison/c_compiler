#include <complex.h>
#include <stdio.h>

typedef float _Complex complex_float;
typedef double _Complex complex_double;

static complex_double global_complex = CMPLX(6.0, -7.0);
static complex_double global_real = 9.0;

static complex_double make_complex(double real, double imaginary) {
  return CMPLX(real, imaginary);
}

static complex_double combine(complex_double left, complex_double right) {
  return left * right + left / right;
}

int main(void) {
  if (creal(global_complex) != 6.0 || cimag(global_complex) != -7.0)
    return 27;
  if (creal(global_real) != 9.0 || cimag(global_real) != 0.0)
    return 28;
  complex_double a = CMPLX(3.0, 4.0);
  complex_double b = CMPLX(1.0, -2.0);

  complex_double sum = a + b;
  if (creal(sum) != 4.0) return 1;
  if (cimag(sum) != 2.0) return 18;

  complex_double difference = a - b;
  if (creal(difference) != 2.0 || cimag(difference) != 6.0) return 2;

  complex_double product = a * b;
  if (creal(product) != 11.0) return 3;
  if (cimag(product) != -2.0) return 19;

  complex_double quotient = a / b;
  if (creal(quotient) != -1.0) return 4;
  if (cimag(quotient) != 2.0) return 5;

  complex_double negated = -a;
  if (creal(negated) != -3.0 || cimag(negated) != -4.0) return 6;

  complex_double from_real = 7;
  if (creal(from_real) != 7.0 || cimag(from_real) != 0.0) return 7;
  if ((double)a != 3.0) return 8;
  if (!a || CMPLX(0.0, 0.0)) return 9;

  complex_float narrow = a;
  if (crealf(narrow) != 3.0f || cimagf(narrow) != 4.0f) return 10;
  complex_double wide = narrow;
  if (wide != a) return 11;

  complex_double made = make_complex(2.0, 5.0);
  if (creal(made) != 2.0 || cimag(made) != 5.0) return 12;

  complex_double combined = combine(a, b);
  if (creal(combined) != 10.0 || cimag(combined) != 0.0) return 13;

  complex_double assigned = a;
  assigned += b;
  if (assigned != sum) return 21;
  assigned -= b;
  if (assigned != a) return 22;
  assigned *= b;
  if (assigned != product) return 23;
  assigned /= b;
  if (!(creal(assigned) > 2.999 && creal(assigned) < 3.001 &&
        cimag(assigned) > 3.999 && cimag(assigned) < 4.001))
    return 24;
  assigned += 2.0;
  if (creal(assigned) != 5.0 || cimag(assigned) != 4.0) return 25;
  assigned -= 2.0;
  complex_float mixed = CMPLXF(1.0f, 2.0f);
  mixed += CMPLX(3.0, 4.0);
  if (crealf(mixed) != 4.0f || cimagf(mixed) != 6.0f) return 26;

  complex_double imaginary_unit = I;
  if (creal(imaginary_unit) != 0.0 || cimag(imaginary_unit) != 1.0)
    return 14;
  complex_double root = csqrt(CMPLX(-4.0, 0.0));
  if (!(creal(root) > -0.001 && creal(root) < 0.001)) return 15;
  if (!(cimag(root) > 1.999 && cimag(root) < 2.001)) return 20;

  if (sizeof(complex_float) != 2 * sizeof(float)) return 16;
  if (sizeof(complex_double) != 2 * sizeof(double)) return 17;

  puts("ok");
  return 0;
}
