.text

.global start
.global syscall
.global __enter
.global __leave
.global __enter_leaf
.global __leave_leaf
.global __var_addr
.global __var_addrb
.global __arg_addr
.global __arg_addrb
.global __var_addr_xy
.global __var_addrb_xy
.global __arg_addr_xy
.global __arg_addrb_xy
.global __push_var1
.global __push_var1b
.global __push_var2
.global __push_var2b
.global __push_var4
.global __push_var4b
.global __push_var8
.global __push_var8b
.global __push_arg1
.global __push_arg1b
.global __push_arg2
.global __push_arg2b
.global __push_arg4
.global __push_arg4b
.global __push_arg8
.global __push_arg8b
.global __var_value1
.global __var_value1b
.global __var_value2
.global __var_value2b
.global __var_value4
.global __var_value4b
.global __var_value8
.global __var_value8b
.global __arg_value1
.global __arg_value1b
.global __arg_value2
.global __arg_value2b
.global __arg_value4
.global __arg_value4b
.global __arg_value8
.global __arg_value8b

.global __set_var_value1
.global __set_var_value1b
.global __set_var_value2
.global __set_var_value2b
.global __set_var_value4
.global __set_var_value4b
.global __set_var_value8
.global __set_var_value8b
.global __set_arg_value1
.global __set_arg_value1b
.global __set_arg_value2
.global __set_arg_value2b
.global __set_arg_value4
.global __set_arg_value4b
.global __set_arg_value8
.global __set_arg_value8b

.global __zero_var_value1
.global __zero_var_value1b
.global __zero_var_value2
.global __zero_var_value2b
.global __zero_var_value4
.global __zero_var_value4b
.global __zero_var_value8
.global __zero_var_value8b
.global __zero_arg_value1
.global __zero_arg_value1b
.global __zero_arg_value2
.global __zero_arg_value2b
.global __zero_arg_value4
.global __zero_arg_value4b
.global __zero_arg_value8
.global __zero_arg_value8b

.global __pusha
.global __pushxy
.global __pushxy0
.global __pushreg1
.global __pushreg2
.global __pushreg4
.global __pushreg8
.global __push4
.global __push8
.global __pulla
.global __pullxy
.global __pull4
.global __pull8
.global __incsp1
.global __incsp2
.global __pushmem1
.global __pushmem2
.global __copymem1
.global __copymem2
.global __result2
.global __result4
.global __result8
.global __load_result

.global __inc1
.global __inc21
.global __inc2b
.global __inc4
.global __inc8
.global __incf
.global __incd

.global __dec1
.global __dec21
.global __dec2b
.global __dec4
.global __dec8
.global __decf
.global __decd

// Runtime math routines:
.global __umul1
.global __umul2
.global __umul4
.global __umul8
.global __smul1
.global __smul2
.global __smul4
.global __smul8
.global __fmul
.global __dmul

.global __sdiv1
.global __sdiv2
.global __sdiv4
.global __sdiv8
.global __udiv1
.global __udiv2
.global __udiv4
.global __udiv8
.global __fdiv
.global __ddiv

.global __smod1
.global __smod2
.global __smod4
.global __smod8
.global __umod1
.global __umod2
.global __umod4
.global __umod8

.global __i1tof
.global __i2tof
.global __i4tof
.global __i8tof
.global __i1tod
.global __i2tod
.global __i4tod
.global __i8tod
 
.global __ui1tof
.global __ui2tof
.global __ui4tof
.global __ui8tof
.global __ui1tod
.global __ui2tod
.global __ui4tod
.global __ui8tod

.global __ftod
.global __dtof
 
.global __ftoi1
.global __ftoi2
.global __ftoi4
.global __ftoi8
.global __dtoi1
.global __dtoi2
.global __dtoi4
.global __dtoi8
.global __ftoui1
.global __ftoui2
.global __ftoui4
.global __ftoui8
.global __dtoui1
.global __dtoui2
.global __dtoui4
.global __dtoui8

.global __cmpeqf
.global __cmpnef
.global __cmpltf
.global __cmpgef
 
.global __cmpeqd
.global __cmpned
.global __cmpltd
.global __cmpged

.global __jump_table

.global __spill1
.global __spill2
.global __spill4
.global __spill8
.global __reload1
.global __reload2
.global __reload4
.global __reload8

.global __spill1b
.global __spill2b
.global __spill4b
.global __spill8b
.global __reload1b
.global __reload2b
.global __reload4b
.global __reload8b

// Zero page locations (must match vars.s / 6502_machine.h).
.set REG_BASE 0
.set __zpr0 (REG_BASE + 0)
.set __zpr1 (REG_BASE + 1)
.set __zpr2 (REG_BASE + 2)
.set __zpr3 (REG_BASE + 3)
.set __zpr4 (REG_BASE + 4)
.set __zpr5 (REG_BASE + 5)
.set __zpr6 (REG_BASE + 6)
.set __zpr7 (REG_BASE + 7)
.set __zpr8 (REG_BASE + 8)
.set __zpr9 (REG_BASE + 9)
.set __zpr10 (REG_BASE + 10)
.set __zpr11 (REG_BASE + 11)
.set __zpr12 (REG_BASE + 12)
.set __zpr13 (REG_BASE + 13)
.set __zpr14 (REG_BASE + 14)
.set __zpr15 (REG_BASE + 15)
.set __zpr16 (REG_BASE + 16)
.set __zpr17 (REG_BASE + 17)
.set __zpr18 (REG_BASE + 18)
.set __zpr19 (REG_BASE + 19)
.set __zpr20 (REG_BASE + 20)
.set __zpr21 (REG_BASE + 21)
.set __zpr22 (REG_BASE + 22)
.set __zpr23 (REG_BASE + 23)
.set __zpr24 (REG_BASE + 24)
.set __zpr25 (REG_BASE + 25)
.set __zpr26 (REG_BASE + 26)
.set __zpr27 (REG_BASE + 27)
.set __zpr28 (REG_BASE + 28)
.set __zpr29 (REG_BASE + 29)
.set __zpr30 (REG_BASE + 30)
.set __zpr31 (REG_BASE + 31)
.set __zpr32 (REG_BASE + 32)
.set __zpr33 (REG_BASE + 33)
.set __zpr34 (REG_BASE + 34)
.set __zpr35 (REG_BASE + 35)
.set __zpr36 (REG_BASE + 36)
.set __zpr37 (REG_BASE + 37)
.set __zpr38 (REG_BASE + 38)
.set __zpr39 (REG_BASE + 39)
.set __zpr40 (REG_BASE + 40)
.set __zpr41 (REG_BASE + 41)
.set __zpr42 (REG_BASE + 42)
.set __zpr43 (REG_BASE + 43)
.set __zpr44 (REG_BASE + 44)
.set __zpr45 (REG_BASE + 45)
.set __zpr46 (REG_BASE + 46)
.set __zpr47 (REG_BASE + 47)
.set __zpr48 (REG_BASE + 48)
.set __zpr49 (REG_BASE + 49)
.set __zpr50 (REG_BASE + 50)
.set __zpr51 (REG_BASE + 51)
.set __zpr52 (REG_BASE + 52)
.set __zpr53 (REG_BASE + 53)
.set __zpr54 (REG_BASE + 54)
.set __zpr55 (REG_BASE + 55)
.set __zpr56 (REG_BASE + 56)
.set __zpr57 (REG_BASE + 57)
.set __zpr58 (REG_BASE + 58)
.set __zpr59 (REG_BASE + 59)
.set __zpr60 (REG_BASE + 60)
.set __zpr61 (REG_BASE + 61)
.set __zpr62 (REG_BASE + 62)
.set __zpr63 (REG_BASE + 63)

