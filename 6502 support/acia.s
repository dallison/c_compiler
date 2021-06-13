//
//  acia.s
//  c_compiler
//
//  Created by David Allison on 9/27/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include "addresses.h"
#include "device.h"

.text

.global acia1_init
acia1_init:
  // ACIA1 Master reset.
  LDA #3
  STA device_base_addr+ACIA1_BASE
  // A little delay.  I've see this somewhere.
  LDY #128
acia1_reset_loop:
  DEY
  BNE acia1_reset_loop
  // Divider: 16 (1.8432Mhz / 16 = 115.2K)
  // 8 bits, 1 stop bit, no parity.
  // Read interrupt disabled.
  LDA # (1 | (4 << 2) | (0 << 5) | (0 << 7))
  STA device_base_addr+ACIA1_BASE
  RTS
  
.global acia2_init
acia2_init:
  // ACIA Master reset.
  LDA #3
  STA device_base_addr+ACIA2_BASE
  // A little delay.  I've see this somewhere.
  LDY #128
acia2_reset_loop:
  DEY
  BNE acia2_reset_loop
  // Divider: 16 (1.8432Mhz / 16 = 115.2K)
  // 8 bits, 1 stop bit, no parity.
  // Read interrupt enabled.
  LDA # (1 | (4 << 2) | (0 << 5) | (1 << 7))
  STA device_base_addr+ACIA2_BASE
  STZ serial_read_index
  STZ serial_write_index
  STZ serial_num_bytes
  RTS

acia2_write_char:
  LDX #ACIA2_BASE
  
// Write character in A Corrupts A.
// X is low byte of ACIA_DATA address.
// Saves A,X, Y
// Corrupts X
.global acia_write_char
acia_write_char:
  PHA
acia_wc_loop:
  LDA device_base_addr,X
  AND #2             // Check TDRE
  BEQ acia_wc_loop
  PLA
  STA device_base_addr+1,X
  RTS

// Reads some bytes into temp_addr.  Blocks until all bytes
// are read.
// Entry:
// temp_addr: address of buffer to write.
// X: ACIA base address (added to 0xffe00)
// Y: length to read
// Corrupts A, X, byteA
// Y is length read.
.global acia_read
acia_read:
  STY byteA
  LDY #0
acia_read_loop:
  CPY byteA
  BEQ acia_read_end
acia_wait_char:
  LDA device_base_addr,X
  AND #1
  BEQ acia_wait_char
  LDA device_base_addr+1,X
  STA (temp_addr), Y
  INY
  JSR print_as_hex
  JMP acia_read_loop
acia_read_end:
  RTS

acia1_read:
  LDX #ACIA1_BASE
  BEQ acia_read

acia2_read:
  LDX #ACIA2_BASE
  BNE acia_read


acia1_write:
  LDX #ACIA1_BASE     // This is zero.
  BEQ acia_write

acia2_write:
  LDX #ACIA2_BASE     // This is non-zero
  // Fall through.

// Entry:
// temp_addr: address of buffer to write.
// X: ACIA base address (added to 0xfe00)
// Y: length to write
// Saves: Y
// Corrupts A and byteA
.global acia_write
acia_write:
  STY byteA
  LDY #0
acia_write_loop:
  CPY byteA
  BEQ acia_write_end
  LDA (temp_addr),Y
  JSR acia_write_char
  INY
  JMP acia_write_loop
acia_write_end:
  RTS

// A=0 => POLLIN
// A=1 => POLLOUT
acia1_poll:
  LDX #ACIA1_BASE
  BEQ acia_poll

acia2_poll:
  LDX #ACIA2_BASE
  // Fall through

acia_poll:
  CLC
  ADC #1
  AND device_base_addr,X
  RTS

// 6550 ACIA1 device (serial console)
.global acia1_device
acia1_device:
.hword acia1_init
.hword 0           // Close.
.hword acia1_read
.hword acia1_write
.hword 0           // Ioctl
.hword acia1_poll

// 6550 ACIA2 device (serial bus)
.global acia2_device
acia2_device:
.hword acia2_init
.hword 0
.hword acia2_read
.hword acia2_write
.hword 0
.hword acia2_poll
