#include <math.h>

#define complex _Complex
#define CMPLXF(real, imaginary) \
  ((float complex){.__real = (real), .__imag = (imaginary)})
#define CMPLX(real, imaginary) \
  ((double complex){.__real = (real), .__imag = (imaginary)})
#define CMPLXL(real, imaginary) \
  ((long double complex){.__real = (real), .__imag = (imaginary)})

float crealf(float complex z) { return z.__real; }
double creal(double complex z) { return z.__real; }
long double creall(long double complex z) { return z.__real; }

float cimagf(float complex z) { return z.__imag; }
double cimag(double complex z) { return z.__imag; }
long double cimagl(long double complex z) { return z.__imag; }

float cabsf(float complex z) {
  return (float)hypot((double)z.__real, (double)z.__imag);
}
double cabs(double complex z) { return hypot(z.__real, z.__imag); }
long double cabsl(long double complex z) {
  return (long double)hypot((double)z.__real, (double)z.__imag);
}

float cargf(float complex z) {
  return (float)atan2((double)z.__imag, (double)z.__real);
}
double carg(double complex z) { return atan2(z.__imag, z.__real); }
long double cargl(long double complex z) {
  return (long double)atan2((double)z.__imag, (double)z.__real);
}

float complex conjf(float complex z) {
  return CMPLXF(z.__real, -z.__imag);
}
double complex conj(double complex z) {
  return CMPLX(z.__real, -z.__imag);
}
long double complex conjl(long double complex z) {
  return CMPLXL(z.__real, -z.__imag);
}

float complex cprojf(float complex z) {
  if (isinf(z.__real) || isinf(z.__imag))
    return CMPLXF(INFINITY, copysignf(0.0f, z.__imag));
  return z;
}
double complex cproj(double complex z) {
  if (isinf(z.__real) || isinf(z.__imag))
    return CMPLX(INFINITY, copysign(0.0, z.__imag));
  return z;
}
long double complex cprojl(long double complex z) {
  if (isinf(z.__real) || isinf(z.__imag))
    return CMPLXL(INFINITY, copysignl(0.0L, z.__imag));
  return z;
}