// Legacy logical register aliases for hand-written runtime modules.
.set __b0 __zpr0
.set __b1 __zpr1
.set __b2 __zpr2
.set __b3 __zpr3
.set __b4 __zpr4
.set __b5 __zpr5
.set __b6 __zpr6
.set __b7 __zpr7
.set __i0 __zpr8
.set __i1 __zpr10
.set __i2 __zpr12
.set __i3 __zpr14
.set __i4 __zpr16
.set __i5 __zpr18
.set __i6 __zpr20
.set __i7 __zpr22
.set __i8 __zpr24
.set __i9 __zpr26
.set __i10 __zpr28
.set __i11 __zpr30
.set __i12 __zpr32
.set __i13 __zpr34
.set __i14 __zpr36
.set __i15 __zpr38
.set __l0 __zpr40
.set __l1 __zpr44
.set __l2 __zpr48
.set __l3 __zpr52
.set __l4 __zpr56
.set __l5 __zpr60
.set __l6 __zpr56
.set __l7 __zpr60
.set __x0 __zpr32
.set __x1 __zpr40
.set __x2 __zpr48
.set __x3 __zpr56
.set __f0 __zpr36
.set __f1 __zpr40
.set __f2 __zpr44
.set __f3 __zpr48

.set __sp (REG_BASE + 64)
.set __fp (REG_BASE + 66)
.set __result (REG_BASE + 68)
.set __t0 (REG_BASE + 70)
.set __t1 (REG_BASE + 71)
.set __t2 (REG_BASE + 72)
.set __t3 (REG_BASE + 73)
.set __mem_src (REG_BASE + 74)
.set __mem_dest (REG_BASE + 76)
.set __mem_size (REG_BASE + 78)
// Runtime syscall scratch (not part of compiler register file).
.set __t4 __mem_dest
.set __t5 __mem_dest+1

.set stack_bottom 0xc000
.set sys_exit 1
.set sys_abort 2

.set SAVE_MASK_BYTES 8
.set REG_FILE_BYTES 64

.set mt1 (REG_BASE + 80)
.set mt2 (REG_BASE + 90)
.set mt3 (REG_BASE + 96)
.set fscratch_start (REG_BASE + 102)
.set fscratch_end (REG_BASE + 126)




// Main entry point
start:
  // Set up stack pointer and frame pointer.
  LDA #stack_bottom & 0xff
  STA __sp
  STA __fp
  LDA #stack_bottom >> 8
  STA __sp+1
  STA __fp+1

  // Install the syscall handler in 0xfd00 as a JMP instruction
  LDA #0x4c         // JMP
  STA 0xfd00
  LDA #%lo(__syscall_handler)
  STA 0xfd01
  LDA #%hi(__syscall_handler)
  STA 0xfd02

  // Push argv and argc onto the stack.
  // argv is at 0x200
  LDX #0
  LDY #0x2
  JSR __pushxy

  // argc is 0
  LDX #0
  LDY #0
  JSR __pushxy

  // Invoke main
  JSR main

  // Push exit code (return value in __result) onto the runtime stack.
  LDX #__result
  JSR __pushreg2

  // Invoke exit syscall with exit code on the runtime stack.
  BRK
  .byte sys_exit

.set max_syscall_number 7
syscalls:
.hword __abort      // 0
.hword __exit       // 1
.hword __open       // 2
.hword __close      // 3
.hword __write      // 4
.hword __read       // 5
.hword __lseek      // 6
.hword __abort      // 7

// Entry with A = syscall number (byte after BRK)
// uses some scratch space
.set syscall_code mt1 // 2 bytes
.set syscall_vector mt1+2

.global __syscall_handler
__syscall_handler:
  CMP #max_syscall_number+1
  BCC syscall_ok
  LDA #0
syscall_ok:
  ASL A
  CLC
  ADC #%lo(syscalls)
  STA syscall_vector
  LDA #%hi(syscalls)
  ADC #0
  STA syscall_vector+1
  LDA (syscall_vector)
  TAX
  LDY #1
  LDA (syscall_vector), Y
  STA syscall_vector+1
  STX syscall_vector
  JMP (syscall_vector)

.set acia1_csr 0xfe00
.set acia1_data 0xfe01

.global __exit
__exit:
  LDA #sys_exit
  .byte 0xef      // Interpreter exit

// write syscall
// Entry:
// sp+0: return address
// sp+2: fd
// sp+4: buffer
// sp+6: size
.global __write
__write:
  // NOTE: only fd = 1 for now.
  // Buffer in t0/t1
  LDY #4
  LDA (__sp), Y
  STA __t0
  INY
  LDA (__sp),Y
  STA __t1
  INY

  // length in t2/t3
  LDA (__sp), Y
  STA __t2
  INY
  LDA (__sp),Y
  STA __t3

write_loop:
  LDA __t2
  ORA __t3
  BEQ write_end
  LDA (__t0)
  STA acia1_data
  
  INC __t0
  BNE wr1
  INC __t1
wr1:
  DEC __t2
  BPL wr2
  DEC __t3
wr2:
  BRA write_loop
write_end:
  LDA (__sp)
  STA __result
  LDY #1
  LDA (__sp),Y
  STA __result+1

  // Store size in result.
  LDY #6
  LDA (__sp), Y
  STA (__result)
  INY
  LDA (__sp), Y
  LDY #1
  STA (__result),Y
  RTS

// read syscall
// Entry:
// sp+0: return address
// sp+2: fd
// sp+4: buffer
// sp+6: size
.global __read
__read:
  // NOTE: only fd = 0 for now.
  // Buffer in t0/t1
  LDY #4
  LDA (__sp), Y
  STA __t0
  INY
  LDA (__sp),Y
  STA __t1
  INY

  // length in t2/t3
  LDA (__sp), Y
  STA __t2
  INY
  LDA (__sp),Y
  STA __t3

read_loop:
  LDA __t2
  ORA __t3
  BEQ read_end
rwait:
  LDA acia1_csr
  AND #1
  BEQ rwait
  LDA acia1_data
  STA (__t0)

  INC __t0
  BNE r1
  INC __t1
r1:
  DEC __t2
  BPL r2
  DEC __t3
r2:
  BRA read_loop

read_end:
  LDA (__sp)
  STA __result
  LDY #1
  LDA (__sp),Y
  STA __result+1
  // Store size in result.
  LDY #6
  LDA (__sp), Y
  STA (__result)
  INY
  LDA (__sp), Y
  LDY #1
  STA (__result),Y
  RTS

__open:
__close:
__lseek:
__abort:
  LDA #sys_abort
  .byte 0xef      // Interpreter exit
  RTS

// Register save mask: 64-bit byte mask (8 bytes) after __enter/__enter_leaf.

// Save register-file byte X onto the expression stack.
save_reg_byte:
  SEC
  LDA __sp
  SBC #1
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  LDA __zpr0,X
  LDY #0
  STA (__sp),Y
  RTS

