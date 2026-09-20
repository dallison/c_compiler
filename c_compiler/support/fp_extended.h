//
//  fp_extended.h
//
//  Software IEEE extended formats used for DaveCC long double:
//  IEEE binary128 (AArch64, RISC-V 64) and Intel 80-bit (x86_64).
//  Arithmetic is implemented in binary128 and rounded to 80-bit when needed.
//

#ifndef davecc_fp_extended_h
#define davecc_fp_extended_h

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  kFPExtFormatFloat32 = 0,
  kFPExtFormatFloat64 = 1,
  kFPExtFormatIntel80 = 2,
  kFPExtFormatIEEEf128 = 3,
};

typedef struct {
  uint64_t lo;
  uint64_t hi;
} FPBits;

FPBits FPBitsFromF32(float value, int format);
FPBits FPBitsFromF64(double value, int format);
FPBits FPBitsFromI64(int64_t value, int format);
FPBits FPBitsFromU64(uint64_t value, int format);

float FPBitsToF32(FPBits bits, int format);
double FPBitsToF64(FPBits bits, int format);
int64_t FPBitsToI64(FPBits bits, int format);

FPBits FPAdd(FPBits a, FPBits b, int format);
FPBits FPSub(FPBits a, FPBits b, int format);
FPBits FPMul(FPBits a, FPBits b, int format);
FPBits FPDiv(FPBits a, FPBits b, int format);
FPBits FPNeg(FPBits a, int format);

int FPCompare(FPBits a, FPBits b, int format);
int FPIsNaN(FPBits bits, int format);
int FPIsInf(FPBits bits, int format);
int FPIsZero(FPBits bits, int format);
int FPSignBit(FPBits bits, int format);
int FPClassify(FPBits bits, int format);
int FPIlogb(FPBits bits, int format);

FPBits FPAbs(FPBits bits, int format);
FPBits FPCopySign(FPBits magnitude, FPBits sign, int format);
FPBits FPFrexp(FPBits bits, int* exponent, int format);
FPBits FPLdexp(FPBits bits, int exponent, int format);
FPBits FPTrunc(FPBits bits, int format);
FPBits FPFloor(FPBits bits, int format);
FPBits FPCeil(FPBits bits, int format);
FPBits FPModf(FPBits bits, FPBits* integer, int format);
FPBits FPNextAfter(FPBits from, FPBits to, int format);

#ifdef __cplusplus
}
#endif

#endif
