// The operand of unary ~ and + undergoes the integer promotions, so ~ on an
// unsigned char yields a negative int and sizeof sees int.
#include <stdio.h>

int main(void) {
  unsigned char c = 1;
  short s = 1;
  _Bool b = 1;
  int complement = ~c;
  if (complement != -2) return 1;
  if (sizeof(~c) != sizeof(int)) return 2;
  if (sizeof(+s) != sizeof(int)) return 3;
  if (sizeof(+c) != sizeof(int)) return 4;
  if (_Generic(~s, int: 1, default: 0) != 1) return 5;
  if (_Generic(+b, int: 1, default: 0) != 1) return 6;
  if (~(unsigned char)0 != -1) return 7;
  printf("ok\n");
  return 0;
}