// Restore register-file byte X from the expression stack.
restore_reg_byte:
  LDY #0
  LDA (__sp),Y
  STA __zpr0,X
  CLC
  LDA __sp
  ADC #1
  STA __sp
  LDA __sp+1
  ADC #0
  STA __sp+1
  RTS

// Entry: __t2,__t3 = address of 8-byte save mask.
.global save_regs
save_regs:
  LDX #0
  LDY #0
  LDA #1
  STA __t0
save_regs_loop:
  CPX #REG_FILE_BYTES
  BCS save_regs_done
  LDA (__t2),Y
  AND __t0
  BEQ save_regs_skip
  TXA
  PHX
  TAX
  JSR save_reg_byte
  PLX
save_regs_skip:
  ASL __t0
  BNE save_regs_next
  INY
  CPY #SAVE_MASK_BYTES
  BEQ save_regs_done
  LDA #1
  STA __t0
save_regs_next:
  INX
  BNE save_regs_loop
save_regs_done:
  RTS

// Entry: save mask at __fp - SAVE_MASK_BYTES.
.global restore_regs
restore_regs:
  PHX
  SEC
  LDA __fp
  SBC #SAVE_MASK_BYTES
  STA __t2
  LDA __fp+1
  SBC #0
  STA __t3
  LDX #REG_FILE_BYTES - 1
  LDY #SAVE_MASK_BYTES - 1
  LDA #0x80
  STA __t0
restore_regs_loop:
  CPX #0xFF
  BEQ restore_regs_done
  LDA (__t2),Y
  AND __t0
  BEQ restore_regs_skip
  TXA
  PHX
  TAX
  JSR restore_reg_byte
  PLX
restore_regs_skip:
  LSR __t0
  BNE restore_regs_next
  DEY
  BMI restore_regs_done
  LDA #0x80
  STA __t0
restore_regs_next:
  DEX
  BNE restore_regs_loop
restore_regs_done:
  PLX
  RTS

// Entry: Y = start offset, __t3 = number of bytes to save.
.global save_reg_set
save_reg_set:
  PHX
  CLC
  LDA __sp
  SBC __t3
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  DEC __t3
  TYA
  LDY __t3
  CLC
  ADC __t3
  TAX
save_reg_set_loop:
  LDA __zpr0,X
  STA (__sp),Y
  DEX
  DEY
  BPL save_reg_set_loop
  PLX
  RTS

// Entry: Y = start offset, __t3 = number of bytes to restore.
.global restore_reg_set
restore_reg_set:
  PHX
  DEC __t3
  TYA
  LDY __t3
  CLC
  ADC __t3
  TAX
restore_reg_set_loop:
  LDA (__sp),Y
  STA __zpr0,X
  DEX
  DEY
  BPL restore_reg_set_loop
  PLX
  CLC
  LDA __sp
  SBC __t3
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
// |   save mask (64)   |
// +--------------------+    <- reg save mask @fp-8
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

  // X = frame_size_lo
  //  Y = frame_size hi

// Followed by 16-bits of register save mask
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
  SEC
  LDA __sp
  SBC #SAVE_MASK_BYTES
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1

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
  // Store save mask (8 bytes)
  LDY #0
enter_mask_loop:
  LDA (__t2),Y
  STA (__sp),Y
  INY
  CPY #SAVE_MASK_BYTES
  BNE enter_mask_loop

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

  // Return to address after save mask (__t2 + 7)
enter_save_regs:
  TSX
  CLC
  LDA __t2
  ADC #7
  STA 0x101,X
  LDA __t3
  ADC #0
  STA 0x102,X

  // Save registers on the stack (__t2,__t3 contains address of save mask).
  JMP save_regs

// Enter leaf procedure.
// Followed by 16-bit save mask.
__enter_leaf:
  LDY #0

// Entry point for >256 bytes on stack frame
  STX __t0
  STY __t1

  // Save old sp
  LDA __sp
  PHA
  LDA __sp+1
  PHA

  // Store reg save mask (8 bytes).
  SEC
  LDA __sp
  SBC #SAVE_MASK_BYTES
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1

  TSX
  LDA 0x103,X       // Load LO byte
  STA __t2          // Copy to temp addr
  LDA 0x104,X       // Load HI byte
  STA __t3
  INC __t2          // JSR puts return address -1 on stack.
  BNE enter_leaf_skip
  INC __t3
enter_leaf_skip:
  LDY #0
enter_leaf_mask_loop:
  LDA (__t2),Y
  STA (__sp),Y
  INY
  CPY #SAVE_MASK_BYTES
  BNE enter_leaf_mask_loop

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
  LDA __fp
  STA (__sp),Y
  LDA __fp+1
  STA (__sp)

  // Make new fp (old sp, prior to decrement)
  PLA
  STA __fp+1
  PLA
  STA __fp
  BRA enter_save_regs

__leave:
  LDX #0

// Entry point for >256 bytes on stack frame
  PHY

  JSR restore_regs

  // Load old fp.
  LDA (__sp)
  STA __fp
  LDY #1
  LDA (__sp),Y
  STA __fp+1
  INY

  // Load result
  LDA (__sp),Y
  STA __result+1
  INY
  LDA (__sp), Y
  STA __result
  
  // Increment sp
  CLC
  PLA
  ADC __sp
  STA __sp
  TXA
  ADC __sp+1
  STA __sp+1
  RTS

// Leave leaf proc
__leave_leaf:
  LDX #0

// Entry point for >256 bytes on stack frame
  PHY
  JSR restore_regs

  // Load old fp.
  LDA (__sp)
  STA __fp
  LDY #1
  LDA (__sp),Y
  STA __fp+1
  INY

  // Increment sp
  CLC
  PLA
  ADC __sp
  STA __sp
  TXA
  ADC __sp+1
  STA __sp+1
  RTS


__var_addr:
  LDY #0
  
  // A = dest offset into zero page.
  // X = var_offset lo
  // Y = var_offset hi
__var_addrb:
  SEC
  STX __t0
  STY __t1
  TAY
  LDA __fp
  SBC __t0
  STA 0, Y
  LDA __fp+1
  SBC __t1
  STA 1, Y
  RTS

__arg_addr:
  LDY #0
  
  // A = dest offset into zero page.
  // X = var_offset lo
  // Y = var_offset hi
__arg_addrb:
  CLC
  STX __t0
  STY __t1
  TAY
  LDA __fp
  ADC __t0
  STA 0, Y
  LDA __fp+1
  ADC __t1
  STA 1, Y
  RTS


__var_addr_xy:
  LDY #0
  
  // X = var_offset lo
  // Y = var_offset hi
  // Result:
  // X = var address LO
  // Y = var address HI
__var_addrb_xy:
  SEC
  STX __t0
  STY __t1
  LDA __fp
  SBC __t0
  TAX
  LDA __fp+1
  SBC __t1
  TAY
  RTS

__arg_addr_xy:
  LDY #0
  
  // X = arg_offset lo
  // Y = arg_offset hi
  // Result:
  // X = arg address LO
  // Y = arg address HI
__arg_addrb_xy:
  CLC
  TXA
  ADC __fp
  TAX
  TYA
  ADC __fp+1
  TAY
  RTS

