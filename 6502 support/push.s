#include "vars.s"
.text

.global __pusha
.global __pushxy
.global __pushxy0
.global __pushreg1
.global __pushreg2
.global __pushreg4
.global __pushreg8
.global __push4xy
.global __push8xy
.global __pulla
.global __pullreg2
.global __replace_top_reg2
.global __pullxy
.global __pull4
.global __pull8
.global __incsp
.global __incsp0
.global __incsp1
.global __decsp1
.global __incsp2
.global __decsp2
.global __incsp4
.global __incsp6
.global __decsp4
.global __incsp8
.global __incsp10
.global __incsp12
.global __incsp14
.global __incsp16
.global __decsp8
.global __decsp
.global __savesp
.global __restoresp

.global __pushi0
.global __pushi1
.global __pushi2
.global __pushi3
.global __pushi4
.global __pushi5
.global __pushi6
.global __pushi7
.global __pushi8
.global __pushi9
.global __pushi10
.global __pushi11
.global __pushi12
.global __pushi13
.global __pushi14
.global __pushi15

.global __pushl0
.global __pushl1
.global __pushl2
.global __pushl3
.global __pushl4
.global __pushl5
.global __pushl6
.global __pushl7

.global __pushf0
.global __pushf1
.global __pushf2
.global __pushf3

.global __pushx0
.global __pushx1
.global __pushx2
.global __pushx3

// t0: number of bytes to increment sp by
__incsp:
  STZ __t1

// t0,t1: number of bytes to increment sp by
__incsp0:
  LDA __t0
  CLC
  ADC __sp
  STA __sp
  LDA __t1
  ADC __sp+1
  STA __sp+1
  RTS

__incsp1:
  INC __sp
  BNE i1
  INC __sp+1
i1:
  RTS

__decsp1:
  DEC __sp
  BNE d1
  DEC __sp+1
d1:
  RTS

__decsp2:
  SEC
  LDA __sp
  SBC #2
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  RTS

__incsp2:
  LDA #2
  STA __t0
  BRA __incsp

__decsp4:
  SEC
  LDA __sp
  SBC #4
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  RTS

__incsp4:
  LDA #4
  STA __t0
  BRA __incsp


__incsp6:
  LDA #6
  STA __t0
  BRA __incsp


__decsp8:
  SEC
  LDA __sp
  SBC #8
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  RTS

__incsp8:
  LDA #8
  STA __t0
  BRA __incsp

__incsp10:
  LDA #10
  STA __t0
  BRA __incsp

__incsp12:
  LDA #12
  STA __t0
  JMP __incsp

__incsp14:
  LDA #14
  STA __t0
  BRA __incsp

__incsp16:
  LDA #16
  STA __t0
  BRA __incsp


// A: byte to push.
// Pushed as 2 bytes with top byte 0.
__pusha:
  TAY
  JSR __decsp2
  TYA
  STA (__sp)
  LDA #0
  LDY #1
  STA (__sp),Y
  RTS

__pulla:
  LDA (__sp)
  INC __sp
  BNE pla
  INC __sp+1
pla:
  INC __sp
  BNE pla1
  INC __sp+1
pla1:
  RTS

// X,Y: int16 to push.
__pushxy0:
  LDY #0
__pushxy:
  JSR __decsp2
  TXA
  STA (__sp)
  TYA
  LDY #1
  STA (__sp), Y
  RTS

// Entry:
// X: index of reg to push
__pushreg1:
  JSR __decsp1
  LDA 0,X
  STA (__sp)
  RTS

__pushi0:
  LDX #__i0
  BRA __pushreg2
__pushi1:
LDX #__i1
BRA __pushreg2
__pushi2:
LDX #__i2
BRA __pushreg2
__pushi3:
LDX #__i3
BRA __pushreg2
__pushi4:
LDX #__i4
BRA __pushreg2
__pushi5:
LDX #__i5
BRA __pushreg2
__pushi6:
LDX #__i6
BRA __pushreg2
__pushi7:
LDX #__i7

__pushreg2:
  JSR __decsp2
  LDA 0,X
  STA (__sp)
  LDY #1
  LDA 1,X
  STA (__sp),Y
  RTS

__pushi8:
LDX #__i8
BRA __pushreg2
__pushi9:
LDX #__i9
BRA __pushreg2
__pushi10:
LDX #__i10
BRA __pushreg2
__pushi11:
LDX #__i11
BRA __pushreg2
__pushi12:
LDX #__i12
BRA __pushreg2
__pushi13:
LDX #__i13
BRA __pushreg2
__pushi14:
LDX #__i14
BRA __pushreg2
__pushi15:
LDX #__i15
BRA __pushreg2

