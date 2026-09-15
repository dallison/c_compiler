#include "vars.s"

.section ".text.__store_indirect4", "ax", @progbits

// Store four bytes through a pointer held in a zero-page register.
// X: zero-page offset of the four-byte source register.
// Y: zero-page offset of the two-byte address register.
// Clobbers A, X, Y, __t0, __t1, and __t2.
.global __store_indirect4
__store_indirect4:
  STX __t2
  TYA
  TAX
  LDA 0,X
  STA __t0
  LDA 1,X
  STA __t1
  LDX __t2
  LDY #0
store_indirect4_loop:
  LDA 0,X
  STA (__t0),Y
  INX
  INY
  CPY #4
  BNE store_indirect4_loop
  RTS
