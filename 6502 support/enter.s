#include "vars.s

.text

// Regular enter and leave with register save mask.
.global __enter
.global __leave
.global __leave_void
.global __enter_leaf
.global __leave_leaf
.global __leave_leaf_void

// Enter and leave with no save mask
.global __enter_nomask
.global __leave_nomask
.global __leave_void_nomask
.global __enter_leaf_nomask
.global __leave_leaf_nomask
.global __leave_leaf_void_nomask

.global __result1
.global __result2
.global __result4
.global __result8
.global __load_result
.global __save_regs
.global __save_reg_set
.global __save_from_mask
.global __restore_regs
.global __restore_reg_set

// Register save mask
// This is a 24-bit bitmask immediately after the __enter and __enter_leaf
// calls.  Consists of a number of registers to save for each of the
// register types in this order:
// i regs (12)   - 5 bits
// b regs (8)    - 4 bits
// l regs (8)    - 4 bits
// x regs (4)    - 3 bits
// f regs (4)    - 3 bits
//    total      =========
//                 19 bits
//
//     3       3     4      5      4
// 19 18 17 16 15 14 13 12 11 10  9 8 7 6 5 4 3  2  1  0
//  +----------+---------+---------+-----+----------+
//  |    f     |   x     |    l    |  b  |    i     |
//  +----------+---------+---------+-----+----------+
//
// If the save mask is 0 it is absent from the instructions and the *_nomask
// variants of the enter and leave subroutines are used.

// Entry:
// Y: start offset for first reg to save
// __t3: number of bytes to save
// Saves X
__save_reg_set:
  // Make space on the stack.
  PHX
  SEC
  LDA __sp
  SBC __t3
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1

  // Copy from top down.
  DEC __t3
  TYA
  LDY __t3
  CLC
  ADC __t3
  TAX
save_reg_loop:
  LDA 0,X
  STA (__sp),Y
  DEX
  DEY
  BPL save_reg_loop
  PLX
  RTS

// Entry:
// Y: start offset for first reg to restore
// __t3: number of bytes to restore
// Saves X
__restore_reg_set:
  PHX

  // Copy from top down.
  DEC __t3
  TYA
  LDY __t3
  CLC
  ADC __t3
  TAX

  // If we don't have a result, don't check for non-restore of result
  // in loop critical path.
  LDA __result+1
  BEQ restore_reg_void_loop

restore_reg_loop:
  // If X is in range [__result, __result1) we don't restore reg.
  CPX __result+1
  BCS restore_ok      // >= result range hi?
  CPX __result
  BCS restore1    // < result range lo?
restore_ok:
  LDA (__sp),Y
  STA 0,X
restore1:
  DEX
  DEY
  BPL restore_reg_loop
  BMI restore_reg_done

// Restore a reg for a void function.  No check for non-restore.
restore_reg_void_loop:
  LDA (__sp),Y
  STA 0,X
  DEX
  DEY
  BPL restore_reg_void_loop

restore_reg_done:
  PLX

  // Remove space from stack.  __t3 has been decremented by 1 so we set the carry
  SEC
  LDA __sp
  ADC __t3
  STA __sp
  LDA __sp+1
  ADC #0
  STA __sp+1
  RTS

// Sizes of each register set in bits, from LSB to MSB
reg_mask_sizes:
  .byte 5,4,4,3,3

// Mask to AND with to get number of regs to save.
reg_mask_masks:
  .byte 31,15,15,7,7

// Start offset of registers in zero page.
// First n for each reg is not saved:
// i: 4
// b: 2
// l: 2
// x: 1
// f: 1
reg_mask_offsets:
  .byte __i0+8, __b0+2, __l0+8, __x0+8, __f0+4

// Log2 of size of each register (left shift count)
reg_mask_reg_sizes:
  .byte 1, 0, 2, 3, 2


// __t0,t1: current save mask.
// X: reg type (0 = b,...)
// Exit:
// __t0, __t1, __t2 shifted right to remove save count.
__save_from_mask:
  LDA reg_mask_masks,X
  AND __t0            // A = number of regs to save
  BEQ skip_save_reg
  STA __t3
  LDY reg_mask_reg_sizes,X
  BEQ skip_reg_mult
// Shift left by log2 of register size to get number of bytes to save.
reg_mult_loop:
  ASL __t3
  DEY
  BNE reg_mult_loop
skip_reg_mult:        # __t2 contains number of bytes
  LDA __t3
  LDY reg_mask_offsets, X
  JSR __save_reg_set
