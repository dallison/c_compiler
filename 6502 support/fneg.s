#include "vars.s"
#include "fp.s"

.text

__fneg:
  TAY
  LDA 0,X
  STA 0,Y
  LDA 1,X
  STA 1,Y
  LDA 2,X
  STA 2,Y
  LDA 3,X
  EOR #0x80     // Flip sign bit.
  STA 3,Y
  RTS

__dneg:
  RTS


