#include <stdio.h>

// Narrowing to an unsigned type keeps the low bits and zero-extends them.
// A signed narrowing of a value with the high bit set is negative.

static unsigned short Indexed(unsigned short base, unsigned char index) {
  return (unsigned short)(base + index);
}

static int WidenUnsigned(unsigned short value) { return value; }

static signed short NarrowUnsigned(unsigned value) {
  return (signed short)value;
}

int main(void) {
  unsigned short sum = Indexed(0xE100, 0x0E);
  signed short negative = NarrowUnsigned(0xE10E);
  if (sum != 0xE10E) return 1;
  if (WidenUnsigned(sum) != 0xE10E) return 2;
  if ((int)(unsigned short)(int)-2 != 65534) return 3;
  if (negative != (signed short)0xE10E) return 4;
  if (negative >= 0) return 5;
  printf("ok\n");
  return 0;
}
