	.file   "dstring.c"
	.text
	.option pic
.PCbegin:
	.local  LazyInit
	.type LazyInit, @function

LazyInit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	bne         t0, x0, .LazyInit_label_18

	// *** Basic block 1

.LazyInit_label_15:
	ret         

	// *** Basic block 2

.LazyInit_label_18:
	ld          t1, 16(t0)
	bne         t1, x0, .LazyInit_label_31

	// *** Basic block 3

	sd          t0, 16(t0)
	sd          x0, 24(t0)
	li          t1, 16		// 0x10 ASCII \x10
	sd          t1, 32(t0)

	// *** Basic block 4

.LazyInit_label_31:
	j           .LazyInit_label_15
.func_end_LazyInit:
	.size LazyInit, .func_end_LazyInit-LazyInit

	.global StringInitFromSegment
	.type StringInitFromSegment, @function

StringInitFromSegment:

	// *** Basic block 0

	.global malloc
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
	mv          s1, a2
	mv          s2, a0
	mv          s3, a1
	addi        s4, s1, 1
	li          s5, 16		// 0x10 ASCII \x10
	bge         s5, s4, .StringInitFromSegment_label_36

	// *** Basic block 1

	mv          a0, s4
	call        malloc

	// *** Basic block 2

	sd          a0, 16(s2)
	sd          s4, 32(s2)
	j           .StringInitFromSegment_label_42

	// *** Basic block 3

.StringInitFromSegment_label_36:
	sd          s2, 16(s2)
	sd          s5, 32(s2)

	// *** Basic block 4

.StringInitFromSegment_label_42:
	sd          s1, 24(s2)
	beq         s3, x0, .StringInitFromSegment_label_61

	// *** Basic block 5

	ld          a0, 16(s2)
	mv          a2, s1
	mv          a1, s3
	call        memcpy

	// *** Basic block 6

	ld          t0, 16(s2)
	add         t0, t0, s1
	sb          x0, 0(t0)
	j           .StringInitFromSegment_label_65

	// *** Basic block 7

.StringInitFromSegment_label_61:
	ld          t0, 16(s2)
	sb          x0, 0(t0)

	// *** Basic block 8

.StringInitFromSegment_label_65:
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
.func_end_StringInitFromSegment:
	.size StringInitFromSegment, .func_end_StringInitFromSegment-StringInitFromSegment

	.global StringInit
	.type StringInit, @function

StringInit:

	// *** Basic block 0

	.global strlen
	.global StringInitFromSegment
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
	bne         s1, x0, .StringInit_label_19

	// *** Basic block 1

	mv          s3, x0
	j           .StringInit_label_24

	// *** Basic block 2

.StringInit_label_19:
	mv          a0, s1
	call        strlen

	// *** Basic block 3

	mv          s3, a0

	// *** Basic block 4

.StringInit_label_24:
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringInitFromSegment
.func_end_StringInit:
	.size StringInit, .func_end_StringInit-StringInit

	.global StringInitImmutable
	.type StringInitImmutable, @function

StringInitImmutable:

	// *** Basic block 0

	.global strlen
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
	bne         s1, x0, .StringInitImmutable_label_22

	// *** Basic block 1

	mv          s3, x0
	j           .StringInitImmutable_label_27

	// *** Basic block 2

.StringInitImmutable_label_22:
	mv          a0, s1
	call        strlen

	// *** Basic block 3

	mv          s3, a0

	// *** Basic block 4

.StringInitImmutable_label_27:
	sd          s1, 16(s2)
	li          t0, -1		// 0xffffffffffffffff
	sd          t0, 32(s2)
	sd          s3, 24(s2)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringInitImmutable:
	.size StringInitImmutable, .func_end_StringInitImmutable-StringInitImmutable

	.local  CheckMutable
	.type CheckMutable, @function

CheckMutable:

	// *** Basic block 0

	.local LazyInit
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
	// End of stack frame
	mv          s1, a0
	call        LazyInit

	// *** Basic block 1

	ld          t0, 32(s1)
	li          t1, -1		// 0xffffffffffffffff
	beq         t0, t1, .CheckMutable_label_26

	// *** Basic block 2

	j           .CheckMutable_label_45

	// *** Basic block 3

.CheckMutable_label_26:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 63		// 0x3f ASCII '?'
	mv          a2, t0
	call        printf

	// *** Basic block 4

	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           abort

	// *** Basic block 5

.CheckMutable_label_45:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CheckMutable:
	.size CheckMutable, .func_end_CheckMutable-CheckMutable

	.global StringClear
	.type StringClear, @function

StringClear:

	// *** Basic block 0

	.local CheckMutable
	.global free
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
	call        CheckMutable

	// *** Basic block 1

	ld          s2, 16(s1)
	beq         s2, s1, .StringClear_label_31

	// *** Basic block 2

	mv          a0, s2
	call        free

	// *** Basic block 3

	sd          s1, 16(s1)
	li          t0, 16		// 0x10 ASCII \x10
	sd          t0, 32(s1)

	// *** Basic block 4

