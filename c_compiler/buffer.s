	.file   "buffer.c"
	.text
	.option pic
.PCbegin:
	.global BufferInit
	.type BufferInit, @function

BufferInit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          x0, 0(a0)
	sd          x0, 8(a0)
	sd          x0, 16(a0)
	ret         
.func_end_BufferInit:
	.size BufferInit, .func_end_BufferInit-BufferInit

	.global NewBuffer
	.type NewBuffer, @function

NewBuffer:

	// *** Basic block 0

	.global malloc
	.global BufferInit
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
	call        BufferInit

	// *** Basic block 2

	mv          a0, s1

	// *** Basic block 3

.NewBuffer_label_16:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewBuffer:
	.size NewBuffer, .func_end_NewBuffer-NewBuffer

	.global BufferDestruct
	.type BufferDestruct, @function

BufferDestruct:

	// *** Basic block 0

	.global free
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 0(t0)
	j           free
.func_end_BufferDestruct:
	.size BufferDestruct, .func_end_BufferDestruct-BufferDestruct

	.global BufferDelete
	.type BufferDelete, @function

BufferDelete:

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
	.global BufferDestruct
	.global free
	mv          s1, a0
	call        BufferDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_BufferDelete:
	.size BufferDelete, .func_end_BufferDelete-BufferDelete

	.global BufferClear
	.type BufferClear, @function

BufferClear:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          x0, 8(a0)
	ret         
.func_end_BufferClear:
	.size BufferClear, .func_end_BufferClear-BufferClear

	.local  ExpandMemory
	.type ExpandMemory, @function

ExpandMemory:

	// *** Basic block 0

	.global calloc
	.global realloc
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
	slli        t0, a1, 1
	sd          t0, 16(s1)
	ld          s2, 0(s1)
	bne         s2, x0, .ExpandMemory_label_33

	// *** Basic block 1

	ld          a0, 16(s1)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        calloc

	// *** Basic block 2

	sd          a0, 0(s1)
	j           .ExpandMemory_label_56

	// *** Basic block 3

.ExpandMemory_label_33:
	ld          a1, 16(s1)
	mv          a0, s2
	call        realloc

	// *** Basic block 4

	sd          a0, 0(s1)
	ld          t0, 0(s1)
	ld          t1, 8(s1)
	add         a0, t0, t1
	ld          t0, 16(s1)
	ld          t1, 8(s1)
	sub         a2, t0, t1
	mv          a1, x0
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           memset

	// *** Basic block 5

.ExpandMemory_label_56:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ExpandMemory:
	.size ExpandMemory, .func_end_ExpandMemory-ExpandMemory

	.global BufferAppend
	.type BufferAppend, @function

BufferAppend:

	// *** Basic block 0

	.local ExpandMemory
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
	mv          s2, a2
	mv          s3, a1
	ld          s4, 8(s1)
	add         s5, s4, s2
	ld          t0, 16(s1)
	bge         t0, s5, .BufferAppend_label_31

	// *** Basic block 1

	mv          a1, s5
	mv          a0, s1
	call        ExpandMemory

	// *** Basic block 2

.BufferAppend_label_31:
	ld          t0, 0(s1)
	add         a0, t0, s4
	mv          a2, s2
	mv          a1, s3
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           memcpy
.func_end_BufferAppend:
	.size BufferAppend, .func_end_BufferAppend-BufferAppend

	.global BufferAppendByte
	.type BufferAppendByte, @function

BufferAppendByte:

	// *** Basic block 0

	.local ExpandMemory
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          t2, 8(t0)
	addi        t3, t2, 1
	ld          t4, 16(t0)
	bge         t4, t3, .BufferAppendByte_label_29

	// *** Basic block 1

	mv          a1, t3
	mv          a0, t0
	j           ExpandMemory

	// *** Basic block 2

.BufferAppendByte_label_29:
	ld          t3, 0(t0)
	add         t2, t3, t2
	sb          t1, 0(t2)
	ld          t2, 8(t0)
	addi        t2, t2, 1
	sd          t2, 8(t0)
	ret         
.func_end_BufferAppendByte:
	.size BufferAppendByte, .func_end_BufferAppendByte-BufferAppendByte

	.global BufferAppendHalfLE
	.type BufferAppendHalfLE, @function

BufferAppendHalfLE:

	// *** Basic block 0

	.global BufferAppend
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a1, -24(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	addi        a1, s0, -24
	li          a2, 2		// 0x2 ASCII \x2
	call        BufferAppend

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BufferAppendHalfLE:
	.size BufferAppendHalfLE, .func_end_BufferAppendHalfLE-BufferAppendHalfLE

	.global BufferAppendWordLE
	.type BufferAppendWordLE, @function

BufferAppendWordLE:

	// *** Basic block 0

	.global BufferAppend
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a1, -24(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	addi        a1, s0, -24
	li          a2, 4		// 0x4 ASCII \x4
	call        BufferAppend

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BufferAppendWordLE:
	.size BufferAppendWordLE, .func_end_BufferAppendWordLE-BufferAppendWordLE

	.global BufferAppendLongLE
	.type BufferAppendLongLE, @function

BufferAppendLongLE:

	// *** Basic block 0

	.global BufferAppend
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a1, -24(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	addi        a1, s0, -24
	li          a2, 8		// 0x8 ASCII \x8
	call        BufferAppend

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BufferAppendLongLE:
	.size BufferAppendLongLE, .func_end_BufferAppendLongLE-BufferAppendLongLE

	.global BufferAddSpace
	.type BufferAddSpace, @function

BufferAddSpace:

	// *** Basic block 0

	.local ExpandMemory
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 8(t0)
	add         t2, t1, a1
	ld          t1, 16(t0)
	bge         t1, t2, .BufferAddSpace_label_26

	// *** Basic block 1

	mv          a1, t2
	mv          a0, t0
	j           ExpandMemory

	// *** Basic block 2

.BufferAddSpace_label_26:
	sd          t2, 8(t0)
	ret         
.func_end_BufferAddSpace:
	.size BufferAddSpace, .func_end_BufferAddSpace-BufferAddSpace

	.global BufferAlignLength
	.type BufferAlignLength, @function

BufferAlignLength:

	// *** Basic block 0

	.global BufferAddSpace
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          t2, 8(t0)
	addi        t3, t1, -1
	add         t4, t2, t3
	not         t3, t3
	and         t5, t4, t3
	sub         a1, t5, t2
	j           BufferAddSpace
.func_end_BufferAlignLength:
	.size BufferAlignLength, .func_end_BufferAlignLength-BufferAlignLength

	.global BufferCompare
	.type BufferCompare, @function

BufferCompare:

	// *** Basic block 0

	.global memcmp
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
	bge         s3, s3, .BufferCompare_label_21

	// *** Basic block 1

	j           .BufferCompare_label_22

	// *** Basic block 2

.BufferCompare_label_21:

	// *** Basic block 3

.BufferCompare_label_22:
	ld          a0, 0(s1)
	ld          a1, 0(s2)
	mv          a2, s3
	call        memcmp

	// *** Basic block 4

	mv          s4, a0
	beqz        s4, .BufferCompare_label_41

	// *** Basic block 5

	mv          a0, s4

	// *** Basic block 6

.BufferCompare_label_38:
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

.BufferCompare_label_41:
	sub         t0, s3, s3
	sext.w      a0, t0
	j           .BufferCompare_label_38
.func_end_BufferCompare:
	.size BufferCompare, .func_end_BufferCompare-BufferCompare

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
