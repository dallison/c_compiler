	.file   "set.c"
	.text
	.option pic
.PCbegin:
	.global SetInit
	.type SetInit, @function

SetInit:

	// *** Basic block 0

	.global VectorInit
	// Leaf procedure, no stack frame generated
	j           VectorInit
.func_end_SetInit:
	.size SetInit, .func_end_SetInit-SetInit

	.global NewSet
	.type NewSet, @function

NewSet:

	// *** Basic block 0

	.global malloc
	.global SetInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	mv          s1, a0
	li          a0, 32		// 0x20 ASCII ' '
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        SetInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.NewSet_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewSet:
	.size NewSet, .func_end_NewSet-NewSet

	.global SetDestruct
	.type SetDestruct, @function

SetDestruct:

	// *** Basic block 0

	.global VectorDestruct
	// Leaf procedure, no stack frame generated
	j           VectorDestruct
.func_end_SetDestruct:
	.size SetDestruct, .func_end_SetDestruct-SetDestruct

	.global SetDelete
	.type SetDelete, @function

SetDelete:

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
	.global SetDestruct
	.global free
	mv          s1, a0
	call        SetDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_SetDelete:
	.size SetDelete, .func_end_SetDelete-SetDelete

	.global SetClear
	.type SetClear, @function

SetClear:

	// *** Basic block 0

	.global VectorClear
	// Leaf procedure, no stack frame generated
	j           VectorClear
.func_end_SetClear:
	.size SetClear, .func_end_SetClear-SetClear

	.local  ComparePointers
	.type ComparePointers, @function

ComparePointers:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 0(a0)
	ld          t1, 0(a1)
	sub         t0, t0, t1
	div         t0, t0, x0
	sext.w      a0, t0

	// *** Basic block 1

.ComparePointers_label_19:
	ret         
.func_end_ComparePointers:
	.size ComparePointers, .func_end_ComparePointers-ComparePointers

	.local  CompareIntegers
	.type CompareIntegers, @function

CompareIntegers:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 0(a0)
	lw          t1, 0(a1)
	sub         a0, t0, t1

	// *** Basic block 1

.CompareIntegers_label_15:
	ret         
.func_end_CompareIntegers:
	.size CompareIntegers, .func_end_CompareIntegers-CompareIntegers

	.global SetInitForPointers
	.type SetInitForPointers, @function

SetInitForPointers:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global SetInit
	.local ComparePointers
	la          a1, ComparePointers
	j           SetInit
.func_end_SetInitForPointers:
	.size SetInitForPointers, .func_end_SetInitForPointers-SetInitForPointers

	.global SetInitForIntegers
	.type SetInitForIntegers, @function

SetInitForIntegers:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global SetInit
	.local CompareIntegers
	la          a1, CompareIntegers
	j           SetInit
.func_end_SetInitForIntegers:
	.size SetInitForIntegers, .func_end_SetInitForIntegers-SetInitForIntegers

	.local  FindLocation
	.type FindLocation, @function

FindLocation:

	// *** Basic block 0

	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Saved argument registers.
	sd a1, -24(s0)
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          t0, a2
	mv          s1, a0
	sb          x0, 0(t0)
	mv          s2, x0
	ld          s3, 8(s1)
	bge         x0, s3, .FindLocation_label_64

	// *** Basic block 1

	li          t1, 1		// 0x1 ASCII \x1
	sb          t1, 0(t0)

	// *** Basic block 2

.FindLocation_label_26:
	sub         t0, s3, s2
	srli        t0, t0, 1
	add         s4, s2, t0
	ld          t0, 24(s1)
	addi        a0, s0, -24
	ld          t1, 0(s1)
	slli        s5, s4, 3
	add         a1, t1, s5
	jalr         x1, t0, 0

	// *** Basic block 3

	mv          s6, a0
	bnez        s6, .FindLocation_label_54

	// *** Basic block 4

	ld          t0, 0(s1)
	add         a0, t0, s5

	// *** Basic block 5

.FindLocation_label_51:
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

	// *** Basic block 6

.FindLocation_label_54:
	bge         s6, x0, .FindLocation_label_59

	// *** Basic block 7

	mv          s3, s4
	j           .FindLocation_label_61

	// *** Basic block 8

.FindLocation_label_59:
	addi        s2, s4, 1

	// *** Basic block 9

.FindLocation_label_61:
	blt         s2, s3, .FindLocation_label_26

	// *** Basic block 10

.FindLocation_label_64:
	ld          t1, 8(s1)
	bne         s3, t1, .FindLocation_label_73

	// *** Basic block 11

	mv          a0, x0
	j           .FindLocation_label_51

	// *** Basic block 12

