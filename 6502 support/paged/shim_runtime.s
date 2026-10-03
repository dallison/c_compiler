#include "../vars.s"
#include "layout.h"

// Main-RAM side of a call into the sideways libc.
//
// Each exported function is a stub (see the generated shim). A is the
// image number (0 = libc, 1 = the extra ROM) and is not a C argument:
//
//   LDA #<image>
//   JSR __libc_paged_enter
//   JSR <fixed vector>
//   JMP __libc_paged_leave
//
// enter pushes the target select byte and the bank in &F4. It writes the
// ROM select latch only when the current bank is not that image. leave
// writes the saved bank back only when it differs from the target, so a
// call from one image into the other does not page the caller out.
//
// The select value is the socket number. On MOS 3.20 and later, bit 7 is
// set so Master sideways RAM is readable and writable for the whole call.
// X and Y are the C result pointer and are preserved.

.section ".text.__libc_paged_ensure_init", "ax", @progbits

.global __libc_paged_ensure_init

__libc_paged_ensure_init:
  LDA __libc_paged_ready
  BNE ensure_done
  TXA
  PHA
  TYA
  PHA
  JSR __libc_paged_compute_select
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
  LDA #1
  STA __libc_paged_ready
  PLA
  TAY
  PLA
  TAX
ensure_done:
  RTS

// A is the image number. Switch to it, call its vector 0, switch back.
__libc_paged_init_one:
  JSR __libc_paged_load_select
  STA __t2
  LDA PAGED_ROM_ID
  PHA
  SEI
  LDA __t2
  STA PAGED_ROMSEL
  STA PAGED_ROM_ID
  CLI
  LDA #%lo(__libc_paged_mbox)
  STA __t0
  LDA #%hi(__libc_paged_mbox)
  STA __t0+1
  JSR PAGED_VECTOR_BASE
  PLA
  SEI
  STA PAGED_ROMSEL
  STA PAGED_ROM_ID
  CLI
  RTS

.section ".text.__libc_paged_compute_select", "ax", @progbits

.global __libc_paged_compute_select

__libc_paged_compute_select:
  LDA __libc_select_known
  BNE select_done
  LDA #0
  LDX #1
  LDY #0
  JSR PAGED_OSBYTE
  TXA
  CMP #PAGED_MASTER_OS
  PHP
  LDX #0
select_loop:
  PLP
  PHP
  LDA __libc_rom_slot, X
  BCC select_store
  ORA #0x80
select_store:
  STA __libc_rom_select, X
  INX
  CPX #PAGED_ROM_COUNT
  BNE select_loop
  PLP
  LDA #1
  STA __libc_select_known
select_done:
  RTS

// A is the image number on entry and the select byte on return.
// X and Y are preserved; they are the C result pointer.
__libc_paged_load_select:
  STA __t2
  TXA
  PHA
  LDX __t2
  LDA __libc_rom_select, X
  STA __t2
  PLA
  TAX
  LDA __t2
  RTS

.section ".text.__libc_paged_enter", "ax", @progbits

.global __libc_paged_enter

// A is the image number. X and Y pass through to the callee.
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
  JSR __libc_paged_load_select
  PHA
  LDA PAGED_ROM_ID
  PHA
  TXA
  PHA
  TYA
  PHA
  TSX
  LDA 0x102, X
  CMP 0x103, X
  BEQ enter_restore
  LDA 0x103, X
  SEI
  STA PAGED_ROMSEL
  STA PAGED_ROM_ID
  CLI
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
  LDA 0x102, X
  CMP 0x103, X
  BEQ leave_restore
  LDA 0x102, X
  SEI
  STA PAGED_ROMSEL
  STA PAGED_ROM_ID
  CLI
leave_restore:
  PLA
  TAY
  PLA
  TAX
  PLA
  PLA
  RTS

// Called from inside a paged image. The three bytes after the JSR are
// the image number and the vector address in that image. X and Y are
// the caller's result pointer and are passed through.
.section ".paged_gate", "ax", @progbits

.global __libc_paged_gate

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
  ; Pushed after the JSR: index, vec, vec+1, X, Y. The return address
  ; is the next two bytes, and it points at the last byte of the JSR.
  LDA 0x106, X
  STA __t0+1
  LDA 0x105, X
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
  STA 0x105, X
  LDA __t0+1
  ADC #0
  STA 0x106, X
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

// Heap bounds are filled in by the user-program link (_end and
// __bbc_stack_cap). The three stream slots are unused: stdin, stdout,
// and stderr are the words at &09E0 in the cassette buffer, which the
// primary ROM fills. The trailing word keeps ensure_init live.
.section ".data.__libc_paged_mbox", "aw", @progbits

.global __libc_paged_mbox
.global __libc_paged_ready
.global __libc_select_known
.global __libc_rom_select
.global __libc_rom_slot
.global __libc_paged_gate

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
__libc_rom_slot:
  .byte LIBC_ROM_SLOT
  .byte LIBC_ROM2_SLOT
  .byte LIBC_ROM3_SLOT
  .byte LIBC_ROM4_SLOT
  .byte LIBC_ROM5_SLOT
  .byte LIBC_ROM6_SLOT
  .byte LIBC_ROM7_SLOT
__libc_rom_select:
  .space PAGED_ROM_COUNT
__libc_gate_index:
  .byte 0
__libc_gate_vec:
  .hword 0
  .hword __libc_paged_ensure_init
  .hword __libc_paged_gate
