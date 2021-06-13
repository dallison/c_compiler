//
//  6502rom.s
//  c_compiler
//
//  Created by David Allison on 7/1/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "addresses.h"
#include "device.h"

.text

// The ROM starts at 0xc000 and is 16K long.
.global start
start:
  JSR init_devices
  STZ exe_addr
  STZ exe_addr+1

  // Init console buffer.
  LDA #(console_input_buffer_addr & 0xff)
  STA console_input_buffer
  LDA #(console_input_buffer_addr >> 8)
  STA console_input_buffer+1

  JSR console_write_string
  .asciz "\033[2J\033[1;1HWelcome to the 80s...\n\r"
loop:
  JSR console_prompt
  JSR console_read_line
  JSR parse_command
  JMP loop
  
init_devices:
  JSR acia1_init
  JSR acia2_init
  LDA #0
  STA rega
  STA REGA
  LDA #1
  STA seqnum
  RTS

// Skip spaces in console_input_buffer.  Y is index
// Updates Y to point to first non-space.
skip_spaces:
  LDA (console_input_buffer),Y
  JSR isspace
  BCC ss_ret
  INY
  BNE skip_spaces
ss_ret:
  RTS

// Copy the string in (console_input_buffer),Y to temp_buffer.
// On exit Y is the index after the copied string.
// X is index of NUL in temp_buffer.
get_string:
  LDX #0
gs_loop:
  LDA (console_input_buffer),Y
  BEQ get_string_end
  JSR isspace
  BCS get_string_end
  STA temp_buffer,X
  INY
  INX
  BNE gs_loop
get_string_end:
  STZ temp_buffer,X
  RTS

compare_command:
  PHY
  PHX
  LDY #0
cc_loop:
  LDA %abs(commands),X
  BEQ cc_end
  CMP temp_buffer,Y
  BNE cc_end
  INX
  INY
  BNE cc_loop
cc_end:
  SEC
  SBC temp_buffer,Y
  PLX
  PLY
  RTS
  
// Parse a command in console_input_buffer.
parse_command:
  LDY #0
  JSR skip_spaces
  LDA (console_input_buffer),Y
  BEQ no_command
  JSR get_string
  LDX #0              // Index into commands array.
pc_loop:
  LDA %abs(commands),X      // Load length of command.
  BEQ pc_bad          // 0 means end of commands.
  PHA                 // Length on stack.
  INX                 // Move to start of command name.
  JSR compare_command
  CMP #0
  BEQ pc_found        // Command found (length still on stack)
  STX byteA       // byteA = index of start of command name.
  PLA             // Pull length off stack.
  CLC
  ADC byteA       // Add to command name index.
  ADC #2          // Add in func address.
  TAX             // This is new X (length of next command).
  BNE pc_loop

pc_bad:
  JSR console_write_string
  .asciz "Unknown command\r\n"
  RTS
  
pc_found:
  STX byteA
  PLA                   // Get length of command name.
  CLC
  ADC byteA             // Add to index.
  TAX                   // X = index into commands of func.
  LDA %abs(commands),X        // Low byte of func
  STA temp_addr
  INX
  LDA %abs(commands),X        // High byte of func
  STA temp_addr+1
  JMP (temp_addr)       // Invoke func.
  
no_command:
  RTS

.global hex_dump_command
hex_dump_command:
  JSR skip_spaces
  LDA (console_input_buffer),Y
  BEQ hex_usage
  
  JSR to_hex2
  LDA dump_length
  PHA
  LDA dump_length+1
  PHA
  JSR skip_spaces
  LDA (console_input_buffer),Y
  BNE hex_parse_length
  LDA #64
  STA dump_length
  LDA #0
  STA dump_length+1
  BEQ hex_go
hex_parse_length:
  JSR to_hex2
hex_go:
  PLY
  PLX
  JMP hex_dump

hex_usage:
  JSR console_write_string
  .asciz "usage: dump addr [length]\r\n"
  RTS
  
// Convert console_input_buffer+Y to hex.  Result in dump_length, Y is after last char
// consumed.
.global to_hex2
to_hex2:
  LDA #0
  STA dump_length
  STA dump_length+1
to_hex_loop:
  LDA (console_input_buffer),Y
  INY
  JSR isdigit
  BCS to_hex_digit
  JSR isxdigit
  BCS to_hex_alpha
to_hex_end:
  DEY
  RTS
to_hex_digit:
  SBC #'0'         // Carry is set.
  JMP to_hex_shift
to_hex_alpha:
  JSR toupper
  SEC
  SBC #'A'-10
to_hex_shift:
  LDX #4
to_hex_shift_loop:
  ASL dump_length
  ROL dump_length+1
  DEX
  BNE to_hex_shift_loop

  // A contains 0x00-0x0f
  ORA dump_length
  STA dump_length
  JMP to_hex_loop

who_command:
  JSR console_write_string
  .asciz "Robodave\r\n"
  RTS
  
ping_command:
 JSR ping
 RTS
 
  
ls_command:
  JSR ls
  RTS

cat_command:
  JSR skip_spaces
  LDA (console_input_buffer),Y
  BEQ cat_usage
  JSR get_string
  JSR cat
  RTS

cat_usage:
  JSR console_write_string
  .asciz "usage: cat <filename>\r\n"
  RTS

load_command:
  JSR skip_spaces
  LDA (console_input_buffer),Y
  BEQ load_usage
  JSR get_string
  JSR load
  RTS
load_usage:
  JSR console_write_string
  .asciz "usage: load <filename>\r\n"
  RTS

