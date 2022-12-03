	.file   "symbol.c"
	.text
	.option pic
.PCbegin:
	.global StorageIs
	.type StorageIs, @function

StorageIs:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	and         t0, a0, a1
	snez        a0, t0

	// *** Basic block 1

.StorageIs_label_12:
	ret         
.func_end_StorageIs:
	.size StorageIs, .func_end_StorageIs-StorageIs

	.global SymbolInit
	.type SymbolInit, @function

SymbolInit:

	// *** Basic block 0

	.global StringInit
	.global VectorInit
	.global SymbolSetType
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
	mv          s2, a3
	mv          s3, a2
	call        StringInit

	// *** Basic block 1

	sd          x0, 40(s1)
	sw          s2, 48(s1)
	lb          t0, 56(s1)
	andi        t0, t0, -2
	sb          t0, 56(s1)
	lb          t0, 56(s1)
	andi        t0, t0, -3
	sb          t0, 56(s1)
	lb          t0, 56(s1)
	andi        t0, t0, -5
	sb          t0, 56(s1)
	lb          t0, 56(s1)
	andi        t0, t0, -9
	sb          t0, 56(s1)
	lb          t0, 56(s1)
	andi        t0, t0, -17
	sb          t0, 56(s1)
	lb          t0, 56(s1)
	andi        t0, t0, -33
	sb          t0, 56(s1)
	lb          t0, 56(s1)
	andi        t0, t0, -65
	sb          t0, 56(s1)
	lb          t0, 56(s1)
	andi        t0, t0, -129
	sb          t0, 56(s1)
	addi        t0, s1, 56
	lb          t1, 1(t0)
	andi        t1, t1, -2
	sb          t1, 1(t0)
	addi        t0, s1, 56
	lb          t1, 1(t0)
	andi        t1, t1, -3
	sb          t1, 1(t0)
	fmv.d.x     ft0, x0
	fsd         ft0, 112(s1)
	sw          x0, 120(s1)
	sd          x0, 104(s1)
	addi        t0, s1, 64
	sw          x0, 8(t0)
	sw          x0, 64(s1)
	addi        t0, s1, 64
	sw          x0, 4(t0)
	addi        a0, s1, 80
	call        VectorInit

	// *** Basic block 2

	mv          a1, s3
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SymbolSetType
.func_end_SymbolInit:
	.size SymbolInit, .func_end_SymbolInit-SymbolInit

	.global NewSymbol
	.type NewSymbol, @function

NewSymbol:

	// *** Basic block 0

	.global malloc
	.global SymbolInit
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
	li          a0, 136		// 0x88 ASCII \x88
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	mv          a3, s3
	mv          a2, s2
	mv          a1, s1
	mv          a0, s4
	call        SymbolInit

	// *** Basic block 2

	mv          a0, s4

	// *** Basic block 3

.NewSymbol_label_31:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewSymbol:
	.size NewSymbol, .func_end_NewSymbol-NewSymbol

	.global SymbolDestruct
	.type SymbolDestruct, @function

SymbolDestruct:

	// *** Basic block 0

	.global StringDestruct
	.global TypeRecordDelete
	.global VectorDestructWithContents
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
	call        StringDestruct

	// *** Basic block 1

	ld          a0, 40(s1)
	call        TypeRecordDelete

	// *** Basic block 2

	addi        a0, s1, 80
	la          t0, StringDestruct
	ld          a1, 0(t0)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorDestructWithContents
.func_end_SymbolDestruct:
	.size SymbolDestruct, .func_end_SymbolDestruct-SymbolDestruct

	.global SymbolDelete
	.type SymbolDelete, @function

SymbolDelete:

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
	.global SymbolDestruct
	.global free
	mv          s1, a0
	call        SymbolDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_SymbolDelete:
	.size SymbolDelete, .func_end_SymbolDelete-SymbolDelete

	.global SymbolAddAttribute
	.type SymbolAddAttribute, @function

SymbolAddAttribute:

	// *** Basic block 0

	.global VectorAppend
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 80
	j           VectorAppend
.func_end_SymbolAddAttribute:
	.size SymbolAddAttribute, .func_end_SymbolAddAttribute-SymbolAddAttribute

	.global SymbolClone
	.type SymbolClone, @function

SymbolClone:

	// *** Basic block 0

	.global NewSymbol
	.global VectorInit
	.global VectorAppend
	.global NewString
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
	ld          a0, 16(s1)
	ld          a1, 40(s1)
	lw          a2, 48(s1)
	call        NewSymbol

	// *** Basic block 1

	mv          s2, a0
	ld          t0, 56(s1)
	sd          t0, 56(s2)
	ld          t0, 64(s1)
	sd          t0, 64(s2)
	ld          t0, 72(s1)
	sd          t0, 72(s2)
	ld          t0, 112(s1)
	sd          t0, 112(s2)
	lw          t0, 120(s1)
	sw          t0, 120(s2)
	ld          t0, 104(s1)
	sd          t0, 104(s2)
	addi        a0, s2, 80
	call        VectorInit

	// *** Basic block 2

	mv          s3, x0
	addi        t0, s1, 80
	ld          s4, 8(t0)
	bge         x0, s4, .SymbolClone_label_90

	// *** Basic block 3

	ld          s5, 80(s1)

	// *** Basic block 4

.SymbolClone_label_70:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          s1, 0(t0)
	addi        s5, s2, 80
	ld          a0, 16(s1)
	call        NewString

	// *** Basic block 5

	mv          a1, a0
	mv          a0, s5
	call        VectorAppend

	// *** Basic block 6

.SymbolClone_label_86:
	addi        s3, s3, 1
	bge         s3, s4, .SymbolClone_label_70

	// *** Basic block 7