skip_save_reg:
// Shift reg mask to the right by the number of bits in the mask
  LDY reg_mask_sizes,X
reg_mask_shift_loop:
  LSR __t2
  ROR __t1
  ROR __t0
  DEY
  BNE reg_mask_shift_loop
end_save_regs:
  RTS


// Entry:
// __t2, __t3: address of save mask, corrupted on exit.
__save_regs:
  LDA (__t2)
  STA __t0
  LDY #1
  LDA (__t2),Y
  STA __t1
  INY
  LDA (__t2),Y
  STA __t2
  LDX #0
save_regs_loop:
  LDA __t0
  ORA __t1
  ORA __t2
  BEQ end_save_regs
  JSR __save_from_mask
  INX
  BNE save_regs_loop

// Entry:
// __fp-3: address of save mask
// This is like save_regs except it needs to operate in reverse.
// It needs to pop the registers off the stack in reverse order.
// Saves X
__restore_regs:
  PHX
  SEC
  LDA __fp
  SBC #3          // 24 bits for save mask
  STA __t2
  LDA __fp+1
  SBC #0
  STA __t3
  LDA (__t2)
  STA __t0
  LDY #1
  LDA (__t2),Y
  STA __t1
  INY
  LDA (__t2),Y
  STA __t2
  LDX #0
restore_regs_loop:
  LDA __t0
  ORA __t1
  ORA __t2
  BEQ end_restore_regs
  // Calculate number of bytes to restore and push onto 6502 stack
  LDA reg_mask_masks,X
  AND __t0            // A = number of regs to save
  LDY reg_mask_reg_sizes,X
  BEQ skip_reg_mult1
// Shift left by log2 of register size to get number of bytes to save.
reg_mult_loop1:
  ASL A
  DEY
  BNE reg_mult_loop1
skip_reg_mult1:        // __t3 contains number of bytes
  PHA                 // Push onto 6502 stack.

  // Shift reg mask to the right by the number of bits in the mask
  LDY reg_mask_sizes,X
reg_mask_shift_loop1:
  LSR __t2
  ROR __t1
  ROR __t0
  DEY
  BNE reg_mask_shift_loop1
  INX
  BNE restore_regs_loop

end_restore_regs:
  CPX #0              // Nothing to restore?
  BEQ end_restore
  DEX                 // X is now index into reg_mask_offsets.

  // 6502 stack contains N bytes which are the number of bytes to restore
  // for each reg type.  X contains the number of bytes pushed.
restore_reg_loop2:
  LDY reg_mask_offsets, X
  PLA
  BEQ skip_restore
  STA __t3
  JSR __restore_reg_set
skip_restore:
  DEX
  BPL restore_reg_loop2
end_restore:
  PLX
  RTS

// Makes room for save mask by decrementing sp.
save_mask_space:
  SEC
  LDA __sp
  SBC #3
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  RTS


// +--------------------+
// |                    |
// |    Previous        |
// |      frame         |
// |                    |
// +--------------------+    <- previous sp, new fp
// |   save mask (24)   |
// +--------------------+    <- reg save mask @fp-3
// |                    |
// |                    |    <- variables (accessed via fp-X)
// |                    |
// |                    |
// |                    |
// +--------------------+
// |                    |
// |    spilled regs    |
// |                    |
// +--------------------+
// |   result addr      |    <- not present for leaf procs
// +--------------------+
// |   saved fp         |
// +--------------------+    <- current sp
//
// First local varialble is at fp-2-size
// Save mask is omitted if it is zero.

// X = frame_size_lo
// Y = frame_size hi

// Possibly followed by 24-bits of register save mask

__enter:
  LDY #0

// Entry point for >256 bytes on stack frame
  STX __t0
  STY __t1

  // Save old sp
  LDA __sp
  PHA
  LDA __sp+1
  PHA

  // Store save mask in frame.
  // First make room on stack by decrementing __sp by 3
  JSR save_mask_space

  // 2 extra bytes have been pushed onto the stack.
  TSX
  LDA 0x103,X       // Load LO byte
  STA __t2          // Copy to temp addr
  LDA 0x104,X       // Load HI byte
  STA __t3
  INC __t2          // JSR puts return address -1 on stack.
  BNE enter_skip
  INC __t3
