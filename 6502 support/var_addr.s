#include "vars.s"

// Functions are emitted in per-symbol ELF sections.

.global __var_addr
.global __var_addrb
.global __arg_addr
.global __arg_addrb
.global __var_addr_xy
.global __var_addrb_xy
.global __arg_addr_xy
.global __arg_addrb_xy
.global __var_addr_push
.global __var_addr_push_i0
.global __var_addr_push_i1
.global __var_addr_push_i2
.global __var_addr_push_i3
.global __var_addr_push_i4
.global __var_addr_push_i5
.global __var_addr_push_i6
.global __var_addr_push_i7
.global __var_addr_push_i8
.global __var_addr_push_i9
.global __var_addr_push_i10
.global __var_addr_push_i11
.global __var_addr_push_i12
.global __var_addr_push_i13
.global __var_addr_push_i14
.global __var_addr_push_i15
.global __var_addrb_push
.global __var_addrb_push_i0
.global __var_addrb_push_i1
.global __var_addrb_push_i2
.global __var_addrb_push_i3
.global __var_addrb_push_i4
.global __var_addrb_push_i5
.global __var_addrb_push_i6
.global __var_addrb_push_i7
.global __var_addrb_push_i8
.global __var_addrb_push_i9
.global __var_addrb_push_i10
.global __var_addrb_push_i11
.global __var_addrb_push_i12
.global __var_addrb_push_i13
.global __var_addrb_push_i14
.global __var_addrb_push_i15
.global __arg_addr_push
.global __arg_addrb_push
.global __varaddr
.global __argaddr

.global __var_addr_i0
.global __var_addr_i1
.global __var_addr_i2
.global __var_addr_i3
.global __var_addr_i4
.global __var_addr_i5
.global __var_addr_i6
.global __var_addr_i7
.global __var_addr_i8
.global __var_addr_i9
.global __var_addr_i10
.global __var_addr_i11
.global __var_addr_i12
.global __var_addr_i13
.global __var_addr_i14
.global __var_addr_i15

.global __var_addrb_i0
.global __var_addrb_i1
.global __var_addrb_i2
.global __var_addrb_i3
.global __var_addrb_i4
.global __var_addrb_i5
.global __var_addrb_i6
.global __var_addrb_i7
.global __var_addrb_i8
.global __var_addrb_i9
.global __var_addrb_i10
.global __var_addrb_i11
.global __var_addrb_i12
.global __var_addrb_i13
.global __var_addrb_i14
.global __var_addrb_i15

.global __arg_addr_i0
.global __arg_addr_i1
.global __arg_addr_i2
.global __arg_addr_i3
.global __arg_addr_i4
.global __arg_addr_i5
.global __arg_addr_i6
.global __arg_addr_i7
.global __arg_addr_i8
.global __arg_addr_i9
.global __arg_addr_i10
.global __arg_addr_i11
.global __arg_addr_i12
.global __arg_addr_i13
.global __arg_addr_i14
.global __arg_addr_i15

.global __arg_addrb_i0
.global __arg_addrb_i1
.global __arg_addrb_i2
.global __arg_addrb_i3
.global __arg_addrb_i4
.global __arg_addrb_i5
.global __arg_addrb_i6
.global __arg_addrb_i7
.global __arg_addrb_i8
.global __arg_addrb_i9
.global __arg_addrb_i10
.global __arg_addrb_i11
.global __arg_addrb_i12
.global __arg_addrb_i13
.global __arg_addrb_i14
.global __arg_addrb_i15

.section ".text.__var_addr_i0", "ax", @progbits
__var_addr_i0:
  LDA #__i0
  JMP __var_addr

.section ".text.__var_addr_i1", "ax", @progbits
__var_addr_i1:
  LDA #__i1
  JMP __var_addr

.section ".text.__var_addr_i2", "ax", @progbits
__var_addr_i2:
  LDA #__i2
  JMP __var_addr

.section ".text.__var_addr_i3", "ax", @progbits
__var_addr_i3:
  LDA #__i3
  JMP __var_addr

