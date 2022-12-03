	.file   "list.c"
	.text
	.option pic
.PCbegin:
	.global ListElementInit
	.type ListElementInit, @function

ListElementInit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          x0, 0(a0)
	sd          x0, 8(a0)
	ret         
.func_end_ListElementInit:
	.size ListElementInit, .func_end_ListElementInit-ListElementInit

	.global ListInit
	.type ListInit, @function

ListInit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          x0, 0(a0)
	sd          x0, 8(a0)
	sd          x0, 16(a0)
	ret         
.func_end_ListInit:
	.size ListInit, .func_end_ListInit-ListInit

	.global NewList
	.type NewList, @function

NewList:

	// *** Basic block 0

	.global malloc
	.global ListInit
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
	call        ListInit

	// *** Basic block 2

	mv          a0, s1

	// *** Basic block 3

.NewList_label_16:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewList:
	.size NewList, .func_end_NewList-NewList

	.global ListDestruct
	.type ListDestruct, @function

ListDestruct:

	// *** Basic block 0

	.global free
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
	ld          s2, 0(s1)
	beq         s2, x0, .ListDestruct_label_25

	// *** Basic block 1

.ListDestruct_label_14:
	ld          s3, 8(s2)
	mv          a0, s2
	call        free

	// *** Basic block 2

	mv          s2, s3
	bne         s2, x0, .ListDestruct_label_14

	// *** Basic block 3

.ListDestruct_label_25:
	ld          t0, 8(s1)
	sd          t0, 0(s1)
	sd          x0, 0(t0)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ListDestruct:
	.size ListDestruct, .func_end_ListDestruct-ListDestruct

	.global ListDelete
	.type ListDelete, @function

ListDelete:

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
	.global ListDestruct
	.global free
	mv          s1, a0
	call        ListDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_ListDelete:
	.size ListDelete, .func_end_ListDelete-ListDelete

	.global ListAppend
	.type ListAppend, @function

ListAppend:

	// *** Basic block 0

	.global printf
	.global abort
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
	mv          s1, a1
	mv          s2, a0
	ld          t0, 0(s1)
	bne         t0, x0, .ListAppend_label_28

	// *** Basic block 1

	j           .ListAppend_label_45

	// *** Basic block 2

.ListAppend_label_28:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 46		// 0x2e ASCII '.'
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.ListAppend_label_45:
	ld          t0, 8(s1)
	bne         t0, x0, .ListAppend_label_52

	// *** Basic block 5

	j           .ListAppend_label_67

	// *** Basic block 6

.ListAppend_label_52:
	lla         a0, .str.4
	lla         a1, .str.5
	lla         a3, .str.6
	li          t0, 47		// 0x2f ASCII '/'
	mv          a2, t0
	call        printf

	// *** Basic block 7

	call        abort

	// *** Basic block 8

.ListAppend_label_67:
	ld          t0, 8(s2)
	bne         t0, x0, .ListAppend_label_76

	// *** Basic block 9

	sd          t0, 0(s2)
	sd          s1, 0(t1)
	j           .ListAppend_label_84

	// *** Basic block 10

.ListAppend_label_76:
	sd          t0, 0(s1)
	ld          t0, 8(s2)
	sd          s1, 8(t0)
	sd          s1, 8(s2)

	// *** Basic block 11

.ListAppend_label_84:
	ld          t0, 16(s2)
	addi        t0, t0, 1
	sd          t0, 16(s2)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ListAppend:
	.size ListAppend, .func_end_ListAppend-ListAppend

	.global ListInsertBefore
	.type ListInsertBefore, @function

ListInsertBefore:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a2
	mv          t2, a0
	sd          t1, 8(t0)
	ld          t3, 0(t1)
	bne         t3, x0, .ListInsertBefore_label_24

	// *** Basic block 1

	sd          t0, 0(t2)
	j           .ListInsertBefore_label_28

	// *** Basic block 2

.ListInsertBefore_label_24:
	ld          t3, 0(t1)
	sd          t0, 8(t3)

	// *** Basic block 3

.ListInsertBefore_label_28:
	ld          t3, 0(t1)
	sd          t3, 0(t0)
	sd          t0, 0(t1)
	ld          t3, 16(t2)
	addi        t3, t3, 1
	sd          t3, 16(t2)
	ret         
.func_end_ListInsertBefore:
	.size ListInsertBefore, .func_end_ListInsertBefore-ListInsertBefore

	.global ListInsertAfter
	.type ListInsertAfter, @function

ListInsertAfter:

	// *** Basic block 0

	.global ListAppend
	// Leaf procedure, no stack frame generated
	mv          t0, a2
	mv          t1, a0
	mv          t2, a1
	bne         t0, x0, .ListInsertAfter_label_29

	// *** Basic block 1

	mv          a1, t2
	mv          a0, t1
	j           ListAppend

	// *** Basic block 2

.ListInsertAfter_label_26:
	ret         

	// *** Basic block 3