__var_value1:
  LDY #0
  

  // A = dest offset into zero page.
  // X = var_offset lo
  // Y = var_offset hi
__var_value1b:
  JSR varaddr
  TAX
  LDA (__t0)
  STA 0, X
  RTS

__var_value2:
  LDY #0
  // A = dest offset into zero page.
  // X = var_offset lo
  // Y = var_offset hi
__var_value2b:
  JSR varaddr
  TAX
  LDA (__t0)
  STA 0, X
  LDY #1
  LDA (__t0), Y
  STA 1,X
  RTS


__var_value4:
  LDY #0
  // A = dest offset into zero page.
  // X = var_offset lo
  // Y = var_offset hi
__var_value4b:
  JSR varaddr
  TAX
  LDY #0
vv4b:
  LDA (__t0), Y
  STA 0, X
  INX
  INY
  CPY #4
  BNE vv4b
  RTS

__var_value8:
  LDY #0

  // A = dest offset into zero page.
  // X = var_offset lo
  // Y = var_offset hi
__var_value8b:
  JSR varaddr
  TAX
  LDY #0
vv8b:
  LDA (__t0), Y
  STA 0, X
  INX
  INY
  CPY #8
  BNE vv8b
  RTS


__arg_value1:
  LDY #0
  
  // A = dest offset into zero page.
  // X = arg_offset lo
  // Y = arg_offset hi
__arg_value1b:
  JSR argaddr
  TAX
  LDA (__t0)
  STA 0, X
  RTS

__arg_value2:
  LDY #0
  // A = dest offset into zero page.
  // X = arg_offset lo
  // Y = arg_offset hi
__arg_value2b:
  JSR argaddr
  TAX
  LDA (__t0)
  STA 0, X
  LDY #1
  LDA (__t0), Y
  STA 1,X
  RTS


__arg_value4:
  LDY #0
  // A = dest offset into zero page.
  // X = arg_offset lo
  // Y = arg_offset hi
__arg_value4b:
  JSR argaddr
  TAX
  LDY #0
av4b:
  LDA (__t0), Y
  STA 0, X
  INX
  INY
  CPY #4
  BNE av4b
  RTS

__arg_value8:
  LDY #0

  // A = dest offset into zero page.
  // X = arg_offset lo
  // Y = arg_offset hi
__arg_value8b:
  JSR argaddr
  TAX
  LDY #0
av8b:
  LDA (__t0), Y
  STA 0, X
  INX
  INY
  CPY #8
  BNE av8b
  RTS

// Entry:
// A: offset of zero page reg
// X,Y: offset from fp for var
__set_var_value1:
  LDY #0
__set_var_value1b:
  JSR varaddr
  TAX
  LDA 0,X
  STA (__t0)
  RTS


__set_var_value2:
  LDY #0
__set_var_value2b:
  JSR varaddr
  TAX
  LDA 0,X
  STA (__t0)
  LDY #1
  LDA 1,X
  STA (__t0), Y
  RTS

__set_var_value4:
  LDY #0
__set_var_value4b:
  JSR varaddr
  TAX
  LDY #0
svv4b:
  LDA 0, X
  STA (__t0), Y
  INX
  INY
  CPY #4
  BNE svv4b
  RTS

__set_var_value8:
  LDY #0
__set_var_value8b:
  JSR varaddr
  TAX
  LDY #0
svv8b:
  LDA 0, X
  STA (__t0), Y
  INX
  INY
  CPY #8
  BNE svv8b
  RTS


__set_arg_value1:
  LDY #0
__set_arg_value1b:
  JSR argaddr
  TAX
  LDA 0,X
  STA (__t0)
  RTS


__set_arg_value2:
  LDY #0
__set_arg_value2b:
  JSR argaddr
  TAX
  LDA 0,X
  STA (__t0)
  LDY #1
  LDA 1,X
  STA (__t0), Y
  RTS

__set_arg_value4:
  LDY #0
__set_arg_value4b:
  JSR argaddr
  TAX
  LDY #0
sav4b:
  LDA 0, X
  STA (__t0), Y
  INX
  INY
  CPY #4
  BNE sav4b
  RTS

__set_arg_value8:
  LDY #0
__set_arg_value8b:
  JSR argaddr
  TAX
  LDY #0
sav8b:
  LDA 0, X
  STA (__t0), Y
  INX
  INY
  CPY #8
  BNE sav8b
  RTS

// Entry:
// X,Y: offset from fp for var
__zero_var_value1:
  LDY #0
__zero_var_value1b:
  JSR varaddr
  LDA #0
  STA (__t0)
  RTS


__zero_var_value2:
  LDY #0
__zero_var_value2b:
  JSR varaddr
  LDA #0
  STA (__t0)
  LDY #1
  STA (__t0), Y
  RTS

__zero_var_value4:
  LDY #0
__zero_var_value4b:
  JSR varaddr
  LDA #0
  TAY
zvv4b:
  STA (__t0), Y
  INY
  CPY #4
  BNE zvv4b
  RTS

__zero_var_value8:
  LDY #0
__zero_var_value8b:
  JSR varaddr
  LDA #0
  TAY
zvv8b:
  STA (__t0), Y
  INY
  CPY #8
  BNE zvv8b
  RTS


__zero_arg_value1:
  LDY #0
__zero_arg_value1b:
  JSR argaddr
  LDA #0
  STA (__t0)
  RTS


__zero_arg_value2:
  LDY #0
__zero_arg_value2b:
  JSR argaddr
  LDA #0
  STA (__t0)
  LDY #1
  STA (__t0), Y
  RTS

__zero_arg_value4:
  LDY #0
__zero_arg_value4b:
  JSR argaddr
  LDA #0
  TAY
zav4b:
  STA (__t0), Y
  INY
  CPY #4
  BNE zav4b
  RTS

__zero_arg_value8:
  LDY #0
__zero_arg_value8b:
  JSR argaddr
  LDA #0
  TAY
zav8b:
  STA (__t0), Y
  INY
  CPY #8
  BNE zav8b
  RTS


// Load the __result from the stack frame.
// It's at (sp) + 2
__load_result:
  LDY #2
  LDA (__sp),Y
  STA __result
  INY
  LDA (__sp),Y
  STA __result+1
  RTS

// A: zero page offset for 2-byte result
// __result: address for result.
__result2:
  TAX
  LDA 0,X
  STA (__result)
  LDA 1,X
  LDY #1
  STA (__result), Y
  RTS

// X,Y: address of result value
// __result: address for result.
__result4:
  STX __t0
  STY __t1
  LDY #0
__result4loop:
  LDA (__t0), Y
  STA (__result), Y
  INY
  INX
  CPY #4
  BNE __result4loop
  RTS

  
// X,Y: address of result value
// __result: address for result.
__result8:
  STX __t0
  STY __t1
  LDY #0
__result8loop:
  LDA (__t0), Y
  STA (__result), Y
  INY
  INX
  CPY #8
  BNE __result8loop
  RTS

// X: number of bytes to increment sp by
__incsp1:
  LDY #0

// X,Y: number of bytes to increment sp by
__incsp2:
  TXA
  CLC
  ADC __sp
  STA __sp
  TYA
  ADC __sp+1
  STA __sp+1
  RTS

incsp1:
  INC __sp
  BNE i1
  INC __sp+1
i1:
  RTS

