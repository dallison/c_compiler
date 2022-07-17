#include "vars.s"
#include "fp.s"

.text

.global __add128
.global __add256
.global __lshift128
.global __lshift256
.global __rshift256
.global __inc128
.global __inc256

// Integer to float (4 byte floating point).
// Entry:
//  A: destination register offset
//  X: source register offset
// Algorithm:
// Shift the integer to the right until we get zero.  Count the number of
// shifts.  Shift it into the mantissa of the result from the left.  The
// exponent is the count of the shift + 127.

__i1tof:
  PHA
  LDA 0,X       // Check for zero
  BNE i1tof_nonzero
  JMP __fzero
i1tof_nonzero:
  // Not zero, check for negative.
  STZ fsign
  BPL pos_i1tof
  LDA #0x80
  STA fsign      // Sign bit set.
  SEC
  LDA #0        // Negate int value.
  SBC 0,X
  STA 0,X
pos_i1tof:
  LDA #127        // Set exponent to bias value.
  STA fexp
  JSR __fzero_mantissa
  CLC
i1tof_loop:
  // C contains the bottom bit of src.
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1

  // Shift int with low bit in C.
  LSR 0,X
  // All bits shifted out?
  BEQ tof_done
  INC fexp
  BRA i1tof_loop
tof_done:
  // One more shift right
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1
  PLX
  JMP __fassemble

__ui1tof:
  PHA
  LDA 0,X       // Check for zero
  BNE ui1tof_nonzero
  JMP __fzero
ui1tof_nonzero:
  // Not zero, check for negative.
  STZ fsign
  LDA #127        // Set exponent to bias value.
  STA fexp
  JSR __fzero_mantissa
  CLC
ui1tof_loop:
  // C contains the bottom bit of src.
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1

  // Shift int with low bit in C.
  LSR 0,X
  // All bits shifted out?
  BEQ tof_done
  INC fexp
  BRA ui1tof_loop

__i2tof:
  PHA
  LDA 0,X       // Check for zero
  ORA 1,X
  BNE i2tof_nonzero
  JMP __fzero
i2tof_nonzero:

  // Not zero, check for negative.
  STZ fsign
  LDA 1,X       // Top byte of source
  BPL pos_i2tof
  LDA #0x80
  STA fsign      // Sign bit set.
  SEC
  LDA #0        // Negate int value.
  SBC 0,X
  STA 0,X
  LDA #0
  SBC 1,X
  STA 1,X
pos_i2tof:
  LDA #127        // Set exponent to bias value.
  STA fexp
  JSR __fzero_mantissa
  CLC
i2tof_loop:
  // C contains the bottom bit of src.
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1

  // Shift int with low bit in C.
  LSR 1,X
  ROR 0,X

  // All bits shifted out?
  LDA 0,X
  ORA 1,X
  BEQ tof_done
  INC fexp
  BRA i2tof_loop

__ui2tof:
  PHA
  LDA 0,X       // Check for zero
  ORA 1,X
  BNE ui2tof_nonzero
  JMP __fzero
ui2tof_nonzero:

  // Not zero, check for negative.
  STZ fsign
  LDA #127        // Set exponent to bias value.
  STA fexp
  JSR __fzero_mantissa
  CLC
ui2tof_loop:
  // C contains the bottom bit of src.
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1

  // Shift int with low bit in C.
  LSR 1,X
  ROR 0,X

  // All bits shifted out?
  LDA 0,X
  ORA 1,X
  BEQ tof_done2
  INC fexp
  BRA ui2tof_loop

tof_done2:
  // One more shift right
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1
  PLX
  JMP __fassemble


__i4tof:
  PHA
  LDA 0,X       // Check for zero
  ORA 1,X
  ORA 2,X
  ORA 3,X
  BNE i4tof_nonzero
  JMP __fzero
i4tof_nonzero:

  // Not zero, check for negative.
  STZ fsign
  LDA 3,X       // Top byte of source
  BPL pos_i4tof
  LDA #0x80
  STA fsign      // Sign bit set.
  SEC
  LDA #0        // Negate int value.
  SBC 0,X
  STA 0,X
  LDA #0
  SBC 1,X
  STA 1,X
  LDA #0
  SBC 2,X
  STA 2,X
  LDA #0
  SBC 3,X
  STA 3,X
