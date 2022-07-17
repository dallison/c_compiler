#include "vars.s"

.text

.global __umul4
.global __smul4

// Entry:
// A: offset into zero page for result
// X: offset into zero page for op1
// Y: offset into zero page for op2
// Places result in 0,A.
.set product mt1
.set multiplicand mt2
.set multiplier mt3

__smul4:
  PHA
  LDA 0,X
  STA multiplicand
  LDA 1,X
  STA multiplicand+1
  LDA 2,X
  STA multiplicand+2
  LDA 3,X
  STA multiplicand+3
  LDA 0,Y
  STA multiplier
  LDA 1,Y
  STA multiplier+1
  LDA 2,Y
  STA multiplier+2
  LDA 3,Y
  STA multiplier+3
  EOR multiplicand+3
  BPL umul4c

  // One of multiplier or multiplicand is negative.  Result will be negative.
  LDA multiplier+3
  BPL smul4a

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
  BRA smul4b
smul4a:
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

smul4b:
  // Perform unsigned mutiply
  PLA
  JSR umul4b

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

__umul4:
  PHA
  LDA 0,X
  STA multiplicand
  LDA 1,X
  STA multiplicand+1
  LDA 2,X
  STA multiplicand+2
  LDA 3,X
  STA multiplicand+3
  LDA 0,Y
  STA multiplier
  LDA 1,Y
  STA multiplier+1
  LDA 2,Y
  STA multiplier+2
  LDA 3,Y
  STA multiplier+3
  BRA umul4c

umul4b:
  PLA
umul4c  :
  LDA #0       // Initialize product to 0
  STA product+6
  STA product+5
  STA product+4
  LDX #32      // There are 32 bits in NUM2
umul4_l1:
  LSR multiplier+3   // Get low bit of NUM2
  ROR multiplier+2
  ROR multiplier+1
  ROR multiplier
  BCC umul4_l2       // 0 or 1?
  TAY          // If 1, add NUM1 (hi byte of RESULT is in A)
  CLC
  LDA multiplicand
  ADC product+4
  STA product+4
  LDA multiplicand+1
  ADC product+5
  STA product+5
  LDA multiplicand+2
  ADC product+6
  STA product+6
  TYA
  ADC multiplicand+3
umul4_l2:
  ROR A        // "Stairstep" shift
  ROR product+6
  ROR product+5
  ROR product+4
  ROR product+3
  ROR product+2
  ROR product+1
  ROR product
  DEX
  BNE umul4_l1
  STA product+7
  PLX
  LDA product
  STA 0,X
  LDA product+1
  STA 1,X
  LDA product+2
  STA 2,X
  LDA product+3
  STA 3,X
  RTS