decsp1:
  DEC __sp
  BNE d1
  DEC __sp+1
d1:
  RTS

decsp2:
  SEC
  LDA __sp
  SBC #2
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  RTS

incsp2:
  CLC
  LDA __sp
  ADC #2
  STA __sp
  LDA __sp+1
  ADC #0
  STA __sp+1
  RTS

decsp4:
  SEC
  LDA __sp
  SBC #4
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  RTS

incsp4:
  CLC
  LDA __sp
  ADC #4
  STA __sp
  LDA __sp+1
  ADC #0
  STA __sp+1
  RTS

decsp8:
  SEC
  LDA __sp
  SBC #8
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  RTS

incsp8:
  CLC
  LDA __sp
  ADC #8
  STA __sp
  LDA __sp+1
  ADC #0
  STA __sp+1
  RTS

// A: byte to push.
__pusha:
  JSR decsp1
  STA (__sp)
  RTS

__pulla:
  LDA (__sp)
  INC __sp
  BNE pla
  INC __sp+1
pla:
  RTS

// X,Y: int16 to push.
__pushxy0:
  LDY #0
__pushxy:
  JSR decsp2
  TXA
  STA (__sp)
  TYA
  LDY #1
  STA (__sp), Y
  RTS

// Entry:
// X: index of reg to push
__pushreg1:
  JSR descp1
  LDA 0,X
  STA (__sp)
  RTS

__pushreg2:
  JSR decsp2
  LDA 0,X
  STA (__sp)
  LDY #1
  LDA 1,X
  STA (__sp),Y
  RTS

__pushreg4:
  JSR decsp4
  LDY #0
preg4:
  LDA 0,X
  STA (__sp),Y
  INX
  INY
  CPY #4
  BNE preg4
  RTS

__pushreg8:
  JSR decsp4
  LDY #0
preg8:
  LDA 0,X
  STA (__sp),Y
  INX
  INY
  CPY #8
  BNE preg8
  RTS

__pullxy:
  LDA (__sp)
  TAX
  LDY #1
  LDA (__sp),Y
  TAY
  JMP incsp2

// X,Y: address of int32 to push
__push4:
  JSR decsp4
  STX __t0
  STY __t1
  LDY #0
p4l:
  LDA (__t0),Y
  STA (__sp), Y
  INY
  CPY #4
  BNE p4l
  RTS

__pull4:
  STX __t0
  STY __t1
  LDY #0
pl4l:
  LDA (__sp),Y
  STA (__t0), Y
  INY
  CPY #4
  BNE pl4l
  JMP incsp4

// X,Y: address of int64 to push
__push8:
  JSR decsp8
  STX __t0
  STY __t1
  LDY #0
p8l:
  LDA (__t0),Y
  STA (__sp), Y
  INY
  CPY #8
  BNE p8l
  RTS

__pull8:
  STX __t0
  STY __t1
  LDY #0
pl8l:
  LDA (__sp),Y
  STA (__t0), Y
  INY
  CPY #8
  BNE pl8l
  JMP incsp8

__pushmem1:
  // Decrement sp by __mem_size (1 byte)
  SEC
  LDA __sp
  SBC __mem_size
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1

  LDY #0
pm1l:
  // while t0,t1 != __mem_size
  CPY __mem_size
  BEQ end_pm1
  // Copy one byte from src to dest
  LDA (__mem_src),Y
  STA (__sp),Y
  INY
  BNE pm1l
end_pm1:
  RTS

__pushmem2:
  // Decrement sp by __mem_size and store sp
  SEC
  LDA __sp
  PHA
  SBC __mem_size
  STA __sp
  LDA __sp+1
  PHA
  SBC __mem_size+1
  STA __sp+1

  // t0,t1: byte counter.
  STZ __t0
  STZ __t1
pml:
  // while t0,t1 != __mem_size
  LDA __t0
  CMP __mem_size
  BNE pml1
  LDA __t1
  CMP __mem_size+1
  BEQ end_pm
pml1:
  // Copy one byte from src to stack
  LDA (__mem_src)
  STA (__sp)

  // Inc src
  INC __mem_src
  BNE pms
  INC __mem_src+1
pms:
  // Inc sp.
  INC __sp
  BNE pmd
  INC __sp+1
pmd:
  // Inc t0.
  INC __t0
  BNE pml
  INC __t1
  JMP pml
end_pm:
  PLA
  STA __sp+1
  PLA
  STA __sp
  RTS

__copymem1:
  LDY #0
cm1l:
  // while t0,t1 < __mem_size
  CPY __mem_size
  BEQ end_cm1
  // Copy one byte from src to dest
  LDA (__mem_src),Y
  STA (__mem_dest),Y
  INY
  BNE cm1l
end_cm1:
  RTS

__copymem2:
  // t0,t1: byte counter.
  STZ __t0
  STZ __t1
cml:
  // while t0,t1 != __mem_size
  LDA __t0
  CMP __mem_size
  BNE cml1
  LDA __t1
  CMP __mem_size+1
  BEQ end_cm
cml1:
  // Copy one byte from src to stack
  LDA (__mem_src)
  STA (__mem_dest)

  // Inc src
  INC __mem_src
  BNE cms
  INC __mem_src+1
cms:
  // Inc dest.
  INC __mem_dest
  BNE cmd
  INC __mem_dest+1
cmd:
  // Inc t0.
  INC __t0
  BNE cml
  INC __t1
  JMP cml
end_cm:
  RTS


// Entry:
// X,Y: address of location to put result
// sp/sp+1: args:
// (sp): syscon number
// (sp), 2: first arg
syscall:
  LDA (__sp)        // Load low byte of syscall
  STA %abs(syscall_brk)   // Store in BRK instruction sequence
  TXA
  STA (__sp)          // Overwrite syscall number on stack with result address
  TYA
  LDY #1
  STA (__sp), Y

  // Invoke BRK.  The byte after the BRK is the syscall number.  The
  // word at (__sp) is the address of where to store the result.
  // The routine invoked by BRK will write its result there.
  BRK
syscall_brk:
  NOP      // Overwritten with syscall number.
  RTS

// Address of var with offset in X,Y.  Result in t0,t1.  Saves A
varaddr:
  PHA
  STX __t0
  STY __t1
  SEC
  LDA __fp
  SBC __t0
  STA __t0
  LDA __fp+1
  SBC __t1
  STA __t1
  PLA
  RTS

// Push a 1-byte varialble at frame offset X,Y.
// Offset is subtracted from fp.
__push_var1:
  LDY #0

__push_var1b:
  JSR varaddr
  JSR decsp1
  LDA (__t0)
  STA (__sp)
  RTS

__push_var2:
  LDY #0

__push_var2b:
  JSR varaddr
  JSR decsp2
  LDA (__t0)
  STA (__sp)
  LDY #1
  LDA (__t0),Y
  STA (__sp), Y
  RTS

__push_var4:
  LDY #0

__push_var4b:
  JSR varaddr
  JSR decsp4
  LDY #3
push_var4bl:
  LDA (__t0),Y
  STA (__sp),Y
  DEY
  BPL push_var4bl
  RTS

__push_var8:
  LDY #0

__push_var8b:
  JSR varaddr
  JSR decsp8
  LDY #7
push_var8bl:
  LDA (__t0),Y
  STA (__sp),Y
  DEY
  BPL push_var8bl
  RTS


