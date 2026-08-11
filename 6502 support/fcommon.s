#include "vars.s"
#include "fp.s"

.text


// Assemble a 32-bit float.
// fexp: 8 bit exponent
// fsign: sign bit (0x80 or 0)
// fmantissa: 40 bits of mantissa with explicit 1 in bit 31.
// X: index into zero page for result.
__fassemble:
  // Exponent and sign are in top byte
  LDA fexp
  LSR A       // Carry contains bottom bit of exponent.
  ORA fsign    // Sign is 0x80 or 0.
  STA 3,X     // Store sign and top 7 bits of exponent.

  // Put bottom bit of exp (in carry) in top bit of __t1
  STZ __t1
  ROR __t1

  // Take bytes 1,2 and 3 from mantissa scratch.
  LDA fmantissa+3
  AND #0x7f           // Remove implicit 1.
  ORA __t1            // OR in bottom bit of exponent.
  STA 2,X
  LDA fmantissa+2
  STA 1,X
  LDA fmantissa+1
  STA 0,X
  RTS

// Entry:
// sp+0: sign
// sp+2: exponent
// sp+4: mantissa (8 bytes).  Take top 3 bytes at offset sp+9
// X: offset into zero page for result
__packIEEE754:
  LDA (__sp)
  STA fsign
  LDY #2
  LDA (__sp),Y
  STA fexp
  LDY #9
  LDA (__sp),Y
  STA fmantissa+1
  INY
  LDA (__sp),Y
  STA fmantissa+2
  INY
  LDA (__sp),Y
  STA fmantissa+3

  // Shift mantissa right by 1 to make room for bottom bit of exponent.
  LSR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1
  JMP __fassemble
  
__fzero:
  PLX
  LDA #0
  STA 0,X
  STA 1,X
  STA 2,X
  STA 3,X
  RTS

__fzero_mantissa:
  STZ fmantissa+0
  STZ fmantissa+1
  STZ fmantissa+2
  STZ fmantissa+3
  STZ fmantissa+4
  RTS

__fmantissa_is_zero:
  LDA fmantissa+4
  ORA fmantissa+3
  ORA fmantissa+2
  ORA fmantissa+1
  ORA fmantissa+0
  RTS


// Input:
//   X: index of float in zero page.
// Output:
//   fexpA: exponent with bias
//   fmanA: middle 3 bytes set to mantissa
//   fsignA: sign (0x80 or 0)
__funpackA:
  LDA 3,X
  STA fexpA      // Bottom 7 bits of exponent (with sign bit)
  AND #0x80      // Get sign bit.
  STA fsignA

  // Put 23 bit mantissa in middle bytes of fmanA.
  STZ fmanA+0
  LDA 0,X
  STA fmanA+1
  LDA 1,X
  STA fmanA+2
  LDA 2,X
  STA fmanA+3
  STZ fmanA+4

  // Top bit of mantissa is bottom bit of exponent - shift it in.
  ASL A     // A contains top byte of mantissa.  MSB is LSB of exponent.
  ROL fexpA

  // Store implicit 1 in bit 31 of mantissa.
  LDA fmanA+3
  ORA #0x80
  STA fmanA+3
  RTS

// Y: index of float in zero page.
__funpackB:
  LDA 3,Y
  STA fexpB      // Bottom 7 bits of exponent (with sign bit)
  AND #0x80     // Get sign bit.
  STA fsignB

  // Put 23 bit mantissa in middle bytes of fmanA.
  STZ fmanB+0
  LDA 0,Y
  STA fmanB+1
  LDA 1,Y
  STA fmanB+2
  LDA 2,Y
  STA fmanB+3
  STZ fmanB+4

  // Top bit of mantissa is bottom bit of exponent - shift it in.
  ASL A     // A contains top byte of mantissa.  MSB is LSB of exponent.
  ROL fexpB

  // Store implicit 1 in bit 31 of mantissa.
  LDA fmanB+3
  ORA #0x80
  STA fmanB+3
  RTS

// Normalize a float mantissa.
// mantissa contains a 40 bit positive value.
// exp is set
//
// If there are bits set in mantissa+4, shift right until it's
// zero and increment exponent for each shift.
//
// If the top bit of mantissa+3 is not set, shift left until
// it is and decrement exponent each shift.
//
// The mantissa must contain a valid encoding - not zero, Nan or infinity.
__fnormalize:
fnorm_right:             // If top byte is non-zero, shift right until 0.
  LDA fmantissa+4
  BEQ fnorm_left
  LSR fmantissa+4
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1
  // fmantissa+0 is beyond the precision at this point.
  INC fexp             // Increment exponent.
  BRA fnorm_right
  RTS
fnorm_left:
  // Shift left until bit 31 is set (implicit 1 bit).
  LDA fmantissa+3
  AND #0x80
  BNE fendnorm
  // fmantissa+4 is not part of the output fmantissa.
  ASL fmantissa+0
  ROL fmantissa+1
  ROL fmantissa+2
  ROL fmantissa+3
  DEC fexp
  BNE fnorm_left
fendnorm:
  RTS

// Round the fmantissa using IEEE754 round to zero even rule.
__fround:
  LDA fmantissa+0
  BIT #0x80     // Lowest byte top bit set?
  BEQ fendnorm  // No, no rounding needed.

  CMP #0x80     // Exactly half?
  BNE round_up

  // Check if remaining mantissa is even
  LDA fmantissa+1
  BIT #0x1
  BNE fendnorm      // Number is odd, no rounding.

round_up:
  CLC
  LDA fmantissa+1
  ADC #1
  STA fmantissa+1
  LDA fmantissa+2
  ADC #0
  STA fmantissa+2
  LDA fmantissa+3
  ADC #0
  STA fmantissa+3
  RTS