go_command:
  LDA exe_addr
  ORA exe_addr+1
  BNE go_ok
  JSR console_write_string
  .asciz "No kernel loaded"
  RTS
go_ok:
  JMP (exe_addr)
  
boot_command:
  JSR boot
  RTS
 
#if 0
#include "../acia.s"
#include "../console.s"
#include "../device.s"
#include "../hexdump.s"
#include "../utils.s"
#include "../server.s"
#include "../comms.s"
#endif

.p2align 8
commands:
  .byte 5
  .asciz "dump"
  .hword hex_dump_command

  .byte 4
  .asciz "who"
  .hword who_command

  .byte 5
  .asciz "ping"
  .hword ping_command

  .byte 3
  .asciz "ls"
  .hword ls_command

  .byte 4
  .asciz "cat"
  .hword cat_command

  .byte 5
  .asciz "load"
  .hword load_command

  .byte 3
  .asciz "go"
  .hword go_command
  
  .byte 5
  .asciz "boot"
  .hword boot_command
  
  .byte 2
  .asciz "b"
  .hword boot_command
  
  .byte 0
  
end_of_code:

// Space to start of permanent ROM.
//.space PERM_ROM-ROM_START-end_of_code

.section ".boot", "ax", @progbits
// Start of boot ROM at FF00
.global perm_rom
perm_rom:
reset_handler:
  SEI
  CLD
  
  // Set up vectors in vector_ram as JMP instructions.
  LDX #0        // Index into vectors.
  LDY #0        // Index into vector_ram
vector_loop:
  // Load vector into addrA.
  LDA %abs(vectors), X
  STA addrA
  INX
  LDA %abs(vectors), X
  STA addrA+1
  INX
  
  // Check for zero, end if so.
  ORA addrA
  BEQ end_vectors
  
  // Write JMP abs instruction into next vector_ram location.
  LDA #0x4c       // JMP abs
  STA vector_ram, Y
  INY
  LDA addrA
  STA vector_ram, Y
  INY
  LDA addrA+1
  STA vector_ram, Y
  INY
  JMP vector_loop

vectors:
  .hword rom_irq
  .hword rom_nmi
  .hword rom_brk
  .hword console_write_string
  .hword 0
  
end_vectors:

#if 0
  // Setup IRQV.
  LDA #0x4c       // JMP abs
  STA irqv
  LDA #%lo(rom_irq)
  STA irqv+1
  LDA #%hi(rom_irq)
  STA irqv+2
  
  // Setup NMIV.
  LDA #0x4c       // JMP abs
  STA nmiv
  LDA #%lo(rom_nmi)
  STA nmiv+1
  LDA #%hi(rom_nmi)
  STA nmiv+2
  
  // Setup BRKV.
  LDA #0x4c       // JMP abs
  STA brkv
  LDA #%lo(rom_brk)
  STA brkv+1
  LDA #%hi(rom_brk)
  STA brkv+2
#endif

  // Reset stack pointer.
  LDX #0xff
  TXS
  
  // Enable interrupts.
  CLI

  // Main loop.
  JMP start

  
// Stack on entry:
// +-------------------+
// |                   |
// +-------------------+
// |      PC hi        |
// +-------------------+
// |      PC lo        |
// +-------------------+
// |      P            |  <- X + 2 (0x102,X)
// +-------------------+
// |                   |  <- S

// Read the serial port as early as possible.
// The serial port runs at 115200 Hz.  The CPU is
// running at 2MHz.  There are 10 bits per byte
// so bytes will arrive at 11520Hz which gives us
// about 173 cycles to read the port after the byte
// arrives.

irq_handler:
  STA irq_accum             // 2 cycles - 9
  STX irq_x                 // 2 cycles - 11
  LDA ACIA2_CSR             // 4 cycles - 15
  STA serial_input_status   // 2 cycles - 17
  BIT #0x80         // Check IRQ bit 3 cycles - 20
  BEQ not_acia2a            // 2 cycles - 22
  BIT #0x01         // Check RDFR.  3 cycles - 25
  BEQ not_acia2a     // 2 cycles - 27
not_acia2a:
  LDA ACIA2_DATA            // 4 cycles - 31 (read complete)
  STA serial_input_data
  
  // Check for BRK.
  // TSX
  // LDA 0x102,X // get the status register from the stack
  // AND #0x10   // mask B flag
  // BNE brk_handler
  
  // IRQ: invoke IRQV
  JSR irqv
  LDA irq_accum
  LDX irq_x
  RTI

brk_handler:
  // BRK: invoke BRKV
  JSR brkv
  PLA
  PLX
  RTI

nmi_handler:
  JSR nmiv
  RTI

rom_nmi:
  RTS
  
// ROM IRQ handler.
// These handlers use RTS to return, not RTI.
.global rom_irq
rom_irq:
  // ROM IRQ checks.
  // Check for IRQ on ACIA2.

  LDA serial_input_status
  BIT #0x80         // Check IRQ bit
  BEQ not_acia2
  BIT #0x01         // Check RDFR.
  BEQ not_acia2
  LDA serial_input_data               // Load data from serial port
  LDX serial_write_index              // Get serial write index.
  STA input_ring_buffer,X             // Store byte in input ring buffer.
  INC serial_write_index              // Move forward one byte.
  INC serial_num_bytes                // One more byte.
not_acia2:
  RTS
  
rom_brk:
  RTS
  


end_of_rom:

// Space to vectors.
//.space ROM_VECTORS-ROM_START-end_of_rom

// Hardware vectors - always ROM.
.section ".hwvectors", "ax", @progbits, 1
.global hwvectors
hwvectors:
.hword nmi_handler
.hword reset_handler
.hword irq_handler


