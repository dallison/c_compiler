	.file   "ar.c"
	.text
	.option pic
.PCbegin:
	.global NewARSymbol
	.type NewARSymbol, @function

NewARSymbol:

	// *** Basic block 0

	.global malloc
	.global StringInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	li          a0, 48		// 0x30 ASCII '0'
	call        malloc

	// *** Basic block 1

	sd          a0, -32(s0)
	ld          t0, -32(s0)
	addi        a0, t0, 0
	ld          a1, -24(s0)
	call        StringInit

	// *** Basic block 2

	ld          t0, -32(s0)
	addi        t0, t0, 40
	sd          x0, 0(t0)
	ld          t0, -32(s0)
	mv          a0, t0

	// *** Basic block 3

.NewARSymbol_label_29:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewARSymbol:
	.size NewARSymbol, .func_end_NewARSymbol-NewARSymbol

	.global ARSymbolDestruct
	.type ARSymbolDestruct, @function

ARSymbolDestruct:

	// *** Basic block 0

	.global StringDestruct
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -24(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        a0, t0, 0
	call        StringDestruct

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARSymbolDestruct:
	.size ARSymbolDestruct, .func_end_ARSymbolDestruct-ARSymbolDestruct

	.global ARSymbolDelete
	.type ARSymbolDelete, @function

ARSymbolDelete:

	// *** Basic block 0

	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -24(s0)
	// End of stack frame
	.global ARSymbolDestruct
	.global free
	ld          a0, -24(s0)
	call        ARSymbolDestruct

	// *** Basic block 1

	ld          a0, -24(s0)
	call        free

	// *** Basic block 2

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARSymbolDelete:
	.size ARSymbolDelete, .func_end_ARSymbolDelete-ARSymbolDelete

	.local  HashSymbol
	.type HashSymbol, @function

HashSymbol:

	// *** Basic block 0

	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a2, -24(s0)
	sd a0, -32(s0)
	// Local vars at offset -48(s0)
	// End of stack frame
	lw          t0, -24(s0)
	addi        t0, t0, 0
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .HashSymbol_label_30

	// *** Basic block 2

	j           .HashSymbol_label_21

	// *** Basic block 3

.HashSymbol_label_21:
	ld          t0, -32(s0)
	addi        t0, t0, 0
	addi        t0, t0, 16
	ld          t0, 0(t0)
	sd          t0, -48(s0)
	j           .HashSymbol_label_34

	// *** Basic block 4

.HashSymbol_label_30:
	ld          t0, -32(s0)
	sd          t0, -48(s0)
	j           .HashSymbol_label_34

	// *** Basic block 5

.HashSymbol_label_34:
	li          t0, 5381		// 0x1505
	sw          t0, -40(s0)
	ld          t0, -48(s0)
	lb          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .HashSymbol_label_58

	// *** Basic block 6

.HashSymbol_label_42:
	lwu         t0, -40(s0)
	slli        t0, t0, 5
	lwu         t1, -40(s0)
	add         t0, t0, t1
	ld          t1, -48(s0)
	addi        t1, t1, 1
	sd          t1, -48(s0)
	lb          t1, 0(t1)
	add         t0, t0, t1
	sw          t0, -40(s0)
	ld          t0, -48(s0)
	lb          t0, 0(t0)
	snez        t1, t0
	bnez        t0, .HashSymbol_label_42

	// *** Basic block 7

.HashSymbol_label_58:
	lwu         t0, -40(s0)
	mv          a0, t0

	// *** Basic block 8

.HashSymbol_label_62:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_HashSymbol:
	.size HashSymbol, .func_end_HashSymbol-HashSymbol

	.local  InsertInHashTable
	.type InsertInHashTable, @function

InsertInHashTable:

	// *** Basic block 0

	.global NewVector
	.global VectorAppend
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	sd a2, -32(s0)
	sd a1, -40(s0)
	// Local vars at offset -48(s0)
	// End of stack frame
	ld          t0, -24(s0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .InsertInHashTable_label_19

	// *** Basic block 1

	call        NewVector

	// *** Basic block 2

	sd          a0, -24(s0)
	ld          t0, -32(s0)
	ld          t1, -24(s0)
	sd          t1, 0(t0)

	// *** Basic block 3

.InsertInHashTable_label_19:
	ld          t0, -24(s0)
	sd          t0, -48(s0)
	ld          a0, -48(s0)
	ld          a1, -40(s0)
	call        VectorAppend

	// *** Basic block 4

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0

	// *** Basic block 5

.InsertInHashTable_label_34:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InsertInHashTable:
	.size InsertInHashTable, .func_end_InsertInHashTable-InsertInHashTable

	.local  FindInHashTable
	.type FindInHashTable, @function

FindInHashTable:

	// *** Basic block 0

	.global StringEqual
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -56(s0)
	// End of stack frame
	ld          t0, -24(s0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .FindInHashTable_label_19

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.FindInHashTable_label_16:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.FindInHashTable_label_19:
	ld          t0, -24(s0)
	sd          t0, -56(s0)
	sd          x0, -48(s0)
	ld          t0, -48(s0)
	ld          t1, -56(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindInHashTable_label_69

	// *** Basic block 4

.FindInHashTable_label_31:
	ld          t0, -56(s0)
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -48(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	addi        a0, t0, 0
	ld          a1, -32(s0)
	call        StringEqual

	// *** Basic block 5

	beqz        a0, .FindInHashTable_label_57

	// *** Basic block 6

	ld          t0, -40(s0)
	mv          a0, t0
	j           .FindInHashTable_label_16

	// *** Basic block 7

.FindInHashTable_label_57:

	// *** Basic block 8

.FindInHashTable_label_58:
	ld          t0, -48(s0)
	addi        t0, t0, 1
	sd          t0, -48(s0)
	ld          t0, -48(s0)
	ld          t1, -56(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindInHashTable_label_31

	// *** Basic block 9

.FindInHashTable_label_69:
	mv          a0, x0
	j           .FindInHashTable_label_16
.func_end_FindInHashTable:
	.size FindInHashTable, .func_end_FindInHashTable-FindInHashTable

	.local  DeleteSymbolList
	.type DeleteSymbolList, @function

DeleteSymbolList:

	// *** Basic block 0

	.global ARSymbolDelete
	.global VectorDelete
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -48(s0)
	// End of stack frame
	ld          t0, -24(s0)
	sd          t0, -48(s0)
	sd          x0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -48(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .DeleteSymbolList_label_49

	// *** Basic block 1

.DeleteSymbolList_label_22:
	ld          t0, -48(s0)
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -40(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -32(s0)
	ld          a0, -32(s0)
	call        ARSymbolDelete

	// *** Basic block 2

.DeleteSymbolList_label_38:
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -48(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .DeleteSymbolList_label_22

	// *** Basic block 3

.DeleteSymbolList_label_49:
	ld          a0, -48(s0)
	call        VectorDelete

	// *** Basic block 4

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DeleteSymbolList:
	.size DeleteSymbolList, .func_end_DeleteSymbolList-DeleteSymbolList

	.local  ClearSymbolTable
	.type ClearSymbolTable, @function

ClearSymbolTable:

	// *** Basic block 0

	.global HashTableTraverse
	.local DeleteSymbolList
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -24(s0)
	// End of stack frame
	ld          a0, -24(s0)
	mv          a2, x0
	la          a1, DeleteSymbolList
	call        HashTableTraverse

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ClearSymbolTable:
	.size ClearSymbolTable, .func_end_ClearSymbolTable-ClearSymbolTable

	.local  PrintSymbolList
	.type PrintSymbolList, @function

PrintSymbolList:

	// *** Basic block 0

	.global printf
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -48(s0)
	// End of stack frame
	ld          t0, -24(s0)
	sd          t0, -48(s0)
	sd          x0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -48(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .PrintSymbolList_label_73

	// *** Basic block 1

.PrintSymbolList_label_24:
	ld          t0, -48(s0)
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -40(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -32(s0)
	lla         a0, .str.1
	ld          t0, -32(s0)
	addi        t0, t0, 0
	addi        t0, t0, 16
	ld          a1, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 40
	ld          t0, 0(t0)
	addi        t0, t0, 0
	addi        t0, t0, 16
	ld          a2, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 40
	ld          t0, 0(t0)
	addi        t0, t0, 40
	ld          a3, 0(t0)
	call        printf

	// *** Basic block 2

.PrintSymbolList_label_62:
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -48(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .PrintSymbolList_label_24

	// *** Basic block 3

.PrintSymbolList_label_73:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PrintSymbolList:
	.size PrintSymbolList, .func_end_PrintSymbolList-PrintSymbolList

	.global ARArchivePrintSymbolTable
	.type ARArchivePrintSymbolTable, @function

ARArchivePrintSymbolTable:

	// *** Basic block 0

	.global HashTableTraverse
	.local PrintSymbolList
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -24(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        a0, t0, 112
	mv          a2, x0
	la          a1, PrintSymbolList
	call        HashTableTraverse

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARArchivePrintSymbolTable:
	.size ARArchivePrintSymbolTable, .func_end_ARArchivePrintSymbolTable-ARArchivePrintSymbolTable

	.global ARArchiveInit
	.type ARArchiveInit, @function

ARArchiveInit:

	// *** Basic block 0

	.global StringInit
	.global VectorInit
	.global HashTableInit
	.local HashSymbol
	.local InsertInHashTable
	.local FindInHashTable
	.global MapInitForInt64Keys
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        a0, t0, 0
	ld          a1, -32(s0)
	call        StringInit

	// *** Basic block 1

	ld          t0, -24(s0)
	addi        a0, t0, 40
	call        VectorInit

	// *** Basic block 2

	ld          t0, -24(s0)
	addi        t0, t0, 96
	sd          x0, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 104
	sd          x0, 0(t0)
	ld          t0, -24(s0)
	addi        a0, t0, 112
	lla         a1, .str.2
	la          t0, FindInHashTable
	mv          a5, t0
	la          t0, InsertInHashTable
	mv          a4, t0
	la          t0, HashSymbol
	mv          a3, t0
	li          t0, 1009		// 0x3f1
	mv          a2, t0
	call        HashTableInit

	// *** Basic block 3

	ld          t0, -24(s0)
	addi        a0, t0, 64
	call        MapInitForInt64Keys

	// *** Basic block 4

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARArchiveInit:
	.size ARArchiveInit, .func_end_ARArchiveInit-ARArchiveInit

	.global NewARArchive
	.type NewARArchive, @function

NewARArchive:

	// *** Basic block 0

	.global malloc
	.global ARArchiveInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	li          a0, 200		// 0xc8 ASCII \xc8
	call        malloc

	// *** Basic block 1

	sd          a0, -32(s0)
	ld          a0, -32(s0)
	ld          a1, -24(s0)
	call        ARArchiveInit

	// *** Basic block 2

	ld          t0, -32(s0)
	mv          a0, t0

	// *** Basic block 3

.NewARArchive_label_22:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewARArchive:
	.size NewARArchive, .func_end_NewARArchive-NewARArchive

	.global ARArchiveDestruct
	.type ARArchiveDestruct, @function

ARArchiveDestruct:

	// *** Basic block 0

	.global ARFileDelete
	.global VectorDestruct
	.local ClearSymbolTable
	.global MapDestruct
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	sd          x0, -32(s0)
	ld          t0, -32(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 40
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ARArchiveDestruct_label_51

	// *** Basic block 1

.ARArchiveDestruct_label_25:
	ld          t0, -24(s0)
	addi        t0, t0, 40
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -32(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          a0, 0(t0)
	call        ARFileDelete

	// *** Basic block 2

.ARArchiveDestruct_label_39:
	ld          t0, -32(s0)
	addi        t0, t0, 1
	sd          t0, -32(s0)
	ld          t0, -32(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 40
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ARArchiveDestruct_label_25

	// *** Basic block 3

.ARArchiveDestruct_label_51:
	ld          t0, -24(s0)
	addi        a0, t0, 40
	call        VectorDestruct

	// *** Basic block 4

	ld          t0, -24(s0)
	addi        a0, t0, 112
	call        ClearSymbolTable

	// *** Basic block 5

	ld          t0, -24(s0)
	addi        a0, t0, 64
	call        MapDestruct

	// *** Basic block 6

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARArchiveDestruct:
	.size ARArchiveDestruct, .func_end_ARArchiveDestruct-ARArchiveDestruct

	.global ARArchiveDelete
	.type ARArchiveDelete, @function

ARArchiveDelete:

	// *** Basic block 0

	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -24(s0)
	// End of stack frame
	.global ARArchiveDestruct
	.global free
	ld          a0, -24(s0)
	call        ARArchiveDestruct

	// *** Basic block 1

	ld          a0, -24(s0)
	call        free

	// *** Basic block 2

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARArchiveDelete:
	.size ARArchiveDelete, .func_end_ARArchiveDelete-ARArchiveDelete

	.local  ReadFilename
	.type ReadFilename, @function

ReadFilename:

	// *** Basic block 0

	.global StringSet
	.global StringAppendChar
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a1, -24(s0)
	sd a0, -32(s0)
	// Local vars at offset -40(s0)
	// End of stack frame
	ld          t0, -24(s0)
	lb          t0, 0(t0)
	addi        t1, t0, -47
	seqz        t1, t1
	li          t1, 47		// 0x2f ASCII '/'
	bne         t0, t1, .ReadFilename_label_80

	// *** Basic block 1

	ld          t0, -24(s0)
	addi        t0, t0, 1
	lb          t0, 0(t0)
	addi        t1, t0, -47
	seqz        t1, t1
	li          t1, 47		// 0x2f ASCII '/'
	bne         t0, t1, .ReadFilename_label_36

	// *** Basic block 2

	ld          a0, -32(s0)
	lla         a1, .str.3
	call        StringSet

	// *** Basic block 3

	j           .ReadFilename_label_76

	// *** Basic block 4

.ReadFilename_label_36:
	ld          t0, -24(s0)
	addi        t0, t0, 1
	lb          t0, 0(t0)
	addi        t1, t0, -32
	seqz        t1, t1
	li          t1, 32		// 0x20 ASCII ' '
	bne         t0, t1, .ReadFilename_label_51

	// *** Basic block 5

	ld          a0, -32(s0)
	lla         a1, .str.4
	call        StringSet

	// *** Basic block 6

	j           .ReadFilename_label_75

	// *** Basic block 7

.ReadFilename_label_51:

	// *** Basic block 8

.ReadFilename_label_52:
	ld          t0, -24(s0)
	addi        t0, t0, 1
	sd          t0, -24(s0)
	lb          t0, 0(t0)
	sb          t0, -40(s0)
	lb          t0, -40(s0)
	addi        t1, t0, -32
	seqz        t1, t1
	li          t1, 32		// 0x20 ASCII ' '
	bne         t0, t1, .ReadFilename_label_66

	// *** Basic block 9

	j           .ReadFilename_label_74

	// *** Basic block 10

.ReadFilename_label_66:
	ld          a0, -32(s0)
	lb          a1, -40(s0)
	call        StringAppendChar

	// *** Basic block 11

.ReadFilename_label_72:
	j           .ReadFilename_label_52

	// *** Basic block 12

.ReadFilename_label_74:

	// *** Basic block 13

.ReadFilename_label_75:

	// *** Basic block 14

.ReadFilename_label_76:

	// *** Basic block 15

.ReadFilename_label_77:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 16

.ReadFilename_label_80:

	// *** Basic block 17

.ReadFilename_label_81:
	ld          t0, -24(s0)
	addi        t0, t0, 1
	sd          t0, -24(s0)
	lb          t0, 0(t0)
	sb          t0, -39(s0)
	lb          t0, -39(s0)
	addi        t1, t0, -47
	seqz        t1, t1
	li          t1, 47		// 0x2f ASCII '/'
	bne         t0, t1, .ReadFilename_label_96

	// *** Basic block 18

.ReadFilename_label_94:
	j           .ReadFilename_label_77

	// *** Basic block 19

.ReadFilename_label_96:
	ld          a0, -32(s0)
	lb          a1, -39(s0)
	call        StringAppendChar

	// *** Basic block 20

.ReadFilename_label_102:
	j           .ReadFilename_label_81
.func_end_ReadFilename:
	.size ReadFilename, .func_end_ReadFilename-ReadFilename

	.local  ReadInteger
	.type ReadInteger, @function

ReadInteger:

	// *** Basic block 0

	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a1, -24(s0)
	sd a0, -32(s0)
	// Local vars at offset -48(s0)
	// End of stack frame
	sd          x0, -48(s0)
	sw          x0, -40(s0)
	lw          t0, -40(s0)
	lw          t1, -24(s0)
	slt         t2, t0, t1
	bge         t0, t1, .ReadInteger_label_52

	// *** Basic block 1

.ReadInteger_label_19:
	ld          t0, -32(s0)
	addi        t0, t0, 1
	sd          t0, -32(s0)
	lb          t0, 0(t0)
	sw          t0, -36(s0)
	lw          t0, -36(s0)
	addi        t1, t0, -32
	seqz        t1, t1
	li          t1, 32		// 0x20 ASCII ' '
	bne         t0, t1, .ReadInteger_label_34

	// *** Basic block 2

	j           .ReadInteger_label_52

	// *** Basic block 3

.ReadInteger_label_34:
	ld          t0, -48(s0)
	slli        t1, t0, 1
	slli        t0, t0, 3
	add         t0, t1, t0
	lw          t1, -36(s0)
	add         t0, t0, t1
	addi        t0, t0, -48
	sd          t0, -48(s0)
	lw          t0, -40(s0)
	addi        t0, t0, 1
	sw          t0, -40(s0)
	lw          t0, -40(s0)
	lw          t1, -24(s0)
	slt         t2, t0, t1
	blt         t0, t1, .ReadInteger_label_19

	// *** Basic block 4

.ReadInteger_label_52:
	ld          t0, -48(s0)
	mv          a0, t0

	// *** Basic block 5

.ReadInteger_label_56:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ReadInteger:
	.size ReadInteger, .func_end_ReadInteger-ReadInteger

	.local  ReadBigEndianInt
	.type ReadBigEndianInt, @function

ReadBigEndianInt:

	// *** Basic block 0

	.global fgetc
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	sw          x0, -32(s0)
	sw          x0, -28(s0)
	lw          t0, -28(s0)
	slti        t1, t0, 4
	li          t1, 4		// 0x4 ASCII \x4
	bge         t0, t1, .ReadBigEndianInt_label_38

	// *** Basic block 1

.ReadBigEndianInt_label_18:
	lw          t0, -32(s0)
	slli        s1, t0, 8
	ld          a0, -24(s0)
	call        fgetc

	// *** Basic block 2

	andi        t0, a0, 255
	or          t0, s1, t0
	sw          t0, -32(s0)

	// *** Basic block 3

.ReadBigEndianInt_label_29:
	lw          t0, -28(s0)
	addi        t0, t0, 1
	sw          t0, -28(s0)
	lw          t0, -28(s0)
	slti        t1, t0, 4
	li          t1, 4		// 0x4 ASCII \x4
	bge         t0, t1, .ReadBigEndianInt_label_18

	// *** Basic block 4

.ReadBigEndianInt_label_38:
	lw          t0, -32(s0)
	mv          a0, t0

	// *** Basic block 5

.ReadBigEndianInt_label_42:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ReadBigEndianInt:
	.size ReadBigEndianInt, .func_end_ReadBigEndianInt-ReadBigEndianInt

	.local  ReadSymbolName
	.type ReadSymbolName, @function

ReadSymbolName:

	// *** Basic block 0

	.global feof
	.global fgetc
	.global StringAppendChar
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a1, -24(s0)
	sd a0, -32(s0)
	// Local vars at offset -40(s0)
	// End of stack frame
	ld          a0, -24(s0)
	call        feof

	// *** Basic block 1

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         t0, t0
	beqz        t0, .ReadSymbolName_label_52

	// *** Basic block 2

.ReadSymbolName_label_19:
	ld          a0, -24(s0)
	call        fgetc

	// *** Basic block 3

	sw          a0, -40(s0)
	lw          t0, -40(s0)
	addi        t1, t0, 1
	seqz        t1, t1
	li          t1, -1		// 0xffffffffffffffff
	bne         t0, t1, .ReadSymbolName_label_32

	// *** Basic block 4

	j           .ReadSymbolName_label_52

	// *** Basic block 5

.ReadSymbolName_label_32:
	lw          t0, -40(s0)
	seqz        t1, t0
	bnez        t0, .ReadSymbolName_label_37

	// *** Basic block 6

	j           .ReadSymbolName_label_52

	// *** Basic block 7

.ReadSymbolName_label_37:
	ld          a0, -32(s0)
	lw          a1, -40(s0)
	call        StringAppendChar

	// *** Basic block 8

	ld          a0, -24(s0)
	call        feof

	// *** Basic block 9

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         t0, t0
	bnez        t0, .ReadSymbolName_label_19

	// *** Basic block 10

.ReadSymbolName_label_52:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ReadSymbolName:
	.size ReadSymbolName, .func_end_ReadSymbolName-ReadSymbolName

	.local  ReadSymbolTable
	.type ReadSymbolTable, @function

ReadSymbolTable:

	// *** Basic block 0

	.global fseek
	.local ReadBigEndianInt
	.global NewARSymbol
	.global MapFindInt64Key
	.global fprintf
	.global stderr
	.global exit
	.global VectorAppend
	.local ReadSymbolName
	.global HashTableInsert
	.global VectorDestruct
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Saved argument registers.
	sd a1, -24(s0)
	sd a0, -32(s0)
	// Local vars at offset -112(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	ld          a0, -24(s0)
	ld          t0, -32(s0)
	addi        t0, t0, 104
	ld          t0, 0(t0)
	addi        t0, t0, 40
	ld          a1, 0(t0)
	li          a2, 2		// 0x2 ASCII \x2
	call        fseek

	// *** Basic block 1

	ld          a0, -24(s0)
	call        ReadBigEndianInt

	// *** Basic block 2

	sw          a0, -112(s0)
	sd          x0, -104(s0)
	sd          x0, -96(s0)
	sd          x0, -88(s0)
	sd          x0, -104(s0)
	sw          x0, -80(s0)
	lw          t0, -80(s0)
	lw          t1, -112(s0)
	slt         t2, t0, t1
	bge         t0, t1, .ReadSymbolTable_label_115

	// *** Basic block 3

.ReadSymbolTable_label_62:
	ld          a0, -24(s0)
	call        ReadBigEndianInt

	// *** Basic block 4

	sd          a0, -72(s0)
	lla         a0, .str.5
	call        NewARSymbol

	// *** Basic block 5

	sd          a0, -64(s0)
	ld          t0, -64(s0)
	addi        s1, t0, 40
	ld          t0, -32(s0)
	addi        a0, t0, 64
	ld          a1, -72(s0)
	call        MapFindInt64Key

	// *** Basic block 6

	sd          a0, 0(s1)
	ld          t0, -64(s0)
	addi        t0, t0, 40
	ld          t0, 0(t0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .ReadSymbolTable_label_100

	// *** Basic block 7

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.6
	call        fprintf

	// *** Basic block 8

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	call        exit

	// *** Basic block 9

.ReadSymbolTable_label_100:
	addi        a0, s0, -104
	ld          a1, -64(s0)
	call        VectorAppend

	// *** Basic block 10

.ReadSymbolTable_label_106:
	lw          t0, -80(s0)
	addi        t0, t0, 1
	sw          t0, -80(s0)
	lw          t0, -80(s0)
	lw          t1, -112(s0)
	slt         t2, t0, t1
	bge         t0, t1, .ReadSymbolTable_label_62

	// *** Basic block 11

.ReadSymbolTable_label_115:
	sw          x0, -56(s0)
	lw          t0, -56(s0)
	lw          t1, -112(s0)
	slt         t2, t0, t1
	bge         t0, t1, .ReadSymbolTable_label_149

	// *** Basic block 12

.ReadSymbolTable_label_122:
	addi        t0, s0, -104
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	lw          t1, -56(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -48(s0)
	ld          t0, -48(s0)
	addi        a0, t0, 0
	ld          a1, -24(s0)
	call        ReadSymbolName

	// *** Basic block 13

.ReadSymbolTable_label_140:
	lw          t0, -56(s0)
	addi        t0, t0, 1
	sw          t0, -56(s0)
	lw          t0, -56(s0)
	lw          t1, -112(s0)
	slt         t2, t0, t1
	bge         t0, t1, .ReadSymbolTable_label_122

	// *** Basic block 14

.ReadSymbolTable_label_149:
	sd          x0, -40(s0)
	ld          t0, -40(s0)
	addi        t1, s0, -104
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ReadSymbolTable_label_183

	// *** Basic block 15

.ReadSymbolTable_label_158:
	ld          t0, -32(s0)
	addi        a0, t0, 112
	addi        t0, s0, -104
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -40(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          a1, 0(t0)
	call        HashTableInsert

	// *** Basic block 16

.ReadSymbolTable_label_172:
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	addi        t1, s0, -104
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ReadSymbolTable_label_158

	// *** Basic block 17

.ReadSymbolTable_label_183:
	addi        a0, s0, -104
	call        VectorDestruct

	// *** Basic block 18

	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ReadSymbolTable:
	.size ReadSymbolTable, .func_end_ReadSymbolTable-ReadSymbolTable

	.local  ReadFileHeaders
	.type ReadFileHeaders, @function

ReadFileHeaders:

	// *** Basic block 0

	.global feof
	.global ftell
	.global fread
	.global memcmp
	.global fprintf
	.global stderr
	.global NewARFile
	.local ReadFilename
	.local ReadInteger
	.global StringEqual
	.global VectorAppend
	.global MapInsert
	.global fseek
	addi sp, sp, -160
	// Saved return address (offset 152) and frame pointer (offset 144)
	sd ra, 152(sp)
	sd s0, 144(sp)
	addi s0, sp, 160
	// Saved argument registers.
	sd a1, -24(s0)
	sd a0, -32(s0)
	// Local vars at offset -144(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	ld          a0, -24(s0)
	call        feof

	// *** Basic block 1

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         t0, t0
	beqz        t0, .ReadFileHeaders_label_259

	// *** Basic block 2

.ReadFileHeaders_label_51:
	ld          a0, -24(s0)
	call        ftell

	// *** Basic block 3

	sd          a0, -144(s0)
	addi        a0, s0, -128
	ld          a3, -24(s0)
	li          t0, 64		// 0x40 ASCII '@'
	mv          a2, t0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        fread

	// *** Basic block 4

	sd          a0, -136(s0)
	ld          t0, -136(s0)
	slti        t1, t0, 1
	li          t1, 1		// 0x1 ASCII \x1
	bge         t0, t1, .ReadFileHeaders_label_79

	// *** Basic block 5

	j           .ReadFileHeaders_label_259

	// *** Basic block 6

.ReadFileHeaders_label_79:
	addi        t0, s0, -128
	addi        a0, t0, 58
	lla         a1, .str.7
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	call        memcmp

	// *** Basic block 7

	snez        t0, a0
	beqz        a0, .ReadFileHeaders_label_105

	// *** Basic block 8

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.8
	call        fprintf

	// *** Basic block 9

	mv          a0, x0

	// *** Basic block 10

.ReadFileHeaders_label_102:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 11

.ReadFileHeaders_label_105:
	lla         a0, .str.9
	call        NewARFile

	// *** Basic block 12

	sd          a0, -64(s0)
	ld          t0, -64(s0)
	addi        a0, t0, 0
	addi        t0, s0, -128
	addi        a1, t0, 0
	call        ReadFilename

	// *** Basic block 13

	ld          t0, -64(s0)
	addi        s1, t0, 48
	addi        t0, s0, -128
	addi        a0, t0, 48
	li          t0, 10		// 0xa ASCII \xa
	mv          a1, t0
	call        ReadInteger

	// *** Basic block 14

	sd          a0, 0(s1)
	ld          s1, -64(s0)
	addi        s1, s1, 56
	addi        t0, s0, -128
	addi        a0, t0, 28
	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	call        ReadInteger

	// *** Basic block 15

	sext.w      t0, a0
	sw          t0, 0(s1)
	ld          t0, -64(s0)
	addi        s1, t0, 60
	addi        t0, s0, -128
	addi        a0, t0, 34
	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	call        ReadInteger

	// *** Basic block 16

	sext.w      t0, a0
	sw          t0, 0(s1)
	ld          t0, -64(s0)
	addi        s1, t0, 64
	addi        t0, s0, -128
	addi        a0, t0, 40
	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	call        ReadInteger

	// *** Basic block 17

	sext.w      t0, a0
	sw          t0, 0(s1)
	ld          t0, -64(s0)
	addi        s1, t0, 72
	addi        t0, s0, -128
	addi        a0, t0, 16
	li          t0, 12		// 0xc ASCII \xc
	mv          a1, t0
	call        ReadInteger

	// *** Basic block 18

	sd          a0, 0(s1)
	ld          t0, -64(s0)
	addi        s1, t0, 40
	ld          a0, -24(s0)
	call        ftell

	// *** Basic block 19

	sd          a0, 0(s1)
	ld          t0, -64(s0)
	addi        a0, t0, 0
	lla         a1, .str.10
	call        StringEqual

	// *** Basic block 20

	beqz        a0, .ReadFileHeaders_label_193

	// *** Basic block 21

	ld          t0, -32(s0)
	addi        t0, t0, 96
	ld          t1, -64(s0)
	addi        t1, t1, 40
	ld          t1, 0(t1)
	sd          t1, 0(t0)

	// *** Basic block 22

.ReadFileHeaders_label_193:
	ld          t0, -64(s0)
	addi        a0, t0, 0
	lla         a1, .str.11
	call        StringEqual

	// *** Basic block 23

	beqz        a0, .ReadFileHeaders_label_206

	// *** Basic block 24

	ld          t0, -32(s0)
	addi        t0, t0, 104
	ld          t1, -64(s0)
	sd          t1, 0(t0)

	// *** Basic block 25

.ReadFileHeaders_label_206:
	ld          t0, -32(s0)
	addi        a0, t0, 40
	ld          a1, -64(s0)
	call        VectorAppend

	// *** Basic block 26

	addi        t0, s0, -56
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t1, -144(s0)
	sd          t1, 0(t0)
	addi        t0, s0, -56
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t1, -64(s0)
	sd          t1, 0(t0)
	ld          t0, -32(s0)
	addi        a0, t0, 64
	addi        t0, s0, -56
	addi        sp, sp, -16
	ld          t1, 0(t0)
	sd          t1, 0(sp)
	ld          t0, 8(t0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 27

	addi        sp, sp, 16
	ld          t0, -64(s0)
	addi        t0, t0, 48
	ld          t0, 0(t0)
	addi        s1, t0, 1
	andi        t0, s1, -2
	sd          t0, -40(s0)
	ld          a0, -24(s0)
	ld          a1, -40(s0)
	mv          a2, x0
	call        fseek

	// *** Basic block 28

	ld          a0, -24(s0)
	call        feof

	// *** Basic block 29

	slli        t0, a0, 56
	srai        t0, t0, 56
	not         t0, t0
	bnez        t0, .ReadFileHeaders_label_51

	// *** Basic block 30

.ReadFileHeaders_label_259:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .ReadFileHeaders_label_102
.func_end_ReadFileHeaders:
	.size ReadFileHeaders, .func_end_ReadFileHeaders-ReadFileHeaders

	.local  ReadExtendedFilenames
	.type ReadExtendedFilenames, @function

ReadExtendedFilenames:

	// *** Basic block 0

	.global isdigit
	.global fseek
	.global StringClear
	.global fgetc
	.global StringAppendChar
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -72(s0)
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	sd          x0, -72(s0)
	ld          t0, -72(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 40
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ReadExtendedFilenames_label_183

	// *** Basic block 1

.ReadExtendedFilenames_label_36:
	ld          t0, -24(s0)
	addi        t0, t0, 40
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -72(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -64(s0)
	ld          t0, -64(s0)
	addi        t0, t0, 0
	addi        t0, t0, 16
	ld          t0, 0(t0)
	addi        t0, t0, 0
	lb          t0, 0(t0)
	addi        t1, t0, -47
	seqz        s1, t1
	li          t1, 47		// 0x2f ASCII '/'
	bne         t0, t1, .ReadExtendedFilenames_label_71

	// *** Basic block 2

	ld          t0, -64(s0)
	addi        t0, t0, 0
	addi        t0, t0, 16
	ld          t0, 0(t0)
	addi        t0, t0, 1
	lb          a0, 0(t0)
	call        isdigit

	// *** Basic block 3

	mv          s1, a0

	// *** Basic block 4

.ReadExtendedFilenames_label_71:
	beqz        s1, .ReadExtendedFilenames_label_170

	// *** Basic block 5

	sd          x0, -56(s0)
	li          t0, 1		// 0x1 ASCII \x1
	sd          t0, -48(s0)
	ld          t0, -48(s0)
	ld          t1, -64(s0)
	addi        t1, t1, 0
	addi        t1, t1, 24
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ReadExtendedFilenames_label_113

	// *** Basic block 6

.ReadExtendedFilenames_label_85:
	ld          t0, -56(s0)
	slli        t1, t0, 1
	slli        t0, t0, 3
	add         t0, t1, t0
	ld          t1, -64(s0)
	addi        t1, t1, 0
	addi        t1, t1, 16
	ld          t1, 0(t1)
	ld          t2, -48(s0)
	add         t1, t1, t2
	lb          t1, 0(t1)
	add         t0, t0, t1
	addi        t0, t0, -48
	sd          t0, -56(s0)

	// *** Basic block 7

.ReadExtendedFilenames_label_101:
	ld          t0, -48(s0)
	addi        t0, t0, 1
	sd          t0, -48(s0)
	ld          t0, -48(s0)
	ld          t1, -64(s0)
	addi        t1, t1, 0
	addi        t1, t1, 24
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ReadExtendedFilenames_label_85

	// *** Basic block 8

.ReadExtendedFilenames_label_113:
	ld          a0, -32(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 96
	ld          t0, 0(t0)
	ld          t1, -56(s0)
	add         t0, t0, t1
	sext.w      a1, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	call        fseek

	// *** Basic block 9

	ld          t0, -64(s0)
	addi        a0, t0, 0
	call        StringClear

	// *** Basic block 10

.ReadExtendedFilenames_label_134:
	ld          a0, -32(s0)
	call        fgetc

	// *** Basic block 11

	sw          a0, -40(s0)
	lw          t0, -40(s0)
	addi        t1, t0, 1
	seqz        t1, t1
	li          t1, -1		// 0xffffffffffffffff
	bne         t0, t1, .ReadExtendedFilenames_label_146

	// *** Basic block 12

	j           .ReadExtendedFilenames_label_169

	// *** Basic block 13

.ReadExtendedFilenames_label_146:
	lw          t1, -40(s0)
	addi        t2, t1, -47
	seqz        t0, t2
	li          t2, 47		// 0x2f ASCII '/'
	beq         t1, t2, .ReadExtendedFilenames_label_157

	// *** Basic block 14

	lw          t1, -40(s0)
	addi        t1, t1, -10
	seqz        t0, t1

	// *** Basic block 15

.ReadExtendedFilenames_label_157:
	beqz        t0, .ReadExtendedFilenames_label_160

	// *** Basic block 16

	j           .ReadExtendedFilenames_label_169

	// *** Basic block 17

.ReadExtendedFilenames_label_160:
	ld          t0, -64(s0)
	addi        a0, t0, 0
	lw          a1, -40(s0)
	call        StringAppendChar

	// *** Basic block 18

.ReadExtendedFilenames_label_167:
	j           .ReadExtendedFilenames_label_134

	// *** Basic block 19

.ReadExtendedFilenames_label_169:

	// *** Basic block 20

.ReadExtendedFilenames_label_170:

	// *** Basic block 21

.ReadExtendedFilenames_label_171:
	ld          t0, -72(s0)
	addi        t0, t0, 1
	sd          t0, -72(s0)
	ld          t0, -72(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 40
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ReadExtendedFilenames_label_36

	// *** Basic block 22

.ReadExtendedFilenames_label_183:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ReadExtendedFilenames:
	.size ReadExtendedFilenames, .func_end_ReadExtendedFilenames-ReadExtendedFilenames

	.global ARArchiveOpen
	.type ARArchiveOpen, @function

ARArchiveOpen:

	// *** Basic block 0

	.global fread
	.global memcmp
	.local ReadFileHeaders
	.local ReadExtendedFilenames
	.local ReadSymbolTable
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Saved argument registers.
	sd a1, -24(s0)
	sd a0, -32(s0)
	// Local vars at offset -56(s0)
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	addi        a0, s0, -48
	ld          a3, -24(s0)
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	li          a1, 1		// 0x1 ASCII \x1
	call        fread

	// *** Basic block 1

	sd          a0, -56(s0)
	ld          t0, -56(s0)
	addi        t1, t0, -8
	snez        s1, t1
	li          t1, 8		// 0x8 ASCII \x8
	bne         t0, t1, .ARArchiveOpen_label_49

	// *** Basic block 2

	addi        a0, s0, -48
	lla         a1, .str.12
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	call        memcmp

	// *** Basic block 3

	snez        s1, a0

	// *** Basic block 4

.ARArchiveOpen_label_49:
	beqz        s1, .ARArchiveOpen_label_57

	// *** Basic block 5

	mv          a0, x0

	// *** Basic block 6

.ARArchiveOpen_label_54:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.ARArchiveOpen_label_57:
	ld          a0, -32(s0)
	ld          a1, -24(s0)
	call        ReadFileHeaders

	// *** Basic block 8

	sb          a0, -40(s0)
	lb          t0, -40(s0)
	not         t0, t0
	beqz        t0, .ARArchiveOpen_label_73

	// *** Basic block 9

	lb          t0, -40(s0)
	mv          a0, t0
	j           .ARArchiveOpen_label_54

	// *** Basic block 10

.ARArchiveOpen_label_73:
	ld          a0, -32(s0)
	ld          a1, -24(s0)
	call        ReadExtendedFilenames

	// *** Basic block 11

	ld          t0, -32(s0)
	addi        t0, t0, 104
	ld          t0, 0(t0)
	sub         t1, t0, x0
	snez        t1, t1
	beq         t0, x0, .ARArchiveOpen_label_90

	// *** Basic block 12

	ld          a0, -32(s0)
	ld          a1, -24(s0)
	call        ReadSymbolTable

	// *** Basic block 13

.ARArchiveOpen_label_90:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .ARArchiveOpen_label_54
.func_end_ARArchiveOpen:
	.size ARArchiveOpen, .func_end_ARArchiveOpen-ARArchiveOpen

	.global NewARFile
	.type NewARFile, @function

NewARFile:

	// *** Basic block 0

	.global malloc
	.global StringInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	li          a0, 104		// 0x68 ASCII 'h'
	call        malloc

	// *** Basic block 1

	sd          a0, -32(s0)
	ld          t0, -32(s0)
	addi        a0, t0, 0
	ld          a1, -24(s0)
	call        StringInit

	// *** Basic block 2

	ld          t0, -32(s0)
	addi        t0, t0, 40
	sd          x0, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 48
	sd          x0, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 56
	sw          x0, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 60
	sw          x0, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 64
	sw          x0, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 72
	sd          x0, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 80
	sd          x0, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 88
	sb          x0, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 89
	sb          x0, 0(t0)
	ld          t0, -32(s0)
	mv          a0, t0

	// *** Basic block 3

.NewARFile_label_62:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewARFile:
	.size NewARFile, .func_end_NewARFile-NewARFile

	.global ARFileDestruct
	.type ARFileDestruct, @function

ARFileDestruct:

	// *** Basic block 0

	.global StringDestruct
	.global free
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -24(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        a0, t0, 0
	call        StringDestruct

	// *** Basic block 1

	ld          t0, -24(s0)
	addi        t0, t0, 89
	lb          t0, 0(t0)
	beqz        t0, .ARFileDestruct_label_24

	// *** Basic block 2

	ld          t0, -24(s0)
	addi        t0, t0, 80
	ld          a0, 0(t0)
	call        free

	// *** Basic block 3

.ARFileDestruct_label_24:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARFileDestruct:
	.size ARFileDestruct, .func_end_ARFileDestruct-ARFileDestruct

	.global ARFileDelete
	.type ARFileDelete, @function

ARFileDelete:

	// *** Basic block 0

	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -24(s0)
	// End of stack frame
	.global ARFileDestruct
	.global free
	ld          a0, -24(s0)
	call        ARFileDestruct

	// *** Basic block 1

	ld          a0, -24(s0)
	call        free

	// *** Basic block 2

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARFileDelete:
	.size ARFileDelete, .func_end_ARFileDelete-ARFileDelete

	.global ARFileCopyFromArchive
	.type ARFileCopyFromArchive, @function

ARFileCopyFromArchive:

	// *** Basic block 0

	.global NewARFile
	.global malloc
	.global fseek
	.global fread
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -40(s0)
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 0
	addi        t0, t0, 16
	ld          a0, 0(t0)
	call        NewARFile

	// *** Basic block 1

	sd          a0, -40(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 48
	ld          t1, -24(s0)
	addi        t1, t1, 48
	ld          t1, 0(t1)
	sd          t1, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 56
	ld          t1, -24(s0)
	addi        t1, t1, 56
	lw          t1, 0(t1)
	sw          t1, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 60
	ld          t1, -24(s0)
	addi        t1, t1, 60
	lw          t1, 0(t1)
	sw          t1, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 64
	ld          t1, -24(s0)
	addi        t1, t1, 64
	lw          t1, 0(t1)
	sw          t1, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 72
	ld          t1, -24(s0)
	addi        t1, t1, 72
	ld          t1, 0(t1)
	sd          t1, 0(t0)
	ld          t0, -40(s0)
	addi        s1, t0, 80
	ld          t0, -40(s0)
	addi        t0, t0, 48
	ld          a0, 0(t0)
	call        malloc

	// *** Basic block 2

	sd          a0, 0(s1)
	ld          a0, -32(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 40
	ld          a1, 0(t0)
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	call        fseek

	// *** Basic block 3

	ld          t0, -40(s0)
	addi        t0, t0, 80
	ld          a0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 48
	ld          a2, 0(t0)
	ld          a3, -32(s0)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        fread

	// *** Basic block 4

	ld          t0, -40(s0)
	addi        t0, t0, 89
	li          t1, 1		// 0x1 ASCII \x1
	sb          t1, 0(t0)
	ld          t0, -40(s0)
	mv          a0, t0

	// *** Basic block 5

.ARFileCopyFromArchive_label_105:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARFileCopyFromArchive:
	.size ARFileCopyFromArchive, .func_end_ARFileCopyFromArchive-ARFileCopyFromArchive

	.global ARArchiveFindSymbol
	.type ARArchiveFindSymbol, @function

ARArchiveFindSymbol:

	// *** Basic block 0

	.global HashTableSearch
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        a0, t0, 112
	ld          a1, -32(s0)
	call        HashTableSearch

	// *** Basic block 1


	// *** Basic block 2

.ARArchiveFindSymbol_label_17:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARArchiveFindSymbol:
	.size ARArchiveFindSymbol, .func_end_ARArchiveFindSymbol-ARArchiveFindSymbol

	.global NewARArchiveBuilder
	.type NewARArchiveBuilder, @function

NewARArchiveBuilder:

	// *** Basic block 0

	.global malloc
	.global ARArchiveBuilderInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	li          a0, 168		// 0xa8 ASCII \xa8
	call        malloc

	// *** Basic block 1

	sd          a0, -32(s0)
	ld          a0, -32(s0)
	ld          a1, -24(s0)
	call        ARArchiveBuilderInit

	// *** Basic block 2

	ld          t0, -32(s0)
	mv          a0, t0

	// *** Basic block 3

.NewARArchiveBuilder_label_22:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewARArchiveBuilder:
	.size NewARArchiveBuilder, .func_end_NewARArchiveBuilder-NewARArchiveBuilder

	.global ARArchiveBuilderInit
	.type ARArchiveBuilderInit, @function

ARArchiveBuilderInit:

	// *** Basic block 0

	.global StringInit
	.global VectorInit
	.global MapInitForInt64Keys
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        a0, t0, 0
	ld          a1, -32(s0)
	call        StringInit

	// *** Basic block 1

	ld          t0, -24(s0)
	addi        a0, t0, 40
	call        VectorInit

	// *** Basic block 2

	ld          t0, -24(s0)
	addi        a0, t0, 64
	call        VectorInit

	// *** Basic block 3

	ld          t0, -24(s0)
	addi        a0, t0, 88
	call        VectorInit

	// *** Basic block 4

	ld          t0, -24(s0)
	addi        t0, t0, 112
	sd          x0, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 152
	sw          x0, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 160
	sd          x0, 0(t0)
	ld          t0, -24(s0)
	addi        a0, t0, 120
	call        MapInitForInt64Keys

	// *** Basic block 5

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARArchiveBuilderInit:
	.size ARArchiveBuilderInit, .func_end_ARArchiveBuilderInit-ARArchiveBuilderInit

	.global ARArchiveBuilderDestruct
	.type ARArchiveBuilderDestruct, @function

ARArchiveBuilderDestruct:

	// *** Basic block 0

	.global StringDestruct
	.global VectorDestructWithContents
	.global ARFileDestruct
	.global ARSymbolDestruct
	.global VectorDestruct
	.global MapDestruct
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -24(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        a0, t0, 0
	call        StringDestruct

	// *** Basic block 1

	ld          t0, -24(s0)
	addi        a0, t0, 40
	la          t0, ARFileDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 2

	ld          t0, -24(s0)
	addi        a0, t0, 64
	la          t0, ARSymbolDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 3

	ld          t0, -24(s0)
	addi        a0, t0, 88
	call        VectorDestruct

	// *** Basic block 4

	ld          t0, -24(s0)
	addi        a0, t0, 120
	call        MapDestruct

	// *** Basic block 5

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARArchiveBuilderDestruct:
	.size ARArchiveBuilderDestruct, .func_end_ARArchiveBuilderDestruct-ARArchiveBuilderDestruct

	.global ARArchiveBuilderDelete
	.type ARArchiveBuilderDelete, @function

ARArchiveBuilderDelete:

	// *** Basic block 0

	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -24(s0)
	// End of stack frame
	.global ARArchiveBuilderDestruct
	.global free
	ld          a0, -24(s0)
	call        ARArchiveBuilderDestruct

	// *** Basic block 1

	ld          a0, -24(s0)
	call        free

	// *** Basic block 2

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARArchiveBuilderDelete:
	.size ARArchiveBuilderDelete, .func_end_ARArchiveBuilderDelete-ARArchiveBuilderDelete

	.global ARArchiveBuilderAddFile
	.type ARArchiveBuilderAddFile, @function

ARArchiveBuilderAddFile:

	// *** Basic block 0

	.global NewARFile
	.global VectorAppend
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Saved argument registers.
	sd a1, -24(s0)
	sd a2, -32(s0)
	sd a3, -40(s0)
	sd a4, -48(s0)
	sd a5, -56(s0)
	sd a6, -64(s0)
	sd a7, -72(s0)
	sd a0, -80(s0)
	// Local vars at offset -88(s0)
	// End of stack frame
	ld          a0, -24(s0)
	call        NewARFile

	// *** Basic block 1

	sd          a0, -88(s0)
	ld          t0, -88(s0)
	addi        t0, t0, 48
	ld          t1, -32(s0)
	sd          t1, 0(t0)
	ld          t0, -88(s0)
	addi        t0, t0, 56
	lw          t1, -40(s0)
	sw          t1, 0(t0)
	ld          t0, -88(s0)
	addi        t0, t0, 60
	lw          t1, -48(s0)
	sw          t1, 0(t0)
	ld          t0, -88(s0)
	addi        t0, t0, 64
	lw          t1, -56(s0)
	sw          t1, 0(t0)
	ld          t0, -88(s0)
	addi        t0, t0, 72
	ld          t1, -64(s0)
	sd          t1, 0(t0)
	ld          t0, -88(s0)
	addi        t0, t0, 80
	ld          t1, -72(s0)
	sd          t1, 0(t0)
	ld          t0, -80(s0)
	addi        a0, t0, 40
	ld          a1, -88(s0)
	call        VectorAppend

	// *** Basic block 2

	ld          t0, -88(s0)
	mv          a0, t0

	// *** Basic block 3

.ARArchiveBuilderAddFile_label_61:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARArchiveBuilderAddFile:
	.size ARArchiveBuilderAddFile, .func_end_ARArchiveBuilderAddFile-ARArchiveBuilderAddFile

	.global ARArchiveBuilderAddExisingFile
	.type ARArchiveBuilderAddExisingFile, @function

ARArchiveBuilderAddExisingFile:

	// *** Basic block 0

	.global VectorAppend
	.global MapInsert
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -48(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        a0, t0, 40
	ld          a1, -32(s0)
	call        VectorAppend

	// *** Basic block 1

	addi        t0, s0, -48
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t1, -32(s0)
	addi        t1, t1, 40
	ld          t1, 0(t1)
	sd          t1, 0(t0)
	addi        t0, s0, -48
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t1, -32(s0)
	sd          t1, 0(t0)
	ld          t0, -24(s0)
	addi        a0, t0, 120
	addi        t0, s0, -48
	addi        sp, sp, -16
	ld          t1, 0(t0)
	sd          t1, 0(sp)
	ld          t0, 8(t0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 2

	addi        sp, sp, 16
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARArchiveBuilderAddExisingFile:
	.size ARArchiveBuilderAddExisingFile, .func_end_ARArchiveBuilderAddExisingFile-ARArchiveBuilderAddExisingFile

	.global ARArchiveBuilderAddSymbol
	.type ARArchiveBuilderAddSymbol, @function

ARArchiveBuilderAddSymbol:

	// *** Basic block 0

	.global NewARSymbol
	.global VectorAppend
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a2, -24(s0)
	sd a1, -32(s0)
	sd a0, -40(s0)
	// Local vars at offset -48(s0)
	// End of stack frame
	ld          a0, -24(s0)
	call        NewARSymbol

	// *** Basic block 1

	sd          a0, -48(s0)
	ld          t0, -48(s0)
	addi        t0, t0, 40
	ld          t1, -32(s0)
	sd          t1, 0(t0)
	ld          t0, -40(s0)
	addi        a0, t0, 64
	ld          a1, -48(s0)
	call        VectorAppend

	// *** Basic block 2

	ld          t0, -48(s0)
	mv          a0, t0

	// *** Basic block 3

.ARArchiveBuilderAddSymbol_label_31:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARArchiveBuilderAddSymbol:
	.size ARArchiveBuilderAddSymbol, .func_end_ARArchiveBuilderAddSymbol-ARArchiveBuilderAddSymbol

	.local  WriteBigEndianInt
	.type WriteBigEndianInt, @function

WriteBigEndianInt:

	// *** Basic block 0

	.global fputc
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -40(s0)
	// End of stack frame
	li          t0, 3		// 0x3 ASCII \x3
	sw          t0, -40(s0)
	lw          t0, -40(s0)
	sltz        t1, t0
	not         t1, t1
	blt         t0, x0, .WriteBigEndianInt_label_41

	// *** Basic block 1

.WriteBigEndianInt_label_17:
	lw          t0, -24(s0)
	lw          t1, -40(s0)
	slli        t1, t1, 3
	sra         t0, t0, t1
	andi        a0, t0, 255
	ld          a1, -32(s0)
	call        fputc

	// *** Basic block 2

.WriteBigEndianInt_label_31:
	lw          t0, -40(s0)
	addi        t0, t0, -1
	sw          t0, -40(s0)
	lw          t0, -40(s0)
	sltz        t1, t0
	not         t1, t1
	blt         t0, x0, .WriteBigEndianInt_label_17

	// *** Basic block 3

.WriteBigEndianInt_label_41:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_WriteBigEndianInt:
	.size WriteBigEndianInt, .func_end_WriteBigEndianInt-WriteBigEndianInt

	.local  WriteSymbolName
	.type WriteSymbolName, @function

WriteSymbolName:

	// *** Basic block 0

	.global fputc
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -40(s0)
	// End of stack frame
	sd          x0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 24
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .WriteSymbolName_label_46

	// *** Basic block 1

.WriteSymbolName_label_20:
	ld          t0, -24(s0)
	addi        t0, t0, 16
	ld          t0, 0(t0)
	ld          t1, -40(s0)
	add         t0, t0, t1
	lb          a0, 0(t0)
	ld          a1, -32(s0)
	call        fputc

	// *** Basic block 2

.WriteSymbolName_label_35:
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 24
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .WriteSymbolName_label_20

	// *** Basic block 3

.WriteSymbolName_label_46:
	ld          a1, -32(s0)
	mv          a0, x0
	call        fputc

	// *** Basic block 4

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_WriteSymbolName:
	.size WriteSymbolName, .func_end_WriteSymbolName-WriteSymbolName

	.local  AlignFileOffset
	.type AlignFileOffset, @function

AlignFileOffset:

	// *** Basic block 0

	.global ftell
	.global fputc
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	ld          a0, -24(s0)
	call        ftell

	// *** Basic block 1

	sd          a0, -32(s0)
	ld          t0, -32(s0)
	andi        t0, t0, 1
	addi        t1, t0, -1
	seqz        t1, t1
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .AlignFileOffset_label_28

	// *** Basic block 2

	ld          a1, -24(s0)
	mv          a0, x0
	call        fputc

	// *** Basic block 3

.AlignFileOffset_label_28:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AlignFileOffset:
	.size AlignFileOffset, .func_end_AlignFileOffset-AlignFileOffset

	.local  WriteLeftString
	.type WriteLeftString, @function

WriteLeftString:

	// *** Basic block 0

	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a1, -24(s0)
	sd a0, -32(s0)
	sd a2, -40(s0)
	// Local vars at offset -40(s0)
	// End of stack frame
	ld          t0, -24(s0)
	lb          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .WriteLeftString_label_36

	// *** Basic block 1

.WriteLeftString_label_14:
	ld          t0, -32(s0)
	addi        t0, t0, 1
	sd          t0, -32(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 1
	sd          t0, -24(s0)
	lb          t0, 0(t0)
	sb          t0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, -1
	sd          t0, -40(s0)
	ld          t0, -24(s0)
	lb          t0, 0(t0)
	snez        t1, t0
	bnez        t0, .WriteLeftString_label_14

	// *** Basic block 2

.WriteLeftString_label_36:
	ld          t0, -40(s0)
	addi        t0, t0, -1
	sd          t0, -40(s0)
	slt         t0, x0, t0
	bge         x0, t0, .WriteLeftString_label_56

	// *** Basic block 3

.WriteLeftString_label_43:
	ld          t0, -32(s0)
	addi        t0, t0, 1
	sd          t0, -32(s0)
	li          t0, 32		// 0x20 ASCII ' '
	sb          t0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, -1
	sd          t0, -40(s0)
	slt         t0, x0, t0
	blt         x0, t0, .WriteLeftString_label_43

	// *** Basic block 4

.WriteLeftString_label_56:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_WriteLeftString:
	.size WriteLeftString, .func_end_WriteLeftString-WriteLeftString

	.local  WriteLeftInt
	.type WriteLeftInt, @function

WriteLeftInt:

	// *** Basic block 0

	.global snprintf
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	sd a2, -32(s0)
	sd a1, -40(s0)
	// Local vars at offset -48(s0)
	// End of stack frame
	ld          a0, -24(s0)
	ld          a1, -32(s0)
	lla         a2, .str.13
	ld          a3, -40(s0)
	call        snprintf

	// *** Basic block 1

	sd          a0, -48(s0)
	ld          t0, -48(s0)
	ld          t1, -24(s0)
	add         t0, t1, t0
	sd          t0, -24(s0)
	ld          t0, -48(s0)
	ld          t1, -32(s0)
	sub         t0, t1, t0
	sd          t0, -32(s0)
	ld          t0, -32(s0)
	addi        t0, t0, -1
	sd          t0, -32(s0)
	slt         t0, x0, t0
	bge         x0, t0, .WriteLeftInt_label_58

	// *** Basic block 2

.WriteLeftInt_label_45:
	ld          t0, -24(s0)
	addi        t0, t0, 1
	sd          t0, -24(s0)
	li          t0, 32		// 0x20 ASCII ' '
	sb          t0, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, -1
	sd          t0, -32(s0)
	slt         t0, x0, t0
	blt         x0, t0, .WriteLeftInt_label_45

	// *** Basic block 3

.WriteLeftInt_label_58:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_WriteLeftInt:
	.size WriteLeftInt, .func_end_WriteLeftInt-WriteLeftInt

	.local  WriteLeftFilename
	.type WriteLeftFilename, @function

WriteLeftFilename:

	// *** Basic block 0

	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a1, -24(s0)
	sd a0, -32(s0)
	sd a2, -40(s0)
	// Local vars at offset -48(s0)
	// End of stack frame
	ld          t0, -24(s0)
	lb          t0, 0(t0)
	addi        t0, t0, -47
	snez        t0, t0
	sb          t0, -48(s0)
	ld          t0, -24(s0)
	lb          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .WriteLeftFilename_label_44

	// *** Basic block 1

.WriteLeftFilename_label_22:
	ld          t0, -32(s0)
	addi        t0, t0, 1
	sd          t0, -32(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 1
	sd          t0, -24(s0)
	lb          t0, 0(t0)
	sb          t0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, -1
	sd          t0, -40(s0)
	ld          t0, -24(s0)
	lb          t0, 0(t0)
	snez        t1, t0
	bnez        t0, .WriteLeftFilename_label_22

	// *** Basic block 2

.WriteLeftFilename_label_44:
	lb          t0, -48(s0)
	beqz        t0, .WriteLeftFilename_label_57

	// *** Basic block 3

	ld          t0, -32(s0)
	addi        t0, t0, 1
	sd          t0, -32(s0)
	li          t0, 47		// 0x2f ASCII '/'
	sb          t0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, -1
	sd          t0, -40(s0)

	// *** Basic block 4

.WriteLeftFilename_label_57:
	ld          t0, -40(s0)
	addi        t0, t0, -1
	sd          t0, -40(s0)
	slt         t0, x0, t0
	bge         x0, t0, .WriteLeftFilename_label_77

	// *** Basic block 5

.WriteLeftFilename_label_64:
	ld          t0, -32(s0)
	addi        t0, t0, 1
	sd          t0, -32(s0)
	li          t0, 32		// 0x20 ASCII ' '
	sb          t0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, -1
	sd          t0, -40(s0)
	slt         t0, x0, t0
	blt         x0, t0, .WriteLeftFilename_label_64

	// *** Basic block 6

.WriteLeftFilename_label_77:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_WriteLeftFilename:
	.size WriteLeftFilename, .func_end_WriteLeftFilename-WriteLeftFilename

	.local  BuildFileHeader
	.type BuildFileHeader, @function

BuildFileHeader:

	// *** Basic block 0

	.local WriteLeftFilename
	.local WriteLeftInt
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Saved argument registers.
	sd a1, -24(s0)
	sd a0, -32(s0)
	sd a6, -40(s0)
	sd a2, -48(s0)
	sd a3, -56(s0)
	sd a4, -64(s0)
	sd a5, -72(s0)
	// Local vars at offset -72(s0)
	// End of stack frame
	ld          t0, -24(s0)
	sub         t1, t0, x0
	snez        t1, t1
	beq         t0, x0, .BuildFileHeader_label_39

	// *** Basic block 1

	ld          t0, -32(s0)
	addi        a0, t0, 0
	ld          a1, -24(s0)
	li          t0, 16		// 0x10 ASCII \x10
	mv          a2, t0
	call        WriteLeftFilename

	// *** Basic block 2

.BuildFileHeader_label_39:
	ld          t0, -32(s0)
	addi        a0, t0, 48
	ld          a1, -40(s0)
	li          t0, 10		// 0xa ASCII \xa
	mv          a2, t0
	call        WriteLeftInt

	// *** Basic block 3

	ld          t0, -32(s0)
	addi        a0, t0, 28
	lw          a1, -48(s0)
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	call        WriteLeftInt

	// *** Basic block 4

	ld          t0, -32(s0)
	addi        a0, t0, 34
	lw          a1, -56(s0)
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	call        WriteLeftInt

	// *** Basic block 5

	ld          t0, -32(s0)
	addi        a0, t0, 40
	lw          a1, -64(s0)
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	call        WriteLeftInt

	// *** Basic block 6

	ld          t0, -32(s0)
	addi        a0, t0, 16
	ld          a1, -72(s0)
	li          t0, 12		// 0xc ASCII \xc
	mv          a2, t0
	call        WriteLeftInt

	// *** Basic block 7

	ld          t0, -32(s0)
	addi        t0, t0, 58
	addi        t0, t0, 0
	lla         t1, .str.14
	addi        t1, t1, 0
	lb          t1, 0(t1)
	sb          t1, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 58
	addi        t0, t0, 1
	lla         t1, .str.15
	addi        t1, t1, 0
	lb          t1, 0(t1)
	sb          t1, 0(t0)
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BuildFileHeader:
	.size BuildFileHeader, .func_end_BuildFileHeader-BuildFileHeader

	.local  WriteFile
	.type WriteFile, @function

WriteFile:

	// *** Basic block 0

	.global ftell
	.global snprintf
	.local WriteLeftString
	.local WriteLeftFilename
	.local BuildFileHeader
	.global fwrite
	.local AlignFileOffset
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Saved argument registers.
	sd a1, -24(s0)
	sd a2, -32(s0)
	// Local vars at offset -112(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        s1, t0, 40
	ld          a0, -32(s0)
	call        ftell

	// *** Basic block 1

	sd          a0, 0(s1)
	ld          t0, -24(s0)
	addi        t0, t0, 0
	addi        t0, t0, 24
	ld          t0, 0(t0)
	li          t1, 15		// 0xf ASCII \xf
	slt         t1, t1, t0
	li          t1, 15		// 0xf ASCII \xf
	bge         t1, t0, .WriteFile_label_71

	// *** Basic block 2

	addi        a0, s0, -112
	lla         a2, .str.16
	ld          t0, -24(s0)
	addi        t0, t0, 96
	ld          a3, 0(t0)
	li          t0, 16		// 0x10 ASCII \x10
	mv          a1, t0
	call        snprintf

	// *** Basic block 3

	addi        t0, s0, -96
	addi        a0, t0, 0
	addi        a1, s0, -112
	li          t0, 16		// 0x10 ASCII \x10
	mv          a2, t0
	call        WriteLeftString

	// *** Basic block 4

	j           .WriteFile_label_84

	// *** Basic block 5

.WriteFile_label_71:
	addi        t0, s0, -96
	addi        a0, t0, 0
	ld          t0, -24(s0)
	addi        t0, t0, 0
	addi        t0, t0, 16
	ld          a1, 0(t0)
	li          t0, 16		// 0x10 ASCII \x10
	mv          a2, t0
	call        WriteLeftFilename

	// *** Basic block 6

.WriteFile_label_84:
	addi        a0, s0, -96
	ld          t0, -24(s0)
	addi        t0, t0, 56
	lw          a2, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 60
	lw          a3, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 64
	lw          a4, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 72
	ld          a5, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 48
	ld          a6, 0(t0)
	mv          a1, x0
	call        BuildFileHeader

	// *** Basic block 7

	addi        a0, s0, -96
	ld          a3, -32(s0)
	li          t0, 64		// 0x40 ASCII '@'
	mv          a2, t0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        fwrite

	// *** Basic block 8

	ld          t0, -24(s0)
	addi        t0, t0, 80
	ld          a0, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 48
	ld          a2, 0(t0)
	ld          a3, -32(s0)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        fwrite

	// *** Basic block 9

	ld          a0, -32(s0)
	call        AlignFileOffset

	// *** Basic block 10

	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_WriteFile:
	.size WriteFile, .func_end_WriteFile-WriteFile

	.local  WriteExtendedFilenames
	.type WriteExtendedFilenames, @function

WriteExtendedFilenames:

	// *** Basic block 0

	.local BuildFileHeader
	.global time
	.global fwrite
	.global fputc
	.local AlignFileOffset
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -112(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 88
	addi        t0, t0, 8
	ld          t0, 0(t0)
	seqz        t1, t0
	bnez        t0, .WriteExtendedFilenames_label_31

	// *** Basic block 1

.WriteExtendedFilenames_label_28:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.WriteExtendedFilenames_label_31:
	addi        s1, s0, -112
	lla         s2, .str.17
	mv          a0, x0
	call        time

	// *** Basic block 3

	ld          t0, -24(s0)
	addi        t0, t0, 112
	ld          a6, 0(t0)
	mv          a5, a0
	li          t0, 511		// 0x1ff
	mv          a4, t0
	mv          a3, x0
	mv          a2, x0
	mv          a1, s2
	mv          a0, s1
	call        BuildFileHeader

	// *** Basic block 4

	addi        a0, s0, -112
	ld          a3, -32(s0)
	li          t0, 64		// 0x40 ASCII '@'
	mv          a2, t0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        fwrite

	// *** Basic block 5

	sd          x0, -48(s0)
	ld          t0, -48(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 88
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .WriteExtendedFilenames_label_130

	// *** Basic block 6

.WriteExtendedFilenames_label_85:
	ld          t0, -24(s0)
	addi        t0, t0, 88
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -48(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 16
	ld          a0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 24
	ld          a2, 0(t0)
	ld          a3, -32(s0)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        fwrite

	// *** Basic block 7

	ld          a1, -32(s0)
	li          t0, 10		// 0xa ASCII \xa
	mv          a0, t0
	call        fputc

	// *** Basic block 8

.WriteExtendedFilenames_label_118:
	ld          t0, -48(s0)
	addi        t0, t0, 1
	sd          t0, -48(s0)
	ld          t0, -48(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 88
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .WriteExtendedFilenames_label_85

	// *** Basic block 9

.WriteExtendedFilenames_label_130:
	ld          a0, -32(s0)
	call        AlignFileOffset

	// *** Basic block 10

	j           .WriteExtendedFilenames_label_28
.func_end_WriteExtendedFilenames:
	.size WriteExtendedFilenames, .func_end_WriteExtendedFilenames-WriteExtendedFilenames

	.local  PrepareSymbolTable
	.type PrepareSymbolTable, @function

PrepareSymbolTable:

	// *** Basic block 0

	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -40(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 160
	li          t1, 4		// 0x4 ASCII \x4
	sd          t1, 0(t0)
	sd          x0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 64
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .PrepareSymbolTable_label_80

	// *** Basic block 1

.PrepareSymbolTable_label_30:
	ld          t0, -24(s0)
	addi        t0, t0, 64
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -40(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -32(s0)
	ld          t0, -32(s0)
	addi        t0, t0, 40
	ld          t0, 0(t0)
	addi        t0, t0, 88
	lb          t0, 0(t0)
	not         t0, t0
	beqz        t0, .PrepareSymbolTable_label_67

	// *** Basic block 2

	ld          t0, -24(s0)
	addi        t0, t0, 152
	lw          t1, 0(t0)
	addi        t1, t1, 1
	sw          t1, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 0
	addi        t0, t0, 24
	ld          t0, 0(t0)
	addi        t0, t0, 1
	addi        t0, t0, 4
	ld          t1, -24(s0)
	addi        t1, t1, 160
	ld          t2, 0(t1)
	add         t0, t2, t0
	sd          t0, 0(t1)

	// *** Basic block 3

.PrepareSymbolTable_label_67:

	// *** Basic block 4

.PrepareSymbolTable_label_68:
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 64
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .PrepareSymbolTable_label_30

	// *** Basic block 5

.PrepareSymbolTable_label_80:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PrepareSymbolTable:
	.size PrepareSymbolTable, .func_end_PrepareSymbolTable-PrepareSymbolTable

	.local  PrepareExtendedFilenames
	.type PrepareExtendedFilenames, @function

PrepareExtendedFilenames:

	// *** Basic block 0

	.global VectorAppend
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -40(s0)
	// End of stack frame
	sd          x0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 40
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .PrepareExtendedFilenames_label_84

	// *** Basic block 1

.PrepareExtendedFilenames_label_25:
	ld          t0, -24(s0)
	addi        t0, t0, 40
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -40(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -32(s0)
	ld          t0, -32(s0)
	addi        t0, t0, 0
	addi        t0, t0, 24
	ld          t0, 0(t0)
	li          t1, 15		// 0xf ASCII \xf
	slt         t1, t1, t0
	li          t1, 15		// 0xf ASCII \xf
	bge         t1, t0, .PrepareExtendedFilenames_label_71

	// *** Basic block 2

	ld          t0, -32(s0)
	addi        t0, t0, 96
	ld          t1, -24(s0)
	addi        t1, t1, 112
	ld          t1, 0(t1)
	sd          t1, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 0
	addi        t0, t0, 24
	ld          t0, 0(t0)
	addi        t0, t0, 1
	ld          t1, -24(s0)
	addi        t1, t1, 112
	ld          t2, 0(t1)
	add         t0, t2, t0
	sd          t0, 0(t1)
	ld          t0, -24(s0)
	addi        a0, t0, 88
	ld          t0, -32(s0)
	addi        a1, t0, 0
	call        VectorAppend

	// *** Basic block 3

.PrepareExtendedFilenames_label_71:

	// *** Basic block 4

.PrepareExtendedFilenames_label_72:
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 40
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .PrepareExtendedFilenames_label_25

	// *** Basic block 5

.PrepareExtendedFilenames_label_84:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PrepareExtendedFilenames:
	.size PrepareExtendedFilenames, .func_end_PrepareExtendedFilenames-PrepareExtendedFilenames

	.local  WriteSymbolTable
	.type WriteSymbolTable, @function

WriteSymbolTable:

	// *** Basic block 0

	.local BuildFileHeader
	.global time
	.global fwrite
	.local WriteBigEndianInt
	.local WriteSymbolName
	.local AlignFileOffset
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -128(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 64
	addi        t0, t0, 8
	ld          t0, 0(t0)
	seqz        t1, t0
	bnez        t0, .WriteSymbolTable_label_33

	// *** Basic block 1

.WriteSymbolTable_label_30:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.WriteSymbolTable_label_33:
	addi        s1, s0, -128
	lla         s2, .str.18
	mv          a0, x0
	call        time

	// *** Basic block 3

	ld          t0, -24(s0)
	addi        t0, t0, 160
	ld          a6, 0(t0)
	mv          a5, a0
	li          t0, 511		// 0x1ff
	mv          a4, t0
	mv          a3, x0
	mv          a2, x0
	mv          a1, s2
	mv          a0, s1
	call        BuildFileHeader

	// *** Basic block 4

	addi        a0, s0, -128
	ld          a3, -32(s0)
	li          t0, 64		// 0x40 ASCII '@'
	mv          a2, t0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        fwrite

	// *** Basic block 5

	ld          t0, -24(s0)
	addi        t0, t0, 152
	lw          a0, 0(t0)
	ld          a1, -32(s0)
	call        WriteBigEndianInt

	// *** Basic block 6

	sd          x0, -64(s0)
	ld          t0, -64(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 64
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .WriteSymbolTable_label_136

	// *** Basic block 7

.WriteSymbolTable_label_94:
	ld          t0, -24(s0)
	addi        t0, t0, 64
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -64(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -56(s0)
	ld          t0, -56(s0)
	addi        t0, t0, 40
	ld          t0, 0(t0)
	addi        t0, t0, 88
	lb          t0, 0(t0)
	beqz        t0, .WriteSymbolTable_label_114

	// *** Basic block 8

	j           .WriteSymbolTable_label_124

	// *** Basic block 9

.WriteSymbolTable_label_114:
	ld          t0, -56(s0)
	addi        t0, t0, 40
	ld          t0, 0(t0)
	addi        t0, t0, 40
	ld          a0, 0(t0)
	ld          a1, -32(s0)
	call        WriteBigEndianInt

	// *** Basic block 10

.WriteSymbolTable_label_124:
	ld          t0, -64(s0)
	addi        t0, t0, 1
	sd          t0, -64(s0)
	ld          t0, -64(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 64
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .WriteSymbolTable_label_94

	// *** Basic block 11

.WriteSymbolTable_label_136:
	sd          x0, -48(s0)
	ld          t0, -48(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 64
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .WriteSymbolTable_label_184

	// *** Basic block 12

.WriteSymbolTable_label_146:
	ld          t0, -24(s0)
	addi        t0, t0, 64
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -48(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 40
	ld          t0, 0(t0)
	addi        t0, t0, 88
	lb          t0, 0(t0)
	beqz        t0, .WriteSymbolTable_label_165

	// *** Basic block 13

	j           .WriteSymbolTable_label_172

	// *** Basic block 14

.WriteSymbolTable_label_165:
	ld          t0, -40(s0)
	addi        a0, t0, 0
	ld          a1, -32(s0)
	call        WriteSymbolName

	// *** Basic block 15

.WriteSymbolTable_label_172:
	ld          t0, -48(s0)
	addi        t0, t0, 1
	sd          t0, -48(s0)
	ld          t0, -48(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 64
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .WriteSymbolTable_label_146

	// *** Basic block 16

.WriteSymbolTable_label_184:
	ld          a0, -32(s0)
	call        AlignFileOffset

	// *** Basic block 17

	j           .WriteSymbolTable_label_30
.func_end_WriteSymbolTable:
	.size WriteSymbolTable, .func_end_WriteSymbolTable-WriteSymbolTable

	.local  Align2
	.type Align2, @function

Align2:

	// *** Basic block 0

	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -24(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 1
	andi        t0, t0, -2
	mv          a0, t0

	// *** Basic block 1

.Align2_label_11:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Align2:
	.size Align2, .func_end_Align2-Align2

	.global ARArchiveBuilderWrite
	.type ARArchiveBuilderWrite, @function

ARArchiveBuilderWrite:

	// *** Basic block 0

	.global fopen
	.global fwrite
	.local PrepareSymbolTable
	.local PrepareExtendedFilenames
	.local Align2
	.local WriteSymbolTable
	.local WriteExtendedFilenames
	.local WriteFile
	.global fclose
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -72(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 0
	addi        t0, t0, 16
	ld          a0, 0(t0)
	lla         a1, .str.19
	call        fopen

	// *** Basic block 1

	sd          a0, -72(s0)
	ld          t0, -72(s0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .ARArchiveBuilderWrite_label_52

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.ARArchiveBuilderWrite_label_49:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.ARArchiveBuilderWrite_label_52:
	lla         a0, .str.20
	ld          a3, -72(s0)
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        fwrite

	// *** Basic block 5

	ld          a0, -24(s0)
	call        PrepareSymbolTable

	// *** Basic block 6

	ld          a0, -24(s0)
	call        PrepareExtendedFilenames

	// *** Basic block 7

	ld          t0, -24(s0)
	addi        t0, t0, 160
	ld          t0, 0(t0)
	addi        a0, t0, 72
	call        Align2

	// *** Basic block 8

	sd          a0, -64(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 88
	addi        t0, t0, 8
	ld          t0, 0(t0)
	slt         t1, x0, t0
	bge         x0, t0, .ARArchiveBuilderWrite_label_96

	// *** Basic block 9

	ld          t0, -24(s0)
	addi        t0, t0, 112
	ld          t0, 0(t0)
	addi        a0, t0, 64
	call        Align2

	// *** Basic block 10

	ld          t0, -64(s0)
	add         t0, t0, a0
	sd          t0, -64(s0)

	// *** Basic block 11

.ARArchiveBuilderWrite_label_96:
	sd          x0, -56(s0)
	ld          t0, -56(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 40
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ARArchiveBuilderWrite_label_144

	// *** Basic block 12

.ARArchiveBuilderWrite_label_106:
	ld          t0, -24(s0)
	addi        t0, t0, 40
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -56(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -48(s0)
	ld          t0, -48(s0)
	addi        t0, t0, 40
	ld          t1, -64(s0)
	sd          t1, 0(t0)
	ld          t0, -48(s0)
	addi        t0, t0, 48
	ld          t0, 0(t0)
	addi        a0, t0, 64
	call        Align2

	// *** Basic block 13

	ld          t0, -64(s0)
	add         t0, t0, a0
	sd          t0, -64(s0)

	// *** Basic block 14

.ARArchiveBuilderWrite_label_132:
	ld          t0, -56(s0)
	addi        t0, t0, 1
	sd          t0, -56(s0)
	ld          t0, -56(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 40
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ARArchiveBuilderWrite_label_106

	// *** Basic block 15

.ARArchiveBuilderWrite_label_144:
	ld          a0, -24(s0)
	ld          a1, -72(s0)
	call        WriteSymbolTable

	// *** Basic block 16

	ld          a0, -24(s0)
	ld          a1, -72(s0)
	call        WriteExtendedFilenames

	// *** Basic block 17

	sd          x0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 40
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ARArchiveBuilderWrite_label_201

	// *** Basic block 18

.ARArchiveBuilderWrite_label_164:
	ld          t0, -24(s0)
	addi        t0, t0, 40
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -40(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -32(s0)
	ld          t0, -32(s0)
	addi        t0, t0, 88
	lb          t0, 0(t0)
	beqz        t0, .ARArchiveBuilderWrite_label_181

	// *** Basic block 19

	j           .ARArchiveBuilderWrite_label_189

	// *** Basic block 20

.ARArchiveBuilderWrite_label_181:
	ld          a0, -24(s0)
	ld          a1, -32(s0)
	ld          a2, -72(s0)
	call        WriteFile

	// *** Basic block 21

.ARArchiveBuilderWrite_label_189:
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 40
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ARArchiveBuilderWrite_label_164

	// *** Basic block 22

.ARArchiveBuilderWrite_label_201:
	ld          a0, -72(s0)
	call        fclose

	// *** Basic block 23

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .ARArchiveBuilderWrite_label_49
.func_end_ARArchiveBuilderWrite:
	.size ARArchiveBuilderWrite, .func_end_ARArchiveBuilderWrite-ARArchiveBuilderWrite

	.local  CopySymbolTable
	.type CopySymbolTable, @function

CopySymbolTable:

	// *** Basic block 0

	.global MapFind
	.global printf
	.global abort
	.global malloc
	.global StringInit
	.global VectorAppend
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -88(s0)
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	ld          t0, -24(s0)
	sd          t0, -88(s0)
	ld          t0, -32(s0)
	sd          t0, -80(s0)
	sd          x0, -72(s0)
	ld          t0, -72(s0)
	ld          t1, -88(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .CopySymbolTable_label_140

	// *** Basic block 1

.CopySymbolTable_label_39:
	ld          t0, -88(s0)
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -72(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -64(s0)
	ld          t0, -64(s0)
	addi        t0, t0, 40
	ld          t0, 0(t0)
	addi        t0, t0, 88
	lb          t0, 0(t0)
	beqz        t0, .CopySymbolTable_label_58

	// *** Basic block 2

	j           .CopySymbolTable_label_129

	// *** Basic block 3

.CopySymbolTable_label_58:
	sd          x0, -56(s0)
	ld          t0, -64(s0)
	addi        t0, t0, 40
	ld          t0, 0(t0)
	sd          t0, -56(s0)
	ld          t0, -80(s0)
	addi        a0, t0, 8
	addi        t0, s0, -56
	ld          a1, 0(t0)
	call        MapFind

	// *** Basic block 4

	sd          a0, -48(s0)
	ld          t0, -48(s0)
	sub         t1, t0, x0
	snez        t1, t1
	beq         t0, x0, .CopySymbolTable_label_83

	// *** Basic block 5

	mv          s1, x0
	j           .CopySymbolTable_label_101

	// *** Basic block 6

.CopySymbolTable_label_83:
	lla         a0, .str.21
	lla         a1, .str.22
	lla         a3, .str.23
	li          t0, 751		// 0x2ef
	mv          a2, t0
	call        printf

	// *** Basic block 7

	call        abort

	// *** Basic block 8

	mv          s1, a0

	// *** Basic block 9

.CopySymbolTable_label_101:
	li          t0, 48		// 0x30 ASCII '0'
	mv          a0, t0
	call        malloc

	// *** Basic block 10

	sd          a0, -40(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 40
	ld          t1, -48(s0)
	sd          t1, 0(t0)
	ld          t0, -40(s0)
	addi        a0, t0, 0
	ld          t0, -64(s0)
	addi        t0, t0, 0
	addi        t0, t0, 16
	ld          a1, 0(t0)
	call        StringInit

	// *** Basic block 11

	ld          t0, -80(s0)
	addi        t0, t0, 0
	ld          t0, 0(t0)
	addi        a0, t0, 64
	ld          a1, -40(s0)
	call        VectorAppend

	// *** Basic block 12

.CopySymbolTable_label_129:
	ld          t0, -72(s0)
	addi        t0, t0, 1
	sd          t0, -72(s0)
	ld          t0, -72(s0)
	ld          t1, -88(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .CopySymbolTable_label_39

	// *** Basic block 13

.CopySymbolTable_label_140:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CopySymbolTable:
	.size CopySymbolTable, .func_end_CopySymbolTable-CopySymbolTable

	.global ARArchiveBuilderCopyArchive
	.type ARArchiveBuilderCopyArchive, @function

ARArchiveBuilderCopyArchive:

	// *** Basic block 0

	.global MapInitForPointerKeys
	.global StringEqual
	.global ARFileCopyFromArchive
	.global ARArchiveBuilderAddExisingFile
	.global MapInsert
	.global HashTableTraverse
	.local CopySymbolTable
	.global MapDestruct
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	sd a2, -40(s0)
	// Local vars at offset -120(s0)
	// End of stack frame
	addi        t0, s0, -120
	addi        t0, t0, 0
	ld          t1, -24(s0)
	sd          t1, 0(t0)
	addi        t0, s0, -120
	addi        a0, t0, 8
	call        MapInitForPointerKeys

	// *** Basic block 1

	sd          x0, -80(s0)
	ld          t0, -80(s0)
	ld          t1, -32(s0)
	addi        t1, t1, 40
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ARArchiveBuilderCopyArchive_label_126

	// *** Basic block 2

.ARArchiveBuilderCopyArchive_label_41:
	ld          t0, -32(s0)
	addi        t0, t0, 40
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -80(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -72(s0)
	ld          t0, -72(s0)
	addi        a0, t0, 0
	lla         a1, .str.24
	call        StringEqual

	// *** Basic block 3

	beqz        a0, .ARArchiveBuilderCopyArchive_label_64

	// *** Basic block 4

	j           .ARArchiveBuilderCopyArchive_label_114

	// *** Basic block 5

.ARArchiveBuilderCopyArchive_label_64:
	ld          t0, -72(s0)
	addi        a0, t0, 0
	lla         a1, .str.25
	call        StringEqual

	// *** Basic block 6

	beqz        a0, .ARArchiveBuilderCopyArchive_label_74

	// *** Basic block 7

	j           .ARArchiveBuilderCopyArchive_label_114

	// *** Basic block 8

.ARArchiveBuilderCopyArchive_label_74:
	ld          a0, -72(s0)
	ld          a1, -40(s0)
	call        ARFileCopyFromArchive

	// *** Basic block 9

	sd          a0, -64(s0)
	ld          a0, -24(s0)
	ld          a1, -64(s0)
	call        ARArchiveBuilderAddExisingFile

	// *** Basic block 10

	addi        t0, s0, -56
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t1, -72(s0)
	sd          t1, 0(t0)
	addi        t0, s0, -56
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t1, -64(s0)
	sd          t1, 0(t0)
	addi        t0, s0, -120
	addi        a0, t0, 8
	addi        t0, s0, -56
	addi        sp, sp, -16
	ld          t1, 0(t0)
	sd          t1, 0(sp)
	ld          t0, 8(t0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 11

	addi        sp, sp, 16

	// *** Basic block 12

.ARArchiveBuilderCopyArchive_label_114:
	ld          t0, -80(s0)
	addi        t0, t0, 1
	sd          t0, -80(s0)
	ld          t0, -80(s0)
	ld          t1, -32(s0)
	addi        t1, t1, 40
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ARArchiveBuilderCopyArchive_label_41

	// *** Basic block 13

.ARArchiveBuilderCopyArchive_label_126:
	ld          t0, -32(s0)
	addi        a0, t0, 112
	addi        a2, s0, -120
	la          t0, CopySymbolTable
	mv          a1, t0
	call        HashTableTraverse

	// *** Basic block 14

	addi        t0, s0, -120
	addi        a0, t0, 8
	call        MapDestruct

	// *** Basic block 15

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ARArchiveBuilderCopyArchive:
	.size ARArchiveBuilderCopyArchive, .func_end_ARArchiveBuilderCopyArchive-ARArchiveBuilderCopyArchive

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%-30s %-20s @0x%" PRIx64 "\n"
	.type .str.1, @object
	.size .str.1, 21

.str.2:
	.asciz "symbol-table"
	.type .str.2, @object
	.size .str.2, 13

.str.3:
	.asciz "//"
	.type .str.3, @object
	.size .str.3, 3

.str.4:
	.asciz "/"
	.type .str.4, @object
	.size .str.4, 2

.str.5:
	.asciz "(null)"
	.type .str.5, @object
	.size .str.5, 1

.str.6:
	.asciz "Corrupted symbol table\n"
	.type .str.6, @object
	.size .str.6, 24

.str.7:
	.asciz "`\n"
	.type .str.7, @object
	.size .str.7, 3

.str.8:
	.asciz "ar: Corrupted archive; invalid file header\n"
	.type .str.8, @object
	.size .str.8, 44

.str.9:
	.asciz "(null)"
	.type .str.9, @object
	.size .str.9, 1

.str.10:
	.asciz "//"
	.type .str.10, @object
	.size .str.10, 3

.str.11:
	.asciz "/"
	.type .str.11, @object
	.size .str.11, 2

.str.12:
	.asciz "!<arch>\n"
	.type .str.12, @object
	.size .str.12, 9

.str.13:
	.asciz "%zd"
	.type .str.13, @object
	.size .str.13, 4

.str.14:
	.asciz "`\n"
	.type .str.14, @object
	.size .str.14, 3

.str.15:
	.asciz "`\n"
	.type .str.15, @object
	.size .str.15, 3

.str.16:
	.asciz "/%zd"
	.type .str.16, @object
	.size .str.16, 5

.str.17:
	.asciz "//"
	.type .str.17, @object
	.size .str.17, 3

.str.18:
	.asciz "/"
	.type .str.18, @object
	.size .str.18, 2

.str.19:
	.asciz "w"
	.type .str.19, @object
	.size .str.19, 2

.str.20:
	.asciz "!<arch>\n"
	.type .str.20, @object
	.size .str.20, 9

.str.21:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.21, @object
	.size .str.21, 30

.str.22:
	.asciz "ar.c"
	.type .str.22, @object
	.size .str.22, 5

.str.23:
	.asciz "new_file != NULL"
	.type .str.23, @object
	.size .str.23, 17

.str.24:
	.asciz "/"
	.type .str.24, @object
	.size .str.24, 2

.str.25:
	.asciz "//"
	.type .str.25, @object
	.size .str.25, 3

