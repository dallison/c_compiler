//
//  hexdump.s
//  c_compiler
//
//  Created by David Allison on 9/29/20.
//  Copyright © 2020 David Allison. All rights reserved.
//
#include "addresses.h"
#include "device.h"

.text

// Output number (X=LO, Y=HI) in hex to output.
// Saves X and Y.
// Corrupts A.
// Always outputs 4 characters.
.global print_as_hex2
print_as_hex2:
  PHX
  PHY
  PHX
  TYA
  JSR print_as_hex      // HI byte
  PLA
  JSR print_as_hex      // LO byte
  PLY
  PLX
  RTS


// Output A as hex to console.
.global print_as_hex
print_as_hex:
  PHP
  PHA
  PHY
  PHX
  LDY #2
  TAX           // Save A
  LSR A         // Shift right by 4
  LSR A
  LSR A
  LSR A
as_hex_loop:
  CMP #10
  BCC as_hex_num
  ADC #('a' - 10 - 1)   // Carry is set
  BCC as_hex1
as_hex_num:
  CLC
  ADC #'0'
as_hex1:
  JSR console_write_char
  DEY
  BEQ as_hex_end
  TXA
  AND #15
  JMP as_hex_loop
as_hex_end:
  PLX
  PLY
  PLA
  PLP
  RTS

// Entry:
// X,Y: address (LO,HI)
// dump_length: length in bytes
.global hex_dump
hex_dump:
  STX dump_addr        // Store X,Y in dump_addr
  STY dump_addr+1
  CLC
  LDA dump_length
  ADC dump_addr        // Add dump_length to get dump_end_addr.
  STA dump_end_addr
  LDA dump_length+1
  ADC dump_addr+1
  STA dump_end_addr+1
hex_dump_loop1:
  LDA dump_addr+1
  CMP dump_end_addr+1
  BCC hex_dump_l1
  BNE hex_dump_end
  LDA dump_addr
  CMP dump_end_addr
  BCS hex_dump_end
  
hex_dump_l1:
  // Output address.
  LDX dump_addr
  LDY dump_addr+1
  JSR print_as_hex2
  JSR space
  JSR space

  // Output 16 bytes in hex.
  LDY #0
hex_dump_loop2:
  LDA (dump_addr),Y
  JSR print_as_hex
  JSR space
  INY
  CPY #8              // Extra space after 8 bytes
  BNE hex_dump8
  JSR space
hex_dump8:
  CPY #16
  BCC hex_dump_loop2
  JSR space
  JSR space

  // Output 16 bytes in ASCII
  LDY #0
hex_dump_loop3:
  LDA (dump_addr),Y
  CMP #' '
  BCC hex_nonprint
  CMP #127
  BCC hex_isprint
hex_nonprint:
  LDA #'.'
hex_isprint:
  JSR console_write_char
  INY
  CPY #8              // Space after 8 bytes
  BNE hex_dump8b
  JSR space
hex_dump8b:
  CPY #16
  BCC hex_dump_loop3
  JSR console_crlf

  // Next 16 bytes
  LDA #16
  CLC
  ADC dump_addr
  STA dump_addr
  LDA #0
  ADC dump_addr+1
  STA dump_addr+1
  JMP hex_dump_loop1

hex_dump_end:
  RTS




space:
  LDA #' '
  JMP console_write_char

