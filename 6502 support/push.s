#include "vars.s"
// Functions are emitted in per-symbol ELF sections.

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
.section ".text.__incsp", "ax", @progbits
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

.section ".text.__incsp1", "ax", @progbits
__incsp1:
  INC __sp
  BNE i1
  INC __sp+1
i1:
  RTS

.section ".text.__decsp1", "ax", @progbits
__decsp1:
  DEC __sp
  BNE d1
  DEC __sp+1
d1:
  RTS

.section ".text.__decsp2", "ax", @progbits
__decsp2:
  SEC
  LDA __sp
  SBC #2
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  RTS

.section ".text.__incsp2", "ax", @progbits
__incsp2:
  LDA #2
  STA __t0
  JMP __incsp

.section ".text.__decsp4", "ax", @progbits
__decsp4:
  SEC
  LDA __sp
  SBC #4
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  RTS

.section ".text.__incsp4", "ax", @progbits
__incsp4:
  LDA #4
  STA __t0
  JMP __incsp


.section ".text.__incsp6", "ax", @progbits
__incsp6:
  LDA #6
  STA __t0
  JMP __incsp


.section ".text.__decsp8", "ax", @progbits
__decsp8:
  SEC
  LDA __sp
  SBC #8
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  RTS

.section ".text.__incsp8", "ax", @progbits
__incsp8:
  LDA #8
  STA __t0
  JMP __incsp

.section ".text.__incsp10", "ax", @progbits
__incsp10:
  LDA #10
  STA __t0
  JMP __incsp

.section ".text.__incsp12", "ax", @progbits
__incsp12:
  LDA #12
  STA __t0
  JMP __incsp

.section ".text.__incsp14", "ax", @progbits
__incsp14:
  LDA #14
  STA __t0
  JMP __incsp

.section ".text.__incsp16", "ax", @progbits
__incsp16:
  LDA #16
  STA __t0
  JMP __incsp


// A: byte to push.
// Pushed as 2 bytes with top byte 0.
.section ".text.__pusha", "ax", @progbits
__pusha:
  TAY
  JSR __decsp2
  TYA
  STA (__sp)
  LDA #0
  LDY #1
  STA (__sp),Y
  RTS

.section ".text.__pulla", "ax", @progbits
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
.section ".text.__pushxy0", "ax", @progbits
__pushxy0:
  LDY #0
  JMP __pushxy
.section ".text.__pushxy", "ax", @progbits
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
.section ".text.__pushreg1", "ax", @progbits
__pushreg1:
  JSR __decsp1
  LDA 0,X
  STA (__sp)
  RTS

.section ".text.__pushi0", "ax", @progbits
__pushi0:
  LDX #__i0
  JMP __pushreg2
.section ".text.__pushi1", "ax", @progbits
__pushi1:
LDX #__i1
JMP __pushreg2
.section ".text.__pushi2", "ax", @progbits
__pushi2:
LDX #__i2
JMP __pushreg2
.section ".text.__pushi3", "ax", @progbits
__pushi3:
LDX #__i3
JMP __pushreg2
.section ".text.__pushi4", "ax", @progbits
__pushi4:
LDX #__i4
JMP __pushreg2
.section ".text.__pushi5", "ax", @progbits
__pushi5:
LDX #__i5
JMP __pushreg2
.section ".text.__pushi6", "ax", @progbits
__pushi6:
LDX #__i6
JMP __pushreg2
.section ".text.__pushi7", "ax", @progbits
__pushi7:
LDX #__i7
  JMP __pushreg2

.section ".text.__pushreg2", "ax", @progbits
__pushreg2:
  JSR __decsp2
  LDA 0,X
  STA (__sp)
  LDY #1
  LDA 1,X
  STA (__sp),Y
  RTS

.section ".text.__pushi8", "ax", @progbits
__pushi8:
LDX #__i8
JMP __pushreg2
.section ".text.__pushi9", "ax", @progbits
__pushi9:
LDX #__i9
JMP __pushreg2
.section ".text.__pushi10", "ax", @progbits
__pushi10:
LDX #__i10
JMP __pushreg2
.section ".text.__pushi11", "ax", @progbits
__pushi11:
LDX #__i11
JMP __pushreg2
.section ".text.__pushi12", "ax", @progbits
__pushi12:
LDX #__i12
JMP __pushreg2
.section ".text.__pushi13", "ax", @progbits
__pushi13:
LDX #__i13
JMP __pushreg2
.section ".text.__pushi14", "ax", @progbits
__pushi14:
LDX #__i14
JMP __pushreg2
.section ".text.__pushi15", "ax", @progbits
__pushi15:
LDX #__i15
JMP __pushreg2

.section ".text.__pushl0", "ax", @progbits
__pushl0:
LDX #__l0
JMP __pushreg4
.section ".text.__pushl1", "ax", @progbits
__pushl1:
LDX #__l1
JMP __pushreg4
.section ".text.__pushl2", "ax", @progbits
__pushl2:
LDX #__l2
JMP __pushreg4
.section ".text.__pushl3", "ax", @progbits
__pushl3:
LDX #__l3
JMP __pushreg4

.section ".text.__pushl4", "ax", @progbits
__pushl4:
LDX #__l4
JMP __pushreg4
.section ".text.__pushl5", "ax", @progbits
__pushl5:
LDX #__l5
JMP __pushreg4
.section ".text.__pushl6", "ax", @progbits
__pushl6:
LDX #__l6
JMP __pushreg4
.section ".text.__pushl7", "ax", @progbits
__pushl7:
LDX #__l7
JMP __pushreg4

.section ".text.__pushf0", "ax", @progbits
__pushf0:
LDX #__f0
JMP __pushreg4
.section ".text.__pushf1", "ax", @progbits
__pushf1:
LDX #__f1
JMP __pushreg4
.section ".text.__pushf2", "ax", @progbits
__pushf2:
LDX #__f2
JMP __pushreg4
.section ".text.__pushf3", "ax", @progbits
__pushf3:
LDX #__f3
  JMP __pushreg4

.section ".text.__pushreg4", "ax", @progbits
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

.section ".text.__pushx0", "ax", @progbits
__pushx0:
LDX #__x0
JMP __pushreg8
.section ".text.__pushx1", "ax", @progbits
__pushx1:
LDX #__x1
JMP __pushreg8
.section ".text.__pushx2", "ax", @progbits
__pushx2:
LDX #__x2
JMP __pushreg8
.section ".text.__pushx3", "ax", @progbits
__pushx3:
LDX #__x3
  JMP __pushreg8

.section ".text.__pushreg8", "ax", @progbits
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
.section ".text.__pullreg2", "ax", @progbits
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
.section ".text.__replace_top_reg2", "ax", @progbits
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

.section ".text.__pullxy", "ax", @progbits
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
.section ".text.__push4xy", "ax", @progbits
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

.section ".text.__pull4", "ax", @progbits
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
.section ".text.__push8xy", "ax", @progbits
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

.section ".text.__pull8", "ax", @progbits
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
.section ".text.__decsp", "ax", @progbits
__decsp:
  SEC
  LDA __sp
  SBC 0,X
  STA __sp
  LDA __sp+1
  SBC 1,X
  STA __sp+1
  RTS

// X: offset into zero page to save to
.section ".text.__savesp", "ax", @progbits
__savesp:
  LDA __sp
  STA 0,X
  LDA __sp+1
  STA 1,X
  RTS

.section ".text.__restoresp", "ax", @progbits
__restoresp:
  LDA 0,X
  STA __sp
  LDA 1,X
  STA __sp+1
  RTS

  

