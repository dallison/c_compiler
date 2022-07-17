#include "vars.s"

.text

.global __inc1
.global __inc21
.global __inc2
.global __inc2b
.global __inc4
.global __inc8
.global __incf
.global __incd

.global __rinc1
.global __rinc21
.global __rinc2
.global __rinc2b
.global __rinc4
.global __rinc8
.global __rincf
.global __rincd

.global __dec1
.global __dec21
.global __dec2
.global __dec2b
.global __dec4
.global __dec8
.global __decf
.global __decd

.global __rdec1
.global __rdec21
.global __rdec2
.global __rdec2b
.global __rdec4
.global __rdec8
.global __rdecf
.global __rdecd

__inc1:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  CLC
  LDA (__t2)
  ADC #1
  STA (__t2)
  RTS

__rinc1:
  TAX
  CLC
  LDA 0,X
  ADC #1
  STA 0,X
  RTS

__inc21:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  CLC
  LDA (__t2)
  ADC #1
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  RTS

// Increment register in X by 1
__rinc21:
  TAX
  CLC
  LDA 0,X
  ADC #1
  STA 0,X
  LDA 1,X
  ADC #0
  STA 1,X
  RTS


__inc2:
  LDY #0
__inc2b:
  STX __t0
  STY __t1
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  CLC
  LDA (__t2)
  ADC __t0
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  ADC __t1
  STA (__t2), Y
  RTS

__rinc2:
  LDY #0
__rinc2b:
  STX __t0
  STY __t1
  TAX
  CLC
  LDA 0,X
  ADC __t0
  STA 0,X
  LDA 1,X
  ADC __t1
  STA 1,X
  RTS

__inc4:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  CLC
  LDA (__t2)
  ADC #1
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  RTS

__rinc4:
  TAX
  CLC
  LDA 0,X
  ADC #1
  STA 0,X
  LDA 1,X
  ADC #0
  STA 1,X
  LDA 2,X
  ADC #0
  STA 2,X
  LDA 3,X
  ADC #0
  STA 3,X
  RTS


__inc8:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  CLC
  LDA (__t2)
  ADC #1
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  RTS

__rinc8:
  TAX
  CLC
  LDA 0,X
  ADC #1
  STA 0,X
  LDA 1,X
  ADC #0
  STA 1,X
  LDA 2,X
  ADC #0
  STA 2,X
  LDA 3,X
  ADC #0
  STA 3,X
  LDA 4,X
  ADC #0
  STA 4,X
  LDA 5,X
  ADC #0
  STA 5,X
  LDA 6,X
  ADC #0
  STA 6,X
  LDA 7,X
  ADC #0
  STA 7,X
  RTS

__incf:
__incfb:
__incd:
__incdb:
__rincf:
__rincfb:
__rincd:
__rincdb:
RTS

__dec1:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  SEC
  LDA (__t2)
  SBC #1
  STA (__t2)
  RTS

__rdec1:
  TAX
  SEC
  LDA 0,X
  SBC #1
  STA 0,X
  RTS

__dec21:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  SEC
  LDA (__t2)
  SBC #1
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  RTS

__rdec21:
  TAX
  SEC
  LDA 0,X
  SBC #1
  STA 0,X
  LDA 1,X
  SBC #0
  STA 1,X
  RTS

__dec2:
  LDY #0
__dec2b:
  STX __t0
  STY __t1
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  SEC
  LDA (__t2)
  SBC __t0
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  SBC __t1
  STA (__t2), Y
  RTS

__rdec2:
  LDY #0
__rdec2b:
  STX __t0
  STY __t1
  TAX
  SEC
  LDA 0,X
  SBC __t0
  STA 0,X
  LDA 1,X
  SBC __t1
  STA 1,X
  RTS

__dec4:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  SEC
  LDA (__t2)
  SBC #1
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  RTS

__rdec4:
  TAX
  SEC
  LDA 0,X
  SBC #1
  STA 0,X
  LDA 1,X
  SBC #0
  STA 1,X
  LDA 2,X
  SBC #0
  STA 2,X
  LDA 3,X
  SBC #0
  STA 3,X
  RTS

__dec8:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  SEC
  LDA (__t2)
  SBC #1
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  RTS

__rdec8:
  TAX
  SEC
  LDA 0,X
  SBC #1
  STA 0,X
  LDA 1,X
  SBC #0
  STA 1,X
  LDA 2,X
  SBC #0
  STA 2,X
  LDA 3,X
  SBC #0
  STA 3,X
  LDA 4,X
  SBC #0
  STA 4,X
  LDA 5,X
  SBC #0
  STA 5,X
  LDA 6,X
  SBC #0
  STA 6,X
  LDA 7,X
  SBC #0
  STA 7,X
  RTS

__decf:
__decfb:
__decd:
__decdb:
__rdecf:
__rdecfb:
__rdecd:
__rdecdb:
  RTS
