#include <complex.h>
#include <math.h>

double complex csin(double complex z) {
  return CMPLX(sin(z.__real) * cosh(z.__imag),
               cos(z.__real) * sinh(z.__imag));
}

double complex ccos(double complex z) {
  return CMPLX(cos(z.__real) * cosh(z.__imag),
               -sin(z.__real) * sinh(z.__imag));
}

double complex ctan(double complex z) { return csin(z) / ccos(z); }

float complex csinf(float complex z) {
  return (float complex)csin((double complex)z);
}
long double complex csinl(long double complex z) {
  return (long double complex)csin((double complex)z);
}
float complex ccosf(float complex z) {
  return (float complex)ccos((double complex)z);
}
long double complex ccosl(long double complex z) {
  return (long double complex)ccos((double complex)z);
}
float complex ctanf(float complex z) {
  return (float complex)ctan((double complex)z);
}
long double complex ctanl(long double complex z) {
  return (long double complex)ctan((double complex)z);
}