.FindLocation_label_73:
	ld          t1, 0(s1)
	slli        t2, s3, 3
	add         a0, t1, t2
	j           .FindLocation_label_51
.func_end_FindLocation:
	.size FindLocation, .func_end_FindLocation-FindLocation

	.local  LinearInsert
	.type LinearInsert, @function

LinearInsert:

	// *** Basic block 0

	.global VectorInsertBefore
	.global VectorAppend
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Saved argument registers.
	sd a1, -24(s0)
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, x0
	ld          t0, 8(s1)
	bge         x0, t0, .LinearInsert_label_59

	// *** Basic block 1

.LinearInsert_label_19:
	ld          t0, 24(s1)
	addi        a0, s0, -24
	ld          t1, 0(s1)
	slli        t2, s2, 3
	add         a1, t1, t2
	jalr         x1, t0, 0

	// *** Basic block 2

	mv          s3, a0
	bnez        s3, .LinearInsert_label_40

	// *** Basic block 3

.LinearInsert_label_37:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.LinearInsert_label_40:
	bge         s3, x0, .LinearInsert_label_52

	// *** Basic block 5

	ld          a2, -24(s0)
	mv          a1, s2
	mv          a0, s1
	call        VectorInsertBefore

	// *** Basic block 6

	j           .LinearInsert_label_37

	// *** Basic block 7

.LinearInsert_label_52:

	// *** Basic block 8

.LinearInsert_label_53:
	addi        s2, s2, 1
	ld          t0, 8(s1)
	bge         s2, t0, .LinearInsert_label_19

	// *** Basic block 9

.LinearInsert_label_59:
	ld          a1, -24(s0)
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 10

	j           .LinearInsert_label_37
.func_end_LinearInsert:
	.size LinearInsert, .func_end_LinearInsert-LinearInsert

	.local  BinaryInsert
	.type BinaryInsert, @function

BinaryInsert:

	// *** Basic block 0

	.local FindLocation
	.global VectorAppend
	.global VectorInsertBefore
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	addi        a2, s0, -32
	call        FindLocation

	// *** Basic block 1

	mv          s3, a0
	bne         s3, x0, .BinaryInsert_label_37

	// *** Basic block 2

	mv          a1, s2
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 3

.BinaryInsert_label_34:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.BinaryInsert_label_37:
	lb          t0, -32(s0)
	beqz        t0, .BinaryInsert_label_41

	// *** Basic block 5

	j           .BinaryInsert_label_34

	// *** Basic block 6

.BinaryInsert_label_41:
	ld          t0, 0(s1)
	sub         t0, s3, t0
	srai        a1, t0, 3
	mv          a2, s2
	mv          a0, s1
	call        VectorInsertBefore

	// *** Basic block 7

	j           .BinaryInsert_label_34
.func_end_BinaryInsert:
	.size BinaryInsert, .func_end_BinaryInsert-BinaryInsert

	.global SetInsert
	.type SetInsert, @function

SetInsert:

	// *** Basic block 0

	.local LinearInsert
	.local BinaryInsert
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	ld          t0, 8(s1)
	li          t1, 5		// 0x5 ASCII \x5
	bge         t0, t1, .SetInsert_label_26

	// *** Basic block 1

	mv          a1, s2
	mv          a0, s1
	call        LinearInsert

	// *** Basic block 2

	j           .SetInsert_label_33

	// *** Basic block 3

.SetInsert_label_26:
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BinaryInsert

	// *** Basic block 4

.SetInsert_label_33:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SetInsert:
	.size SetInsert, .func_end_SetInsert-SetInsert

	.global SetRemove
	.type SetRemove, @function

SetRemove:

	// *** Basic block 0

	.local FindLocation
	.global VectorDeleteElement
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	mv          s1, a0
	addi        a2, s0, -32
	call        FindLocation

	// *** Basic block 1

	mv          s2, a0
	sub         t1, s2, x0
	seqz        t0, t1
	beq         s2, x0, .SetRemove_label_31

	// *** Basic block 2

	lb          t1, -32(s0)
	not         t0, t1

	// *** Basic block 3

.SetRemove_label_31:
	beqz        t0, .SetRemove_label_36

	// *** Basic block 4

.SetRemove_label_33:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.SetRemove_label_36:
	ld          t0, 0(s1)
	sub         t0, s2, t0
	srai        a1, t0, 3
	mv          a0, s1
	call        VectorDeleteElement

	// *** Basic block 6

	j           .SetRemove_label_33
.func_end_SetRemove:
	.size SetRemove, .func_end_SetRemove-SetRemove

	.global SetContains
	.type SetContains, @function

