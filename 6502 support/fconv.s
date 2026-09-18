#include "vars.s"
#include "fp.s"

// Functions are emitted in per-symbol ELF sections.

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

.section ".text.__i1tof", "ax", @progbits
__i1tof:
  PHA
  LDA 0,X       // Check for zero
  STA mt1+0
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
  SBC mt1+0
  STA mt1+0
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
  LSR mt1+0

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

.section ".text.__ui1tof", "ax", @progbits
__ui1tof:
  PHA
  LDA 0,X       // Check for zero
  STA mt1+0
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
  LSR mt1+0

  // All bits shifted out?
  BEQ ui1tof_done
  INC fexp
  BRA ui1tof_loop
ui1tof_done:
  // One more shift right
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1
  PLX
  JMP __fassemble

.section ".text.__i2tof", "ax", @progbits
__i2tof:
  PHA
  LDA 0,X       // Check for zero
  STA mt1+0
  LDA 1,X
  STA mt1+1
  ORA mt1+0
  BNE i2tof_nonzero
  JMP __fzero
i2tof_nonzero:

  // Not zero, check for negative.
  STZ fsign
  LDA mt1+1       // Top byte of source
  BPL pos_i2tof
  LDA #0x80
  STA fsign      // Sign bit set.
  SEC
  LDA #0        // Negate int value.
  SBC mt1+0
  STA mt1+0
  LDA #0
  SBC mt1+1
  STA mt1+1
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
  LSR mt1+1
  ROR mt1+0

  // All bits shifted out?
  LDA mt1+0
  ORA mt1+1
  BEQ i2tof_done
  INC fexp
  BRA i2tof_loop
i2tof_done:
  // One more shift right
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1
  PLX
  JMP __fassemble

.section ".text.__ui2tof", "ax", @progbits
__ui2tof:
  PHA
  LDA 0,X       // Check for zero
  STA mt1+0
  LDA 1,X
  STA mt1+1
  ORA mt1+0
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
  LSR mt1+1
  ROR mt1+0

  // All bits shifted out?
  LDA mt1+0
  ORA mt1+1
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


.section ".text.__i4tof", "ax", @progbits
__i4tof:
  PHA
  LDA 0,X       // Check for zero
  STA mt1+0
  LDA 1,X
  STA mt1+1
  LDA 2,X
  STA mt1+2
  LDA 3,X
  STA mt1+3
  ORA mt1+0
  ORA mt1+1
  ORA mt1+2
  BNE i4tof_nonzero
  JMP __fzero
i4tof_nonzero:

  // Not zero, check for negative.
  STZ fsign
  LDA mt1+3       // Top byte of source
  BPL pos_i4tof
  LDA #0x80
  STA fsign      // Sign bit set.
  SEC
  LDA #0        // Negate int value.
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
  LSR mt1+3
  ROR mt1+2
  ROR mt1+1
  ROR mt1+0

  // All bits shifted out?
  LDA mt1+0
  ORA mt1+1
  ORA mt1+2
  ORA mt1+3
  BEQ i4tof_done
  INC fexp
  BRA i4tof_loop
i4tof_done:
  // One more shift right
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1
  PLX
  JMP __fassemble

.section ".text.__ui4tof", "ax", @progbits
__ui4tof:
  PHA
  LDA 0,X       // Check for zero
  STA mt1+0
  LDA 1,X
  STA mt1+1
  LDA 2,X
  STA mt1+2
  LDA 3,X
  STA mt1+3
  ORA mt1+0
  ORA mt1+1
  ORA mt1+2
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
  LSR mt1+3
  ROR mt1+2
  ROR mt1+1
  ROR mt1+0

  // All bits shifted out?
  LDA mt1+0
  ORA mt1+1
  ORA mt1+2
  ORA mt1+3
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

