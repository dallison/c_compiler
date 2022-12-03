	.file   "Architecture/loader_arch_6502.c"
	.text
	.option pic
.PCbegin:
	.local  InitGOTPLT
	.type InitGOTPLT, @function

InitGOTPLT:

	// *** Basic block 0

	ret         
.func_end_InitGOTPLT:
	.size InitGOTPLT, .func_end_InitGOTPLT-InitGOTPLT

	.local  ApplyGOTDataRelocation
	.type ApplyGOTDataRelocation, @function

ApplyGOTDataRelocation:

	// *** Basic block 0

	ret         
.func_end_ApplyGOTDataRelocation:
	.size ApplyGOTDataRelocation, .func_end_ApplyGOTDataRelocation-ApplyGOTDataRelocation

	.local  ApplyGOTPLTRelocation
	.type ApplyGOTPLTRelocation, @function

ApplyGOTPLTRelocation:

	// *** Basic block 0

	ret         
.func_end_ApplyGOTPLTRelocation:
	.size ApplyGOTPLTRelocation, .func_end_ApplyGOTPLTRelocation-ApplyGOTPLTRelocation

	.global W65C02LoaderArchitectureInit
	.type W65C02LoaderArchitectureInit, @function

W65C02LoaderArchitectureInit:

	// *** Basic block 0

	.local InitGOTPLT
	.local ApplyGOTDataRelocation
	.local ApplyGOTPLTRelocation
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -24(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 0
	li          t1, 6502		// 0x1966
	sw          t1, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 8
	lla         t1, .str.1
	sd          t1, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 16
	li          t1, 1		// 0x1 ASCII \x1
	sb          t1, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 24
	la          t1, InitGOTPLT
	sd          t1, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 32
	la          t1, ApplyGOTDataRelocation
	sd          t1, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 40
	la          t1, ApplyGOTPLTRelocation
	sd          t1, 0(t0)
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_W65C02LoaderArchitectureInit:
	.size W65C02LoaderArchitectureInit, .func_end_W65C02LoaderArchitectureInit-W65C02LoaderArchitectureInit

	.global New6502LoaderArchitecture
	.type New6502LoaderArchitecture, @function

New6502LoaderArchitecture:

	// *** Basic block 0

	.global malloc
	.global W65C02LoaderArchitectureInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -24(s0)
	// End of stack frame
	li          a0, 48		// 0x30 ASCII '0'
	call        malloc

	// *** Basic block 1

	sd          a0, -24(s0)
	ld          a0, -24(s0)
	call        W65C02LoaderArchitectureInit

	// *** Basic block 2

	ld          t0, -24(s0)
	mv          a0, t0

	// *** Basic block 3

.New6502LoaderArchitecture_label_18:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_New6502LoaderArchitecture:
	.size New6502LoaderArchitecture, .func_end_New6502LoaderArchitecture-New6502LoaderArchitecture

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "6502"
	.type .str.1, @object
	.size .str.1, 5