enter_skip:
  // Store save mask (24 bits)
  LDA (__t2)
  STA (__sp)
  LDY #1
  LDA (__t2),Y
  STA (__sp),Y
  INY
  LDA (__t2),Y
  STA (__sp),Y

  // Decrement sp by frame size
  SEC
  LDA __sp
  SBC __t0
  STA __sp
  LDA __sp+1
  SBC __t1
  STA __sp+1

  // Store __result and __result+1 in frame.
  LDY #3
  LDA __result+1
  STA (__sp),Y
  DEY
  LDA __result
  STA (__sp),Y
  DEY

  // Store old fp.
  LDA __fp+1
  STA (__sp),Y
  LDA __fp
  STA (__sp)

  // Make new fp (old sp, prior to decrement)
  PLA
  STA __fp+1
  PLA
  STA __fp

  // Return to address after save mask (__t2 + 2)
  // The save mask is 3 bytes but we set the return address to
  // one byte less than the next instruction as the RTS will add one
  // to it before jumping to it.
enter_save_regs:
  TSX
  CLC
  LDA __t2
  ADC #2
  STA 0x101,X
  LDA __t3
  ADC #0
  STA 0x102,X

  // Save registers on the stack (__t2,__t3 contains address of save mask).
  JMP __save_regs


// Enter leaf procedure.
// Followed by 16-bit save mask.

__enter_leaf:
  LDY #0

// Entry point for >256 bytes on stack frameload
  STX __t0
  STY __t1

  // Save old sp
  LDA __sp
  PHA
  LDA __sp+1
  PHA

  // Store reg save mask.
  // First make room on stack by decrementing __sp by 3
  JSR save_mask_space

  TSX
  LDA 0x103,X       // Load LO byte
  STA __t2          // Copy to temp addr
  LDA 0x104,X       // Load HI byte
  STA __t3
  INC __t2          // JSR puts return address -1 on stack.
  BNE enter_leaf_skip
  INC __t3
enter_leaf_skip:
  // Store save mask
  LDA (__t2)
  STA (__sp)
  LDY #1
  LDA (__t2),Y
  STA (__sp),Y
  INY
  LDA (__t2),Y
  STA (__sp),Y

  // Decrement sp by frame size.
  SEC
  LDA __sp
  SBC __t0
  STA __sp
  LDA __sp+1
  SBC __t1
  STA __sp+1

  LDY #1
  // Store old fp.
  LDA __fp+1
  STA (__sp),Y
  LDA __fp
  STA (__sp)

  // Make new fp (old sp, prior to decrement)
  PLA
  STA __fp+1
  PLA
  STA __fp
  BRA enter_save_regs

// Enter with no savemask.
__enter_nomask:
  LDY #0

// Entry point for >256 bytes on stack frame
  STX __t0
  STY __t1

  // Save old sp
  LDA __sp
  PHA
  LDA __sp+1
  PHA

  // We still need space for the save mask in the frame, even though
  // it won't be populated.
  JSR save_mask_space

  // Decrement sp by frame size
  SEC
  LDA __sp
  SBC __t0
  STA __sp
  LDA __sp+1
  SBC __t1
  STA __sp+1

  // Store __result and __result+1 in frame.
  LDY #3
  LDA __result+1
  STA (__sp),Y
  DEY
  LDA __result
  STA (__sp),Y
  DEY

  // Store old fp.
  LDA __fp+1
  STA (__sp),Y
  LDA __fp
  STA (__sp)

  // Make new fp (old sp, prior to decrement)
  PLA
  STA __fp+1
  PLA
  STA __fp
  RTS

// Enter leaf with no mask.
__enter_leaf_nomask:
  LDY #0

// Entry point for >256 bytes on stack frameload
  STX __t0
  STY __t1

  // Save old sp
  LDA __sp
  PHA
  LDA __sp+1
  PHA

  // We still need space for the save mask in the frame, even though
  // it won't be populated.
  JSR save_mask_space

  // Decrement sp by frame size.
  SEC
  LDA __sp
  SBC __t0
  STA __sp
  LDA __sp+1
  SBC __t1
  STA __sp+1

  LDY #1
  // Store old fp.
  LDA __fp+1
  STA (__sp),Y
  LDA __fp
  STA (__sp)

  // Make new fp (old sp, prior to decrement)
  PLA
  STA __fp+1
  PLA
  STA __fp
  RTS


// No result, zero out result range for restore_regs.
__leave_void:
  LDX #0

  // Entry point for >256 bytes on stack frame
  STZ __result
  STZ __result+1
  BRA leave_small

__leave:
  LDX #0

  // Entry point for >256 bytes on stack frame
