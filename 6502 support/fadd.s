#include "vars.s"
#include "fp.s"

// Single precision floating point addition and subtraction.
.section ".text.__fadd", "ax", @progbits

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
  // Default the result sign to positive.  fsign is only set explicitly on the
  // negative-result path below, but __fassemble always ORs it into the result.
  // fsign aliases __t0, which __argaddr clobbers whenever a function reads a
  // parameter, so it must be initialized here rather than relied upon to be 0.
  STZ fsign
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
  // Negate the numbers is sign is set.
  LDA fsignA
  BPL fadd_a_pos
  SEC
  LDA #0
  SBC fmanA+0
  STA fmanA+0
  LDA #0
  SBC fmanA+1
  STA fmanA+1
  LDA #0
  SBC fmanA+2
  STA fmanA+2
  LDA #0
  SBC fmanA+3
  STA fmanA+3
  LDA #0
  SBC fmanA+4
  STA fmanA+4

fadd_a_pos:
  LDA fsignB
  BPL fadd_b_pos
  SEC
  LDA #0
  SBC fmanB+0
  STA fmanB+0
  LDA #0
  SBC fmanB+1
  STA fmanB+1
  LDA #0
  SBC fmanB+2
  STA fmanB+2
  LDA #0
  SBC fmanB+3
  STA fmanB+3
  LDA #0
  SBC fmanB+4
  STA fmanB+4

fadd_b_pos:
  // Add the mantissas.
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
  LDA #0x80
  STA fsign
  JSR __fnegmantissa

fadd_pos_result:
  // Check for zero result.
  JSR __fmantissa_is_zero
  BEQ fadd_res_0

  JSR __fnormalize
  JSR __fround
  PLX
  JMP __fassemble


// Result is zero.
fadd_res_0:
  PLX
  LDA #0
  STA 0,X
  STA 1,X
  STA 2,X
  STA 3,X
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
