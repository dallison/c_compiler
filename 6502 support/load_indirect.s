#include "vars.s"

.text

// Load four bytes through a pointer held in a zero-page register.
// X: zero-page offset of the two-byte address register.
// Y: zero-page offset of the four-byte destination register.
// Clobbers A, X, Y, __t0, __t1, and __t2.
.global __load_indirect4
__load_indirect4:
  STY __t2
  LDA 0,X
  STA __t0
  LDA 1,X
  STA __t1
  LDX __t2
  LDY #0
load_indirect4_loop:
  LDA (__t0),Y
  STA 0,X
  INX
  INY
  CPY #4
  BNE load_indirect4_loop
  RTS
