//
//  romsyscalls.s
//  c_compiler
//
//  Created by David Allison on 3/8/23.
//  Copyright © 2023 David Allison. All rights reserved.
//

#include "addresses.h"
#include "device.h"
#include "comms.h"

.text

// Zero page locations used by the compiler runtime system.
// Make sure this is kept in sync with the 6502 runtime system.
.set __sp 0x78
.set __fp 0x7a
.set __result 0x7c
.set __t0 0x7e
.set __t1 0x7f
.set __t2 0x80
.set __t3 0x81
.set __i0 0x8
.set __i1 0xa


// File descriptors are at 0x200 and each consists of:
// 0: device id (index into device table)
// 1: 2 bytes of data specific to the device type
.set fd_size 4      // So we can use ASL twice.
.set fd_device 0    // Device id (index into devices table).
.set fd_data 1      // Device-specific data.
.set fd_used 3      // non zero means entry is used

.set max_fds 10

.set fd_table 0x200

// Errno is just above the fd table.
.set errno fd_table + fd_size*max_fds

// A libc device.
.set libc_device_size 14
.set libc_device_open_func 0
.set libc_device_close_func 2
.set libc_device_read_func 4
.set libc_device_write_func 6
.set libc_device_ioctl_func 8
.set libc_device_poll_func 10
.set libc_device_lseek_func 12

.set stdin_libc_device_id 0
.set stdout_libc_device_id 1
.set file_libc_device_id 2

// Set errno
// Entry:
// A: errno
set_errno:
  STA errno
#ifdef __65c02__
  STZ errno+1
#else
  LDA #0
  STA errno+1
#endif
  RTS

  
// Setup the file descriptors in the fd_table
.global setup_fds
setup_fds:
  LDX #max_fds*fd_size
  LDA #0
setup_fds_loop:
  STA fd_table,X
  DEX
  BNE setup_fds_loop

  LDA #stdin_libc_device_id
  STA fd_table            // Stdin is device 0
  LDA #stdout_libc_device_id
  STA fd_table+fd_size      // Stdout and stderr are device 1
  STA fd_table+fd_size*2

  // Set the fd_used for 0, 1, and 2.
  LDA #1
  STA fd_table+fd_used
  STA fd_table+fd_size+fd_used
  STA fd_table+fd_size*2+fd_used

  // The rest are for files.
  LDA #file_libc_device_id
  LDX #fd_size*3
  LDY #max_fds-3
setup_fds_files_loop:
  STA fd_table, X
  INX
  INX
  INX
  INX
  DEY
  BNE setup_fds_files_loop
  RTS


// This table contains the addresses of individual hardware
// devices.  The first two are the ACIA (serial port for console).
// The third is for files, which are handled by the interpreter calling
// into the actual hosted OS provided functions.
devices:
  .hword stdin_device
  .hword stdout_device
  .hword file_device

stdout_device:
  .hword 0      // open
  .hword 0      // close
  .hword 0      // read
  .hword acia_write  // write
  .hword 0      // ioctl
  .hword acia_out_poll   // poll
  .hword 0      // lseek


stdin_device:
  .hword 0      // open
  .hword 0      // close
  .hword 0      // read
  .hword acia_read  // write
  .hword 0      // ioctl
  .hword acia_in_poll   // poll
  .hword 0      // lseek

// A file device.  fd_data holds an index into the
// open files table in the interpreter.
file_device:
  .hword file_open      // open
  .hword file_close      // close
  .hword file_read      // read
  .hword file_write  // write
  .hword file_ioctl      // ioctl
  .hword file_poll   // poll
  .hword file_lseek      // lseek


// Write a buffer of bytes to the ACIA
// Entry:
// sp+4: buffer to write
// sp+6: number of bytes to write
// Exit:
// X,Y: number of bytes written
//
// Scratch space:
// __i0: address of buffer
// __i1: number of bytes left to write.
acia_write:
  // Load buffer address into i0
  LDY #4
  LDA (__sp),Y
  STA __i0
  INY
  LDA (__sp),Y
  STA __i0+1

  // Load size in to i1
  INY
  LDA (__sp),Y
  STA __i1
  INY
  LDA (__sp),Y
  STA __i1+1

acia_write_loop:
  LDA __i1
  ORA __i1+1
  BEQ acia_write_end

acia_wc_loop:
  LDA (__i0)
  JSR console_write_char
  
  INC __i0
  BNE acia_wr1
  INC __i0+1
acia_wr1:
  DEC __i1
  BPL acia_wr2
  DEC __i1+1
acia_wr2:
  BRA acia_write_loop
acia_write_end:
  // Result is sp+6 - i1
  LDY #6
  SEC
  LDA (__sp),Y
  SBC __i1
  TAX
  INY
  LDA (__sp),Y
  SBC __i1+1
  TAY
  RTS


acia_read:
  // Load buffer address into i0
  LDY #4
  LDA (__sp),Y
  STA __i0
  INY
  LDA (__sp),Y
  STA __i0+1

  // Load size in to i1
  INY
  LDA (__sp),Y
  STA __i1
  INY

  LDA (__sp),Y
  STA __i1+1

