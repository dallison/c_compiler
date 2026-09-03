#include <stdlib.h>

#include <stdio.h>

int strfromd(char* restrict output, size_t count,
             const char* restrict format, double value) {
  return snprintf(output, count, format, value);
}

int strfromf(char* restrict output, size_t count,
             const char* restrict format, float value) {
  return snprintf(output, count, format, (double)value);
}

int strfroml(char* restrict output, size_t count,
             const char* restrict format, long double value) {
  return snprintf(output, count, format, value);
}
