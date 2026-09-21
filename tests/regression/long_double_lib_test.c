#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
#if defined(__DAVECC_LDBL_FORMAT__) && \
    (__DAVECC_LDBL_FORMAT__ == 2 || __DAVECC_LDBL_FORMAT__ == 3)
  if (sizeof(long double) != 16) {
    return 1;
  }

  char* end = NULL;
  long double parsed = strtold("1.25e2", &end);
  if (parsed != 125.0L || end == NULL || *end != '\0') {
    return 2;
  }
  if (strtold("-inf", NULL) >= 0.0L || !isinf(strtold("-inf", NULL))) {
    return 3;
  }

  long double extra = 1.0L + LDBL_EPSILON;
  if (!(extra > 1.0L)) {
    return 4;
  }
  if (fpclassify(extra) != FP_NORMAL || signbit(extra)) {
    return 5;
  }
  if (fabsl(-extra) != extra) {
    return 6;
  }
  if (copysignl(2.0L, -1.0L) >= 0.0L) {
    return 7;
  }
  if (ceill(1.25L) != 2.0L || floorl(1.25L) != 1.0L || truncl(-1.75L) != -1.0L) {
    return 8;
  }

  int exponent = 0;
  long double mantissa = frexpl(8.0L, &exponent);
  if (mantissa != 0.5L || exponent != 4) {
    return 9;
  }
  if (ldexpl(0.5L, 4) != 8.0L) {
    return 10;
  }
  if (ilogbl(8.0L) != 3) {
    return 11;
  }

  long double whole = 0;
  long double frac = modfl(-3.25L, &whole);
  if (whole != -3.0L || frac != -0.25L) {
    return 12;
  }
  if (nextafterl(1.0L, 2.0L) <= 1.0L) {
    return 13;
  }

  char printed[64];
  snprintf(printed, sizeof(printed), "%.0Lf", 3.0L);
  if (strcmp(printed, "3.") != 0 && strcmp(printed, "3") != 0 &&
      strcmp(printed, "3.000000") != 0) {
    if (printed[0] != '3') {
      return 14;
    }
  }

  long double scanned = 0;
  int dummy = 0x11111111;
  if (sscanf("9.5", "%Lf", &scanned) != 1 || scanned != 9.5L) {
    return 15;
  }
  (void)dummy;

  extra = strtold("1.0000000000000000001", NULL);
  if (!(extra > 1.0L)) {
    return 16;
  }

  if (expl(0.0L) != 1.0L) {
    return 17;
  }
  if (fabsl(logl(expl(0.5L)) - 0.5L) > 0x1p-40L) {
    return 18;
  }
  {
    long double log_eps = logl(1.0L + LDBL_EPSILON);
    if (!(log_eps > 0.0L) ||
        fabsl(log_eps / LDBL_EPSILON - 1.0L) > 0x1p-20L) {
      return 19;
    }
  }
  if (sinl(0.0L) != 0.0L ||
      fabsl(sinl(LDBL_EPSILON) / LDBL_EPSILON - 1.0L) > 0x1p-20L) {
    return 20;
  }
  if (fabsl(sqrtl(4.0L) - 2.0L) > LDBL_EPSILON) {
    return 21;
  }

  snprintf(printed, sizeof(printed), "%a", 1.0);
  if (strcmp(printed, "0x1p+0") != 0 && strcmp(printed, "0x1.0p+0") != 0) {
    return 22;
  }
  snprintf(printed, sizeof(printed), "%La", 1.0L);
  if (strcmp(printed, "0x1p+0") != 0 && strcmp(printed, "0x1.0p+0") != 0) {
    return 23;
  }
  snprintf(printed, sizeof(printed), "%La", 2.5L);
  if (strcmp(printed, "0x1.4p+1") != 0 && strcmp(printed, "0x1.40p+1") != 0) {
    return 24;
  }
#else
  if (strtold("2.5", NULL) != 2.5L) {
    return 1;
  }
#endif
  puts("ok");
  return 0;
}
