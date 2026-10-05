#include "../vars.s"
#include "layout.h"

// Main-RAM side of a call into the sideways libc.
//
// All seven images share one physical socket. Bits 0-3 of the ROM select
// latch are that socket. Bits 4-6 are the virtual image (0 = the language
// ROM MOS sees, 1 = libm, 2 = math_a, 3 = math_b, 4 = C++, 5 = printf
// and scanf, 6 = float). Bit 7 is the Master's ANDY overlay and is left
// alone. The socket is found by scanning for the image 0 title; it is
// not assembled in.
//
// Each exported function is a stub. A is the virtual image, not a C
// argument:
//
//   LDA #<image>
//   JSR __libc_paged_enter
//   JSR <fixed vector>
//   JMP __libc_paged_leave
//
// MOS keeps only the socket in &F4 and writes that back to &FE30 on an
// interrupt or OS call, which selects image 0. A non-zero image therefore
// runs with interrupts masked, and every MOS call from the ROMs goes
// through __libc_mos_call, which writes the virtual image back afterwards.
// X and Y are the C result pointer and are preserved.

.section ".paged_mos", "ax", @progbits

.global __libc_mos_call

// A, X, and Y are the MOS arguments. The caller filled the absolute
// address at PAGED_MOS_JSR. Returns with A, X, Y, and the flags from MOS.
// A non-zero virtual image is selected again and interrupts stay masked.
__libc_mos_call:
  CLI
__libc_mos_jsr:
  JSR 0x0000
  PHP
  PHA
  TXA
  PHA
  TYA
  PHA
  LDA __libc_virt_id
  BEQ mos_pop
  LDA __libc_virt_select
  STA PAGED_ROMSEL
  TSX
  LDA 0x104, X
  ORA #0x04
  STA 0x104, X
mos_pop:
  PLA
  TAY
  PLA
  TAX
  PLA
  PLP
  RTS
mos_end:
  .space 0x30 - (mos_end - __libc_mos_call)

.section ".paged_gate", "ax", @progbits

.global __libc_paged_gate

// Called from inside a paged image. The three bytes after the JSR are
// the virtual image and the vector address in that image.
__libc_paged_gate:
  LDA __libc_gate_index
  PHA
  LDA __libc_gate_vec
  PHA
  LDA __libc_gate_vec+1
  PHA
  TXA
  PHA
  TYA
  PHA
  TSX
  ; 0x101,X is Y. Below X, vec+1, vec, and the saved index is the JSR
  ; return address (low, then high). It points at the last byte of the JSR.
  LDA 0x107, X
  STA __t0+1
  LDA 0x106, X
  STA __t0
  INC __t0
  BNE gate_ptr_ok
  INC __t0+1
gate_ptr_ok:
  LDY #0
  LDA (__t0), Y
  STA __libc_gate_index
  INY
  LDA (__t0), Y
  STA __libc_gate_vec
  INY
  LDA (__t0), Y
  STA __libc_gate_vec+1
  CLC
  LDA __t0
  ADC #2
  STA 0x106, X
  LDA __t0+1
  ADC #0
  STA 0x107, X
  PLA
  TAY
  PLA
  TAX
  LDA __libc_gate_index
  JSR __libc_paged_enter
  LDA __libc_gate_vec
  STA gate_call+1
  LDA __libc_gate_vec+1
  STA gate_call+2
  JSR gate_call
  JSR __libc_paged_leave
  PLA
  STA __libc_gate_vec+1
  PLA
  STA __libc_gate_vec
  PLA
  STA __libc_gate_index
  RTS

gate_call:
  JSR 0x0000
  RTS

.section ".text.__libc_paged_ensure_init", "ax", @progbits

.global __libc_paged_ensure_init

__libc_paged_ensure_init:
  LDA __libc_paged_ready
  BNE ensure_done
  TXA
  PHA
  TYA
  PHA
  JSR __libc_paged_find_socket
  LDA #1
  STA __libc_paged_ready
  LDX #0
ensure_loop:
  TXA
  PHA
  JSR __libc_paged_init_one
  PLA
  TAX
  INX
  CPX #PAGED_ROM_COUNT
  BNE ensure_loop
  PLA
  TAY
  PLA
  TAX
ensure_done:
  RTS

// A is the virtual image. Page it in, call vector 0, page the previous
// image back. Does not go through enter: enter calls ensure_init.
__libc_paged_init_one:
  STA __libc_virt_id
  JSR __libc_paged_load_select
  STA __libc_virt_select
  LDA PAGED_ROMSEL
  PHA
  LDA PAGED_ROM_ID
  PHA
  SEI
  LDA __libc_virt_select
  STA PAGED_ROMSEL
  LDA __libc_socket
  STA PAGED_ROM_ID
  LDA __libc_virt_id
  BNE init_call
  CLI
init_call:
  LDA #%lo(__libc_paged_mbox)
  STA __t0
  LDA #%hi(__libc_paged_mbox)
  STA __t0+1
  JSR PAGED_VECTOR_BASE
  PLA
  SEI
  STA PAGED_ROM_ID
  PLA
  STA PAGED_ROMSEL
  LDA #0
  STA __libc_virt_id
  CLI
  RTS