.section ".text.__var_addr_i4", "ax", @progbits
__var_addr_i4:
  LDA #__i4
  JMP __var_addr

.section ".text.__var_addr_i5", "ax", @progbits
__var_addr_i5:
  LDA #__i5
  JMP __var_addr

.section ".text.__var_addr_i6", "ax", @progbits
__var_addr_i6:
  LDA #__i6
  JMP __var_addr

.section ".text.__var_addr_i7", "ax", @progbits
__var_addr_i7:
  LDA #__i7
  JMP __var_addr

.section ".text.__var_addr_i8", "ax", @progbits
__var_addr_i8:
  LDA #__i8
  JMP __var_addr

.section ".text.__var_addrb_i0", "ax", @progbits
__var_addrb_i0:
  LDA #__i0
  JMP __var_addrb

.section ".text.__var_addrb_i1", "ax", @progbits
__var_addrb_i1:
  LDA #__i1
  JMP __var_addrb

.section ".text.__var_addrb_i2", "ax", @progbits
__var_addrb_i2:
  LDA #__i2
  JMP __var_addrb

.section ".text.__var_addrb_i3", "ax", @progbits
__var_addrb_i3:
  LDA #__i3
  JMP __var_addrb

.section ".text.__var_addrb_i4", "ax", @progbits
__var_addrb_i4:
  LDA #__i4
  JMP __var_addrb

.section ".text.__var_addrb_i5", "ax", @progbits
__var_addrb_i5:
  LDA #__i5
  JMP __var_addrb

.section ".text.__var_addrb_i6", "ax", @progbits
__var_addrb_i6:
  LDA #__i6
  JMP __var_addrb

.section ".text.__var_addrb_i7", "ax", @progbits
__var_addrb_i7:
  LDA #__i7
  JMP __var_addrb

.section ".text.__var_addrb_i8", "ax", @progbits
__var_addrb_i8:
  LDA #__i8
  JMP __var_addrb

.section ".text.__var_addr", "ax", @progbits
__var_addr:
  LDY #0
  
  // A = dest offset into zero page.
  // X = var_offset lo
  // Y = var_offset hi
.section ".text.__var_addrb", "ax", @progbits
__var_addrb:
  SEC
  STX __t0
  STY __t1
  TAY
  LDA __fp
  SBC __t0
  STA 0, Y
  LDA __fp+1
  SBC __t1
  STA 1, Y
  RTS

.section ".text.__var_addr_i9", "ax", @progbits
__var_addr_i9:
  LDA #__i9
  JMP __var_addr

.section ".text.__var_addr_i10", "ax", @progbits
__var_addr_i10:
  LDA #__i10
  JMP __var_addr

.section ".text.__var_addr_i11", "ax", @progbits
__var_addr_i11:
  LDA #__i11
  JMP __var_addr

.section ".text.__var_addr_i12", "ax", @progbits
__var_addr_i12:
  LDA #__i12
  JMP __var_addr

.section ".text.__var_addr_i13", "ax", @progbits
__var_addr_i13:
  LDA #__i13
  JMP __var_addr

.section ".text.__var_addr_i14", "ax", @progbits
__var_addr_i14:
  LDA #__i14
  JMP __var_addr

.section ".text.__var_addr_i15", "ax", @progbits
__var_addr_i15:
  LDA #__i15
  JMP __var_addr

.section ".text.__var_addrb_i9", "ax", @progbits
__var_addrb_i9:
  LDA #__i9
  JMP __var_addrb

.section ".text.__var_addrb_i10", "ax", @progbits
__var_addrb_i10:
  LDA #__i10
  JMP __var_addrb

.section ".text.__var_addrb_i11", "ax", @progbits
__var_addrb_i11:
  LDA #__i11
  JMP __var_addrb

.section ".text.__var_addrb_i12", "ax", @progbits
__var_addrb_i12:
  LDA #__i12
  JMP __var_addrb

