#include <stdio.h>
#include <limits.h>

enum {
  enum_conditional = 0 ? (1 / 0) : 12,
  enum_logical = (0 && (1 / 0)) || (1 && 1),
  enum_cast = (unsigned char)-1,
};

static unsigned long max_int = (unsigned long)-1 / sizeof(int);
static unsigned short ushort_wrap = (unsigned short)-1;
static signed char schar_wrap = (signed char)255;
static int short_and = 0 && (1 / 0);
static int short_or = 1 || (1 / 0);
static int unsigned_equal = (unsigned)-1 == -1;
static unsigned mixed_unsigned_div = -1 / 2U;
static unsigned mixed_unsigned_mod = -1 % 2U;
static int float_less = 1.25 < 2.5;
static int float_noteq = 1.5 != 1.0;
static double float_fold = (1.5 + 0.25) * 2.0;

int main(void) {
  int array_bound[(enum_conditional == 12 && enum_cast == 255) ? 1 : -1];
  array_bound[0] = enum_logical;

  if (max_int != ULONG_MAX / sizeof(int)) {
    return 1;
  }
  if (ushort_wrap != 65535) {
    return 2;
  }
  if (schar_wrap != -1) {
    return 3;
  }
  if (short_and != 0 || short_or != 1) {
    return 4;
  }
  if (!unsigned_equal) {
    return 5;
  }
  if (mixed_unsigned_div != 2147483647U || mixed_unsigned_mod != 1U) {
    return 6;
  }
  if (!float_less || !float_noteq) {
    return 7;
  }
  if (float_fold < 3.4 || float_fold > 3.6) {
    return 8;
  }

  switch (enum_conditional) {
    case 12:
      printf("constant evaluator ok %d\n", array_bound[0]);
      return 0;
    default:
      return 9;
  }
}