// Address of arg with offset in X,Y.  Result in t0,t1.
argaddr:
  PHA
  STX __t0
  STY __t1
  CLC
  LDA __fp
  ADC __t0
  STA __t0
  LDA __fp+1
  ADC __t1
  STA __t1
  PLA
  RTS

// Push a 1-byte arg at frame offset X,Y.
// Offset is subtracted from fp.
__push_arg1:
  LDY #0

__push_arg1b:
  JSR argaddr
  JSR decsp1
  LDA (__t0)
  STA (__sp)
  RTS

__push_arg2:
  LDY #0

__push_arg2b:
  JSR argaddr
  JSR decsp2
  LDA (__t0)
  STA (__sp)
  LDY #1
  LDA (__t0),Y
  STA (__sp), Y
  RTS

__push_arg4:
  LDY #0

__push_arg4b:
  JSR argaddr
  JSR decsp4
  LDY #3
push_arg4bl:
  LDA (__t0),Y
  STA (__sp),Y
  DEY
  BPL push_arg4bl
  RTS

__push_arg8:
  LDY #0

__push_arg8b:
  JSR argaddr
  JSR decsp8
  LDY #7
push_arg8bl:
  LDA (__t0),Y
  STA (__sp),Y
  DEY
  BPL push_arg8bl
  RTS

// Entry A: offet into zero page containing entry offset into table.
// We multiply this by 2 to get the byte offset into the table.
// The table is immediately after the JSR instruction
__jump_table:
  TAY               // Offset of entry number in zero page.
  TSX               // X = stack pointer
  LDA 0x101,X       // Load LO byte
  STA __t0          // Copy to temp addr
  LDA 0x102,X       // Load HI byte
  STA __t1
  INC __t0          // JSR puts return address -1 on stack.
  BNE jp_skip
  INC __t1
jp_skip:
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

// Entry:
// A: offset into zero page for result
// X: offset into zero page for op1
// Y: offset into zero page for op2
// Places result in 0,A.
.set product mt1
.set multiplicand mt2
.set multiplier mt3

// For signed multiply we check the sign bits of the 2 operands.
// If they are different we know one of them is negative.  We negate
// the negative one and then negate the result.
__smul1:
  PHA
  LDA 0,X
  STA multiplicand
  LDA 0,Y
  STA multiplier
  EOR multiplicand
  BPL umul1b

  // One of multiplier or multiplicand is negative.  Result will be negative.
  LDA multiplier
  BPL smul1a

  // Multiplier is negative, negate it.
  SEC
  LDA #0
  SBC multiplier
  STA multiplier
  BRA smul1b
smul1a:
  // Multiplicand is negative, negate it.
  SEC
  LDA #0
  SBC multiplicand
  STA multiplicand

smul1b:
  // Perform unsigned mutiply
  JSR umul1b

  // Negate result.
  SEC
  LDA #0
  SBC 0,X
  STA 0,X
  RTS

__umul1:
  PHA
  LDA 0,X
  STA multiplicand
  LDA 0,Y
  STA multiplier

// These are from:
// https://llx.com/Neil/a2/mult.html
umul1b:
        LDA #0       // Initialize RESULT to 0
        LDX #$8       // There are 8 bits in multiplier
umul1_l1:
        LSR multiplier       // Get low bit of multiplier
        BCC umul1_l2        // 0 or 1?
        CLC                 // If 1, add multiplicand
        ADC multiplicand
umul1_l2:
        ROR A        // "Stairstep" shift (catching carry from add)
        ROR product
        DEX
        BNE umul1_l1
        PLX
        LDA product
        STA 0,X
        RTS

__smul2:
  PHA
  LDA 0,X
  STA multiplicand
  LDA 1,X
  STA multiplicand+1
  LDA 0,Y
  STA multiplier
  LDA 1,Y
  STA multiplier+1
  EOR multiplicand+1
  BPL umul2b

  // One of multiplier or multiplicand is negative.  Result will be negative.
  LDA multiplier+1
  BPL smul2a

  // Multiplier is negative, negate it.
  SEC
  LDA #0
  SBC multiplier
  STA multiplier
  LDA #0
  SBC multiplier+1
  STA multiplier+1
  BRA smul2b
smul2a:
  // Multiplicand is negative, negate it.
  SEC
  LDA #0
  SBC multiplicand
  STA multiplicand
  LDA #0
  SBC multiplicand+1
  STA multiplicand+1

smul2b:
  // Perform unsigned mutiply
  JSR umul2b

  // Negate result.
  SEC
  LDA #0
  SBC 0,X
  STA 0,X
  LDA #0
  SBC 1,X
  STA 1,X
  RTS

__umul2:
    PHA
    LDA 0,X
    STA multiplicand
    LDA 1,X
    STA multiplicand+1
    LDA 0,Y
    STA multiplier
    LDA 1,Y
    STA multiplier+1


umul2b:
            LDA #0       // Initialize product to 0
            STA product+2
            LDX #16      // There are 16 bits in NUM2
umul2_l1:
            LSR multiplier+1   // Get low bit of NUM2
            ROR multiplier
            BCC umul2_l2       // 0 or 1?
            TAY          // If 1, add NUM1 (hi byte of RESULT is in A)
            CLC
            LDA multiplicand
            ADC product+2
            STA product+2
            TYA
            ADC multiplicand+1
umul2_l2:
            ROR A        // "Stairstep" shift
            ROR product+2
            ROR product+1
            ROR product
            DEX
            BNE umul2_l1
            PLX
            LDA product
            STA 0,X
            LDA product+1
            STA 1,X
            RTS

__smul4:
  PHA
  LDA 0,X
  STA multiplicand
  LDA 1,X
  STA multiplicand+1
  LDA 2,X
  STA multiplicand+2
  LDA 3,X
  STA multiplicand+3
  LDA 0,Y
  STA multiplier
  LDA 1,Y
  STA multiplier+1
  LDA 2,Y
  STA multiplier+2
  LDA 3,Y
  STA multiplier+3
  EOR multiplicand+3
  BPL umul4b

  // One of multiplier or multiplicand is negative.  Result will be negative.
  LDA multiplier+3
  BPL smul4a

  // Multiplier is negative, negate it.
  SEC
  LDA #0
  SBC multiplier
  STA multiplier
  LDA #0
  SBC multiplier+1
  STA multiplier+1
  LDA #0
  SBC multiplier+2
  STA multiplier+2
  LDA #0
  SBC multiplier+3
  STA multiplier+3
  BRA smul4b
smul4a:
  // Multiplicand is negative, negate it.
  SEC
  LDA #0
  SBC multiplicand
  STA multiplicand
  LDA #0
  SBC multiplicand+1
  STA multiplicand+1
  LDA #0
  SBC multiplicand+2
  STA multiplicand+2
  LDA #0
  SBC multiplicand+3
  STA multiplicand+3

smul4b:
  // Perform unsigned mutiply
  JSR umul4b

  // Negate result.
  SEC
  LDA #0
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
  RTS

__umul4:
    PHA
    LDA 0,X
    STA multiplicand
    LDA 1,X
    STA multiplicand+1
    LDA 2,X
    STA multiplicand+2
    LDA 3,X
    STA multiplicand+3
    LDA 0,Y
    STA multiplier
    LDA 1,Y
    STA multiplier+1
    LDA 2,Y
    STA multiplier+2
    LDA 3,Y
    STA multiplier+3


