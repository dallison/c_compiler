//
//  modf.s
//  c_compiler
//
//  Created by David Allison on 12/24/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include "vars.s"
#include "fp.s"

.text

.global modf

// Extract the address pointed to by p, 5 bytes up the stack.
// Exit:
// t0,t1: addr
// Y: 3
extract_p_addr:
  LDY #5
  LDA (__sp),Y
  STA __t0+1
  DEY
  LDA (__sp),Y
  STA __t0+0
  DEY
  RTS

// No fractional part.  Store float value in address at sp+4.
// Return +-0.
modf_no_fraction:
  JSR extract_p_addr

  // Y is 3
  // Copy x to integer address.
modf_copy_x:
  LDA (__sp),Y
  STA (__t0),Y
  DEY
  BPL modf_copy_x

  // Return 0 with sign set.  Y is zero.
  TYA
modf_return_0:
  STA (__result),Y
  INY
  CPY #3
  BNE modf_return_0
  LDA fsignA
  STA (__result),Y
  JMP __incsp6

// Entry:
// X,Y: address to store fractional part.
// sp+0: floating value
// sp+4: address to store integral part
modf:
  STX __result
  STY __result+1

  // Put 23 bit mantissa in middle bytes of fmanA.
  STZ fmanA+0
  LDY #0
  LDA (__sp),Y
  STA fmanA+1
  INY
  LDA (__sp),Y
  STA fmanA+2
  INY
  LDA (__sp),Y
  STA fmanA+3
  STZ fmanA+4
  PHA

  INY
  LDA (__sp),Y
  STA fexpA      // Bottom 7 bits of exponent (with sign bit)
  AND #0x80     // Get sign bit.
  STA fsignA

  // Top bit of mantissa is bottom bit of exponent - shift it in.
  PLA
  ASL A     // A contains top byte of mantissa.  MSB is LSB of exponent.
  ROL fexpA

  // Store implicit 1 in bit 31 of mantissa.
  LDA fmanA+3
  ORA #0x80
  STA fmanA+3

  // Look at exponent to see if it's in range.
  LDA fexpA
  CMP #127+24
  BCS modf_no_fraction
  CMP #127
  BCC modf_no_integer
  // There are both integer and fractional parts.

  // Calculate unbiased exponent (carry is set).
  SBC #126    // Account for implicit 1 in upper bit of mantissa.
  TAX

  // Shift mantissa to left, keeping upper bits in mt1.
  STZ mt1+0
  STZ mt1+1
  STZ mt1+2
  STZ mt1+3
modf_shift_loop:
  CPX #0
  BEQ modf_shift_end
  ASL fmanA+1
  ROL fmanA+2
  ROL fmanA+3
  ROL mt1+0
  ROL mt1+1
  ROL mt1+2
  ROL mt1+3
  DEX
  BRA modf_shift_loop
modf_shift_end:
  // mt1 contains the integer part, fmanA contains the fractional
  // part.
  // Convert integer in mt1 to floating point, put result in mt2
  LDA #mt2
  LDX #mt1
  JSR __ui4tof

  JSR extract_p_addr

  // Y is 3
  // Copy mt2 (floating point) to p.
modf_copy_int:
  LDA mt2,Y
  STA (__t0),Y
  DEY
  BPL modf_copy_int

  // Now we need to return the fractional part as a floating point
  // value.  The exponent is -1 (126).  Sign is the same as input.
  LDA #126
  STA fexp
  LDA fsignA
  STA fsign
  LDX #4
modf_copy_mantissa:
  LDA fmanA,X
  STA fmantissa,X
  DEX
  BPL modf_copy_mantissa
  JSR __fnormalize

  LDX #mt1
  JMP __fassemble     // Asssmeble into mt1 as temp

  LDY #3
modf_return_fract:
  LDA mt1,Y
  STA (__result),Y
  DEY
  BPL modf_return_fract
  JMP __incsp6

modf_no_integer:
  // No integral part.  Return x and write +-0 into (sp)+4
  JSR extract_p_addr

  // Y is 3
  // Copy x to result address.
modf_return_x:
  LDA (__sp),Y
  STA (__result),Y
  DEY
  BPL modf_return_x

  // Y = 0
  TYA
modf_copy_0:
  STA (__t0),Y
  INY
  CPY #3
  BNE modf_copy_0
  // Y = 3, copy sign.
  LDA fsignA
  STA (__t0),Y
  JMP __incsp6


