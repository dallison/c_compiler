#include "vars.s"
#ifdef DAVECC_PAGED_LIBC
#include "layout.h"
#endif

// DaveCC syscall numbers. On 6502, exit and abort use the 6502-specific
// values. Arguments are 2-byte ints and pointers; a long is 4 bytes.
// X,Y on entry is the address of the 4-byte result.
.set SYS_EXIT_6502 1
.set SYS_OPEN 2
.set SYS_CLOSE 3
.set SYS_WRITE 4
.set SYS_READ 5
.set SYS_LSEEK 7
.set SYS_ABORT_6502 8
.set SYS_TIME 13
.set SYS_CLOCK 14
.set SYS_EXIT_CLEAN 22

.set OSFIND 0xffce
.set OSBGET 0xffd7
.set OSBPUT 0xffd4
.set OSARGS 0xffda
.set OSWORD 0xfff1
.set OSWRCH 0xffee
.set OSASCI 0xffe3
.set OSRDCH 0xffe0

.comm res_ptr, 2
.comm sysno, 1
.comm fd, 2
.comm ptr, 2
.comm len, 2
.comm count, 2
.comm byte, 1
.comm handle, 1
.comm slot, 1
.comm os_a, 1
.comm os_x, 1
.comm os_y, 1
.comm os_p, 1
.comm os_target, 2
.comm result0, 1
.comm result1, 1
.comm result2, 1
.comm result3, 1
.comm argblk, 4
.comm handles, 8
.comm bbc_name, 16
.comm clk, 5

.section ".text.syscall", "ax", @progbits

.global syscall

syscall:
  STX res_ptr
  STY res_ptr+1
  LDY #1
  LDA (__sp), Y
  BNE enosys
  LDY #0
  LDA (__sp), Y
  STA sysno
  CMP #SYS_EXIT_6502
  BEQ syscall_exit
  CMP #SYS_EXIT_CLEAN
  BNE syscall_not_exit
syscall_exit:
  JMP do_exit
syscall_not_exit:
  CMP #SYS_ABORT_6502
  BNE dispatch_write
  JMP do_abort
dispatch_write:
  CMP #SYS_WRITE
  BNE dispatch_read
  JMP do_write
dispatch_read:
  CMP #SYS_READ
  BNE dispatch_open
  JMP do_read
dispatch_open:
  CMP #SYS_OPEN
  BNE dispatch_close
  JMP do_open
dispatch_close:
  CMP #SYS_CLOSE
  BNE dispatch_lseek
  JMP do_close
dispatch_lseek:
  CMP #SYS_LSEEK
  BNE dispatch_time
  JMP do_lseek
dispatch_time:
  CMP #SYS_TIME
  BNE dispatch_clock
  JMP do_time
dispatch_clock:
  CMP #SYS_CLOCK
  BNE enosys
  JMP do_clock
enosys:
  JMP set_minus_one

.section ".text.do_exit", "ax", @progbits
.global do_exit
do_exit:
  JMP bbc_return

.section ".text.do_abort", "ax", @progbits
.global do_abort
do_abort:
  BRK
  .byte 0
  .asciz "abort"

.section ".text.do_write", "ax", @progbits
.global do_write
do_write:
  JSR load_write_args
  LDA #0
  STA count
  STA count+1
  LDA fd+1
  BNE write_file
  LDA fd
  CMP #1
  BEQ write_loop
  CMP #2
  BEQ write_loop
  CMP #0
  BNE write_file
  JMP write_bad
write_file:
  JSR lookup_handle
  BCC write_loop
  JMP write_bad
write_loop:
  LDA len
  ORA len+1
  BEQ write_done
  JSR load_byte
  LDA fd+1
  BNE write_put
  LDA fd
  CMP #3
  BCC write_console
write_put:
  LDA byte
  STA os_a
  LDA #0
  STA os_x
  LDA handle
  STA os_y
  LDA #%lo(OSBPUT)
  STA os_target
  LDA #%hi(OSBPUT)
  STA os_target+1
  JSR bbc_os_call
  JMP write_advance
