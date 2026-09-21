#include <stdio.h>
#include <string.h>

static int has_exponent(const char* s) {
  return strchr(s, 'e') != NULL || strchr(s, 'E') != NULL;
}

int main(void) {
  char buf[64];

  // Default %g (precision 6) must keep the exponent for |x| < 1e-4.
  snprintf(buf, sizeof(buf), "%g", 1.2246467991473532e-16);
  if (!has_exponent(buf) || strcmp(buf, "1.22465") == 0) {
    return 1;
  }

  snprintf(buf, sizeof(buf), "%g", -2.4492935982947064e-16);
  if (buf[0] != '-' || !has_exponent(buf)) {
    return 2;
  }

  // Trailing zeros in the significand are stripped, but the exponent stays.
  snprintf(buf, sizeof(buf), "%.6g", 1.22000e-16);
  if (!has_exponent(buf)) {
    return 3;
  }

  // The %f branch of %g (exponent >= -4) must not invent an exponent.
  snprintf(buf, sizeof(buf), "%g", 0.00122465);
  if (has_exponent(buf)) {
    return 4;
  }
  return 0;
}