__pushl0:
LDX #__l0
BRA __pushreg4
__pushl1:
LDX #__l1
BRA __pushreg4
__pushl2:
LDX #__l2
BRA __pushreg4
__pushl3:
LDX #__l3
BRA __pushreg4

__pushl4:
LDX #__l4
BRA __pushreg4
__pushl5:
LDX #__l5
BRA __pushreg4
__pushl6:
LDX #__l6
BRA __pushreg4
__pushl7:
LDX #__l7
BRA __pushreg4

__pushf0:
LDX #__f0
BRA __pushreg4
__pushf1:
LDX #__f1
BRA __pushreg4
__pushf2:
LDX #__f2
BRA __pushreg4
__pushf3:
LDX #__f3

__pushreg4:
  JSR __decsp4
  LDY #0
preg4:
  LDA 0,X
  STA (__sp),Y
  INX
  INY
  CPY #4
  BNE preg4
  RTS

__pushx0:
LDX #__x0
BRA __pushreg8
__pushx1:
LDX #__x1
BRA __pushreg8
__pushx2:
LDX #__x2
BRA __pushreg8
__pushx3:
LDX #__x3


__pushreg8:
  JSR __decsp8
  LDY #0
preg8:
  LDA 0,X
  STA (__sp),Y
  INX
  INY
  CPY #8
  BNE preg8
  RTS

// X: index of the two-byte register to pull into.
__pullreg2:
  LDY #0
  LDA (__sp),Y
  STA 0,X
  INY
  LDA (__sp),Y
  STA 1,X
  LDA 0,X
  PHA
  LDA 1,X
  TAY
  PLA
  TAX
  CLC
  LDA __sp
  ADC #2
  STA __sp
  LDA __sp+1
  ADC #0
  STA __sp+1
  RTS

// Replace the stack item below the top value while retaining the top value.
// A: number of bytes to discard below the top value.
// X: index of the two-byte register that receives the top value.
__replace_top_reg2:
  STA __t0
  STZ __t1
  LDY #0
  LDA (__sp),Y
  STA 0,X
  LDY __t0
  STA (__sp),Y
  LDY #1
  LDA (__sp),Y
  STA 1,X
  LDY __t0
  INY
  STA (__sp),Y
  CLC
  LDA __sp
  ADC __t0
  STA __sp
  LDA __sp+1
  ADC #0
  STA __sp+1
  LDY #1
  LDA 1,X
  RTS

__pullxy:
  LDA (__sp)
  TAX
  LDY #1
  LDA (__sp),Y
  TAY
  CLC
  LDA __sp
  ADC #2
  STA __sp
  LDA __sp+1
  ADC #0
  STA __sp+1
  RTS

// X,Y: address of int32 to push
__push4xy:
  JSR __decsp4
  STX __t0
  STY __t1
  LDY #0
p4l:
  LDA (__t0),Y
  STA (__sp), Y
  INY
  CPY #4
  BNE p4l
  RTS

__pull4:
  STX __t0
  STY __t1
  LDY #0
pl4l:
  LDA (__sp),Y
  STA (__t0), Y
  INY
  CPY #4
  BNE pl4l
  JMP __incsp4

// X,Y: address of int64 to push
__push8xy:
  JSR __decsp8
  STX __t0
  STY __t1
  LDY #0
p8l:
  LDA (__t0),Y
  STA (__sp), Y
  INY
  CPY #8
  BNE p8l
  RTS

__pull8:
  STX __t0
  STY __t1
  LDY #0
pl8l:
  LDA (__sp),Y
  STA (__t0), Y
  INY
  CPY #8
  BNE pl8l
  JMP __incsp8

// Enter:
// X: offset into sp of int containing size to decrement by
__decsp:
  SEC
  LDA 0,X
  STA __t0
  LDA 1,X
  STA __t1
  
  LDY #0
  LDA __sp
  SBC (__t0),Y
  STA __sp
  LDA __sp+1
  INY
  SBC (__t0),Y
  STA __sp+1
  RTS

// X: offset into zero page to save to
__savesp:
  LDA __sp
  STA 0,X
  LDA __sp+1
  STA 1,X
  RTS

__restoresp:
  LDA 0,X
  STA __sp
  LDA 1,X
  STA __sp+1
  RTS

  