umul4b:
            LDA #0       // Initialize product to 0
            STA product+6
            STA product+5
            STA product+4
            LDX #32      // There are 32 bits in NUM2
umul4_l1:
            LSR multiplier+3   // Get low bit of NUM2
            ROR multiplier+2
            ROR multiplier+1
            ROR multiplier
            BCC umul4_l2       // 0 or 1?
            TAY          // If 1, add NUM1 (hi byte of RESULT is in A)
            CLC
            LDA multiplicand
            ADC product+4
            STA product+4
            LDA multiplicand+1
            ADC product+5
            STA product+5
            LDA multiplicand+2
            ADC product+6
            STA product+6
            TYA
            ADC multiplicand+3
umul4_l2:
            ROR A        // "Stairstep" shift
            ROR product+6
            ROR product+5
            ROR product+4
            ROR product+3
            ROR product+2
            ROR product+1
            ROR product
            DEX
            BNE umul4_l1
            PLX
            LDA product
            STA 0,X
            LDA product+1
            STA 1,X
            LDA product+2
            STA 2,X
            LDA product+3
            STA 3,X
            RTS

__smul8:
  PHA
  LDA 0,X
  STA multiplicand
  LDA 1,X
  STA multiplicand+1
  LDA 2,X
  STA multiplicand+2
  LDA 3,X
  STA multiplicand+3
  LDA 4,X
  STA multiplicand+4
  LDA 5,X
  STA multiplicand+5
  LDA 6,X
  STA multiplicand+6
  LDA 7,X
  STA multiplicand+7
  LDA 0,Y
  STA multiplier
  LDA 1,Y
  STA multiplier+1
  LDA 2,Y
  STA multiplier+2
  LDA 3,Y
  STA multiplier+3
  LDA 4,Y
  STA multiplier+4
  LDA 5,Y
  STA multiplier+5
  LDA 6,Y
  STA multiplier+6
  LDA 7,Y
  STA multiplier+7
  EOR multiplicand+7
  BMI smul8neg
  JMP umul8b
smul8neg:

  // One of multiplier or multiplicand is negative.  Result will be negative.
  LDA multiplier+7
  BPL smul8a

  // Multiplier is negative, negate it.
  SEC
  LDA #0
  SBC multiplier
  STA multiplier
  LDA #0
  SBC multiplier+1
  STA multiplier+1
  LDA #0
  SBC multiplier+2
  STA multiplier+2
  LDA #0
  SBC multiplier+3
  STA multiplier+3
  LDA #0
  SBC multiplier+4
  STA multiplier+4
  LDA #0
  SBC multiplier+5
  STA multiplier+5
  LDA #0
  SBC multiplier+6
  STA multiplier+6
  LDA #0
  SBC multiplier+7
  STA multiplier+7
  BRA smul8b
smul8a:
  // Multiplicand is negative, negate it.
  SEC
  LDA #0
  SBC multiplicand
  STA multiplicand
  LDA #0
  SBC multiplicand+1
  STA multiplicand+1
  LDA #0
  SBC multiplicand+2
  STA multiplicand+2
  LDA #0
  SBC multiplicand+3
  STA multiplicand+3
  LDA #0
  SBC multiplicand+4
  STA multiplicand+4
  LDA #0
  SBC multiplicand+5
  STA multiplicand+5
  LDA #0
  SBC multiplicand+6
  STA multiplicand+6
  LDA #0
  SBC multiplicand+7
  STA multiplicand+7

smul8b:
  // Perform unsigned mutiply
  JSR umul8b

  // Negate result.
  SEC
  LDA #0
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
  RTS

__umul8:
    PHA
    LDA 0,X
    STA multiplicand
    LDA 1,X
    STA multiplicand+1
    LDA 2,X
    STA multiplicand+2
    LDA 3,X
    STA multiplicand+3
    LDA 4,X
    STA multiplicand+4
    LDA 5,X
    STA multiplicand+5
    LDA 6,X
    STA multiplicand+6
    LDA 7,X
    STA multiplicand+7
    LDA 0,Y
    STA multiplier
    LDA 1,Y
    STA multiplier+1
    LDA 2,Y
    STA multiplier+2
    LDA 3,Y
    STA multiplier+3
    LDA 4,Y
    STA multiplier+4
    LDA 5,Y
    STA multiplier+5
    LDA 6,Y
    STA multiplier+6
    LDA 7,Y
    STA multiplier+7

umul8b:
            LDA #0       // Initialize product to 0
            STA product+14
            STA product+13
            STA product+12
            STA product+11
            STA product+10
            STA product+9
            STA product+8
            LDX #64      // There are 64 bits in NUM2
umul8_l1:
            LSR multiplier+7   // Get low bit of NUM2
            ROR multiplier+6
            ROR multiplier+5
            ROR multiplier+4
            ROR multiplier+3
            ROR multiplier+2
            ROR multiplier+1
            ROR multiplier
            BCC umul8_l2       // 0 or 1?
            TAY          // If 1, add NUM1 (hi byte of RESULT is in A)
            CLC
            LDA multiplicand
            ADC product+8
            STA product+8
            LDA multiplicand+1
            ADC product+9
            STA product+9
            LDA multiplicand+2
            ADC product+10
            STA product+10
            LDA multiplicand+3
            ADC product+11
            STA product+11
            LDA multiplicand+4
            ADC product+12
            STA product+12
            LDA multiplicand+5
            ADC product+13
            STA product+13
            LDA multiplicand+6
            ADC product+14
            STA product+14
            TYA
            ADC multiplicand+7
umul8_l2:
            ROR A        // "Stairstep" shift
            ROR product+14
            ROR product+13
            ROR product+12
            ROR product+11
            ROR product+10
            ROR product+9
            ROR product+8
            ROR product+7
            ROR product+6
            ROR product+5
            ROR product+4
            ROR product+3
            ROR product+2
            ROR product+1
            ROR product
            DEX
            BNE umul8_l1
    PLX
    LDA product
    STA 0,X
    LDA product+1
    STA 1,X
    LDA product+2
    STA 2,X
    LDA product+2
    STA 2,X
    LDA product+3
    STA 3,X
    LDA product+4
    STA 4,X
    LDA product+5
    STA 5,X
    LDA product+6
    STA 6,X
    LDA product+7
    STA 7,X
    rts

__fmul:
__dmul:

// Division routines:
.set dividend mt2
.set divisor mt3
.set remainder mt1
.set quotient mt2

__sdiv1:
__sdiv4:
__sdiv8:
  RTS

__udiv1:
  RTS


// Signed 2-byte divide.
// TODO: this is wrong for negative numbers according to C99.
// We need to round the quotient toward zero and the sign of the remainer
// can be negative.
__sdiv2:
  PHA
  LDA 0,X
  STA dividend
  LDA 1,X
  STA dividend+1
  LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1
  EOR dividend+1bp
  BPL udiv2_1

  // One of divisor or dividend is negative.  Result will be negative.
  LDA divisor+1
  BPL sdiv2_l1

  // Divisor is negative, negate it.
  SEC
  LDA #0
  SBC divisor
  STA divisor
  LDA #0
  SBC divisor+1
  STA divisor+1
  BRA sdiv2_l2