.section ".text.__var_addrb_i13", "ax", @progbits
__var_addrb_i13:
  LDA #__i13
  JMP __var_addrb

.section ".text.__var_addrb_i14", "ax", @progbits
__var_addrb_i14:
  LDA #__i14
  JMP __var_addrb

.section ".text.__var_addrb_i15", "ax", @progbits
__var_addrb_i15:
  LDA #__i15
  JMP __var_addrb


.section ".text.__arg_addr_i0", "ax", @progbits
__arg_addr_i0:
  LDA #__i0
  JMP __arg_addr

.section ".text.__arg_addr_i1", "ax", @progbits
__arg_addr_i1:
  LDA #__i1
  JMP __arg_addr

.section ".text.__arg_addr_i2", "ax", @progbits
__arg_addr_i2:
  LDA #__i2
  JMP __arg_addr

.section ".text.__arg_addr_i3", "ax", @progbits
__arg_addr_i3:
  LDA #__i3
  JMP __arg_addr

.section ".text.__arg_addr_i4", "ax", @progbits
__arg_addr_i4:
  LDA #__i4
  JMP __arg_addr

.section ".text.__arg_addr_i5", "ax", @progbits
__arg_addr_i5:
  LDA #__i5
  JMP __arg_addr

.section ".text.__arg_addr_i6", "ax", @progbits
__arg_addr_i6:
  LDA #__i6
  JMP __arg_addr

.section ".text.__arg_addr_i7", "ax", @progbits
__arg_addr_i7:
  LDA #__i7
  JMP __arg_addr

.section ".text.__arg_addr_i8", "ax", @progbits
__arg_addr_i8:
  LDA #__i8
  JMP __arg_addr

.section ".text.__arg_addrb_i0", "ax", @progbits
__arg_addrb_i0:
  LDA #__i0
  JMP __arg_addrb

.section ".text.__arg_addrb_i1", "ax", @progbits
__arg_addrb_i1:
  LDA #__i1
  JMP __arg_addrb

.section ".text.__arg_addrb_i2", "ax", @progbits
__arg_addrb_i2:
  LDA #__i2
  JMP __arg_addrb

.section ".text.__arg_addrb_i3", "ax", @progbits
__arg_addrb_i3:
  LDA #__i3
  JMP __arg_addrb

.section ".text.__arg_addrb_i4", "ax", @progbits
__arg_addrb_i4:
  LDA #__i4
  JMP __arg_addrb

.section ".text.__arg_addrb_i5", "ax", @progbits
__arg_addrb_i5:
  LDA #__i5
  JMP __arg_addrb

.section ".text.__arg_addrb_i6", "ax", @progbits
__arg_addrb_i6:
  LDA #__i6
  JMP __arg_addrb

.section ".text.__arg_addrb_i7", "ax", @progbits
__arg_addrb_i7:
  LDA #__i7
  JMP __arg_addrb

.section ".text.__arg_addrb_i8", "ax", @progbits
__arg_addrb_i8:
  LDA #__i8
  JMP __arg_addrb

.section ".text.__arg_addr", "ax", @progbits
__arg_addr:
  LDY #0
  
  // A = dest offset into zero page.
  // X = var_offset lo
  // Y = var_offset hi
.section ".text.__arg_addrb", "ax", @progbits
__arg_addrb:
  CLC
  STX __t0
  STY __t1
  TAY
  LDA __fp
  ADC __t0
  STA 0, Y
  LDA __fp+1
  ADC __t1
  STA 1, Y
  RTS

.section ".text.__arg_addr_i9", "ax", @progbits
__arg_addr_i9:
  LDA #__i9
  JMP __arg_addr

.section ".text.__arg_addr_i10", "ax", @progbits
__arg_addr_i10:
  LDA #__i10
  JMP __arg_addr

.section ".text.__arg_addr_i11", "ax", @progbits
__arg_addr_i11:
  LDA #__i11
  JMP __arg_addr