.StringClear_label_31:
	sd          x0, 24(s1)
	ld          t0, 16(s1)
	sb          x0, 0(t0)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringClear:
	.size StringClear, .func_end_StringClear-StringClear

	.global NewString
	.type NewString, @function

NewString:

	// *** Basic block 0

	.global malloc
	.global StringInit
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
	li          a0, 40		// 0x28 ASCII '('
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        StringInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.NewString_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewString:
	.size NewString, .func_end_NewString-NewString

	.global NewStringWithLength
	.type NewStringWithLength, @function

NewStringWithLength:

	// *** Basic block 0

	.global calloc
	.global StringAppendSegment
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
	li          a1, 1		// 0x1 ASCII \x1
	li          a0, 40		// 0x28 ASCII '('
	call        calloc

	// *** Basic block 1

	mv          s3, a0
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        StringAppendSegment

	// *** Basic block 2

	mv          a0, s3

	// *** Basic block 3

.NewStringWithLength_label_29:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewStringWithLength:
	.size NewStringWithLength, .func_end_NewStringWithLength-NewStringWithLength

	.global StringDestruct
	.type StringDestruct, @function

StringDestruct:

	// *** Basic block 0

	.global free
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t2, 32(t0)
	addi        t3, t2, 1
	snez        t1, t3
	li          t3, -1		// 0xffffffffffffffff
	beq         t2, t3, .StringDestruct_label_25

	// *** Basic block 1

	ld          t2, 16(t0)
	sub         t2, t2, t0
	snez        t1, t2

	// *** Basic block 2

.StringDestruct_label_25:
	beqz        t1, .StringDestruct_label_32

	// *** Basic block 3

	ld          a0, 16(t0)
	j           free

	// *** Basic block 4

.StringDestruct_label_32:
	sd          x0, 16(t0)
	sd          x0, 24(t0)
	sd          x0, 32(t0)
	ret         
.func_end_StringDestruct:
	.size StringDestruct, .func_end_StringDestruct-StringDestruct

	.global StringDelete
	.type StringDelete, @function

StringDelete:

	// *** Basic block 0

	.local CheckMutable
	.global free
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
	call        CheckMutable

	// *** Basic block 1

	ld          s2, 16(s1)
	beq         s2, s1, .StringDelete_label_21

	// *** Basic block 2

	mv          a0, s2
	call        free

	// *** Basic block 3

.StringDelete_label_21:
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_StringDelete:
	.size StringDelete, .func_end_StringDelete-StringDelete

	.global StringCharAt
	.type StringCharAt, @function

StringCharAt:

	// *** Basic block 0

	.local LazyInit
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
	call        LazyInit

	// *** Basic block 1

	ld          t0, 16(s1)
	add         t0, t0, s2
	lb          a0, 0(t0)

	// *** Basic block 2

.StringCharAt_label_20:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringCharAt:
	.size StringCharAt, .func_end_StringCharAt-StringCharAt

	.global StringEqual
	.type StringEqual, @function

StringEqual:

	// *** Basic block 0

	.local LazyInit
	.global strcmp
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
	call        LazyInit

	// *** Basic block 1

	sub         t0, s1, x0
	snez        a0, t0
	beq         s1, x0, .StringEqual_label_27

	// *** Basic block 2

	ld          a0, 16(s1)
	mv          a1, s2
	call        strcmp

	// *** Basic block 3

	seqz        a0, a0

	// *** Basic block 4

.StringEqual_label_27:

	// *** Basic block 5

.StringEqual_label_29:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringEqual:
	.size StringEqual, .func_end_StringEqual-StringEqual

	.global StringEqualCaseBlind
	.type StringEqualCaseBlind, @function

StringEqualCaseBlind:

	// *** Basic block 0

	.local LazyInit
	.global strcasecmp
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
	call        LazyInit

	// *** Basic block 1

	sub         t0, s1, x0
	snez        a0, t0
	beq         s1, x0, .StringEqualCaseBlind_label_27

	// *** Basic block 2

	ld          a0, 16(s1)
	mv          a1, s2
	call        strcasecmp

	// *** Basic block 3

	seqz        a0, a0

	// *** Basic block 4

.StringEqualCaseBlind_label_27:

	// *** Basic block 5

.StringEqualCaseBlind_label_29:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringEqualCaseBlind:
	.size StringEqualCaseBlind, .func_end_StringEqualCaseBlind-StringEqualCaseBlind

	.global StringCompare
	.type StringCompare, @function

StringCompare:

	// *** Basic block 0

	.local LazyInit
	.global strcmp
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
	call        LazyInit

	// *** Basic block 1

	ld          a0, 16(s1)
	mv          a1, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           strcmp
.func_end_StringCompare:
	.size StringCompare, .func_end_StringCompare-StringCompare

	.global StringCompareCaseBlind
	.type StringCompareCaseBlind, @function

