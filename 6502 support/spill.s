#include "vars.s"

// Functions are emitted in per-symbol ELF sections.

.global __spill1
.global __spill2
.global __spill4
.global __spill8
.global __reload1
.global __reload2
.global __reload4
.global __reload8

// Spilling and reloading.
// Entry:
// ret+0: offset into zero page
// ret+1,2: offset subtracted from frame pointer to spill location.
.section ".text.__spill1", "ax", @progbits
__spill1:
  PHP
  PHA
#ifdef __65c02__
  PHX
#else
  STA __nmos_tmp
  TXA
  PHA
  LDA __nmos_tmp
#endif
#ifdef __65c02__
  PHY
#else
  STA __nmos_tmp
  TYA
  PHA
  LDA __nmos_tmp
#endif
  // 4 extra bytes have been pushed onto the stack.
  TSX
  LDA 0x105,X       // Load LO byte
  STA __t2          // Copy to temp addr
  LDA 0x106,X       // Load HI byte
  STA __t3
  INC __t2          // JSR puts return address -1 on stack.
  BNE sp1_skip
  INC __t3
sp1_skip:  // t2,t3 contain address of data for spill
  LDY #1
  LDA (__t2),Y
  TAX
  INY
  LDA (__t2), Y
  TAY
#ifdef __65c02__
  LDA (__t2)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__t2),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  // X,Y: offset to subtract from __fp (spill offset).
  JSR __varaddr
  TAX
  LDA 0,X
#ifdef __65c02__
  STA (__t0)
#else
  PHP
  STY __nmos_tmp
  LDY #0
  STA (__t0),Y
  LDY __nmos_tmp
  PLP
#endif

  // Return to address after data (__t2 + 3)
  // The data is 3 bytes but we set the return address to
  // one byte less than the next instruction as the RTS will add one
  // to it before jumping to it.
  TSX
  CLC
  LDA __t2
  ADC #2
  STA 0x105,X
  LDA __t3
  ADC #0
  STA 0x106,X

#ifdef __65c02__
  PLY
#else
  PLA
  TAY
#endif
#ifdef __65c02__
  PLX
#else
  PLA
  TAX
#endif
  PLA
  PLP
  RTS

.section ".text.__spill2", "ax", @progbits
__spill2:
  PHP
  PHA
#ifdef __65c02__
  PHX
#else
  STA __nmos_tmp
  TXA
  PHA
  LDA __nmos_tmp
#endif
#ifdef __65c02__
  PHY
#else
  STA __nmos_tmp
  TYA
  PHA
  LDA __nmos_tmp
#endif
  // 4 extra bytes have been pushed onto the stack.
  TSX
  LDA 0x105,X       // Load LO byte
  STA __t2          // Copy to temp addr
  LDA 0x106,X       // Load HI byte
  STA __t3
  INC __t2          // JSR puts return address -1 on stack.
  BNE sp2_skip
  INC __t3
sp2_skip:  // t2,t3 contain address of data for spill
  LDY #1
  LDA (__t2),Y
  TAX
  INY
  LDA (__t2), Y
  TAY
#ifdef __65c02__
  LDA (__t2)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__t2),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  // X,Y: offset to subtract from __fp (spill offset).
  JSR __varaddr
  TAX
  LDA 0,X
#ifdef __65c02__
  STA (__t0)
#else
  PHP
  STY __nmos_tmp
  LDY #0
  STA (__t0),Y
  LDY __nmos_tmp
  PLP
#endif
  LDY #1
  LDA 1,X
  STA (__t0),Y

  // Return to address after data (__t2 + 3)
  // The data is 3 bytes but we set the return address to
  // one byte less than the next instruction as the RTS will add one
  // to it before jumping to it.
  TSX
  CLC
  LDA __t2
  ADC #2
  STA 0x105,X
  LDA __t3
  ADC #0
  STA 0x106,X

#ifdef __65c02__
  PLY
#else
  PLA
  TAY
#endif
#ifdef __65c02__
  PLX
#else
  PLA
  TAX
#endif
  PLA
  PLP
  RTS

.section ".text.__spill4", "ax", @progbits
__spill4:
  PHP
  PHA
#ifdef __65c02__
  PHX
#else
  STA __nmos_tmp
  TXA
  PHA
  LDA __nmos_tmp
#endif
#ifdef __65c02__
  PHY
#else
  STA __nmos_tmp
  TYA
  PHA
  LDA __nmos_tmp
#endif
  // 4 extra bytes have been pushed onto the stack.
  TSX
  LDA 0x105,X       // Load LO byte
  STA __t2          // Copy to temp addr
  LDA 0x106,X       // Load HI byte
  STA __t3
  INC __t2          // JSR puts return address -1 on stack.
  BNE sp4_skip
  INC __t3
