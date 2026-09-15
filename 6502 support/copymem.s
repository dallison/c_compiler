#include "vars.s"

// Functions are emitted in per-symbol ELF sections.

.global __copymem1
.global __copymem2
.global __load_inline_mem_params

.section ".text.__copymem2", "ax", @progbits
__copymem2:
  LDA #7
  JSR __load_inline_mem_params

copymem2_large_page_loop:
  LDX __mem_size+1
  BEQ copymem2_small

  // Copy one page from __mem_src to __mem_dest.
  LDY #0
copymem2_large_loop:
  LDA (__mem_src),Y
  STA (__mem_dest),Y
  INY
  BNE copymem2_large_loop

  DEC __mem_size+1
  INC __mem_src+1
  INC __mem_dest+1
  BRA copymem2_large_page_loop

copymem2_small:
  LDX __mem_size
  BEQ end_cm2
  LDY #0
copymem2_small_loop:
  LDA (__mem_src),Y
  STA (__mem_dest),Y
  INY
  CPY __mem_size
  BNE copymem2_small_loop
end_cm2:
  RTS

.section ".text.__copymem1", "ax", @progbits
__copymem1:
  LDA #3
  JSR __load_inline_mem_params

  LDY #0
cm1l:
  CPY __mem_size
  BEQ end_cm1
  LDA (__mem_src),Y
  STA (__mem_dest),Y
  INY
  BNE cm1l
end_cm1:
  RTS
