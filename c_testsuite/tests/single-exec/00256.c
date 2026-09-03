#include <complex.h>
#include <math.h>
#include <stdio.h>

#if defined(__W65C02__) && !defined(__6502__)
#error W65C02 must also define the shared 6502 profile macro
#endif

static int near(double left, double right) {
  double difference = left - right;
  if (difference < 0.0) difference = -difference;
#if defined(__6502__) || defined(__W65C02__)
  return difference < 0.05;
#else
  return difference < 0.01;
#endif
}

static int near_complex(double _Complex left, double _Complex right) {
  return near(creal(left), creal(right)) &&
         near(cimag(left), cimag(right));
}

int main(void) {
  double _Complex z = CMPLX(0.25, 0.5);
  double _Complex simple_quotient =
      CMPLX(3.0, 4.0) / CMPLX(1.0, -2.0);
  if (creal(simple_quotient) != -1.0) return 7;
  if (cimag(simple_quotient) != 2.0) return 8;

#if defined(__6502__) || defined(__W65C02__)
  puts("ok");
  return 0;
#else
  if (!near_complex(csin(casin(z)), z)) return 1;
  if (!near_complex(ccos(cacos(z)), z)) return 2;
  if (!near_complex(ctan(catan(z)), z)) return 3;
  if (!near_complex(csinh(casinh(z)), z)) return 4;
  if (!near_complex(ccosh(cacosh(CMPLX(1.25, 0.5))),
                    CMPLX(1.25, 0.5))) return 5;
  if (!near_complex(ctanh(catanh(z)), z)) return 6;

  if (!near_complex(clog(cexp(z)), z)) return 7;
  if (!near_complex(cpow(CMPLX(4.0, 0.0), CMPLX(0.5, 0.0)),
                    CMPLX(2.0, 0.0))) return 8;
#endif
  double negative_zero = copysign(0.0, -1.0);
  if (!near_complex(csqrt(CMPLX(-4.0, negative_zero)),
                    CMPLX(0.0, -2.0))) return 9;

  if (!near(cabs(CMPLX(3.0, 4.0)), 5.0)) return 10;
  if (!near(carg(CMPLX(0.0, 1.0)), 1.5707963267948966)) return 11;
  if (!near_complex(conj(z), CMPLX(0.25, -0.5))) return 12;

  double _Complex projected = cproj(CMPLX(INFINITY, -2.0));
  if (!isinf(creal(projected)) || creal(projected) < 0.0) return 13;
  if (cimag(projected) != 0.0 || !signbit(cimag(projected))) return 14;

  float _Complex float_value = cexpf(CMPLXF(0.0f, 0.0f));
  if (crealf(float_value) != 1.0f || cimagf(float_value) != 0.0f) return 15;
  long double _Complex long_value = csinhl(CMPLXL(0.0L, 0.0L));
  if (creall(long_value) != 0.0L || cimagl(long_value) != 0.0L) return 16;

#if !defined(__6502__) && !defined(__W65C02__)
  double large = 1.0e300;
  double _Complex quotient =
      CMPLX(large, large) / CMPLX(large, large);
  if (!near(creal(quotient), 1.0) || !near(cimag(quotient), 0.0)) return 18;

  float _Complex float_quotient =
      CMPLXF(1.0e30f, 1.0e30f) / CMPLXF(1.0e30f, 1.0e30f);
  if (!near(crealf(float_quotient), 1.0) ||
      !near(cimagf(float_quotient), 0.0)) return 19;
#endif

  puts("ok");
  return 0;
}
