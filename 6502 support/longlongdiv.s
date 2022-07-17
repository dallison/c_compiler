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

__sdiv8:
  PHA
  LDA 0,X
  STA dividend
  LDA 1,X
  STA dividend+1
  LDA 2,X
  STA dividend+2
  LDA 3,X
  STA dividend+3
  LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1
  LDA 2,Y
  STA divisor+2
  LDA 2,Y
  STA divisor+3
  EOR dividend+3
  BPL udiv8_1

  // One of divisor or dividend is negative.  Result will be negative.
  LDA divisor+3
  BPL sdiv8_l1

  // Divisor is negative, negate it.
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
  BRA sdiv8_l2

sdiv8_l1:
  // Dividend is negative, negate it.
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

sdiv8_l2:
  // Perform unsigned divide
  PLA
  JSR udiv8

  // Negate result.
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
  LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1
  LDA 2,Y
  STA divisor+2
  LDA 3,Y
  STA divisor+3

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
  RTS

// Main udiv4 routine.  Produces both remainder and quotent
udiv8:
        // Zero out remainder.
        STZ remainder
        STZ remainder+1
        STZ remainder+2
        STZ remainder+3
        LDX #32     // There are 32 bits in NUM1
udiv8_l1:
        ASL dividend    // Shift hi bit of divisor into remainder
        ROL dividend+1
        ROL dividend+2
        ROL dividend+3   // (vacating the lo bit, which will be used for the quotient)
        ROL remainder
        ROL remainder+1
        ROL remainder+2
        ROL remainder+3
        LDA remainder
        SEC         // Trial subtraction
        SBC divisor
        TAY
        LDA remainder+1
        SBC divisor+1
        STA __t0
        LDA remainder+2
        SBC divisor+2
        STA __t1
        LDA remainder+3
        SBC divisor+3
        BCC udiv8_l2       // Did subtraction succeed?
        STA remainder+3   // If yes, save it
        LDA __t1
        STA remainder+2
        LDA __t0
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
  LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1
  LDA 2,Y
  STA divisor+2
  LDA 3,Y
  STA divisor+3
  EOR dividend+3
  BPL umod8_1

  // One of divisor or dividend is negative.  Result will be negative.
  LDA divisor+3
  BPL smod8_l1

  // Divisor is negative, negate it.
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
  BRA smod8_l2

smod8_l1:
  // Dividend is negative, negate it.
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

smod8_l2:
  // Perform unsigned divide
  PLA
  JSR umod8_1

  // Negate result.
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
  RTS

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
  LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1
  LDA 2,Y
  STA divisor+2
  LDA 3,Y
  STA divisor+3

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
  // 0000 0000 0010 0000 0000 0000 = 0x002000
  .byte 0,0x20,0         // Save x1 and x2.

  LDA #__x1           // Numerator in i1
  LDX #2
  JSR __arg_value8

  LDA #__x2           // Denominartor in i2
  LDX #6
  JSR __arg_value8

  LDA #__l0     // Not used.
  LDX #__l1
  LDY #__l2
  JSR __sdiv4

  LDA #__i0           // Result in i0
  LDX #0
  JSR __arg_value2

  // Quotient in first 4 bytes of result.
  LDX #7
  LDY #7
qloop:
  LDA quotient,X
  STA (__i0),Y
  DEY
  DEX
  BNE qloop

  LDX #7
  LDY #7
rloop:
  LDA remainder,X
  STA (__i0),Y
  DEY
  DEX
  BNE rloop
  LDY #7
  JMP __leave_leaf
