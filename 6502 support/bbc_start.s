#include "vars.s"

.section ".text._start", "ax", @progbits

.global _start

// *RUN enters with JSR, so the MOS return address is on the 6502 stack.
// Page 1 is the hardware stack. The C stack is the software stack and
// stops at the mode 7 screen.
.set bbc_stack_top 0x7c00

_start:
  LDA #bbc_stack_top & 0xff
  STA __sp
  STA __fp
  LDA #bbc_stack_top >> 8
  STA __sp+1
  STA __fp+1
  TSX
  STX bbc_s

  LDA #%lo(__init_array_start)
  STA init_ptr
  LDA #%hi(__init_array_start)
  STA init_ptr+1
init_loop:
  LDA init_ptr
  CMP #%lo(__init_array_end)
  BNE init_call
  LDA init_ptr+1
  CMP #%hi(__init_array_end)
  BEQ init_done
init_call:
  LDA init_ptr
  STA __t0
  LDA init_ptr+1
  STA __t0+1
  LDY #0
  LDA (__t0), Y
  STA init_jsr+1
  INY
  LDA (__t0), Y
  STA init_jsr+2
  JSR init_jsr
  CLC
  LDA init_ptr
  ADC #2
  STA init_ptr
  BCC init_loop
  INC init_ptr+1
  JMP init_loop
init_done:
  STZ __i0
  STZ __i0+1
  LDX #0
  LDY #0
  JSR __pushxy
  LDX #__i0
  JSR __pushreg2
  LDX #__i0
  LDY #0
  JSR main
  LDX #__i0
  JSR __pushreg2
  LDX #__i0
  LDY #0
  JSR exit
  LDX bbc_s
  TXS
  RTS

init_jsr:
  .byte 0x20, 0x00, 0x00
  RTS

.comm init_ptr, 2
