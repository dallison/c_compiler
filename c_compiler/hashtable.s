	.file   "hashtable.c"
	.text
	.option pic
.PCbegin:
	.global HashTableInit
	.type HashTableInit, @function

HashTableInit:

	// *** Basic block 0

	.global StringInit
	.global calloc
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
	mv          s2, a2
	call        StringInit

	// *** Basic block 1

	sd          s2, 48(s1)
	sd          x0, 56(s1)
	mv          a1, s2
	li          t0, 8		// 0x8 ASCII \x8
	mv          a0, t0
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           calloc
.func_end_HashTableInit:
	.size HashTableInit, .func_end_HashTableInit-HashTableInit

	.global NewHashTable
	.type NewHashTable, @function

NewHashTable:

	// *** Basic block 0

	.global malloc
	.global HashTableInit
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
	mv          s3, a2
	mv          s4, a3
	mv          s5, a4
	li          a0, 88		// 0x58 ASCII 'X'
	call        malloc

	// *** Basic block 1

	mv          s6, a0
	mv          a5, s5
	mv          a4, s4
	mv          a3, s3
	mv          a2, s2
	mv          a1, s1
	mv          a0, s6
	call        HashTableInit

	// *** Basic block 2

	mv          a0, s6

	// *** Basic block 3

.NewHashTable_label_41:
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
.func_end_NewHashTable:
	.size NewHashTable, .func_end_NewHashTable-NewHashTable

	.global HashTableDestruct
	.type HashTableDestruct, @function

HashTableDestruct:

	// *** Basic block 0

	.global free
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 40(t0)
	j           free
.func_end_HashTableDestruct:
	.size HashTableDestruct, .func_end_HashTableDestruct-HashTableDestruct

	.global HashTableDelete
	.type HashTableDelete, @function

HashTableDelete:

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
	.global HashTableDestruct
	.global free
	mv          s1, a0
	call        HashTableDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_HashTableDelete:
	.size HashTableDelete, .func_end_HashTableDelete-HashTableDelete

	.global HashTableInsert
	.type HashTableInsert, @function

HashTableInsert:

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
	mv          s1, a0
	mv          s2, a1
	ld          t0, 64(s1)
	li          a2, 1		// 0x1 ASCII \x1
	mv          a1, s1
	mv          a0, s2
	jalr         x1, t0, 0

	// *** Basic block 1

	mv          s3, a0
	ld          t0, 48(s1)
	rem         s3, s3, t0
	ld          t0, 72(s1)
	ld          t1, 40(s1)
	slli        t2, s3, 3
	add         t1, t1, t2
	ld          a0, 0(t1)
	ld          t1, 40(s1)
	add         a2, t1, t2
	mv          a1, s2
	jalr         x1, t0, 0

	// *** Basic block 2

	mv          s4, a0
	beqz        s4, .HashTableInsert_label_55

	// *** Basic block 3

	ld          t0, 56(s1)
	addi        t0, t0, 1
	sd          t0, 56(s1)

	// *** Basic block 4

.HashTableInsert_label_55:
	mv          a0, s4

	// *** Basic block 5

.HashTableInsert_label_58:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_HashTableInsert:
	.size HashTableInsert, .func_end_HashTableInsert-HashTableInsert

	.global HashTableSearch
	.type HashTableSearch, @function

HashTableSearch:

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
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	ld          t0, 64(s1)
	mv          a2, x0
	mv          a1, s1
	mv          a0, s2
	jalr         x1, t0, 0

	// *** Basic block 1

	mv          s3, a0
	ld          t0, 48(s1)
	rem         s3, s3, t0
	ld          t0, 80(s1)
	ld          t1, 40(s1)
	slli        t2, s3, 3
	add         t1, t1, t2
	ld          a0, 0(t1)
	mv          a1, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_HashTableSearch:
	.size HashTableSearch, .func_end_HashTableSearch-HashTableSearch

	.global HashTableTraverse
	.type HashTableTraverse, @function

HashTableTraverse:

	// *** Basic block 0

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
	mv          s2, a1
	mv          s3, x0
	ld          s4, 48(t0)
	bge         x0, s4, .HashTableTraverse_label_43

	// *** Basic block 1

	ld          t1, 40(t0)

	// *** Basic block 2

.HashTableTraverse_label_25:
	slli        t0, s3, 3
	add         t0, t1, t0
	ld          s5, 0(t0)
	beq         s5, x0, .HashTableTraverse_label_38

	// *** Basic block 3

	mv          a1, s2
	mv          a0, s5
	jalr         x1, s1, 0

	// *** Basic block 4

.HashTableTraverse_label_38:

	// *** Basic block 5

.HashTableTraverse_label_39:
	addi        s3, s3, 1
	bge         s3, s4, .HashTableTraverse_label_25

	// *** Basic block 6

.HashTableTraverse_label_43:
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
.func_end_HashTableTraverse:
	.size HashTableTraverse, .func_end_HashTableTraverse-HashTableTraverse

	.global HashTableCopy
	.type HashTableCopy, @function

HashTableCopy:

	// *** Basic block 0

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
	mv          t0, a1
	mv          s1, a0
	mv          s2, a2
	mv          s3, x0
	ld          s4, 48(t0)
	bge         x0, s4, .HashTableCopy_label_51

	// *** Basic block 1

	ld          t1, 40(t0)

	// *** Basic block 2

.HashTableCopy_label_27:
	slli        s5, s3, 3
	add         t0, t1, s5
	ld          s6, 0(t0)
	beq         s6, x0, .HashTableCopy_label_46

	// *** Basic block 3

	ld          t0, 40(s1)
	add         s5, t0, s5
	mv          a0, s6
	jalr         x1, s2, 0

	// *** Basic block 4

	sd          a0, 0(s5)
	ld          t0, 56(s1)
	addi        t0, t0, 1
	sd          t0, 56(s1)

	// *** Basic block 5

.HashTableCopy_label_46:

	// *** Basic block 6

.HashTableCopy_label_47:
	addi        s3, s3, 1
	bge         s3, s4, .HashTableCopy_label_27

	// *** Basic block 7

.HashTableCopy_label_51:
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
.func_end_HashTableCopy:
	.size HashTableCopy, .func_end_HashTableCopy-HashTableCopy

	.global HashTableClear
	.type HashTableClear, @function

HashTableClear:

	// *** Basic block 0

	.global memset
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 40(t0)
	ld          t1, 48(t0)
	slli        a2, t1, 3
	mv          a1, x0
	j           memset
.func_end_HashTableClear:
	.size HashTableClear, .func_end_HashTableClear-HashTableClear

	.global HashTablePrintStats
	.type HashTablePrintStats, @function

HashTablePrintStats:

	// *** Basic block 0

	.global fprintf
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	lla         a1, .str.1
	ld          a2, 16(t1)
	ld          a3, 56(t1)
	mv          a0, t0
	j           fprintf
.func_end_HashTablePrintStats:
	.size HashTablePrintStats, .func_end_HashTablePrintStats-HashTablePrintStats

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "table %s: %zd objects\n"
	.type .str.1, @object
	.size .str.1, 23

