//
//  6502rom.s
//  c_compiler
//
//  Created by David Allison on 7/1/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

.set SAVED_A 0x200    // Saved A
.set SAVED_X 0x201
.set SAVED_Y 0x202
.set SAVED_S 0x203
.set SAVED_P 0x204

.set ROM_START 0xc000
.set PERM_ROM 0xff00
.set ROM_VECTORS 0xfffa

// VIA (6522 at address 0xfe00)
.set VIA_ORB 0xfe00
.set VIA_ORA 0xfe01
.set VIA_DDRB 0xfe02
.set VIA_DDRA 0xfe03

// The ROM starts at 0xc000 and is 16K long.
.text
.global main
main:
  LDA #0xff
  STA VIA_DDRA
  STA VIA_DDRB
  LDX #0
loop:
  STX VIA_ORA
  INX
  JMP loop


// ROM entry point for syscalls.  Entered in KMODE with interrupts
// disabled.  X contains BRK argument.
rom_brk:
  JMP return_to_user

end_of_code:

// Space to start of permanent ROM.
.space PERM_ROM-end_of_code-ROM_START

// Start of permanent ROM at FF00
perm_rom:
reset_handler:
  LDX #0xff
  TXS
  CLI
  JMP main

//.global irq_handler
irq_handler:
  STA SAVED_A         // In user memory.
  STX SAVED_X
  STY SAVED_Y
  TSX               // Stack pointer in X
  STX SAVED_S

  LDA 0x101,X
  AND #0x10          // B bit
  BNE brk
irq:
  // IRQ handling code here.
  // ...
  JMP end_of_irq

// BRK handler, uses 0xfe,0xff in zero page.
brk:
  LDA 0x102,X   // Low addr of BRK return address
  SEC
  CLD
  SBC #1
  STA 0xfe
  LDA 0x103,X   // High half
  SBC #0
  STA 0xff
  LDY #0
  LDA (0xfe),Y    // BRK argument
  TAX

  // Enter K Mode
  LDA 0xfe01
  ORA #0x80
  STA 0xfe01
  JMP rom_brk

end_of_irq:
  LDX SAVED_S
  TXS
  LDA SAVED_A
  LDX SAVED_X
  LDY SAVED_Y

  RTI

nmi_handler:
  RTI

return_to_user:
  LDA VIA_ORA
  AND #0x7f
  STA VIA_ORA
  JMP end_of_irq

end_of_rom:

// Space to vectors.
.space ROM_VECTORS-end_of_rom-ROM_START

// Hardware vectors - always ROM.
.hword nmi_handler
.hword reset_handler
.hword irq_handler