StringCompareCaseBlind:

	// *** Basic block 0

	.local LazyInit
	.global strcasecmp
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
	call        LazyInit

	// *** Basic block 1

	ld          a0, 16(s1)
	mv          a1, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           strcasecmp
.func_end_StringCompareCaseBlind:
	.size StringCompareCaseBlind, .func_end_StringCompareCaseBlind-StringCompareCaseBlind

	.global StringEqualString
	.type StringEqualString, @function

StringEqualString:

	// *** Basic block 0

	.local LazyInit
	.global strcmp
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
	call        LazyInit

	// *** Basic block 1

	mv          a0, s2
	call        LazyInit

	// *** Basic block 2

	ld          a0, 16(s1)
	ld          a1, 16(s2)
	call        strcmp

	// *** Basic block 3

	seqz        a0, a0

	// *** Basic block 4

.StringEqualString_label_28:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringEqualString:
	.size StringEqualString, .func_end_StringEqualString-StringEqualString

	.global StringCompareString
	.type StringCompareString, @function

StringCompareString:

	// *** Basic block 0

	.local LazyInit
	.global strcmp
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
	call        LazyInit

	// *** Basic block 1

	mv          a0, s2
	call        LazyInit

	// *** Basic block 2

	ld          a0, 16(s1)
	ld          a1, 16(s2)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           strcmp
.func_end_StringCompareString:
	.size StringCompareString, .func_end_StringCompareString-StringCompareString

	.global StringCompareStringCaseBlind
	.type StringCompareStringCaseBlind, @function

StringCompareStringCaseBlind:

	// *** Basic block 0

	.local LazyInit
	.global strcasecmp
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
	call        LazyInit

	// *** Basic block 1

	mv          a0, s2
	call        LazyInit

	// *** Basic block 2

	ld          a0, 16(s1)
	ld          a1, 16(s2)
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           strcasecmp
.func_end_StringCompareStringCaseBlind:
	.size StringCompareStringCaseBlind, .func_end_StringCompareStringCaseBlind-StringCompareStringCaseBlind

	.global StringSet
	.type StringSet, @function

StringSet:

	// *** Basic block 0

	.local CheckMutable
	.global strlen
	.global malloc
	.global realloc
	.global strcpy
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
	call        CheckMutable

	// *** Basic block 1

	bne         s2, x0, .StringSet_label_28

	// *** Basic block 2

	li          s3, 1		// 0x1 ASCII \x1
	j           .StringSet_label_33

	// *** Basic block 3

.StringSet_label_28:
	mv          a0, s2
	call        strlen

	// *** Basic block 4

	addi        s3, a0, 1

	// *** Basic block 5

.StringSet_label_33:
	ld          t0, 32(s1)
	bge         t0, s3, .StringSet_label_61

	// *** Basic block 6

	ld          s4, 16(s1)
	bne         s4, s1, .StringSet_label_50

	// *** Basic block 7

	mv          a0, s3
	call        malloc

	// *** Basic block 8

	sd          a0, 16(s1)
	j           .StringSet_label_58

	// *** Basic block 9

.StringSet_label_50:
	mv          a1, s3
	mv          a0, s4
	call        realloc

	// *** Basic block 10

	sd          a0, 16(s1)

	// *** Basic block 11

.StringSet_label_58:
	sd          s3, 32(s1)

	// *** Basic block 12

.StringSet_label_61:
	beq         s2, x0, .StringSet_label_72

	// *** Basic block 13

	ld          a0, 16(s1)
	mv          a1, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           strcpy

	// *** Basic block 14

.StringSet_label_72:
	addi        t0, s3, -1
	sd          t0, 24(s1)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringSet:
	.size StringSet, .func_end_StringSet-StringSet

	.global StringSetString
	.type StringSetString, @function

StringSetString:

	// *** Basic block 0

	.local LazyInit
	.global StringSet
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
	mv          a0, s1
	call        LazyInit

	// *** Basic block 1

	ld          a1, 16(s1)
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringSet
.func_end_StringSetString:
	.size StringSetString, .func_end_StringSetString-StringSetString

	.global StringAppendSegment
	.type StringAppendSegment, @function

StringAppendSegment:

	// *** Basic block 0

	.local CheckMutable
	.global malloc
	.global memcpy
	.global realloc
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
	call        CheckMutable

	// *** Basic block 1

	ld          t0, 24(s1)
	add         t0, t0, s2
	addi        s4, t0, 1
	ld          t0, 32(s1)
	bge         t0, s4, .StringAppendSegment_label_66

	// *** Basic block 2

	ld          s5, 16(s1)
	bne         s5, s1, .StringAppendSegment_label_55

	// *** Basic block 3

	mv          a0, s4
	call        malloc

	// *** Basic block 4

	sd          a0, 16(s1)
	ld          a0, 16(s1)
	li          t0, 16		// 0x10 ASCII \x10
	mv          a2, t0
	mv          a1, s1
	call        memcpy

	// *** Basic block 5

	j           .StringAppendSegment_label_63

	// *** Basic block 6

