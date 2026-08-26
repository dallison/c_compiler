#include <stdio.h>

// A conditional expression between two object pointers gets a freshly built
// composite pointer type.  The pointee used to be released once too often,
// which handed back a pointee node still linked into that pointer, so a later
// use of the type read a recycled type record.

int main(void) {
  int values[2] = { 3, 4 };
  int (*pointer)[2] = &values;
  int condition = 1;

  __typeof__(condition ? pointer : pointer) same_pointer = &values;
  if ((*same_pointer)[0] != 3 || (*same_pointer)[1] != 4) return 1;

  __typeof__(*(condition ? pointer : pointer)) copy = { 5, 6 };
  if (sizeof(copy) != sizeof(int) * 2) return 2;
  if (copy[0] != 5 || copy[1] != 6) return 3;

  int scalar = 7;
  int *scalar_pointer = &scalar;
  __typeof__(condition ? scalar_pointer : scalar_pointer) same_scalar_pointer =
      &scalar;
  if (*same_scalar_pointer != 7) return 4;

  puts("ok");
  return 0;
}
