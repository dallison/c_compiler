	.file   "bitset.c"
	.text
	.option pic
.PCbegin:
	.global BitSetInit
	.type BitSetInit, @function

BitSetInit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          x0, 0(a0)
	sd          x0, 8(a0)
	ret         
.func_end_BitSetInit:
	.size BitSetInit, .func_end_BitSetInit-BitSetInit

	.global NewBitSet
	.type NewBitSet, @function

NewBitSet:

	// *** Basic block 0

	.global malloc
	.global BitSetInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	li          a0, 16		// 0x10 ASCII \x10
	call        malloc

	// *** Basic block 1

	mv          s1, a0
	mv          a0, s1
	call        BitSetInit

	// *** Basic block 2

	mv          a0, s1

	// *** Basic block 3

.NewBitSet_label_16:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewBitSet:
	.size NewBitSet, .func_end_NewBitSet-NewBitSet

	.global BitSetDestruct
	.type BitSetDestruct, @function

BitSetDestruct:

	// *** Basic block 0

	.global free
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 0(t0)
	j           free
.func_end_BitSetDestruct:
	.size BitSetDestruct, .func_end_BitSetDestruct-BitSetDestruct

	.global BitSetDelete
	.type BitSetDelete, @function

BitSetDelete:

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
	.global BitSetDestruct
	.global free
	mv          s1, a0
	call        BitSetDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_BitSetDelete:
	.size BitSetDelete, .func_end_BitSetDelete-BitSetDelete

	.global BitSetClear
	.type BitSetClear, @function

BitSetClear:

	// *** Basic block 0

	.global memset
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
	ld          s2, 0(s1)
	bne         s2, x0, .BitSetClear_label_17

	// *** Basic block 1

.BitSetClear_label_14:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.BitSetClear_label_17:
	ld          t0, 8(s1)
	slli        a2, t0, 2
	mv          a1, x0
	mv          a0, s2
	call        memset

	// *** Basic block 3

	j           .BitSetClear_label_14
.func_end_BitSetClear:
	.size BitSetClear, .func_end_BitSetClear-BitSetClear

	.local  CalculateIndexes
	.type CalculateIndexes, @function

CalculateIndexes:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	srai        t0, a0, 5
	sd          t0, 0(a1)
	andi        t0, a0, 31
	sd          t0, 0(a2)
	ret         
.func_end_CalculateIndexes:
	.size CalculateIndexes, .func_end_CalculateIndexes-CalculateIndexes

	.local  MakeRoomFor
	.type MakeRoomFor, @function

MakeRoomFor:

	// *** Basic block 0

	.global calloc
	.global realloc
	.global memset
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
	addi        t0, a1, 32
	srai        s2, t0, 5
	ld          s3, 0(s1)
	bne         s3, x0, .MakeRoomFor_label_35

	// *** Basic block 1

	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	mv          a0, s2
	call        calloc

	// *** Basic block 2

	sd          a0, 0(s1)
	sd          s2, 8(s1)
	j           .MakeRoomFor_label_65

	// *** Basic block 3

.MakeRoomFor_label_35:
	ld          t0, 8(s1)
	bge         t0, s2, .MakeRoomFor_label_64

	// *** Basic block 4

	slli        a1, s2, 2
	mv          a0, s3
	call        realloc

	// *** Basic block 5

	sd          a0, 0(s1)
	ld          t0, 0(s1)
	ld          t1, 8(s1)
	slli        t1, t1, 2
	add         a0, t0, t1
	ld          t0, 8(s1)
	sub         t0, s2, t0
	slli        a2, t0, 2
	mv          a1, x0
	call        memset

	// *** Basic block 6

	sd          s2, 8(s1)

	// *** Basic block 7

.MakeRoomFor_label_64:

	// *** Basic block 8

.MakeRoomFor_label_65:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_MakeRoomFor:
	.size MakeRoomFor, .func_end_MakeRoomFor-MakeRoomFor

	.local  MakeRoom
	.type MakeRoom, @function

MakeRoom:

	// *** Basic block 0

	.global calloc
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
	ld          s3, 0(s1)
	bne         s3, x0, .MakeRoom_label_28

	// *** Basic block 1

	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	mv          a0, s2
	call        calloc

	// *** Basic block 2

	sd          a0, 0(s1)
	sd          s2, 8(s1)
	j           .MakeRoom_label_43

	// *** Basic block 3