.StringAppendSegment_label_55:
	mv          a1, s4
	mv          a0, s5
	call        realloc

	// *** Basic block 7

	sd          a0, 16(s1)

	// *** Basic block 8

.StringAppendSegment_label_63:
	sd          s4, 32(s1)

	// *** Basic block 9

.StringAppendSegment_label_66:
	ld          t0, 16(s1)
	ld          t1, 24(s1)
	add         a0, t0, t1
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
.func_end_StringAppendSegment:
	.size StringAppendSegment, .func_end_StringAppendSegment-StringAppendSegment

	.global StringAppend
	.type StringAppend, @function

StringAppend:

	// *** Basic block 0

	.local LazyInit
	.global strlen
	.global StringAppendSegment
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
	call        LazyInit

	// *** Basic block 1

	bne         s2, x0, .StringAppend_label_23

	// *** Basic block 2

	mv          s3, x0
	j           .StringAppend_label_28

	// *** Basic block 3

.StringAppend_label_23:
	mv          a0, s2
	call        strlen

	// *** Basic block 4

	mv          s3, a0

	// *** Basic block 5

.StringAppend_label_28:
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringAppendSegment
.func_end_StringAppend:
	.size StringAppend, .func_end_StringAppend-StringAppend

	.global StringAppendString
	.type StringAppendString, @function

StringAppendString:

	// *** Basic block 0

	.local LazyInit
	.global StringAppend
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
	mv          a0, s1
	call        LazyInit

	// *** Basic block 1

	ld          a1, 16(s1)
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringAppend
.func_end_StringAppendString:
	.size StringAppendString, .func_end_StringAppendString-StringAppendString

	.global StringTrimEnd
	.type StringTrimEnd, @function

StringTrimEnd:

	// *** Basic block 0

	.local CheckMutable
	.global isspace
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	call        CheckMutable

	// *** Basic block 1

	ld          t0, 24(s1)
	bge         x0, t0, .StringTrimEnd_label_43

	// *** Basic block 2

.StringTrimEnd_label_21:
	ld          t0, 16(s1)
	ld          t1, 24(s1)
	addi        t1, t1, -1
	add         t0, t0, t1
	lb          a0, 0(t0)
	call        isspace

	// *** Basic block 3

	beqz        a0, .StringTrimEnd_label_41

	// *** Basic block 4

	ld          t0, 24(s1)
	addi        t1, t0, -1
	sd          t1, 24(s1)

	// *** Basic block 5

.StringTrimEnd_label_37:
	blt         x0, t0, .StringTrimEnd_label_21

	// *** Basic block 6

	j           .StringTrimEnd_label_43

	// *** Basic block 7

.StringTrimEnd_label_41:
	j           .StringTrimEnd_label_43

	// *** Basic block 8

.StringTrimEnd_label_43:
	ld          t0, 16(s1)
	ld          t1, 24(s1)
	add         t0, t0, t1
	sb          x0, 0(t0)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringTrimEnd:
	.size StringTrimEnd, .func_end_StringTrimEnd-StringTrimEnd

	.global StringTrimStart
	.type StringTrimStart, @function

StringTrimStart:

	// *** Basic block 0

	.local CheckMutable
	.global isspace
	.global StringClear
	.global StringReplace
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
	call        CheckMutable

	// *** Basic block 1

	ld          s2, 16(s1)
	mv          s3, x0
	ld          s4, 24(s1)
	bge         x0, s4, .StringTrimStart_label_41

	// *** Basic block 2

.StringTrimStart_label_28:
	add         t0, s2, s3
	lb          a0, 0(t0)
	call        isspace

	// *** Basic block 3

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         t0, t0
	bnez        t0, .StringTrimStart_label_41

	// *** Basic block 4

.StringTrimStart_label_37:
	addi        s3, s3, 1
	blt         s3, s4, .StringTrimStart_label_28

	// *** Basic block 5

.StringTrimStart_label_41:
	bne         s3, s4, .StringTrimStart_label_52

	// *** Basic block 6

	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringClear

	// *** Basic block 7

.StringTrimStart_label_49:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 8

.StringTrimStart_label_52:
	lla         a3, .str.4
	mv          a4, x0
	mv          a2, s3
	mv          a1, x0
	mv          a0, s1
	call        StringReplace

	// *** Basic block 9

	j           .StringTrimStart_label_49
.func_end_StringTrimStart:
	.size StringTrimStart, .func_end_StringTrimStart-StringTrimStart

	.global StringTrim
	.type StringTrim, @function

StringTrim:

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
	.global StringTrimStart
	.global StringTrimEnd
	mv          s1, a0
	call        StringTrimStart

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringTrimEnd
.func_end_StringTrim:
	.size StringTrim, .func_end_StringTrim-StringTrim

	.global StringAppendChar
	.type StringAppendChar, @function