sp4_skip:  // t2,t3 contain address of data for spill
  LDY #1
  LDA (__t2),Y
  TAX
  INY
  LDA (__t2), Y
  TAY
#ifdef __65c02__
  LDA (__t2)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__t2),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  // X,Y: offset to subtract from __fp (spill offset).
  JSR __varaddr
  TAX
  LDY #0
  spill4loop:
  LDA 0,X
  STA (__t0),Y
  INX
  INY
  CPY #4
  BNE spill4loop

  // Return to address after data (__t2 + 3)
  // The data is 3 bytes but we set the return address to
  // one byte less than the next instruction as the RTS will add one
  // to it before jumping to it.
  TSX
  CLC
  LDA __t2
  ADC #2
  STA 0x105,X
  LDA __t3
  ADC #0
  STA 0x106,X

#ifdef __65c02__
  PLY
#else
  PLA
  TAY
#endif
#ifdef __65c02__
  PLX
#else
  PLA
  TAX
#endif
  PLA
  PLP
  RTS


.section ".text.__spill8", "ax", @progbits
__spill8:
  PHP
  PHA
#ifdef __65c02__
  PHX
#else
  STA __nmos_tmp
  TXA
  PHA
  LDA __nmos_tmp
#endif
#ifdef __65c02__
  PHY
#else
  STA __nmos_tmp
  TYA
  PHA
  LDA __nmos_tmp
#endif
  // 4 extra bytes have been pushed onto the stack.
  TSX
  LDA 0x105,X       // Load LO byte
  STA __t2          // Copy to temp addr
  LDA 0x106,X       // Load HI byte
  STA __t3
  INC __t2          // JSR puts return address -1 on stack.
  BNE sp8_skip
  INC __t3
sp8_skip:  // t2,t3 contain address of data for spill
  LDY #1
  LDA (__t2),Y
  TAX
  INY
  LDA (__t2), Y
  TAY
#ifdef __65c02__
  LDA (__t2)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__t2),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  // X,Y: offset to subtract from __fp (spill offset).
  JSR __varaddr
  TAX
  LDY #0
spill8loop:
  LDA 0,X
  STA (__t0),Y
  INX
  INY
  CPY #8
  BNE spill8loop

  // Return to address after data (__t2 + 3)
  // The data is 3 bytes but we set the return address to
  // one byte less than the next instruction as the RTS will add one
  // to it before jumping to it.
  TSX
  CLC
  LDA __t2
  ADC #2
  STA 0x105,X
  LDA __t3
  ADC #0
  STA 0x106,X

#ifdef __65c02__
  PLY
#else
  PLA
  TAY
#endif
#ifdef __65c02__
  PLX
#else
  PLA
  TAX
#endif
  PLA
  PLP
  RTS


.section ".text.__reload1", "ax", @progbits
__reload1:
  PHP
  PHA
#ifdef __65c02__
  PHX
#else
  STA __nmos_tmp
  TXA
  PHA
  LDA __nmos_tmp
#endif
#ifdef __65c02__
  PHY
#else
  STA __nmos_tmp
  TYA
  PHA
  LDA __nmos_tmp
#endif
  // 4 extra bytes have been pushed onto the stack.
  TSX
  LDA 0x105,X       // Load LO byte
  STA __t2          // Copy to temp addr
  LDA 0x106,X       // Load HI byte
  STA __t3
  INC __t2          // JSR puts return address -1 on stack.
  BNE rl1_skip
  INC __t3
rl1_skip:  // t2,t3 contain address of data for spill
  LDY #1
  LDA (__t2),Y
  TAX
  INY
  LDA (__t2), Y
  TAY
#ifdef __65c02__
  LDA (__t2)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__t2),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  // X,Y: offset to subtract from __fp (spill offset).

  JSR __varaddr
  TAX
#ifdef __65c02__
  LDA (__t0)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__t0),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  STA 0,X

  // Return to address after data (__t2 + 3)
  // The data is 3 bytes but we set the return address to
  // one byte less than the next instruction as the RTS will add one
  // to it before jumping to it.
  TSX
  CLC
  LDA __t2
  ADC #2
  STA 0x105,X
  LDA __t3
  ADC #0
  STA 0x106,X

#ifdef __65c02__
  PLY
#else
  PLA
  TAY
#endif
#ifdef __65c02__
  PLX
#else
  PLA
  TAX
#endif
  PLA
  PLP
  RTS

.section ".text.__reload2", "ax", @progbits
__reload2:
  PHP
  PHA
#ifdef __65c02__
  PHX
#else
  STA __nmos_tmp
  TXA
  PHA
  LDA __nmos_tmp
#endif
#ifdef __65c02__
  PHY
#else
  STA __nmos_tmp
  TYA
  PHA
  LDA __nmos_tmp