.MakeRoom_label_28:
	ld          t0, 8(s1)
	bge         t0, s2, .MakeRoom_label_42

	// *** Basic block 4

	slli        a1, s2, 2
	mv          a0, s3
	call        realloc

	// *** Basic block 5

	sd          a0, 0(s1)
	sd          s2, 8(s1)

	// *** Basic block 6

.MakeRoom_label_42:

	// *** Basic block 7

.MakeRoom_label_43:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_MakeRoom:
	.size MakeRoom, .func_end_MakeRoom-MakeRoom

	.global BitSetInsert
	.type BitSetInsert, @function

BitSetInsert:

	// *** Basic block 0

	.local MakeRoomFor
	.local CalculateIndexes
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
	mv          s2, a1
	call        MakeRoomFor

	// *** Basic block 1

	addi        a1, s0, -32
	addi        a2, s0, -24
	mv          a0, s2
	call        CalculateIndexes

	// *** Basic block 2

	ld          t0, -24(s0)
	li          t1, 1		// 0x1 ASCII \x1
	sll         t0, t1, t0
	ld          t1, 0(s1)
	ld          t2, -32(s0)
	slli        t2, t2, 2
	add         t1, t1, t2
	lwu         t2, 0(t1)
	or          t0, t2, t0
	sw          t0, 0(t1)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BitSetInsert:
	.size BitSetInsert, .func_end_BitSetInsert-BitSetInsert

	.global BitSetContains
	.type BitSetContains, @function

BitSetContains:

	// *** Basic block 0

	.local CalculateIndexes
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          t0, a1
	mv          s1, a0
	addi        a1, s0, -32
	addi        a2, s0, -24
	mv          a0, t0
	call        CalculateIndexes

	// *** Basic block 1

	ld          t0, -32(s0)
	ld          t1, 8(s1)
	blt         t0, t1, .BitSetContains_label_36

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.BitSetContains_label_33:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.BitSetContains_label_36:
	ld          t1, 0(s1)
	slli        t0, t0, 2
	add         t0, t1, t0
	lwu         t0, 0(t0)
	ld          t1, -24(s0)
	li          t2, 1		// 0x1 ASCII \x1
	sll         t1, t2, t1
	and         t0, t0, t1
	snez        a0, t0
	j           .BitSetContains_label_33
.func_end_BitSetContains:
	.size BitSetContains, .func_end_BitSetContains-BitSetContains

	.global BitSetRemove
	.type BitSetRemove, @function

BitSetRemove:

	// *** Basic block 0

	.local MakeRoomFor
	.local CalculateIndexes
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
	mv          s2, a1
	call        MakeRoomFor

	// *** Basic block 1

	addi        a1, s0, -32
	addi        a2, s0, -24
	mv          a0, s2
	call        CalculateIndexes

	// *** Basic block 2

	ld          t0, -24(s0)
	li          t1, 1		// 0x1 ASCII \x1
	sll         t0, t1, t0
	not         t0, t0
	ld          t1, 0(s1)
	ld          t2, -32(s0)
	slli        t2, t2, 2
	add         t1, t1, t2
	lwu         t2, 0(t1)
	and         t0, t2, t0
	sw          t0, 0(t1)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BitSetRemove:
	.size BitSetRemove, .func_end_BitSetRemove-BitSetRemove

	.global BitSetIntersection
	.type BitSetIntersection, @function

BitSetIntersection:

	// *** Basic block 0

	.local MakeRoom
	.global memcpy
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
	mv          s3, a2
	ld          s4, 8(s1)
	ld          s4, 8(s2)
	bge         s4, s4, .BitSetIntersection_label_28

	// *** Basic block 1

	ld          t0, 0(s2)

	// *** Basic block 2

.BitSetIntersection_label_28:
	bnez        s4, .BitSetIntersection_label_34

	// *** Basic block 3

.BitSetIntersection_label_31:
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

	// *** Basic block 4

.BitSetIntersection_label_34:
	mv          a1, s4
	mv          a0, s3
	call        MakeRoom

	// *** Basic block 5

	ld          a0, 0(s3)
	ld          a1, 0(s1)
	slli        a2, s4, 2
	call        memcpy

	// *** Basic block 6

	mv          s5, x0
	slt         t0, x0, s4
	bge         x0, s4, .BitSetIntersection_label_54

	// *** Basic block 7

	slt         t0, x0, s4

	// *** Basic block 8

.BitSetIntersection_label_54:
	beqz        t0, .BitSetIntersection_label_73

	// *** Basic block 9

