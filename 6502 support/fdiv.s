#include "vars.s"
#include "fp.s"

.text

// Divide 48-bit numbers.  For mantissa division we need to divide a
// 48 bit number with the upper 24 bits populated by another with the
// lower 24 bits populated.
//
// +--------------------+------------------+
// |           a        |         0        |
// +--------------------+------------------+
// Divided by
// +--------------------+------------------+
// |           0        |         b        |
// +--------------------+------------------+
//
// A normal division rotates left 'a' into 'remainder'.
// We can shortcut this a little by noticing that for the first 24 shifts
// we will be putting 'a' into 'remainder' because 'b' cannot be less than
// 'remainder' until we get the top bit set (both numbers are normalized with
// a 1 in the top bit).  So we put 'a' into 'remainder' initially and shift
// 24 times.  We don't need 'a' any more since we've put it in rem.  We also
// need some 'remainder' to be 32 bits wide since we will be shifting it
// left once per
//
// All mantissas (fmanA, fmanB and fmantissa) are 40 bits wide and the
// unpacked mantissa is in bits 32-8.  The top and bottom bytes are zero.
// This subroutine ignores the bottom byte and uses the top byte as overflow.
//
// Don't use t0 or t1 as these are shared by fexp and fsign.
.set remainder fmanA
.set quotient fmantissa
.set divisor fmanB
// Result goes directly into fmantissa.  It will be 24 bits wide but will
// be in the upper 24 bits.
udiv6:
  LDX #24
  BRA udiv61        // First iteration, no shifts.
udiv6_l1:
  // Shift remainder left.  This is a 32 bit shift starting at bit 8.
  ASL remainder+1
  ROL remainder+2
  ROL remainder+3
  ROL remainder+4
  // Shift quotient left.
  ASL quotient+1
  ROL quotient+2
  ROL quotient+3
udiv61:
  // Trial subtraction.  This is a 32 bit subtraction starting at bit 8.
  LDA remainder+1
  SEC
  SBC divisor+1
  TAY
  LDA remainder+2
  SBC divisor+2
  STA remainder+0     // Unused temp location
  LDA remainder+3
  SBC divisor+3
  STA quotient+0      // Unused temp location
  LDA remainder+4
  SBC divisor+4
  BCC udiv6_l2       // Did subtraction succeed?

  STA remainder+4   // If yes, save it
  LDA quotient+0
  STA remainder+3
  LDA remainder+0
  STA remainder+2
  STY remainder+1
  INC quotient+1    // and record a 1 in the quotient
udiv6_l2:
  DEX
  BNE udiv6_l1
  RTS

// 40 bit division.
udiv40:
  LDX #40
  BRA udiv401        // First iteration, no shifts.
udiv40_l1:
  // Shift remainder left.  This is a 40 bit shift starting at bit 0.
  ASL remainder+0
  ROL remainder+1
  ROL remainder+2
  ROL remainder+3
  ROL remainder+4
  // Shift quotient left.
  ASL quotient+0
  ROL quotient+1
  ROL quotient+2
  ROL quotient+3
  ROL quotient+4
udiv401:
  // Trial subtraction.  This is a 40 bit subtraction starting at bit 0.
  LDA remainder+0
  SEC
  SBC divisor+0
  TAY
  LDA remainder+1
  SBC divisor+1
  STA mt1+0     // Unused temp location
  LDA remainder+2
  SBC divisor+2
  STA mt1+1    // Unused temp location
  LDA remainder+3
  SBC divisor+3
  STA mt1+2      // Unused temp location
  LDA remainder+4
  SBC divisor+4
  BCC udiv40_l2       // Did subtraction succeed?

  STA remainder+4   // If yes, save it
  LDA mt1+2
  STA remainder+3
  LDA mt1+1
  STA remainder+2
  LDA mt1+0
  STA remainder+1
  STY remainder+0
  INC quotient+0    // and record a 1 in the quotient
udiv40_l2:
  DEX
  BNE udiv40_l1
  RTS

// Zero result
fdiv_res_0:
  PLX
  JMP __fres0

// NaN result.
fdiv_res_nan:
  PLX
  JMP __fnan

fdiv_res_inf:
  PLX
  JMP __finf

// Either NaN or infinity.  If both A and B are zero then NaN otherwise
// infinity.
fdiv0:
  JSR __fcheckA0
  BEQ fdiv_res_nan
  PLX
  JMP __finf

// Entry:
// A: offset into zero page for result
// X: offset into zero page for A
// Y: offset into zero page for B
//
// Algorithm:
// Subtract exponents, divide mantissas, normalize.
// If A and B are zero then result is Nan
// If B is zero result is infinity.
// If A is zero result is zero.
__fdiv:
  PHA
  JSR __fcheckB0
  BEQ fdiv0        // B is zero - division by zero or nan
  JSR __fcheckA0
  BEQ fdiv_res_0      // B is zero => result is zero.
  JSR __fisnanA
  BCS fdiv_res_nan
  JSR __fisnanB
  BCS fdiv_res_nan
  JSR __fisinfA
  BCS fdiv_res_inf
  JSR __fisinfB
  BCS fdiv_res_inf

  JSR __funpackA
  JSR __funpackB

  // Remove bias from exponents and subtract them.
  // (eA - 127) - (eB - 127) + 127
  // = eA - eB - 127 + 127 + 127
  // = eA - eB + 127
  SEC
  LDA fexpA
  SBC fexpB
  CLC
  ADC #127
  STA fexp

  // Sign of result is the EOR of the two signs.
  LDA fsignA
  EOR fsignB
  STA fsign

  // Zero out result mantissa.  The udiv function will put the quotient here.
  JSR __fzero_mantissa

  // Divide the mantissas.  This is a special 24 bit division putting
  // the result in fmantissa.
  // JSR udiv6
  JSR udiv40

  // If byte 4 of the mantissa is non-zero, continually shift the mantissa
  // right.
fdiv_loop:
  LDA fmantissa+4
  BEQ fdiv_end
  LSR fmantissa+4
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1
  ROR fmantissa+0
  INC fexp
  BRA fdiv_loop

fdiv_end:
  // udiv40 leaves the normalized quotient seven bits farther left than the
  // old 24-bit divider.  Account for that position as well as the division's
  // one-bit normalization adjustment.
  SEC
  LDA fexp
  SBC #8
  STA fexp
  // Normalize result and assemble into destination.
  JSR __fnormalize
  JSR __fround
  PLX
  JMP __fassemble
