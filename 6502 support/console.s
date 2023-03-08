//
//  console.s
//  c_compiler
//
//  Created by David Allison on 9/29/20.
//  Copyright © 2020 David Allison. All rights reserved.
//
#include "addresses.h"
#include "device.h"

.text

.global console_prompt
console_prompt:
  JSR console_write_string
  .asciz "1981> "
  RTS


.global console_crlf
console_crlf:
  JSR console_write_string
  .asciz "\r\n"
  RTS

// Write a string to the UART.
// Address of string (minus 1) is on the stack.
//
// +----------------+
// |     ...        |
// |     addr HI    |
// |     addr LO    |
// |                | <- S
// |                |
// .                .
// |                |
// +----------------+ <- 0x100
// Uses temp_addr as temporary.

.global console_write_string
console_write_string:
  TSX               // X = stack pointer
  LDA 0x101,X       // Load LO byte
  STA temp_addr     // Copy to temp addr
  LDA 0x102,X       // Load HI byte
  STA temp_addr+1
  INC temp_addr     // JSR puts return address -1 on stack.
  BNE ws_skip
  INC temp_addr+1
ws_skip:
  PHY                // Save Y.
  // Print the string in temp_addr
  JSR console_print_string
  CLC
  TYA                // Y is index of end of string.
  PLY                // Restore Y
  ADC temp_addr      // Add in start address LO.
  STA 0x101,X        // New return address LO
  LDA #0
  ADC temp_addr+1    // Add in start address HI.
  STA 0x102,X        // New return address HI.
  RTS

// Print string in temp_addr,temp_addr+1
// On exit, Y is the index at of the last char.
.global console_print_string
console_print_string:
  LDY #0xff
ps_loop:
  INY
  LDA (temp_addr),Y  // Single char.
  BEQ ps_end
  JSR console_write_char     // Write it.
  JMP ps_loop
ps_end:
  RTS

.global console_write_char
console_write_char:
  PHX
  LDX #ACIA1_BASE
  JSR acia_write_char
  PLX
  RTS


// Read a single char from the console.  Also echos it to the console.
// Returns A.
.global console_read_char
console_read_char:
  LDA ACIA1_CSR
  AND #1
  BEQ console_read_char
  LDA ACIA1_DATA
  STA ACIA1_DATA
  RTS

.global console_poll_in
console_poll_in:
  LDA ACIA1_CSR
  AND #1
  RTS

.global console_poll_out
console_poll_out:
  LDA ACIA1_CSR
  AND #2
  RTS
  
// Read a line into console_input_buffer.
// Returns length in Y.
.global console_read_line
console_read_line:
  LDY #0
rl_loop:
  JSR console_read_char
  CMP #0x08       // Backspace.
  BEQ del_char
  CMP #127        // Delete
  BNE ins_char
del_char:
  CPY #0
  BEQ rl_loop
  PHY
  JSR console_write_string
  .asciz "\x08 \x08"
  PLY
  DEY
  JMP rl_loop
ins_char:
  STA (console_input_buffer),Y
  INY
  CMP #13
  BNE rl_loop
  DEY         // Remove CR

  // Terminate with 0.
  LDA #0
  STA (console_input_buffer),Y
  LDA #10
  JSR console_write_char
  RTS

// This function converts a 16 bit binary value into a 24 bit BCD. It
// works by transferring one bit a time from the source and adding it
// into a BCD value that is being doubled on each iteration. As all the
// arithmetic is being done in BCD the result is a binary to decimal
// conversion. All conversions take 915 clock cycles.
//
// See BINBCD8 for more details of its operation.
//
// Andrew Jacobs, 28-Feb-2004

.global console_print_decimal
console_print_decimal:
  PHX
  SED                     // Switch to decimal mode
  STZ decimal_out+0
  STZ decimal_out+1
  STZ decimal_out+2
  LDX #16                 // The number of source bits

dec_bit:
  ASL decimal_in+0        // Shift out one bit
  ROL decimal_in+1
  LDA decimal_out+0       // And add into result
  ADC decimal_out+0
  STA decimal_out+0
  LDA decimal_out+1       // propagating any carry
  ADC decimal_out+1
  STA decimal_out+1
  LDA decimal_out+2       // ... thru whole result
  ADC decimal_out+2
  STA decimal_out+2
  DEX                     // And repeat for next bit
  BNE dec_bit
  CLD                     // Back to binary
  
  PLX
  RTS
  
  
  
  