.SymbolClone_label_90:
	mv          a0, s2

	// *** Basic block 8

.SymbolClone_label_93:
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
.func_end_SymbolClone:
	.size SymbolClone, .func_end_SymbolClone-SymbolClone

	.global SymbolHasAttribute
	.type SymbolHasAttribute, @function

SymbolHasAttribute:

	// *** Basic block 0

	.global StringEqual
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
	mv          s2, x0
	addi        t0, a0, 80
	ld          s3, 8(t0)
	bge         x0, s3, .SymbolHasAttribute_label_44

	// *** Basic block 1

	ld          s4, 80(a0)

	// *** Basic block 2

.SymbolHasAttribute_label_24:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          a0, 0(t0)
	mv          a1, s1
	call        StringEqual

	// *** Basic block 3

	beqz        a0, .SymbolHasAttribute_label_39

	// *** Basic block 4

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 5

.SymbolHasAttribute_label_36:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.SymbolHasAttribute_label_39:

	// *** Basic block 7

.SymbolHasAttribute_label_40:
	addi        s2, s2, 1
	bge         s2, s3, .SymbolHasAttribute_label_24

	// *** Basic block 8

.SymbolHasAttribute_label_44:
	mv          a0, x0
	j           .SymbolHasAttribute_label_36
.func_end_SymbolHasAttribute:
	.size SymbolHasAttribute, .func_end_SymbolHasAttribute-SymbolHasAttribute

	.global SymbolPrintDetails
	.type SymbolPrintDetails, @function

SymbolPrintDetails:

	// *** Basic block 0

	.global StringInit
	.global StorageIs
	.global StringAppend
	.local storages
	.global fprintf
	.global TypeRecordPrintDetails
	.global StringDestruct
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -64(s0)
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
	addi        a0, s0, -64
	lla         a1, .str.9
	call        StringInit

	// *** Basic block 1

	lw          s4, 48(s1)
	mv          s5, x0

	// *** Basic block 2

.SymbolPrintDetails_label_41:
	li          t0, 1		// 0x1 ASCII \x1
	sll         a1, t0, s5
	mv          a0, s4
	call        StorageIs

	// *** Basic block 3

	beqz        a0, .SymbolPrintDetails_label_58

	// *** Basic block 4

	addi        a0, s0, -64
	slli        t0, s5, 3
	la          t1, storages
	add         t0, t1, t0
	ld          a1, 0(t0)
	call        StringAppend

	// *** Basic block 5

.SymbolPrintDetails_label_58:

	// *** Basic block 6

.SymbolPrintDetails_label_59:
	addi        s5, s5, 1
	li          t0, 32		// 0x20 ASCII ' '
	bge         s5, t0, .SymbolPrintDetails_label_41

	// *** Basic block 7

.SymbolPrintDetails_label_64:
	lla         a1, .str.10
	ld          a2, 16(s1)
	addi        t0, s0, -64
	ld          a3, 16(t0)
	mv          a0, s2
	call        fprintf

	// *** Basic block 8

	ld          a0, 40(s1)
	mv          a2, s2
	mv          a1, s3
	call        TypeRecordPrintDetails

	// *** Basic block 9

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 10

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
.func_end_SymbolPrintDetails:
	.size SymbolPrintDetails, .func_end_SymbolPrintDetails-SymbolPrintDetails

	.global SymbolPrint
	.type SymbolPrint, @function

SymbolPrint:

	// *** Basic block 0

	.global SymbolPrintDetails
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          a2, t0
	mv          a1, x0
	j           SymbolPrintDetails
.func_end_SymbolPrint:
	.size SymbolPrint, .func_end_SymbolPrint-SymbolPrint

	.global SymbolSetType
	.type SymbolSetType, @function

SymbolSetType:

	// *** Basic block 0

	.global TypeRecordDelete
	.global TypeRecordIncRef
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
	ld          s3, 40(s1)
	bne         s3, s2, .SymbolSetType_label_21

	// *** Basic block 1

.SymbolSetType_label_18:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.SymbolSetType_label_21:
	beq         s3, x0, .SymbolSetType_label_28

	// *** Basic block 3

	mv          a0, s3
	call        TypeRecordDelete

	// *** Basic block 4

.SymbolSetType_label_28:
	sd          s2, 40(s1)
	mv          a0, s2
	call        TypeRecordIncRef

	// *** Basic block 5

	j           .SymbolSetType_label_18
.func_end_SymbolSetType:
	.size SymbolSetType, .func_end_SymbolSetType-SymbolSetType

.PCend:
	.data
storages:
	.type   storages,@object
	.local  storages
	.size   storages,64
	.p2align  3
	.long    .str.1
	.long    .str.2
	.long    .str.3
	.long    .str.4
	.long    .str.5
	.long    .str.6
	.long    .str.7
	.long    .str.8

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "(null)"
	.type .str.1, @object
	.size .str.1, 1

.str.2:
	.asciz "auto "
	.type .str.2, @object
	.size .str.2, 6

.str.3:
	.asciz "static "
	.type .str.3, @object
	.size .str.3, 8

.str.4:
	.asciz "typedef "
	.type .str.4, @object
	.size .str.4, 9

.str.5:
	.asciz "extern "
	.type .str.5, @object
	.size .str.5, 8

.str.6:
	.asciz "register "
	.type .str.6, @object
	.size .str.6, 10

.str.7:
	.asciz "assembler "
	.type .str.7, @object
	.size .str.7, 11

.str.8:
	.asciz "__thread "
	.type .str.8, @object
	.size .str.8, 10

.str.9:
	.asciz "(null)"
	.type .str.9, @object
	.size .str.9, 1

.str.10:
	.asciz "%s: %s"
	.type .str.10, @object
	.size .str.10, 7