.section ".text.__i8tof", "ax", @progbits
__i8tof:
  PHA
  LDA 0,X       // Check for zero
  STA mt1+0
  LDA 1,X
  STA mt1+1
  LDA 2,X
  STA mt1+2
  LDA 3,X
  STA mt1+3
  LDA 4,X
  STA mt1+4
  LDA 5,X
  STA mt1+5
  LDA 6,X
  STA mt1+6
  LDA 7,X
  STA mt1+7
  ORA mt1+0
  ORA mt1+1
  ORA mt1+2
  ORA mt1+3
  ORA mt1+4
  ORA mt1+5
  ORA mt1+6
  BNE i8tof_nonzero
  JMP __fzero
i8tof_nonzero:

  // Not zero, check for negative.
  STZ fsign
  LDA mt1+6       // Top byte of source
  BPL pos_i8tof
  LDA #0x80
  STA fsign      // Sign bit set.
  SEC
  LDA #0        // Negate int value.
  SBC mt1+1
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
  LSR mt1+7
  ROR mt1+6
  ROR mt1+5
  ROR mt1+4
  ROR mt1+3
  ROR mt1+2
  ROR mt1+1
  ROR mt1+0

  // All bits shifted out?
  LDA mt1+0
  ORA mt1+1
  ORA mt1+2
  ORA mt1+3
  ORA mt1+4
  ORA mt1+5
  ORA mt1+6
  ORA mt1+7
  BEQ i8tof_done
  INC fexp
  BRA i8tof_loop
i8tof_done:
  // One more shift right
  ROR fmantissa+3
  ROR fmantissa+2
  ROR fmantissa+1
  PLX
  JMP __fassemble

.section ".text.__ui8tof", "ax", @progbits
__ui8tof:
  PHA
  LDA 0,X       // Check for zero  LDA 0,X       // Check for zero
  STA mt1+0
  LDA 1,X
  STA mt1+1
  LDA 2,X
  STA mt1+2
  LDA 3,X
  STA mt1+3
  LDA 4,X
  STA mt1+4
  LDA 5,X
  STA mt1+5
  LDA 6,X
  STA mt1+6
  LDA 7,X
  STA mt1+7
  ORA mt1+0
  ORA mt1+1
  ORA mt1+2
  ORA mt1+3
  ORA mt1+4
  ORA mt1+5
  ORA mt1+6
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
  LSR mt1+7
  ROR mt1+6
  ROR mt1+5
  ROR mt1+4
  ROR mt1+3
  ROR mt1+2
  ROR mt1+1
  ROR mt1+0

  // All bits shifted out?
  LDA mt1+0
  ORA mt1+1
  ORA mt1+2
  ORA mt1+3
  ORA mt1+4
  ORA mt1+5
  ORA mt1+6
  ORA mt1+7
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
.section ".text.__ftoi2", "ax", @progbits
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
  PLA
  TAX
  LDA mt1+0
  STA 0,X
  LDA mt1+1
  STA 1,X
  RTS

ftoi2_zero:
  PLA
  TAX
  LDA #0
  STA 0,X
  STA 1,X
  RTS

// Convert float to 8-bit int.
// Entry:
// A: offset into zero page for int reg output
// X: offset into zero page for float input
.section ".text.__ftoi1", "ax", @progbits
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
  PLA
  TAX
  LDA mt1+0
  STA 0,X
  RTS

ftoi1_zero:
  PLA
  TAX
  LDA #0
  STA 0,X
  RTS


// Convert float to 32-bit int.
// Entry:
// A: offset into zero page for int reg output
// X: offset into zero page for float input
.section ".text.__ftoi4", "ax", @progbits
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
  PLA
  TAX
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
  PLA
  TAX
  LDA #0
  STA 0,X
  STA 1,X
  STA 2,X
  STA 3,X
  RTS

ftoi8_zero:
  PLA
  TAX
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
.section ".text.__ftoi8", "ax", @progbits
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
  PLA
  TAX
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
.section ".text.__add128", "ax", @progbits
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
.section ".text.__add256", "ax", @progbits
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

.section ".text.__lshift128", "ax", @progbits
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
.section ".text.__lshift256", "ax", @progbits
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
.section ".text.__rshift256", "ax", @progbits
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
.section ".text.__inc128", "ax", @progbits
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
.section ".text.__inc256", "ax", @progbits
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


