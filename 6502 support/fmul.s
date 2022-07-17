#include "vars.s"
#include "fp.s"

.text

// We do a 32 bit mutliplication and take the top 32 bits for the result.
// The inputs are fmanA and fmanB and the output is fmantissa.  The 64
// bit result is divided in two halves with the bottom half in math temp 1
// (mt1) and the upper half in the destination fmantissa.
.set multiplier fmanA
.set multiplicand fmanB
.set product_lo mt1
.set product_hi fmantissa

manmul:
  LDA #0       // Initialize product to 0 (this is top byte of product_hi)
  STA product_hi+2
  STA product_hi+1
  STA product_hi+0
  LDX #32      // 32 iterations.
manmul_l1:
  LSR multiplier+3
  ROR multiplier+2
  ROR multiplier+1
  ROR multiplier
  BCC manmul_l2       // 0 or 1?
  TAY          // If 1, add NUM1 (hi byte of RESULT is in A)
  CLC
  LDA multiplicand
  ADC product_hi+0
  STA product_hi+0
  LDA multiplicand+1
  ADC product_hi+2
  STA product_hi+1
  LDA multiplicand+2
  ADC product_hi+2
  STA product_hi+2
  TYA
  ADC multiplicand+3
manmul_l2:
  ROR A        // "Stairstep" shift
  ROR product_hi+2
  ROR product_hi+1
  ROR product_hi+0
  ROR product_lo+3
  ROR product_lo+2
  ROR product_lo+1
  ROR product_lo+0
  DEX
  BNE manmul_l1
  STA product_hi+3
  RTS



// Entry:
// A: offset into zero page for result
// X: offset into zero page for A
// Y: offset into zero page for B
//
// Algorithm:
// Add exponents, multiply mantissas, normalize.
fmul_res_0:
  PLX
  JMP __fres0

fmul_res_nan:
  PLX
  JMP __fnan

fmul_res_inf:
  PLX
  JMP __finf

__fmul:
  PHA
  JSR __fcheckA0
  BEQ fmul_res_0      // A is zero => result is zero.
  JSR __fcheckB0
  BEQ fmul_res_0      // B is zero => result is zero.
  JSR __fisnanA
  BCS fmul_res_nan
  JSR __fisnanB
  BCS fmul_res_nan
  JSR __fisinfA
  BCS fmul_res_inf
  JSR __fisinfB
  BCS fmul_res_inf

  JSR __funpackA
  JSR __funpackB

  // Remove bias from exponents and add them.
  // (eA - 127) + (eB - 127) + 127
  // = eA + eB - 127 - 127 + 127
  // = eA + eB - 127
  CLC
  LDA fexpA
  ADC fexpB
  SEC
  SBC #127 
  STA fexp

  // Sign of result is the EOR of the two signs.
  LDA fsignA
  EOR fsignB
  STA fsign

  // Multiply mantissas putting result in fmantissa
  JSR manmul

  // We have 2 bits to the left of the binary point, so move the point to
  // the left by incrementing the exponent.
  INC fexp

  // Normalize result and assemble into destination.
  JSR __fnormalize
  PLX
  JMP __fassemble


