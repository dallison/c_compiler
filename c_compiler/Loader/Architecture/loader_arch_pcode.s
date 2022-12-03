	.file   "Architecture/loader_arch_pcode.c"
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
	ld          t1, -24(s0)
	sd          t1, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 8
	ld          t1, -32(s0)
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
	slti        t0, s1, 14
	li          t0, 14		// 0xe ASCII \xe
	blt         s1, t0, .ApplyGOTDataRelocation_label_72

	// *** Basic block 1

	li          t0, 15		// 0xf ASCII \xf
	slt         t0, t0, s1
	li          t0, 15		// 0xf ASCII \xf
	blt         t0, s1, .ApplyGOTDataRelocation_label_72

	// *** Basic block 2

	addi        t0, s1, -14
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .ApplyGOTDataRelocation_label_36

	// *** Basic block 4

	j           .ApplyGOTDataRelocation_label_66

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

	j           .ApplyGOTDataRelocation_label_64

	// *** Basic block 8

.ApplyGOTDataRelocation_label_52:
	ld          t0, -48(s0)
	ld          t1, -56(s0)
	addi        t1, t1, 112
	ld          t1, 0(t1)
	ld          t2, -32(s0)
	addi        t2, t2, 8
	ld          t2, 0(t2)
	add         t1, t1, t2
	sd          t1, 0(t0)

	// *** Basic block 9

.ApplyGOTDataRelocation_label_64:
	j           .ApplyGOTDataRelocation_label_74

	// *** Basic block 10

.ApplyGOTDataRelocation_label_66:
	lla         a0, .str.2
	call        LoaderError

	// *** Basic block 11

	j           .ApplyGOTDataRelocation_label_74

	// *** Basic block 12

.ApplyGOTDataRelocation_label_72:
	call        abort

	// *** Basic block 13

.ApplyGOTDataRelocation_label_74:
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
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Saved argument registers.
	sd a1, -24(s0)
	sd a5, -32(s0)
	sd a0, -40(s0)
	sd a4, -48(s0)
	sd a2, -56(s0)
	sd a3, -64(s0)
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 8
	ld          t0, 0(t0)
	li          t1, 4294967295		// 0xffffffff
	and         s1, t0, t1
	slti        t0, s1, 14
	li          t0, 14		// 0xe ASCII \xe
	blt         s1, t0, .ApplyGOTPLTRelocation_label_86

	// *** Basic block 1

	li          t0, 15		// 0xf ASCII \xf
	slt         t0, t0, s1
	li          t0, 15		// 0xf ASCII \xf
	blt         t0, s1, .ApplyGOTPLTRelocation_label_86

	// *** Basic block 2

	addi        t0, s1, -14
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .ApplyGOTPLTRelocation_label_37

	// *** Basic block 4

	j           .ApplyGOTPLTRelocation_label_44

	// *** Basic block 5

.ApplyGOTPLTRelocation_label_37:
	lla         a0, .str.3
	call        LoaderError

	// *** Basic block 6

	j           .ApplyGOTPLTRelocation_label_88

	// *** Basic block 7

.ApplyGOTPLTRelocation_label_44:
	lb          t0, -32(s0)
	beqz        t0, .ApplyGOTPLTRelocation_label_58

	// *** Basic block 8

	ld          t0, -40(s0)
	addi        t0, t0, 112
	ld          t0, 0(t0)
	ld          t1, -48(s0)
	ld          t2, 0(t1)
	add         t0, t2, t0
	sd          t0, 0(t1)
	j           .ApplyGOTPLTRelocation_label_84

	// *** Basic block 9

.ApplyGOTPLTRelocation_label_58:
	ld          t0, -56(s0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .ApplyGOTPLTRelocation_label_73

	// *** Basic block 10

	lla         a0, .str.4
	ld          a1, -64(s0)
	call        LoaderError

	// *** Basic block 11

	j           .ApplyGOTPLTRelocation_label_83

	// *** Basic block 12

.ApplyGOTPLTRelocation_label_73:
	ld          t0, -48(s0)
	ld          t1, -40(s0)
	addi        t1, t1, 112
	ld          t1, 0(t1)
	ld          t2, -56(s0)
	addi        t2, t2, 8
	ld          t2, 0(t2)
	add         t1, t1, t2
	sd          t1, 0(t0)

	// *** Basic block 13

.ApplyGOTPLTRelocation_label_83:

	// *** Basic block 14

.ApplyGOTPLTRelocation_label_84:
	j           .ApplyGOTPLTRelocation_label_88

	// *** Basic block 15

.ApplyGOTPLTRelocation_label_86:
	call        abort

	// *** Basic block 16

.ApplyGOTPLTRelocation_label_88:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ApplyGOTPLTRelocation:
	.size ApplyGOTPLTRelocation, .func_end_ApplyGOTPLTRelocation-ApplyGOTPLTRelocation

	.global PCodeLoaderArchitectureInit
	.type PCodeLoaderArchitectureInit, @function

PCodeLoaderArchitectureInit:

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
	li          t1, 6500		// 0x1964
	sw          t1, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 8
	lla         t1, .str.5
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
.func_end_PCodeLoaderArchitectureInit:
	.size PCodeLoaderArchitectureInit, .func_end_PCodeLoaderArchitectureInit-PCodeLoaderArchitectureInit

	.global NewPCodeLoaderArchitecture
	.type NewPCodeLoaderArchitecture, @function

NewPCodeLoaderArchitecture:

	// *** Basic block 0

	.global malloc
	.global PCodeLoaderArchitectureInit
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
	call        PCodeLoaderArchitectureInit

	// *** Basic block 2

	ld          t0, -24(s0)
	mv          a0, t0

	// *** Basic block 3

.NewPCodeLoaderArchitecture_label_18:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewPCodeLoaderArchitecture:
	.size NewPCodeLoaderArchitecture, .func_end_NewPCodeLoaderArchitecture-NewPCodeLoaderArchitecture

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "Relocation refers on undefined symbol \'%s\'\n"
	.type .str.1, @object
	.size .str.1, 44

.str.2:
	.asciz "Unexpected GOT_FUNC relocation in .rela.dyn"
	.type .str.2, @object
	.size .str.2, 44

.str.3:
	.asciz "Unexpected GOT_DATA relocation in .rela.dyn"
	.type .str.3, @object
	.size .str.3, 44

.str.4:
	.asciz "Relocation refers on undefined symbol \'%s\'\n"
	.type .str.4, @object
	.size .str.4, 44

.str.5:
	.asciz "p-code"
	.type .str.5, @object
	.size .str.5, 7