.ListInsertAfter_label_29:
	sd          t0, 0(t2)
	ld          t3, 8(t0)
	bne         t3, x0, .ListInsertAfter_label_39

	// *** Basic block 4

	sd          t2, 8(t1)
	j           .ListInsertAfter_label_43

	// *** Basic block 5

.ListInsertAfter_label_39:
	ld          t3, 8(t0)
	sd          t2, 0(t3)

	// *** Basic block 6

.ListInsertAfter_label_43:
	ld          t3, 8(t0)
	sd          t3, 8(t2)
	sd          t2, 8(t0)
	ld          t3, 16(t1)
	addi        t3, t3, 1
	sd          t3, 16(t1)
	j           .ListInsertAfter_label_26
.func_end_ListInsertAfter:
	.size ListInsertAfter, .func_end_ListInsertAfter-ListInsertAfter

	.global ListDeleteElement
	.type ListDeleteElement, @function

ListDeleteElement:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	ld          t2, 0(t0)
	bne         t2, x0, .ListDeleteElement_label_21

	// *** Basic block 1

	ld          t2, 8(t0)
	sd          t2, 0(t1)
	j           .ListDeleteElement_label_27

	// *** Basic block 2

.ListDeleteElement_label_21:
	ld          t2, 0(t0)
	ld          t3, 8(t0)
	sd          t3, 8(t2)

	// *** Basic block 3

.ListDeleteElement_label_27:
	ld          t2, 8(t0)
	bne         t2, x0, .ListDeleteElement_label_37

	// *** Basic block 4

	ld          t2, 0(t0)
	sd          t2, 8(t1)
	j           .ListDeleteElement_label_42

	// *** Basic block 5

.ListDeleteElement_label_37:
	ld          t2, 8(t0)
	ld          t3, 0(t0)
	sd          t3, 0(t2)

	// *** Basic block 6

.ListDeleteElement_label_42:
	ld          t2, 16(t1)
	addi        t2, t2, -1
	sd          t2, 16(t1)
	ret         
.func_end_ListDeleteElement:
	.size ListDeleteElement, .func_end_ListDeleteElement-ListDeleteElement

	.global ListTraverse
	.type ListTraverse, @function

ListTraverse:

	// *** Basic block 0

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
	mv          s2, a2
	ld          s3, 0(a0)
	beq         s3, x0, .ListTraverse_label_32

	// *** Basic block 1

.ListTraverse_label_19:
	ld          s4, 8(s3)
	mv          a1, s2
	mv          a0, s3
	jalr         x1, s1, 0

	// *** Basic block 2

	mv          s3, s4
	bne         s3, x0, .ListTraverse_label_19

	// *** Basic block 3

.ListTraverse_label_32:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ListTraverse:
	.size ListTraverse, .func_end_ListTraverse-ListTraverse

	.global ListFind
	.type ListFind, @function

ListFind:

	// *** Basic block 0

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
	mv          s1, a2
	mv          s2, a1
	ld          s3, 0(a0)
	beq         s3, x0, .ListFind_label_40

	// *** Basic block 1

.ListFind_label_19:
	ld          s4, 8(s3)
	mv          a1, s3
	mv          a0, s2
	jalr         x1, s1, 0

	// *** Basic block 2

	bnez        a0, .ListFind_label_35

	// *** Basic block 3

	mv          a0, s3

	// *** Basic block 4

.ListFind_label_32:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.ListFind_label_35:
	mv          s3, s4
	bne         s3, x0, .ListFind_label_19

	// *** Basic block 6

.ListFind_label_40:
	mv          a0, x0
	j           .ListFind_label_32
.func_end_ListFind:
	.size ListFind, .func_end_ListFind-ListFind

	.global ListCopy
	.type ListCopy, @function

ListCopy:

	// *** Basic block 0

	.global ListAppend
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
	mv          s1, a2
	mv          s2, a0
	ld          s3, 0(a1)
	beq         s3, x0, .ListCopy_label_38

	// *** Basic block 1

.ListCopy_label_20:
	ld          s4, 8(s3)
	mv          a0, s3
	jalr         x1, s1, 0

	// *** Basic block 2

	mv          s5, a0
	mv          a1, s5
	mv          a0, s2
	call        ListAppend

	// *** Basic block 3

	mv          s3, s4
	bne         s3, x0, .ListCopy_label_20

	// *** Basic block 4

.ListCopy_label_38:
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
.func_end_ListCopy:
	.size ListCopy, .func_end_ListCopy-ListCopy

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.1, @object
	.size .str.1, 30

.str.2:
	.asciz "list.c"
	.type .str.2, @object
	.size .str.2, 7

.str.3:
	.asciz "e->prev == NULL"
	.type .str.3, @object
	.size .str.3, 16

.str.4:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.4, @object
	.size .str.4, 30

.str.5:
	.asciz "list.c"
	.type .str.5, @object
	.size .str.5, 7

.str.6:
	.asciz "e->next == NULL"
	.type .str.6, @object
	.size .str.6, 16

