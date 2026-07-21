#include "vars.s"

.text

// Store eight bytes through a pointer held in a zero-page register.
// X: zero-page offset of the eight-byte source register.
// Y: zero-page offset of the two-byte address register.
// Clobbers A, X, Y, __t0, __t1, and __t2.
.global __store_indirect8
__store_indirect8:
  STX __t2
  TYA
  TAX
  LDA 0,X
  STA __t0
  LDA 1,X
  STA __t1
  LDX __t2
  LDY #0
store_indirect8_loop:
  LDA 0,X
  STA (__t0),Y
  INX
  INY
  CPY #8
  BNE store_indirect8_loop
  RTS