.section ".text.__arg_addr_i12", "ax", @progbits
__arg_addr_i12:
  LDA #__i12
  JMP __arg_addr

.section ".text.__arg_addr_i13", "ax", @progbits
__arg_addr_i13:
  LDA #__i13
  JMP __arg_addr

.section ".text.__arg_addr_i14", "ax", @progbits
__arg_addr_i14:
  LDA #__i14
  JMP __arg_addr

.section ".text.__arg_addr_i15", "ax", @progbits
__arg_addr_i15:
  LDA #__i15
  JMP __arg_addr

.section ".text.__arg_addrb_i9", "ax", @progbits
__arg_addrb_i9:
  LDA #__i9
  JMP __arg_addrb

.section ".text.__arg_addrb_i10", "ax", @progbits
__arg_addrb_i10:
  LDA #__i10
  JMP __arg_addrb

.section ".text.__arg_addrb_i11", "ax", @progbits
__arg_addrb_i11:
  LDA #__i11
  JMP __arg_addrb

.section ".text.__arg_addrb_i12", "ax", @progbits
__arg_addrb_i12:
  LDA #__i12
  JMP __arg_addrb

.section ".text.__arg_addrb_i13", "ax", @progbits
__arg_addrb_i13:
  LDA #__i13
  JMP __arg_addrb

.section ".text.__arg_addrb_i14", "ax", @progbits
__arg_addrb_i14:
  LDA #__i14
  JMP __arg_addrb

.section ".text.__arg_addrb_i15", "ax", @progbits
__arg_addrb_i15:
  LDA #__i15
  JMP __arg_addrb

.section ".text.__var_addr_xy", "ax", @progbits
__var_addr_xy:
  LDY #0
  
  // X = var_offset lo
  // Y = var_offset hi
  // Result:
  // X = var address LO
  // Y = var address HI
.section ".text.__var_addrb_xy", "ax", @progbits
__var_addrb_xy:
  SEC
  STX __t0
  STY __t1
  LDA __fp
  SBC __t0
  TAX
  LDA __fp+1
  SBC __t1
  TAY
  RTS

.section ".text.__arg_addr_xy", "ax", @progbits
__arg_addr_xy:
  LDY #0
  
  // X = arg_offset lo
  // Y = arg_offset hi
  // Result:
  // X = arg address LO
  // Y = arg address HI
.section ".text.__arg_addrb_xy", "ax", @progbits
__arg_addrb_xy:
  CLC
  TXA
  ADC __fp
  TAX
  TYA
  ADC __fp+1
  TAY
  RTS

// Push the address of a variable or argument directly onto the software
// stack. A is the destination zero-page register. These retain the same
// register and scratch state as __*_addr followed by __pushreg2.
.section ".text.__var_addr_push_i0", "ax", @progbits
__var_addr_push_i0:
  LDA #__i0
  JMP __var_addr_push

.section ".text.__var_addr_push_i1", "ax", @progbits
__var_addr_push_i1:
  LDA #__i1
  JMP __var_addr_push

.section ".text.__var_addr_push_i2", "ax", @progbits
__var_addr_push_i2:
  LDA #__i2
  JMP __var_addr_push

.section ".text.__var_addr_push_i3", "ax", @progbits
__var_addr_push_i3:
  LDA #__i3
  JMP __var_addr_push

.section ".text.__var_addr_push_i4", "ax", @progbits
__var_addr_push_i4:
  LDA #__i4
  JMP __var_addr_push

.section ".text.__var_addr_push_i5", "ax", @progbits
__var_addr_push_i5:
  LDA #__i5
  JMP __var_addr_push

.section ".text.__var_addr_push_i6", "ax", @progbits
__var_addr_push_i6:
  LDA #__i6
  JMP __var_addr_push

.section ".text.__var_addr_push_i7", "ax", @progbits
__var_addr_push_i7:
  LDA #__i7
  JMP __var_addr_push

.section ".text.__var_addr_push_i8", "ax", @progbits
__var_addr_push_i8:
  LDA #__i8
  JMP __var_addr_push