acia_read_loop:
  LDA __i1
  ORA __i1+1
  BEQ acia_read_end
rwait:
  JSR console_read_char
  STA (__i0)

  INC __i0
  BNE acia_r1
  INC __i0+1
acia_r1:
  DEC __i1
  BPL acia_r2
  DEC __i1+1
acia_r2:
  BRA acia_read_loop

acia_read_end:
  // Result is sp+6 - i1
  LDY #6
  SEC
  LDA (__sp),Y
  SBC __i1
  TAX
  INY
  LDA (__sp),Y
  SBC __i1+1
  TAY
  RTS


// Exit:
// Carry set = can write
acia_out_poll:
  JMP console_poll_out

acia_in_poll:
  JMP console_poll_in

// Exit:
// X,Y: 2-byte result from interrupt call
.global file_open
file_open:
  RTS

// Entry:
// X: offset into fd table
// Exit:
// X,Y: fd index from fd_data in table entry
get_fd_index:
  LDA fd_table+fd_data,X
  PHA
  LDA fd_table+fd_data+1, X
  TAY
  PLX
  RTS

// Check that a file descriptor is open.  If not, this returns to the
// caller's caller, thus saving a check at each poitn of call.
//
// Entry:
// X: fd
// Exit if file is not open:
// X,Y = 0xff,0xff
// Returns to caller of caller.
// If file is open:
// Saves X.
check_fd_open:
  CPX #max_fds
  BCS file_not_open

  PHX
  TXA
  ASL A
  ASL A
  TAX
  LDA fd_table+fd_used,X
  BNE file_is_open
  PLA

file_not_open:
  // Pull return address off stack.
  PLA
  PLA

  // Load -1 into X,Y
  LDX #0xff
  LDY #0xff
  // Return to caller of caller.
  RTS
file_is_open:
  PLX
  RTS

.global file_close
file_close:
  JSR get_fd_index
  // TODO
  RTS

.global file_read
file_read:
  JSR get_fd_index
// TODO
  RTS

.global file_write
file_write:
  JSR get_fd_index
// TODO
  RTS

file_ioctl:
  JSR get_fd_index
// TODO
  RTS

file_poll:
  JSR get_fd_index
// TODO
  RTS

file_lseek:
  JSR get_fd_index
// TODO
  RTS

// Entry:
// A: offset of device function into device table entry.
// X: fd
// Exit:
// 0xfe,0xff: address of function to call
// X: offset into fd table at start of fd entry
get_libc_device_func:
  PHA
  STX 0xfd
  ASL 0xfd
  ASL 0xfd            // 0xfd = offset into fd table
  LDX 0xfd
  LDA fd_table, X     // Device id from fd.
  ASL A
  TAX                 // X = index into device table
  LDA devices,X
  STA 0xfe
  LDA devices+1,X
  STA 0xff            // 0xfe,0xff: address of device descriptor

  CLC
  PLA
  ADC 0xfe
  STA 0xfe
  LDA 0xff
  ADC #0
  STA 0xff            // 0xfe,0xff: address of write function address

  PHY
  LDA (0xfe)
  TAX
  LDY #1
  LDA (0xfe), Y
  STA 0xff
  STX 0xfe

  PLY
  LDX 0xfd
  RTS

// Convert a file descriptor into an offset into the fd table.
// Entry:
// A: fd
// Exit:
// X: offset
get_fd_offset:
  ASL A
  ASL A
  TAX
  RTS

// Convert an offset into the fd table into a file descriptor.
// Entry:
// X: offset into fd table
// Exit:
// X: fd
get_fd_from_offset:
  TXA
  LSR A
  LSR A
  TAX
  RTS

// Write a buffer of bytes to a device.
// Entry:
// X: fd to write to
// 0xfe,0xff: address of write function
// sp+4: buffer to write
// sp+6: number of bytes to write
// Exit:
// X,Y: number of bytes written

// Device write function is called with:
// X: offset into fd table of start of fd
write:
  JSR check_fd_open
  LDA #libc_device_write_func
  JSR get_libc_device_func
func_call:
  LDA 0xfe
  ORA 0xff
  BEQ no_func
  JMP (0xfe)
no_func:
  RTS

// Read a buffer of bytes from a device.
// Entry:
// X: fd to read from
// 0xfe,0xff: address of read function
// sp+4: buffer to read
// sp+6: number of bytes to read
// Exit:
// X,Y: number of bytes read
//
// Device read function is called with:
// X: offset into fd table of start of fd
read:
  JSR check_fd_open
  LDA #libc_device_read_func
  JSR get_libc_device_func
  JMP func_call

open:
  JMP file_open

close:
  JSR check_fd_open
  LDA #libc_device_close_func
  JSR get_libc_device_func
  JMP func_call

lseek:
  JSR check_fd_open
  LDA #libc_device_lseek_func
  JSR get_libc_device_func
  JMP func_call

ioctl:
  JSR check_fd_open
  LDA #libc_device_ioctl_func
  JSR get_libc_device_func
  JMP func_call

