	.file   "vector.c"
	.text
	.option pic
.PCbegin:
	.global VectorInit
	.type VectorInit, @function

VectorInit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          x0, 0(a0)
	sd          x0, 8(a0)
	sd          x0, 16(a0)
	ret         
.func_end_VectorInit:
	.size VectorInit, .func_end_VectorInit-VectorInit

	.global NewVector
	.type NewVector, @function

NewVector:

	// *** Basic block 0

	.global malloc
	.global VectorInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	li          a0, 24		// 0x18 ASCII \x18
	call        malloc

	// *** Basic block 1

	mv          s1, a0
	mv          a0, s1
	call        VectorInit

	// *** Basic block 2

	mv          a0, s1

	// *** Basic block 3

.NewVector_label_16:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewVector:
	.size NewVector, .func_end_NewVector-NewVector

	.global VectorDestruct
	.type VectorDestruct, @function

VectorDestruct:

	// *** Basic block 0

	.global free
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 0(t0)
	j           free
.func_end_VectorDestruct:
	.size VectorDestruct, .func_end_VectorDestruct-VectorDestruct

	.global VectorDelete
	.type VectorDelete, @function

VectorDelete:

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
	.global VectorDestruct
	.global free
	mv          s1, a0
	call        VectorDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_VectorDelete:
	.size VectorDelete, .func_end_VectorDelete-VectorDelete

	.global VectorDestructWithContents
	.type VectorDestructWithContents, @function

VectorDestructWithContents:

	// *** Basic block 0

	.global free
	.global VectorDestruct
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
	mv          s3, x0
	ld          s4, 8(s1)
	bge         x0, s4, .VectorDestructWithContents_label_43

	// *** Basic block 1

	ld          s5, 0(s1)
	ld          s6, 0(s1)

	// *** Basic block 2

.VectorDestructWithContents_label_25:
	beq         s2, x0, .VectorDestructWithContents_label_33

	// *** Basic block 3

	slli        t0, s3, 3
	add         t0, s5, t0
	ld          a0, 0(t0)
	jalr         x1, s2, 0

	// *** Basic block 4

.VectorDestructWithContents_label_33:
	slli        t0, s3, 3
	add         t0, s6, t0
	ld          a0, 0(t0)
	call        free

	// *** Basic block 5

.VectorDestructWithContents_label_39:
	addi        s3, s3, 1
	bge         s3, s4, .VectorDestructWithContents_label_25

	// *** Basic block 6

.VectorDestructWithContents_label_43:
	mv          a0, s1
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
	j           VectorDestruct
.func_end_VectorDestructWithContents:
	.size VectorDestructWithContents, .func_end_VectorDestructWithContents-VectorDestructWithContents

	.global VectorDeleteWithContents
	.type VectorDeleteWithContents, @function

VectorDeleteWithContents:

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
	.global VectorDestructWithContents
	.global free
	mv          s1, a0
	call        VectorDestructWithContents

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_VectorDeleteWithContents:
	.size VectorDeleteWithContents, .func_end_VectorDeleteWithContents-VectorDeleteWithContents

	.global VectorClear
	.type VectorClear, @function

VectorClear:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          x0, 8(a0)
	ret         
.func_end_VectorClear:
	.size VectorClear, .func_end_VectorClear-VectorClear

	.local  MakeSpace
	.type MakeSpace, @function

MakeSpace:

	// *** Basic block 0

	.global malloc
	.global memset
	.global realloc
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
	ld          t0, 0(s1)
	bne         t0, x0, .MakeSpace_label_39

	// *** Basic block 1

	li          t0, 1		// 0x1 ASCII \x1
	sd          t0, 16(s1)
	ld          t0, 16(s1)
	slli        a0, t0, 3
	call        malloc

	// *** Basic block 2

	sd          a0, 0(s1)
	ld          a0, 0(s1)
	ld          t0, 16(s1)
	slli        a2, t0, 3
	mv          a1, x0
	call        memset

	// *** Basic block 3

