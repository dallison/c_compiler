#include <inttypes.h>

#include <stdlib.h>

intmax_t imaxabs(intmax_t value) {
  return value < 0 ? -value : value;
}

imaxdiv_t imaxdiv(intmax_t numerator, intmax_t denominator) {
  imaxdiv_t result;
  result.quot = numerator / denominator;
  result.rem = numerator % denominator;
  return result;
}

intmax_t strtoimax(const char* restrict string, char** restrict end, int base) {
  return (intmax_t)strtoll(string, end, base);
}

uintmax_t strtoumax(const char* restrict string, char** restrict end, int base) {
  return (uintmax_t)strtoull(string, end, base);
}
