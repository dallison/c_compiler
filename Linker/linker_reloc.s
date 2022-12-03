	.file   "linker_reloc.c"
	.text
	.option pic
.PCbegin:
	.global NewRelocation
	.type NewRelocation, @function

NewRelocation:

	// *** Basic block 0

	.global malloc
	.global StringInit
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a2
	mv          s3, a3
	mv          s4, a4
	mv          s5, a1
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s6, a0
	mv          a1, s1
	mv          a0, s6
	call        StringInit

	// *** Basic block 2

	sd          x0, 40(s6)
	sd          s2, 64(s6)
	sw          s3, 56(s6)
	sd          s4, 72(s6)
	sd          s5, 48(s6)
	mv          a0, s6

	// *** Basic block 3

.NewRelocation_label_49:
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld s6, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewRelocation:
	.size NewRelocation, .func_end_NewRelocation-NewRelocation

	.global NewSymbolRelocation
	.type NewSymbolRelocation, @function

NewSymbolRelocation:

	// *** Basic block 0

	.global malloc
	.global StringInit
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	addi        t0, s1, 8
	ld          a1, 16(t0)
	mv          a0, s4
	call        StringInit

	// *** Basic block 2

	sd          s1, 40(s4)
	sd          s2, 64(s4)
	sw          s3, 56(s4)
	sd          x0, 72(s4)
	sd          x0, 48(s4)
	mv          a0, s4

	// *** Basic block 3

.NewSymbolRelocation_label_48:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewSymbolRelocation:
	.size NewSymbolRelocation, .func_end_NewSymbolRelocation-NewSymbolRelocation

	.global NewRelativeRelocation
	.type NewRelativeRelocation, @function

NewRelativeRelocation:

	// *** Basic block 0

	.global malloc
	.global StringInit
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a2
	mv          s3, a1
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	mv          a1, x0
	mv          a0, s4
	call        StringInit

	// *** Basic block 2

	sd          x0, 40(s4)
	sd          s1, 64(s4)
	sw          s2, 56(s4)
	sd          x0, 72(s4)
	sd          s3, 48(s4)
	mv          a0, s4

	// *** Basic block 3

.NewRelativeRelocation_label_44:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewRelativeRelocation:
	.size NewRelativeRelocation, .func_end_NewRelativeRelocation-NewRelativeRelocation

	.global RelocationDestruct
	.type RelocationDestruct, @function

RelocationDestruct:

	// *** Basic block 0

	.global StringDestruct
	// Leaf procedure, no stack frame generated
	j           StringDestruct
.func_end_RelocationDestruct:
	.size RelocationDestruct, .func_end_RelocationDestruct-RelocationDestruct

	.global RelocationDelete
	.type RelocationDelete, @function

RelocationDelete:

	// *** Basic block 0

	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	.global RelocationDestruct
	.global free
	mv          s1, a0
	call        RelocationDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_RelocationDelete:
	.size RelocationDelete, .func_end_RelocationDelete-RelocationDelete

	.global LinkerReadRelocation
	.type LinkerReadRelocation, @function

LinkerReadRelocation:

	// *** Basic block 0

	.global LinkerWarning
	.global NewRelocation
	.global VectorAppend
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 72(sp)
	sd s2, 64(sp)
	sd s3, 56(sp)
	sd s4, 48(sp)
	sd s5, 40(sp)
	sd s6, 32(sp)
	sd s7, 24(sp)
	sd s8, 16(sp)
	sd s9, 8(sp)
	// End of stack frame
	mv          s1, a3
	mv          s2, a5
	mv          t0, a4
	mv          t1, a6
	mv          s3, a2
	mv          s4, a1
	mv          s5, a7
	ld          t2, 8(s1)
	srli        t3, t2, 32
	li          t4, 4294967295		// 0xffffffff
	and         s6, t2, t4
	ld          t2, 0(s2)
	lwu         t4, 4(t2)
	li          t5, 4		// 0x4 ASCII \x4
	bne         t4, t5, .LinkerReadRelocation_label_60

	// *** Basic block 1

	ld          s7, 16(s1)
	j           .LinkerReadRelocation_label_62

	// *** Basic block 2