.MakeSpace_label_39:
	ld          t0, 8(s1)
	addi        t0, t0, 1
	ld          s2, 16(s1)
	bge         s2, t0, .MakeSpace_label_72

	// *** Basic block 4

	ld          t0, 16(s1)
	slli        t1, t0, 1
	sd          t1, 16(s1)
	ld          a0, 0(s1)
	slli        a1, t0, 3
	call        realloc

	// *** Basic block 5

	sd          a0, 0(s1)
	ld          t0, 0(s1)
	slli        t1, s2, 3
	add         a0, t0, t1
	ld          t0, 16(s1)
	sub         t0, t0, s2
	slli        a2, t0, 3
	mv          a1, x0
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           memset

	// *** Basic block 6

.MakeSpace_label_72:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_MakeSpace:
	.size MakeSpace, .func_end_MakeSpace-MakeSpace

	.global VectorReserve
	.type VectorReserve, @function

VectorReserve:

	// *** Basic block 0

	.global malloc
	.global memset
	.global realloc
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
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	ld          t0, 0(s1)
	bne         t0, x0, .VectorReserve_label_38

	// *** Basic block 1

	sd          s2, 16(s1)
	ld          t0, 16(s1)
	slli        a0, t0, 3
	call        malloc

	// *** Basic block 2

	sd          a0, 0(s1)
	ld          a0, 0(s1)
	ld          t0, 16(s1)
	slli        a2, t0, 3
	mv          a1, x0
	call        memset

	// *** Basic block 3

.VectorReserve_label_38:
	ld          s3, 16(s1)
	bge         s2, s3, .VectorReserve_label_47

	// *** Basic block 4

.VectorReserve_label_44:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.VectorReserve_label_47:
	sd          s2, 16(s1)
	ld          a0, 0(s1)
	ld          t0, 16(s1)
	slli        a1, t0, 3
	call        realloc

	// *** Basic block 6

	sd          a0, 0(s1)
	ld          t0, 0(s1)
	slli        t1, s3, 3
	add         a0, t0, t1
	ld          t0, 16(s1)
	sub         t0, t0, s3
	slli        a2, t0, 3
	mv          a1, x0
	call        memset

	// *** Basic block 7

	j           .VectorReserve_label_44
.func_end_VectorReserve:
	.size VectorReserve, .func_end_VectorReserve-VectorReserve

	.global VectorAppend
	.type VectorAppend, @function

VectorAppend:

	// *** Basic block 0

	.local MakeSpace
	// Leaf procedure, no stack frame generated
	j           MakeSpace
.func_end_VectorAppend:
	.size VectorAppend, .func_end_VectorAppend-VectorAppend

	.global VectorSet
	.type VectorSet, @function

VectorSet:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 0(a0)
	slli        t1, a1, 3
	add         t0, t0, t1
	sd          a2, 0(t0)
	ret         
.func_end_VectorSet:
	.size VectorSet, .func_end_VectorSet-VectorSet

	.global VectorGet
	.type VectorGet, @function

VectorGet:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 0(a0)
	slli        t1, a1, 3
	add         t0, t0, t1
	ld          a0, 0(t0)

	// *** Basic block 1

.VectorGet_label_17:
	ret         
.func_end_VectorGet:
	.size VectorGet, .func_end_VectorGet-VectorGet

	.global VectorFirst
	.type VectorFirst, @function

VectorFirst:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 8(t0)
	bnez        t1, .VectorFirst_label_16

	// *** Basic block 1

	j           .VectorFirst_label_19

	// *** Basic block 2

.VectorFirst_label_16:
	ld          t1, 0(t0)
	ld          a0, 0(t1)

	// *** Basic block 3

.VectorFirst_label_19:

	// *** Basic block 4

.VectorFirst_label_21:
	ret         
.func_end_VectorFirst:
	.size VectorFirst, .func_end_VectorFirst-VectorFirst

	.global VectorLast
	.type VectorLast, @function

VectorLast:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 8(t0)
	bnez        t1, .VectorLast_label_17

	// *** Basic block 1

	j           .VectorLast_label_25

	// *** Basic block 2

.VectorLast_label_17:
	ld          t2, 0(t0)
	addi        t1, t1, -1
	slli        t1, t1, 3
	add         t1, t2, t1
	ld          a0, 0(t1)

	// *** Basic block 3