pos_i4tof:
  LDA #127        // Set exponent to bias value.
  STA fexp
  JSR __fzero_mantissa
  CLC
i4tof_loop:
  // C contains the bottom bit of src.
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1

  // Shift int with low bit in C.
  LSR 3,X
  ROR 2,X
  ROR 1,X
  ROR 0,X

  // All bits shifted out?
  LDA 0,X
  ORA 1,X
  ORA 2,X
  ORA 3,X
  BEQ tof_done3
  INC fexp
  BRA i4tof_loop

__ui4tof:
  PHA
  LDA 0,X       // Check for zero
  ORA 1,X
  ORA 2,X
  ORA 3,X
  BNE ui4tof_nonzero
  JMP __fzero
ui4tof_nonzero:

  // Not zero, check for negative.
  STZ fsign
  LDA #127        // Set exponent to bias value.
  STA fexp
  JSR __fzero_mantissa
  CLC
ui4tof_loop:
  // C contains the bottom bit of src.
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1

  // Shift int with low bit in C.
  LSR 3,X
  ROR 2,X
  ROR 1,X
  ROR 0,X

  // All bits shifted out?
  LDA 0,X
  ORA 1,X
  ORA 2,X
  ORA 3,X
  BEQ tof_done3
  INC fexp
  BRA ui4tof_loop

tof_done3:
  // One more shift right
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1
  PLX
  JMP __fassemble

__i8tof:
  PHA
  LDA 0,X       // Check for zero
  ORA 1,X
  ORA 2,X
  ORA 3,X
  ORA 4,X
  ORA 5,X
  ORA 6,X
  ORA 7,X
  BNE i8tof_nonzero
  JMP __fzero
i8tof_nonzero:

  // Not zero, check for negative.
  STZ fsign
  LDA 3,X       // Top byte of source
  BPL pos_i8tof
  LDA #0x80
  STA fsign      // Sign bit set.
  SEC
  LDA #0        // Negate int value.
  SBC 0,X
  STA 0,X
  LDA #0
  SBC 1,X
  STA 1,X
  LDA #0
  SBC 2,X
  STA 2,X
  LDA #0
  SBC 3,X
  STA 3,X
  LDA #0
  SBC 4,X
  STA 4,X
  LDA #0
  SBC 5,X
  STA 5,X
  LDA #0
  SBC 6,X
  STA 6,X
  LDA #0
  SBC 7,X
  STA 7,X
pos_i8tof:
  LDA #127        // Set exponent to bias value.
  STA fexp
  JSR __fzero_mantissa
  CLC
i8tof_loop:
  // C contains the bottom bit of src.
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1

  // Shift int with low bit in C.
  LSR 7,X
  ROR 6,X
  ROR 5,X
  ROR 4,X
  ROR 3,X
  ROR 2,X
  ROR 1,X
  ROR 0,X

  // All bits shifted out?
  LDA 0,X
  ORA 1,X
  ORA 2,X
  ORA 3,X
  ORA 4,X
  ORA 5,X
  ORA 6,X
  ORA 7,X
  BEQ tof_done4
  INC fexp
  BRA i8tof_loop

__ui8tof:
  PHA
  LDA 0,X       // Check for zero
  ORA 1,X
  ORA 2,X
  ORA 3,X
  ORA 4,X
  ORA 5,X
  ORA 6,X
  ORA 7,X
  BNE ui8tof_nonzero
  JMP __fzero
ui8tof_nonzero:

  // Not zero, check for negative.
  STZ fsign
  LDA #127        // Set exponent to bias value.
  STA fexp
  JSR __fzero_mantissa
  CLC
ui8tof_loop:
  // C contains the bottom bit of src.
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1

  // Shift int with low bit in C.
  LSR 7,X
  ROR 6,X
  ROR 5,X
  ROR 4,X
  ROR 3,X
  ROR 2,X
  ROR 1,X
  ROR 0,X

  // All bits shifted out?
  LDA 0,X
  ORA 1,X
  ORA 2,X
  ORA 3,X
  ORA 4,X
  ORA 5,X
  ORA 6,X
  ORA 7,X
  BEQ tof_done4
  INC fexp
  BRA ui8tof_loop

tof_done4:
  // One more shift right
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1
  PLX
  JMP __fassemble