// X: offset of A in zero page in IEE754 format.
__fcheckA0:
  LDA 3,X
  AND #0x7f   // Mask off sign bit.
  ORA 0,X
  ORA 1,X
  ORA 2,X
  RTS

// Y: offset of B in zero page in IEE754 format.
__fcheckB0:
  LDA 3,Y
  AND #0x7f   // Mask off sign bit.
  ORA 0,Y
  ORA 1,Y
  ORA 2,Y
  RTS

// Result is B.
// X: dest offset in zero page.
// Y: offset of B in zero page
__fresB:
  LDA #4
  STA __t1
fres_B_loop:
  LDA 0,Y
  STA 0,X
  INX
  INY
  DEC __t1
  BNE fres_B_loop
  RTS

// Result is A.
// X: offset of A in zero page
// Y: dest offset in zero page.
__fresA:
  LDA #4
  STA __t1
fres_A_loop:
  LDA 0,X
  STA 0,Y
  INX
  INY
  DEC __t1
  BNE fres_A_loop
  RTS

__fres0:
  LDY #4
  LDA #0
fres_0_loop:
  STA 0,X
  INX
  DEY
  BNE fres_0_loop
  RTS

// Entry:
// frshift: number of bits to shift
__frshiftB:
  LSR fmanB+4
  ROR fmanB+3
  ROR fmanB+2
  ROR fmanB+1
  ROR fmanB+0
  DEC frshift
  BNE __frshiftB
  RTS

__frshiftA:
  LSR fmanA+4
  ROR fmanA+3
  ROR fmanA+2
  ROR fmanA+1
  ROR fmanA+0
  DEC frshift
  BNE __frshiftA
  RTS

__fnegmantissa:
  SEC
  LDA #0
  SBC fmantissa+0
  STA fmantissa+0
  LDA #0
  SBC fmantissa+1
  STA fmantissa+1
  LDA #0
  SBC fmantissa+2
  STA fmantissa+2
  LDA #0
  SBC fmantissa+3
  STA fmantissa+3
  LDA #0
  SBC fmantissa+4
  STA fmantissa+4
  RTS

// Nan is 0x7fffffff
// Infinity is 0x7f800000

// X: offset into zero page for result.
__fnan:
  LDA #0x7f
  STA 0,X
  LDA #0xff
  STA 1,X
  STA 2,X
  STA 3,X
  RTS

// X: offset into zero page for result.
__finf:
  LDA #0x7f
  STA 0,X
  LDA #0x80
  STA 1,X
  LDA #0
  STA 2,X
  STA 3,X
  RTS

// X: offset into zero page
__fisnanA:
  LDA 3,X
  AND #0x7f
  CMP #0x7f
  BNE notnaninf
  LDA 2,X
  AND #0x80
  BEQ notnaninf
  LDA 2,X
  AND #0x7f
  ORA 1,X
  ORA 0,X
  BEQ notnaninf
  SEC
  RTS
notnaninf:
  CLC
  RTS

// Y: offset into zero page
__fisnanB:
  LDA 3,Y
  AND #0x7f
  CMP #0x7f
  BNE notnaninf
  LDA 2,Y
  AND #0x80
  BEQ notnaninf
  LDA 2,Y
  AND #0x7f
  ORA 1,Y
  ORA 0,Y
  BEQ notnaninf
  SEC
  RTS


// X: offset into zero page
__fisinfA:
  LDA 3,X
  AND #0x7f
  CMP #0x7f
  BNE notnaninf
  LDA 2,X
  AND #0x80
  BEQ notnaninf
  LDA 2,X
  AND #0x7f
  ORA 1,X
  ORA 0,X
  BNE notnaninf
  SEC
  RTS


// Y: offset into zero page
__fisinfB:
  LDA 3,Y
  AND #0x7f
  CMP #0x7f
  BNE notnaninf
  LDA 2,Y
  AND #0x80
  BEQ notnaninf
  LDA 2,Y
  AND #0x7f
  ORA 1,Y
  ORA 0,Y
  BNE notnaninf
  SEC
  RTS

// Entry:
// sp+0: address of IEE754 single precision number
// sp+2: address of stuct with the following fields:
// uint8_t sign;
// uint8_t exp;
// uint32_t mantissa;

__unpackIEEE754:
  // Load fp address into t0,t1
  LDA (__sp)
  STA __t0
  LDY #1
  LDA (__sp),Y
  STA __t1
  INY

  // Load dest address into t2,t3
  LDA (__sp), Y
  STA __t2
  INY
  LDA (__sp),Y
  STA __t3

  // Load sign and part of exponent.
  // Y is 3.
  LDA (__t0), Y
  STA fexpA
  AND #128
  STA (__t2)        // Store sign in result.

  // Copy mantissa into fmanA.
  // Mantissa is 24 bits long in IEEE754.  The output we want
  // is in the top bits of a uint32_t
  LDY #0
  LDX #1
unpack752_man_loop:
  LDA (__t0),Y
  STA fmanA,X
  INX
  INY
  CPY #3
  BNE unpack752_man_loop
  STZ fmanA+0

  // Top bit of A is the bottom bit of the exponent in fexpA
  ASL A
  ROL fexpA

  // Store implicit 1 in bit 31 of mantissa.
  LDA fmanA+3
  ORA #0x80
  STA fmanA+3

  // Store exponent.
  LDY #1
  LDA fexpA
  STA (__t2),Y

  INY
  LDX #0
unpack752_loop:
  LDA fmanA,X
  STA (__t2),Y
  INY
  INX
  CPX #4
  BNE unpack752_loop
  RTS

  

