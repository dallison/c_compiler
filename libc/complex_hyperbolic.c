#include <complex.h>
#include <math.h>

double complex csinh(double complex z) {
  return CMPLX(sinh(z.__real) * cos(z.__imag),
               cosh(z.__real) * sin(z.__imag));
}

double complex ccosh(double complex z) {
  return CMPLX(cosh(z.__real) * cos(z.__imag),
               sinh(z.__real) * sin(z.__imag));
}

double complex ctanh(double complex z) { return csinh(z) / ccosh(z); }

float complex csinhf(float complex z) {
  return (float complex)csinh((double complex)z);
}
long double complex csinhl(long double complex z) {
  return (long double complex)csinh((double complex)z);
}
float complex ccoshf(float complex z) {
  return (float complex)ccosh((double complex)z);
}
long double complex ccoshl(long double complex z) {
  return (long double complex)ccosh((double complex)z);
}
float complex ctanhf(float complex z) {
  return (float complex)ctanh((double complex)z);
}
long double complex ctanhl(long double complex z) {
  return (long double complex)ctanh((double complex)z);
}