SetContains:

	// *** Basic block 0

	.global bsearch
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a1, -24(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	mv          t0, a0
	addi        a0, s0, -24
	ld          a1, 0(t0)
	ld          a2, 8(t0)
	ld          a4, 24(t0)
	li          a3, 8		// 0x8 ASCII \x8
	call        bsearch

	// *** Basic block 1

	sub         t0, a0, x0
	snez        a0, t0

	// *** Basic block 2

.SetContains_label_33:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SetContains:
	.size SetContains, .func_end_SetContains-SetContains

	.global SetIntersection
	.type SetIntersection, @function

SetIntersection:

	// *** Basic block 0

	.global SetInsert
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
	mv          s2, a1
	mv          s3, a2
	mv          s4, x0
	mv          s5, x0
	ld          t1, 8(s1)
	slt         t0, x0, t1
	bge         x0, t1, .SetIntersection_label_30

	// *** Basic block 1

	ld          t1, 8(s2)
	slt         t0, x0, t1

	// *** Basic block 2

.SetIntersection_label_30:
	beqz        t0, .SetIntersection_label_76

	// *** Basic block 3

.SetIntersection_label_32:
	ld          t0, 24(s1)
	ld          t1, 0(s1)
	slli        t2, s4, 3
	add         a0, t1, t2
	ld          t1, 0(s2)
	slli        t2, s5, 3
	add         a1, t1, t2
	jalr         x1, t0, 0

	// *** Basic block 4

	mv          s6, a0
	bnez        s6, .SetIntersection_label_57

	// *** Basic block 5

	ld          a1, 0(a0)
	mv          a0, s3
	call        SetInsert

	// *** Basic block 6

	addi        s4, s4, 1
	addi        s5, s5, 1
	j           .SetIntersection_label_65

	// *** Basic block 7

.SetIntersection_label_57:
	bge         s6, x0, .SetIntersection_label_62

	// *** Basic block 8

	addi        s4, s4, 1
	j           .SetIntersection_label_64

	// *** Basic block 9

.SetIntersection_label_62:
	addi        s5, s5, 1

	// *** Basic block 10

.SetIntersection_label_64:

	// *** Basic block 11

.SetIntersection_label_65:
	ld          t1, 8(s1)
	slt         t0, s4, t1
	bge         s4, t1, .SetIntersection_label_74

	// *** Basic block 12

	ld          t1, 8(s2)
	slt         t0, s5, t1

	// *** Basic block 13

.SetIntersection_label_74:
	bnez        t0, .SetIntersection_label_32

	// *** Basic block 14

.SetIntersection_label_76:
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
.func_end_SetIntersection:
	.size SetIntersection, .func_end_SetIntersection-SetIntersection

	.global SetUnion
	.type SetUnion, @function

SetUnion:

	// *** Basic block 0

	.global SetInsert
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
	mv          s2, a2
	mv          s3, a1
	mv          s4, x0
	ld          s5, 8(s1)
	bge         x0, s5, .SetUnion_label_37

	// *** Basic block 1

	ld          s6, 0(s1)

	// *** Basic block 2

.SetUnion_label_24:
	slli        t0, s4, 3
	add         t0, s6, t0
	ld          a1, 0(t0)
	mv          a0, s2
	call        SetInsert

	// *** Basic block 3

.SetUnion_label_33:
	addi        s4, s4, 1
	bge         s4, s5, .SetUnion_label_24

	// *** Basic block 4

.SetUnion_label_37:
	mv          s5, x0
	ld          s6, 8(s3)
	bge         x0, s6, .SetUnion_label_57

	// *** Basic block 5

	ld          s7, 0(s3)

	// *** Basic block 6

.SetUnion_label_45:
	slli        t0, s5, 3
	add         t0, s7, t0
	ld          a1, 0(t0)
	mv          a0, s2
	call        SetInsert

	// *** Basic block 7

.SetUnion_label_53:
	addi        s5, s5, 1
	bge         s5, s6, .SetUnion_label_45

	// *** Basic block 8

.SetUnion_label_57:
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
.func_end_SetUnion:
	.size SetUnion, .func_end_SetUnion-SetUnion

	.global SetCopy
	.type SetCopy, @function

SetCopy:

	// *** Basic block 0

	.global VectorCopy
	// Leaf procedure, no stack frame generated
	j           VectorCopy
.func_end_SetCopy:
	.size SetCopy, .func_end_SetCopy-SetCopy

	.global SetEqual
	.type SetEqual, @function

SetEqual:

	// *** Basic block 0

	.global VectorEqual
	// Leaf procedure, no stack frame generated
	j           VectorEqual
.func_end_SetEqual:
	.size SetEqual, .func_end_SetEqual-SetEqual

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