leave_small:
  PHY
  JSR __restore_regs

  // Load old fp.
  LDA (__sp)
  STA __fp
  LDY #1
  LDA (__sp),Y    // Y = 1
  STA __fp+1
  INY

  // Load result
  LDA (__sp),Y    // Y = 2
  STA __result
  INY
  LDA (__sp), Y    // Y = 3
  STA __result+1
  
  // Increment sp
  CLC
  PLA
  ADC __sp
  STA __sp
  TXA
  ADC __sp+1
  STA __sp+1
  RTS

// No mask.
// No result, zero out result range for restore_regs.
__leave_void_nomask:
  LDX #0

  // Entry point for >256 bytes on stack frame
  STZ __result
  STZ __result+1
  BRA leave_nomask_small

__leave_nomask:
  LDX #0

  // Entry point for >256 bytes on stack frame
leave_nomask_small:
  PHY

  // Load old fp.
  LDA (__sp)
  STA __fp
  LDY #1
  LDA (__sp),Y    // Y = 1
  STA __fp+1
  INY

  // Load result
  LDA (__sp),Y    // Y = 2
  STA __result
  INY
  LDA (__sp), Y    // Y = 3
  STA __result+1

leave_reload:
  // Increment sp
  CLC
  PLA
  ADC __sp
  STA __sp
  TXA
  ADC __sp+1
  STA __sp+1
  RTS
'
__leave_leaf_void:
  LDX #0

  // Entry point for >256 bytes on stack frame
  STZ __result
  STZ __result+1
  BRA leave_leaf_small

__leave_leaf:
  LDX #0

leave_leaf_small:
  // Entry point for >256 bytes on stack frame
  PHY
  JSR __restore_regs

leave_leaf_common:
  // Load old fp.
  LDA (__sp)
  STA __fp
  LDY #1
  LDA (__sp),Y
  STA __fp+1
  BRA leave_reload

// No mask
// Leave leaf proc
__leave_leaf_void_nomask:
  LDX #0

  // Entry point for >256 bytes on stack frame
  STZ __result
  STZ __result+1
  BRA leave_leaf_nomask_small

__leave_leaf_nomask:
  LDX #0

leave_leaf_nomask_small:
  // Entry point for >256 bytes on stack frame
  PHY
  BRA leave_leaf_common
 

// Load the __result from the stack frame.
// Entry:
// X,Y: offset from fp to location of result addr
// which is frame_size + 3 - 2 = frame_size + 1
// +--------------------+    <- previous sp, new fp
// |   save mask (24)   |
// +--------------------+    <- reg save mask @fp-3
// |                    |
// |                    |    <- variables (accessed via fp-X)
// |                    |
// |                    |
// |                    |
// +--------------------+
// |                    |
// |    spilled regs    |
// |                    |
// +--------------------+   <- fp - (X,Y)
// |   result addr      |
// +--------------------+
// |   saved fp         |
// +--------------------+
// |                    |
// |  saved regs        |
// |                    |
// +--------------------+

__load_result:
  LDY #0
  STX __t0
  STY __t1
  SEC
  LDA __fp
  SBC __t0
  STA __t0
  LDA __fp+1
  SBC __t1
  STA __t1
  LDA (__t0)
  STA __result
  LDY #1
  LDA (__t0),Y
  STA __result+1
  RTS

// If the result is going into a zero page location we have to tell the
// restore_regs subroutine not to restore that location.
// We put the start address in __result and the end address in __result+1
// Enter:
// Y: size of result
// Exit:
// __result: zero page offset of start of result range
// __result+1: zero page offset at end of result range.
set_result_range:
  LDA __result+1
  BNE no_result_range
  TYA
  CLC
  ADC __result
  STA __result+1
  RTS

no_result_range:
  LDA __result      // Zero result range.
  STA __result+1
  RTS

// A: zero page offset for 2-byte result
// __result: address for result.
__result1:
  TAX
  LDA 0,X
  STA (__result)
  LDA 1,X
  LDY #1
  JMP set_result_range

// A: zero page offset for 2-byte result
// __result: address for result.
__result2:
  TAX
  LDA 0,X
  STA (__result)
  LDA 1,X
  LDY #1
  STA (__result), Y
  INY
  JMP set_result_range

// A: zero page offset for 4-byte result
// __result: address for result.
__result4:
  TAX
  LDY #0
__result4loop:
  LDA 0,X
  STA (__result), Y
  INY
  INX
  CPY #4
  BNE __result4loop
  JMP set_result_range

  
// A: zero page offset for 8-byte result
// __result: address for result.
__result8:
  TAX
  LDY #0
__result8loop:
  LDA 0, X
  STA (__result), Y
  INY
  INX
  CPY #8
  BNE __result8loop
  JMP set_result_range