.BitSetIntersection_label_56:
	slli        t0, s5, 2
	add         t1, t0, t0
	lwu         t1, 0(t1)
	ld          t2, 0(s3)
	add         t0, t2, t0
	lwu         t2, 0(t0)
	and         t1, t2, t1
	sw          t1, 0(t0)

	// *** Basic block 10

.BitSetIntersection_label_65:
	addi        s5, s5, 1
	slt         t0, s5, s4
	bge         s5, s4, .BitSetIntersection_label_71

	// *** Basic block 11

	slt         t0, s5, s4

	// *** Basic block 12

.BitSetIntersection_label_71:
	beqz        t0, .BitSetIntersection_label_56

	// *** Basic block 13

.BitSetIntersection_label_73:
	j           .BitSetIntersection_label_31
.func_end_BitSetIntersection:
	.size BitSetIntersection, .func_end_BitSetIntersection-BitSetIntersection

	.global BitSetUnion
	.type BitSetUnion, @function

BitSetUnion:

	// *** Basic block 0

	.local MakeRoom
	.global memcpy
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
	mv          s3, a2
	ld          s4, 8(s1)
	ld          s4, 8(s2)
	bge         s4, s4, .BitSetUnion_label_28

	// *** Basic block 1

	ld          t0, 0(s2)

	// *** Basic block 2

.BitSetUnion_label_28:
	mv          a1, s4
	mv          a0, s3
	call        MakeRoom

	// *** Basic block 3

	ld          a0, 0(s3)
	ld          a1, 0(s1)
	slli        a2, s4, 2
	call        memcpy

	// *** Basic block 4

	mv          s5, x0
	bge         x0, s4, .BitSetUnion_label_59

	// *** Basic block 5

.BitSetUnion_label_46:
	slli        t0, s5, 2
	add         t1, t0, t0
	lwu         t1, 0(t1)
	ld          t2, 0(s3)
	add         t0, t2, t0
	lwu         t2, 0(t0)
	or          t1, t2, t1
	sw          t1, 0(t0)

	// *** Basic block 6

.BitSetUnion_label_55:
	addi        s5, s5, 1
	bge         s5, s4, .BitSetUnion_label_46

	// *** Basic block 7

.BitSetUnion_label_59:
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
.func_end_BitSetUnion:
	.size BitSetUnion, .func_end_BitSetUnion-BitSetUnion

	.global BitSetUnionInPlace
	.type BitSetUnionInPlace, @function

BitSetUnionInPlace:

	// *** Basic block 0

	.local MakeRoom
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
	ld          s3, 8(s1)
	ld          s3, 8(s2)
	bge         s3, s3, .BitSetUnionInPlace_label_23

	// *** Basic block 1

	ld          t0, 0(s2)

	// *** Basic block 2

.BitSetUnionInPlace_label_23:
	mv          a1, s3
	mv          a0, s1
	call        MakeRoom

	// *** Basic block 3

	mv          s4, x0
	bge         x0, s3, .BitSetUnionInPlace_label_47

	// *** Basic block 4

.BitSetUnionInPlace_label_33:
	slli        t0, s4, 2
	add         t1, t0, t0
	lwu         t1, 0(t1)
	ld          t2, 0(s1)
	add         t0, t2, t0
	lwu         t2, 0(t0)
	or          t1, t2, t1
	sw          t1, 0(t0)

	// *** Basic block 5

.BitSetUnionInPlace_label_43:
	addi        s4, s4, 1
	bge         s4, s3, .BitSetUnionInPlace_label_33

	// *** Basic block 6

.BitSetUnionInPlace_label_47:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BitSetUnionInPlace:
	.size BitSetUnionInPlace, .func_end_BitSetUnionInPlace-BitSetUnionInPlace

	.global BitSetCopy
	.type BitSetCopy, @function

BitSetCopy:

	// *** Basic block 0

	.local MakeRoom
	.global memcpy
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
	ld          s3, 8(s2)
	mv          a1, s3
	call        MakeRoom

	// *** Basic block 1

	ld          a0, 0(s1)
	ld          a1, 0(s2)
	slli        a2, s3, 2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           memcpy
.func_end_BitSetCopy:
	.size BitSetCopy, .func_end_BitSetCopy-BitSetCopy

	.global BitSetEqual
	.type BitSetEqual, @function

BitSetEqual:

	// *** Basic block 0

	.global memcmp
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
	mv          s1, a1
	mv          s2, a0
	mv          s3, s1
	ld          s4, 8(s2)
	ld          s4, 8(s1)
	bge         s4, s4, .BitSetEqual_label_26

	// *** Basic block 1

	mv          s3, s2

	// *** Basic block 2

