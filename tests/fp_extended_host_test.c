#include "../c_compiler/support/fp_extended.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>

static int failures;

static void Expect(int cond, const char* msg) {
  if (!cond) {
    printf("FAIL: %s\n", msg);
    failures++;
  }
}

static void TestF128(void) {
  int fmt = kFPExtFormatIEEEf128;
  FPBits one = FPBitsFromF64(1.0, fmt);
  FPBits two = FPBitsFromF64(2.0, fmt);
  FPBits half = FPBitsFromF64(0.5, fmt);
  FPBits three = FPAdd(one, two, fmt);
  Expect(FPBitsToF64(three, fmt) == 3.0, "1+2==3 f128");
  Expect(FPBitsToF64(FPSub(two, one, fmt), fmt) == 1.0, "2-1==1 f128");
  Expect(FPBitsToF64(FPMul(two, two, fmt), fmt) == 4.0, "2*2==4 f128");
  Expect(FPBitsToF64(FPDiv(one, two, fmt), fmt) == 0.5, "1/2==0.5 f128");
  Expect(FPCompare(one, two, fmt) < 0, "1<2 f128");
  Expect(FPCompare(two, one, fmt) > 0, "2>1 f128");
  Expect(FPCompare(one, one, fmt) == 0, "1==1 f128");
  Expect(FPBitsToF64(FPNeg(one, fmt), fmt) == -1.0, "neg 1 f128");
  Expect(FPBitsToI64(FPBitsFromI64(-42, fmt), fmt) == -42, "i64 roundtrip");
  Expect(FPBitsToF64(FPBitsFromF64(3.141592653589793, fmt), fmt) ==
             3.141592653589793,
         "pi roundtrip f64->f128->f64");
  FPBits small = FPAdd(one, FPBitsFromF64(0x1p-60, fmt), fmt);
  Expect(FPCompare(small, one, fmt) > 0, "1+2^-60 > 1 in f128");
  Expect(FPBitsToF64(FPAdd(half, half, fmt), fmt) == 1.0, "0.5+0.5");
  Expect(FPBitsToF64(FPAdd(FPBitsFromF64(-2.0, fmt), one, fmt), fmt) == -1.0,
         "-2+1 f128");
  Expect(FPBitsToF64(FPMul(FPBitsFromF64(-3.0, fmt), two, fmt), fmt) == -6.0,
         "-3*2 f128");
  Expect(FPBitsToF64(FPDiv(FPBitsFromF64(9.0, fmt),
                            FPBitsFromF64(3.0, fmt), fmt),
                      fmt) == 3.0,
         "9/3 f128");
}

static void TestF80(void) {
  int fmt = kFPExtFormatIntel80;
  FPBits one = FPBitsFromF64(1.0, fmt);
  FPBits two = FPBitsFromF64(2.0, fmt);
  Expect(FPBitsToF64(FPAdd(one, two, fmt), fmt) == 3.0, "1+2==3 f80");
  Expect(FPBitsToF64(FPMul(two, two, fmt), fmt) == 4.0, "2*2==4 f80");
  Expect(FPBitsToF64(FPDiv(one, two, fmt), fmt) == 0.5, "1/2==0.5 f80");
  Expect(one.lo == 0x8000000000000000ULL, "f80 1.0 significand");
  Expect((one.hi & 0xffff) == 0x3fff, "f80 1.0 exponent");
  FPBits small = FPAdd(one, FPBitsFromF64(0x1p-60, fmt), fmt);
  Expect(FPCompare(small, one, fmt) > 0, "1+2^-60 > 1 in f80");
}

int main(void) {
  TestF128();
  TestF80();
  if (failures) {
    printf("%d failures\n", failures);
    return 1;
  }
  printf("ok\n");
  return 0;
}
