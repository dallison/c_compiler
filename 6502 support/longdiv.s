#include "vars.s"

// Long division routines.  4 and 8 byte.
.section ".text.__udiv4", "ax", @progbits

.global __sdiv4
.global __udiv4

.global __smod4
.global __umod4

.global __cdivmod4

// Division routines:
.set dividend mt2
.set divisor mt3
.set remainder mt1
.set quotient mt2


// Signed 4-byte divide.  As in C99 the quotient rounds toward zero: divide
// the magnitudes, then negate when the operand signs differ.
__sdiv4:
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
  PHA             // Sign of the quotient.
  JSR sdiv4_abs
  JSR udiv4
  PLA
  BPL sdiv4_l1
  SEC
  LDA #0
  SBC quotient
  STA quotient
  LDA #0
  SBC quotient+1
  STA quotient+1
  LDA #0
  SBC quotient+2
  STA quotient+2
  LDA #0
  SBC quotient+3
  STA quotient+3
sdiv4_l1:
#ifdef __65c02__
  PLX
#else
  PLA
  TAX
#endif
  LDA quotient
  STA 0,X
  LDA quotient+1
  STA 1,X
  LDA quotient+2
  STA 2,X
  LDA quotient+3
  STA 3,X
  RTS

// Replace dividend and divisor by their magnitudes.
sdiv4_abs:
  LDA divisor+3
  BPL sdiv4_abs_l1
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
sdiv4_abs_l1:
  LDA dividend+3
  BPL sdiv4_abs_l2
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
sdiv4_abs_l2:
  RTS

__udiv4:
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

udiv4_1:
  JSR udiv4

  // Store result.
#ifdef __65c02__
  PLX
#else
  PLA
  TAX
#endif
  LDA quotient
  STA 0,X
  LDA quotient+1
  STA 1,X
  LDA quotient+2
  STA 2,X
  LDA quotient+3
  STA 3,X
  RTS

// Main udiv4 routine.  Produces both remainder and quotient
udiv4:
        // Zero out remainder.
#ifdef __65c02__
        STZ remainder
        STZ remainder+1
        STZ remainder+2
        STZ remainder+3
#else
        PHA
        LDA #0
        STA remainder
        STA remainder+1
        STA remainder+2
        STA remainder+3
        STA __nmos_tmp
        PLA
        BIT __nmos_tmp
#endif
        LDX #32     // There are 32 bits in NUM1
udiv4_l1:
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
        BCC udiv4_l2       // Did subtraction succeed?
        STA remainder+3   // If yes, save it
        LDA __t1
        STA remainder+2
        LDA __t0
        STA remainder+1
        STY remainder
        INC dividend    // and record a 1 in the quotient
udiv4_l2:
        DEX
        BNE udiv4_l1
        RTS

// Signed 4-byte remainder.  As in C99 it takes the sign of the dividend.
__smod4:
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
  LDA dividend+3
  PHA             // Sign of the remainder.
  JSR sdiv4_abs
  JSR udiv4
  PLA
  BPL smod4_l1
  SEC
  LDA #0
  SBC remainder
  STA remainder
  LDA #0
  SBC remainder+1
  STA remainder+1
  LDA #0
  SBC remainder+2
  STA remainder+2
  LDA #0
  SBC remainder+3
  STA remainder+3
smod4_l1:
#ifdef __65c02__
  PLX
#else
  PLA
  TAX
#endif
  LDA remainder
  STA 0,X
  LDA remainder+1
  STA 1,X
  LDA remainder+2
  STA 2,X
  LDA remainder+3
  STA 3,X
  RTS

__umod4:
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

umod4_1:
  JSR udiv4

  // Store remainder as result.
#ifdef __65c02__
  PLX
#else
  PLA
  TAX
#endif
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
// sp+2-5: numerator
// sp+6-9: demoninator
__cdivmod4:
  LDX #4              // Minimum frame.
  JSR __enter_leaf
  // L regs start at bit 9 in the save mask.
  // l0 and l1 are temporaries.  l2 is the first preserved long and the
  // only callee-saved register this leaf writes.
  // bit 9 set => save 1 L reg: 0x000200
  .byte 0,2,0         // Save l2.

  LDA #__l1           // Numerator in i1
  LDX #2
  JSR __arg_value4

  LDA #__l2           // Denominartor in i2
  LDX #6
  JSR __arg_value4

  LDA #__l0     // Not used.
  LDX #__l1
  LDY #__l2
  JSR __sdiv4

  // __sdiv4 leaves the remainder of the magnitudes; it takes the sign of
  // the numerator.
  LDA __l1+3
  BPL cdivmod4_l1
  SEC
  LDA #0
  SBC remainder
  STA remainder
  LDA #0
  SBC remainder+1
  STA remainder+1
  LDA #0
  SBC remainder+2
  STA remainder+2
  LDA #0
  SBC remainder+3
  STA remainder+3
cdivmod4_l1:

  LDA #__i0           // Result in i0
  LDX #0
  JSR __arg_value2

  // Quotient in first 4 bytes of result.
  LDA quotient
#ifdef __65c02__
  STA (__i0)
#else
  PHP
  STY __nmos_tmp
  LDY #0
  STA (__i0),Y
  LDY __nmos_tmp
  PLP
#endif
  LDY #1
  LDA quotient+1
  STA (__i0),Y
  INY
  LDA quotient+2
  STA (__i0),Y
  INY
  LDA quotient+3
  STA (__i0),Y

  INY
  LDA remainder
  STA (__i0),Y
  INY
  LDA remainder+1
  STA (__i0),Y
  INY
  LDA remainder+2
  STA (__i0),Y
  INY
  LDA remainder+3
  STA (__i0),Y
  LDY #7
  JMP __leave_leaf