.LinkerReadRelocation_label_60:

	// *** Basic block 3

.LinkerReadRelocation_label_62:
	ld          t4, 0(t1)
	ld          t4, 56(t4)
	mul         t3, t3, t4
	add         s8, t0, t3
	lwu         s9, 44(t2)
	sltz        t2, s9
	blt         s9, x0, .LinkerReadRelocation_label_81

	// *** Basic block 4

	addi        t0, s3, 56
	ld          t0, 8(t0)
	slt         t0, s9, t0
	not         t2, t0

	// *** Basic block 5

.LinkerReadRelocation_label_81:
	beqz        t2, .LinkerReadRelocation_label_103

	// *** Basic block 6

	lla         a1, .str.1
	lla         a2, .str.2
	addi        t0, s2, 8
	ld          a3, 16(t0)
	mv          a4, s9
	mv          a0, s4
	// Restored registers.
	ld s1, 72(sp)
	ld s2, 64(sp)
	ld s3, 56(sp)
	ld s4, 48(sp)
	ld s5, 40(sp)
	ld s6, 32(sp)
	ld s7, 24(sp)
	ld s8, 16(sp)
	ld s9, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LinkerWarning

	// *** Basic block 7

.LinkerReadRelocation_label_100:
	// Restored registers.
	ld s1, 72(sp)
	ld s2, 64(sp)
	ld s3, 56(sp)
	ld s4, 48(sp)
	ld s5, 40(sp)
	ld s6, 32(sp)
	ld s7, 24(sp)
	ld s8, 16(sp)
	ld s9, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 8

.LinkerReadRelocation_label_103:
	ld          t0, 56(s3)
	slli        t1, s9, 3
	add         t0, t0, t1
	ld          s9, 0(t0)
	ld          t0, 48(s5)
	lwu         t1, 0(s8)
	add         a0, t0, t1
	ld          a2, 0(s1)
	mv          a4, s7
	mv          a3, s6
	mv          a1, s9
	call        NewRelocation

	// *** Basic block 9

	mv          s6, a0
	addi        a0, s4, 136
	mv          a1, s6
	call        VectorAppend

	// *** Basic block 10

	j           .LinkerReadRelocation_label_100
.func_end_LinkerReadRelocation:
	.size LinkerReadRelocation, .func_end_LinkerReadRelocation-LinkerReadRelocation

	.local  ApplyRelocation
	.type ApplyRelocation, @function

ApplyRelocation:

	// *** Basic block 0

	.global ObjectFileFindSymbol
	.global LinkerError
	.global printf
	.global abort
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 72(sp)
	sd s2, 64(sp)
	sd s3, 56(sp)
	sd s4, 48(sp)
	sd s5, 40(sp)
	sd s6, 32(sp)
	sd s7, 24(sp)
	sd s8, 16(sp)
	sd s9, 8(sp)
	// End of stack frame
	mv          s1, a2
	mv          s2, a1
	mv          s3, a0
	ld          s4, 48(s1)
	bne         s4, x0, .ApplyRelocation_label_42

	// *** Basic block 1

.ApplyRelocation_label_39:
	// Restored registers.
	ld s1, 72(sp)
	ld s2, 64(sp)
	ld s3, 56(sp)
	ld s4, 48(sp)
	ld s5, 40(sp)
	ld s6, 32(sp)
	ld s7, 24(sp)
	ld s8, 16(sp)
	ld s9, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.ApplyRelocation_label_42:
	ld          s5, 16(s1)
	mv          a1, s5
	mv          a0, s2
	call        ObjectFileFindSymbol

	// *** Basic block 3

	mv          s6, a0
	bne         s6, x0, .ApplyRelocation_label_64

	// *** Basic block 4

	lla         a1, .str.3
	mv          a2, s5
	mv          a0, s2
	call        LinkerError

	// *** Basic block 5

	j           .ApplyRelocation_label_39

	// *** Basic block 6

