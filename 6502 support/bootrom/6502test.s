//
//  6502test.s
//  c_compiler
//
//  Created by David Allison on 9/21/23.
//  Copyright © 2023 David Allison. All rights reserved.
//

// LED array.
.set REGA 0xfec0

.text

.global _start
_start:
  LDA #0
loop:
  STA REGA
  LDX #20
delay1:
  LDY #0
delay:
  DEY
  BNE delay
  DEX
  BNE delay1
  INC A
  JMP loop

.section ".boot", "ax", @progbits
// Start of boot ROM at FF00
.global perm_rom
perm_rom:
reset_handler:
  SEI
  CLD
  
  // Reset stack pointer.
  LDX #0xff
  TXS
  
  // Enable interrupts.
  CLI

  // Main loop.
  JMP _start

nmi_handler:
irq_handler:
  RTI

.section ".hwvectors", "ax", @progbits, 1
.global hwvectors
hwvectors:
.hword nmi_handler
.hword reset_handler
.hword irq_handler