write_console:
  LDA byte
  CMP #0x0d
  BEQ write_cr
  STA os_a
  LDA #%lo(OSWRCH)
  STA os_target
  LDA #%hi(OSWRCH)
  STA os_target+1
  JMP write_emit
write_cr:
  LDA #0x0d
  STA os_a
  LDA #%lo(OSASCI)
  STA os_target
  LDA #%hi(OSASCI)
  STA os_target+1
write_emit:
  LDA #0
  STA os_x
  STA os_y
  JSR bbc_os_call
write_advance:
  INC ptr
  BNE write_ptr_ok
  INC ptr+1
write_ptr_ok:
  INC count
  BNE write_count_ok
  INC count+1
write_count_ok:
  LDA len
  BNE write_len_lo
  DEC len+1
write_len_lo:
  DEC len
  JMP write_loop
write_done:
  JMP set_from_count
write_bad:
  JMP set_minus_one

.section ".text.do_read", "ax", @progbits
.global do_read
do_read:
  JSR load_write_args
  LDA #0
  STA count
  STA count+1
  LDA fd+1
  BNE read_file
  LDA fd
  BNE read_not_stdin
  JMP read_loop
read_not_stdin:
  CMP #3
  BCS read_file
  JMP read_bad
read_file:
  JSR lookup_handle
  BCC read_loop
  JMP read_bad
read_loop:
  LDA len
  ORA len+1
  BEQ read_done
  LDA fd
  BNE read_bget
  LDA #0
  STA os_x
  STA os_y
  LDA #%lo(OSRDCH)
  STA os_target
  LDA #%hi(OSRDCH)
  STA os_target+1
  JSR bbc_os_call
  JMP read_got
read_bget:
  LDA #0
  STA os_a
  STA os_x
  LDA handle
  STA os_y
  LDA #%lo(OSBGET)
  STA os_target
  LDA #%hi(OSBGET)
  STA os_target+1
  JSR bbc_os_call
read_got:
  LDA os_p
  AND #1
  BNE read_done
  LDA os_a
  CMP #0x0d
  BNE read_store
  LDA #0x0a
read_store:
  STA byte
  LDA ptr
  STA __t2
  LDA ptr+1
  STA __t2+1
  LDY #0
  LDA byte
  STA (__t2), Y
  INC ptr
  BNE read_ptr_ok
  INC ptr+1
read_ptr_ok:
  INC count
  BNE read_count_ok
  INC count+1
read_count_ok:
  LDA len
  BNE read_len_lo
  DEC len+1
read_len_lo:
  DEC len
  JMP read_loop
read_done:
  JMP set_from_count
read_bad:
  JMP set_minus_one

.section ".text.do_open", "ax", @progbits
.global do_open
do_open:
  LDY #2
  LDA (__sp), Y
  STA __t2
  INY
  LDA (__sp), Y
  STA __t2+1
  LDY #4
  LDA (__sp), Y
  STA fd
  LDY #0
open_copy:
  LDA (__t2), Y
  BEQ open_copied
  CPY #14
  BEQ open_copied
  STA bbc_name, Y
  INY
  BNE open_copy
open_copied:
  LDA #0x0d
  STA bbc_name, Y
  LDX #0
open_slot:
  LDA handles, X
  BEQ open_have_slot
  INX
  CPX #8
  BNE open_slot
  JMP set_minus_one
open_have_slot:
  STX slot
  LDA fd
  AND #3
  BEQ open_read
  CMP #1
  BEQ open_write
  LDA #0xc0
  JMP open_find
open_read:
  LDA #0x40
  JMP open_find
open_write:
  LDA #0x80
open_find:
  STA os_a
  LDA #%lo(bbc_name)
  STA os_x
  LDA #%hi(bbc_name)
  STA os_y
  LDA #%lo(OSFIND)
  STA os_target
  LDA #%hi(OSFIND)
  STA os_target+1
  JSR bbc_os_call
  LDA os_a
  BEQ open_fail
  LDX slot
  STA handles, X
  CLC
  TXA
  ADC #3
  STA result0
  LDA #0
  STA result1
  STA result2
  STA result3
  JMP store_result
open_fail:
  JMP set_minus_one

