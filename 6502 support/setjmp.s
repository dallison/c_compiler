//
//  setjmp.s
//  c_compiler
//
//  Created by David Allison on 1/5/23.
//  Copyright © 2023 David Allison. All rights reserved.
//

#include "vars.s"
#include "fp.s"

.text

.global setjmp
.global longjmp

// jmp_buf is defined as follows:
// struct __jmp_buf {
//  char b[6];
//  int i[12];
//  long l[6];
//  long long x[3];
//  float f[3];
//  int sp;
//  int fp;
//  int result;
//  char machine_sp;
// };

// Save a set of registers:
// Entry:
// X: start index of registers
// Y: number of bytes to copy
// t0,t1: address to copy to
saveregs:
  STY __t2
  LDY #0
saveloop:
  LDA 0,X
  STA (__t0),Y
  INX
  INY
  CPY __t2
  BNE saveloop
  RTS

// Restore a set of registers:
// Entry:
// X: start index of registers
// Y: number of bytes to copy
// t0,t1: address to copy from
restoreregs:
  STY __t2
  LDY #0
restoreloop:
  LDA (__t0),Y
  STA 0,X
  INX
  INY
  CPY __t2
  BNE restoreloop
  RTS

reg_offsets:
  .byte __b0+2, __i0+8, __l0+8, __x0+8, __f0+4

reg_sizes:
  .byte 6*1, 12*2, 6*4, 3*8, 3*4

// Entry:
// sp,sp+1: address of jmp_buf
// X,Y: address for result.
setjmp:
  STX __result
  STY __result+1

  // Copy all registers into the jmp_buf.
  LDA #0
  STA __b0    // Register set index.

  // Get jmp_buf from stack and store in __t0,__t1,
  LDA (__sp)
  STA __t0
  LDY #1
  LDA (__sp),Y
  STA __t1
setjmploop:
  LDX __b0
  LDA reg_offsets,X
  LDY reg_sizes,X
  PHY
  TAX
  JSR saveregs

  // Increment __t0,__t1 by size of registers stored.
  PLA
  CLC
  ADC __t0
  STA __t0
  LDA __t1
  ADC #0
  STA __t1

  // Next register set.
  INC __b0
  LDA __b0
  CMP #5
  BNE setjmploop

  // t0,t1 points to address after registers.

  // Save sp, fp and result.
  LDY #0
  LDA __sp
  STA (__t0),Y
  INY
  LDA __sp+1
  STA (__t0),Y
  INY

  LDA __fp
  STA (__t0),Y
  INY
  LDA __fp+1
  STA (__t0),Y
  INY

  LDA __result
  STA (__t0),Y
  INY
  LDA __result+1
  STA (__t0),Y
  INY

  // Save machine sp
  TSX
  TXA
  STA (__t0), Y
  INY

  // Save return address
  LDA 0x101,X
  STA (__t0), Y
  INY
  LDA 0x102,X
  STA (__t0), Y

  // Return 0.
  LDA #0
  STA (__result)
  LDY #1
  STA (__result),Y
  RTS

// Entry:
// sp,sp+1: address of jmp_buf
// sp+2,sp+3: value to return from function.
longjmp:
  STX __result
  STY __result+1

  // Copy all registers from jmp_buf.
  LDA #0
  STA __b0    // Register set index.

  // Get jmp_buf from stack and store in __t0,__t1,
  LDA (__sp)
  STA __t0
  LDY #1
  LDA (__sp),Y
  STA __t1
longjmploop:
  LDX __b0
  LDA reg_offsets,X
  LDY reg_sizes,X
  PHY
  TAX
  JSR restoreregs

  // Increment __t0,__t1 by size of registers stored.
  PLA
  CLC
  ADC __t0
  STA __t0
  LDA __t1
  ADC #0
  STA __t1

  // Next register set.
  INC __b0
  LDA __b0
  CMP #5
  BNE longjmploop

  // t0,t1 points to address after registers.
  // Restore result.
  LDY #4
  LDA (__t0),Y
  STA __result
  INY
  LDA (__t0),Y
  STA __result+1

  // Copy return value to result.
  LDY #2
  LDA (__sp),Y
  STA (__result)
  INY
  LDA (__sp), Y
  LDY #1
  STA (__result),Y

  // Restore sp, fp.
  LDY #0
  LDA (__t0),Y
  STA __sp
  INY
  LDA (__t0),Y
  STA __sp+1
  INY

  LDA (__t0),Y
  STA __fp
  INY
  LDA (__t0),Y
  STA __fp+1

  // Resore machine sp
  LDY #6
  LDA (__t0), Y
  TAX
  TXS
  INY

  // Restore return address
  LDA (__t0), Y
  STA 0x101,X
  INY
  LDA (__t0), Y
  STA 0x102,X

  // Return from setjmp.
  RTS
