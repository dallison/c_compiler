#include "../vars.s"
#include "layout.h"

// Main-RAM side of a call into the sideways libc.
//
// Each exported function is a nine-byte stub (see the generated shim):
//
//   JSR __libc_paged_enter
//   JSR <fixed vector>
//   JMP __libc_paged_leave
//
// enter saves the bank in &F4 on the hardware stack and, when that bank
// is not the libc bank, writes the ROM select latch. leave switches back
// only when the saved bank was not the libc bank, so a callback that
// re-enters libc (qsort, for example) does not page the image out from
// under the caller still running inside it.
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
  LDA PAGED_ROM_ID
  PHA
  SEI
  LDA __libc_rom_select
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
  LDA #1
  STA __libc_paged_ready
  PLA
  TAY
  PLA
  TAX
ensure_done:
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
  LDA #LIBC_ROM_SLOT
  BCC select_store
  ORA #0x80
select_store:
  STA __libc_rom_select
  LDA #1
  STA __libc_select_known
select_done:
  RTS

.section ".text.__libc_paged_enter", "ax", @progbits

.global __libc_paged_enter

__libc_paged_enter:
  JSR __libc_paged_ensure_init
  LDA PAGED_ROM_ID
  PHA
  CMP __libc_rom_select
  BEQ enter_done
  SEI
  LDA __libc_rom_select
  STA PAGED_ROMSEL
  STA PAGED_ROM_ID
  CLI
enter_done:
  RTS

.section ".text.__libc_paged_leave", "ax", @progbits

.global __libc_paged_leave

__libc_paged_leave:
  PLA
  CMP __libc_rom_select
  BEQ leave_done
  SEI
  STA PAGED_ROMSEL
  STA PAGED_ROM_ID
  CLI
leave_done:
  RTS

// Heap bounds are filled in by the user-program link (_end and
// __bbc_stack_cap). The stream pointers are written by ROM init.
// The trailing word keeps ensure_init live whenever this section is.
.section ".data.__libc_paged_mbox", "aw", @progbits

.global __libc_paged_mbox
.global stdin
.global stdout
.global stderr
.global __libc_paged_ready
.global __libc_select_known
.global __libc_rom_select

__libc_paged_mbox:
  .hword _end
  .hword __bbc_stack_cap
stdin:
  .hword 0
stdout:
  .hword 0
stderr:
  .hword 0
__libc_paged_ready:
  .byte 0
__libc_select_known:
  .byte 0
__libc_rom_select:
  .byte 0
  .byte 0
  .hword __libc_paged_ensure_init
