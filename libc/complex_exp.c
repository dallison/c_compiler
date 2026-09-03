#include <complex.h>
#include <math.h>

#undef cabs
#undef carg

double complex cexp(double complex z) {
  if (z.__real == 0.0 && z.__imag == 0.0) {
    return CMPLX(1.0, z.__imag);
  }
  double magnitude = exp(z.__real);
  return CMPLX(magnitude * cos(z.__imag), magnitude * sin(z.__imag));
}

double complex clog(double complex z) {
  return CMPLX(log(cabs(z)), carg(z));
}

double complex cpow(double complex base, double complex exponent) {
  return cexp(exponent * clog(base));
}

float complex cexpf(float complex z) {
  return (float complex)cexp((double complex)z);
}
long double complex cexpl(long double complex z) {
  return (long double complex)cexp((double complex)z);
}
float complex clogf(float complex z) {
  return (float complex)clog((double complex)z);
}
long double complex clogl(long double complex z) {
  return (long double complex)clog((double complex)z);
}
float complex cpowf(float complex base, float complex exponent) {
  return (float complex)cpow((double complex)base,
                             (double complex)exponent);
}
long double complex cpowl(long double complex base,
                          long double complex exponent) {
  return (long double complex)cpow((double complex)base,
                                   (double complex)exponent);
}
