#include "vars.s"

.text

.global __pullreg2_drop2

// Pull the top two-byte result into register X and discard the following
// two-byte argument in one operation.
// X: index of the two-byte register to pull into.
__pullreg2_drop2:
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
  ADC #4
  STA __sp
  LDA __sp+1
  ADC #0
  STA __sp+1
  RTS