sdiv2_l1:
  // Dividend is negative, negate it.
  SEC
  LDA #0
  SBC dividend
  STA dividend
  LDA #0
  SBC dividend+1
  STA dividend+1

sdiv2_l2:
  // Perform unsigned divide
  JSR udiv2

  // Negate result.
  SEC
  LDA #0
  SBC 0,X
  STA 0,X
  LDA #0
  SBC 1,X
  STA 1,X
  RTS

__udiv2:
  PHA
  LDA 0,X
  STA dividend
  LDA 1,X
  STA dividend+1
  LDA 0,Y
  STY divisor
  LDA 1,Y
  STA divisor+1

udiv2_1:
  JSR udiv2

  // Store result.
  PLX
  LDA quotient
  STA 0,X
  LDA quotient+1
  STA 1,X
  RTS

// Main udiv2 routine.  Produces both remainder and quotent
udiv2:
        // Zero out remainder.
        STZ remainder
        STZ remainder+1
        LDX #16     // There are 16 bits in NUM1
udiv2_l1:
        ASL dividend    // Shift hi bit of divisor into remainder
        ROL dividend+1   // (vacating the lo bit, which will be used for the quotient)
        ROL remainder
        ROL remainder+1
        LDA remainder
        SEC         // Trial subtraction
        SBC divisor
        TAY
        LDA remainder+1
        SBC divisor+1
        BCC udiv2_l2       // Did subtraction succeed?
        STA remainder+1   // If yes, save it
        STY remainder
        INC dividend    // and record a 1 in the quotient
udiv2_l2:
        DEX
        BNE udiv2_l1
        RTS

__smod2:
  PHA
  LDA 0,X
  STA dividend
  LDA 1,X
  STA dividend+1
  LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1
  EOR dividend+1
  BPL umod2_1

  // One of divisor or dividend is negative.  Result will be negative.
  LDA divisor+1
  BPL smod2_l1

  // Divisor is negative, negate it.
  SEC
  LDA #0
  SBC divisor
  STA divisor
  LDA #0
  SBC divisor+1
  STA divisor+1
  BRA smod2_l2

smod2_l1:
  // Dividend is negative, negate it.
  SEC
  LDA #0
  SBC dividend
  STA dividend
  LDA #0
  SBC dividend+1
  STA dividend+1

smod2_l2:
  // Perform unsigned divide
  JSR umod2_1

  // Negate result.
  SEC
  LDA #0
  SBC 0,X
  STA 0,X
  LDA #0
  SBC 1,X
  STA 1,X
  RTS

__umod2:
  PHA
  LDA 0,X
  STA dividend
  LDA 1,X
  STA dividend+1
  LDA 0,Y
  STA divisor
  LDA 1,Y
  STA divisor+1

umod2_1:
  JSR udiv2

  // Store remainder as result.
  PLX
  LDA remainder
  STA 0,X
  LDA remainder+1
  STA 1,X
  RTS
 


__udiv4:
__udiv8:
__fdiv:
__ddiv:

__smod1:
__smod4:
__smod8:
__umod1:
__umod4:
__umod8:

__i1tof:
__i2tof:
__i4tof:
__i8tof:
__i1tod:
__i2tod:
__i4tod:
__i8tod:
 
__ui1tof:
__ui2tof:
__ui4tof:
__ui8tof:
__ui1tod:
__ui2tod:
__ui4tod:
__ui8tod:

__ftod:
__dtof:
 
__ftoi1:
__ftoi2:
__ftoi4:
__ftoi8:
__dtoi1:
__dtoi2:
__dtoi4:
__dtoi8:
__ftoui1:
__ftoui2:
__ftoui4:
__ftoui8:
__dtoui1:
__dtoui2:
__dtoui4:
__dtoui8:

__cmpeqf:
__cmpnef:
__cmpltf:
__cmpgef:
 
__cmpeqd:
__cmpned:
__cmpltd:
__cmpged:
  RTS

// Spilling and reloading.
// Entry:
// A: offset into zero page for value to spill.
// X,Y: offset into frame, subtracted from frame pointer.
__spill1:
  LDY #0
__spill1b:
  JSR varaddr
  TAX
  LDA 0,X
  STA (__t0)
  RTS

__spill2:
  LDY #0
__spill2b:
  JSR varaddr
  TAX
  LDA 0,X
  STA (__t0)
  LDY #1
  LDA 1,X
  STA (__t0),Y
  RTS

__spill4:
  LDY #0
__spill4b:
  JSR varaddr
  TAX
  LDY #0
spill4loop:
  LDA 0,X
  STA (__t0),Y
  INX
  INY
  CPY #4
  BNE spill4loop
  RTS

__spill8:
  LDY #0
__spill8b:
  JSR varaddr
  TAX
  LDY #0
spill8loop:
  LDA 0,X
  STA (__t0),Y
  INX
  INY
  CPY #8
  BNE spill8loop
  RTS

__reload1:
  LDY #0
__reload1b:
  JSR varaddr
  TAX
  LDA (__t0)
  STA 0,X
  RTS

__reload2:
  LDY #0
__reload2b:
  JSR varaddr
  TAX
  LDA (__t0)
  STA 0,X
  LDY #1
  LDA (__t0),Y
  STA 1,X
  RTS

__reload4:
  LDY #0
__reload4b:
  JSR varaddr
  TAX
  LDY #0
reload4loop:
  LDA (__t0),Y
  STA 0,X
  INX
  INY
  CPY #4
  BNE reload4loop
  RTS

__reload8:
  LDY #0
__reload8b:
  JSR varaddr
  TAX
  LDY #0
reload8loop:
  LDA (__t0),Y
  STA 0,X
  INX
  INY
  CPY #8
  BNE reload8loop
  RTS

__inc1:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  CLC
  LDA (__t2)
  ADC #1
  STA (__t2)
  RTS


__inc21:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  CLC
  LDA (__t2)
  ADC #1
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  RTS

__inc2:
  LDY #0
__inc2b:
  STX __t0
  STY __t1
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  CLC
  LDA (__t2)
  ADC __t0
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  ADC __t1
  STA (__t2), Y
  RTS

__inc4:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  CLC
  LDA (__t2)
  ADC #1
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  RTS


__inc8:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  CLC
  LDA (__t2)
  ADC #1
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  ADC #0
  STA (__t2), Y
  RTS

__incf:
__incfb:
__incd:
__incdb:
  RTS

__dec1:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  SEC
  LDA (__t2)
  SBC #1
  STA (__t2)
  RTS

__dec21:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  SEC
  LDA (__t2)
  SBC #1
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  RTS

__dec2:
  LDY #0
__dec2b:
  STX __t0
  STY __t1
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  SEC
  LDA (__t2)
  SBC __t0
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  SBC __t1
  STA (__t2), Y
  RTS

__dec4:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  SEC
  LDA (__t2)
  SBC #1
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  RTS


__dec8:
  TAX
  LDA 0,X
  STA __t2
  LDA 1,X
  STA __t3
  SEC
  LDA (__t2)
  SBC #1
  STA (__t2)
  LDY #1
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  INY
  LDA (__t2), Y
  SBC #0
  STA (__t2), Y
  RTS

__decf:
__decfb:
__decd:
__decdb:
  RTS

.global __end
__end:

