//
//  longlongdiv.s
//  c_compiler
//
//  Created by David Allison on 12/7/21.
//  Copyright © 2021 David Allison. All rights reserved.
//
#include "vars.s"
.text

.global __sdiv8
.global __udiv8

.global __smod8
.global __umod8

.global __cdivmod8

// Division routines:
.set dividend mt2
.set divisor mt3
.set remainder mt1
.set quotient mt2

negate_dividend:
SEC
LDA #0
SBC dividend
STA dividend
LDA #0
SBC dividend+1
STA dividend+1
LDA #0
SBC dividend+2
STA dividend+2
LDA #0
SBC dividend+3
STA dividend+3
LDA #0
SBC dividend+4
STA dividend+4
LDA #0
SBC dividend+5
STA dividend+5
LDA #0
SBC dividend+6
STA dividend+6
LDA #0
SBC dividend+7
STA dividend+7
  RTS

negate_divisor:
SEC
LDA #0
SBC divisor
STA divisor
LDA #0
SBC divisor+1
STA divisor+1
LDA #0
SBC divisor+2
STA divisor+2
LDA #0
SBC divisor+3
STA divisor+3
LDA #0
SBC divisor+4
STA divisor+4
LDA #0
SBC divisor+5
STA divisor+5
LDA #0
SBC divisor+6
STA divisor+6
LDA #0
SBC divisor+7
STA divisor+7
RTS

negate_result:
SEC
LDA #0
SBC 0,X
STA 0,X
LDA #0
SBC 1,X
STA 1,X
LDA #0
SBC 2,X
STA 2,X
LDA #0
SBC 3,X
STA 3,X
LDA #0
SBC 4,X
STA 4,X
LDA #0
SBC 5,X
STA 5,X
LDA #0
SBC 6,X
STA 6,X
LDA #0
SBC 7,X
STA 7,X
RTS

__sdiv8:
  STA __t0
  LDA 0,X
  STA dividend
  LDA 1,X
  STA dividend+1
  LDA 2,X
  STA dividend+2
  LDA 3,X
  STA dividend+3
LDA 3,X
STA dividend+3
LDA 4,X
STA dividend+4
LDA 5,X
STA dividend+5
LDA 6,X
STA dividend+6
LDA 7,X
STA dividend+7

LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1
  LDA 2,Y
  STA divisor+2
  LDA 3,Y
  STA divisor+3
LDA 4,Y
STA divisor+4
LDA 5,Y
STA divisor+5
LDA 6,Y
STA divisor+6
LDA 7,Y
STA divisor+7
  EOR dividend+7
  AND #0x80
  STA __t1

  // Divide magnitudes, then apply the quotient sign. Keep the destination in
  // scratch rather than on the hardware stack: udiv8 is called with JSR and
  // therefore cannot pop a caller byte from that stack.
  LDA dividend+7
  BPL sdiv8_dividend_positive
  JSR negate_dividend
sdiv8_dividend_positive:
  LDA divisor+7
  BPL sdiv8_divisor_positive
  JSR negate_divisor
sdiv8_divisor_positive:
  JSR udiv8

  LDX __t0
  LDA quotient
  STA 0,X
  LDA quotient+1
  STA 1,X
  LDA quotient+2
  STA 2,X
  LDA quotient+3
  STA 3,X
  LDA quotient+4
  STA 4,X
  LDA quotient+5
  STA 5,X
  LDA quotient+6
  STA 6,X
  LDA quotient+7
  STA 7,X
  LDA __t1
  BPL sdiv8_done
  JMP negate_result
sdiv8_done:
  RTS

__udiv8:
  PHA
  LDA 0,X
  STA dividend
  LDA 1,X
  STA dividend+1
  LDA 2,X
  STA dividend+2
  LDA 3,X
  STA dividend+3
LDA 4,X
STA dividend+4
LDA 5,X
STA dividend+5
LDA 6,X
STA dividend+6
LDA 7,X
STA dividend+7

  LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1
  LDA 2,Y
  STA divisor+2
  LDA 3,Y
  STA divisor+3
LDA 4,Y
STA divisor+4
LDA 5,Y
STA divisor+5
LDA 6,Y
STA divisor+6
LDA 7,Y
STA divisor+7

udiv8_1:
  JSR udiv8

  // Store result.
  PLX
  LDA quotient
  STA 0,X
  LDA quotient+1
  STA 1,X
  LDA quotient+2
  STA 2,X
  LDA quotient+3
  STA 3,X
LDA quotient+4
STA 4,X
LDA quotient+5
STA 5,X
LDA quotient+6
STA 6,X
LDA quotient+7
STA 7,X
  RTS

// Main udiv4 routine.  Produces both remainder and quotent
udiv8:
        // Zero out remainder.
        STZ remainder
        STZ remainder+1
        STZ remainder+2
STZ remainder+3
STZ remainder+4
STZ remainder+5
STZ remainder+6
STZ remainder+7

        LDX #64     // There are 32 bits in NUM1
