#include "vars.s"

.section ".text.__load_inline_mem_params", "ax", @progbits

.global __load_inline_mem_params

// Load memory helper arguments embedded immediately after the caller's JSR.
// A is a bitmask: bit 0 = destination pointer, bit 1 = source pointer,
// bit 2 = two-byte size.  Pointer arguments are zero-page register offsets.
__load_inline_mem_params:
  STA __t0
  TAX
  LDA inline_mem_param_sizes,X
  STA __t1

  // This helper's JSR is above the caller's JSR on the hardware stack.
  // Build a pointer to the first inline byte (caller's return address + 1).
  TSX
  LDA 0x103,X
  STA __t2
  LDA 0x104,X
  STA __t3
  INC __t2
  BNE load_inline_mem_no_carry
  INC __t3
load_inline_mem_no_carry:

  // Make the outer RTS skip the inline descriptor.
  CLC
  LDA 0x103,X
  ADC __t1
  STA 0x103,X
  LDA 0x104,X
  ADC #0
  STA 0x104,X

  LDY #0
  LDA __t0
  AND #1
  BEQ load_inline_mem_no_dest
  LDA (__t2),Y
  TAX
  LDA 0,X
  STA __mem_dest
  LDA 1,X
  STA __mem_dest+1
  INY
load_inline_mem_no_dest:
  LDA __t0
  AND #2
  BEQ load_inline_mem_no_src
  LDA (__t2),Y
  TAX
  LDA 0,X
  STA __mem_src
  LDA 1,X
  STA __mem_src+1
  INY
load_inline_mem_no_src:
  LDA (__t2),Y
  STA __mem_size
  LDA __t0
  AND #4
  BEQ load_inline_mem_done
  INY
  LDA (__t2),Y
  STA __mem_size+1
load_inline_mem_done:
  RTS

inline_mem_param_sizes:
  .byte 1, 2, 2, 3, 2, 3, 3, 4