// Convert float to 16-bit int.
// Entry:
// A: offset into zero page for int reg output
// X: offset into zero page for float input
__ftoi2:
__ftoui2:
  PHA
  LDA 0,X
  ORA 1,X
  ORA 2,X
  ORA 3,X
  BEQ ftoi2_zero  // Input is zero, result is zero.

  LDA 3,X         // Load exponent and sign.
  STA fexp
  AND #0x80
  STA fsign        // Sign bit.

  // Load top byte of mantissa and bottom bit of exponent.
  LDA 2,X
  ASL A
  ROL fexp

  // If exponent is less than 127 (0 without bias), we have zero output.
  SEC
  LDA fexp
  SBC #127
  BCC ftoi2_zero
  ADC #0        // Carry is set to this adds 1
  STA fexp       // Exponent without bias + 1.  2^0 is 1

  // Put result in mt1,
  STZ mt1+0
  STZ mt1+1

  // Copy 23 bit mantissa plus implicit 1 to fmanA.
  LDA 0,X
  STA fmanA+0
  LDA 1,X
  STA fmanA+1
  LDA 2,X
  // Put implicit 1 bit in to mantissa.
  AND #0x7f
  ORA #0x80
  STA fmanA+2
  
ftoi2_loop:
  ASL fmanA+0
  ROL fmanA+1
  ROL fmanA+2
  ROL mt1+0
  ROL mt1+1
  DEC fexp
  BNE ftoi2_loop
  LDA fsign
  BEQ ftoi2_pos

  // Negative, negate mt1
  SEC
  LDA #0
  SBC mt1+0
  STA mt1+0
  LDA #0
  SBC mt1+1
  STA mt1+1
  
ftoi2_pos:
  PLX
  LDA mt1+0
  STA 0,X
  LDA mt1+2
  STA 1,X
  RTS

ftoi2_zero:
  PLX
  LDA #0
  STA 0,X
  STA 1,X
  RTS

// Convert float to 8-bit int.
// Entry:
// A: offset into zero page for int reg output
// X: offset into zero page for float input
__ftoi1:
__ftoui1:
  PHA
  LDA 0,X
  ORA 1,X
  ORA 2,X
  ORA 3,X
  BEQ ftoi1_zero  // Input is zero, result is zero.

  LDA 3,X         // Load exponent and sign.
  STA fexp
  AND #0x80
  STA fsign        // Sign bit.

  // Load top byte of mantissa and bottom bit of exponent.
  LDA 2,X
  ASL A
  ROL fexp

  // If exponent is less than 127 (0 without bias), we have zero output.
  SEC
  LDA fexp
  SBC #127
  BCC ftoi1_zero
  ADC #0        // Carry is set to this adds 1
  STA fexp       // Exponent without bias + 1.  2^0 is 1

  // Put result in mt1,
  STZ mt1+0

  // Copy 23 bit mantissa plus implicit 1 to fmanA.
  LDA 0,X
  STA fmanA+0
  LDA 1,X
  STA fmanA+1
  LDA 2,X
  // Put implicit 1 bit in to mantissa.
  AND #0x7f
  ORA #0x80
  STA fmanA+2
  
ftoi1_loop:
  ASL fmanA+0
  ROL fmanA+1
  ROL fmanA+2
  ROL mt1+0
  DEC fexp
  BNE ftoi1_loop
  LDA fsign
  BEQ ftoi1_pos

  // Negative, negate mt1
  SEC
  LDA #0
  SBC mt1+0
  STA mt1+0
  
ftoi1_pos:
  PLX
  LDA mt1+0
  STA 0,X
  RTS

ftoi1_zero:
  PLX
  LDA #0
  STA 0,X
  RTS


// Convert float to 32-bit int.
// Entry:
// A: offset into zero page for int reg output
// X: offset into zero page for float input
__ftoi4:
__ftoui4:
  PHA
  LDA 0,X
  ORA 1,X
  ORA 2,X
  ORA 3,X
  BEQ ftoi4_zero  // Input is zero, result is zero.

  LDA 3,X         // Load exponent and sign.
  STA fexp
  AND #0x80
  STA fsign        // Sign bit.

  // Load top byte of mantissa and bottom bit of exponent.
  LDA 2,X
  ASL A
  ROL fexp

  // If exponent is less than 127 (0 without bias), we have zero output.
  SEC
  LDA fexp
  SBC #127
  BCC ftoi4_zero
  ADC #0        // Carry is set to this adds 1
  STA fexp       // Exponent without bias + 1.  2^0 is 1

  // Put result in mt1,
  STZ mt1+0
  STZ mt1+1
  STZ mt1+2
  STZ mt1+3

  // Copy 23 bit mantissa plus implicit 1 to fmanA.
  LDA 0,X
  STA fmanA+0
  LDA 1,X
  STA fmanA+1
  LDA 2,X
  // Put implicit 1 bit in to mantissa.
  AND #0x7f
  ORA #0x80
  STA fmanA+2
  