udiv8_l1:
        ASL dividend    // Shift hi bit of divisor into remainder
        ROL dividend+1
ROL dividend+2
ROL dividend+3
ROL dividend+4
ROL dividend+5
ROL dividend+6
        ROL dividend+7   // (vacating the lo bit, which will be used for the quotient)
        ROL remainder
        ROL remainder+1
        ROL remainder+2
ROL remainder+3
ROL remainder+4
ROL remainder+5
ROL remainder+6
ROL remainder+7
        LDA remainder
        SEC         // Trial subtraction
        SBC divisor
        TAY
        LDA remainder+1
        SBC divisor+1
        STA remainder+8     // tmp
        LDA remainder+2
        SBC divisor+2
        STA remainder+9
LDA remainder+3
SBC divisor+3
STA remainder+10
LDA remainder+4
SBC divisor+4
STA remainder+11
LDA remainder+5
SBC divisor+5
STA remainder+12
LDA remainder+6
SBC divisor+6
STA remainder+13

        LDA remainder+7
        SBC divisor+7
        BCC udiv8_l2       // Did subtraction succeed?
        STA remainder+7   // If yes, save it
LDA remainder+13
STA remainder+6
LDA remainder+12
STA remainder+5
LDA remainder+11
STA remainder+4
LDA remainder+10
STA remainder+3
LDA remainder+9
STA remainder+2
LDA remainder+8
STA remainder+1
        STY remainder
        INC dividend    // and record a 1 in the quotient
udiv8_l2:
        DEX
        BNE udiv8_l1
        RTS

__smod8:
  PHA
  LDA 0,X
  STA dividend
  LDA 1,X
  STA dividend+1
  LDA 2,X
  STA dividend+2
LDA 3,X
STA dividend+3
LDA 4,X
STA dividend+4
LDA 5,X
STA dividend+5
LDA 6,X
STA dividend+6
LDA 7,X
STA dividend+7

  LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1
  LDA 2,Y
  STA divisor+2
  LDA 3,Y
  STA divisor+3
LDA 4,Y
STA divisor+4
LDA 5,Y
STA divisor+5
LDA 6,Y
STA divisor+6
LDA 7,Y
STA divisor+7
  EOR dividend+7
  BPL umod8_1

  // One of divisor or dividend is negative.  Result will be negative.
  LDA divisor+7
  BPL smod8_l1

  // Divisor is negative, negate it.
  JSR negate_divisor
  BRA smod8_l2

smod8_l1:
  // Dividend is negative, negate it.
  JSR negate_dividend

smod8_l2:
  // Perform unsigned divide
  PLA
  JSR umod8_1

  // Negate result.
  JMP negate_result
 
__umod8:
  PHA
  LDA 0,X
  STA dividend
  LDA 1,X
  STA dividend+1
  LDA 2,X
  STA dividend+2
  LDA 3,X
  STA dividend+3
LDA 3,X
STA dividend+3
LDA 4,X
STA dividend+4
LDA 5,X
STA dividend+5
LDA 6,X
STA dividend+6
LDA 7,X
STA dividend+7
LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1
  LDA 2,Y
  STA divisor+2
  LDA 3,Y
  STA divisor+3
LDA 4,Y
STA divisor+4
LDA 5,Y
STA divisor+5
LDA 6,Y
STA divisor+6
LDA 7,Y
STA divisor+7

umod8_1:
  JSR udiv8

  // Store remainder as result.
  PLX
  LDA remainder
  STA 0,X
  LDA remainder+1
  STA 1,X
  LDA remainder+2
  STA 2,X
  LDA remainder+3
  STA 3,X
LDA remainder+4
STA 4,X
LDA remainder+5
STA 5,X
LDA remainder+6
STA 6,X
LDA remainder+7
STA 7,X
 RTS
 

// Entry from C:
// sp+0,1: result address
// sp+2-9: numerator
// sp+10-17: demoninator
__cdivmod8:
  LDX #4              // Minimum frame.
  JSR __enter_leaf
  // X regs start at bit 13 in save mask.
  // To save 2 X regs:
  //              12
  // 0000 0000 0100 0000 0000 0000 = 0x002000
  .byte 0,0x40,0         // Save x1 and x2.

  LDA #__x1           // Numerator in i1
  LDX #2
  JSR __arg_value8

  LDA #__x2           // Denominator in i2
  LDX #10
  JSR __arg_value8

  LDA #__x0     // Not used.
  LDX #__x1
  LDY #__x2
  JSR __sdiv8

  LDA #__i0           // Result in i0
  LDX #0
  JSR __arg_value2

  // Quotient in first 8 bytes of result.
  LDY #7
qloop:
  LDA quotient,Y
  STA (__i0),Y
  DEY
  BPL qloop

  LDY #15
  LDX #7
rloop:
  LDA remainder,X
  STA (__i0),Y
  DEY
  DEX
  BPL rloop

  LDY #7      // Includes reg mask.
  JMP __leave_leaf
