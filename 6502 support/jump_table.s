//
//  jump_table.s
//  c_compiler
//
//  Created by David Allison on 6/22/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "vars.s"
// Functions are emitted in per-symbol ELF sections.

.global __jump_table1
.global __jump_table2
.global __jump_table4
.global __jump_table8


// Entry A: offet into zero page containing entry offset into table.
// We multiply this by 2 to get the byte offset into the table.
// The table is immediately after the JSR instruction
.section ".text.__jump_table1", "ax", @progbits
__jump_table1:
  TAY               // Offset of entry number in zero page.
  TSX               // X = stack pointer
  LDA 0x101,X       // Load LO byte
  STA __t0          // Copy to temp addr
  LDA 0x102,X       // Load HI byte
  STA __t1
  INC __t0          // JSR puts return address -1 on stack.
  BNE jp1_skip
  INC __t1
jp1_skip:
  // t0, t1 contain jump table address
  LDA 0,Y     // A = offset into table in entries
  STA __t2
  // t2 and t3 contain the offset in entries
  // Multiply by 2 to get offset in bytes.
  ASL __t2

  // Add offset in bytes to jump table address.
  CLC
  LDA __t2
  ADC __t0
  STA __t0
  LDA __t1
  ADC #0
  STA __t1

  // Remove return address from stack.
  TSX
  INX
  INX
  TXS

  // Jump indirect via jump table entry.
  LDA (__t0)
  STA __t2
  LDY #1
  LDA (__t0),Y
  STA __t2+1
  JMP (__t2)

.section ".text.__jump_table2", "ax", @progbits
__jump_table2:
  TAY               // Offset of entry number in zero page.
  TSX               // X = stack pointer
  LDA 0x101,X       // Load LO byte
  STA __t0          // Copy to temp addr
  LDA 0x102,X       // Load HI byte
  STA __t1
  INC __t0          // JSR puts return address -1 on stack.
  BNE jp2_skip
  INC __t1
jp2_skip:
  // t0, t1 contain jump table address
  LDA 0,Y     // A = offset into table in entries
  STA __t2
  LDA 1,Y
  STA __t3
  // t2 and t3 contain the offset in entries
  // Multiply by 2 to get offset in bytes.
  ASL __t2
  ROL __t3

  // Add offset in bytes to jump table address.
  CLC
  LDA __t2
  ADC __t0
  STA __t0
  LDA __t3
  ADC __t1
  STA __t1

  // Remove return address from stack.
  TSX
  INX
  INX
  TXS

  // Jump indirect via jump table entry.
  LDA (__t0)
  STA __t2
  LDY #1
  LDA (__t0),Y
  STA __t2+1
  JMP (__t2)

.section ".text.__jump_table4", "ax", @progbits
__jump_table4:
  TAY               // Offset of entry number in zero page.
  TSX               // X = stack pointer
  LDA 0x101,X       // Load LO byte
  STA __t0          // Copy to temp addr
  LDA 0x102,X       // Load HI byte
  STA __t1
  INC __t0          // JSR puts return address -1 on stack.
  BNE jp4_skip
  INC __t1
jp4_skip:
  // t0, t1 contain jump table address
  LDA 0,Y     // A = offset into table in entries
  STA __t2
  LDA 1,Y
  STA __t3
  // t2 and t3 contain the offset in entries
  // Multiply by 2 to get offset in bytes.
  ASL __t2
  ROL __t3

  // Add offset in bytes to jump table address.
  CLC
  LDA __t2
  ADC __t0
  STA __t0
  LDA __t3
  ADC __t1
  STA __t1

  // Remove return address from stack.
  TSX
  INX
  INX
  TXS

  // Jump indirect via jump table entry.
  LDA (__t0)
  STA __t2
  LDY #1
  LDA (__t0),Y
  STA __t2+1
  JMP (__t2)

.section ".text.__jump_table8", "ax", @progbits
__jump_table8:
  TAY               // Offset of entry number in zero page.
  TSX               // X = stack pointer
  LDA 0x101,X       // Load LO byte
  STA __t0          // Copy to temp addr
  LDA 0x102,X       // Load HI byte
  STA __t1
  INC __t0          // JSR puts return address -1 on stack.
  BNE jp8_skip
  INC __t1
jp8_skip:
  // t0, t1 contain jump table address
  LDA 0,Y     // A = offset into table in entries
  STA __t2
  LDA 1,Y
  STA __t3
  // t2 and t3 contain the offset in entries
  // Multiply by 2 to get offset in bytes.
  ASL __t2
  ROL __t3

  // Add offset in bytes to jump table address.
  CLC
  LDA __t2
  ADC __t0
  STA __t0
  LDA __t3
  ADC __t1
  STA __t1

  // Remove return address from stack.
  TSX
  INX
  INX
  TXS

  // Jump indirect via jump table entry.
  LDA (__t0)
  STA __t2
  LDY #1
  LDA (__t0),Y
  STA __t2+1
  JMP (__t2)