.ApplyRelocation_label_64:
	ld          t0, 48(s4)
	ld          s4, 64(s1)
	add         s7, t0, s4
	ld          s8, 80(s6)
	ld          s9, 72(s1)
	lb          t0, 585(s3)
	beqz        t0, .ApplyRelocation_label_95

	// *** Basic block 7

	lla         a0, .str.4
	lw          a1, 56(s1)
	mv          a4, s4
	mv          a3, s8
	mv          a2, s5
	call        printf

	// *** Basic block 8

.ApplyRelocation_label_95:
	ld          s4, 88(s3)
	beq         s4, x0, .ApplyRelocation_label_102

	// *** Basic block 9

	j           .ApplyRelocation_label_117

	// *** Basic block 10

.ApplyRelocation_label_102:
	lla         a0, .str.5
	lla         a1, .str.6
	lla         a3, .str.7
	li          t0, 143		// 0x8f ASCII \x8f
	mv          a2, t0
	call        printf

	// *** Basic block 11

	call        abort

	// *** Basic block 12

.ApplyRelocation_label_117:
	ld          t0, 40(s4)
	mv          a6, s9
	mv          a5, s8
	mv          a4, s7
	mv          a3, s6
	mv          a2, s1
	mv          a1, s2
	mv          a0, s3
	jalr         x1, t0, 0

	// *** Basic block 13

	j           .ApplyRelocation_label_39
.func_end_ApplyRelocation:
	.size ApplyRelocation, .func_end_ApplyRelocation-ApplyRelocation

	.global LinkerApplyAllRelocations
	.type LinkerApplyAllRelocations, @function

LinkerApplyAllRelocations:

	// *** Basic block 0

	.local ApplyRelocation
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 56(sp)
	sd s2, 48(sp)
	sd s3, 40(sp)
	sd s4, 32(sp)
	sd s5, 24(sp)
	sd s6, 16(sp)
	sd s7, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, x0
	addi        t0, s1, 40
	ld          s3, 8(t0)
	bge         x0, s3, .LinkerApplyAllRelocations_label_56

	// *** Basic block 1

	ld          t0, 40(s1)

	// *** Basic block 2

.LinkerApplyAllRelocations_label_21:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	mv          s5, x0
	addi        t0, s4, 136
	ld          s6, 8(t0)
	bge         x0, s6, .LinkerApplyAllRelocations_label_51

	// *** Basic block 3

	ld          s7, 136(s4)

	// *** Basic block 4

.LinkerApplyAllRelocations_label_35:
	slli        t0, s5, 3
	add         t0, s7, t0
	ld          a2, 0(t0)
	mv          a1, s4
	mv          a0, s1
	call        ApplyRelocation

	// *** Basic block 5

.LinkerApplyAllRelocations_label_47:
	addi        s5, s5, 1
	bge         s5, s6, .LinkerApplyAllRelocations_label_35

	// *** Basic block 6

.LinkerApplyAllRelocations_label_51:

	// *** Basic block 7

.LinkerApplyAllRelocations_label_52:
	addi        s2, s2, 1
	bge         s2, s3, .LinkerApplyAllRelocations_label_21

	// *** Basic block 8

.LinkerApplyAllRelocations_label_56:
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LinkerApplyAllRelocations:
	.size LinkerApplyAllRelocations, .func_end_LinkerApplyAllRelocations-LinkerApplyAllRelocations

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "bad-rel-section"
	.type .str.1, @object
	.size .str.1, 16

.str.2:
	.asciz "Bad info in relocation section %s (info: %d)"
	.type .str.2, @object
	.size .str.2, 45

.str.3:
	.asciz "Undefined symbol %s used in relocation"
	.type .str.3, @object
	.size .str.3, 39

.str.4:
	.asciz "Applying relocation type %d for symbol %s(0x%llx) to offset %lld\n"
	.type .str.4, @object
	.size .str.4, 66

.str.5:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.5, @object
	.size .str.5, 30

.str.6:
	.asciz "linker_reloc.c"
	.type .str.6, @object
	.size .str.6, 15

.str.7:
	.asciz "linker->arch != NULL"
	.type .str.7, @object
	.size .str.7, 21