.section ".text.__libc_paged_find_socket", "ax", @progbits

.global __libc_paged_find_socket

// Image 0 of the libc socket has the language title at PAGED_TITLE.
// Empty sockets read as $00 or $FF and fail the compare.
__libc_paged_find_socket:
  LDA __libc_select_known
  BNE find_done
  SEI
  LDA PAGED_ROMSEL
  PHA
  LDA PAGED_ROM_ID
  PHA
  LDX #0
find_loop:
  TXA
  STA PAGED_ROMSEL
  LDA PAGED_ROM_BASE
  CMP #$4c
  BNE find_next
  LDA PAGED_ROM_BASE+6
  CMP #$c0
  BNE find_next
  LDY #0
find_title:
  LDA find_name, Y
  BEQ find_hit
  CMP PAGED_TITLE, Y
  BNE find_next
  INY
  BNE find_title
find_hit:
  STX __libc_socket
  JMP find_restore
find_next:
  INX
  CPX #16
  BNE find_loop
  LDA #0
  STA __libc_socket
find_restore:
  PLA
  STA PAGED_ROM_ID
  PLA
  STA PAGED_ROMSEL
  CLI
  LDA #1
  STA __libc_select_known
find_done:
  RTS

find_name:
  .asciz "DaveCC libc"

// A is the virtual image on entry and the select byte on return.
// X and Y are preserved.
__libc_paged_load_select:
  AND #7
  ASL
  ASL
  ASL
  ASL
  ORA __libc_socket
  RTS

.section ".text.__libc_paged_enter", "ax", @progbits

.global __libc_paged_enter

// A is the virtual image. X and Y pass through to the callee.
// At the TSX below, 0x101,X is Y, then X, the previous &F4, the previous
// &FE30, and the target select. Those last three stay for leave. Leave
// pushes X and Y again before its own TSX, so it reads &F4 at 0x103,X,
// the previous latch at 0x104,X, and the target at 0x105,X.
__libc_paged_enter:
  PHA
  TXA
  PHA
  TYA
  PHA
  JSR __libc_paged_ensure_init
  PLA
  TAY
  PLA
  TAX
  PLA
  STA __libc_virt_id
  JSR __libc_paged_load_select
  STA __libc_virt_select
  PHA
  LDA PAGED_ROMSEL
  PHA
  LDA PAGED_ROM_ID
  PHA
  TXA
  PHA
  TYA
  PHA
  TSX
  LDA 0x104, X
  CMP 0x105, X
  BEQ enter_irq
  SEI
  LDA 0x105, X
  STA PAGED_ROMSEL
  LDA __libc_socket
  STA PAGED_ROM_ID
enter_irq:
  LDA __libc_virt_id
  BNE enter_sei
  CLI
  JMP enter_restore
enter_sei:
  SEI
enter_restore:
  PLA
  TAY
  PLA
  TAX
  RTS

.section ".text.__libc_paged_leave", "ax", @progbits

.global __libc_paged_leave

__libc_paged_leave:
  TXA
  PHA
  TYA
  PHA
  TSX
  LDA 0x104, X
  CMP 0x105, X
  BEQ leave_which
  SEI
  LDA 0x104, X
  STA PAGED_ROMSEL
  LDA 0x103, X
  STA PAGED_ROM_ID
leave_which:
  LDA PAGED_ROMSEL
  AND #$0f
  CMP __libc_socket
  BNE leave_outside
  LDA PAGED_ROMSEL
  LSR
  LSR
  LSR
  LSR
  AND #7
  STA __libc_virt_id
  LDA PAGED_ROMSEL
  STA __libc_virt_select
  JMP leave_irq
leave_outside:
  LDA #0
  STA __libc_virt_id
leave_irq:
  LDA __libc_virt_id
  BNE leave_sei
  CLI
  JMP leave_pop
leave_sei:
  SEI
leave_pop:
  PLA
  TAY
  PLA
  TAX
  PLA
  PLA
  PLA
  RTS

// Heap bounds are filled in by the user-program link (_end and
// __bbc_stack_cap). The three stream slots are unused: stdin, stdout,
// and stderr are the words at &09E0 in the cassette buffer, which the
// primary ROM fills. The trailing words keep the gate and the MOS
// trampoline live under --gc-sections.
.section ".data.__libc_paged_mbox", "aw", @progbits

.global __libc_paged_mbox
.global __libc_paged_ready
.global __libc_select_known
.global __libc_socket
.global __libc_virt_id
.global __libc_virt_select
.global __libc_paged_gate
.global __libc_mos_call

__libc_paged_mbox:
  .hword _end
  .hword __bbc_stack_cap
  .hword 0
  .hword 0
  .hword 0
__libc_paged_ready:
  .byte 0
__libc_select_known:
  .byte 0
__libc_socket:
  .byte 0
__libc_virt_id:
  .byte 0
__libc_virt_select:
  .byte 0
__libc_gate_index:
  .byte 0
__libc_gate_vec:
  .hword 0
  .hword __libc_paged_ensure_init
  .hword __libc_paged_gate
  .hword __libc_mos_call
