#include "vars.s"

// *RUN enters with JSR. The C ABI uses the language zero page (&00..&8F);
// MOS entry points leave that page alone, but compiled code clobbers it during
// the run. On exit we re-enter the current language ROM (OSBYTE &8E) instead
// of restoring saved zero page or RTS-ing to the *RUN caller.
//
// The linker script defines the absolute symbol __bbc_stack_cap as the initial
// software stack top (zero means use BASIC HIMEM from &06/&07 as-is).

.section ".text._start", "ax", @progbits

.global _start

_start:
  ; Software stack grows down from BASIC HIMEM (&06/&07). Read before the C
  ; ABI clobbers zero page. mode() will refresh __sp after a mode change.
  LDA 0x06
  STA __sp
  LDA 0x07
  STA __sp+1
  LDA #%hi(__bbc_stack_cap)
  BEQ stack_cap_done
  STA __sp+1
  LDA #%lo(__bbc_stack_cap)
  STA __sp
stack_cap_done:
  LDA __sp
  STA __fp
  LDA __sp+1
  STA __fp+1
  ; Soft-float and MOS nest deeply on the 6502 hardware stack.
  LDX #0xff
  TXS

  ; Paged libc replaces this with a real init. The weak stub below is an
  ; RTS, so a program that does not link the shim pays one call and returns.
  JSR __libc_paged_ensure_init

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
  LDA #0
  STA __i0
  STA __i0+1
  LDX #0
  LDY #0
  JSR __pushxy
  LDX #__i0
  JSR __pushreg2
  LDX #__i0
  LDY #0
  JSR main
  JMP bbc_return

init_jsr:
  .byte 0x20, 0x00, 0x00
  RTS

// Re-enter the current language (BASIC, etc.). OSBYTE &8E does not return.
// The C ABI uses __i0 at &06/&07, the same bytes as BASIC HIMEM, so refresh
// HIMEM from MOS before handing control back.
.section ".text.bbc_return", "ax", @progbits
.global bbc_return
bbc_return:
  LDA #0x84
  LDX #0
  LDY #0
  JSR 0xfff4
  STX 0x06
  STY 0x07
  ; Read the current language ROM number into X (Y=&FF leaves it unchanged).
  LDA #0xfc
  LDX #0
  LDY #0xff
  JSR 0xfff4
  LDA #0x8e
  JSR 0xfff4
  BRK

.comm init_ptr, 2

.section ".text.__libc_paged_ensure_init", "ax", @progbits
.weak __libc_paged_ensure_init
__libc_paged_ensure_init:
  RTS
