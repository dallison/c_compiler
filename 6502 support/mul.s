#include "vars.s"

.text

// Only declare symbols defined in this file.  __umul4/__smul4 live in
// longmul.s; declaring them here would pull that member into every link.
.global __umul1
.global __umul2
.global __smul1
.global __smul2
.global __umul2_10    // Multiply int by 10.
.global __smul2_10

// Entry:
// A: offset into zero page for result
// X: offset into zero page for op1
// Y: offset into zero page for op2
// Places result in 0,A.
.set product mt1
.set multiplicand mt2
.set multiplier mt3

// For signed multiply we check the sign bits of the 2 operands.
// If they are different we know one of them is negative.  We negate
// the negative one and then negate the result.
__smul1:
  PHA
  LDA 0,X
  STA multiplicand
  LDA 0,Y
  STA multiplier
  EOR multiplicand
  BPL umul1c

  // One of multiplier or multiplicand is negative.  Result will be negative.
  LDA multiplier
  BPL smul1a

  // Multiplier is negative, negate it.
  SEC
  LDA #0
  SBC multiplier
  STA multiplier
  BRA smul1b
smul1a:
  // Multiplicand is negative, negate it.
  SEC
  LDA #0
  SBC multiplicand
  STA multiplicand

smul1b:
  // Perform unsigned mutiply
  PLA
  JSR umul1b

  // Negate result.
  SEC
  LDA #0
  SBC 0,X
  STA 0,X
  RTS

__umul1:
  PHA
  LDA 0,X
  STA multiplicand
  LDA 0,Y
  STA multiplier
  BRA umul1c

// These are from:
// https://llx.com/Neil/a2/mult.html
umul1b:
  PHA
umul1c:
  LDA #0       // Initialize RESULT to 0
  LDX #$8       // There are 8 bits in multiplier
umul1_l1:
  LSR multiplier       // Get low bit of multiplier
  BCC umul1_l2        // 0 or 1?
  CLC                 // If 1, add multiplicand
  ADC multiplicand
umul1_l2:
  ROR A        // "Stairstep" shift (catching carry from add)
  ROR product
  DEX
  BNE umul1_l1
  STA product+1
  PLX
  LDA product
  STA 0,X
  RTS

__smul2:
  PHA
  LDA 0,X
  STA multiplicand
  LDA 1,X
  STA multiplicand+1
  LDA 0,Y
  STA multiplier
  LDA 1,Y
  STA multiplier+1
  EOR multiplicand+1
  BPL umul2c

  // One of multiplier or multiplicand is negative.  Result will be negative.
  LDA multiplier+1
  BPL smul2a

  // Multiplier is negative, negate it.
  SEC
  LDA #0
  SBC multiplier
  STA multiplier
  LDA #0
  SBC multiplier+1
  STA multiplier+1
  BRA smul2b
smul2a:
  // Multiplicand is negative, negate it.
  SEC
  LDA #0
  SBC multiplicand
  STA multiplicand
  LDA #0
  SBC multiplicand+1
  STA multiplicand+1

smul2b:
  // Perform unsigned mutiply
  PLA
  JSR umul2b

  // On exit from umul2b X will contain output index.
  // Negate result.
  SEC
  LDA #0
  SBC 0,X
  STA 0,X
  LDA #0
  SBC 1,X
  STA 1,X
  RTS

__umul2:
  PHA
  LDA 0,X
  STA multiplicand
  LDA 1,X
  STA multiplicand+1
  LDA 0,Y
  STA multiplier
  LDA 1,Y
  STA multiplier+1
  BRA umul2c

umul2b:
  PHA
umul2c:
  LDA #0       // Initialize product to 0
  STA product+2
  LDX #16      // There are 16 bits in NUM2
umul2_l1:
  LSR multiplier+1   // Get low bit of NUM2
  ROR multiplier
  BCC umul2_l2       // 0 or 1?
  TAY          // If 1, add NUM1 (hi byte of RESULT is in A)
  CLC
  LDA multiplicand
  ADC product+2
  STA product+2
  TYA
  ADC multiplicand+1
umul2_l2:
  ROR A        // "Stairstep" shift
  ROR product+2
  ROR product+1
  ROR product
  DEX
  BNE umul2_l1
  PLX
  STA product+3
  LDA product
  STA 0,X
  LDA product+1
  STA 1,X
  RTS

// Multiply an int (16 bits) by 10.
// X: offset of multiplicand in zero page.
// Perform multiplication by:
// b = a * 8 + a * 2
__umul2_10:
  PHA
umul2_10b:
  LDA 0,X
  STA __t0
  STA __t2
  LDA 1,X
  STA __t1
  STA __t3

umul2_10c:
  // t0,t1 *= 8
  ASL __t0
  ROL __t1
  ASL __t0
  ROL __t1
  ASL __t0
  ROL __t1

  // t2,t3 *= 2
  ASL __t2
  ROL __t3

  PLX
  CLC
  LDA __t0
  ADC __t2
  STA 0,X
  LDA __t1
  ADC __t3
  STA 1,X
  RTS

// Signed multiply int by 10
__smul2_10:
  PHA
  LDA 1,X
  BPL umul2_10b     // Positive multiplicand, just unsigned multiply

  // Negate multiplicand.
  SEC
  LDA #0
  SBC 0,X
  STA __t0
  STA __t2
  LDA #0
  SBC 1,X
  STA __t1
  STA __t3

  // Do unsigned multiply (leaves X = result offet)
  JSR umul2_10c

  // Negate result.
  SEC
  LDA #0
  SBC 0,X
  STA 0,X
  LDA #0
  SBC 1,X
  STA 1,X
  RTS

