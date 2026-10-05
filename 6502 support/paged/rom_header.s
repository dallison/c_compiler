#include "layout.h"

// Language ROM header. MOS validates a sideways ROM by reading this
// image at &8000: two JMP entry points, a type byte with the language
// and service bits set, and a copyright string containing "(C)".
//
// The header section is padded to PAGED_HEADER_SIZE bytes so the vector
// table that follows it is linked at PAGED_VECTOR_BASE.

.section ".rom_header", "ax", @progbits

.global paged_rom_entry

paged_rom_entry:
  JMP language_entry
  JMP service_entry
#ifdef PAGED_EXTRA
  .byte 0x80
#else
  .byte 0xc0
#endif
  .byte copyright - paged_rom_entry
  .byte 1
#ifdef PAGED_EXTRA
#include "image_title.h"
  .asciz PAGED_NAME
#else
  .asciz "DaveCC libc"
#endif
  .asciz "1.00"
copyright:
  .asciz "(C)2026 DaveCC"
  .hword __paged_vectors
header_end:
  .space PAGED_HEADER_SIZE - (header_end - paged_rom_entry)

.section ".text.language_entry", "ax", @progbits

.global language_entry

// Entered with JMP when image 0 is selected as the current language.
// The other images are not visible to MOS. The primary image offers a
// *command line. The title at offset 9 is what the socket scan matches.
language_entry:
#ifdef PAGED_EXTRA
  JMP bbc_return
#else
  LDX #0xff
  TXS
  JSR print_banner
language_loop:
  LDA #0x3e
  JSR PAGED_OSWRCH
  LDA #0x20
  JSR PAGED_OSWRCH
  LDX #%lo(line_block)
  LDY #%hi(line_block)
  LDA #0
  JSR PAGED_OSWORD
  BCS language_loop
  LDX #%lo(line_buf)
  LDY #%hi(line_buf)
  JSR PAGED_OSCLI
  JMP language_loop
#endif

.section ".text.service_entry", "ax", @progbits

.global service_entry

// A = service call, X = our ROM number, Y = call parameter.
// *HELP (call 9, Y=0) prints the title. Every other call is ignored.
// X, Y, and A are preserved so MOS keeps walking the other ROMs.
service_entry:
  CMP #9
  BNE service_done
  PHA
  TXA
  PHA
  TYA
  PHA
  CPY #0
  BNE service_restore
  JSR print_banner
service_restore:
  PLA
  TAY
  PLA
  TAX
  PLA
service_done:
  RTS

.section ".text.print_banner", "ax", @progbits

print_banner:
  LDX #0
banner_loop:
  LDA banner, X
  BEQ banner_cr
  JSR PAGED_OSWRCH
  INX
  BNE banner_loop
banner_cr:
  LDA #0x0d
  JSR PAGED_OSWRCH
  RTS

banner:
#ifdef PAGED_EXTRA
  .asciz PAGED_BANNER
#else
  .asciz "DaveCC libc 1.00"
#endif

#ifndef PAGED_EXTRA
.section ".data.line_block", "aw", @progbits

line_block:
  .hword line_buf
  .byte 80
  .byte 0x20
  .byte 0xff

.comm line_buf, 80
#endif

// exit() in this image is the BBC path: bbc_syscall jumps here instead of
// returning to a *RUN caller. Same sequence as bbc_start.s.
.section ".text.bbc_return", "ax", @progbits
.global bbc_return
bbc_return:
  LDA #0x84
  LDX #0
  LDY #0
  JSR PAGED_OSBYTE
  STX 0x06
  STY 0x07
  LDA #0xfc
  LDX #0
  LDY #0xff
  JSR PAGED_OSBYTE
  LDA #0x8e
  JSR PAGED_OSBYTE
  BRK