poll:
  JSR check_fd_open
  LDA #libc_device_poll_func
  JSR get_libc_device_func
  JMP func_call

.set max_syscall_number 7
syscalls:
.hword __abort      // 0
.hword __exit       // 1
.hword __open       // 2
.hword __close      // 3
.hword __write      // 4
.hword __read       // 5
.hword __lseek      // 6
.hword __abort      // 7

// Entry with A = syscall number (byte after BRK)
// uses some scratch space


.global __syscall_handler
__syscall_handler:
  CMP #max_syscall_number+1
  BCC syscall_ok
  LDA #0
syscall_ok:
  ASL A
  CLC
  ADC #%lo(syscalls)
  STA syscall_vector
  LDA #%hi(syscalls)
  ADC #0
  STA syscall_vector+1
  LDA (syscall_vector)
  TAX
  LDY #1
  LDA (syscall_vector), Y
  STA syscall_vector+1
  STX syscall_vector
  JMP (syscall_vector)


.global __exit
__exit:
  LDA #1
  .byte 0xef      // Interpreter exit


// write syscall
// Entry:
// sp+0: return address
// sp+2: fd
// sp+4: buffer
// sp+6: size
.global __write
__write:
  LDY #2
  LDA (__sp), Y     // Load fd (low byte only)
  TAX               // X = fd

  JSR write

  STX __i0
  STY __i0+1

  LDA (__sp)
  STA __result
  LDY #1
  LDA (__sp),Y
  STA __result+1

  // Store size in result.
  LDA __i0
  STA (__result)
  LDA __i0+1
  STA (__result),Y
  RTS

// read syscall
// Entry:
// sp+0: return address
// sp+2: fd
// sp+4: buffer
// sp+6: size
.global __read
__read:
  LDY #2
  LDA (__sp), Y     // Load fd (low byte only)
  TAX               // X = fd

  JSR read

  STX __i0
  STY __i0+1

  LDA (__sp)
  STA __result
  LDY #1
  LDA (__sp),Y
  STA __result+1

  // Store size in result.
  LDA __i0
  STA (__result)
  LDA __i0+1
  STA (__result),Y
  RTS

// Exit:
// X: index into fd_table (or 0 for none available)
find_free_fd:
  LDX #3*fd_size              // Ignore 0, 1 and 2
find_free_fd_loop:
  LDA fd_table+fd_used,X        // Used byte.
  BEQ free_fd_found
  INX
  INX
  INX
  INX
  CPX #max_fds*fd_size
  BCC find_free_fd_loop
  LDX #0
free_fd_found:
  RTS

.global __open
__open:
  JSR open

  STX __i0
  STY __i0+1

  // i0 contains fd index in interpreter's open_files array or -1
  TYA
  ORA __i0
  CMP #0xff
  BNE open_ok1
  TAX
  BRA open_failed

open_ok1:
  // Find a free fd entry
  JSR find_free_fd
  CPX #0
  BNE open_ok
  LDA #0xff
  STA __i0
  STA __i0+1
  BRA open_failed

open_ok:
  // Store the result from the interpreter into the fd_table entry's
  // data area.
  INX
  LDA __i0
  STA fd_table,X
  INX
  LDA __i0+1
  STA fd_table,X
  INX
  LDA #1              // Set used flag.
  STA fd_table,X
  JSR get_fd_from_offset    // Convert fd offset to fd.

open_failed:
  LDA (__sp)
  STA __result
  LDY #1
  LDA (__sp),Y
  STA __result+1

  // Store fd in result.
  TXA
  STA (__result)
  CMP #0xff
  BEQ open_ff
  LDA #0        // Store 0 in upper byte unless X is 0xff.
open_ff:
  STA (__result),Y
  RTS

__close:
  LDY #2
  LDA (__sp), Y     // Load fd (low byte only)
  TAX               // X = fd
  PHA

  JSR close

  STX __i0
  STY __i0+1
  TYA
  ORA __i0
  CMP #0xff
  BEQ close_failed

  // Close worked, write 0 into fd_used
  PLA
  JSR get_fd_offset
  LDA #0
  STA fd_table+fd_used, X
  BRA close_ok
close_failed:
  PLA
close_ok:
  // Write i0 to result.  This will be -1 or 0.
  LDA (__sp)
  STA __result
  LDY #1
  LDA (__sp),Y
  STA __result+1

  // Store fd in result.
  LDA __i0
  STA (__result)
  LDA __i0+1
  STA (__result),Y
  RTS


__lseek:
  LDY #2
  LDA (__sp), Y     // Load fd (low byte only)
  TAX               // X = fd

  JSR lseek

  STX __i0
  STY __i0+1

  // Write i0 to result.  This will be -1 or 0.
  LDA (__sp)
  STA __result
  LDY #1
  LDA (__sp),Y
  STA __result+1

  // Store result of lseek syscall in result.
  LDA __i0
  STA (__result)
  LDA __i0+1
  STA (__result),Y
  RTS

__abort:
  // TODO
  RTS