ftoi4_loop:
  ASL fmanA+0
  ROL fmanA+1
  ROL fmanA+2
  ROL mt1+0
  ROL mt1+1
  ROL mt1+2
  ROL mt1+3
  DEC fexp
  BNE ftoi4_loop
  LDA fsign
  BEQ ftoi4_pos

  // Negative, negate mt1
  SEC
  LDA #0
  SBC mt1+0
  STA mt1+0
  LDA #0
  SBC mt1+1
  STA mt1+1
  LDA #0
  SBC mt1+2
  STA mt1+2
  LDA #0
  SBC mt1+3
  STA mt1+3

ftoi4_pos:
  PLX
  LDY #0
ftoi4_loop2:
  LDA mt1,Y
  STA 0,X
  INX
  INY
  CPY #4
  BNE ftoi4_loop2
  RTS

ftoi4_zero:
  PLX
  LDA #0
  STA 0,X
  STA 1,X
  STA 2,X
  STA 3,X
  RTS

ftoi8_zero:
  PLX
  LDA #0
  STA 0,X
  STA 1,X
  STA 2,X
  STA 3,X
  STA 4,X
  STA 5,X
  STA 6,X
  STA 7,X
  RTS

// Convert float to 64-bit int.
// Entry:
// A: offset into zero page for int reg output
// X: offset into zero page for float input
__ftoi8:
__ftoui8:
  PHA
  LDA 0,X
  ORA 1,X
  ORA 2,X
  ORA 3,X
  BEQ ftoi8_zero  // Input is zero, result is zero.

  LDA 3,X         // Load exponent and sign.
  STA fexp
  AND #0x80
  STA fsign        // Sign bit.

  // Load top byte of mantissa and bottom bit of exponent.
  LDA 2,X
  ASL A
  ROL fexp

  // If exponent is less than 127 (0 without bias), we have zero output.
  SEC
  LDA fexp
  SBC #127
  BCC ftoi8_zero
  ADC #0        // Carry is set to this adds 1
  STA fexp       // Exponent without bias + 1.  2^0 is 1

  // Put result in mt1,
  STZ mt1+0
  STZ mt1+1
  STZ mt1+2
  STZ mt1+3

  // Copy 23 bit mantissa plus implicit 1 to fmanA.
  LDA 0,X
  STA fmanA+0
  LDA 1,X
  STA fmanA+1
  LDA 2,X
  // Put implicit 1 bit in to mantissa.
  AND #0x7f
  ORA #0x80
  STA fmanA+2
  
ftoi8_loop:
  ASL fmanA+0
  ROL fmanA+1
  ROL fmanA+2
  ROL mt1+0
  ROL mt1+1
  ROL mt1+2
  ROL mt1+3
  ROL mt1+4
  ROL mt1+5
  ROL mt1+6
  ROL mt1+7
  DEC fexp
  BNE ftoi8_loop
  LDA fsign
  BEQ ftoi8_pos

  // Negative, negate mt1
  SEC
  LDA #0
  SBC mt1+0
  STA mt1+0
  LDA #0
  SBC mt1+1
  STA mt1+1
  LDA #0
  SBC mt1+2
  STA mt1+2
  LDA #0
  SBC mt1+3
  STA mt1+3
  LDA #0
  SBC mt1+4
  STA mt1+4
  LDA #0
  SBC mt1+5
  STA mt1+5
  LDA #0
  SBC mt1+6
  STA mt1+6
  LDA #0
  SBC mt1+7
  STA mt1+7

ftoi8_pos:
  PLX
  LDY #0
ftoi8_loop2:
  LDA mt1,Y
  STA 0,X
  INX
  INY
  CPY #8
  BNE ftoi8_loop2
  RTS


