	.file   "Architecture/loader_arch_riscv.c"
	.text
	.option pic
.PCbegin:
	.local  InitGOTPLT
	.type InitGOTPLT, @function

InitGOTPLT:

	// *** Basic block 0

	.global DynamicLoaderFindDynamicSectionAddressEntry
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -48(s0)
	// End of stack frame
	ld          a0, -24(s0)
	li          a1, 3		// 0x3 ASCII \x3
	call        DynamicLoaderFindDynamicSectionAddressEntry

	// *** Basic block 1

	sd          a0, -48(s0)
	ld          t0, -48(s0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .InitGOTPLT_label_26

	// *** Basic block 2

.InitGOTPLT_label_23:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.InitGOTPLT_label_26:
	ld          t0, -48(s0)
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 0
	ld          t1, -32(s0)
	sd          t1, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 8
	ld          t1, -24(s0)
	sd          t1, 0(t0)
	j           .InitGOTPLT_label_23
.func_end_InitGOTPLT:
	.size InitGOTPLT, .func_end_InitGOTPLT-InitGOTPLT

	.local  ApplyGOTDataRelocation
	.type ApplyGOTDataRelocation, @function

ApplyGOTDataRelocation:

	// *** Basic block 0

	.global LoaderError
	.global abort
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Saved argument registers.
	sd a1, -24(s0)
	sd a2, -32(s0)
	sd a3, -40(s0)
	sd a4, -48(s0)
	sd a0, -56(s0)
	// Local vars at offset -56(s0)
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 8
	ld          t0, 0(t0)
	li          t1, 4294967295		// 0xffffffff
	and         s1, t0, t1
	slti        t0, s1, 2
	li          t0, 2		// 0x2 ASCII \x2
	blt         s1, t0, .ApplyGOTDataRelocation_label_87

	// *** Basic block 1

	li          t0, 3		// 0x3 ASCII \x3
	slt         t0, t0, s1
	li          t0, 3		// 0x3 ASCII \x3
	blt         t0, s1, .ApplyGOTDataRelocation_label_87

	// *** Basic block 2

	addi        t0, s1, -2
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .ApplyGOTDataRelocation_label_36

	// *** Basic block 4

	j           .ApplyGOTDataRelocation_label_73

	// *** Basic block 5

.ApplyGOTDataRelocation_label_36:
	ld          t0, -32(s0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .ApplyGOTDataRelocation_label_52

	// *** Basic block 6

	lla         a0, .str.1
	ld          a1, -40(s0)
	call        LoaderError

	// *** Basic block 7

	j           .ApplyGOTDataRelocation_label_71

	// *** Basic block 8

.ApplyGOTDataRelocation_label_52:
	ld          t0, -48(s0)
	ld          t1, -48(s0)
	ld          t1, 0(t1)
	ld          t2, -56(s0)
	addi        t2, t2, 112
	ld          t2, 0(t2)
	add         t1, t1, t2
	ld          t2, -32(s0)
	addi        t2, t2, 8
	ld          t2, 0(t2)
	add         t1, t1, t2
	ld          t2, -24(s0)
	addi        t2, t2, 16
	ld          t2, 0(t2)
	add         t1, t1, t2
	sd          t1, 0(t0)

	// *** Basic block 9

.ApplyGOTDataRelocation_label_71:
	j           .ApplyGOTDataRelocation_label_89

	// *** Basic block 10

.ApplyGOTDataRelocation_label_73:
	ld          t0, -48(s0)
	ld          t1, -48(s0)
	ld          t1, 0(t1)
	ld          t2, -56(s0)
	addi        t2, t2, 112
	ld          t2, 0(t2)
	add         t1, t1, t2
	ld          t2, -24(s0)
	addi        t2, t2, 16
	ld          t2, 0(t2)
	add         t1, t1, t2
	sd          t1, 0(t0)
	j           .ApplyGOTDataRelocation_label_89

	// *** Basic block 11

.ApplyGOTDataRelocation_label_87:
	call        abort

	// *** Basic block 12

.ApplyGOTDataRelocation_label_89:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ApplyGOTDataRelocation:
	.size ApplyGOTDataRelocation, .func_end_ApplyGOTDataRelocation-ApplyGOTDataRelocation

	.local  ApplyGOTPLTRelocation
	.type ApplyGOTPLTRelocation, @function

ApplyGOTPLTRelocation:

	// *** Basic block 0

	.global LoaderError
	.global abort
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Saved argument registers.
	sd a1, -24(s0)
	sd a5, -32(s0)
	sd a0, -40(s0)
	sd a4, -48(s0)
	sd a2, -56(s0)
	sd a3, -64(s0)
	// Local vars at offset -64(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 8
	ld          t0, 0(t0)
	li          t1, 4294967295		// 0xffffffff
	and         t0, t0, t1
	addi        t1, t0, -5
	seqz        t1, t1
	li          t1, 5		// 0x5 ASCII \x5
	beq         t0, t1, .ApplyGOTPLTRelocation_label_26

	// *** Basic block 1

.ApplyGOTPLTRelocation_label_23:
	call        abort

	// *** Basic block 2

	j           .ApplyGOTPLTRelocation_label_69

	// *** Basic block 3

.ApplyGOTPLTRelocation_label_26:
	lb          t0, -32(s0)
	beqz        t0, .ApplyGOTPLTRelocation_label_40

	// *** Basic block 4

	ld          t0, -40(s0)
	addi        t0, t0, 112
	ld          t0, 0(t0)
	ld          t1, -48(s0)
	ld          t2, 0(t1)
	add         t0, t2, t0
	sd          t0, 0(t1)
	j           .ApplyGOTPLTRelocation_label_67

	// *** Basic block 5

.ApplyGOTPLTRelocation_label_40:
	ld          t0, -56(s0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .ApplyGOTPLTRelocation_label_56

	// *** Basic block 6

	lla         a0, .str.2
	ld          a1, -64(s0)
	call        LoaderError

	// *** Basic block 7

	j           .ApplyGOTPLTRelocation_label_66

	// *** Basic block 8

.ApplyGOTPLTRelocation_label_56:
	ld          t0, -48(s0)
	ld          t1, -40(s0)
	addi        t1, t1, 112
	ld          t1, 0(t1)
	ld          t2, -56(s0)
	addi        t2, t2, 8
	ld          t2, 0(t2)
	add         t1, t1, t2
	sd          t1, 0(t0)

	// *** Basic block 9

.ApplyGOTPLTRelocation_label_66:

	// *** Basic block 10

.ApplyGOTPLTRelocation_label_67:
	j           .ApplyGOTPLTRelocation_label_69

	// *** Basic block 11

.ApplyGOTPLTRelocation_label_69:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ApplyGOTPLTRelocation:
	.size ApplyGOTPLTRelocation, .func_end_ApplyGOTPLTRelocation-ApplyGOTPLTRelocation

	.global RISCVLoaderArchitectureInit
	.type RISCVLoaderArchitectureInit, @function

RISCVLoaderArchitectureInit:

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
	li          t1, 243		// 0xf3 ASCII \xf3
	sw          t1, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 8
	lla         t1, .str.3
	sd          t1, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 16
	sb          x0, 0(t0)
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
.func_end_RISCVLoaderArchitectureInit:
	.size RISCVLoaderArchitectureInit, .func_end_RISCVLoaderArchitectureInit-RISCVLoaderArchitectureInit

	.global NewRISCVLoaderArchitecture
	.type NewRISCVLoaderArchitecture, @function

NewRISCVLoaderArchitecture:

	// *** Basic block 0

	.global malloc
	.global RISCVLoaderArchitectureInit
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
	call        RISCVLoaderArchitectureInit

	// *** Basic block 2

	ld          t0, -24(s0)
	mv          a0, t0

	// *** Basic block 3

.NewRISCVLoaderArchitecture_label_18:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewRISCVLoaderArchitecture:
	.size NewRISCVLoaderArchitecture, .func_end_NewRISCVLoaderArchitecture-NewRISCVLoaderArchitecture

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "Relocation refers on undefined symbol \'%s\'\n"
	.type .str.1, @object
	.size .str.1, 44

.str.2:
	.asciz "Relocation refers on undefined symbol \'%s\'\n"
	.type .str.2, @object
	.size .str.2, 44

.str.3:
	.asciz "risc-v"
	.type .str.3, @object
	.size .str.3, 7

