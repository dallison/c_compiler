#include <float.h>
#include <stdio.h>

int main(void) {
#if defined(__DAVECC_LDBL_FORMAT__) && \
    (__DAVECC_LDBL_FORMAT__ == 2 || __DAVECC_LDBL_FORMAT__ == 3)
  if (sizeof(long double) != 16) return 1;
  if (LDBL_MANT_DIG < 64) return 2;
  if (!(1.0L + LDBL_EPSILON > 1.0L)) return 3;
  if (LDBL_EPSILON >= (long double)DBL_EPSILON) return 4;

  long double a = 1.0L;
  long double b = 2.0L;
  if (a + b != 3.0L) return 5;
  if (b - a != 1.0L) return 6;
  if (b * b != 4.0L) return 7;
  if (a / b != 0.5L) return 8;
  if (-a != -1.0L) return 9;
  if (!(a < b) || (a > b) || !(a == 1.0L)) return 10;

  long double from_int = (long double)42;
  if (from_int != 42.0L) return 11;
  if ((int)(a + b) != 3) return 12;
#else
  if (sizeof(long double) != sizeof(double) &&
      sizeof(long double) != sizeof(float)) {
    return 1;
  }
  if (1.0L + 1.0L != 2.0L) return 2;
#endif
  puts("ok");
  return 0;
}
