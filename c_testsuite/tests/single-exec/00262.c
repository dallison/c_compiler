// A switch compares the whole promoted control value, however small the case
// constants are.
#include <stdio.h>

enum Rep { kRep8 = 0, kRep32 = 0x80, kRep64 = 0xc0, kRepMask = 0x1c0 };

static int Mask(unsigned short card) {
  switch (card & kRepMask) {
    case kRep8: return 8;
    case kRep32: return 32;
    case kRep64: return 64;
    default: return -1;
  }
}

static int Byte(int x) {
  switch (x) {
    case 1: return 1;
    case 200: return 200;
    default: return 0;
  }
}

static int Wide(long long x) {
  switch (x) {
    case 2: return 2;
    case 0x8000: return 3;
    default: return 0;
  }
}

static int Enum(enum Rep rep) {
  switch (rep) {
    case kRep8: return 8;
    case kRep32: return 32;
    case kRep64: return 64;
    case kRepMask: return 0;
  }
  return -1;
}

int main(void) {
  if (Mask(0x80) != 32 || Mask(0xc3) != 64 || Mask(0x7) != 8 ||
      Mask(0x100) != -1) {
    return 1;
  }
  if (Byte(1) != 1 || Byte(200) != 200 || Byte(0x101) != 0 ||
      Byte(-56) != 0 || Byte(456) != 0) {
    return 2;
  }
  if (Wide(2) != 2 || Wide(0x100000002LL) != 0 || Wide(0x8000) != 3 ||
      Wide(0x18000) != 0) {
    return 3;
  }
  if (Enum(kRep32) != 32 || Enum(kRep64) != 64) {
    return 4;
  }
  printf("ok\n");
  return 0;
}