.VectorLast_label_25:

	// *** Basic block 4

.VectorLast_label_27:
	ret         
.func_end_VectorLast:
	.size VectorLast, .func_end_VectorLast-VectorLast

	.global VectorCopy
	.type VectorCopy, @function

VectorCopy:

	// *** Basic block 0

	.global VectorClear
	.global VectorAppend
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
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	call        VectorClear

	// *** Basic block 1

	mv          s3, x0
	ld          s4, 8(s2)
	bge         x0, s4, .VectorCopy_label_38

	// *** Basic block 2

	ld          s5, 0(s2)

	// *** Basic block 3

.VectorCopy_label_25:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 4

.VectorCopy_label_34:
	addi        s3, s3, 1
	bge         s3, s4, .VectorCopy_label_25

	// *** Basic block 5

.VectorCopy_label_38:
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_VectorCopy:
	.size VectorCopy, .func_end_VectorCopy-VectorCopy

	.global VectorAppendVector
	.type VectorAppendVector, @function

VectorAppendVector:

	// *** Basic block 0

	.global VectorAppend
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
	mv          t0, a1
	mv          s1, a0
	mv          s2, x0
	ld          s3, 8(t0)
	bge         x0, s3, .VectorAppendVector_label_34

	// *** Basic block 1

	ld          s4, 0(t0)

	// *** Basic block 2

.VectorAppendVector_label_21:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 3

.VectorAppendVector_label_30:
	addi        s2, s2, 1
	bge         s2, s3, .VectorAppendVector_label_21

	// *** Basic block 4

.VectorAppendVector_label_34:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_VectorAppendVector:
	.size VectorAppendVector, .func_end_VectorAppendVector-VectorAppendVector

	.global VectorPush
	.type VectorPush, @function

VectorPush:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global VectorAppend
	j           VectorAppend
.func_end_VectorPush:
	.size VectorPush, .func_end_VectorPush-VectorPush

	.global VectorPop
	.type VectorPop, @function

VectorPop:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 8(t0)
	bge         x0, t1, .VectorPop_label_18

	// *** Basic block 1

	ld          t1, 8(t0)
	addi        t1, t1, -1
	sd          t1, 8(t0)

	// *** Basic block 2

.VectorPop_label_18:
	ret         
.func_end_VectorPop:
	.size VectorPop, .func_end_VectorPop-VectorPop

	.global VectorEqual
	.type VectorEqual, @function

VectorEqual:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          t2, 8(t0)
	ld          t3, 8(t1)
	beq         t2, t3, .VectorEqual_label_28

	// *** Basic block 1

	ld          t3, 0(t0)
	ld          t4, 0(t1)
	mv          a0, x0

	// *** Basic block 2

.VectorEqual_label_25:
	ret         

	// *** Basic block 3

.VectorEqual_label_28:
	mv          t3, x0
	bge         x0, t2, .VectorEqual_label_51

	// *** Basic block 4

.VectorEqual_label_33:
	slli        t4, t3, 3
	add         t5, t3, t4
	ld          t5, 0(t5)
	add         t4, t4, t4
	ld          t4, 0(t4)
	beq         t5, t4, .VectorEqual_label_46

	// *** Basic block 5

	mv          a0, x0
	ret         

	// *** Basic block 6

.VectorEqual_label_46:

	// *** Basic block 7

.VectorEqual_label_47:
	addi        t3, t3, 1
	bge         t3, t2, .VectorEqual_label_33

	// *** Basic block 8

.VectorEqual_label_51:
	li          a0, 1		// 0x1 ASCII \x1
	ret         
.func_end_VectorEqual:
	.size VectorEqual, .func_end_VectorEqual-VectorEqual

	.global VectorInsertBefore
	.type VectorInsertBefore, @function

VectorInsertBefore:

	// *** Basic block 0

	.global printf
	.global abort
	.local MakeSpace
	.global memmove
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
	mv          s1, a1
	mv          s2, a0
	ld          s3, 8(s2)
	bge         s1, s3, .VectorInsertBefore_label_29

	// *** Basic block 1

	j           .VectorInsertBefore_label_45

	// *** Basic block 2

