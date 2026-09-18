#include "vars.s"

// Functions are emitted in per-symbol ELF sections.

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

.section ".text.__inc1", "ax", @progbits
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

.section ".text.__rinc1", "ax", @progbits
__rinc1:
  TAX
  CLC
  LDA 0,X
  ADC #1
  STA 0,X
  RTS

.section ".text.__inc21", "ax", @progbits
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
.section ".text.__rinc21", "ax", @progbits
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


.section ".text.__inc2", "ax", @progbits
__inc2:
  LDY #0
  JMP __inc2b
.section ".text.__inc2b", "ax", @progbits
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

.section ".text.__rinc2", "ax", @progbits
__rinc2:
  LDY #0
  JMP __rinc2b
.section ".text.__rinc2b", "ax", @progbits
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

.section ".text.__inc4", "ax", @progbits
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

.section ".text.__rinc4", "ax", @progbits
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


.section ".text.__inc8", "ax", @progbits
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

.section ".text.__rinc8", "ax", @progbits
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

.section ".text.__incf", "ax", @progbits
__incf:
__incfb:
.section ".text.__incd", "ax", @progbits
__incd:
__incdb:
.section ".text.__rincf", "ax", @progbits
__rincf:
__rincfb:
.section ".text.__rincd", "ax", @progbits
__rincd:
__rincdb:
RTS

.section ".text.__dec1", "ax", @progbits
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

.section ".text.__rdec1", "ax", @progbits
__rdec1:
  TAX
  SEC
  LDA 0,X
  SBC #1
  STA 0,X
  RTS

.section ".text.__dec21", "ax", @progbits
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

.section ".text.__rdec21", "ax", @progbits
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

.section ".text.__dec2", "ax", @progbits
__dec2:
  LDY #0
  JMP __dec2b
.section ".text.__dec2b", "ax", @progbits
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

.section ".text.__rdec2", "ax", @progbits
__rdec2:
  LDY #0
  JMP __rdec2b
.section ".text.__rdec2b", "ax", @progbits
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

.section ".text.__dec4", "ax", @progbits
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

.section ".text.__rdec4", "ax", @progbits
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

.section ".text.__dec8", "ax", @progbits
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

.section ".text.__rdec8", "ax", @progbits
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

.section ".text.__decf", "ax", @progbits
__decf:
__decfb:
.section ".text.__decd", "ax", @progbits
__decd:
__decdb:
.section ".text.__rdecf", "ax", @progbits
__rdecf:
__rdecfb:
.section ".text.__rdecd", "ax", @progbits
__rdecd:
__rdecdb:
  RTS