StringAppendChar:

	// *** Basic block 0

	.local CheckMutable
	.global malloc
	.global memcpy
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
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	call        CheckMutable

	// *** Basic block 1

	ld          t0, 24(s1)
	addi        s3, t0, 2
	ld          t0, 32(s1)
	bge         t0, s3, .StringAppendChar_label_65

	// *** Basic block 2

	slli        s3, s3, 1
	ld          s4, 16(s1)
	bne         s4, s1, .StringAppendChar_label_54

	// *** Basic block 3

	mv          a0, s3
	call        malloc

	// *** Basic block 4

	sd          a0, 16(s1)
	ld          a0, 16(s1)
	li          t0, 16		// 0x10 ASCII \x10
	mv          a2, t0
	mv          a1, s1
	call        memcpy

	// *** Basic block 5

	j           .StringAppendChar_label_62

	// *** Basic block 6

.StringAppendChar_label_54:
	mv          a1, s3
	mv          a0, s4
	call        realloc

	// *** Basic block 7

	sd          a0, 16(s1)

	// *** Basic block 8

.StringAppendChar_label_62:
	sd          s3, 32(s1)

	// *** Basic block 9

.StringAppendChar_label_65:
	ld          t0, 16(s1)
	ld          t1, 24(s1)
	addi        t1, t1, 1
	sd          t1, 24(s1)
	add         t0, t0, t1
	sb          s2, 0(t0)
	ld          t0, 16(s1)
	ld          t1, 24(s1)
	add         t0, t0, t1
	sb          x0, 0(t0)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringAppendChar:
	.size StringAppendChar, .func_end_StringAppendChar-StringAppendChar

	.global StringReplace
	.type StringReplace, @function

StringReplace:

	// *** Basic block 0

	.local CheckMutable
	.global malloc
	.global memcpy
	.global realloc
	.global memmove
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
	sd s8, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a4
	mv          s3, a2
	mv          s4, a1
	mv          s5, a3
	call        CheckMutable

	// *** Basic block 1

	sub         s6, s2, s3
	ld          t0, 24(s1)
	add         t0, t0, s6
	addi        s7, t0, 1
	ld          t0, 32(s1)
	bge         t0, s7, .StringReplace_label_77

	// *** Basic block 2

	slli        s7, s7, 1
	ld          s8, 16(s1)
	bne         s8, s1, .StringReplace_label_66

	// *** Basic block 3

	mv          a0, s7
	call        malloc

	// *** Basic block 4

	sd          a0, 16(s1)
	ld          a0, 16(s1)
	li          t0, 16		// 0x10 ASCII \x10
	mv          a2, t0
	mv          a1, s1
	call        memcpy

	// *** Basic block 5

	j           .StringReplace_label_74

	// *** Basic block 6

.StringReplace_label_66:
	mv          a1, s7
	mv          a0, s8
	call        realloc

	// *** Basic block 7

	sd          a0, 16(s1)

	// *** Basic block 8

.StringReplace_label_74:
	sd          s7, 32(s1)

	// *** Basic block 9

.StringReplace_label_77:
	ld          t0, 24(s1)
	add         s7, s4, s3
	sub         t0, t0, s7
	addi        s8, t0, 1
	bge         x0, s8, .StringReplace_label_98

	// *** Basic block 10

	ld          t0, 16(s1)
	add         t1, s4, s2
	add         a0, t0, t1
	ld          t0, 16(s1)
	add         a1, t0, s7
	mv          a2, s8
	call        memmove

	// *** Basic block 11

.StringReplace_label_98:
	bge         x0, s2, .StringReplace_label_111

	// *** Basic block 12

	ld          t0, 16(s1)
	add         a0, t0, s4
	mv          a2, s2
	mv          a1, s5
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	ld s8, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           memcpy

	// *** Basic block 13

.StringReplace_label_111:
	ld          t0, 24(s1)
	add         t0, t0, s6
	sd          t0, 24(s1)
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	ld s8, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringReplace:
	.size StringReplace, .func_end_StringReplace-StringReplace

	.global StringReplaceString
	.type StringReplaceString, @function

StringReplaceString:

	// *** Basic block 0

	.global StringReplace
	// Leaf procedure, no stack frame generated
	mv          t0, a3
	ld          a3, 16(t0)
	ld          a4, 24(t0)
	j           StringReplace
.func_end_StringReplaceString:
	.size StringReplaceString, .func_end_StringReplaceString-StringReplaceString

	.global StringErase
	.type StringErase, @function

StringErase:

	// *** Basic block 0

	.global StringReplace
	// Leaf procedure, no stack frame generated
	mv          a4, x0
	mv          a3, x0
	j           StringReplace
.func_end_StringErase:
	.size StringErase, .func_end_StringErase-StringErase

	.global StringIndexOf
	.type StringIndexOf, @function