.BitSetEqual_label_26:
	ld          a0, 0(s2)
	ld          a1, 0(s1)
	slli        a2, s4, 2
	call        memcmp

	// *** Basic block 3

	mv          s5, a0
	beqz        s5, .BitSetEqual_label_45

	// *** Basic block 4

	mv          a0, x0

	// *** Basic block 5

.BitSetEqual_label_42:
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

	// *** Basic block 6

.BitSetEqual_label_45:
	mv          s1, s4
	ld          t0, 8(s3)
	bge         s1, t0, .BitSetEqual_label_67

	// *** Basic block 7

	ld          t1, 0(s3)

	// *** Basic block 8

.BitSetEqual_label_53:
	slli        t2, s1, 2
	add         t1, t1, t2
	lwu         t1, 0(t1)
	beqz        t1, .BitSetEqual_label_62

	// *** Basic block 9

	mv          a0, x0
	j           .BitSetEqual_label_42

	// *** Basic block 10

.BitSetEqual_label_62:

	// *** Basic block 11

.BitSetEqual_label_63:
	addi        s1, s1, 1
	bge         s1, t0, .BitSetEqual_label_53

	// *** Basic block 12

.BitSetEqual_label_67:
	li          a0, 1		// 0x1 ASCII \x1
	j           .BitSetEqual_label_42
.func_end_BitSetEqual:
	.size BitSetEqual, .func_end_BitSetEqual-BitSetEqual

	.global BitSetExpand
	.type BitSetExpand, @function

BitSetExpand:

	// *** Basic block 0

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
	mv          t0, a0
	mv          s1, a1
	mv          s2, x0
	mv          s3, x0
	ld          s4, 8(t0)
	bge         x0, s4, .BitSetExpand_label_58

	// *** Basic block 1

	ld          t1, 0(t0)

	// *** Basic block 2

.BitSetExpand_label_26:
	mv          s5, x0
	slli        t0, s3, 2
	add         t0, t1, t0
	lwu         t0, 0(t0)

	// *** Basic block 3

.BitSetExpand_label_34:
	li          t1, 1		// 0x1 ASCII \x1
	sll         t1, t1, s5
	and         t0, t0, t1
	beqz        t0, .BitSetExpand_label_45

	// *** Basic block 4

	mv          a1, s2
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 5

.BitSetExpand_label_45:
	addi        s2, s2, 1

	// *** Basic block 6

.BitSetExpand_label_47:
	addi        s5, s5, 1
	li          t0, 32		// 0x20 ASCII ' '
	bge         s5, t0, .BitSetExpand_label_34

	// *** Basic block 7

.BitSetExpand_label_53:

	// *** Basic block 8

.BitSetExpand_label_54:
	addi        s3, s3, 1
	bge         s3, s4, .BitSetExpand_label_26

	// *** Basic block 9

.BitSetExpand_label_58:
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
.func_end_BitSetExpand:
	.size BitSetExpand, .func_end_BitSetExpand-BitSetExpand

	.global BitSetCount
	.type BitSetCount, @function

BitSetCount:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, x0
	mv          t2, x0
	ld          t3, 8(t0)
	bge         x0, t3, .BitSetCount_label_49

	// *** Basic block 1

	ld          t4, 0(t0)

	// *** Basic block 2

.BitSetCount_label_22:
	mv          t0, x0
	slli        t5, t2, 2
	add         t4, t4, t5
	lwu         t4, 0(t4)

	// *** Basic block 3

.BitSetCount_label_30:
	li          t5, 1		// 0x1 ASCII \x1
	sll         t5, t5, t0
	and         t4, t4, t5
	beqz        t4, .BitSetCount_label_37

	// *** Basic block 4

	addi        t1, t1, 1

	// *** Basic block 5

.BitSetCount_label_37:

	// *** Basic block 6

.BitSetCount_label_38:
	addi        t0, t0, 1
	li          t4, 32		// 0x20 ASCII ' '
	bge         t0, t4, .BitSetCount_label_30

	// *** Basic block 7

.BitSetCount_label_44:

	// *** Basic block 8

.BitSetCount_label_45:
	addi        t2, t2, 1
	bge         t2, t3, .BitSetCount_label_22

	// *** Basic block 9

.BitSetCount_label_49:
	mv          a0, t1

	// *** Basic block 10

