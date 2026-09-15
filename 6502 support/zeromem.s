#include "vars.s"

// Functions are emitted in per-symbol ELF sections.

.global __zeromem1
.global __zeromem2
.global __load_inline_mem_params

.section ".text.__zeromem1", "ax", @progbits
__zeromem1:
  LDA #1
  JSR __load_inline_mem_params

  LDY #0
  TYA
zm1l:
  CPY __mem_size
  BEQ end_zm1
  STA (__mem_dest),Y
  INY
  BNE zm1l
end_zm1:
  RTS

.section ".text.__zeromem2", "ax", @progbits
__zeromem2:
  LDA #5
  JSR __load_inline_mem_params

  LDA #0
zeromem2_large_page_loop:
  LDX __mem_size+1
  BEQ zeromem2_small

  LDY #0
zeromem2_large_loop:
  STA (__mem_dest),Y
  INY
  BNE zeromem2_large_loop

  DEC __mem_size+1
  INC __mem_dest+1
  BRA zeromem2_large_page_loop

zeromem2_small:
  LDX __mem_size
  BEQ end_zm2
  LDY #0
zeromem2_small_loop:
  STA (__mem_dest),Y
  INY
  CPY __mem_size
  BNE zeromem2_small_loop
end_zm2:
  RTS
