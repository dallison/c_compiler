	.file   "linker_file.c"
	.text
	.option pic
.PCbegin:
	.global NewObjectFile
	.type NewObjectFile, @function

NewObjectFile:

	// *** Basic block 0

	.global malloc
	.global StringInit
	.global HashTableInit
	.global SymbolHash
	.global SymbolInsertInHashTable
	.global SymbolFindInHashTable
	.global VectorInit
	.global MapInitForStringKeys
	.global MapInitForInt64Keys
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
	mv          s2, a0
	mv          s3, a1
	li          a0, 256		// 0x100
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	mv          a1, s1
	mv          a0, s4
	call        StringInit

	// *** Basic block 2

	sd          s2, 40(s4)
	addi        a0, s4, 48
	lla         a1, .str.1
	la          t0, SymbolFindInHashTable
	mv          a5, t0
	la          t0, SymbolInsertInHashTable
	mv          a4, t0
	la          t0, SymbolHash
	mv          a3, t0
	li          t0, 111		// 0x6f ASCII 'o'
	mv          a2, t0
	call        HashTableInit

	// *** Basic block 3

	addi        a0, s4, 136
	call        VectorInit

	// *** Basic block 4

	sd          s3, 160(s4)
	addi        a0, s4, 168
	call        MapInitForStringKeys

	// *** Basic block 5

	addi        a0, s4, 200
	call        MapInitForInt64Keys

	// *** Basic block 6

	addi        a0, s4, 232
	call        VectorInit

	// *** Basic block 7

	mv          a0, s4

	// *** Basic block 8

.NewObjectFile_label_81:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewObjectFile:
	.size NewObjectFile, .func_end_NewObjectFile-NewObjectFile

	.local  DeleteSectionMapEntry
	.type DeleteSectionMapEntry, @function

DeleteSectionMapEntry:

	// *** Basic block 0

	.global VectorDelete
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 8(t0)
	mv          a0, t1
	j           VectorDelete
.func_end_DeleteSectionMapEntry:
	.size DeleteSectionMapEntry, .func_end_DeleteSectionMapEntry-DeleteSectionMapEntry

	.global ObjectFileDestruct
	.type ObjectFileDestruct, @function

ObjectFileDestruct:

	// *** Basic block 0

	.global StringDestruct
	.global LinkerClearSymbolTable
	.global HashTableDestruct
	.global MapDestruct
	.global VectorDestructWithContents
	.global RelocationDestruct
	.global MapTraverse
	.local DeleteSectionMapEntry
	.global VectorDestruct
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

	addi        a0, s1, 48
	call        LinkerClearSymbolTable

	// *** Basic block 2

	addi        a0, s1, 48
	call        HashTableDestruct

	// *** Basic block 3

	addi        a0, s1, 168
	call        MapDestruct

	// *** Basic block 4

	addi        a0, s1, 136
	la          t0, RelocationDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 5

	addi        a0, s1, 200
	mv          a2, x0
	la          t0, DeleteSectionMapEntry
	mv          a1, t0
	call        MapTraverse

	// *** Basic block 6

	addi        a0, s1, 200
	call        MapDestruct

	// *** Basic block 7

	addi        a0, s1, 232
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorDestruct
.func_end_ObjectFileDestruct:
	.size ObjectFileDestruct, .func_end_ObjectFileDestruct-ObjectFileDestruct

	.global ObjectFileDelete
	.type ObjectFileDelete, @function

ObjectFileDelete:

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
	.global ObjectFileDestruct
	.global free
	mv          s1, a0
	call        ObjectFileDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_ObjectFileDelete:
	.size ObjectFileDelete, .func_end_ObjectFileDelete-ObjectFileDelete

	.global ObjectFileFindSymbol
	.type ObjectFileFindSymbol, @function

ObjectFileFindSymbol:

	// *** Basic block 0

	.global LinkerFindSymbol
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
	addi        a0, s1, 48
	call        LinkerFindSymbol

	// *** Basic block 1

	mv          s3, a0
	bne         s3, x0, .ObjectFileFindSymbol_label_31

	// *** Basic block 2

	ld          t0, 160(s1)
	addi        a0, t0, 160
	mv          a1, s2
	call        LinkerFindSymbol

	// *** Basic block 3

	mv          s3, a0

	// *** Basic block 4

.ObjectFileFindSymbol_label_31:
	mv          a0, s3

	// *** Basic block 5

.ObjectFileFindSymbol_label_34:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ObjectFileFindSymbol:
	.size ObjectFileFindSymbol, .func_end_ObjectFileFindSymbol-ObjectFileFindSymbol

	.global ObjectFileFindSection
	.type ObjectFileFindSection, @function

ObjectFileFindSection:

	// *** Basic block 0

	.global MapFindPointerKey
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 168
	j           MapFindPointerKey
.func_end_ObjectFileFindSection:
	.size ObjectFileFindSection, .func_end_ObjectFileFindSection-ObjectFileFindSection

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "local-symbol-table"
	.type .str.1, @object
	.size .str.1, 19