.BitSetCount_label_52:
	ret         
.func_end_BitSetCount:
	.size BitSetCount, .func_end_BitSetCount-BitSetCount

	.global BitSetPrint
	.type BitSetPrint, @function

BitSetPrint:

	// *** Basic block 0

	.global fprintf
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
	mv          s1, a1
	mv          s2, a0
	lla         a1, .str.1
	mv          a0, s1
	call        fprintf

	// *** Basic block 1

	lla         s3, .str.2
	mv          s4, x0
	mv          s5, x0
	ld          s6, 8(s2)
	bge         x0, s6, .BitSetPrint_label_78

	// *** Basic block 2

	ld          t0, 0(s2)

	// *** Basic block 3

.BitSetPrint_label_38:
	mv          s2, x0
	slli        t1, s5, 2
	add         t0, t0, t1
	lwu         t0, 0(t0)

	// *** Basic block 4

.BitSetPrint_label_45:
	li          t1, 1		// 0x1 ASCII \x1
	sll         t1, t1, s2
	and         t0, t0, t1
	beqz        t0, .BitSetPrint_label_65

	// *** Basic block 5

	lla         a1, .str.3
	mv          a3, s4
	mv          a2, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 7

.BitSetPrint_label_65:
	addi        s4, s4, 1

	// *** Basic block 8

.BitSetPrint_label_67:
	addi        s2, s2, 1
	li          t0, 32		// 0x20 ASCII ' '
	bge         s2, t0, .BitSetPrint_label_45

	// *** Basic block 9

.BitSetPrint_label_73:

	// *** Basic block 10

.BitSetPrint_label_74:
	addi        s5, s5, 1
	bge         s5, s6, .BitSetPrint_label_38

	// *** Basic block 11

.BitSetPrint_label_78:
	lla         a1, .str.5
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
	j           fprintf
.func_end_BitSetPrint:
	.size BitSetPrint, .func_end_BitSetPrint-BitSetPrint

	.global BitSetIteratorStart
	.type BitSetIteratorStart, @function

BitSetIteratorStart:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	sd          t1, 0(t0)
	sd          x0, 8(t0)
	sd          x0, 16(t0)
	mv          t2, x0
	mv          t3, x0
	ld          t4, 8(t1)
	bge         x0, t4, .BitSetIteratorStart_label_65

	// *** Basic block 1

	ld          t2, 0(t1)

	// *** Basic block 2

.BitSetIteratorStart_label_31:
	mv          t1, x0
	slli        t5, t3, 2
	add         t2, t2, t5
	lwu         t2, 0(t2)

	// *** Basic block 3

.BitSetIteratorStart_label_39:
	li          t5, 1		// 0x1 ASCII \x1
	sll         t5, t5, t1
	and         t2, t2, t5
	beqz        t2, .BitSetIteratorStart_label_52

	// *** Basic block 4

	sd          t3, 8(t0)
	sd          t1, 16(t0)

	// *** Basic block 5

.BitSetIteratorStart_label_49:
	ret         

	// *** Basic block 6

.BitSetIteratorStart_label_52:

	// *** Basic block 7

.BitSetIteratorStart_label_54:
	addi        t1, t1, 1
	li          t2, 32		// 0x20 ASCII ' '
	bge         t1, t2, .BitSetIteratorStart_label_39

	// *** Basic block 8

.BitSetIteratorStart_label_60:

	// *** Basic block 9

.BitSetIteratorStart_label_61:
	addi        t3, t3, 1
	bge         t3, t4, .BitSetIteratorStart_label_31

	// *** Basic block 10

.BitSetIteratorStart_label_65:
	j           .BitSetIteratorStart_label_49
.func_end_BitSetIteratorStart:
	.size BitSetIteratorStart, .func_end_BitSetIteratorStart-BitSetIteratorStart

	.global BitSetIteratorDone
	.type BitSetIteratorDone, @function

BitSetIteratorDone:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 0(t0)
	ld          t2, 0(t1)
	bne         t2, x0, .BitSetIteratorDone_label_22

	// *** Basic block 1

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 2

.BitSetIteratorDone_label_19:
	ret         

	// *** Basic block 3

.BitSetIteratorDone_label_22:
	ld          t3, 8(t0)
	ld          t1, 8(t1)
	blt         t3, t1, .BitSetIteratorDone_label_33

	// *** Basic block 4

	li          a0, 1		// 0x1 ASCII \x1
	ret         

	// *** Basic block 5