StringIndexOf:

	// *** Basic block 0

	.local LazyInit
	.global strstr
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
	call        LazyInit

	// *** Basic block 1

	ld          s3, 16(s1)
	mv          a1, s2
	mv          a0, s3
	call        strstr

	// *** Basic block 2

	mv          s4, a0
	bne         s4, x0, .StringIndexOf_label_35

	// *** Basic block 3

	li          a0, -1		// 0xffffffffffffffff

	// *** Basic block 4

.StringIndexOf_label_32:
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

.StringIndexOf_label_35:
	sub         a0, s4, s3
	j           .StringIndexOf_label_32
.func_end_StringIndexOf:
	.size StringIndexOf, .func_end_StringIndexOf-StringIndexOf

	.global StringLastIndexOf
	.type StringLastIndexOf, @function

StringLastIndexOf:

	// *** Basic block 0

	.local LazyInit
	.global strlen
	.global strncmp
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
	call        LazyInit

	// *** Basic block 1

	ld          s3, 16(s1)
	mv          a0, s2
	call        strlen

	// *** Basic block 2

	mv          s4, a0
	ld          t0, 24(s1)
	sub         s5, t0, s4
	bge         x0, s5, .StringLastIndexOf_label_55

	// *** Basic block 3

.StringLastIndexOf_label_35:
	add         a0, s3, s5
	mv          a2, s4
	mv          a1, s2
	call        strncmp

	// *** Basic block 4

	bnez        a0, .StringLastIndexOf_label_51

	// *** Basic block 5

	mv          a0, s5

	// *** Basic block 6

.StringLastIndexOf_label_48:
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

	// *** Basic block 7

.StringLastIndexOf_label_51:
	addi        s5, s5, -1
	blt         x0, s5, .StringLastIndexOf_label_35

	// *** Basic block 8

.StringLastIndexOf_label_55:
	ld          a0, 16(s1)
	mv          a2, s4
	mv          a1, s2
	call        strncmp

	// *** Basic block 9

	bnez        a0, .StringLastIndexOf_label_69

	// *** Basic block 10

	j           .StringLastIndexOf_label_71

	// *** Basic block 11

.StringLastIndexOf_label_69:
	li          a0, -1		// 0xffffffffffffffff

	// *** Basic block 12

.StringLastIndexOf_label_71:
	j           .StringLastIndexOf_label_48
.func_end_StringLastIndexOf:
	.size StringLastIndexOf, .func_end_StringLastIndexOf-StringLastIndexOf

	.global StringSubstring
	.type StringSubstring, @function

StringSubstring:

	// *** Basic block 0

	.local LazyInit
	.global StringClear
	.global StringAppendSegment
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
	mv          s4, a3
	call        LazyInit

	// *** Basic block 1

	ld          s5, 24(s1)
	blt         s2, s5, .StringSubstring_label_32

	// *** Basic block 2

.StringSubstring_label_29:
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

	// *** Basic block 3

.StringSubstring_label_32:
	add         t0, s2, s3
	blt         t0, s5, .StringSubstring_label_38

	// *** Basic block 4

	sub         s3, s5, s2

	// *** Basic block 5

.StringSubstring_label_38:
	mv          a0, s4
	call        StringClear

	// *** Basic block 6

	ld          t0, 16(s1)
	add         a1, t0, s2
	mv          a2, s3
	mv          a0, s4
	call        StringAppendSegment

	// *** Basic block 7

	j           .StringSubstring_label_29
.func_end_StringSubstring:
	.size StringSubstring, .func_end_StringSubstring-StringSubstring

	.global StringStartsWith
	.type StringStartsWith, @function

StringStartsWith:

	// *** Basic block 0

	.local LazyInit
	.global strstr
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
	call        LazyInit

	// *** Basic block 1

	ld          s3, 16(s1)
	mv          a1, s2
	mv          a0, s3
	call        strstr

	// *** Basic block 2

	sub         t0, a0, s3
	seqz        a0, t0

	// *** Basic block 3

.StringStartsWith_label_25:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringStartsWith:
	.size StringStartsWith, .func_end_StringStartsWith-StringStartsWith

	.global StringEndsWith
	.type StringEndsWith, @function

StringEndsWith:

	// *** Basic block 0

	.local LazyInit
	.global strlen
	.global strcmp
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
	call        LazyInit

	// *** Basic block 1

	mv          a0, s2
	call        strlen

	// *** Basic block 2

	mv          s3, a0
	ld          s4, 24(s1)
	bge         s4, s3, .StringEndsWith_label_33

	// *** Basic block 3

	mv          a0, x0

	// *** Basic block 4

.StringEndsWith_label_30:
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

.StringEndsWith_label_33:
	ld          t0, 16(s1)
	add         t0, t0, s4
	sub         a0, t0, s3
	mv          a1, s2
	call        strcmp

	// *** Basic block 6

	seqz        a0, a0
	j           .StringEndsWith_label_30
