//
//  longlongmul.s
//  c_compiler
//
//  Created by David Allison on 12/7/21.
//  Copyright © 2021 David Allison. All rights reserved.
//
#include "vars.s"
.section ".text.__umul8", "ax", @progbits

.global __umul8
.global __smul8

// Entry:
// A: offset into zero page for result
// X: offset into zero page for op1
// Y: offset into zero page for op2
// Places result in 0,A.
.set product mt1
.set multiplicand mt2
.set multiplier mt3

__smul8:
  // Signed and unsigned multiplication have the same low 64 result bits.
  // Sharing this path also avoids nesting a JSR below the result address
  // saved on the hardware stack.
  JMP __umul8
  PHA
  LDA 0,X
  STA multiplicand
  LDA 1,X
  STA multiplicand+1
  LDA 2,X
  STA multiplicand+2
  LDA 3,X
  STA multiplicand+3
  LDA 4,X
  STA multiplicand+4
  LDA 5,X
  STA multiplicand+5
  LDA 6,X
  STA multiplicand+6
  LDA 7,X
  STA multiplicand+7
  LDA 0,Y
  STA multiplier
  LDA 1,Y
  STA multiplier+1
  LDA 2,Y
  STA multiplier+2
  LDA 3,Y
  STA multiplier+3
  LDA 4,Y
  STA multiplier+4
  LDA 5,Y
  STA multiplier+5
  LDA 6,Y
  STA multiplier+6
  LDA 7,Y
  STA multiplier+7
  EOR multiplicand+7
  BMI smul8_1
  JMP umul8c        // Branch out of range.
smul8_1:
  // One of multiplier or multiplicand is negative.  Result will be negative.
  LDA multiplier+7
  BPL smul8a

  // Multiplier is negative, negate it.
  SEC
  LDA #0
  SBC multiplier
  STA multiplier
  LDA #0
  SBC multiplier+1
  STA multiplier+1
  LDA #0
  SBC multiplier+2
  STA multiplier+2
  LDA #0
  SBC multiplier+3
  STA multiplier+3
  LDA #0
  SBC multiplier+4
  STA multiplier+4
  LDA #0
  SBC multiplier+5
  STA multiplier+5
  LDA #0
  SBC multiplier+6
  STA multiplier+6
  LDA #0
  SBC multiplier+7
  STA multiplier+7
  BRA smul8b
smul8a:
  // Multiplicand is negative, negate it.
  SEC
  LDA #0
  SBC multiplicand
  STA multiplicand
  LDA #0
  SBC multiplicand+1
  STA multiplicand+1
  LDA #0
  SBC multiplicand+2
  STA multiplicand+2
  LDA #0
  SBC multiplicand+3
  STA multiplicand+3
  LDA #0
  SBC multiplicand+4
  STA multiplicand+4
  LDA #0
  SBC multiplicand+5
  STA multiplicand+5
  LDA #0
  SBC multiplicand+6
  STA multiplicand+6
  LDA #0
  SBC multiplicand+7
  STA multiplicand+7

smul8b:
  // Perform unsigned mutiply
  PLA
  JSR umul8b

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

__umul8:
  PHA
  LDA 0,X
  STA multiplicand
  LDA 1,X
  STA multiplicand+1
  LDA 2,X
  STA multiplicand+2
  LDA 3,X
  STA multiplicand+3
  LDA 4,X
  STA multiplicand+4
  LDA 5,X
  STA multiplicand+5
  LDA 6,X
  STA multiplicand+6
  LDA 7,X
  STA multiplicand+7
  LDA 0,Y
  STA multiplier
  LDA 1,Y
  STA multiplier+1
  LDA 2,Y
  STA multiplier+2
  LDA 3,Y
  STA multiplier+3
  LDA 4,Y
  STA multiplier+4
  LDA 5,Y
  STA multiplier+5
  LDA 6,Y
  STA multiplier+6
  LDA 7,Y
  STA multiplier+7
  BRA umul8c

umul8b:
  PLA
umul8c:
  LDA #0       // Initialize product to 0
STA product+14
STA product+13
STA product+12
STA product+11
STA product+10
STA product+9
STA product+8
  LDX #64      // There are 64 bits in NUM2
umul8_l1:
  LSR multiplier+7   // Get low bit of NUM2
  ROR multiplier+6
  ROR multiplier+5
  ROR multiplier+4
  ROR multiplier+3
  ROR multiplier+2
  ROR multiplier+1
  ROR multiplier
  BCC umul8_l2       // 0 or 1?
  TAY          // If 1, add NUM1 (hi byte of RESULT is in A)
  CLC
  LDA multiplicand
  ADC product+8
  STA product+8
  LDA multiplicand+1
  ADC product+9
  STA product+9
  LDA multiplicand+2
  ADC product+10
  STA product+10
  LDA multiplicand+3
  ADC product+11
  STA product+11
  LDA multiplicand+4
  ADC product+12
  STA product+12
  LDA multiplicand+5
  ADC product+13
  STA product+13
  LDA multiplicand+6
  ADC product+14
  STA product+14
  TYA
  ADC multiplicand+7
umul8_l2:
  ROR A        // "Stairstep" shift
  ROR product+14
  ROR product+13
  ROR product+12
  ROR product+11
  ROR product+10
  ROR product+9
  ROR product+8
  ROR product+7
  ROR product+6
  ROR product+5
  ROR product+4
  ROR product+3
  ROR product+2
  ROR product+1
  ROR product
  DEX
  BNE umul8_l1
  STA product+15
  PLX
  LDA product
  STA 0,X
  LDA product+1
  STA 1,X
  LDA product+1
  STA 1,X
  LDA product+2
  STA 2,X
  LDA product+3
  STA 3,X
  LDA product+4
  STA 4,X
  LDA product+5
  STA 5,X
  LDA product+6
  STA 6,X
  LDA product+7
  STA 7,X
  RTS

