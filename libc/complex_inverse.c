#include <complex.h>

double complex casin(double complex z) {
  if (z.__real == 0.0 && z.__imag == 0.0)
    return z;
  double complex iz = CMPLX(-z.__imag, z.__real);
  double complex value = clog(iz + csqrt(CMPLX(1.0, 0.0) - z * z));
  return CMPLX(value.__imag, -value.__real);
}

double complex cacos(double complex z) {
  if (z.__real == 1.0 && z.__imag == 0.0)
    return CMPLX(0.0, -z.__imag);
  double complex value = casin(z);
  return CMPLX(1.57079632679489661923 - value.__real, -value.__imag);
}

double complex catan(double complex z) {
  if (z.__real == 0.0 && z.__imag == 0.0)
    return z;
  double complex iz = CMPLX(-z.__imag, z.__real);
  double complex value =
      clog(CMPLX(1.0, 0.0) - iz) - clog(CMPLX(1.0, 0.0) + iz);
  return CMPLX(-0.5 * value.__imag, 0.5 * value.__real);
}

double complex casinh(double complex z) {
  if (z.__real == 0.0 && z.__imag == 0.0)
    return z;
  return clog(z + csqrt(z * z + CMPLX(1.0, 0.0)));
}

double complex cacosh(double complex z) {
  if (z.__real == 1.0 && z.__imag == 0.0)
    return CMPLX(0.0, z.__imag);
  return clog(z + csqrt(z + CMPLX(1.0, 0.0)) *
                      csqrt(z - CMPLX(1.0, 0.0)));
}

double complex catanh(double complex z) {
  if (z.__real == 0.0 && z.__imag == 0.0)
    return z;
  return (clog(CMPLX(1.0, 0.0) + z) -
          clog(CMPLX(1.0, 0.0) - z)) *
         0.5;
}

#define DAVECC_COMPLEX_INVERSE_WRAPPERS(name)                         \
  float complex name##f(float complex z) {                            \
    return (float complex)name((double complex)z);                    \
  }                                                                   \
  long double complex name##l(long double complex z) {                \
    return (long double complex)name((double complex)z);              \
  }

DAVECC_COMPLEX_INVERSE_WRAPPERS(casin)
DAVECC_COMPLEX_INVERSE_WRAPPERS(cacos)
DAVECC_COMPLEX_INVERSE_WRAPPERS(catan)
DAVECC_COMPLEX_INVERSE_WRAPPERS(casinh)
DAVECC_COMPLEX_INVERSE_WRAPPERS(cacosh)
DAVECC_COMPLEX_INVERSE_WRAPPERS(catanh)

#undef DAVECC_COMPLEX_INVERSE_WRAPPERS