.func_end_StringEndsWith:
	.size StringEndsWith, .func_end_StringEndsWith-StringEndsWith

	.global StringPrintf
	.type StringPrintf, @function

StringPrintf:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global StringVPrintf
	mv          t0, s0
	mv          a2, t0
	j           StringVPrintf
.func_end_StringPrintf:
	.size StringPrintf, .func_end_StringPrintf-StringPrintf

	.global StringVPrintf
	.type StringVPrintf, @function

StringVPrintf:

	// *** Basic block 0

	.local LazyInit
	.global vsnprintf
	.global StringAppend
	addi sp, sp, -1072
	// Saved return address (offset 1064) and frame pointer (offset 1056)
	sd ra, 1064(sp)
	sd s0, 1056(sp)
	addi s0, sp, 1072
	// Local vars at offset -1040(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	call        LazyInit

	// *** Basic block 1

	addi        s4, s0, -1040
	mv          a3, s3
	mv          a2, s2
	li          t0, 1024		// 0x400
	mv          a1, t0
	mv          a0, s4
	call        vsnprintf

	// *** Basic block 2

	mv          a1, s4
	mv          a0, s1
	call        StringAppend

	// *** Basic block 3

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringVPrintf:
	.size StringVPrintf, .func_end_StringVPrintf-StringVPrintf

	.global StringContainsChar
	.type StringContainsChar, @function

StringContainsChar:

	// *** Basic block 0

	.local LazyInit
	.global strchr
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
	call        LazyInit

	// *** Basic block 1

	ld          a0, 16(s1)
	mv          a1, s2
	call        strchr

	// *** Basic block 2

	sub         t0, a0, x0
	snez        a0, t0

	// *** Basic block 3

.StringContainsChar_label_25:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringContainsChar:
	.size StringContainsChar, .func_end_StringContainsChar-StringContainsChar

	.global StringContainsString
	.type StringContainsString, @function

StringContainsString:

	// *** Basic block 0

	.local LazyInit
	.global strstr
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
	call        LazyInit

	// *** Basic block 1

	ld          a0, 16(s1)
	mv          a1, s2
	call        strstr

	// *** Basic block 2

	sub         t0, a0, x0
	snez        a0, t0

	// *** Basic block 3

.StringContainsString_label_25:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_StringContainsString:
	.size StringContainsString, .func_end_StringContainsString-StringContainsString

	.global StringEscape
	.type StringEscape, @function

StringEscape:

	// *** Basic block 0

	.local LazyInit
	.global StringPrintf
	.global StringAppendChar
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
	call        LazyInit

	// *** Basic block 1

	ld          t0, 16(s1)
	mv          s3, x0
	ld          s4, 24(s1)
	bge         x0, s4, .StringEscape_label_179

	// *** Basic block 2

.StringEscape_label_47:
	add         t0, t0, s3
	lb          s1, 0(t0)
	slti        t0, s1, 32
	li          t1, 32		// 0x20 ASCII ' '
	blt         s1, t1, .StringEscape_label_59

	// *** Basic block 3

	slti        t1, s1, 127
	not         t0, t1

	// *** Basic block 4

.StringEscape_label_59:
	beqz        t0, .StringEscape_label_131

	// *** Basic block 5

	mv          s5, x0
	li          t0, 7		// 0x7 ASCII \x7
	blt         s1, t0, .StringEscape_label_107

	// *** Basic block 6

	li          t0, 13		// 0xd ASCII \xd
	blt         t0, s1, .StringEscape_label_107

	// *** Basic block 7

	addi        t0, s1, -7
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 8

	j           .StringEscape_label_89

	// *** Basic block 9

	j           .StringEscape_label_101

	// *** Basic block 10

	j           .StringEscape_label_98

	// *** Basic block 11

	j           .StringEscape_label_86

	// *** Basic block 12

	j           .StringEscape_label_104

	// *** Basic block 13

	j           .StringEscape_label_95

	// *** Basic block 14

	j           .StringEscape_label_92

	// *** Basic block 15

.StringEscape_label_86:
	li          s5, 110		// 0x6e ASCII 'n'
	j           .StringEscape_label_107

	// *** Basic block 16

.StringEscape_label_89:
	li          s5, 97		// 0x61 ASCII 'a'
	j           .StringEscape_label_107

	// *** Basic block 17

.StringEscape_label_92:
	li          s5, 114		// 0x72 ASCII 'r'
	j           .StringEscape_label_107

	// *** Basic block 18

.StringEscape_label_95:
	li          s5, 102		// 0x66 ASCII 'f'
	j           .StringEscape_label_107

	// *** Basic block 19

.StringEscape_label_98:
	li          s5, 116		// 0x74 ASCII 't'
	j           .StringEscape_label_107

	// *** Basic block 20

.StringEscape_label_101:
	li          s5, 98		// 0x62 ASCII 'b'
	j           .StringEscape_label_107

	// *** Basic block 21

.StringEscape_label_104:
	li          s5, 118		// 0x76 ASCII 'v'
	j           .StringEscape_label_107

	// *** Basic block 22