.VectorInsertBefore_label_29:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 150		// 0x96 ASCII \x96
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.VectorInsertBefore_label_45:
	mv          a0, s2
	call        MakeSpace

	// *** Basic block 5

	sub         s4, s3, s1
	ld          t0, 0(s2)
	slli        t1, s1, 3
	add         a1, t0, t1
	addi        a0, a1, 8
	slli        a2, s4, 3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           memmove
.func_end_VectorInsertBefore:
	.size VectorInsertBefore, .func_end_VectorInsertBefore-VectorInsertBefore

	.global VectorInsertAfter
	.type VectorInsertAfter, @function

VectorInsertAfter:

	// *** Basic block 0

	.global printf
	.global abort
	.global VectorAppend
	.local MakeSpace
	.global memmove
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
	mv          s1, a1
	mv          s2, a0
	mv          s3, a2
	ld          s4, 8(s2)
	bge         s1, s4, .VectorInsertAfter_label_31

	// *** Basic block 1

	j           .VectorInsertAfter_label_47

	// *** Basic block 2

.VectorInsertAfter_label_31:
	lla         a0, .str.4
	lla         a1, .str.5
	lla         a3, .str.6
	li          t0, 160		// 0xa0 ASCII \xa0
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.VectorInsertAfter_label_47:
	addi        t0, s4, -1
	bne         s1, t0, .VectorInsertAfter_label_62

	// *** Basic block 5

	mv          a1, s3
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorAppend

	// *** Basic block 6

.VectorInsertAfter_label_59:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.VectorInsertAfter_label_62:
	mv          a0, s2
	call        MakeSpace

	// *** Basic block 8

	sub         t0, s4, s1
	addi        s4, t0, -1
	ld          t0, 0(s2)
	slli        t1, s1, 3
	add         t0, t0, t1
	addi        a0, t0, 16
	addi        a1, t0, 8
	slli        a2, s4, 3
	call        memmove

	// *** Basic block 9

	ld          t0, 0(s2)
	addi        t1, s1, 1
	slli        t1, t1, 3
	add         t0, t0, t1
	sd          s3, 0(t0)
	ld          t0, 8(s2)
	addi        t0, t0, 1
	sd          t0, 8(s2)
	j           .VectorInsertAfter_label_59
.func_end_VectorInsertAfter:
	.size VectorInsertAfter, .func_end_VectorInsertAfter-VectorInsertAfter

	.global VectorDeleteElement
	.type VectorDeleteElement, @function

VectorDeleteElement:

	// *** Basic block 0

	.global printf
	.global abort
	.global memmove
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
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	ld          s3, 8(s2)
	bge         s1, s3, .VectorDeleteElement_label_24

	// *** Basic block 1

	j           .VectorDeleteElement_label_41

	// *** Basic block 2

.VectorDeleteElement_label_24:
	lla         a0, .str.7
	lla         a1, .str.8
	lla         a3, .str.9
	li          t0, 176		// 0xb0 ASCII \xb0
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.VectorDeleteElement_label_41:
	sub         t0, s3, s1
	addi        s3, t0, -1
	ld          t0, 0(s2)
	slli        t1, s1, 3
	add         a0, t0, t1
	addi        a1, a0, 8
	slli        a2, s3, 3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           memmove
.func_end_VectorDeleteElement:
	.size VectorDeleteElement, .func_end_VectorDeleteElement-VectorDeleteElement

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.1, @object
	.size .str.1, 30

.str.2:
	.asciz "vector.c"
	.type .str.2, @object
	.size .str.2, 9

.str.3:
	.asciz "index < vec->length"
	.type .str.3, @object
	.size .str.3, 20

.str.4:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.4, @object
	.size .str.4, 30

.str.5:
	.asciz "vector.c"
	.type .str.5, @object
	.size .str.5, 9

.str.6:
	.asciz "index < vec->length"
	.type .str.6, @object
	.size .str.6, 20

.str.7:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.7, @object
	.size .str.7, 30

.str.8:
	.asciz "vector.c"
	.type .str.8, @object
	.size .str.8, 9

.str.9:
	.asciz "index < vec->length"
	.type .str.9, @object
	.size .str.9, 20

