	.file   "strings.c"
	.text
	.option pic
.PCbegin:
	.global bcmp
	.type bcmp, @function

bcmp:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global memcmp
	j           memcmp
.func_end_bcmp:
	.size bcmp, .func_end_bcmp-bcmp

	.global bcopy
	.type bcopy, @function

bcopy:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global memmove
	mv          t0, a1
	mv          t1, a0
	mv          a1, t1
	mv          a0, t0
	j           memmove
.func_end_bcopy:
	.size bcopy, .func_end_bcopy-bcopy

	.global bzero
	.type bzero, @function

bzero:

	// *** Basic block 0

	.global memset
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          a2, t0
	mv          a1, x0
	j           memset
.func_end_bzero:
	.size bzero, .func_end_bzero-bzero

	.global ffs
	.type ffs, @function

ffs:

	// *** Basic block 0

	.local MultiplyDeBruijnBitPosition2
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	bnez        t0, .ffs_label_19

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.ffs_label_16:
	ret         

	// *** Basic block 3

.ffs_label_19:
	addi        t1, t0, -1
	and         t1, t0, t1
	sub         t0, t0, t1
	li          t1, 125613361		// 0x77cb531
	mul         t1, t0, t1
	srli        t1, t1, 27
	slli        t1, t1, 2
	lla         t2, MultiplyDeBruijnBitPosition2
	add         t1, t2, t1
	lw          t1, 0(t1)
	addi        a0, t1, 1
	ret         
.func_end_ffs:
	.size ffs, .func_end_ffs-ffs

	.global index
	.type index, @function

index:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global strchr
	j           strchr
.func_end_index:
	.size index, .func_end_index-index

	.global rindex
	.type rindex, @function

rindex:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global strrchr
	j           strrchr
.func_end_rindex:
	.size rindex, .func_end_rindex-rindex

	.global strcasecmp
	.type strcasecmp, @function

strcasecmp:

	// *** Basic block 0

	.global tolower
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
	lb          t1, 0(s1)
	snez        t0, t1
	beqz        t1, .strcasecmp_label_18

	// *** Basic block 1

	lb          t1, 0(s2)
	snez        t0, t1

	// *** Basic block 2

.strcasecmp_label_18:
	beqz        t0, .strcasecmp_label_45

	// *** Basic block 3

.strcasecmp_label_20:
	lb          a0, 0(s1)
	call        tolower

	// *** Basic block 4

	mv          s3, a0
	lb          a0, 0(s2)
	call        tolower

	// *** Basic block 5

	mv          t0, a0
	bne         s3, t0, .strcasecmp_label_45

	// *** Basic block 6

.strcasecmp_label_34:
	addi        s1, s1, 1
	addi        s2, s2, 1
	lb          t1, 0(s1)
	snez        t0, t1
	beqz        t1, .strcasecmp_label_43

	// *** Basic block 7

	lb          t1, 0(s2)
	snez        t0, t1

	// *** Basic block 8

.strcasecmp_label_43:
	bnez        t0, .strcasecmp_label_20

	// *** Basic block 9

.strcasecmp_label_45:
	lb          a0, 0(s1)
	call        tolower

	// *** Basic block 10

	mv          s3, a0
	lb          a0, 0(s2)
	call        tolower

	// *** Basic block 11

	mv          t0, a0
	sub         a0, s3, t0

	// *** Basic block 12

.strcasecmp_label_58:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_strcasecmp:
	.size strcasecmp, .func_end_strcasecmp-strcasecmp

	.global strncasecmp
	.type strncasecmp, @function

strncasecmp:

	// *** Basic block 0

	.global tolower
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
	mv          t1, s3
	addi        s3, s3, -1
	slt         t0, x0, t1
	bge         x0, t1, .strncasecmp_label_28

	// *** Basic block 1

	lb          t1, 0(s1)
	snez        t0, t1

	// *** Basic block 2

.strncasecmp_label_28:
	beqz        t0, .strncasecmp_label_57

	// *** Basic block 3

.strncasecmp_label_30:
	lb          a0, 0(s1)
	call        tolower

	// *** Basic block 4

	mv          s4, a0
	lb          a0, 0(s2)
	call        tolower

	// *** Basic block 5

	mv          t0, a0
	bne         s4, t0, .strncasecmp_label_57

	// *** Basic block 6

.strncasecmp_label_44:
	addi        s1, s1, 1
	addi        s2, s2, 1
	mv          t1, s3
	slt         t0, x0, t1
	bge         x0, t1, .strncasecmp_label_55

	// *** Basic block 7

	lb          t1, 0(s1)
	snez        t0, t1

	// *** Basic block 8

.strncasecmp_label_55:
	bnez        t0, .strncasecmp_label_30

	// *** Basic block 9

.strncasecmp_label_57:
	lb          a0, 0(s1)
	call        tolower

	// *** Basic block 10

	mv          s4, a0
	lb          a0, 0(s2)
	call        tolower

	// *** Basic block 11

	mv          t0, a0
	sub         a0, s4, t0

	// *** Basic block 12

.strncasecmp_label_70:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_strncasecmp:
	.size strncasecmp, .func_end_strncasecmp-strncasecmp

.PCend:
	.data
MultiplyDeBruijnBitPosition2:
	.type   MultiplyDeBruijnBitPosition2,@object
	.local  MultiplyDeBruijnBitPosition2
	.size   MultiplyDeBruijnBitPosition2,128
	.p2align  2
	.word   0
	.word   1
	.word   28
	.word   2
	.word   29
	.word   14
	.word   24
	.word   3
	.word   30
	.word   22
	.word   20
	.word   15
	.word   25
	.word   17
	.word   4
	.word   8
	.word   31
	.word   27
	.word   13
	.word   23
	.word   21
	.word   19
	.word   16
	.word   7
	.word   26
	.word   12
	.word   18
	.word   6
	.word   11
	.word   5
	.word   10
	.word   9

	.section ".rodata", "aMS", @progbits