// This is called from a C function.  The args on the stack are:
// sp+0: a
// sp+2: b
// Calculates a += b
__add128:
   // Load a into t0,t1
  LDA (__sp)
  STA __t0
  LDY #1
  LDA (__sp),Y
  STA __t1
  INY

  // Load b into t2,t3
  LDA (__sp),Y
  STA __t2
  INY
  LDA (__sp),Y
  STA __t3

  LDY #0
  LDX #16
  CLC
add128_loop:
  LDA (__t0),Y
  ADC (__t2),Y
  STA (__t0),Y
  INY
  DEX
  BNE add128_loop
  RTS

// This is called from a C function.  The args on the stack are:
// sp+0: a
// sp+2: b
// Calculates a += b
__add256:
   // Load a into t0,t1
  LDA (__sp)
  STA __t0
  LDY #1
  LDA (__sp),Y
  STA __t1
  INY

  // Load b into t2,t3
  LDA (__sp),Y
  STA __t2
  INY
  LDA (__sp),Y
  STA __t3

  LDY #0
  LDX #32
  CLC
add256_loop:
  LDA (__t0),Y
  ADC (__t2),Y
  STA (__t0),Y
  INY
  DEX
  BNE add256_loop
  RTS
// This is called from a C function.  The args on the stack are:
// sp+0: a
// Calculates a <<= 1
.set t mt1

__lshift128:
  // Load a into t0,t1
  LDA (__sp)
  STA __t0
  LDY #1
  LDA (__sp),Y
  STA __t1

  // Shifts and rotations don't work with (zp),Y addressing mode.
  LDY #15
lshift128_loop:
  LDA (__t0),Y
  STA t,Y
  DEY
  BPL lshift128_loop
  
  ASL t+0
  LDY #15
  LDX #1
lshift128_loop2:
  ROL t,X
  INX
  DEY
  BNE lshift128_loop2

  // Copy back out.
  LDY #15
lshift128_loop3:
  LDA t,Y
  STA (__t0),Y
  DEY
  BPL lshift128_loop3
  RTS

// This is called from a C function.  The args on the stack are:
// sp+0: a
// Calculates a <<= 1
__lshift256:
  // Load a into t0,t1
  LDA (__sp)
  STA __t0
  LDY #1
  LDA (__sp),Y
  STA __t1

  // Shifts and rotations don't work with (zp),Y addressing mode.
  LDY #31
lshift256_loop:
  LDA (__t0),Y
  STA t,Y
  DEY
  BPL lshift256_loop
  
  ASL t+0
  LDY #31
  LDX #1
lshift256_loop2:
  ROL t,X
  INX
  DEY
  BNE lshift256_loop2

  LDY #31
lshift256_loop3:
  LDA t,Y
  STA (__t0),Y
  DEY
  BPL lshift256_loop3

  RTS

// This is called from a C function.  The args on the stack are:
// sp+0: a
// Calculates a >>= 1
__rshift256:
  // Load a into t0,t1
  LDA (__sp)
  STA __t0
  LDY #1
  LDA (__sp),Y
  STA __t1

  // Shifts and rotations don't work with (zp),Y addressing mode.
  LDY #31
rshift256_loop:
  LDA (__t0),Y
  STA t,Y
  DEY
  BPL rshift256_loop
  
  LSR t+31
  LDY #31
  LDX #30
rshift256_loop2:
  ROR t,X
  DEX
  DEY
  BNE rshift256_loop2

  LDY #31
rshift256_loop3:
  LDA t,Y
  STA (__t0),Y
  DEY
  BPL rshift256_loop3
  RTS

// Entry:
// sp+0: a
// Calculates ++a
__inc128:
  // Load a into t0,t1
  LDA (__sp)
  STA __t0
  LDY #1
  LDA (__sp),Y
  STA __t1

  LDY #0
  LDX #15
  SEC               // Carry set to first iteration will add 1
inc128_loop:
  LDA (__t0),Y
  ADC #0
  STA (__t0),Y
  INY
  DEX
  BPL inc128_loop
  RTS


// Entry:
// sp+0: a
// Calculates ++a
__inc256:
  // Load a into t0,t1
  LDA (__sp)
  STA __t0
  LDY #1
  LDA (__sp),Y
  STA __t1

  LDY #0
  LDX #15
  SEC               // Carry set to first iteration will add 1
inc256_loop:
  LDA (__t0),Y
  ADC #0
  STA (__t0),Y
  INY
  DEX
  BPL inc256_loop
  RTS