.StringEscape_label_107:
	beqz        s5, .StringEscape_label_120

	// *** Basic block 23

	lla         a1, .str.5
	mv          a2, s5
	mv          a0, s2
	call        StringPrintf

	// *** Basic block 24

	j           .StringEscape_label_129

	// *** Basic block 25

.StringEscape_label_120:
	lla         a1, .str.6
	mv          a2, s1
	mv          a0, s2
	call        StringPrintf

	// *** Basic block 26

.StringEscape_label_129:
	j           .StringEscape_label_174

	// *** Basic block 27

.StringEscape_label_131:
	li          t0, 34		// 0x22 ASCII '"'
	beq         s1, t0, .StringEscape_label_155

	// *** Basic block 28

	li          t0, 39		// 0x27 ASCII '''
	beq         s1, t0, .StringEscape_label_154

	// *** Basic block 29

	li          t0, 92		// 0x5c ASCII '\'
	beq         s1, t0, .StringEscape_label_165

	// *** Basic block 30

.StringEscape_label_147:
	mv          a1, s1
	mv          a0, s2
	call        StringAppendChar

	// *** Basic block 31

	j           .StringEscape_label_173

	// *** Basic block 32

.StringEscape_label_154:

	// *** Basic block 33

.StringEscape_label_155:
	lla         a1, .str.7
	mv          a2, s1
	mv          a0, s2
	call        StringPrintf

	// *** Basic block 34

	j           .StringEscape_label_173

	// *** Basic block 35

.StringEscape_label_165:
	lla         a1, .str.8
	mv          a0, s2
	call        StringPrintf

	// *** Basic block 36

	j           .StringEscape_label_173

	// *** Basic block 37

.StringEscape_label_173:

	// *** Basic block 38

.StringEscape_label_174:

	// *** Basic block 39

.StringEscape_label_175:
	addi        s3, s3, 1
	bge         s3, s4, .StringEscape_label_47

	// *** Basic block 40

.StringEscape_label_179:
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
.func_end_StringEscape:
	.size StringEscape, .func_end_StringEscape-StringEscape

	.global StringSplit
	.type StringSplit, @function

StringSplit:

	// *** Basic block 0

	.local LazyInit
	.global NewString
	.global StringAppendSegment
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
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	call        LazyInit

	// *** Basic block 1

	mv          s4, x0
	ld          t0, 24(s1)
	bge         x0, t0, .StringSplit_label_85

	// *** Basic block 2

.StringSplit_label_31:
	mv          s5, s4
	ld          s6, 24(s1)
	slt         t0, s4, s6
	bge         s4, s6, .StringSplit_label_47

	// *** Basic block 3

	ld          t1, 16(s1)
	ld          t2, 16(s1)
	add         t2, t2, s4
	lb          t2, 0(t2)
	sub         t2, t2, s2
	snez        t0, t2

	// *** Basic block 4

.StringSplit_label_47:
	beqz        t0, .StringSplit_label_60

	// *** Basic block 5

.StringSplit_label_49:
	addi        s4, s4, 1
	slt         t0, s4, s6
	bge         s4, s6, .StringSplit_label_58

	// *** Basic block 6

	add         t1, t1, s4
	lb          t1, 0(t1)
	sub         t1, t1, s2
	snez        t0, t1

	// *** Basic block 7

.StringSplit_label_58:
	bnez        t0, .StringSplit_label_49

	// *** Basic block 8

.StringSplit_label_60:
	mv          a0, x0
	call        NewString

	// *** Basic block 9

	mv          s6, a0
	ld          t0, 16(s1)
	add         a1, t0, s5
	sub         a2, s4, s5
	mv          a0, s6
	call        StringAppendSegment

	// *** Basic block 10

	mv          a1, s6
	mv          a0, s3
	call        VectorAppend

	// *** Basic block 11

	addi        s4, s4, 1
	ld          t0, 24(s1)
	blt         s4, t0, .StringSplit_label_31

	// *** Basic block 12

.StringSplit_label_85:
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
.func_end_StringSplit:
	.size StringSplit, .func_end_StringSplit-StringSplit

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.1, @object
	.size .str.1, 30

.str.2:
	.asciz "dstring.c"
	.type .str.2, @object
	.size .str.2, 10

.str.3:
	.asciz "s->capacity != STRING_IMMUTABLE"
	.type .str.3, @object
	.size .str.3, 32

.str.4:
	.asciz "(null)"
	.type .str.4, @object
	.size .str.4, 1

.str.5:
	.asciz "\\%c"
	.type .str.5, @object
	.size .str.5, 4

.str.6:
	.asciz "\\%x"
	.type .str.6, @object
	.size .str.6, 4

.str.7:
	.asciz "\\%c"
	.type .str.7, @object
	.size .str.7, 4

.str.8:
	.asciz "\\\\"
	.type .str.8, @object
	.size .str.8, 3

