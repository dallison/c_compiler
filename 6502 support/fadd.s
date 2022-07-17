#include "vars.s"
#include "fp.s"

// Single precision floating point addition and subtraction.
.text

fadd_res_B:
  PLX
  JMP __fresB

fadd_res_A:
  PLY
  JMP __fresA

fadd_res_nan:
  PLX
  JMP __fnan

fadd_res_inf:
  PLX
  JMP __finf

// Add two 32-bit floats
// Entry:
// A: offset of result in zero page
// X,Y: offsets of inputs
// Algorithm:
// Shift the mantissa of the smaller exponent right to make exponents
// the same.  Add the mantissas.  If carry is set from addition of top
// word, we have to normalize by shifting right by 1 and increment exponent.
__fadd:
  PHA
  JSR __fcheckA0
  BEQ fadd_res_B
  JSR __fcheckB0
  BEQ fadd_res_A
  JSR __fisnanA
  BCS fadd_res_nan
  JSR __fisnanB
  BCS fadd_res_nan
  JSR __fisinfA
  BCS fadd_res_inf
  JSR __fisinfB
  BCS fadd_res_inf

  JSR __funpackA
  JSR __funpackB
add_common:
  SEC
  LDA fexpB
  STA fexp
  SBC fexpA            // A is RHS - LHS.
  STA frshift
  BEQ fadd_same_exp    // Exponents the same.
  BPL fadd_A_smaller     // A is less than B

  // B is smaller than A
  LDA fexpA
  STA fexp
  SEC
  LDA #0
  SBC frshift   // rshift = -rshift
  STA frshift

  // Shift fmanB right by diff bits.
  JSR __frshiftB
  BRA fadd_same_exp

fadd_A_smaller:
  LDA fexpB
  STA fexp          // Exponent is A
  // Shift fmanA right by rshift bits.
  JSR __frshiftA

// Exponents are the same.  Add mantissas, putting result in fmantissa
fadd_same_exp:
  LDA fsignA
  STA fsign
  EOR fsignB
  BEQ fadd_same_sign

  // Signs are different, subtract mantissas.
  SEC
  LDA fmanA+0
  SBC fmanB+0
  STA fmantissa+0
  LDA fmanA+1
  SBC fmanB+1
  STA fmantissa+1
  LDA fmanA+2
  SBC fmanB+2
  STA fmantissa+2
  LDA fmanA+3
  SBC fmanB+3
  STA fmantissa+3
  LDA fmanA+4
  SBC fmanB+4
  STA fmantissa+4
  BRA fadd_rpos

fadd_same_sign:
  CLC
  LDA fmanA+0
  ADC fmanB+0
  STA fmantissa+0
  LDA fmanA+1
  ADC fmanB+1
  STA fmantissa+1
  LDA fmanA+2
  ADC fmanB+2
  STA fmantissa+2
  LDA fmanA+3
  ADC fmanB+3
  STA fmantissa+3
  LDA fmanA+4
  ADC fmanB+4
  STA fmantissa+4

fadd_rpos:
  BPL fadd_pos_result
  // Mantissa is negative.
  LDA fsign
  EOR #0x80
  STA fsign
  JSR __fnegmantissa

fadd_pos_result:
  // Check for zero result.
  JSR __fmantissa_is_zero
  BEQ fadd_res_0

  JSR __fnormalize
  PLX
  JMP __fassemble


// Result is zero.
fadd_res_0:
  PLY
  LDA #0
  LDX #4
fadd_res0_loop:
  STA 0,X
  DEY
  BNE fadd_res0_loop
  RTS

// A is zero.  Result is 0 - B or -B.
fsub_res_B:
  TYA         // B has offset in Y.
  TAX         // __fneg needs it in X.
  PLA
  JMP __fneg

// B is zero, result is in A.
fsub_res_A:
  PLY
  JMP __fresA

fsub_res_nan:
  PLX
  JMP __fnan

fsub_res_inf:
  PLX
  JMP __finf


// Subtraction negates B and adds.
__fsub:
  PHA
  JSR __fcheckA0
  BEQ fsub_res_B
  JSR __fcheckB0
  BEQ fsub_res_A
  JSR __fisnanA
  BCS fsub_res_nan
  JSR __fisnanB
  BCS fsub_res_nan
  JSR __fisinfA
  BCS fsub_res_inf
  JSR __fisinfB
  BCS fsub_res_inf

  JSR __funpackA
  JSR __funpackB

  // Negate B.
  LDA fsignB
  EOR #0x80
  STA fsignB
  JMP add_common
