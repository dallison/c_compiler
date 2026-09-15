#include "vars.s"

// Functions are emitted in per-symbol ELF sections.

// Only declare symbols defined in this file.  Declaring symbols that live in
// other archive members (longdiv.s, longlongdiv.s, fdiv.s) creates undefined
// references that force the linker to pull those members into every link.
.global __sdiv1
.global __sdiv2
.global __udiv1
.global __udiv2

.global __smod1
.global __smod2
.global __umod1
.global __umod2

// C's div function, both quotient and remainder
.global __cdivmod2


// Division routines:
.set dividend mt2
.set divisor mt3
.set remainder mt1
.set quotient mt2

.section ".text.__sdiv1", "ax", @progbits
__sdiv1:
  RTS

.section ".text.__udiv1", "ax", @progbits
__udiv1:
  RTS


// Signed 2-byte divide.
// TODO: this is wrong for negative numbers according to C99.
// We need to round the quotient toward zero and the sign of the remainer
// can be negative.
.section ".text.__sdiv2", "ax", @progbits
__sdiv2:
  PHA
  LDA 0,X
  STA dividend
  LDA 1,X
  STA dividend+1
  LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1
  EOR dividend+1
  BPL udiv2_1

  // One of divisor or dividend is negative.  Result will be negative.
  LDA divisor+1
  BPL sdiv2_l1

  // Divisor is negative, negate it.
  SEC
  LDA #0
  SBC divisor
  STA divisor
  LDA #0
  SBC divisor+1
  STA divisor+1
  BRA sdiv2_l2

sdiv2_l1:
  // Dividend is negative, negate it.
  SEC
  LDA #0
  SBC dividend
  STA dividend
  LDA #0
  SBC dividend+1
  STA dividend+1

sdiv2_l2:
  // Perform unsigned divide
  PLA
  JSR udiv2

  // Negate result.
  SEC
  LDA #0
  SBC 0,X
  STA 0,X
  LDA #0
  SBC 1,X
  STA 1,X
  RTS

.section ".text.__sdiv2", "ax", @progbits
__udiv2:
  PHA
  LDA 0,X
  STA dividend
  LDA 1,X
  STA dividend+1
  LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1

udiv2_1:
  JSR udiv2

  // Store result.
  PLX
  LDA quotient
  STA 0,X
  LDA quotient+1
  STA 1,X
  RTS

// Main udiv2 routine.  Produces both remainder and quotent
udiv2:
        // Zero out remainder.
        STZ remainder
        STZ remainder+1
        LDX #16     // There are 16 bits in NUM1
udiv2_l1:
        ASL dividend    // Shift hi bit of divisor into remainder
        ROL dividend+1   // (vacating the lo bit, which will be used for the quotient)
        ROL remainder
        ROL remainder+1
        LDA remainder
        SEC         // Trial subtraction
        SBC divisor
        TAY
        LDA remainder+1
        SBC divisor+1
        BCC udiv2_l2       // Did subtraction succeed?
        STA remainder+1   // If yes, save it
        STY remainder
        INC quotient    // and record a 1 in the quotient
udiv2_l2:
        DEX
        BNE udiv2_l1
        RTS

.section ".text.__smod2", "ax", @progbits
__smod2:
  PHA
  LDA 0,X
  STA dividend
  LDA 1,X
  STA dividend+1
  LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1
  EOR dividend+1
  BPL umod2_1

  // One of divisor or dividend is negative.  Result will be negative.
  LDA divisor+1
  BPL smod2_l1

  // Divisor is negative, negate it.
  SEC
  LDA #0
  SBC divisor
  STA divisor
  LDA #0
  SBC divisor+1
  STA divisor+1
  BRA smod2_l2

smod2_l1:
  // Dividend is negative, negate it.
  SEC
  LDA #0
  SBC dividend
  STA dividend
  LDA #0
  SBC dividend+1
  STA dividend+1

smod2_l2:
  // Perform unsigned divide
  PLA
  JSR umod2_1

  // Negate result.
  SEC
  LDA #0
  SBC 0,X
  STA 0,X
  LDA #0
  SBC 1,X
  STA 1,X
  RTS

.section ".text.__smod2", "ax", @progbits
__umod2:
  PHA
  LDA 0,X
  STA dividend
  LDA 1,X
  STA dividend+1
  LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1

umod2_1:
  JSR udiv2

  // Store remainder as result.
  PLX
  LDA remainder
  STA 0,X
  LDA remainder+1
  STA 1,X
  RTS
 


.section ".text.__smod2", "ax", @progbits
__smod1:
__umod1:
  RTS


// Entry from C:
// sp+0,1: result address
// sp+2,3: numerator
// sp+4,5: demoninator
.section ".text.__smod2", "ax", @progbits
__cdivmod2:
  LDX #4              // Minimum frame.
  JSR __enter_leaf_nomask

  LDA #__i1           // Numerator in i1
  LDX #2
  JSR __arg_value2

  LDA #__i2           // Denominartor in i2
  LDX #4
  JSR __arg_value2

  LDA #__i0     // Not used.
  LDX #__i1
  LDY #__i2
  JSR __sdiv2

  LDA #__i0           // Result in i0
  LDX #0
  JSR __arg_value2

  // Quotient in first 2 bytes of result.
  LDA quotient
  STA (__i0)
  LDY #1
  LDA quotient+1
  STA (__i0),Y

  INY
  LDA remainder
  STA (__i0),Y
  INY
  LDA remainder+1
  STA (__i0),Y

  LDY #7
  JMP __leave_leaf_nomask