#endif
  // 4 extra bytes have been pushed onto the stack.
  TSX
  LDA 0x105,X       // Load LO byte
  STA __t2          // Copy to temp addr
  LDA 0x106,X       // Load HI byte
  STA __t3
  INC __t2          // JSR puts return address -1 on stack.
  BNE rl2_skip
  INC __t3
rl2_skip:  // t2,t3 contain address of data for spill
  LDY #1
  LDA (__t2),Y
  TAX
  INY
  LDA (__t2), Y
  TAY
#ifdef __65c02__
  LDA (__t2)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__t2),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  // X,Y: offset to subtract from __fp (spill offset).

  JSR __varaddr
  TAX
#ifdef __65c02__
  LDA (__t0)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__t0),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  STA 0,X
  LDY #1
  LDA (__t0),Y
  STA 1,X

  // Return to address after data (__t2 + 3)
  // The data is 3 bytes but we set the return address to
  // one byte less than the next instruction as the RTS will add one
  // to it before jumping to it.
  TSX
  CLC
  LDA __t2
  ADC #2
  STA 0x105,X
  LDA __t3
  ADC #0
  STA 0x106,X

#ifdef __65c02__
  PLY
#else
  PLA
  TAY
#endif
#ifdef __65c02__
  PLX
#else
  PLA
  TAX
#endif
  PLA
  PLP
  RTS

.section ".text.__reload4", "ax", @progbits
__reload4:
  PHP
  PHA
#ifdef __65c02__
  PHX
#else
  STA __nmos_tmp
  TXA
  PHA
  LDA __nmos_tmp
#endif
#ifdef __65c02__
  PHY
#else
  STA __nmos_tmp
  TYA
  PHA
  LDA __nmos_tmp
#endif
  // 4 extra bytes have been pushed onto the stack.
  TSX
  LDA 0x105,X       // Load LO byte
  STA __t2          // Copy to temp addr
  LDA 0x106,X       // Load HI byte
  STA __t3
  INC __t2          // JSR puts return address -1 on stack.
  BNE rl4_skip
  INC __t3
rl4_skip:  // t2,t3 contain address of data for spill
  LDY #1
  LDA (__t2),Y
  TAX
  INY
  LDA (__t2), Y
  TAY
#ifdef __65c02__
  LDA (__t2)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__t2),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  // X,Y: offset to subtract from __fp (spill offset).


  JSR __varaddr
  TAX
  LDY #0
reload4loop:
  LDA (__t0),Y
  STA 0,X
  INX
  INY
  CPY #4
  BNE reload4loop

  // Return to address after data (__t2 + 3)
  // The data is 3 bytes but we set the return address to
  // one byte less than the next instruction as the RTS will add one
  // to it before jumping to it.
  TSX
  CLC
  LDA __t2
  ADC #2
  STA 0x105,X
  LDA __t3
  ADC #0
  STA 0x106,X

#ifdef __65c02__
  PLY
#else
  PLA
  TAY
#endif
#ifdef __65c02__
  PLX
#else
  PLA
  TAX
#endif
  PLA
  PLP
  RTS

.section ".text.__reload8", "ax", @progbits
__reload8:
  PHP
  PHA
#ifdef __65c02__
  PHX
#else
  STA __nmos_tmp
  TXA
  PHA
  LDA __nmos_tmp
#endif
#ifdef __65c02__
  PHY
#else
  STA __nmos_tmp
  TYA
  PHA
  LDA __nmos_tmp
#endif
  // 4 extra bytes have been pushed onto the stack.
  TSX
  LDA 0x105,X       // Load LO byte
  STA __t2          // Copy to temp addr
  LDA 0x106,X       // Load HI byte
  STA __t3
  INC __t2          // JSR puts return address -1 on stack.
  BNE rl8_skip
  INC __t3
rl8_skip:  // t2,t3 contain address of data for spill
  LDY #1
  LDA (__t2),Y
  TAX
  INY
  LDA (__t2), Y
  TAY
#ifdef __65c02__
  LDA (__t2)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__t2),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
  // X,Y: offset to subtract from __fp (spill offset).

  JSR __varaddr
  TAX
  LDY #0
reload8loop:
  LDA (__t0),Y
  STA 0,X
  INX
  INY
  CPY #8
  BNE reload8loop

  // Return to address after data (__t2 + 3)
  // The data is 3 bytes but we set the return address to
  // one byte less than the next instruction as the RTS will add one
  // to it before jumping to it.
  TSX
  CLC
  LDA __t2
  ADC #2
  STA 0x105,X
  LDA __t3
  ADC #0
  STA 0x106,X

#ifdef __65c02__
  PLY
#else
  PLA
  TAY
#endif
#ifdef __65c02__
  PLX
#else
  PLA
  TAX
#endif
  PLA
  PLP
  RTS