.section ".text.__var_addr_push_i9", "ax", @progbits
__var_addr_push_i9:
  LDA #__i9
  JMP __var_addr_push

.section ".text.__var_addr_push_i10", "ax", @progbits
__var_addr_push_i10:
  LDA #__i10
  JMP __var_addr_push

.section ".text.__var_addr_push_i11", "ax", @progbits
__var_addr_push_i11:
  LDA #__i11
  JMP __var_addr_push

.section ".text.__var_addr_push_i12", "ax", @progbits
__var_addr_push_i12:
  LDA #__i12
  JMP __var_addr_push

.section ".text.__var_addr_push_i13", "ax", @progbits
__var_addr_push_i13:
  LDA #__i13
  JMP __var_addr_push

.section ".text.__var_addr_push_i14", "ax", @progbits
__var_addr_push_i14:
  LDA #__i14
  JMP __var_addr_push

.section ".text.__var_addr_push_i15", "ax", @progbits
__var_addr_push_i15:
  LDA #__i15
  JMP __var_addr_push

.section ".text.__var_addr_push", "ax", @progbits
__var_addr_push:
  JSR __var_addr
  TYA
  TAX
  JMP __pushreg2

.section ".text.__var_addrb_push_i0", "ax", @progbits
__var_addrb_push_i0:
  LDA #__i0
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i1", "ax", @progbits
__var_addrb_push_i1:
  LDA #__i1
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i2", "ax", @progbits
__var_addrb_push_i2:
  LDA #__i2
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i3", "ax", @progbits
__var_addrb_push_i3:
  LDA #__i3
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i4", "ax", @progbits
__var_addrb_push_i4:
  LDA #__i4
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i5", "ax", @progbits
__var_addrb_push_i5:
  LDA #__i5
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i6", "ax", @progbits
__var_addrb_push_i6:
  LDA #__i6
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i7", "ax", @progbits
__var_addrb_push_i7:
  LDA #__i7
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i8", "ax", @progbits
__var_addrb_push_i8:
  LDA #__i8
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i9", "ax", @progbits
__var_addrb_push_i9:
  LDA #__i9
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i10", "ax", @progbits
__var_addrb_push_i10:
  LDA #__i10
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i11", "ax", @progbits
__var_addrb_push_i11:
  LDA #__i11
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i12", "ax", @progbits
__var_addrb_push_i12:
  LDA #__i12
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i13", "ax", @progbits
__var_addrb_push_i13:
  LDA #__i13
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i14", "ax", @progbits
__var_addrb_push_i14:
  LDA #__i14
  JMP __var_addrb_push

.section ".text.__var_addrb_push_i15", "ax", @progbits
__var_addrb_push_i15:
  LDA #__i15
  JMP __var_addrb_push

.section ".text.__var_addrb_push", "ax", @progbits
__var_addrb_push:
  JSR __var_addrb
  TYA
  TAX
  JMP __pushreg2

.section ".text.__arg_addr_push", "ax", @progbits
__arg_addr_push:
  JSR __arg_addr
  TYA
  TAX
  JMP __pushreg2

.section ".text.__arg_addrb_push", "ax", @progbits
__arg_addrb_push:
  JSR __arg_addrb
  TYA
  TAX
  JMP __pushreg2

// Address of var with offset in X,Y.  Result in t0,t1.  Saves A
.section ".text.__varaddr", "ax", @progbits
__varaddr:
  PHA
  STX __t0
  STY __t1
  SEC
  LDA __fp
  SBC __t0
  STA __t0
  LDA __fp+1
  SBC __t1
  STA __t1
  PLA
  RTS

// Address of arg with offset in X,Y.  Result in t0,t1.
.section ".text.__argaddr", "ax", @progbits
__argaddr:
  PHA
  STX __t0
  STY __t1
  CLC
  LDA __fp
  ADC __t0
  STA __t0
  LDA __fp+1
  ADC __t1
  STA __t1
  PLA
  RTS