.section ".text.do_close", "ax", @progbits
.global do_close
do_close:
  LDY #2
  LDA (__sp), Y
  STA fd
  INY
  LDA (__sp), Y
  STA fd+1
  LDA fd+1
  BNE close_file
  LDA fd
  CMP #3
  BCC close_ok
close_file:
  JSR lookup_handle
  BCC close_go
  JMP close_bad
close_go:
  LDA #0
  STA os_a
  STA os_x
  LDA handle
  STA os_y
  LDA #%lo(OSFIND)
  STA os_target
  LDA #%hi(OSFIND)
  STA os_target+1
  JSR bbc_os_call
  LDX slot
  LDA #0
  STA handles, X
close_ok:
  JMP set_zero
close_bad:
  JMP set_minus_one

.section ".text.do_lseek", "ax", @progbits
.global do_lseek
do_lseek:
  LDY #2
  LDA (__sp), Y
  STA fd
  INY
  LDA (__sp), Y
  STA fd+1
  LDY #4
  LDX #0
lseek_off:
  LDA (__sp), Y
  STA argblk, X
  INY
  INX
  CPX #4
  BNE lseek_off
  LDA (__sp), Y
  STA byte
  LDA fd+1
  BNE lseek_file
  LDA fd
  CMP #3
  BCC lseek_bad
lseek_file:
  JSR lookup_handle
  BCC lseek_go
  JMP lseek_bad
lseek_go:
  LDA byte
  BNE lseek_not_set
  LDA #1
  JSR os_args
  JMP lseek_result
lseek_not_set:
  CMP #1
  BEQ lseek_cur
  CMP #2
  BNE lseek_bad
  LDA #2
  STA slot
  JMP lseek_save
lseek_cur:
  LDA #0
  STA slot
lseek_save:
  LDX #0
lseek_save_off:
  LDA argblk, X
  STA clk, X
  INX
  CPX #4
  BNE lseek_save_off
  LDA slot
  JSR os_args
  LDX #0
  CLC
lseek_add_cur:
  LDA argblk, X
  ADC clk, X
  STA argblk, X
  INX
  CPX #4
  BNE lseek_add_cur
lseek_add:
  LDA #1
  JSR os_args
lseek_result:
  LDX #0
lseek_copy:
  LDA argblk, X
  STA result0, X
  INX
  CPX #4
  BNE lseek_copy
  JMP store_result
lseek_bad:
  JMP set_minus_one

.section ".text.do_time", "ax", @progbits
.global do_time
do_time:
  JSR read_clock
  JSR div100
  JMP store_result

.section ".text.do_clock", "ax", @progbits
.global do_clock
do_clock:
  JSR read_clock
  LDX #0
clock_copy:
  LDA clk, X
  STA result0, X
  INX
  CPX #4
  BNE clock_copy
  JMP store_result

.section ".text.load_write_args", "ax", @progbits
.global load_write_args
load_write_args:
  LDY #2
  LDA (__sp), Y
  STA fd
  INY
  LDA (__sp), Y
  STA fd+1
  INY
  LDA (__sp), Y
  STA ptr
  INY
  LDA (__sp), Y
  STA ptr+1
  INY
  LDA (__sp), Y
  STA len
  INY
  LDA (__sp), Y
  STA len+1
  RTS

.section ".text.load_byte", "ax", @progbits
.global load_byte
load_byte:
  LDA ptr
  STA __t2
  LDA ptr+1
  STA __t2+1
  LDY #0
  LDA (__t2), Y
  CMP #0x0a
  BNE load_byte_keep
  LDA #0x0d
load_byte_keep:
  STA byte
  RTS

// fd is an OS handle slot. Carry set if it is not open.
.section ".text.lookup_handle", "ax", @progbits
.global lookup_handle
lookup_handle:
  SEC
  LDA fd
  SBC #3
  CMP #8
  BCS lookup_bad
  TAX
  STX slot
  LDA handles, X
  BEQ lookup_bad
  STA handle
  CLC
  RTS
lookup_bad:
  SEC
  RTS

