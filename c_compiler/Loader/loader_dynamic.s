	.file   "loader_dynamic.c"
	.text
	.option pic
.PCbegin:
	.global DynamicLibraryRegistryInit
	.type DynamicLibraryRegistryInit, @function

DynamicLibraryRegistryInit:

	// *** Basic block 0

	.global MapInitForStringKeys
	.global VectorInit
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
	call        MapInitForStringKeys

	// *** Basic block 1

	ld          t0, -24(s0)
	addi        a0, t0, 32
	call        VectorInit

	// *** Basic block 2

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DynamicLibraryRegistryInit:
	.size DynamicLibraryRegistryInit, .func_end_DynamicLibraryRegistryInit-DynamicLibraryRegistryInit

	.global DynamicLibraryRegistryDestruct
	.type DynamicLibraryRegistryDestruct, @function

DynamicLibraryRegistryDestruct:

	// *** Basic block 0

	.global VectorDestructWithContents
	.global LoadedDynamicLibraryDestruct
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
	addi        a0, t0, 32
	la          t0, LoadedDynamicLibraryDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 1

	ld          t0, -24(s0)
	addi        a0, t0, 0
	call        MapDestruct

	// *** Basic block 2

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DynamicLibraryRegistryDestruct:
	.size DynamicLibraryRegistryDestruct, .func_end_DynamicLibraryRegistryDestruct-DynamicLibraryRegistryDestruct

	.local  AlignUp
	.type AlignUp, @function

AlignUp:

	// *** Basic block 0

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
	ld          t1, -32(s0)
	addi        t1, t1, -1
	add         t0, t0, t1
	ld          t1, -32(s0)
	addi        t1, t1, -1
	not         t1, t1
	and         t0, t0, t1
	mv          a0, t0

	// *** Basic block 1

.AlignUp_label_17:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AlignUp:
	.size AlignUp, .func_end_AlignUp-AlignUp

	.local  AlignDown
	.type AlignDown, @function

AlignDown:

	// *** Basic block 0

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
	ld          t1, -32(s0)
	addi        t1, t1, -1
	not         t1, t1
	and         t0, t0, t1
	mv          a0, t0

	// *** Basic block 1

.AlignDown_label_14:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AlignDown:
	.size AlignDown, .func_end_AlignDown-AlignDown

	.local  NextAddress
	.type NextAddress, @function

NextAddress:

	// *** Basic block 0

	.local AlignUp
	.global sysconf
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	sd a2, -32(s0)
	sd a1, -40(s0)
	// Local vars at offset -40(s0)
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	ld          t1, -24(s0)
	addi        t1, t1, 120
	ld          t1, 0(t1)
	addi        t1, t1, 16
	lhu         t1, 0(t1)
	addi        t2, t1, -3
	seqz        t0, t2
	li          t2, 3		// 0x3 ASCII \x3
	bne         t1, t2, .NextAddress_label_27

	// *** Basic block 1

	ld          t1, -32(s0)
	sub         t1, t1, x0
	snez        t0, t1

	// *** Basic block 2

.NextAddress_label_27:
	beqz        t0, .NextAddress_label_47

	// *** Basic block 3

	ld          t0, -32(s0)
	ld          s1, 0(t0)
	li          t0, 30		// 0x1e ASCII \x1e
	mv          a0, t0
	call        sysconf

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s1
	call        AlignUp

	// *** Basic block 5


	// *** Basic block 6

.NextAddress_label_44:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.NextAddress_label_47:
	ld          t0, -40(s0)
	mv          a0, t0
	j           .NextAddress_label_44
.func_end_NextAddress:
	.size NextAddress, .func_end_NextAddress-NextAddress

	.local  NewMappedSegment
	.type NewMappedSegment, @function

