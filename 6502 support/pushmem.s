#include "vars.s"

.text

.global __pushmem1
.global __pushmem2
.global __pushmem_xy1
.global __pushmem_xy2
.global __load_inline_mem_params

__pushmem_xy1:
  STX __mem_src
  STY __mem_src+1
  LDA #0
  JSR __load_inline_mem_params
  BRA pushmem1_start

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

__pushmem_xy2:
  STX __mem_src
  STY __mem_src+1
  LDA #4
  JSR __load_inline_mem_params
  BRA pushmem2_start

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

  STZ __t0
  STZ __t1
pml:
  LDA __t0
  CMP __mem_size
  BNE pml1
  LDA __t1
  CMP __mem_size+1
  BEQ end_pm
pml1:
  LDA (__mem_src)
  STA (__sp)

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
