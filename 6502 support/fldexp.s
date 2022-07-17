//
//  fldexp.s
//  c_compiler
//
//  Created by David Allison on 12/26/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include "vars.s"
#include "fp.s"

.text

.global ldexp

// Entry:
// X,Y: address to store result.
// sp+4: floating value
// sp+6: exponent
ldexp:
  STX __result+0
  STY __result+1
  LDX #__sp+4
  JSR __checkA0
  BEQ ldexp0
  JSR __fisnanA
  BCS ldexp_res_nan
  JSR __fisinfA
  BCS ldexp_res_inf

  JSR __funpackA
  LDY #7
  LDA (__sp),Y        // If more than 256 definitely infinity.
  BNE ldexp_res_inf
  DEY
  CLC
  LDA (__sp), Y
  ADC fexpA
  BCS ldexp_res_inf
  STA fexp
  LDA fsignA
  STA fsign
  JSR __fzeromantissa
  LDA fmanA+1
  STA fmantissa+1
  LDA fmanA+2
  STA fmantissa+2
  LDA fmanA+3
  STA fmantissa+3

  // Assemble into mt1
  LDX #mt1
  JSR __fassemble
  BRA ldexp_ret
  
ldexp0:
  LDX #mt1
  JSR __fres0
  BRA ldexp_ret
ldexp_res_nan:
  LDX #mt1
  JSR __fnan
  BRA ldexp_ret
ldexp_res_inf:
  LDX #mt1
  JSR __finf

ldexp_ret:
  LDY #3
ldexp_ret_loop:
  LDA mt1,Y
  STA (__result),Y
  DEY
  BPL ldexp_ret_loop
  RTS
  