.section ".text.set_from_count", "ax", @progbits
.global set_from_count
set_from_count:
  LDA count
  STA result0
  LDA count+1
  STA result1
  LDA #0
  STA result2
  STA result3
  JMP store_result

.section ".text.set_zero", "ax", @progbits
.global set_zero
set_zero:
  LDA #0
  STA result0
  STA result1
  STA result2
  STA result3
  JMP store_result

.section ".text.set_minus_one", "ax", @progbits
.global set_minus_one
set_minus_one:
  LDA #0xff
  STA result0
  STA result1
  STA result2
  STA result3
  JMP store_result

.section ".text.store_result", "ax", @progbits
.global store_result
store_result:
  LDA res_ptr
  STA __t2
  LDA res_ptr+1
  STA __t2+1
  LDY #0
  LDA result0
  STA (__t2), Y
  INY
  LDA result1
  STA (__t2), Y
  INY
  LDA result2
  STA (__t2), Y
  INY
  LDA result3
  STA (__t2), Y
  RTS

.section ".text.read_clock", "ax", @progbits
.global read_clock
read_clock:
  LDX #0
  LDA #0
clock_clear:
  STA clk, X
  INX
  CPX #5
  BNE clock_clear
  LDA #1
  STA os_a
  LDA #%lo(clk)
  STA os_x
  LDA #%hi(clk)
  STA os_y
  LDA #%lo(OSWORD)
  STA os_target
  LDA #%hi(OSWORD)
  STA os_target+1
  JMP bbc_os_call

// Divide the 4-byte clock by 100 into result0.
.section ".text.div100", "ax", @progbits
.global div100
div100:
  LDA #0
  STA result0
  STA result1
  STA result2
  STA result3
  STA count
  STA count+1
  LDY #32
div_loop:
  ASL clk
  ROL clk+1
  ROL clk+2
  ROL clk+3
  ROL count
  ROL count+1
  ASL result0
  ROL result1
  ROL result2
  ROL result3
  LDA count
  SEC
  SBC #0x64
  TAX
  LDA count+1
  SBC #0
  BCC div_next
  STA count+1
  STX count
  INC result0
div_next:
  DEY
  BNE div_loop
  RTS

// A is the OSARGS reason. Y is the handle. argblk is the 4-byte block,
// which MOS requires in zero page. &70 is math scratch.
.section ".text.os_args", "ax", @progbits
.global os_args
os_args:
  STA byte
  LDX #0
os_args_put:
  LDA argblk, X
  STA 0x70, X
  INX
  CPX #4
  BNE os_args_put
  LDA byte
  LDX #0x70
  LDY handle
#ifdef DAVECC_PAGED_LIBC
  STA os_a
  STX os_x
  STY os_y
  LDA #%lo(OSARGS)
  STA os_target
  LDA #%hi(OSARGS)
  STA os_target+1
  JSR bbc_os_call
#else
  JSR OSARGS
#endif
  LDX #0
os_args_get:
  LDA 0x70, X
  STA argblk, X
  INX
  CPX #4
  BNE os_args_get
  RTS

// Mailbox is os_a, os_x, os_y, os_target. bbc_mos.s calls this too.
// MOS leaves the language zero page alone, so this does not copy it.
.section ".text.bbc_os_call", "ax", @progbits
.global bbc_os_call
.global bbc_os_invoke

bbc_os_call:
bbc_os_invoke:
  LDA os_target
#ifdef DAVECC_PAGED_LIBC
  // The JSR returns into main RAM. MOS has by then selected image 0,
  // and the trampoline writes the virtual image back before returning.
  STA PAGED_MOS_JSR+1
  LDA os_target+1
  STA PAGED_MOS_JSR+2
  LDA os_a
  LDX os_x
  LDY os_y
  JSR PAGED_MOS_CALL
#else
  STA bbc_os_jsr+1
  LDA os_target+1
  STA bbc_os_jsr+2
  LDA os_a
  LDX os_x
  LDY os_y
bbc_os_jsr:
  .byte 0x20, 0x00, 0x00
#endif
  STA os_a
  STX os_x
  STY os_y
  PHP
  PLA
  STA os_p
  RTS