NewMappedSegment:

	// *** Basic block 0

	.global malloc
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
	li          a0, 16		// 0x10 ASCII \x10
	call        malloc

	// *** Basic block 1

	sd          a0, -40(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 0
	ld          t1, -24(s0)
	sd          t1, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 8
	ld          t1, -32(s0)
	sd          t1, 0(t0)
	ld          t0, -40(s0)
	mv          a0, t0

	// *** Basic block 2

.NewMappedSegment_label_27:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewMappedSegment:
	.size NewMappedSegment, .func_end_NewMappedSegment-NewMappedSegment

	.local  MappedSegmentDestruct
	.type MappedSegmentDestruct, @function

MappedSegmentDestruct:

	// *** Basic block 0

	.global munmap
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
	addi        t0, t0, 0
	ld          a0, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 8
	ld          a1, 0(t0)
	call        munmap

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_MappedSegmentDestruct:
	.size MappedSegmentDestruct, .func_end_MappedSegmentDestruct-MappedSegmentDestruct

	.global DynamicLoaderFindLibrary
	.type DynamicLoaderFindLibrary, @function

DynamicLoaderFindLibrary:

	// *** Basic block 0

	.global MapFindPointerKey
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
	call        MapFindPointerKey

	// *** Basic block 1


	// *** Basic block 2

.DynamicLoaderFindLibrary_label_17:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DynamicLoaderFindLibrary:
	.size DynamicLoaderFindLibrary, .func_end_DynamicLoaderFindLibrary-DynamicLoaderFindLibrary

	.global DynamicLoaderFindSymbol
	.type DynamicLoaderFindSymbol, @function

DynamicLoaderFindSymbol:

	// *** Basic block 0

	.global LoadedDynamicLibraryFindSymbol
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	sd a2, -40(s0)
	sd a3, -48(s0)
	// Local vars at offset -72(s0)
	// End of stack frame
	sd          x0, -72(s0)
	ld          t0, -72(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 32
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .DynamicLoaderFindSymbol_label_75

	// *** Basic block 1

.DynamicLoaderFindSymbol_label_21:
	ld          t0, -24(s0)
	addi        t0, t0, 32
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -72(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -64(s0)
	ld          a0, -64(s0)
	ld          a1, -32(s0)
	call        LoadedDynamicLibraryFindSymbol

	// *** Basic block 2

	sd          a0, -56(s0)
	ld          t0, -56(s0)
	sub         t1, t0, x0
	snez        t1, t1
	beq         t0, x0, .DynamicLoaderFindSymbol_label_62

	// *** Basic block 3

	ld          t0, -40(s0)
	ld          t1, -56(s0)
	sd          t1, 0(t0)
	ld          t0, -48(s0)
	ld          t1, -64(s0)
	sd          t1, 0(t0)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0

	// *** Basic block 4

.DynamicLoaderFindSymbol_label_59:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.DynamicLoaderFindSymbol_label_62:

	// *** Basic block 6

.DynamicLoaderFindSymbol_label_63:
	ld          t0, -72(s0)
	addi        t0, t0, 1
	sd          t0, -72(s0)
	ld          t0, -72(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 32
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .DynamicLoaderFindSymbol_label_21

	// *** Basic block 7

.DynamicLoaderFindSymbol_label_75:
	mv          a0, x0
	j           .DynamicLoaderFindSymbol_label_59
.func_end_DynamicLoaderFindSymbol:
	.size DynamicLoaderFindSymbol, .func_end_DynamicLoaderFindSymbol-DynamicLoaderFindSymbol

	.global DynamicLoaderLookupSymbolByAddress
	.type DynamicLoaderLookupSymbolByAddress, @function

DynamicLoaderLookupSymbolByAddress:

	// *** Basic block 0

	.global LoadedDynamicLibraryLookupSymbolByAddress
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	sd a2, -40(s0)
	sd a3, -48(s0)
	sd a4, -56(s0)
	// Local vars at offset -80(s0)
	// End of stack frame
	sd          x0, -80(s0)
	ld          t0, -80(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 32
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .DynamicLoaderLookupSymbolByAddress_label_77

	// *** Basic block 1

.DynamicLoaderLookupSymbolByAddress_label_21:
	ld          t0, -24(s0)
	addi        t0, t0, 32
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -80(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -72(s0)
	ld          a0, -72(s0)
	ld          a1, -32(s0)
	ld          a2, -40(s0)
	ld          a3, -48(s0)
	ld          a4, -56(s0)
	call        LoadedDynamicLibraryLookupSymbolByAddress

	// *** Basic block 2

	sb          a0, -64(s0)
	lb          t0, -64(s0)
	beqz        t0, .DynamicLoaderLookupSymbolByAddress_label_64

	// *** Basic block 3

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0

	// *** Basic block 4

.DynamicLoaderLookupSymbolByAddress_label_61:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.DynamicLoaderLookupSymbolByAddress_label_64:

	// *** Basic block 6

.DynamicLoaderLookupSymbolByAddress_label_65:
	ld          t0, -80(s0)
	addi        t0, t0, 1
	sd          t0, -80(s0)
	ld          t0, -80(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 32
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .DynamicLoaderLookupSymbolByAddress_label_21

	// *** Basic block 7

.DynamicLoaderLookupSymbolByAddress_label_77:
	mv          a0, x0
	j           .DynamicLoaderLookupSymbolByAddress_label_61
.func_end_DynamicLoaderLookupSymbolByAddress:
	.size DynamicLoaderLookupSymbolByAddress, .func_end_DynamicLoaderLookupSymbolByAddress-DynamicLoaderLookupSymbolByAddress

	.global DynamicLibraryRegistryInsert
	.type DynamicLibraryRegistryInsert, @function

DynamicLibraryRegistryInsert:

	// *** Basic block 0

	.global MapInsert
	.global VectorAppend
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
	addi        t0, s0, -48
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t1, -24(s0)
	addi        t1, t1, 40
	sd          t1, 0(t0)
	addi        t0, s0, -48
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t1, -24(s0)
	sd          t1, 0(t0)
	ld          t0, -32(s0)
	addi        a0, t0, 0
	addi        t0, s0, -48
	addi        sp, sp, -16
	ld          t1, 0(t0)
	sd          t1, 0(sp)
	ld          t0, 8(t0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 1

	addi        sp, sp, 16
	ld          t0, -32(s0)
	addi        a0, t0, 32
	ld          a1, -24(s0)
	call        VectorAppend

	// *** Basic block 2

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DynamicLibraryRegistryInsert:
	.size DynamicLibraryRegistryInsert, .func_end_DynamicLibraryRegistryInsert-DynamicLibraryRegistryInsert

	.global DynamicLoaderFindDynamicSectionAddressEntry
	.type DynamicLoaderFindDynamicSectionAddressEntry, @function

DynamicLoaderFindDynamicSectionAddressEntry:

	// *** Basic block 0

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
	addi        t0, t0, 144
	ld          t0, 0(t0)
	sd          t0, -48(s0)
	sd          x0, -40(s0)
	ld          t0, -48(s0)
	addi        t0, t0, 0
	ld          t1, -40(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .DynamicLoaderFindDynamicSectionAddressEntry_label_91

	// *** Basic block 1

.DynamicLoaderFindDynamicSectionAddressEntry_label_30:
	ld          t0, -48(s0)
	addi        t0, t0, 0
	ld          t1, -40(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	lw          t1, -32(s0)
	sub         t2, t0, t1
	seqz        t2, t2
	bne         t0, t1, .DynamicLoaderFindDynamicSectionAddressEntry_label_76

	// *** Basic block 2

	ld          t0, -24(s0)
	addi        t0, t0, 240
	lb          t0, 0(t0)
	beqz        t0, .DynamicLoaderFindDynamicSectionAddressEntry_label_60

	// *** Basic block 3

	ld          t0, -48(s0)
	addi        t0, t0, 0
	ld          t1, -40(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	mv          a0, t0

	// *** Basic block 4

.DynamicLoaderFindDynamicSectionAddressEntry_label_57:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.DynamicLoaderFindDynamicSectionAddressEntry_label_60:
	ld          t0, -24(s0)
	addi        t0, t0, 112
	ld          t0, 0(t0)
	ld          t1, -48(s0)
	addi        t1, t1, 0
	ld          t2, -40(s0)
	slli        t2, t2, 4
	add         t1, t1, t2
	addi        t1, t1, 8
	addi        t1, t1, 0
	ld          t1, 0(t1)
	add         t0, t0, x0
	mv          a0, t0
	j           .DynamicLoaderFindDynamicSectionAddressEntry_label_57

	// *** Basic block 6

.DynamicLoaderFindDynamicSectionAddressEntry_label_76:

	// *** Basic block 7

.DynamicLoaderFindDynamicSectionAddressEntry_label_77:
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	ld          t0, -48(s0)
	addi        t0, t0, 0
	ld          t1, -40(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .DynamicLoaderFindDynamicSectionAddressEntry_label_30

	// *** Basic block 8

.DynamicLoaderFindDynamicSectionAddressEntry_label_91:
	mv          a0, x0
	j           .DynamicLoaderFindDynamicSectionAddressEntry_label_57
.func_end_DynamicLoaderFindDynamicSectionAddressEntry:
	.size DynamicLoaderFindDynamicSectionAddressEntry, .func_end_DynamicLoaderFindDynamicSectionAddressEntry-DynamicLoaderFindDynamicSectionAddressEntry

	.global DynamicLoaderFindDynamicSectionOffsetEntry
	.type DynamicLoaderFindDynamicSectionOffsetEntry, @function

DynamicLoaderFindDynamicSectionOffsetEntry:

	// *** Basic block 0

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
	addi        t0, t0, 144
	ld          t0, 0(t0)
	sd          t0, -48(s0)
	sd          x0, -40(s0)
	ld          t0, -48(s0)
	addi        t0, t0, 0
	ld          t1, -40(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .DynamicLoaderFindDynamicSectionOffsetEntry_label_70

	// *** Basic block 1

.DynamicLoaderFindDynamicSectionOffsetEntry_label_29:
	ld          t0, -48(s0)
	addi        t0, t0, 0
	ld          t1, -40(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	lw          t1, -32(s0)
	sub         t2, t0, t1
	seqz        t2, t2
	bne         t0, t1, .DynamicLoaderFindDynamicSectionOffsetEntry_label_55

	// *** Basic block 2

	ld          t0, -48(s0)
	addi        t0, t0, 0
	ld          t1, -40(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	mv          a0, t0

	// *** Basic block 3

.DynamicLoaderFindDynamicSectionOffsetEntry_label_52:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.DynamicLoaderFindDynamicSectionOffsetEntry_label_55:

	// *** Basic block 5

.DynamicLoaderFindDynamicSectionOffsetEntry_label_56:
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	ld          t0, -48(s0)
	addi        t0, t0, 0
	ld          t1, -40(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .DynamicLoaderFindDynamicSectionOffsetEntry_label_29

	// *** Basic block 6

.DynamicLoaderFindDynamicSectionOffsetEntry_label_70:
	li          t0, -1		// 0xffffffffffffffff
	mv          a0, t0
	j           .DynamicLoaderFindDynamicSectionOffsetEntry_label_52
.func_end_DynamicLoaderFindDynamicSectionOffsetEntry:
	.size DynamicLoaderFindDynamicSectionOffsetEntry, .func_end_DynamicLoaderFindDynamicSectionOffsetEntry-DynamicLoaderFindDynamicSectionOffsetEntry

	.global DynamicLoaderGNUHash
	.type DynamicLoaderGNUHash, @function

DynamicLoaderGNUHash:

	// *** Basic block 0

	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	li          t0, 5381		// 0x1505
	sw          t0, -32(s0)
	ld          t0, -24(s0)
	lb          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .DynamicLoaderGNUHash_label_32

	// *** Basic block 1

.DynamicLoaderGNUHash_label_16:
	lwu         t0, -32(s0)
	slli        t0, t0, 5
	lwu         t1, -32(s0)
	add         t0, t0, t1
	ld          t1, -24(s0)
	addi        t1, t1, 1
	sd          t1, -24(s0)
	lb          t1, 0(t1)
	add         t0, t0, t1
	sw          t0, -32(s0)
	ld          t0, -24(s0)
	lb          t0, 0(t0)
	snez        t1, t0
	bnez        t0, .DynamicLoaderGNUHash_label_16

	// *** Basic block 2

.DynamicLoaderGNUHash_label_32:
	lwu         t0, -32(s0)
	mv          a0, t0

	// *** Basic block 3

.DynamicLoaderGNUHash_label_36:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DynamicLoaderGNUHash:
	.size DynamicLoaderGNUHash, .func_end_DynamicLoaderGNUHash-DynamicLoaderGNUHash

	.global DynamicLoaderBloomBits64
	.type DynamicLoaderBloomBits64, @function

DynamicLoaderBloomBits64:

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
	lwu         t0, -24(s0)
	andi        t0, t0, 63
	li          t1, 1		// 0x1 ASCII \x1
	sll         t0, t1, t0
	lwu         t1, -24(s0)
	srli        t1, t1, 26
	andi        t1, t1, 63
	li          t2, 1		// 0x1 ASCII \x1
	sll         t1, t2, t1
	or          t0, t0, t1
	mv          a0, t0

	// *** Basic block 1

.DynamicLoaderBloomBits64_label_20:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DynamicLoaderBloomBits64:
	.size DynamicLoaderBloomBits64, .func_end_DynamicLoaderBloomBits64-DynamicLoaderBloomBits64

	.local  FindDynamicLibraryFile
	.type FindDynamicLibraryFile, @function

FindDynamicLibraryFile:

	// *** Basic block 0

	.global stat
	.global StringSet
	.global StringClear
	.global StringPrintf
	addi sp, sp, -224
	// Saved return address (offset 216) and frame pointer (offset 208)
	sd ra, 216(sp)
	sd s0, 208(sp)
	addi s0, sp, 224
	// Saved argument registers.
	sd a2, -24(s0)
	sd a3, -32(s0)
	sd a0, -40(s0)
	sd a1, -48(s0)
	// Local vars at offset -208(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 0
	lb          t0, 0(t0)
	addi        t1, t0, -47
	seqz        s1, t1
	li          t1, 47		// 0x2f ASCII '/'
	bne         t0, t1, .FindDynamicLibraryFile_label_35

	// *** Basic block 1

	ld          a0, -24(s0)
	addi        a1, s0, -208
	call        stat

	// *** Basic block 2

	seqz        s1, a0

	// *** Basic block 3

.FindDynamicLibraryFile_label_35:
	beqz        s1, .FindDynamicLibraryFile_label_49

	// *** Basic block 4

	ld          a0, -32(s0)
	ld          a1, -24(s0)
	call        StringSet

	// *** Basic block 5

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0

	// *** Basic block 6

.FindDynamicLibraryFile_label_46:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.FindDynamicLibraryFile_label_49:
	ld          t0, -40(s0)
	sub         t1, t0, x0
	snez        t1, t1
	beq         t0, x0, .FindDynamicLibraryFile_label_118

	// *** Basic block 8

	sd          x0, -80(s0)
	ld          t0, -80(s0)
	ld          t1, -40(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindDynamicLibraryFile_label_117

	// *** Basic block 9

.FindDynamicLibraryFile_label_63:
	ld          t0, -40(s0)
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -80(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -72(s0)
	ld          a0, -32(s0)
	call        StringClear

	// *** Basic block 10

	ld          a0, -32(s0)
	lla         a1, .str.1
	ld          t0, -72(s0)
	addi        t0, t0, 16
	ld          a2, 0(t0)
	ld          a3, -24(s0)
	call        StringPrintf

	// *** Basic block 11

	ld          t0, -32(s0)
	addi        t0, t0, 16
	ld          a0, 0(t0)
	addi        a1, s0, -208
	call        stat

	// *** Basic block 12

	seqz        t0, a0
	bnez        a0, .FindDynamicLibraryFile_label_105

	// *** Basic block 13

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .FindDynamicLibraryFile_label_46

	// *** Basic block 14

.FindDynamicLibraryFile_label_105:

	// *** Basic block 15

.FindDynamicLibraryFile_label_106:
	ld          t0, -80(s0)
	addi        t0, t0, 1
	sd          t0, -80(s0)
	ld          t0, -80(s0)
	ld          t1, -40(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindDynamicLibraryFile_label_63

	// *** Basic block 16

.FindDynamicLibraryFile_label_117:

	// *** Basic block 17

.FindDynamicLibraryFile_label_118:
	ld          t0, -48(s0)
	sub         t1, t0, x0
	snez        t1, t1
	beq         t0, x0, .FindDynamicLibraryFile_label_184

	// *** Basic block 18

	sd          x0, -64(s0)
	ld          t0, -64(s0)
	ld          t1, -48(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindDynamicLibraryFile_label_183

	// *** Basic block 19

.FindDynamicLibraryFile_label_132:
	ld          t0, -48(s0)
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -64(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -56(s0)
	ld          a0, -32(s0)
	call        StringClear

	// *** Basic block 20

	ld          a0, -32(s0)
	lla         a1, .str.2
	ld          t0, -56(s0)
	addi        t0, t0, 16
	ld          a2, 0(t0)
	ld          a3, -24(s0)
	call        StringPrintf

	// *** Basic block 21

	ld          t0, -32(s0)
	addi        t0, t0, 16
	ld          a0, 0(t0)
	addi        a1, s0, -208
	call        stat

	// *** Basic block 22

	seqz        t0, a0
	bnez        a0, .FindDynamicLibraryFile_label_171

	// *** Basic block 23

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .FindDynamicLibraryFile_label_46

	// *** Basic block 24

.FindDynamicLibraryFile_label_171:

	// *** Basic block 25

.FindDynamicLibraryFile_label_172:
	ld          t0, -64(s0)
	addi        t0, t0, 1
	sd          t0, -64(s0)
	ld          t0, -64(s0)
	ld          t1, -48(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindDynamicLibraryFile_label_132

	// *** Basic block 26

.FindDynamicLibraryFile_label_183:

	// *** Basic block 27

.FindDynamicLibraryFile_label_184:
	mv          a0, x0
	j           .FindDynamicLibraryFile_label_46
.func_end_FindDynamicLibraryFile:
	.size FindDynamicLibraryFile, .func_end_FindDynamicLibraryFile-FindDynamicLibraryFile

	.local  LoadNeededLibraries
	.type LoadNeededLibraries, @function

LoadNeededLibraries:

	// *** Basic block 0

	.global DynamicLoaderFindDynamicSectionAddressEntry
	.global VectorCopy
	.global VectorAppendVector
	.local FindDynamicLibraryFile
	.global DynamicLoaderFindLibrary
	.global NewLoadedDynamicLibrary
	.global DynamicLibraryRegistryInsert
	.global VectorAppend
	.global LoaderError
	.global StringDestruct
	.global LoadedDynamicLibraryLoad
	.local NextAddress
	.global VectorDestruct
	addi sp, sp, -256
	// Saved return address (offset 248) and frame pointer (offset 240)
	sd ra, 248(sp)
	sd s0, 240(sp)
	addi s0, sp, 256
	// Saved argument registers.
	sd a1, -24(s0)
	sd a2, -32(s0)
	sd a0, -40(s0)
	sd a3, -48(s0)
	sd a4, -56(s0)
	// Local vars at offset -256(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t0, 0(t0)
	sd          t0, -256(s0)
	ld          a0, -24(s0)
	li          a1, 5		// 0x5 ASCII \x5
	call        DynamicLoaderFindDynamicSectionAddressEntry

	// *** Basic block 1

	sd          a0, -248(s0)
	ld          t0, -248(s0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .LoadNeededLibraries_label_51

	// *** Basic block 2

.LoadNeededLibraries_label_48:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.LoadNeededLibraries_label_51:
	sd          x0, -240(s0)
	sd          x0, -232(s0)
	sd          x0, -224(s0)
	sd          x0, -216(s0)
	sd          x0, -208(s0)
	sb          x0, -240(s0)
	sd          x0, -200(s0)
	sd          x0, -192(s0)
	sd          x0, -184(s0)
	sd          x0, -200(s0)
	sd          x0, -176(s0)
	sd          x0, -168(s0)
	sd          x0, -160(s0)
	sd          x0, -176(s0)
	addi        a0, s0, -176
	ld          a1, -32(s0)
	call        VectorCopy

	// *** Basic block 4

	addi        a0, s0, -176
	ld          t0, -24(s0)
	addi        a1, t0, 160
	call        VectorAppendVector

	// *** Basic block 5

	sd          x0, -152(s0)
	ld          t0, -256(s0)
	addi        t0, t0, 0
	ld          t1, -152(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .LoadNeededLibraries_label_214

	// *** Basic block 6

.LoadNeededLibraries_label_101:
	ld          t0, -256(s0)
	addi        t0, t0, 0
	ld          t1, -152(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	addi        t1, t0, -1
	seqz        t1, t1
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .LoadNeededLibraries_label_199

	// *** Basic block 7

	ld          t0, -248(s0)
	ld          t1, -256(s0)
	addi        t1, t1, 0
	ld          t2, -152(s0)
	slli        t2, t2, 4
	add         t1, t1, t2
	addi        t1, t1, 8
	addi        t1, t1, 0
	ld          t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -144(s0)
	sd          x0, -136(s0)
	sd          x0, -128(s0)
	sd          x0, -120(s0)
	sd          x0, -112(s0)
	sd          x0, -104(s0)
	sb          x0, -136(s0)
	addi        a1, s0, -176
	ld          a2, -144(s0)
	addi        a3, s0, -136
	mv          a0, x0
	call        FindDynamicLibraryFile

	// *** Basic block 8

	sb          a0, -96(s0)
	lb          t0, -96(s0)
	beqz        t0, .LoadNeededLibraries_label_188

	// *** Basic block 9

	ld          a0, -40(s0)
	addi        a1, s0, -136
	call        DynamicLoaderFindLibrary

	// *** Basic block 10

	sd          a0, -88(s0)
	ld          t0, -88(s0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .LoadNeededLibraries_label_186

	// *** Basic block 11

	addi        t0, s0, -136
	addi        t0, t0, 16
	ld          a0, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 80
	ld          a1, 0(t0)
	call        NewLoadedDynamicLibrary

	// *** Basic block 12

	sd          a0, -80(s0)
	ld          a0, -40(s0)
	ld          a1, -80(s0)
	call        DynamicLibraryRegistryInsert

	// *** Basic block 13

	addi        a0, s0, -200
	ld          a1, -80(s0)
	call        VectorAppend

	// *** Basic block 14

.LoadNeededLibraries_label_186:
	j           .LoadNeededLibraries_label_195

	// *** Basic block 15

.LoadNeededLibraries_label_188:
	lla         a0, .str.3
	ld          a1, -144(s0)
	call        LoaderError

	// *** Basic block 16

.LoadNeededLibraries_label_195:
	addi        a0, s0, -136
	call        StringDestruct

	// *** Basic block 17

.LoadNeededLibraries_label_199:

	// *** Basic block 18

.LoadNeededLibraries_label_200:
	ld          t0, -152(s0)
	addi        t0, t0, 1
	sd          t0, -152(s0)
	ld          t0, -256(s0)
	addi        t0, t0, 0
	ld          t1, -152(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .LoadNeededLibraries_label_101

	// *** Basic block 19

.LoadNeededLibraries_label_214:
	sd          x0, -72(s0)
	ld          t0, -72(s0)
	addi        t1, s0, -200
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoadNeededLibraries_label_267

	// *** Basic block 20

.LoadNeededLibraries_label_223:
	addi        t0, s0, -200
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -72(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -64(s0)
	ld          a0, -64(s0)
	ld          a1, -40(s0)
	ld          a2, -32(s0)
	ld          a3, -48(s0)
	ld          a4, -56(s0)
	call        LoadedDynamicLibraryLoad

	// *** Basic block 21

	ld          a0, -64(s0)
	ld          a1, -48(s0)
	ld          a2, -56(s0)
	call        NextAddress

	// *** Basic block 22

	sd          a0, -48(s0)

	// *** Basic block 23

.LoadNeededLibraries_label_256:
	ld          t0, -72(s0)
	addi        t0, t0, 1
	sd          t0, -72(s0)
	ld          t0, -72(s0)
	addi        t1, s0, -200
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoadNeededLibraries_label_223

	// *** Basic block 24

.LoadNeededLibraries_label_267:
	addi        a0, s0, -240
	call        StringDestruct

	// *** Basic block 25

	addi        a0, s0, -200
	call        VectorDestruct

	// *** Basic block 26

	addi        a0, s0, -176
	call        VectorDestruct

	// *** Basic block 27

	j           .LoadNeededLibraries_label_48
.func_end_LoadNeededLibraries:
	.size LoadNeededLibraries, .func_end_LoadNeededLibraries-LoadNeededLibraries

	.local  RelocateDynamicSection
	.type RelocateDynamicSection, @function

RelocateDynamicSection:

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
	addi        t0, t0, 120
	ld          t0, 0(t0)
	addi        t0, t0, 16
	lhu         t0, 0(t0)
	addi        t1, t0, -3
	snez        t1, t1
	li          t1, 3		// 0x3 ASCII \x3
	beq         t0, t1, .RelocateDynamicSection_label_39

	// *** Basic block 1

	ld          t0, -24(s0)
	addi        t0, t0, 240
	li          t1, 1		// 0x1 ASCII \x1
	sb          t1, 0(t0)

	// *** Basic block 2

.RelocateDynamicSection_label_36:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.RelocateDynamicSection_label_39:
	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t0, 0(t0)
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .RelocateDynamicSection_label_50

	// *** Basic block 4

	j           .RelocateDynamicSection_label_36

	// *** Basic block 5

.RelocateDynamicSection_label_50:
	sd          x0, -32(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 0
	ld          t1, -32(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .RelocateDynamicSection_label_137

	// *** Basic block 6

.RelocateDynamicSection_label_63:
	ld          t0, -40(s0)
	addi        t0, t0, 0
	ld          t1, -32(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	addi        t1, t0, -3
	seqz        t1, t1
	li          t1, 3		// 0x3 ASCII \x3
	beq         t0, t1, .RelocateDynamicSection_label_106

	// *** Basic block 7

	addi        t1, t0, -5
	seqz        t1, t1
	li          t1, 5		// 0x5 ASCII \x5
	beq         t0, t1, .RelocateDynamicSection_label_103

	// *** Basic block 8

	addi        t1, t0, -6
	seqz        t1, t1
	li          t1, 6		// 0x6 ASCII \x6
	beq         t0, t1, .RelocateDynamicSection_label_102

	// *** Basic block 9

	addi        t1, t0, -7
	seqz        t1, t1
	li          t1, 7		// 0x7 ASCII \x7
	beq         t0, t1, .RelocateDynamicSection_label_104

	// *** Basic block 10

	addi        t1, t0, -23
	seqz        t1, t1
	li          t1, 23		// 0x17 ASCII \x17
	beq         t0, t1, .RelocateDynamicSection_label_105

	// *** Basic block 11

	li          t1, 1879047925		// 0x6ffffef5
	sub         t1, t0, t1
	seqz        t1, t1
	li          t1, 1879047925		// 0x6ffffef5
	beq         t0, t1, .RelocateDynamicSection_label_107

	// *** Basic block 12

.RelocateDynamicSection_label_100:
	j           .RelocateDynamicSection_label_122

	// *** Basic block 13

.RelocateDynamicSection_label_102:

	// *** Basic block 14

.RelocateDynamicSection_label_103:

	// *** Basic block 15

.RelocateDynamicSection_label_104:

	// *** Basic block 16

.RelocateDynamicSection_label_105:

	// *** Basic block 17

.RelocateDynamicSection_label_106:

	// *** Basic block 18

.RelocateDynamicSection_label_107:
	ld          t0, -24(s0)
	addi        t0, t0, 112
	ld          t0, 0(t0)
	ld          t1, -40(s0)
	addi        t1, t1, 0
	ld          t2, -32(s0)
	slli        t2, t2, 4
	add         t1, t1, t2
	addi        t1, t1, 8
	addi        t1, t1, 0
	ld          t2, 0(t1)
	add         t0, t2, t0
	sd          t0, 0(t1)
	j           .RelocateDynamicSection_label_122

	// *** Basic block 19

.RelocateDynamicSection_label_122:

	// *** Basic block 20

.RelocateDynamicSection_label_123:
	ld          t0, -32(s0)
	addi        t0, t0, 1
	sd          t0, -32(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 0
	ld          t1, -32(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .RelocateDynamicSection_label_63

	// *** Basic block 21

.RelocateDynamicSection_label_137:
	ld          t0, -24(s0)
	addi        t0, t0, 240
	li          t1, 1		// 0x1 ASCII \x1
	sb          t1, 0(t0)
	j           .RelocateDynamicSection_label_36
.func_end_RelocateDynamicSection:
	.size RelocateDynamicSection, .func_end_RelocateDynamicSection-RelocateDynamicSection

	.local  PerformDynamicRelocations
	.type PerformDynamicRelocations, @function

PerformDynamicRelocations:

	// *** Basic block 0

	.global DynamicLoaderFindSymbol
	addi sp, sp, -272
	// Saved return address (offset 264) and frame pointer (offset 256)
	sd ra, 264(sp)
	sd s0, 256(sp)
	addi s0, sp, 272
	// Saved argument registers.
	sd a1, -24(s0)
	sd a0, -32(s0)
	sd a2, -40(s0)
	sd a3, -48(s0)
	// Local vars at offset -272(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t0, 0(t0)
	sd          t0, -272(s0)
	ld          t0, -272(s0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .PerformDynamicRelocations_label_43

	// *** Basic block 1

.PerformDynamicRelocations_label_40:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.PerformDynamicRelocations_label_43:
	sd          x0, -264(s0)
	sd          x0, -256(s0)
	sd          x0, -248(s0)
	sd          x0, -240(s0)
	sd          x0, -232(s0)
	li          t0, 7		// 0x7 ASCII \x7
	sd          t0, -224(s0)
	sd          x0, -216(s0)
	sd          x0, -208(s0)
	sd          x0, -200(s0)
	sd          x0, -192(s0)
	sd          x0, -184(s0)
	ld          t0, -272(s0)
	addi        t0, t0, 0
	ld          t1, -184(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .PerformDynamicRelocations_label_261

	// *** Basic block 3

.PerformDynamicRelocations_label_77:
	ld          t0, -272(s0)
	addi        t0, t0, 0
	ld          t1, -184(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	addi        t1, t0, -2
	seqz        t1, t1
	li          t1, 2		// 0x2 ASCII \x2
	beq         t0, t1, .PerformDynamicRelocations_label_235

	// *** Basic block 4

	addi        t1, t0, -3
	seqz        t1, t1
	li          t1, 3		// 0x3 ASCII \x3
	beq         t0, t1, .PerformDynamicRelocations_label_180

	// *** Basic block 5

	addi        t1, t0, -5
	seqz        t1, t1
	li          t1, 5		// 0x5 ASCII \x5
	beq         t0, t1, .PerformDynamicRelocations_label_147

	// *** Basic block 6

	addi        t1, t0, -6
	seqz        t1, t1
	li          t1, 6		// 0x6 ASCII \x6
	beq         t0, t1, .PerformDynamicRelocations_label_136

	// *** Basic block 7

	addi        t1, t0, -7
	seqz        t1, t1
	li          t1, 7		// 0x7 ASCII \x7
	beq         t0, t1, .PerformDynamicRelocations_label_158

	// *** Basic block 8

	addi        t1, t0, -8
	seqz        t1, t1
	li          t1, 8		// 0x8 ASCII \x8
	beq         t0, t1, .PerformDynamicRelocations_label_202

	// *** Basic block 9

	addi        t1, t0, -9
	seqz        t1, t1
	li          t1, 9		// 0x9 ASCII \x9
	beq         t0, t1, .PerformDynamicRelocations_label_224

	// *** Basic block 10

	addi        t1, t0, -20
	seqz        t1, t1
	li          t1, 20		// 0x14 ASCII \x14
	beq         t0, t1, .PerformDynamicRelocations_label_191

	// *** Basic block 11

	addi        t1, t0, -23
	seqz        t1, t1
	li          t1, 23		// 0x17 ASCII \x17
	beq         t0, t1, .PerformDynamicRelocations_label_169

	// *** Basic block 12

	li          t1, 1879048185		// 0x6ffffff9
	sub         t1, t0, t1
	seqz        t1, t1
	li          t1, 1879048185		// 0x6ffffff9
	beq         t0, t1, .PerformDynamicRelocations_label_213

	// *** Basic block 13

	j           .PerformDynamicRelocations_label_246

	// *** Basic block 14

.PerformDynamicRelocations_label_136:
	ld          t0, -272(s0)
	addi        t0, t0, 0
	ld          t1, -184(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	sd          t0, -264(s0)
	j           .PerformDynamicRelocations_label_246

	// *** Basic block 15

.PerformDynamicRelocations_label_147:
	ld          t0, -272(s0)
	addi        t0, t0, 0
	ld          t1, -184(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	sd          t0, -256(s0)
	j           .PerformDynamicRelocations_label_246

	// *** Basic block 16

.PerformDynamicRelocations_label_158:
	ld          t0, -272(s0)
	addi        t0, t0, 0
	ld          t1, -184(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	sd          t0, -248(s0)
	j           .PerformDynamicRelocations_label_246

	// *** Basic block 17

.PerformDynamicRelocations_label_169:
	ld          t0, -272(s0)
	addi        t0, t0, 0
	ld          t1, -184(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	sd          t0, -240(s0)
	j           .PerformDynamicRelocations_label_246

	// *** Basic block 18

.PerformDynamicRelocations_label_180:
	ld          t0, -272(s0)
	addi        t0, t0, 0
	ld          t1, -184(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	sd          t0, -232(s0)
	j           .PerformDynamicRelocations_label_246

	// *** Basic block 19

.PerformDynamicRelocations_label_191:
	ld          t0, -272(s0)
	addi        t0, t0, 0
	ld          t1, -184(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	sd          t0, -224(s0)
	j           .PerformDynamicRelocations_label_246

	// *** Basic block 20

.PerformDynamicRelocations_label_202:
	ld          t0, -272(s0)
	addi        t0, t0, 0
	ld          t1, -184(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	sd          t0, -216(s0)
	j           .PerformDynamicRelocations_label_246

	// *** Basic block 21

.PerformDynamicRelocations_label_213:
	ld          t0, -272(s0)
	addi        t0, t0, 0
	ld          t1, -184(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	sd          t0, -200(s0)
	j           .PerformDynamicRelocations_label_246

	// *** Basic block 22

.PerformDynamicRelocations_label_224:
	ld          t0, -272(s0)
	addi        t0, t0, 0
	ld          t1, -184(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	sd          t0, -208(s0)
	j           .PerformDynamicRelocations_label_246

	// *** Basic block 23

.PerformDynamicRelocations_label_235:
	ld          t0, -272(s0)
	addi        t0, t0, 0
	ld          t1, -184(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	sd          t0, -192(s0)
	j           .PerformDynamicRelocations_label_246

	// *** Basic block 24

.PerformDynamicRelocations_label_246:

	// *** Basic block 25

.PerformDynamicRelocations_label_247:
	ld          t0, -184(s0)
	addi        t0, t0, 1
	sd          t0, -184(s0)
	ld          t0, -272(s0)
	addi        t0, t0, 0
	ld          t1, -184(s0)
	slli        t1, t1, 4
	add         t0, t0, t1
	addi        t0, t0, 0
	ld          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .PerformDynamicRelocations_label_77

	// *** Basic block 26

.PerformDynamicRelocations_label_261:
	ld          t1, -264(s0)
	sub         t2, t1, x0
	seqz        t0, t2
	beq         t1, x0, .PerformDynamicRelocations_label_270

	// *** Basic block 27

	ld          t1, -256(s0)
	sub         t1, t1, x0
	seqz        t0, t1

	// *** Basic block 28

.PerformDynamicRelocations_label_270:
	beqz        t0, .PerformDynamicRelocations_label_273

	// *** Basic block 29

	j           .PerformDynamicRelocations_label_40

	// *** Basic block 30

.PerformDynamicRelocations_label_273:
	ld          t0, -32(s0)
	addi        t0, t0, 48
	ld          t0, 0(t0)
	addi        t0, t0, 24
	ld          t0, 0(t0)
	ld          a0, -24(s0)
	ld          t1, -32(s0)
	addi        t1, t1, 56
	ld          a1, 0(t1)
	jalr         x1, t0, 0

	// *** Basic block 31

	ld          t0, -248(s0)
	sub         t1, t0, x0
	snez        t1, t1
	beq         t0, x0, .PerformDynamicRelocations_label_416

	// *** Basic block 32

	sd          x0, -176(s0)
	ld          t0, -200(s0)
	slt         t1, x0, t0
	bge         x0, t0, .PerformDynamicRelocations_label_301

	// *** Basic block 33

	ld          t0, -200(s0)
	sd          t0, -176(s0)
	j           .PerformDynamicRelocations_label_315

	// *** Basic block 34

.PerformDynamicRelocations_label_301:
	ld          t1, -216(s0)
	slt         t0, x0, t1
	bge         x0, t1, .PerformDynamicRelocations_label_308

	// *** Basic block 35

	ld          t1, -208(s0)
	slt         t0, x0, t1

	// *** Basic block 36

.PerformDynamicRelocations_label_308:
	beqz        t0, .PerformDynamicRelocations_label_314

	// *** Basic block 37

	ld          t0, -216(s0)
	ld          t1, -208(s0)
	div         t0, t0, t1
	sd          t0, -176(s0)

	// *** Basic block 38

.PerformDynamicRelocations_label_314:

	// *** Basic block 39

.PerformDynamicRelocations_label_315:
	sd          x0, -168(s0)
	ld          t0, -168(s0)
	ld          t1, -176(s0)
	slt         t2, t0, t1
	bge         t0, t1, .PerformDynamicRelocations_label_415

	// *** Basic block 40

.PerformDynamicRelocations_label_322:
	ld          t0, -248(s0)
	ld          t1, -168(s0)
	slli        t2, t1, 3
	slli        t1, t1, 4
	add         t1, t2, t1
	add         t0, t0, t1
	sd          t0, -160(s0)
	ld          t0, -160(s0)
	addi        t0, t0, 8
	ld          t0, 0(t0)
	srli        t0, t0, 32
	sext.w      t1, t0
	sw          t0, -152(s0)
	ld          t0, -256(s0)
	ld          t1, -264(s0)
	lw          t2, -152(s0)
	slli        t3, t2, 3
	slli        t2, t2, 4
	add         t2, t3, t2
	add         t1, t1, t2
	addi        t1, t1, 0
	lwu         t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -144(s0)
	sd          x0, -136(s0)
	sd          x0, -128(s0)
	ld          a0, -40(s0)
	ld          a1, -144(s0)
	addi        a2, s0, -136
	addi        a3, s0, -128
	call        DynamicLoaderFindSymbol

	// *** Basic block 41

	ld          t0, -128(s0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .PerformDynamicRelocations_label_374

	// *** Basic block 42

	ld          t0, -24(s0)
	sd          t0, -128(s0)

	// *** Basic block 43

.PerformDynamicRelocations_label_374:
	ld          t0, -24(s0)
	addi        t0, t0, 112
	ld          t0, 0(t0)
	ld          t1, -160(s0)
	addi        t1, t1, 0
	ld          t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -120(s0)
	ld          t0, -32(s0)
	addi        t0, t0, 48
	ld          t0, 0(t0)
	addi        t0, t0, 32
	ld          t0, 0(t0)
	ld          a0, -128(s0)
	ld          a1, -160(s0)
	ld          a2, -136(s0)
	ld          a3, -144(s0)
	ld          a4, -120(s0)
	lb          a5, -48(s0)
	jalr         x1, t0, 0

	// *** Basic block 44

.PerformDynamicRelocations_label_406:
	ld          t0, -168(s0)
	addi        t0, t0, 1
	sd          t0, -168(s0)
	ld          t0, -168(s0)
	ld          t1, -176(s0)
	slt         t2, t0, t1
	bge         t0, t1, .PerformDynamicRelocations_label_322

	// *** Basic block 45

.PerformDynamicRelocations_label_415:

	// *** Basic block 46

.PerformDynamicRelocations_label_416:
	ld          t1, -240(s0)
	sub         t2, t1, x0
	snez        t0, t2
	beq         t1, x0, .PerformDynamicRelocations_label_424

	// *** Basic block 47

	ld          t1, -192(s0)
	snez        t0, t1

	// *** Basic block 48

.PerformDynamicRelocations_label_424:
	beqz        t0, .PerformDynamicRelocations_label_532

	// *** Basic block 49

	ld          t0, -192(s0)
	ld          t2, -224(s0)
	addi        t3, t2, -7
	snez        t3, t3
	li          t3, 7		// 0x7 ASCII \x7
	beq         t2, t3, .PerformDynamicRelocations_label_435

	// *** Basic block 50

	li          t1, 16		// 0x10 ASCII \x10
	j           .PerformDynamicRelocations_label_437

	// *** Basic block 51

.PerformDynamicRelocations_label_435:
	li          t1, 24		// 0x18 ASCII \x18

	// *** Basic block 52

.PerformDynamicRelocations_label_437:
	div         t0, t0, t1
	sd          t0, -112(s0)
	sd          x0, -104(s0)
	ld          t0, -104(s0)
	ld          t1, -112(s0)
	slt         t2, t0, t1
	bge         t0, t1, .PerformDynamicRelocations_label_531

	// *** Basic block 53

.PerformDynamicRelocations_label_447:
	ld          t0, -240(s0)
	ld          t1, -104(s0)
	slli        t2, t1, 3
	slli        t1, t1, 4
	add         t1, t2, t1
	add         t0, t0, t1
	sd          t0, -96(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 112
	ld          t0, 0(t0)
	ld          t1, -96(s0)
	addi        t1, t1, 0
	ld          t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -88(s0)
	ld          t0, -96(s0)
	addi        t0, t0, 8
	ld          t0, 0(t0)
	srli        t0, t0, 32
	sext.w      t1, t0
	sw          t0, -80(s0)
	ld          t0, -256(s0)
	ld          t1, -264(s0)
	lw          t2, -80(s0)
	slli        t3, t2, 3
	slli        t2, t2, 4
	add         t2, t3, t2
	add         t1, t1, t2
	addi        t1, t1, 0
	lwu         t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -72(s0)
	sd          x0, -64(s0)
	ld          t0, -24(s0)
	sd          t0, -56(s0)
	lb          t0, -48(s0)
	not         t0, t0
	beqz        t0, .PerformDynamicRelocations_label_503

	// *** Basic block 54

	ld          a0, -40(s0)
	ld          a1, -72(s0)
	addi        a2, s0, -64
	addi        a3, s0, -56
	call        DynamicLoaderFindSymbol

	// *** Basic block 55

.PerformDynamicRelocations_label_503:
	ld          t0, -32(s0)
	addi        t0, t0, 48
	ld          t0, 0(t0)
	addi        t0, t0, 40
	ld          t0, 0(t0)
	ld          a0, -56(s0)
	ld          a1, -96(s0)
	ld          a2, -64(s0)
	ld          a3, -72(s0)
	ld          a4, -88(s0)
	lb          a5, -48(s0)
	jalr         x1, t0, 0

	// *** Basic block 56

.PerformDynamicRelocations_label_522:
	ld          t0, -104(s0)
	addi        t0, t0, 1
	sd          t0, -104(s0)
	ld          t0, -104(s0)
	ld          t1, -112(s0)
	slt         t2, t0, t1
	bge         t0, t1, .PerformDynamicRelocations_label_447

	// *** Basic block 57

.PerformDynamicRelocations_label_531:

	// *** Basic block 58

.PerformDynamicRelocations_label_532:
	j           .PerformDynamicRelocations_label_40
.func_end_PerformDynamicRelocations:
	.size PerformDynamicRelocations, .func_end_PerformDynamicRelocations-PerformDynamicRelocations

	.local  MapDynamicLibraryHeader
	.type MapDynamicLibraryHeader, @function

MapDynamicLibraryHeader:

	// *** Basic block 0

	.global open
	.global read
	.global close
	.global sysconf
	.local AlignUp
	.global mmap
	addi sp, sp, -176
	// Saved return address (offset 168) and frame pointer (offset 160)
	sd ra, 168(sp)
	sd s0, 160(sp)
	addi s0, sp, 176
	// Saved argument registers.
	sd a0, -24(s0)
	sd a2, -32(s0)
	sd a1, -40(s0)
	// Local vars at offset -160(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 16
	ld          a0, 0(t0)
	mv          a1, x0
	call        open

	// *** Basic block 1

	sw          a0, -160(s0)
	lw          t0, -160(s0)
	sltz        t1, t0
	bge         t0, x0, .MapDynamicLibraryHeader_label_45

	// *** Basic block 2

	lw          t0, -160(s0)
	mv          a0, t0

	// *** Basic block 3

.MapDynamicLibraryHeader_label_42:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.MapDynamicLibraryHeader_label_45:
	lw          a0, -160(s0)
	addi        a1, s0, -144
	li          t0, 64		// 0x40 ASCII '@'
	mv          a2, t0
	call        read

	// *** Basic block 5

	sd          a0, -152(s0)
	ld          t0, -152(s0)
	addi        t1, t0, -64
	snez        t1, t1
	li          t1, 64		// 0x40 ASCII '@'
	beq         t0, t1, .MapDynamicLibraryHeader_label_71

	// *** Basic block 6

	lw          a0, -160(s0)
	call        close

	// *** Basic block 7

	li          t0, -1		// 0xffffffffffffffff
	mv          a0, t0
	j           .MapDynamicLibraryHeader_label_42

	// *** Basic block 8

.MapDynamicLibraryHeader_label_71:
	li          t0, 30		// 0x1e ASCII \x1e
	mv          a0, t0
	call        sysconf

	// *** Basic block 9

	sext.w      t0, a0
	sw          t0, -80(s0)
	addi        t0, s0, -144
	addi        t0, t0, 32
	ld          t0, 0(t0)
	addi        t1, s0, -144
	addi        t1, t1, 56
	lhu         t1, 0(t1)
	addi        t2, s0, -144
	addi        t2, t2, 54
	lhu         t2, 0(t2)
	mul         t1, t1, t2
	add         t0, t0, t1
	sd          t0, -72(s0)
	addi        t0, s0, -144
	addi        t0, t0, 40
	ld          t0, 0(t0)
	addi        t1, s0, -144
	addi        t1, t1, 60
	lhu         t1, 0(t1)
	addi        t2, s0, -144
	addi        t2, t2, 58
	lhu         t2, 0(t2)
	mul         t1, t1, t2
	add         t0, t0, t1
	sd          t0, -64(s0)
	ld          t0, -72(s0)
	ld          t1, -64(s0)
	slt         t2, t1, t0
	bge         t1, t0, .MapDynamicLibraryHeader_label_111

	// *** Basic block 10

	ld          s1, -72(s0)
	j           .MapDynamicLibraryHeader_label_113

	// *** Basic block 11

.MapDynamicLibraryHeader_label_111:
	ld          s1, -64(s0)

	// *** Basic block 12

.MapDynamicLibraryHeader_label_113:
	sd          s1, -56(s0)
	ld          a0, -56(s0)
	lw          a1, -80(s0)
	call        AlignUp

	// *** Basic block 13

	sd          a0, -48(s0)
	ld          t0, -32(s0)
	ld          t1, -48(s0)
	sd          t1, 0(t0)
	ld          s1, -40(s0)
	ld          a1, -48(s0)
	lw          a4, -160(s0)
	mv          a5, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a3, t0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a0, x0
	call        mmap

	// *** Basic block 14

	sd          a0, 0(s1)
	ld          t0, -40(s0)
	ld          t0, 0(t0)
	li          t1, -1		// 0xffffffffffffffff
	sub         t2, t0, t1
	seqz        t2, t2
	bne         t0, t1, .MapDynamicLibraryHeader_label_158

	// *** Basic block 15

	li          t0, -1		// 0xffffffffffffffff
	mv          a0, t0
	j           .MapDynamicLibraryHeader_label_42

	// *** Basic block 16

.MapDynamicLibraryHeader_label_158:
	lw          t0, -160(s0)
	mv          a0, t0
	j           .MapDynamicLibraryHeader_label_42
.func_end_MapDynamicLibraryHeader:
	.size MapDynamicLibraryHeader, .func_end_MapDynamicLibraryHeader-MapDynamicLibraryHeader

	.local  MapWholeDynamicLibrary
	.type MapWholeDynamicLibrary, @function

MapWholeDynamicLibrary:

	// *** Basic block 0

	.global stat
	.global open
	.global mmap
	addi sp, sp, -192
	// Saved return address (offset 184) and frame pointer (offset 176)
	sd ra, 184(sp)
	sd s0, 176(sp)
	addi s0, sp, 192
	// Saved argument registers.
	sd a0, -24(s0)
	sd a2, -32(s0)
	sd a1, -40(s0)
	// Local vars at offset -184(s0)
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 16
	ld          a0, 0(t0)
	addi        a1, s0, -176
	call        stat

	// *** Basic block 1

	sw          a0, -184(s0)
	lw          t0, -184(s0)
	snez        t1, t0
	beqz        t0, .MapWholeDynamicLibrary_label_36

	// *** Basic block 2

	li          t0, -1		// 0xffffffffffffffff
	mv          a0, t0

	// *** Basic block 3

.MapWholeDynamicLibrary_label_33:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.MapWholeDynamicLibrary_label_36:
	ld          t0, -32(s0)
	addi        t1, s0, -176
	addi        t1, t1, 48
	ld          t1, 0(t1)
	sd          t1, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 16
	ld          a0, 0(t0)
	mv          a1, x0
	call        open

	// *** Basic block 5

	sw          a0, -48(s0)
	lw          t0, -48(s0)
	sltz        t1, t0
	bge         t0, x0, .MapWholeDynamicLibrary_label_59

	// *** Basic block 6

	lw          t0, -48(s0)
	mv          a0, t0
	j           .MapWholeDynamicLibrary_label_33

	// *** Basic block 7

.MapWholeDynamicLibrary_label_59:
	ld          s1, -40(s0)
	ld          t0, -32(s0)
	ld          a1, 0(t0)
	lw          a4, -48(s0)
	mv          a5, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a3, t0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a0, x0
	call        mmap

	// *** Basic block 8

	sd          a0, 0(s1)
	ld          t0, -40(s0)
	ld          t0, 0(t0)
	li          t1, -1		// 0xffffffffffffffff
	sub         t2, t0, t1
	seqz        t2, t2
	bne         t0, t1, .MapWholeDynamicLibrary_label_93

	// *** Basic block 9

	li          t0, -1		// 0xffffffffffffffff
	mv          a0, t0
	j           .MapWholeDynamicLibrary_label_33

	// *** Basic block 10

.MapWholeDynamicLibrary_label_93:
	lw          t0, -48(s0)
	mv          a0, t0
	j           .MapWholeDynamicLibrary_label_33
.func_end_MapWholeDynamicLibrary:
	.size MapWholeDynamicLibrary, .func_end_MapWholeDynamicLibrary-MapWholeDynamicLibrary

	.local  MapSymbolTable
	.type MapSymbolTable, @function

MapSymbolTable:

	// *** Basic block 0

	.global sysconf
	.local AlignUp
	.global mmap
	.global VectorAppend
	.local NewMappedSegment
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Saved argument registers.
	sd a1, -24(s0)
	sd a2, -32(s0)
	sd a0, -40(s0)
	// Local vars at offset -104(s0)
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 24
	ld          t0, 0(t0)
	ld          t1, -32(s0)
	addi        t1, t1, 24
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .MapSymbolTable_label_46

	// *** Basic block 1

	ld          t0, -24(s0)
	addi        t0, t0, 24
	ld          t0, 0(t0)
	sd          t0, -104(s0)
	ld          t0, -32(s0)
	addi        t0, t0, 24
	ld          t0, 0(t0)
	ld          t1, -32(s0)
	addi        t1, t1, 32
	ld          t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -96(s0)
	j           .MapSymbolTable_label_59

	// *** Basic block 2

.MapSymbolTable_label_46:
	ld          t0, -32(s0)
	addi        t0, t0, 24
	ld          t0, 0(t0)
	sd          t0, -104(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 24
	ld          t0, 0(t0)
	ld          t1, -24(s0)
	addi        t1, t1, 32
	ld          t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -96(s0)

	// *** Basic block 3

.MapSymbolTable_label_59:
	li          t0, 30		// 0x1e ASCII \x1e
	mv          a0, t0
	call        sysconf

	// *** Basic block 4

	sd          a0, -88(s0)
	ld          t0, -88(s0)
	addi        t0, t0, -1
	sd          t0, -80(s0)
	ld          t0, -104(s0)
	ld          t1, -80(s0)
	and         t0, t0, t1
	sd          t0, -72(s0)
	ld          t0, -104(s0)
	ld          t1, -80(s0)
	not         t1, t1
	and         t0, t0, t1
	sd          t0, -64(s0)
	ld          t0, -96(s0)
	ld          t1, -64(s0)
	sub         a0, t0, t1
	ld          a1, -88(s0)
	call        AlignUp

	// *** Basic block 5

	sd          a0, -56(s0)
	ld          a1, -56(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 104
	lw          a4, 0(t0)
	ld          a5, -64(s0)
	li          t0, 2		// 0x2 ASCII \x2
	mv          a3, t0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a0, x0
	call        mmap

	// *** Basic block 6

	sd          a0, -48(s0)
	ld          t0, -48(s0)
	li          t1, -1		// 0xffffffffffffffff
	sub         t2, t0, t1
	seqz        t2, t2
	bne         t0, t1, .MapSymbolTable_label_124

	// *** Basic block 7

.MapSymbolTable_label_121:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 8

.MapSymbolTable_label_124:
	ld          t0, -40(s0)
	addi        t0, t0, 216
	ld          t1, -48(s0)
	ld          t2, -72(s0)
	add         t1, t1, t2
	ld          t2, -24(s0)
	addi        t2, t2, 24
	ld          t2, 0(t2)
	ld          t3, -104(s0)
	sub         t2, t2, t3
	add         t1, t1, t2
	sd          t1, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 224
	ld          t1, -48(s0)
	ld          t2, -72(s0)
	add         t1, t1, t2
	ld          t2, -32(s0)
	addi        t2, t2, 24
	ld          t2, 0(t2)
	ld          t3, -104(s0)
	sub         t2, t2, t3
	add         t1, t1, t2
	sd          t1, 0(t0)
	ld          t0, -40(s0)
	addi        s1, t0, 248
	ld          a0, -48(s0)
	ld          a1, -56(s0)
	call        NewMappedSegment

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 10

	j           .MapSymbolTable_label_121
.func_end_MapSymbolTable:
	.size MapSymbolTable, .func_end_MapSymbolTable-MapSymbolTable

	.local  FindAndLoadSymbolTable
	.type FindAndLoadSymbolTable, @function

FindAndLoadSymbolTable:

	// *** Basic block 0

	.local MapSymbolTable
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -48(s0)
	// End of stack frame
	sw          x0, -48(s0)
	lw          t0, -48(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 120
	ld          t1, 0(t1)
	addi        t1, t1, 60
	lhu         t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindAndLoadSymbolTable_label_91

	// *** Basic block 1

.FindAndLoadSymbolTable_label_28:
	ld          t0, -24(s0)
	addi        t0, t0, 136
	ld          t0, 0(t0)
	lw          t1, -48(s0)
	slli        t1, t1, 6
	add         t0, t0, t1
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 4
	lwu         t0, 0(t0)
	addi        t1, t0, -2
	seqz        t1, t1
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .FindAndLoadSymbolTable_label_77

	// *** Basic block 2

	ld          t0, -24(s0)
	addi        t0, t0, 232
	ld          t1, -40(s0)
	addi        t1, t1, 32
	ld          t1, 0(t1)
	ld          t2, -40(s0)
	addi        t2, t2, 56
	ld          t2, 0(t2)
	div         t1, t1, t2
	sd          t1, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 136
	ld          t0, 0(t0)
	ld          t1, -40(s0)
	addi        t1, t1, 40
	lwu         t1, 0(t1)
	slli        t1, t1, 6
	add         t0, t0, t1
	sd          t0, -32(s0)
	ld          a0, -24(s0)
	ld          a1, -40(s0)
	ld          a2, -32(s0)
	call        MapSymbolTable

	// *** Basic block 3

	j           .FindAndLoadSymbolTable_label_91

	// *** Basic block 4

.FindAndLoadSymbolTable_label_77:

	// *** Basic block 5

.FindAndLoadSymbolTable_label_78:
	lw          t0, -48(s0)
	addi        t0, t0, 1
	sw          t0, -48(s0)
	lw          t0, -48(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 120
	ld          t1, 0(t1)
	addi        t1, t1, 60
	lhu         t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindAndLoadSymbolTable_label_28

	// *** Basic block 6

.FindAndLoadSymbolTable_label_91:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_FindAndLoadSymbolTable:
	.size FindAndLoadSymbolTable, .func_end_FindAndLoadSymbolTable-FindAndLoadSymbolTable

	.local  LoadSegments
	.type LoadSegments, @function

LoadSegments:

	// *** Basic block 0

	.global LoadedDynamicLibraryLoadSegments
	.global LoadedDynamicLibraryDelete
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
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	ld          t0, -24(s0)
	snez        t1, t0
	beqz        t0, .LoadSegments_label_76

	// *** Basic block 1

	ld          t0, -32(s0)
	addi        t0, t0, 120
	ld          t0, 0(t0)
	addi        t0, t0, 16
	lhu         t0, 0(t0)
	addi        t1, t0, -2
	seqz        t1, t1
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .LoadSegments_label_42

	// *** Basic block 2

	ld          a0, -32(s0)
	ld          a2, -40(s0)
	mv          a1, x0
	call        LoadedDynamicLibraryLoadSegments

	// *** Basic block 3

	j           .LoadSegments_label_74

	// *** Basic block 4

.LoadSegments_label_42:
	ld          t0, -32(s0)
	addi        t0, t0, 120
	ld          t0, 0(t0)
	addi        t0, t0, 16
	lhu         t0, 0(t0)
	addi        t1, t0, -3
	seqz        t1, t1
	li          t1, 3		// 0x3 ASCII \x3
	bne         t0, t1, .LoadSegments_label_64

	// *** Basic block 5

	ld          t0, -32(s0)
	addi        s1, t0, 112
	ld          a0, -32(s0)
	ld          a1, -24(s0)
	ld          a2, -40(s0)
	call        LoadedDynamicLibraryLoadSegments

	// *** Basic block 6

	sd          a0, 0(s1)
	j           .LoadSegments_label_73

	// *** Basic block 7

.LoadSegments_label_64:
	ld          a0, -32(s0)
	call        LoadedDynamicLibraryDelete

	// *** Basic block 8

	mv          a0, x0

	// *** Basic block 9

.LoadSegments_label_70:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 10

.LoadSegments_label_73:

	// *** Basic block 11

.LoadSegments_label_74:
	j           .LoadSegments_label_83

	// *** Basic block 12

.LoadSegments_label_76:
	ld          t0, -32(s0)
	addi        t0, t0, 112
	ld          t1, -32(s0)
	addi        t1, t1, 88
	ld          t1, 0(t1)
	sd          t1, 0(t0)

	// *** Basic block 13

.LoadSegments_label_83:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .LoadSegments_label_70
.func_end_LoadSegments:
	.size LoadSegments, .func_end_LoadSegments-LoadSegments

	.local  FindDynamicSection
	.type FindDynamicSection, @function

FindDynamicSection:

	// *** Basic block 0

	.local RelocateDynamicSection
	.global DynamicLoaderFindDynamicSectionAddressEntry
	.global LoaderError
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
	addi        t0, t0, 144
	ld          t0, 0(t0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .FindDynamicSection_label_151

	// *** Basic block 1

	sw          x0, -40(s0)
	lw          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 120
	ld          t1, 0(t1)
	addi        t1, t1, 56
	lhu         t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindDynamicSection_label_150

	// *** Basic block 2

.FindDynamicSection_label_42:
	ld          t0, -24(s0)
	addi        t0, t0, 128
	ld          t0, 0(t0)
	lw          t1, -40(s0)
	slli        t2, t1, 3
	slli        t3, t1, 4
	add         t2, t2, t3
	slli        t1, t1, 5
	add         t1, t2, t1
	add         t0, t0, t1
	addi        t0, t0, 0
	lwu         t0, 0(t0)
	addi        t1, t0, -2
	seqz        t1, t1
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .FindDynamicSection_label_136

	// *** Basic block 3

	ld          t0, -32(s0)
	seqz        t1, t0
	bnez        t0, .FindDynamicSection_label_86

	// *** Basic block 4

	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t1, -24(s0)
	addi        t1, t1, 112
	ld          t1, 0(t1)
	ld          t2, -24(s0)
	addi        t2, t2, 128
	ld          t2, 0(t2)
	lw          t3, -40(s0)
	slli        t4, t3, 3
	slli        t5, t3, 4
	add         t4, t4, t5
	slli        t3, t3, 5
	add         t3, t4, t3
	add         t2, t2, t3
	addi        t2, t2, 8
	ld          t2, 0(t2)
	add         t1, t1, t2
	sd          t1, 0(t0)
	j           .FindDynamicSection_label_134

	// *** Basic block 5

.FindDynamicSection_label_86:
	ld          t0, -24(s0)
	addi        t0, t0, 120
	ld          t0, 0(t0)
	addi        t0, t0, 16
	lhu         t0, 0(t0)
	addi        t1, t0, -2
	seqz        t1, t1
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .FindDynamicSection_label_112

	// *** Basic block 6

	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t1, -24(s0)
	addi        t1, t1, 128
	ld          t1, 0(t1)
	lw          t2, -40(s0)
	slli        t3, t2, 3
	slli        t4, t2, 4
	add         t3, t3, t4
	slli        t2, t2, 5
	add         t2, t3, t2
	add         t1, t1, t2
	addi        t1, t1, 16
	ld          t1, 0(t1)
	sd          t1, 0(t0)
	j           .FindDynamicSection_label_133

	// *** Basic block 7

.FindDynamicSection_label_112:
	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t1, -24(s0)
	addi        t1, t1, 144
	ld          t1, 0(t1)
	ld          t2, -24(s0)
	addi        t2, t2, 128
	ld          t2, 0(t2)
	lw          t3, -40(s0)
	slli        t4, t3, 3
	slli        t5, t3, 4
	add         t4, t4, t5
	slli        t3, t3, 5
	add         t3, t4, t3
	add         t2, t2, t3
	addi        t2, t2, 8
	ld          t2, 0(t2)
	add         t1, t1, t2
	sd          t1, 0(t0)

	// *** Basic block 8

.FindDynamicSection_label_133:

	// *** Basic block 9

.FindDynamicSection_label_134:
	j           .FindDynamicSection_label_150

	// *** Basic block 10

.FindDynamicSection_label_136:

	// *** Basic block 11

.FindDynamicSection_label_137:
	lw          t0, -40(s0)
	addi        t0, t0, 1
	sw          t0, -40(s0)
	lw          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 120
	ld          t1, 0(t1)
	addi        t1, t1, 56
	lhu         t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindDynamicSection_label_42

	// *** Basic block 12

.FindDynamicSection_label_150:

	// *** Basic block 13

.FindDynamicSection_label_151:
	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t0, 0(t0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .FindDynamicSection_label_163

	// *** Basic block 14

	mv          a0, x0

	// *** Basic block 15

.FindDynamicSection_label_160:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 16

.FindDynamicSection_label_163:
	ld          t0, -32(s0)
	snez        t1, t0
	beqz        t0, .FindDynamicSection_label_171

	// *** Basic block 17

	ld          a0, -24(s0)
	call        RelocateDynamicSection

	// *** Basic block 18

.FindDynamicSection_label_171:
	ld          t0, -24(s0)
	addi        s1, t0, 184
	ld          a0, -24(s0)
	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	call        DynamicLoaderFindDynamicSectionAddressEntry

	// *** Basic block 19

	sd          a0, 0(s1)
	ld          t0, -24(s0)
	addi        s1, t0, 192
	ld          a0, -24(s0)
	li          t0, 5		// 0x5 ASCII \x5
	mv          a1, t0
	call        DynamicLoaderFindDynamicSectionAddressEntry

	// *** Basic block 20

	sd          a0, 0(s1)
	ld          t1, -24(s0)
	addi        t1, t1, 184
	ld          t1, 0(t1)
	sub         t2, t1, x0
	seqz        t0, t2
	beq         t1, x0, .FindDynamicSection_label_203

	// *** Basic block 21

	ld          t1, -24(s0)
	addi        t1, t1, 192
	ld          t1, 0(t1)
	sub         t1, t1, x0
	seqz        t0, t1

	// *** Basic block 22

.FindDynamicSection_label_203:
	beqz        t0, .FindDynamicSection_label_217

	// *** Basic block 23

	lla         a0, .str.4
	ld          t0, -24(s0)
	addi        t0, t0, 40
	addi        t0, t0, 16
	ld          a1, 0(t0)
	call        LoaderError

	// *** Basic block 24

	mv          a0, x0
	j           .FindDynamicSection_label_160

	// *** Basic block 25

.FindDynamicSection_label_217:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .FindDynamicSection_label_160
.func_end_FindDynamicSection:
	.size FindDynamicSection, .func_end_FindDynamicSection-FindDynamicSection

	.local  FindHashTable
	.type FindHashTable, @function

FindHashTable:

	// *** Basic block 0

	.global DynamicLoaderFindDynamicSectionAddressEntry
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -40(s0)
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        s1, t0, 152
	ld          a0, -24(s0)
	li          a1, 1879047925		// 0x6ffffef5
	call        DynamicLoaderFindDynamicSectionAddressEntry

	// *** Basic block 1

	sd          a0, 0(s1)
	ld          t0, -24(s0)
	addi        t0, t0, 152
	ld          t0, 0(t0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .FindHashTable_label_89

	// *** Basic block 2

	sw          x0, -40(s0)
	lw          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 120
	ld          t1, 0(t1)
	addi        t1, t1, 60
	lhu         t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindHashTable_label_88

	// *** Basic block 3

.FindHashTable_label_45:
	ld          t0, -24(s0)
	addi        t0, t0, 136
	ld          t0, 0(t0)
	lw          t1, -40(s0)
	slli        t1, t1, 6
	add         t0, t0, t1
	sd          t0, -32(s0)
	ld          t0, -32(s0)
	addi        t0, t0, 4
	lwu         t0, 0(t0)
	addi        t1, t0, -11
	seqz        t1, t1
	li          t1, 11		// 0xb ASCII \xb
	bne         t0, t1, .FindHashTable_label_74

	// *** Basic block 4

	ld          t0, -24(s0)
	addi        t0, t0, 200
	ld          t1, -32(s0)
	addi        t1, t1, 32
	ld          t1, 0(t1)
	ld          t2, -32(s0)
	addi        t2, t2, 56
	ld          t2, 0(t2)
	div         t1, t1, t2
	sd          t1, 0(t0)
	j           .FindHashTable_label_88

	// *** Basic block 5

.FindHashTable_label_74:

	// *** Basic block 6

.FindHashTable_label_75:
	lw          t0, -40(s0)
	addi        t0, t0, 1
	sw          t0, -40(s0)
	lw          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 120
	ld          t1, 0(t1)
	addi        t1, t1, 60
	lhu         t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindHashTable_label_45

	// *** Basic block 7

.FindHashTable_label_88:

	// *** Basic block 8

.FindHashTable_label_89:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_FindHashTable:
	.size FindHashTable, .func_end_FindHashTable-FindHashTable

	.global ExpandRuntimePath
	.type ExpandRuntimePath, @function

ExpandRuntimePath:

	// *** Basic block 0

	.global StringAppendChar
	.global StringEqual
	.global StringSetString
	.global StringSet
	.global StringAppendString
	.global StringDestruct
	addi sp, sp, -160
	// Saved return address (offset 152) and frame pointer (offset 144)
	sd ra, 152(sp)
	sd s0, 144(sp)
	addi s0, sp, 160
	// Saved argument registers.
	sd a1, -24(s0)
	sd a0, -32(s0)
	// Local vars at offset -120(s0)
	// Saved integer registers.
	sd s1, 32(sp)
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	sd s5, 0(sp)
	// End of stack frame
	sd          x0, -120(s0)
	sd          x0, -112(s0)
	sd          x0, -104(s0)
	sd          x0, -96(s0)
	sd          x0, -88(s0)
	sb          x0, -120(s0)
	sd          x0, -80(s0)
	ld          t0, -80(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 24
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .ExpandRuntimePath_label_289

	// *** Basic block 1

.ExpandRuntimePath_label_52:
	sd          x0, -72(s0)
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sb          x0, -72(s0)
	ld          t1, -80(s0)
	ld          t2, -24(s0)
	addi        t2, t2, 24
	ld          t2, 0(t2)
	slt         t0, t1, t2
	bge         t1, t2, .ExpandRuntimePath_label_81

	// *** Basic block 2

	ld          t1, -24(s0)
	addi        t1, t1, 16
	ld          t1, 0(t1)
	ld          t2, -80(s0)
	add         t1, t1, t2
	lb          t1, 0(t1)
	addi        t1, t1, -47
	snez        t0, t1

	// *** Basic block 3

.ExpandRuntimePath_label_81:
	beqz        t0, .ExpandRuntimePath_label_119

	// *** Basic block 4

.ExpandRuntimePath_label_83:
	addi        a0, s0, -72
	ld          t0, -24(s0)
	addi        t0, t0, 16
	ld          t0, 0(t0)
	ld          t1, -80(s0)
	add         t0, t0, t1
	lb          a1, 0(t0)
	call        StringAppendChar

	// *** Basic block 5

	ld          t0, -80(s0)
	addi        t0, t0, 1
	sd          t0, -80(s0)
	ld          t1, -80(s0)
	ld          t2, -24(s0)
	addi        t2, t2, 24
	ld          t2, 0(t2)
	slt         t0, t1, t2
	bge         t1, t2, .ExpandRuntimePath_label_117

	// *** Basic block 6

	ld          t1, -24(s0)
	addi        t1, t1, 16
	ld          t1, 0(t1)
	ld          t2, -80(s0)
	add         t1, t1, t2
	lb          t1, 0(t1)
	addi        t1, t1, -47
	snez        t0, t1

	// *** Basic block 7

.ExpandRuntimePath_label_117:
	bnez        t0, .ExpandRuntimePath_label_83

	// *** Basic block 8

.ExpandRuntimePath_label_119:
	addi        t1, s0, -72
	addi        t1, t1, 24
	ld          t1, 0(t1)
	slt         t0, x0, t1
	bge         x0, t1, .ExpandRuntimePath_label_134

	// *** Basic block 9

	addi        t1, s0, -72
	addi        t1, t1, 16
	ld          t1, 0(t1)
	addi        s1, t1, 0
	lb          t1, 0(s1)
	addi        t1, t1, -36
	seqz        t0, t1

	// *** Basic block 10

.ExpandRuntimePath_label_134:
	beqz        t0, .ExpandRuntimePath_label_252

	// *** Basic block 11

	addi        a0, s0, -72
	lla         a1, .str.5
	call        StringEqual

	// *** Basic block 12

	mv          s1, a0
	bnez        a0, .ExpandRuntimePath_label_152

	// *** Basic block 13

	addi        a0, s0, -72
	lla         a1, .str.6
	call        StringEqual

	// *** Basic block 14

	mv          s1, a0

	// *** Basic block 15

.ExpandRuntimePath_label_152:
	beqz        s1, .ExpandRuntimePath_label_164

	// *** Basic block 16

	addi        a0, s0, -72
	ld          t0, -32(s0)
	addi        t0, t0, 80
	ld          t0, 0(t0)
	addi        a1, t0, 104
	call        StringSetString

	// *** Basic block 17

	j           .ExpandRuntimePath_label_251

	// *** Basic block 18

.ExpandRuntimePath_label_164:
	addi        a0, s0, -72
	lla         a1, .str.7
	call        StringEqual

	// *** Basic block 19

	mv          s2, a0
	bnez        a0, .ExpandRuntimePath_label_181

	// *** Basic block 20

	addi        a0, s0, -72
	lla         a1, .str.8
	call        StringEqual

	// *** Basic block 21

	mv          s2, a0

	// *** Basic block 22

.ExpandRuntimePath_label_181:
	beqz        s2, .ExpandRuntimePath_label_192

	// *** Basic block 23

	addi        a0, s0, -72
	ld          t0, -32(s0)
	addi        t0, t0, 80
	ld          t0, 0(t0)
	addi        a1, t0, 64
	call        StringSetString

	// *** Basic block 24

	j           .ExpandRuntimePath_label_250

	// *** Basic block 25

.ExpandRuntimePath_label_192:
	addi        a0, s0, -72
	lla         a1, .str.9
	call        StringEqual

	// *** Basic block 26

	mv          s3, a0
	bnez        a0, .ExpandRuntimePath_label_209

	// *** Basic block 27

	addi        a0, s0, -72
	lla         a1, .str.10
	call        StringEqual

	// *** Basic block 28

	mv          s3, a0

	// *** Basic block 29

.ExpandRuntimePath_label_209:
	beqz        s3, .ExpandRuntimePath_label_218

	// *** Basic block 30

	addi        a0, s0, -72
	lla         a1, .str.11
	call        StringSet

	// *** Basic block 31

	j           .ExpandRuntimePath_label_249

	// *** Basic block 32

.ExpandRuntimePath_label_218:
	addi        a0, s0, -72
	lla         a1, .str.12
	call        StringEqual

	// *** Basic block 33

	mv          s4, a0
	bnez        a0, .ExpandRuntimePath_label_235

	// *** Basic block 34

	addi        a0, s0, -72
	lla         a1, .str.13
	call        StringEqual

	// *** Basic block 35

	mv          s4, a0

	// *** Basic block 36

.ExpandRuntimePath_label_235:
	beqz        s4, .ExpandRuntimePath_label_248

	// *** Basic block 37

	addi        a0, s0, -72
	ld          t0, -32(s0)
	addi        t0, t0, 80
	ld          t0, 0(t0)
	addi        s5, t0, 48
	ld          t0, 0(s5)
	addi        t0, t0, 8
	ld          a1, 0(t0)
	call        StringSet

	// *** Basic block 38

.ExpandRuntimePath_label_248:

	// *** Basic block 39

.ExpandRuntimePath_label_249:

	// *** Basic block 40

.ExpandRuntimePath_label_250:

	// *** Basic block 41

.ExpandRuntimePath_label_251:

	// *** Basic block 42

.ExpandRuntimePath_label_252:
	addi        a0, s0, -120
	addi        a1, s0, -72
	call        StringAppendString

	// *** Basic block 43

	addi        a0, s0, -72
	call        StringDestruct

	// *** Basic block 44

	ld          t0, -24(s0)
	addi        t0, t0, 16
	ld          t0, 0(t0)
	ld          t1, -80(s0)
	add         t0, t0, t1
	lb          t0, 0(t0)
	addi        t1, t0, -47
	seqz        t1, t1
	li          t1, 47		// 0x2f ASCII '/'
	bne         t0, t1, .ExpandRuntimePath_label_282

	// *** Basic block 45

	addi        a0, s0, -120
	li          t0, 47		// 0x2f ASCII '/'
	mv          a1, t0
	call        StringAppendChar

	// *** Basic block 46

	ld          t0, -80(s0)
	addi        t0, t0, 1
	sd          t0, -80(s0)

	// *** Basic block 47

.ExpandRuntimePath_label_282:
	ld          t0, -80(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 24
	ld          t1, 0(t1)
	slt         t2, t0, t1
	blt         t0, t1, .ExpandRuntimePath_label_52

	// *** Basic block 48

.ExpandRuntimePath_label_289:
	ld          a0, -24(s0)
	addi        a1, s0, -120
	call        StringSetString

	// *** Basic block 49

	addi        a0, s0, -120
	call        StringDestruct

	// *** Basic block 50

	// Restored registers.
	ld s1, 32(sp)
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ExpandRuntimePath:
	.size ExpandRuntimePath, .func_end_ExpandRuntimePath-ExpandRuntimePath

	.local  BuildRuntimePaths
	.type BuildRuntimePaths, @function

BuildRuntimePaths:

	// *** Basic block 0

	.global DynamicLoaderFindDynamicSectionOffsetEntry
	.global NewString
	.global StringAppendChar
	.global ExpandRuntimePath
	.global VectorAppend
	.global StringDelete
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -56(s0)
	// End of stack frame
	ld          a0, -24(s0)
	li          a1, 29		// 0x1d ASCII \x1d
	call        DynamicLoaderFindDynamicSectionOffsetEntry

	// *** Basic block 1

	sd          a0, -56(s0)
	ld          t0, -56(s0)
	addi        t1, t0, 1
	seqz        t1, t1
	li          t1, -1		// 0xffffffffffffffff
	bne         t0, t1, .BuildRuntimePaths_label_37

	// *** Basic block 2

.BuildRuntimePaths_label_34:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.BuildRuntimePaths_label_37:
	ld          t0, -24(s0)
	addi        t0, t0, 192
	ld          t0, 0(t0)
	ld          t1, -56(s0)
	add         t0, t0, t1
	sd          t0, -48(s0)
	ld          t0, -48(s0)
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	lb          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .BuildRuntimePaths_label_131

	// *** Basic block 4

.BuildRuntimePaths_label_54:
	lla         a0, .str.14
	call        NewString

	// *** Basic block 5

	sd          a0, -32(s0)
	ld          t1, -40(s0)
	lb          t1, 0(t1)
	snez        t0, t1
	beqz        t1, .BuildRuntimePaths_label_71

	// *** Basic block 6

	ld          t1, -40(s0)
	lb          t1, 0(t1)
	addi        t1, t1, -58
	snez        t0, t1

	// *** Basic block 7

.BuildRuntimePaths_label_71:
	beqz        t0, .BuildRuntimePaths_label_94

	// *** Basic block 8

.BuildRuntimePaths_label_73:
	ld          a0, -32(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	lb          a1, 0(t0)
	call        StringAppendChar

	// *** Basic block 9

	ld          t1, -40(s0)
	lb          t1, 0(t1)
	snez        t0, t1
	beqz        t1, .BuildRuntimePaths_label_92

	// *** Basic block 10

	ld          t1, -40(s0)
	lb          t1, 0(t1)
	addi        t1, t1, -58
	snez        t0, t1

	// *** Basic block 11

.BuildRuntimePaths_label_92:
	bnez        t0, .BuildRuntimePaths_label_73

	// *** Basic block 12

.BuildRuntimePaths_label_94:
	ld          a0, -24(s0)
	ld          a1, -32(s0)
	call        ExpandRuntimePath

	// *** Basic block 13

	ld          t0, -32(s0)
	addi        t0, t0, 24
	ld          t0, 0(t0)
	slt         t1, x0, t0
	bge         x0, t0, .BuildRuntimePaths_label_112

	// *** Basic block 14

	ld          t0, -24(s0)
	addi        a0, t0, 160
	ld          a1, -32(s0)
	call        VectorAppend

	// *** Basic block 15

	j           .BuildRuntimePaths_label_116

	// *** Basic block 16

.BuildRuntimePaths_label_112:
	ld          a0, -32(s0)
	call        StringDelete

	// *** Basic block 17

.BuildRuntimePaths_label_116:
	ld          t0, -40(s0)
	lb          t0, 0(t0)
	seqz        t1, t0
	bnez        t0, .BuildRuntimePaths_label_122

	// *** Basic block 18

	j           .BuildRuntimePaths_label_131

	// *** Basic block 19

.BuildRuntimePaths_label_122:
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	lb          t0, 0(t0)
	snez        t1, t0
	bnez        t0, .BuildRuntimePaths_label_54

	// *** Basic block 20

.BuildRuntimePaths_label_131:
	j           .BuildRuntimePaths_label_34
.func_end_BuildRuntimePaths:
	.size BuildRuntimePaths, .func_end_BuildRuntimePaths-BuildRuntimePaths

	.global LoadedDynamicLibraryLoad
	.type LoadedDynamicLibraryLoad, @function

LoadedDynamicLibraryLoad:

	// *** Basic block 0

	.local FindDynamicLibraryFile
	.local MapWholeDynamicLibrary
	.local MapDynamicLibraryHeader
	.local LoadSegments
	.global print_libraries_only
	.global printf
	.local FindDynamicSection
	.local FindHashTable
	.local BuildRuntimePaths
	.local FindAndLoadSymbolTable
	.local NextAddress
	.local LoadNeededLibraries
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Saved argument registers.
	sd a2, -24(s0)
	sd a0, -32(s0)
	sd a3, -40(s0)
	sd a4, -48(s0)
	sd a1, -56(s0)
	// Local vars at offset -80(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	ld          a0, -24(s0)
	ld          t0, -32(s0)
	addi        a1, t0, 160
	ld          t0, -32(s0)
	addi        t0, t0, 0
	addi        t0, t0, 16
	ld          a2, 0(t0)
	ld          t0, -32(s0)
	addi        a3, t0, 40
	call        FindDynamicLibraryFile

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .LoadedDynamicLibraryLoad_label_60

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.LoadedDynamicLibraryLoad_label_57:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.LoadedDynamicLibraryLoad_label_60:
	ld          t0, -40(s0)
	seqz        t1, t0
	bnez        t0, .LoadedDynamicLibraryLoad_label_79

	// *** Basic block 5

	ld          t0, -32(s0)
	addi        s1, t0, 104
	ld          t0, -32(s0)
	addi        a0, t0, 40
	addi        a1, s0, -80
	addi        a2, s0, -72
	call        MapWholeDynamicLibrary

	// *** Basic block 6

	sw          a0, 0(s1)
	j           .LoadedDynamicLibraryLoad_label_91

	// *** Basic block 7

.LoadedDynamicLibraryLoad_label_79:
	ld          t0, -32(s0)
	addi        s1, t0, 104
	ld          t0, -32(s0)
	addi        a0, t0, 40
	addi        a1, s0, -80
	addi        a2, s0, -72
	call        MapDynamicLibraryHeader

	// *** Basic block 8

	sw          a0, 0(s1)

	// *** Basic block 9

.LoadedDynamicLibraryLoad_label_91:
	ld          t0, -32(s0)
	addi        t0, t0, 104
	lw          t0, 0(t0)
	sltz        t1, t0
	bge         t0, x0, .LoadedDynamicLibraryLoad_label_100

	// *** Basic block 10

	mv          a0, x0
	j           .LoadedDynamicLibraryLoad_label_57

	// *** Basic block 11

.LoadedDynamicLibraryLoad_label_100:
	ld          t0, -32(s0)
	addi        t0, t0, 88
	ld          t1, -80(s0)
	sd          t1, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 96
	ld          t1, -72(s0)
	sd          t1, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 120
	ld          t1, -32(s0)
	addi        t1, t1, 88
	ld          t1, 0(t1)
	sd          t1, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 128
	ld          t1, -32(s0)
	addi        t1, t1, 88
	ld          t1, 0(t1)
	ld          t2, -32(s0)
	addi        t2, t2, 120
	ld          t2, 0(t2)
	addi        t2, t2, 32
	ld          t2, 0(t2)
	add         t1, t1, x0
	sd          t1, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 136
	ld          t1, -32(s0)
	addi        t1, t1, 88
	ld          t1, 0(t1)
	ld          t2, -32(s0)
	addi        t2, t2, 120
	ld          t2, 0(t2)
	addi        t2, t2, 40
	ld          t2, 0(t2)
	add         t1, t1, x0
	sd          t1, 0(t0)
	ld          a0, -32(s0)
	ld          a1, -40(s0)
	ld          a2, -48(s0)
	call        LoadSegments

	// *** Basic block 12

	not         t0, a0
	beqz        t0, .LoadedDynamicLibraryLoad_label_152

	// *** Basic block 13

	mv          a0, x0
	j           .LoadedDynamicLibraryLoad_label_57

	// *** Basic block 14

.LoadedDynamicLibraryLoad_label_152:
	la          t0, print_libraries_only
	lb          t0, 0(t0)
	beqz        t0, .LoadedDynamicLibraryLoad_label_181

	// *** Basic block 15

	ld          t0, -32(s0)
	addi        t0, t0, 112
	ld          t0, 0(t0)
	sd          t0, -64(s0)
	ld          t0, -64(s0)
	snez        t1, t0
	beqz        t0, .LoadedDynamicLibraryLoad_label_180

	// *** Basic block 16

	lla         a0, .str.15
	ld          t0, -32(s0)
	addi        t0, t0, 0
	addi        t0, t0, 16
	ld          a1, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 40
	addi        t0, t0, 16
	ld          a2, 0(t0)
	ld          a3, -64(s0)
	call        printf

	// *** Basic block 17

.LoadedDynamicLibraryLoad_label_180:

	// *** Basic block 18

.LoadedDynamicLibraryLoad_label_181:
	ld          a0, -32(s0)
	ld          a1, -40(s0)
	call        FindDynamicSection

	// *** Basic block 19

	not         t0, a0
	beqz        t0, .LoadedDynamicLibraryLoad_label_192

	// *** Basic block 20

	mv          a0, x0
	j           .LoadedDynamicLibraryLoad_label_57

	// *** Basic block 21

.LoadedDynamicLibraryLoad_label_192:
	ld          a0, -32(s0)
	call        FindHashTable

	// *** Basic block 22

	ld          t0, -32(s0)
	addi        t0, t0, 80
	ld          t0, 0(t0)
	sub         t1, t0, x0
	snez        t1, t1
	beq         t0, x0, .LoadedDynamicLibraryLoad_label_205

	// *** Basic block 23

	ld          a0, -32(s0)
	call        BuildRuntimePaths

	// *** Basic block 24

.LoadedDynamicLibraryLoad_label_205:
	ld          t0, -32(s0)
	addi        t0, t0, 208
	lb          t0, 0(t0)
	beqz        t0, .LoadedDynamicLibraryLoad_label_213

	// *** Basic block 25

	ld          a0, -32(s0)
	call        FindAndLoadSymbolTable

	// *** Basic block 26

.LoadedDynamicLibraryLoad_label_213:
	ld          a0, -32(s0)
	ld          a1, -40(s0)
	ld          a2, -48(s0)
	call        NextAddress

	// *** Basic block 27

	sd          a0, -40(s0)
	ld          a0, -56(s0)
	ld          a1, -32(s0)
	ld          a2, -24(s0)
	ld          a3, -40(s0)
	ld          a4, -48(s0)
	call        LoadNeededLibraries

	// *** Basic block 28

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .LoadedDynamicLibraryLoad_label_57
.func_end_LoadedDynamicLibraryLoad:
	.size LoadedDynamicLibraryLoad, .func_end_LoadedDynamicLibraryLoad-LoadedDynamicLibraryLoad

	.global NewLoadedDynamicLibrary
	.type NewLoadedDynamicLibrary, @function

NewLoadedDynamicLibrary:

	// *** Basic block 0

	.global malloc
	.global StringInit
	.global VectorInit
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
	li          a0, 272		// 0x110
	call        malloc

	// *** Basic block 1

	sd          a0, -40(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 80
	ld          t1, -24(s0)
	sd          t1, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 104
	sw          x0, 0(t0)
	ld          t0, -40(s0)
	addi        a0, t0, 0
	ld          a1, -32(s0)
	call        StringInit

	// *** Basic block 2

	ld          t0, -40(s0)
	addi        a0, t0, 40
	mv          a1, x0
	call        StringInit

	// *** Basic block 3

	ld          t0, -40(s0)
	addi        t0, t0, 88
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 96
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 200
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 120
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 144
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 192
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 184
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 152
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 128
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 136
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 112
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        a0, t0, 248
	call        VectorInit

	// *** Basic block 4

	ld          t0, -40(s0)
	addi        a0, t0, 160
	call        VectorInit

	// *** Basic block 5

	ld          t0, -40(s0)
	addi        t0, t0, 208
	ld          t2, -24(s0)
	sub         t3, t2, x0
	seqz        t3, t3
	bne         t2, x0, .NewLoadedDynamicLibrary_label_111

	// *** Basic block 6

	mv          t1, x0
	j           .NewLoadedDynamicLibrary_label_117

	// *** Basic block 7

.NewLoadedDynamicLibrary_label_111:
	ld          t2, -24(s0)
	addi        t2, t2, 40
	lw          t2, 0(t2)
	andi        t2, t2, 1
	snez        t1, t2

	// *** Basic block 8

.NewLoadedDynamicLibrary_label_117:
	slli        t2, t1, 56
	srai        t2, t2, 56
	sb          t2, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 216
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 224
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 232
	sd          x0, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 240
	sb          x0, 0(t0)
	ld          t0, -40(s0)
	mv          a0, t0

	// *** Basic block 9

.NewLoadedDynamicLibrary_label_136:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewLoadedDynamicLibrary:
	.size NewLoadedDynamicLibrary, .func_end_NewLoadedDynamicLibrary-NewLoadedDynamicLibrary

	.global LoadedDynamicLibraryDestruct
	.type LoadedDynamicLibraryDestruct, @function

LoadedDynamicLibraryDestruct:

	// *** Basic block 0

	.global munmap
	.global close
	.global VectorDestructWithContents
	.local MappedSegmentDestruct
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
	addi        t0, t0, 96
	ld          t0, 0(t0)
	slt         t1, x0, t0
	bge         x0, t0, .LoadedDynamicLibraryDestruct_label_32

	// *** Basic block 1

	ld          t0, -24(s0)
	addi        t0, t0, 88
	ld          a0, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 96
	ld          a1, 0(t0)
	call        munmap

	// *** Basic block 2

.LoadedDynamicLibraryDestruct_label_32:
	ld          t0, -24(s0)
	addi        t0, t0, 104
	lw          a0, 0(t0)
	call        close

	// *** Basic block 3

	ld          t0, -24(s0)
	addi        a0, t0, 248
	la          t0, MappedSegmentDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 4

	ld          t0, -24(s0)
	addi        a0, t0, 160
	la          t0, StringDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 5

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LoadedDynamicLibraryDestruct:
	.size LoadedDynamicLibraryDestruct, .func_end_LoadedDynamicLibraryDestruct-LoadedDynamicLibraryDestruct

	.global LoadedDynamicLibraryDelete
	.type LoadedDynamicLibraryDelete, @function

LoadedDynamicLibraryDelete:

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
	.global LoadedDynamicLibraryDestruct
	.global free
	ld          a0, -24(s0)
	call        LoadedDynamicLibraryDestruct

	// *** Basic block 1

	ld          a0, -24(s0)
	call        free

	// *** Basic block 2

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LoadedDynamicLibraryDelete:
	.size LoadedDynamicLibraryDelete, .func_end_LoadedDynamicLibraryDelete-LoadedDynamicLibraryDelete

	.local  FindSymbolSlowly
	.type FindSymbolSlowly, @function

FindSymbolSlowly:

	// *** Basic block 0

	.global strcmp
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
	sd          x0, -56(s0)
	ld          t0, -56(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 200
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindSymbolSlowly_label_82

	// *** Basic block 1

.FindSymbolSlowly_label_24:
	ld          t0, -24(s0)
	addi        t0, t0, 184
	ld          t0, 0(t0)
	ld          t1, -56(s0)
	slli        t2, t1, 3
	slli        t1, t1, 4
	add         t1, t2, t1
	add         t0, t0, t1
	sd          t0, -48(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 192
	ld          t0, 0(t0)
	ld          t1, -48(s0)
	addi        t1, t1, 0
	lwu         t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -40(s0)
	ld          t0, -48(s0)
	addi        t0, t0, 6
	lhu         t0, 0(t0)
	seqz        t1, t0
	bnez        t0, .FindSymbolSlowly_label_53

	// *** Basic block 2

	j           .FindSymbolSlowly_label_71

	// *** Basic block 3

.FindSymbolSlowly_label_53:
	ld          a0, -40(s0)
	ld          a1, -32(s0)
	call        strcmp

	// *** Basic block 4

	seqz        t0, a0
	bnez        a0, .FindSymbolSlowly_label_70

	// *** Basic block 5

	ld          t0, -48(s0)
	mv          a0, t0

	// *** Basic block 6

.FindSymbolSlowly_label_67:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.FindSymbolSlowly_label_70:

	// *** Basic block 8

.FindSymbolSlowly_label_71:
	ld          t0, -56(s0)
	addi        t0, t0, 1
	sd          t0, -56(s0)
	ld          t0, -56(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 200
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindSymbolSlowly_label_24

	// *** Basic block 9

.FindSymbolSlowly_label_82:
	mv          a0, x0
	j           .FindSymbolSlowly_label_67
.func_end_FindSymbolSlowly:
	.size FindSymbolSlowly, .func_end_FindSymbolSlowly-FindSymbolSlowly

	.global LoadedDynamicLibraryFindSymbol
	.type LoadedDynamicLibraryFindSymbol, @function

LoadedDynamicLibraryFindSymbol:

	// *** Basic block 0

	.local FindSymbolSlowly
	.global DynamicLoaderGNUHash
	.global DynamicLoaderBloomBits64
	.global strcmp
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -120(s0)
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 152
	ld          t0, 0(t0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .LoadedDynamicLibraryFindSymbol_label_38

	// *** Basic block 1

	ld          a0, -24(s0)
	ld          a1, -32(s0)
	call        FindSymbolSlowly

	// *** Basic block 2


	// *** Basic block 3

.LoadedDynamicLibraryFindSymbol_label_35:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.LoadedDynamicLibraryFindSymbol_label_38:
	ld          a0, -32(s0)
	call        DynamicLoaderGNUHash

	// *** Basic block 5

	sw          a0, -120(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 152
	ld          t0, 0(t0)
	addi        t0, t0, 16
	sd          t0, -112(s0)
	lwu         t0, -120(s0)
	li          t1, 64		// 0x40 ASCII '@'
	div         t0, t0, t1
	ld          t1, -24(s0)
	addi        t1, t1, 152
	ld          t1, 0(t1)
	addi        t1, t1, 8
	lwu         t1, 0(t1)
	rem         t0, t0, t1
	sw          t0, -104(s0)
	lwu         a0, -120(s0)
	call        DynamicLoaderBloomBits64

	// *** Basic block 6

	sd          a0, -96(s0)
	ld          t0, -112(s0)
	lw          t1, -104(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	ld          t1, -96(s0)
	and         t0, t0, t1
	ld          t1, -96(s0)
	sub         t2, t0, t1
	snez        t2, t2
	beq         t0, t1, .LoadedDynamicLibraryFindSymbol_label_81

	// *** Basic block 7

	mv          a0, x0
	j           .LoadedDynamicLibraryFindSymbol_label_35

	// *** Basic block 8

.LoadedDynamicLibraryFindSymbol_label_81:
	lwu         t0, -120(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 152
	ld          t1, 0(t1)
	addi        t1, t1, 0
	lwu         t1, 0(t1)
	rem         t0, t0, t1
	sw          t0, -88(s0)
	ld          t0, -112(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 152
	ld          t1, 0(t1)
	addi        t1, t1, 8
	lwu         t1, 0(t1)
	slli        t1, t1, 3
	add         t0, t0, t1
	sd          t0, -80(s0)
	ld          t0, -80(s0)
	lw          t1, -88(s0)
	slli        t1, t1, 2
	add         t0, t0, t1
	lwu         t0, 0(t0)
	sw          t0, -72(s0)
	lw          t0, -72(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 152
	ld          t1, 0(t1)
	addi        t1, t1, 4
	lwu         t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoadedDynamicLibraryFindSymbol_label_120

	// *** Basic block 9

	mv          a0, x0
	j           .LoadedDynamicLibraryFindSymbol_label_35

	// *** Basic block 10

.LoadedDynamicLibraryFindSymbol_label_120:
	ld          t0, -80(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 152
	ld          t1, 0(t1)
	addi        t1, t1, 0
	lwu         t1, 0(t1)
	slli        t1, t1, 2
	add         t0, t0, t1
	sd          t0, -64(s0)

	// *** Basic block 11

.LoadedDynamicLibraryFindSymbol_label_131:
	ld          t0, -24(s0)
	addi        t0, t0, 184
	ld          t0, 0(t0)
	lw          t1, -72(s0)
	slli        t2, t1, 3
	slli        t1, t1, 4
	add         t1, t2, t1
	add         t0, t0, t1
	sd          t0, -56(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 192
	ld          t0, 0(t0)
	ld          t1, -56(s0)
	addi        t1, t1, 0
	lwu         t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -48(s0)
	ld          t0, -64(s0)
	lw          t1, -72(s0)
	ld          t2, -24(s0)
	addi        t2, t2, 152
	ld          t2, 0(t2)
	addi        t2, t2, 4
	lwu         t2, 0(t2)
	sub         t1, t1, t2
	slli        t1, t1, 2
	add         t0, t0, t1
	lwu         t0, 0(t0)
	sw          t0, -40(s0)
	lwu         t0, -40(s0)
	ori         t0, t0, 1
	lwu         t1, -120(s0)
	ori         t1, t1, 1
	sub         t2, t0, t1
	seqz        s1, t2
	bne         t0, t1, .LoadedDynamicLibraryFindSymbol_label_179

	// *** Basic block 12

	ld          a0, -32(s0)
	ld          a1, -48(s0)
	call        strcmp

	// *** Basic block 13

	seqz        s1, a0

	// *** Basic block 14

.LoadedDynamicLibraryFindSymbol_label_179:
	beqz        s1, .LoadedDynamicLibraryFindSymbol_label_185

	// *** Basic block 15

	ld          t0, -56(s0)
	mv          a0, t0
	j           .LoadedDynamicLibraryFindSymbol_label_35

	// *** Basic block 16

.LoadedDynamicLibraryFindSymbol_label_185:
	lwu         t0, -40(s0)
	andi        t0, t0, 1
	snez        t1, t0
	beqz        t0, .LoadedDynamicLibraryFindSymbol_label_194

	// *** Basic block 17

.LoadedDynamicLibraryFindSymbol_label_190:
	mv          a0, x0
	j           .LoadedDynamicLibraryFindSymbol_label_35

	// *** Basic block 18

.LoadedDynamicLibraryFindSymbol_label_194:
	lw          t0, -72(s0)
	addi        t0, t0, 1
	sw          t0, -72(s0)

	// *** Basic block 19

.LoadedDynamicLibraryFindSymbol_label_199:
	j           .LoadedDynamicLibraryFindSymbol_label_131
.func_end_LoadedDynamicLibraryFindSymbol:
	.size LoadedDynamicLibraryFindSymbol, .func_end_LoadedDynamicLibraryFindSymbol-LoadedDynamicLibraryFindSymbol

	.global LoadedDynamicLibraryLookupSymbolByAddress
	.type LoadedDynamicLibraryLookupSymbolByAddress, @function

LoadedDynamicLibraryLookupSymbolByAddress:

	// *** Basic block 0

	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	sd a3, -40(s0)
	sd a4, -48(s0)
	sd a2, -56(s0)
	// Local vars at offset -120(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 216
	ld          t0, 0(t0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .LoadedDynamicLibraryLookupSymbolByAddress_label_28

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.LoadedDynamicLibraryLookupSymbolByAddress_label_25:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 3

.LoadedDynamicLibraryLookupSymbolByAddress_label_28:
	sb          x0, -120(s0)
	sd          x0, -112(s0)
	ld          t0, -112(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 248
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoadedDynamicLibraryLookupSymbolByAddress_label_90

	// *** Basic block 4

.LoadedDynamicLibraryLookupSymbolByAddress_label_40:
	ld          t0, -24(s0)
	addi        t0, t0, 248
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -112(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -104(s0)
	ld          t0, -104(s0)
	addi        t0, t0, 0
	ld          t0, 0(t0)
	sd          t0, -96(s0)
	ld          t1, -32(s0)
	ld          t2, -96(s0)
	slt         t3, t1, t2
	not         t0, t3
	blt         t1, t2, .LoadedDynamicLibraryLookupSymbolByAddress_label_72

	// *** Basic block 5

	ld          t1, -32(s0)
	ld          t2, -96(s0)
	ld          t3, -104(s0)
	addi        t3, t3, 8
	ld          t3, 0(t3)
	add         t2, t2, t3
	slt         t0, t1, t2

	// *** Basic block 6

.LoadedDynamicLibraryLookupSymbolByAddress_label_72:
	beqz        t0, .LoadedDynamicLibraryLookupSymbolByAddress_label_77

	// *** Basic block 7

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, -120(s0)
	j           .LoadedDynamicLibraryLookupSymbolByAddress_label_90

	// *** Basic block 8

.LoadedDynamicLibraryLookupSymbolByAddress_label_77:

	// *** Basic block 9

.LoadedDynamicLibraryLookupSymbolByAddress_label_78:
	ld          t0, -112(s0)
	addi        t0, t0, 1
	sd          t0, -112(s0)
	ld          t0, -112(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 248
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoadedDynamicLibraryLookupSymbolByAddress_label_40

	// *** Basic block 10

.LoadedDynamicLibraryLookupSymbolByAddress_label_90:
	lb          t0, -120(s0)
	not         t0, t0
	beqz        t0, .LoadedDynamicLibraryLookupSymbolByAddress_label_97

	// *** Basic block 11

	mv          a0, x0
	j           .LoadedDynamicLibraryLookupSymbolByAddress_label_25

	// *** Basic block 12

.LoadedDynamicLibraryLookupSymbolByAddress_label_97:
	sw          x0, -88(s0)
	lw          t0, -88(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 232
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoadedDynamicLibraryLookupSymbolByAddress_label_182

	// *** Basic block 13

.LoadedDynamicLibraryLookupSymbolByAddress_label_106:
	ld          t0, -24(s0)
	addi        t0, t0, 216
	ld          t0, 0(t0)
	lw          t1, -88(s0)
	slli        t2, t1, 3
	slli        t1, t1, 4
	add         t1, t2, t1
	add         t0, t0, t1
	sd          t0, -80(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 112
	ld          t0, 0(t0)
	ld          t1, -80(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -72(s0)
	ld          t0, -72(s0)
	ld          t1, -80(s0)
	addi        t1, t1, 16
	ld          t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -64(s0)
	ld          t1, -32(s0)
	ld          t2, -72(s0)
	slt         t3, t1, t2
	not         t0, t3
	blt         t1, t2, .LoadedDynamicLibraryLookupSymbolByAddress_label_143

	// *** Basic block 14

	ld          t1, -32(s0)
	ld          t2, -64(s0)
	slt         t0, t1, t2

	// *** Basic block 15

.LoadedDynamicLibraryLookupSymbolByAddress_label_143:
	beqz        t0, .LoadedDynamicLibraryLookupSymbolByAddress_label_170

	// *** Basic block 16

	ld          t0, -40(s0)
	ld          t1, -72(s0)
	sd          t1, 0(t0)
	ld          t0, -48(s0)
	ld          t1, -80(s0)
	addi        t1, t1, 16
	ld          t1, 0(t1)
	sd          t1, 0(t0)
	ld          t0, -56(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 224
	ld          t1, 0(t1)
	ld          t2, -80(s0)
	addi        t2, t2, 0
	lwu         t2, 0(t2)
	add         t1, t1, t2
	sd          t1, 0(t0)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .LoadedDynamicLibraryLookupSymbolByAddress_label_25

	// *** Basic block 17

.LoadedDynamicLibraryLookupSymbolByAddress_label_170:

	// *** Basic block 18

.LoadedDynamicLibraryLookupSymbolByAddress_label_171:
	lw          t0, -88(s0)
	addi        t0, t0, 1
	sw          t0, -88(s0)
	lw          t0, -88(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 232
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoadedDynamicLibraryLookupSymbolByAddress_label_106

	// *** Basic block 19

.LoadedDynamicLibraryLookupSymbolByAddress_label_182:
	mv          a0, x0
	j           .LoadedDynamicLibraryLookupSymbolByAddress_label_25
.func_end_LoadedDynamicLibraryLookupSymbolByAddress:
	.size LoadedDynamicLibraryLookupSymbolByAddress, .func_end_LoadedDynamicLibraryLookupSymbolByAddress-LoadedDynamicLibraryLookupSymbolByAddress

	.local  LoadLoadableSegment
	.type LoadLoadableSegment, @function

LoadLoadableSegment:

	// *** Basic block 0

	.global sysconf
	.local AlignDown
	.local AlignUp
	.global mmap
	.global LoaderError
	.global strerror
	.global errno
	.global memset
	.global VectorAppend
	.local NewMappedSegment
	addi sp, sp, -192
	// Saved return address (offset 184) and frame pointer (offset 176)
	sd ra, 184(sp)
	sd s0, 176(sp)
	addi s0, sp, 192
	// Saved argument registers.
	sd a0, -24(s0)
	sd a2, -32(s0)
	sd a1, -40(s0)
	sd a4, -48(s0)
	sd a3, -56(s0)
	// Local vars at offset -176(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	li          a0, 30		// 0x1e ASCII \x1e
	call        sysconf

	// *** Basic block 1

	sext.w      t0, a0
	sw          t0, -176(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 120
	ld          t1, 0(t1)
	addi        t1, t1, 16
	lhu         t1, 0(t1)
	addi        t2, t1, -3
	seqz        t2, t2
	li          t2, 3		// 0x3 ASCII \x3
	bne         t1, t2, .LoadLoadableSegment_label_61

	// *** Basic block 2

	ld          t1, -32(s0)
	ld          t2, -40(s0)
	addi        t2, t2, 16
	ld          t2, 0(t2)
	add         t0, t1, t2
	j           .LoadLoadableSegment_label_65

	// *** Basic block 3

.LoadLoadableSegment_label_61:
	ld          t1, -40(s0)
	addi        t1, t1, 16
	ld          t0, 0(t1)

	// *** Basic block 4

.LoadLoadableSegment_label_65:
	sd          t0, -168(s0)
	ld          t0, -168(s0)
	sd          t0, -160(s0)
	ld          t0, -48(s0)
	sub         t1, t0, x0
	snez        t1, t1
	beq         t0, x0, .LoadLoadableSegment_label_79

	// *** Basic block 5

	ld          t0, -48(s0)
	ld          t1, -160(s0)
	sd          t1, 0(t0)

	// *** Basic block 6

.LoadLoadableSegment_label_79:
	ld          t0, -168(s0)
	sd          t0, -152(s0)
	ld          a0, -168(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 48
	ld          a1, 0(t0)
	call        AlignDown

	// *** Basic block 7

	sd          a0, -168(s0)
	ld          t0, -40(s0)
	addi        t0, t0, 8
	ld          t0, 0(t0)
	sd          t0, -144(s0)
	ld          a0, -168(s0)
	lw          a1, -176(s0)
	call        AlignDown

	// *** Basic block 8

	sd          a0, -168(s0)
	ld          a0, -144(s0)
	lw          a1, -176(s0)
	call        AlignDown

	// *** Basic block 9

	sd          a0, -144(s0)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, -136(s0)
	ld          t1, -40(s0)
	addi        t1, t1, 4
	lwu         t1, 0(t1)
	andi        t1, t1, 2
	snez        t0, t1
	bnez        t1, .LoadLoadableSegment_label_126

	// *** Basic block 10

	ld          t1, -24(s0)
	addi        t1, t1, 80
	ld          t1, 0(t1)
	addi        t1, t1, 40
	lw          t1, 0(t1)
	andi        t1, t1, 4
	snez        t0, t1

	// *** Basic block 11

.LoadLoadableSegment_label_126:
	beqz        t0, .LoadLoadableSegment_label_131

	// *** Basic block 12

	lw          t0, -136(s0)
	ori         t0, t0, 2
	sw          t0, -136(s0)

	// *** Basic block 13

.LoadLoadableSegment_label_131:
	ld          t0, -40(s0)
	addi        t0, t0, 4
	lwu         t0, 0(t0)
	andi        t0, t0, 1
	snez        t1, t0
	beqz        t0, .LoadLoadableSegment_label_141

	// *** Basic block 14

	lw          t0, -136(s0)
	ori         t0, t0, 4
	sw          t0, -136(s0)

	// *** Basic block 15

.LoadLoadableSegment_label_141:
	ld          t0, -160(s0)
	ld          t1, -168(s0)
	sub         t0, t0, t1
	sd          t0, -128(s0)
	ld          t0, -128(s0)
	ld          t1, -40(s0)
	addi        t1, t1, 32
	ld          t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -120(s0)
	ld          a0, -120(s0)
	lw          a1, -176(s0)
	call        AlignUp

	// *** Basic block 16

	sd          a0, -112(s0)
	ld          t0, -56(s0)
	ld          t1, -112(s0)
	sd          t1, 0(t0)
	ld          t0, -56(s0)
	ld          t0, 0(t0)
	seqz        t1, t0
	bnez        t0, .LoadLoadableSegment_label_174

	// *** Basic block 17

	mv          a0, x0

	// *** Basic block 18

.LoadLoadableSegment_label_171:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 19

.LoadLoadableSegment_label_174:
	ld          t0, -168(s0)
	ld          t1, -112(s0)
	add         t0, t0, t1
	sd          t0, -104(s0)
	ld          a0, -168(s0)
	ld          t0, -56(s0)
	ld          a1, 0(t0)
	lw          a2, -136(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 104
	lw          a4, 0(t0)
	ld          a5, -144(s0)
	li          t0, 18		// 0x12 ASCII \x12
	mv          a3, t0
	call        mmap

	// *** Basic block 20

	sd          a0, -96(s0)
	ld          t0, -96(s0)
	li          t1, -1		// 0xffffffffffffffff
	sub         t2, t0, t1
	seqz        t2, t2
	bne         t0, t1, .LoadLoadableSegment_label_222

	// *** Basic block 21

	lla         s1, .str.16
	la          t0, errno
	lw          a0, 0(t0)
	call        strerror

	// *** Basic block 22

	mv          a1, a0
	mv          a0, s1
	call        LoaderError

	// *** Basic block 23

	mv          a0, x0
	j           .LoadLoadableSegment_label_171

	// *** Basic block 24

.LoadLoadableSegment_label_222:
	lw          t1, -136(s0)
	andi        t1, t1, 2
	snez        t0, t1
	beqz        t1, .LoadLoadableSegment_label_235

	// *** Basic block 25

	ld          t1, -40(s0)
	addi        t1, t1, 40
	ld          t1, 0(t1)
	ld          t2, -40(s0)
	addi        t2, t2, 32
	ld          t2, 0(t2)
	slt         t0, t2, t1

	// *** Basic block 26

.LoadLoadableSegment_label_235:
	beqz        t0, .LoadLoadableSegment_label_324

	// *** Basic block 27

	ld          t0, -152(s0)
	ld          t1, -40(s0)
	addi        t1, t1, 32
	ld          t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -88(s0)
	ld          t0, -104(s0)
	ld          t1, -88(s0)
	sub         t0, t0, t1
	sd          t0, -80(s0)
	ld          a0, -88(s0)
	ld          a2, -80(s0)
	mv          a1, x0
	call        memset

	// *** Basic block 28

	ld          t0, -40(s0)
	addi        t0, t0, 40
	ld          t0, 0(t0)
	ld          t1, -56(s0)
	ld          t1, 0(t1)
	sub         a0, t0, t1
	lw          a1, -176(s0)
	call        AlignUp

	// *** Basic block 29

	sd          a0, -72(s0)
	ld          t0, -72(s0)
	slt         t1, x0, t0
	bge         x0, t0, .LoadLoadableSegment_label_323

	// *** Basic block 30

	ld          t0, -104(s0)
	sd          t0, -64(s0)
	ld          a0, -64(s0)
	ld          a1, -72(s0)
	mv          a5, x0
	mv          a4, x0
	li          t0, 50		// 0x32 ASCII '2'
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	call        mmap

	// *** Basic block 31

	sd          a0, -64(s0)
	ld          t0, -64(s0)
	li          t1, -1		// 0xffffffffffffffff
	sub         t2, t0, t1
	seqz        t2, t2
	bne         t0, t1, .LoadLoadableSegment_label_310

	// *** Basic block 32

	lla         s1, .str.17
	la          t0, errno
	lw          a0, 0(t0)
	call        strerror

	// *** Basic block 33

	mv          a1, a0
	mv          a0, s1
	call        LoaderError

	// *** Basic block 34

	mv          a0, x0
	j           .LoadLoadableSegment_label_171

	// *** Basic block 35

.LoadLoadableSegment_label_310:
	ld          t0, -24(s0)
	addi        s1, t0, 248
	ld          a0, -64(s0)
	ld          a1, -72(s0)
	call        NewMappedSegment

	// *** Basic block 36

	mv          a1, a0
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 37

.LoadLoadableSegment_label_323:

	// *** Basic block 38

.LoadLoadableSegment_label_324:
	ld          t0, -96(s0)
	mv          a0, t0
	j           .LoadLoadableSegment_label_171
.func_end_LoadLoadableSegment:
	.size LoadLoadableSegment, .func_end_LoadLoadableSegment-LoadLoadableSegment

	.global LoadedDynamicLibraryLoadSegments
	.type LoadedDynamicLibraryLoadSegments, @function

LoadedDynamicLibraryLoadSegments:

	// *** Basic block 0

	.local LoadLoadableSegment
	.global VectorAppend
	.local NewMappedSegment
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Saved argument registers.
	sd a2, -24(s0)
	sd a0, -32(s0)
	sd a1, -40(s0)
	// Local vars at offset -88(s0)
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	ld          t0, -24(s0)
	sd          x0, 0(t0)
	sd          x0, -88(s0)
	sw          x0, -80(s0)
	lw          t0, -80(s0)
	ld          t1, -32(s0)
	addi        t1, t1, 120
	ld          t1, 0(t1)
	addi        t1, t1, 56
	lhu         t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoadedDynamicLibraryLoadSegments_label_154

	// *** Basic block 1

.LoadedDynamicLibraryLoadSegments_label_32:
	ld          t0, -32(s0)
	addi        t0, t0, 128
	ld          t0, 0(t0)
	lw          t1, -80(s0)
	slli        t2, t1, 3
	slli        t3, t1, 4
	add         t2, t2, t3
	slli        t1, t1, 5
	add         t1, t2, t1
	add         t0, t0, t1
	sd          t0, -72(s0)
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	ld          t0, -72(s0)
	addi        t0, t0, 0
	lwu         t0, 0(t0)
	addi        t1, t0, -1
	seqz        t1, t1
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .LoadedDynamicLibraryLoadSegments_label_79

	// *** Basic block 2

	ld          a0, -32(s0)
	ld          a1, -72(s0)
	ld          a2, -40(s0)
	addi        a3, s0, -56
	mv          a4, x0
	call        LoadLoadableSegment

	// *** Basic block 3

	sd          a0, -64(s0)
	j           .LoadedDynamicLibraryLoadSegments_label_102

	// *** Basic block 4

.LoadedDynamicLibraryLoadSegments_label_79:
	ld          t0, -72(s0)
	addi        t0, t0, 0
	lwu         t0, 0(t0)
	addi        t1, t0, -2
	seqz        t1, t1
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .LoadedDynamicLibraryLoadSegments_label_101

	// *** Basic block 5

	ld          a0, -32(s0)
	ld          a1, -72(s0)
	ld          a2, -40(s0)
	addi        a3, s0, -56
	ld          t0, -32(s0)
	addi        a4, t0, 144
	call        LoadLoadableSegment

	// *** Basic block 6

	sd          a0, -64(s0)

	// *** Basic block 7

.LoadedDynamicLibraryLoadSegments_label_101:

	// *** Basic block 8

.LoadedDynamicLibraryLoadSegments_label_102:
	ld          t0, -64(s0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .LoadedDynamicLibraryLoadSegments_label_108

	// *** Basic block 9

	j           .LoadedDynamicLibraryLoadSegments_label_141

	// *** Basic block 10

.LoadedDynamicLibraryLoadSegments_label_108:
	ld          t0, -88(s0)
	seqz        t1, t0
	bnez        t0, .LoadedDynamicLibraryLoadSegments_label_114

	// *** Basic block 11

	ld          t0, -64(s0)
	sd          t0, -88(s0)

	// *** Basic block 12

.LoadedDynamicLibraryLoadSegments_label_114:
	ld          t0, -64(s0)
	ld          t1, -56(s0)
	add         t0, t0, t1
	sd          t0, -48(s0)
	ld          t0, -32(s0)
	addi        s1, t0, 248
	ld          a0, -64(s0)
	ld          a1, -56(s0)
	call        NewMappedSegment

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 14

	ld          t0, -48(s0)
	ld          t1, -24(s0)
	ld          t1, 0(t1)
	slt         t2, t1, t0
	bge         t1, t0, .LoadedDynamicLibraryLoadSegments_label_140

	// *** Basic block 15

	ld          t0, -24(s0)
	ld          t1, -48(s0)
	sd          t1, 0(t0)

	// *** Basic block 16

.LoadedDynamicLibraryLoadSegments_label_140:

	// *** Basic block 17

.LoadedDynamicLibraryLoadSegments_label_141:
	lw          t0, -80(s0)
	addi        t0, t0, 1
	sw          t0, -80(s0)
	lw          t0, -80(s0)
	ld          t1, -32(s0)
	addi        t1, t1, 120
	ld          t1, 0(t1)
	addi        t1, t1, 56
	lhu         t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoadedDynamicLibraryLoadSegments_label_32

	// *** Basic block 18

.LoadedDynamicLibraryLoadSegments_label_154:
	ld          t0, -88(s0)
	mv          a0, t0

	// *** Basic block 19

.LoadedDynamicLibraryLoadSegments_label_158:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LoadedDynamicLibraryLoadSegments:
	.size LoadedDynamicLibraryLoadSegments, .func_end_LoadedDynamicLibraryLoadSegments-LoadedDynamicLibraryLoadSegments

	.global LoadedDynamicLibraryRelocate
	.type LoadedDynamicLibraryRelocate, @function

LoadedDynamicLibraryRelocate:

	// *** Basic block 0

	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	sd a2, -40(s0)
	sd a3, -48(s0)
	// Local vars at offset -48(s0)
	// End of stack frame
	.local PerformDynamicRelocations
	ld          a0, -24(s0)
	ld          a1, -32(s0)
	ld          a2, -40(s0)
	lb          a3, -48(s0)
	call        PerformDynamicRelocations

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LoadedDynamicLibraryRelocate:
	.size LoadedDynamicLibraryRelocate, .func_end_LoadedDynamicLibraryRelocate-LoadedDynamicLibraryRelocate

.PCend:
	.data
	.type   print_libraries_only,@object
	.global print_libraries_only
	.comm   print_libraries_only,1,1

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s/%s"
	.type .str.1, @object
	.size .str.1, 6

.str.2:
	.asciz "%s/%s"
	.type .str.2, @object
	.size .str.2, 6

.str.3:
	.asciz "Cannot find library %s"
	.type .str.3, @object
	.size .str.3, 23

.str.4:
	.asciz "Failed to load dynamic library %s: no symbol table"
	.type .str.4, @object
	.size .str.4, 51

.str.5:
	.asciz "$ORIGIN"
	.type .str.5, @object
	.size .str.5, 8

.str.6:
	.asciz "${ORIGIN}"
	.type .str.6, @object
	.size .str.6, 10

.str.7:
	.asciz "$EXEC_ORIGIN"
	.type .str.7, @object
	.size .str.7, 13

.str.8:
	.asciz "${EXEC_ORIGIN}"
	.type .str.8, @object
	.size .str.8, 15

.str.9:
	.asciz "$LIB"
	.type .str.9, @object
	.size .str.9, 5

.str.10:
	.asciz "${LIB}"
	.type .str.10, @object
	.size .str.10, 7

.str.11:
	.asciz "lib"
	.type .str.11, @object
	.size .str.11, 4

.str.12:
	.asciz "$PLATFORM"
	.type .str.12, @object
	.size .str.12, 10

.str.13:
	.asciz "${PLATFORM}"
	.type .str.13, @object
	.size .str.13, 12

.str.14:
	.asciz "(null)"
	.type .str.14, @object
	.size .str.14, 1

.str.15:
	.asciz "\t%s => %s (0x%" PRIx64 ")\n"
	.type .str.15, @object
	.size .str.15, 20

.str.16:
	.asciz "Failed to map in dynamic segment: %s\n"
	.type .str.16, @object
	.size .str.16, 38

.str.17:
	.asciz "Failed to map in dynamic segment: %s\n"
	.type .str.17, @object
	.size .str.17, 38

