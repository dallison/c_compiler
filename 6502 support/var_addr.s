#include "vars.s"

.text

.global __var_addr
.global __var_addrb
.global __arg_addr
.global __arg_addrb
.global __var_addr_xy
.global __var_addrb_xy
.global __arg_addr_xy
.global __arg_addrb_xy
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

__var_addr_i0:
  LDA #__i0
  BRA __var_addr

__var_addr_i1:
  LDA #__i1
  BRA __var_addr

__var_addr_i2:
  LDA #__i2
  BRA __var_addr

__var_addr_i3:
  LDA #__i3
  BRA __var_addr

__var_addr_i4:
  LDA #__i4
  BRA __var_addr

__var_addr_i5:
  LDA #__i5
  BRA __var_addr

__var_addr_i6:
  LDA #__i6
  BRA __var_addr

__var_addr_i7:
  LDA #__i7
  BRA __var_addr

__var_addr_i8:
  LDA #__i8
  BRA __var_addr

__var_addrb_i0:
  LDA #__i0
  BRA __var_addrb

__var_addrb_i1:
  LDA #__i1
  BRA __var_addrb

__var_addrb_i2:
  LDA #__i2
  BRA __var_addrb

__var_addrb_i3:
  LDA #__i3
  BRA __var_addrb

__var_addrb_i4:
  LDA #__i4
  BRA __var_addrb

__var_addrb_i5:
  LDA #__i5
  BRA __var_addrb

__var_addrb_i6:
  LDA #__i6
  BRA __var_addrb

__var_addrb_i7:
  LDA #__i7
  BRA __var_addrb

__var_addrb_i8:
  LDA #__i8
  BRA __var_addrb

__var_addr:
  LDY #0
  
  // A = dest offset into zero page.
  // X = var_offset lo
  // Y = var_offset hi
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

__var_addr_i9:
  LDA #__i9
  BRA __var_addr

__var_addr_i10:
  LDA #__i10
  BRA __var_addr

__var_addr_i11:
  LDA #__i11
  BRA __var_addr

__var_addr_i12:
  LDA #__i12
  BRA __var_addr

__var_addr_i13:
  LDA #__i13
  BRA __var_addr

__var_addr_i14:
  LDA #__i14
  BRA __var_addr

__var_addr_i15:
  LDA #__i15
  BRA __var_addr

__var_addrb_i9:
  LDA #__i9
  BRA __var_addrb

__var_addrb_i10:
  LDA #__i10
  BRA __var_addrb

__var_addrb_i11:
  LDA #__i11
  BRA __var_addrb

__var_addrb_i12:
  LDA #__i12
  BRA __var_addrb

__var_addrb_i13:
  LDA #__i13
  BRA __var_addrb

__var_addrb_i14:
  LDA #__i14
  BRA __var_addrb

__var_addrb_i15:
  LDA #__i15
  BRA __var_addrb


__arg_addr_i0:
  LDA #__i0
  BRA __arg_addr

__arg_addr_i1:
  LDA #__i1
  BRA __arg_addr

__arg_addr_i2:
  LDA #__i2
  BRA __arg_addr

__arg_addr_i3:
  LDA #__i3
  BRA __arg_addr

__arg_addr_i4:
  LDA #__i4
  BRA __arg_addr

__arg_addr_i5:
  LDA #__i5
  BRA __arg_addr

__arg_addr_i6:
  LDA #__i6
  BRA __arg_addr

__arg_addr_i7:
  LDA #__i7
  BRA __arg_addr

__arg_addr_i8:
  LDA #__i8
  BRA __arg_addr

__arg_addrb_i0:
  LDA #__i0
  BRA __arg_addrb

__arg_addrb_i1:
  LDA #__i1
  BRA __arg_addrb

__arg_addrb_i2:
  LDA #__i2
  BRA __arg_addrb

__arg_addrb_i3:
  LDA #__i3
  BRA __arg_addrb

__arg_addrb_i4:
  LDA #__i4
  BRA __arg_addrb

__arg_addrb_i5:
  LDA #__i5
  BRA __arg_addrb

__arg_addrb_i6:
  LDA #__i6
  BRA __arg_addrb

__arg_addrb_i7:
  LDA #__i7
  BRA __arg_addrb

__arg_addrb_i8:
  LDA #__i8
  BRA __arg_addrb

__arg_addr:
  LDY #0
  
  // A = dest offset into zero page.
  // X = var_offset lo
  // Y = var_offset hi
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

__arg_addr_i9:
  LDA #__i9
  BRA __arg_addr

__arg_addr_i10:
  LDA #__i10
  BRA __arg_addr

__arg_addr_i11:
  LDA #__i11
  BRA __arg_addr

__arg_addr_i12:
  LDA #__i12
  BRA __arg_addr

__arg_addr_i13:
  LDA #__i13
  BRA __arg_addr

__arg_addr_i14:
  LDA #__i14
  BRA __arg_addr

__arg_addr_i15:
  LDA #__i15
  BRA __arg_addr

__arg_addrb_i9:
  LDA #__i9
  BRA __arg_addrb

__arg_addrb_i10:
  LDA #__i10
  BRA __arg_addrb

__arg_addrb_i11:
  LDA #__i11
  BRA __arg_addrb

__arg_addrb_i12:
  LDA #__i12
  BRA __arg_addrb

__arg_addrb_i13:
  LDA #__i13
  BRA __arg_addrb

__arg_addrb_i14:
  LDA #__i14
  BRA __arg_addrb

__arg_addrb_i15:
  LDA #__i15
  BRA __arg_addrb

__var_addr_xy:
  LDY #0
  
  // X = var_offset lo
  // Y = var_offset hi
  // Result:
  // X = var address LO
  // Y = var address HI
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

__arg_addr_xy:
  LDY #0
  
  // X = arg_offset lo
  // Y = arg_offset hi
  // Result:
  // X = arg address LO
  // Y = arg address HI
__arg_addrb_xy:
  CLC
  TXA
  ADC __fp
  TAX
  TYA
  ADC __fp+1
  TAY
  RTS

// Address of var with offset in X,Y.  Result in t0,t1.  Saves A
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