.BitSetIteratorDone_label_33:
	addi        t1, t1, -1
	bge         t3, t1, .BitSetIteratorDone_label_41

	// *** Basic block 6

	mv          a0, x0
	ret         

	// *** Basic block 7

.BitSetIteratorDone_label_41:
	ld          t1, 16(t0)
	li          t4, 1		// 0x1 ASCII \x1
	sll         t1, t4, t1
	addi        t4, t1, -1
	slli        t1, t3, 2
	add         t1, t2, t1
	lwu         t1, 0(t1)
	not         t2, t4
	and         t1, t1, t2
	seqz        a0, t1
	ret         
.func_end_BitSetIteratorDone:
	.size BitSetIteratorDone, .func_end_BitSetIteratorDone-BitSetIteratorDone

	.global BitSetIteratorNext
	.type BitSetIteratorNext, @function

BitSetIteratorNext:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 16(t0)
	addi        t1, t1, 1
	sd          t1, 16(t0)
	ld          t1, 8(t0)
	ld          t2, 0(t0)
	ld          t2, 8(t2)
	bge         t1, t2, .BitSetIteratorNext_label_90

	// *** Basic block 1

.BitSetIteratorNext_label_25:
	ld          t2, 16(t0)
	slti        t1, t2, 32
	li          t3, 32		// 0x20 ASCII ' '
	bge         t2, t3, .BitSetIteratorNext_label_45

	// *** Basic block 2

	ld          t4, 0(t0)
	ld          t4, 0(t4)
	ld          t5, 8(t0)
	slli        t5, t5, 2
	add         t4, t4, t5
	lwu         t4, 0(t4)
	li          t5, 1		// 0x1 ASCII \x1
	sll         t2, t5, t2
	and         t2, t4, t2
	seqz        t1, t2

	// *** Basic block 3

.BitSetIteratorNext_label_45:
	beqz        t1, .BitSetIteratorNext_label_69

	// *** Basic block 4

.BitSetIteratorNext_label_47:
	ld          t1, 16(t0)
	addi        t2, t1, 1
	sd          t2, 16(t0)
	slti        t2, t1, 32
	bge         t1, t3, .BitSetIteratorNext_label_67

	// *** Basic block 5

	ld          t4, 0(t0)
	ld          t4, 0(t4)
	ld          t5, 8(t0)
	slli        t5, t5, 2
	add         t4, t4, t5
	lwu         t4, 0(t4)
	li          t5, 1		// 0x1 ASCII \x1
	sll         t1, t5, t1
	and         t1, t4, t1
	seqz        t2, t1

	// *** Basic block 6

.BitSetIteratorNext_label_67:
	bnez        t2, .BitSetIteratorNext_label_47

	// *** Basic block 7

.BitSetIteratorNext_label_69:
	ld          t1, 16(t0)
	bge         t1, t3, .BitSetIteratorNext_label_78

	// *** Basic block 8

.BitSetIteratorNext_label_75:
	ret         

	// *** Basic block 9

.BitSetIteratorNext_label_78:
	sd          x0, 16(t0)
	ld          t1, 8(t0)
	addi        t2, t1, 1
	sd          t2, 8(t0)
	ld          t2, 0(t0)
	ld          t2, 8(t2)
	blt         t1, t2, .BitSetIteratorNext_label_25

	// *** Basic block 10

.BitSetIteratorNext_label_90:
	j           .BitSetIteratorNext_label_75
.func_end_BitSetIteratorNext:
	.size BitSetIteratorNext, .func_end_BitSetIteratorNext-BitSetIteratorNext

	.global BitSetIteratorValue
	.type BitSetIteratorValue, @function

BitSetIteratorValue:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 8(a0)
	slli        t0, t0, 5
	ld          t1, 16(a0)
	add         a0, t0, t1

	// *** Basic block 1

.BitSetIteratorValue_label_17:
	ret         
.func_end_BitSetIteratorValue:
	.size BitSetIteratorValue, .func_end_BitSetIteratorValue-BitSetIteratorValue

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "{"
	.type .str.1, @object
	.size .str.1, 2

.str.2:
	.asciz "(null)"
	.type .str.2, @object
	.size .str.2, 1

.str.3:
	.asciz "%s%zd"
	.type .str.3, @object
	.size .str.3, 6

.str.4:
	.asciz ", "
	.type .str.4, @object
	.size .str.4, 3

.str.5:
	.asciz "}"
	.type .str.5, @object
	.size .str.5, 2

