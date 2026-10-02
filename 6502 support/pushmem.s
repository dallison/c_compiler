#include "vars.s"

// Functions are emitted in per-symbol ELF sections.

.global __pushmem1
.global __pushmem2
.global __pushmem_xy1
.global __pushmem_xy2
.global __load_inline_mem_params

.section ".text.__pushmem_xy1", "ax", @progbits
__pushmem_xy1:
  STX __mem_src
  STY __mem_src+1
  LDA #0
  JSR __load_inline_mem_params
#ifdef __65c02__
  BRA pushmem1_start
#else
  JMP pushmem1_start
#endif

.section ".text.__pushmem1", "ax", @progbits
__pushmem1:
  LDA #2
  JSR __load_inline_mem_params

pushmem1_start:
  SEC
  LDA __sp
  SBC __mem_size
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1

  LDY #0
pm1l:
  CPY __mem_size
  BEQ end_pm1
  LDA (__mem_src),Y
  STA (__sp),Y
  INY
  BNE pm1l
end_pm1:
  RTS

.section ".text.__pushmem_xy2", "ax", @progbits
__pushmem_xy2:
  STX __mem_src
  STY __mem_src+1
  LDA #4
  JSR __load_inline_mem_params
#ifdef __65c02__
  BRA pushmem2_start
#else
  JMP pushmem2_start
#endif

.section ".text.__pushmem2", "ax", @progbits
__pushmem2:
  LDA #6
  JSR __load_inline_mem_params

pushmem2_start:
  SEC
  LDA __sp
  SBC __mem_size
  STA __sp
  PHA
  LDA __sp+1
  SBC __mem_size+1
  STA __sp+1
  PHA

#ifdef __65c02__
  STZ __t0
  STZ __t1
#else
  PHA
  LDA #0
  STA __t0
  STA __t1
  STA __nmos_tmp
  PLA
  BIT __nmos_tmp
#endif
pml:
  LDA __t0
  CMP __mem_size
  BNE pml1
  LDA __t1
  CMP __mem_size+1
  BEQ end_pm
pml1:
#ifdef __65c02__
  LDA (__mem_src)
#else
  STY __nmos_tmp
  LDY #0
  LDA (__mem_src),Y
  PHA
  LDA __nmos_tmp
  TAY
  PLA
#endif
#ifdef __65c02__
  STA (__sp)
#else
  PHP
  STY __nmos_tmp
  LDY #0
  STA (__sp),Y
  LDY __nmos_tmp
  PLP
#endif

  INC __mem_src
  BNE pms
  INC __mem_src+1
pms:
  INC __sp
  BNE pmd
  INC __sp+1
pmd:
  INC __t0
  BNE pml
  INC __t1
  JMP pml
end_pm:
  PLA
  STA __sp+1
  PLA
  STA __sp
  RTS
