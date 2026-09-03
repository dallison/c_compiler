#include <math.h>

#define complex _Complex
#define CMPLX(real, imaginary) \
  ((double complex){.__real = (real), .__imag = (imaginary)})

double complex csqrt(double complex z) {
  if (z.__real == 0.0 && z.__imag == 0.0)
    return CMPLX(0.0, z.__imag);
  double magnitude = hypot(z.__real, z.__imag);
  double real_squared = (magnitude + z.__real) * 0.5;
  double imaginary_squared = (magnitude - z.__real) * 0.5;
  // Lower-precision targets can round hypot(x, 0) just below |x|.
  if (real_squared < 0.0) real_squared = 0.0;
  if (imaginary_squared < 0.0) imaginary_squared = 0.0;
  double real = sqrt(real_squared);
  double imaginary = sqrt(imaginary_squared);
  if (z.__imag < 0.0) imaginary = -imaginary;
  return CMPLX(real, imaginary);
}

float complex csqrtf(float complex z) {
  return (float complex)csqrt((double complex)z);
}

long double complex csqrtl(long double complex z) {
  return (long double complex)csqrt((double complex)z);
}
