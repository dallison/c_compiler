#include "../vars.s"
#include "layout.h"

// One-time setup for the sideways image. The shim calls vector 0 with
// __t0 pointing at the main-RAM mailbox. .bss (including this file's
// commons) is zeroed, the user program's heap bounds are copied in, and
// The primary image builds the standard streams in the cassette buffer.
//
// Nested entry is a no-op once __paged_ready is set, so a constructor
// that calls back through the shim does not zero the image again.

.section ".text.__paged_rom_init", "ax", @progbits

.global __paged_rom_init

__paged_rom_init:
  LDA __paged_ready
  BNE init_done
  LDA __t0
  PHA
  LDA __t0+1
  PHA
  JSR zero_bss
  PLA
  STA __t0+1
  PLA
  STA __t0
  JSR copy_mailbox
  LDA #1
  STA __paged_ready
  JSR run_init_array
init_done:
  RTS

zero_bss:
  LDA #%lo(__bss_start)
  STA __t0
  LDA #%hi(__bss_start)
  STA __t0+1
zero_loop:
  LDA __t0
  CMP #%lo(_end)
  BNE zero_byte
  LDA __t0+1
  CMP #%hi(_end)
  BEQ zero_done
zero_byte:
  LDA #0
  LDY #0
  STA (__t0), Y
  INC __t0
  BNE zero_loop
  INC __t0+1
  JMP zero_loop
zero_done:
  RTS

copy_mailbox:
  LDY #PAGED_MBOX_HEAP
  LDA (__t0), Y
  STA __paged_heap_start
  INY
  LDA (__t0), Y
  STA __paged_heap_start+1
  LDY #PAGED_MBOX_LIMIT
  LDA (__t0), Y
  STA __paged_heap_limit
  INY
  LDA (__t0), Y
  STA __paged_heap_limit+1
  LDA __paged_heap_limit
  ORA __paged_heap_limit+1
  BNE limits_ready
  LDA #0x00
  STA __paged_heap_limit
  LDA #0x7c
  STA __paged_heap_limit+1
limits_ready:
#ifndef PAGED_SKIP_STREAMS
  JSR __paged_init_stdio
#endif
  RTS

// Same walk as bbc_start. Empty arrays have equal start and end (both 0
// when the image has no constructors).
run_init_array:
  LDA #%lo(__init_array_start)
  STA __t0
  LDA #%hi(__init_array_start)
  STA __t0+1
init_loop:
  LDA __t0
  CMP #%lo(__init_array_end)
  BNE init_call
  LDA __t0+1
  CMP #%hi(__init_array_end)
  BEQ init_arrays_done
init_call:
  LDY #0
  LDA (__t0), Y
  STA init_jsr+1
  INY
  LDA (__t0), Y
  STA init_jsr+2
  JSR init_jsr
  CLC
  LDA __t0
  ADC #2
  STA __t0
  BCC init_loop
  INC __t0+1
  JMP init_loop
init_arrays_done:
  RTS

init_jsr:
  .byte 0x20, 0x00, 0x00
  RTS

.comm __paged_heap_start, 2
.comm __paged_heap_limit, 2
.comm __paged_ready, 1

// fputc installs __davecc_stdio_fini here on the first buffered write.
.comm __davecc_stdio_fini_hook, 2

// exit walks these. An image that does not run constructors leaves them
// empty so the walk is a no-op.
.section ".data.paged_init_arrays", "aw", @progbits
.global __preinit_array_start
.global __preinit_array_end
.global __init_array_start
.global __init_array_end
.global __fini_array_start
.global __fini_array_end
__preinit_array_start:
__preinit_array_end:
__init_array_start:
__init_array_end:
__fini_array_start:
__fini_array_end:
  .byte 0
