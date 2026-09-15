#include "vars.s"

.section ".text.__load_indirect8", "ax", @progbits

// Load eight bytes through a pointer held in a zero-page register.
// X: zero-page offset of the two-byte address register.
// Y: zero-page offset of the eight-byte destination register.
// Clobbers A, X, Y, __t0, __t1, and __t2.
.global __load_indirect8
__load_indirect8:
  STY __t2
  LDA 0,X
  STA __t0
  LDA 1,X
  STA __t1
  LDX __t2
  LDY #0
load_indirect8_loop:
  LDA (__t0),Y
  STA 0,X
  INX
  INY
  CPY #8
  BNE load_indirect8_loop
  RTS
