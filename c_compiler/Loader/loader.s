	.file   "loader.c"
	.text
	.option pic
.PCbegin:
	.global LoaderNumErrors
	.type LoaderNumErrors, @function

LoaderNumErrors:

	// *** Basic block 0

	// Local vars at offset -16(s0)
	// End of stack frame
	.local num_errors
	la          t0, num_errors
	lw          t0, 0(t0)
	mv          a0, t0

	// *** Basic block 1

.LoaderNumErrors_label_8:
	// Restored registers.
	ret         
.func_end_LoaderNumErrors:
	.size LoaderNumErrors, .func_end_LoaderNumErrors-LoaderNumErrors

	.global NewRegion
	.type NewRegion, @function

NewRegion:

	// *** Basic block 0

	.global malloc
	.global VectorInit
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
	li          a0, 40		// 0x28 ASCII '('
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
	addi        a0, t0, 16
	call        VectorInit

	// *** Basic block 2

	ld          t0, -40(s0)
	mv          a0, t0

	// *** Basic block 3

.NewRegion_label_33:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewRegion:
	.size NewRegion, .func_end_NewRegion-NewRegion

	.global RegionDestruct
	.type RegionDestruct, @function

RegionDestruct:

	// *** Basic block 0

	.global VectorDestruct
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
	addi        a0, t0, 16
	call        VectorDestruct

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_RegionDestruct:
	.size RegionDestruct, .func_end_RegionDestruct-RegionDestruct

	.global VLoaderError
	.type VLoaderError, @function

VLoaderError:

	// *** Basic block 0

	.global vsnprintf
	.global fprintf
	.global stderr
	.local num_errors
	addi sp, sp, -16
	sd ra, 8(sp)
	sd s0, 0(sp)
	lui t0, 1
	addi t0, t0, 32
	sub sp, sp, t0
	addi t0, t0, 16
	add s0, sp, t0
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -4128(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	li          t0, -4096		// 0xfffffffffffff000
	add         s1, s0, t0
	addi        a0, s1, -32
	ld          a2, -24(s0)
	ld          a3, -32(s0)
	li          a1, 4096		// 0x1000
	call        vsnprintf

	// *** Basic block 1

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.1
	addi        a2, s1, -32
	call        fprintf

	// *** Basic block 2

	la          t0, num_errors
	lw          t1, 0(t0)
	addi        t0, t1, 1
	la          t1, num_errors
	sw          t0, 0(t1)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_VLoaderError:
	.size VLoaderError, .func_end_VLoaderError-VLoaderError

	.global LoaderError
	.type LoaderError, @function

LoaderError:

	// *** Basic block 0

	addi sp, sp, -96
	// Saved return address (offset 32) and frame pointer (offset 24)
	sd ra, 32(sp)
	sd s0, 24(sp)
	addi s0, sp, 40
	// varargs function with 1 declared args
	sd a1, 0(s0)
	sd a2, 8(s0)
	sd a3, 16(s0)
	sd a4, 24(s0)
	sd a5, 32(s0)
	sd a6, 40(s0)
	sd a7, 48(s0)
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	.global VLoaderError
	sd          s0, -32(s0)
	ld          a0, -24(s0)
	ld          a1, -32(s0)
	call        VLoaderError

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 56
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LoaderError:
	.size LoaderError, .func_end_LoaderError-LoaderError

	.global VLoaderWarning
	.type VLoaderWarning, @function

VLoaderWarning:

	// *** Basic block 0

	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a1, -24(s0)
	sd a2, -32(s0)
	// Local vars at offset -32(s0)
	// End of stack frame
	.global vfprintf
	.global stderr
	la          t0, stderr
	ld          a0, 0(t0)
	ld          a1, -24(s0)
	ld          a2, -32(s0)
	call        vfprintf

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_VLoaderWarning:
	.size VLoaderWarning, .func_end_VLoaderWarning-VLoaderWarning

	.global LoaderWarning
	.type LoaderWarning, @function

LoaderWarning:

	// *** Basic block 0

	addi sp, sp, -96
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// varargs function with 2 declared args
	sd a2, 0(s0)
	sd a3, 8(s0)
	sd a4, 16(s0)
	sd a5, 24(s0)
	sd a6, 32(s0)
	sd a7, 40(s0)
	// Saved argument registers.
	sd a1, -24(s0)
	sd a0, -32(s0)
	// Local vars at offset -40(s0)
	// End of stack frame
	.global VLoaderWarning
	sd          s0, -40(s0)
	ld          a0, -32(s0)
	ld          a1, -24(s0)
	ld          a2, -40(s0)
	call        VLoaderWarning

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 48
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LoaderWarning:
	.size LoaderWarning, .func_end_LoaderWarning-LoaderWarning

	.global LoaderFindSymbol
	.type LoaderFindSymbol, @function

LoaderFindSymbol:

	// *** Basic block 0

	.global DynamicLoaderLookupSymbolByAddress
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	sd a2, -40(s0)
	// Local vars at offset -72(s0)
	// End of stack frame
	ld          t0, -24(s0)
	addi        a0, t0, 224
	ld          a1, -32(s0)
	addi        a2, s0, -64
	addi        a3, s0, -56
	addi        a4, s0, -48
	call        DynamicLoaderLookupSymbolByAddress

	// *** Basic block 1

	sb          a0, -72(s0)
	lb          t0, -72(s0)
	not         t0, t0
	beqz        t0, .LoaderFindSymbol_label_42

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.LoaderFindSymbol_label_39:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.LoaderFindSymbol_label_42:
	ld          t0, -40(s0)
	addi        t0, t0, 0
	ld          t1, -56(s0)
	sd          t1, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 8
	ld          t1, -56(s0)
	ld          t2, -48(s0)
	add         t1, t1, t2
	sd          t1, 0(t0)
	ld          t0, -40(s0)
	addi        t0, t0, 16
	ld          t1, -64(s0)
	sd          t1, 0(t0)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .LoaderFindSymbol_label_39
.func_end_LoaderFindSymbol:
	.size LoaderFindSymbol, .func_end_LoaderFindSymbol-LoaderFindSymbol

	.global LoaderFindSymbolAndCacheResult
	.type LoaderFindSymbolAndCacheResult, @function

LoaderFindSymbolAndCacheResult:

	// *** Basic block 0

	.global LoaderFindSymbol
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
	ld          t1, -24(s0)
	ld          t2, -32(s0)
	addi        t2, t2, 280
	addi        t2, t2, 0
	ld          t2, 0(t2)
	slt         t3, t1, t2
	not         t0, t3
	blt         t1, t2, .LoaderFindSymbolAndCacheResult_label_25

	// *** Basic block 1

	ld          t1, -24(s0)
	ld          t2, -32(s0)
	addi        t2, t2, 280
	addi        t2, t2, 8
	ld          t2, 0(t2)
	slt         t0, t1, t2

	// *** Basic block 2

.LoaderFindSymbolAndCacheResult_label_25:
	beqz        t0, .LoaderFindSymbolAndCacheResult_label_34

	// *** Basic block 3

	ld          t0, -32(s0)
	addi        t0, t0, 280
	mv          a0, t0

	// *** Basic block 4

.LoaderFindSymbolAndCacheResult_label_31:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.LoaderFindSymbolAndCacheResult_label_34:
	ld          a0, -32(s0)
	ld          a1, -24(s0)
	ld          t0, -32(s0)
	addi        a2, t0, 280
	call        LoaderFindSymbol

	// *** Basic block 6

	sb          a0, -40(s0)
	lb          t0, -40(s0)
	not         t0, t0
	beqz        t0, .LoaderFindSymbolAndCacheResult_label_54

	// *** Basic block 7

	mv          a0, x0
	j           .LoaderFindSymbolAndCacheResult_label_31

	// *** Basic block 8

.LoaderFindSymbolAndCacheResult_label_54:
	ld          t0, -32(s0)
	addi        t0, t0, 280
	mv          a0, t0
	j           .LoaderFindSymbolAndCacheResult_label_31
.func_end_LoaderFindSymbolAndCacheResult:
	.size LoaderFindSymbolAndCacheResult, .func_end_LoaderFindSymbolAndCacheResult-LoaderFindSymbolAndCacheResult

	.global LoaderLookupSymbol
	.type LoaderLookupSymbol, @function

LoaderLookupSymbol:

	// *** Basic block 0

	.global DynamicLoaderFindSymbol
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
	addi        a0, t0, 224
	ld          a1, -32(s0)
	addi        a2, s0, -48
	addi        a3, s0, -40
	call        DynamicLoaderFindSymbol

	// *** Basic block 1

	sb          a0, -56(s0)
	lb          t0, -56(s0)
	not         t0, t0
	beqz        t0, .LoaderLookupSymbol_label_37

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.LoaderLookupSymbol_label_34:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.LoaderLookupSymbol_label_37:
	ld          t0, -40(s0)
	addi        t0, t0, 112
	ld          t0, 0(t0)
	ld          t1, -48(s0)
	addi        t1, t1, 8
	ld          t1, 0(t1)
	add         t0, t0, t1
	mv          a0, t0
	j           .LoaderLookupSymbol_label_34
.func_end_LoaderLookupSymbol:
	.size LoaderLookupSymbol, .func_end_LoaderLookupSymbol-LoaderLookupSymbol

	.global LoaderSetCurrentSymbol
	.type LoaderSetCurrentSymbol, @function

LoaderSetCurrentSymbol:

	// *** Basic block 0

	ret         
.func_end_LoaderSetCurrentSymbol:
	.size LoaderSetCurrentSymbol, .func_end_LoaderSetCurrentSymbol-LoaderSetCurrentSymbol

	.global LoaderGetCurrentSymbol
	.type LoaderGetCurrentSymbol, @function

LoaderGetCurrentSymbol:

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
	addi        t0, t0, 280
	mv          a0, t0

	// *** Basic block 1

.LoaderGetCurrentSymbol_label_9:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LoaderGetCurrentSymbol:
	.size LoaderGetCurrentSymbol, .func_end_LoaderGetCurrentSymbol-LoaderGetCurrentSymbol

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

	.local  GetRegionSections
	.type GetRegionSections, @function

GetRegionSections:

	// *** Basic block 0

	.global VectorAppend
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
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 8
	ld          t0, 0(t0)
	sd          t0, -88(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 8
	ld          t0, 0(t0)
	ld          t1, -24(s0)
	addi        t1, t1, 32
	ld          t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -80(s0)
	sd          x0, -72(s0)
	ld          t0, -72(s0)
	ld          t1, -32(s0)
	addi        t1, t1, 144
	ld          t1, 0(t1)
	addi        t1, t1, 56
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .GetRegionSections_label_122

	// *** Basic block 1

.GetRegionSections_label_42:
	ld          t0, -32(s0)
	addi        t0, t0, 144
	ld          t0, 0(t0)
	addi        t0, t0, 56
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
	ld          t0, 0(t0)
	addi        t0, t0, 4
	lwu         t0, 0(t0)
	seqz        t1, t0
	bnez        t0, .GetRegionSections_label_65

	// *** Basic block 2

	j           .GetRegionSections_label_108

	// *** Basic block 3

.GetRegionSections_label_65:
	ld          t0, -64(s0)
	addi        t0, t0, 0
	ld          t0, 0(t0)
	addi        t0, t0, 24
	ld          t0, 0(t0)
	sd          t0, -56(s0)
	ld          t0, -64(s0)
	addi        t0, t0, 0
	ld          t0, 0(t0)
	addi        t0, t0, 24
	ld          t0, 0(t0)
	ld          t1, -64(s0)
	addi        t1, t1, 0
	ld          t1, 0(t1)
	addi        t1, t1, 32
	ld          t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -48(s0)
	ld          t1, -56(s0)
	ld          t2, -88(s0)
	slt         t3, t1, t2
	not         t0, t3
	blt         t1, t2, .GetRegionSections_label_96

	// *** Basic block 4

	ld          t1, -48(s0)
	ld          t2, -80(s0)
	slt         t1, t2, t1
	not         t0, t1

	// *** Basic block 5

.GetRegionSections_label_96:
	beqz        t0, .GetRegionSections_label_107

	// *** Basic block 6

	ld          t0, -40(s0)
	addi        a0, t0, 16
	ld          a1, -64(s0)
	call        VectorAppend

	// *** Basic block 7

.GetRegionSections_label_107:

	// *** Basic block 8

.GetRegionSections_label_108:
	ld          t0, -72(s0)
	addi        t0, t0, 1
	sd          t0, -72(s0)
	ld          t0, -72(s0)
	ld          t1, -32(s0)
	addi        t1, t1, 144
	ld          t1, 0(t1)
	addi        t1, t1, 56
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .GetRegionSections_label_42

	// *** Basic block 9

.GetRegionSections_label_122:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GetRegionSections:
	.size GetRegionSections, .func_end_GetRegionSections-GetRegionSections

	.local  LoadStaticSegments
	.type LoadStaticSegments, @function

LoadStaticSegments:

	// *** Basic block 0

	.global sysconf
	.local AlignDown
	.local AlignUp
	.global open
	.global printf
	.global mmap
	.global strerror
	.global errno
	.global close
	.global memset
	.global LoaderError
	.global VectorAppend
	.global NewRegion
	.local GetRegionSections
	addi sp, sp, -192
	// Saved return address (offset 184) and frame pointer (offset 176)
	sd ra, 184(sp)
	sd s0, 176(sp)
	addi s0, sp, 192
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -168(s0)
	// Saved integer registers.
	sd s1, 16(sp)
	sd s2, 8(sp)
	// End of stack frame
	li          a0, 30		// 0x1e ASCII \x1e
	call        sysconf

	// *** Basic block 1

	sext.w      t0, a0
	sw          t0, -168(s0)
	sd          x0, -160(s0)
	ld          t0, -160(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 144
	ld          t1, 0(t1)
	addi        t1, t1, 80
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoadStaticSegments_label_422

	// *** Basic block 2

.LoadStaticSegments_label_55:
	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t0, 0(t0)
	addi        t0, t0, 80
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -160(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -152(s0)
	ld          t0, -152(s0)
	addi        t0, t0, 0
	lwu         t0, 0(t0)
	addi        t1, t0, -1
	seqz        t1, t1
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .LoadStaticSegments_label_407

	// *** Basic block 3

	ld          t0, -152(s0)
	addi        t0, t0, 40
	ld          t0, 0(t0)
	seqz        t1, t0
	bnez        t0, .LoadStaticSegments_label_82

	// *** Basic block 4

	j           .LoadStaticSegments_label_422

	// *** Basic block 5

.LoadStaticSegments_label_82:
	ld          t0, -24(s0)
	addi        t0, t0, 48
	ld          t0, 0(t0)
	addi        t0, t0, 16
	lb          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .LoadStaticSegments_label_93

	// *** Basic block 6

	mv          s1, x0
	j           .LoadStaticSegments_label_97

	// *** Basic block 7

.LoadStaticSegments_label_93:
	ld          t0, -152(s0)
	addi        t0, t0, 16
	ld          s1, 0(t0)

	// *** Basic block 8

.LoadStaticSegments_label_97:
	sd          s1, -144(s0)
	ld          t0, -152(s0)
	addi        t0, t0, 8
	ld          t0, 0(t0)
	sd          t0, -136(s0)
	ld          t0, -152(s0)
	addi        t0, t0, 48
	ld          t0, 0(t0)
	addi        t0, t0, -1
	sd          t0, -128(s0)
	ld          t0, -128(s0)
	not         t0, t0
	ld          t1, -144(s0)
	and         t0, t1, t0
	sd          t0, -144(s0)
	ld          a0, -144(s0)
	lw          a1, -168(s0)
	call        AlignDown

	// *** Basic block 9

	sd          a0, -144(s0)
	ld          a0, -136(s0)
	lw          a1, -168(s0)
	call        AlignDown

	// *** Basic block 10

	sd          a0, -136(s0)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, -120(s0)
	sw          x0, -116(s0)
	ld          t1, -152(s0)
	addi        t1, t1, 4
	lwu         t1, 0(t1)
	andi        t1, t1, 2
	snez        t0, t1
	bnez        t1, .LoadStaticSegments_label_146

	// *** Basic block 11

	ld          t1, -24(s0)
	addi        t1, t1, 40
	lw          t1, 0(t1)
	andi        t1, t1, 4
	snez        t0, t1

	// *** Basic block 12

.LoadStaticSegments_label_146:
	beqz        t0, .LoadStaticSegments_label_153

	// *** Basic block 13

	lw          t0, -120(s0)
	ori         t0, t0, 2
	sw          t0, -120(s0)
	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, -116(s0)

	// *** Basic block 14

.LoadStaticSegments_label_153:
	ld          t0, -152(s0)
	addi        t0, t0, 4
	lwu         t0, 0(t0)
	andi        t0, t0, 1
	snez        t1, t0
	beqz        t0, .LoadStaticSegments_label_163

	// *** Basic block 15

	lw          t0, -120(s0)
	ori         t0, t0, 4
	sw          t0, -120(s0)

	// *** Basic block 16

.LoadStaticSegments_label_163:
	ld          t0, -152(s0)
	addi        t0, t0, 16
	ld          t0, 0(t0)
	ld          t1, -144(s0)
	sub         t0, t0, t1
	sd          t0, -112(s0)
	ld          t0, -112(s0)
	ld          t1, -152(s0)
	addi        t1, t1, 32
	ld          t1, 0(t1)
	add         t0, t0, t1
	sd          t0, -104(s0)
	ld          a0, -104(s0)
	lw          a1, -168(s0)
	call        AlignUp

	// *** Basic block 17

	sd          a0, -104(s0)
	ld          t0, -32(s0)
	addi        t0, t0, 16
	ld          a0, 0(t0)
	lw          a1, -116(s0)
	call        open

	// *** Basic block 18

	sw          a0, -96(s0)
	lw          t0, -96(s0)
	sltz        t1, t0
	bge         t0, x0, .LoadStaticSegments_label_206

	// *** Basic block 19

	lla         a0, .str.2
	call        printf

	// *** Basic block 20

	mv          a0, x0

	// *** Basic block 21

.LoadStaticSegments_label_203:
	// Restored registers.
	ld s1, 16(sp)
	ld s2, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 22

.LoadStaticSegments_label_206:
	ld          t0, -104(s0)
	seqz        t1, t0
	bnez        t0, .LoadStaticSegments_label_211

	// *** Basic block 23

	j           .LoadStaticSegments_label_408

	// *** Basic block 24

.LoadStaticSegments_label_211:
	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, -92(s0)
	ld          t0, -144(s0)
	snez        t1, t0
	beqz        t0, .LoadStaticSegments_label_221

	// *** Basic block 25

	lw          t0, -92(s0)
	ori         t0, t0, 16
	sw          t0, -92(s0)

	// *** Basic block 26

.LoadStaticSegments_label_221:
	ld          a0, -144(s0)
	ld          a1, -104(s0)
	lw          a2, -120(s0)
	lw          a3, -92(s0)
	lw          a4, -96(s0)
	ld          a5, -136(s0)
	call        mmap

	// *** Basic block 27

	sd          a0, -88(s0)
	ld          t0, -88(s0)
	li          t1, -1		// 0xffffffffffffffff
	sub         t2, t0, t1
	seqz        t2, t2
	bne         t0, t1, .LoadStaticSegments_label_260

	// *** Basic block 28

	lla         s2, .str.3
	la          t0, errno
	lw          a0, 0(t0)
	call        strerror

	// *** Basic block 29

	mv          a1, a0
	mv          a0, s2
	call        printf

	// *** Basic block 30

	mv          a0, x0
	j           .LoadStaticSegments_label_203

	// *** Basic block 31

.LoadStaticSegments_label_260:
	lw          a0, -96(s0)
	call        close

	// *** Basic block 32

	lw          t0, -120(s0)
	andi        t0, t0, 2
	snez        t1, t0
	beqz        t0, .LoadStaticSegments_label_384

	// *** Basic block 33

	ld          t1, -24(s0)
	addi        t1, t1, 48
	ld          t1, 0(t1)
	addi        t1, t1, 16
	lb          t1, 0(t1)
	snez        t2, t1
	beqz        t1, .LoadStaticSegments_label_278

	// *** Basic block 34

	ld          t0, -88(s0)
	j           .LoadStaticSegments_label_282

	// *** Basic block 35

.LoadStaticSegments_label_278:
	ld          t1, -152(s0)
	addi        t1, t1, 16
	ld          t0, 0(t1)

	// *** Basic block 36

.LoadStaticSegments_label_282:
	ld          t1, -152(s0)
	addi        t1, t1, 32
	ld          t1, 0(t1)
	add         t0, t0, x0
	sd          t0, -80(s0)
	ld          t0, -24(s0)
	addi        t0, t0, 48
	ld          t0, 0(t0)
	addi        t0, t0, 16
	lb          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .LoadStaticSegments_label_299

	// *** Basic block 37

	ld          s2, -88(s0)
	j           .LoadStaticSegments_label_301

	// *** Basic block 38

.LoadStaticSegments_label_299:
	ld          s2, -144(s0)

	// *** Basic block 39

.LoadStaticSegments_label_301:
	ld          t0, -104(s0)
	add         t0, s2, t0
	sd          t0, -72(s0)
	ld          t0, -72(s0)
	ld          t1, -80(s0)
	sub         t0, t0, t1
	sd          t0, -64(s0)
	ld          a0, -80(s0)
	ld          a2, -64(s0)
	mv          a1, x0
	call        memset

	// *** Basic block 40

	ld          t0, -152(s0)
	addi        t0, t0, 40
	ld          t0, 0(t0)
	ld          t1, -104(s0)
	sub         a0, t0, t1
	lw          a1, -168(s0)
	call        AlignUp

	// *** Basic block 41

	sd          a0, -56(s0)
	ld          t0, -56(s0)
	slt         t1, x0, t0
	bge         x0, t0, .LoadStaticSegments_label_383

	// *** Basic block 42

	ld          t0, -72(s0)
	sd          t0, -48(s0)
	ld          a0, -48(s0)
	ld          a1, -56(s0)
	mv          a5, x0
	mv          a4, x0
	li          t0, 50		// 0x32 ASCII '2'
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	call        mmap

	// *** Basic block 43

	sd          a0, -48(s0)
	ld          t0, -48(s0)
	li          t1, -1		// 0xffffffffffffffff
	sub         t2, t0, t1
	seqz        t2, t2
	bne         t0, t1, .LoadStaticSegments_label_370

	// *** Basic block 44

	lla         s2, .str.4
	la          t0, errno
	lw          a0, 0(t0)
	call        strerror

	// *** Basic block 45

	mv          a1, a0
	mv          a0, s2
	call        LoaderError

	// *** Basic block 46

	mv          a0, x0
	j           .LoadStaticSegments_label_203

	// *** Basic block 47

.LoadStaticSegments_label_370:
	ld          t0, -24(s0)
	addi        s2, t0, 160
	ld          a0, -48(s0)
	ld          a1, -56(s0)
	call        NewRegion

	// *** Basic block 48

	mv          a1, a0
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 49

.LoadStaticSegments_label_383:

	// *** Basic block 50

.LoadStaticSegments_label_384:
	ld          a0, -88(s0)
	ld          a1, -104(s0)
	call        NewRegion

	// *** Basic block 51

	sd          a0, -40(s0)
	ld          t0, -24(s0)
	addi        a0, t0, 160
	ld          a1, -40(s0)
	call        VectorAppend

	// *** Basic block 52

	ld          a0, -24(s0)
	ld          a1, -40(s0)
	ld          a2, -152(s0)
	ld          a3, -160(s0)
	call        GetRegionSections

	// *** Basic block 53

.LoadStaticSegments_label_407:

	// *** Basic block 54

.LoadStaticSegments_label_408:
	ld          t0, -160(s0)
	addi        t0, t0, 1
	sd          t0, -160(s0)
	ld          t0, -160(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 144
	ld          t1, 0(t1)
	addi        t1, t1, 80
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoadStaticSegments_label_55

	// *** Basic block 55

.LoadStaticSegments_label_422:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .LoadStaticSegments_label_203
.func_end_LoadStaticSegments:
	.size LoadStaticSegments, .func_end_LoadStaticSegments-LoadStaticSegments

	.local  FindDynamicSection
	.type FindDynamicSection, @function

FindDynamicSection:

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
	sd          x0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 144
	ld          t1, 0(t1)
	addi        t1, t1, 56
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindDynamicSection_label_73

	// *** Basic block 1

.FindDynamicSection_label_25:
	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t0, 0(t0)
	addi        t0, t0, 56
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
	ld          t0, 0(t0)
	addi        t0, t0, 4
	lwu         t0, 0(t0)
	addi        t1, t0, -6
	seqz        t1, t1
	li          t1, 6		// 0x6 ASCII \x6
	bne         t0, t1, .FindDynamicSection_label_58

	// *** Basic block 2

	ld          t0, -32(s0)
	addi        t0, t0, 48
	ld          t0, 0(t0)
	mv          a0, t0

	// *** Basic block 3

.FindDynamicSection_label_55:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.FindDynamicSection_label_58:

	// *** Basic block 5

.FindDynamicSection_label_59:
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 144
	ld          t1, 0(t1)
	addi        t1, t1, 56
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .FindDynamicSection_label_25

	// *** Basic block 6

.FindDynamicSection_label_73:
	mv          a0, x0
	j           .FindDynamicSection_label_55
.func_end_FindDynamicSection:
	.size FindDynamicSection, .func_end_FindDynamicSection-FindDynamicSection

	.local  LoadDynamic
	.type LoadDynamic, @function

LoadDynamic:

	// *** Basic block 0

	.global NewLoadedDynamicLibrary
	.global DynamicLibraryRegistryInsert
	.global LoadedDynamicLibraryLoad
	.local load_addr
	.global LoaderError
	.global LoadedDynamicLibraryDelete
	.local AlignUp
	.global sysconf
	.global LoadedDynamicLibraryRelocate
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Saved argument registers.
	sd a0, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -64(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	sd          x0, -64(s0)
	ld          t0, -24(s0)
	addi        s1, t0, 216
	ld          t0, -24(s0)
	addi        t0, t0, 0
	addi        t0, t0, 16
	ld          a0, 0(t0)
	ld          a1, -24(s0)
	call        NewLoadedDynamicLibrary

	// *** Basic block 1

	sd          a0, 0(s1)
	ld          t0, -24(s0)
	addi        a0, t0, 224
	ld          t0, -24(s0)
	addi        t0, t0, 216
	ld          a1, 0(t0)
	call        DynamicLibraryRegistryInsert

	// *** Basic block 2

	ld          t0, -24(s0)
	addi        t0, t0, 216
	ld          a0, 0(t0)
	ld          t0, -24(s0)
	addi        a1, t0, 224
	ld          t0, -24(s0)
	addi        a2, t0, 192
	lla         t0, load_addr
	ld          a3, 0(t0)
	addi        a4, s0, -64
	call        LoadedDynamicLibraryLoad

	// *** Basic block 3

	sb          a0, -56(s0)
	lb          t0, -56(s0)
	not         t0, t0
	beqz        t0, .LoadDynamic_label_94

	// *** Basic block 4

	lla         a0, .str.5
	ld          t0, -24(s0)
	addi        t0, t0, 216
	ld          t0, 0(t0)
	addi        t0, t0, 0
	addi        t0, t0, 16
	ld          a1, 0(t0)
	call        LoaderError

	// *** Basic block 5

	ld          t0, -24(s0)
	addi        t0, t0, 216
	ld          a0, 0(t0)
	call        LoadedDynamicLibraryDelete

	// *** Basic block 6

	mv          a0, x0

	// *** Basic block 7

.LoadDynamic_label_91:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 8

.LoadDynamic_label_94:
	ld          s1, -64(s0)
	li          t0, 30		// 0x1e ASCII \x1e
	mv          a0, t0
	call        sysconf

	// *** Basic block 9

	sext.w      a1, a0
	mv          a0, s1
	call        AlignUp

	// *** Basic block 10

	lla         t0, load_addr
	sd          a0, 0(t0)
	sd          x0, -48(s0)
	ld          t0, -48(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 224
	addi        t1, t1, 32
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoadDynamic_label_155

	// *** Basic block 11

.LoadDynamic_label_117:
	ld          t0, -24(s0)
	addi        t0, t0, 224
	addi        t0, t0, 32
	addi        t0, t0, 0
	addi        t0, t0, 0
	ld          t0, 0(t0)
	ld          t1, -48(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -40(s0)
	ld          a0, -24(s0)
	ld          a1, -40(s0)
	ld          t0, -24(s0)
	addi        a2, t0, 224
	lb          a3, -32(s0)
	call        LoadedDynamicLibraryRelocate

	// *** Basic block 12

.LoadDynamic_label_142:
	ld          t0, -48(s0)
	addi        t0, t0, 1
	sd          t0, -48(s0)
	ld          t0, -48(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 224
	addi        t1, t1, 32
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoadDynamic_label_117

	// *** Basic block 13

.LoadDynamic_label_155:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	j           .LoadDynamic_label_91
.func_end_LoadDynamic:
	.size LoadDynamic, .func_end_LoadDynamic-LoadDynamic

	.local  InitLibrarySearchPath
	.type InitLibrarySearchPath, @function

InitLibrarySearchPath:

	// *** Basic block 0

	.global VectorAppend
	.global NewString
	.global getenv
	.global StringAppendSegment
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -56(s0)
	// Saved integer registers.
	sd s1, 0(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        s1, t0, 192
	lla         a0, .str.6
	call        NewString

	// *** Basic block 1

	mv          a1, a0
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 2

	ld          t0, -24(s0)
	addi        s1, t0, 192
	lla         a0, .str.7
	call        NewString

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 4

	lla         a0, .str.8
	call        getenv

	// *** Basic block 5

	sd          a0, -56(s0)
	ld          t0, -56(s0)
	sub         t1, t0, x0
	snez        t1, t1
	beq         t0, x0, .InitLibrarySearchPath_label_135

	// *** Basic block 6

	ld          t0, -56(s0)
	sd          t0, -48(s0)
	ld          t0, -48(s0)
	lb          t0, 0(t0)
	snez        t1, t0
	beqz        t0, .InitLibrarySearchPath_label_134

	// *** Basic block 7

.InitLibrarySearchPath_label_59:
	ld          t0, -48(s0)
	sd          t0, -40(s0)
	ld          t1, -40(s0)
	lb          t1, 0(t1)
	addi        t2, t1, -58
	snez        t0, t2
	li          t2, 58		// 0x3a ASCII ':'
	beq         t1, t2, .InitLibrarySearchPath_label_74

	// *** Basic block 8

	ld          t1, -40(s0)
	lb          t1, 0(t1)
	snez        t0, t1

	// *** Basic block 9

.InitLibrarySearchPath_label_74:
	beqz        t0, .InitLibrarySearchPath_label_93

	// *** Basic block 10

.InitLibrarySearchPath_label_76:
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	ld          t1, -40(s0)
	lb          t1, 0(t1)
	addi        t2, t1, -58
	snez        t0, t2
	li          t2, 58		// 0x3a ASCII ':'
	beq         t1, t2, .InitLibrarySearchPath_label_91

	// *** Basic block 11

	ld          t1, -40(s0)
	lb          t1, 0(t1)
	snez        t0, t1

	// *** Basic block 12

.InitLibrarySearchPath_label_91:
	bnez        t0, .InitLibrarySearchPath_label_76

	// *** Basic block 13

.InitLibrarySearchPath_label_93:
	lla         a0, .str.9
	call        NewString

	// *** Basic block 14

	sd          a0, -32(s0)
	ld          a0, -32(s0)
	ld          a1, -48(s0)
	ld          t0, -40(s0)
	ld          t1, -48(s0)
	sub         t0, t0, t1
	mv          a2, t0
	call        StringAppendSegment

	// *** Basic block 15

	ld          t0, -24(s0)
	addi        a0, t0, 192
	ld          a1, -32(s0)
	call        VectorAppend

	// *** Basic block 16

	ld          t0, -40(s0)
	sd          t0, -48(s0)
	ld          t0, -48(s0)
	lb          t0, 0(t0)
	addi        t1, t0, -58
	seqz        t1, t1
	li          t1, 58		// 0x3a ASCII ':'
	bne         t0, t1, .InitLibrarySearchPath_label_129

	// *** Basic block 17

	ld          t0, -48(s0)
	addi        t0, t0, 1
	sd          t0, -48(s0)

	// *** Basic block 18

.InitLibrarySearchPath_label_129:
	ld          t0, -48(s0)
	lb          t0, 0(t0)
	snez        t1, t0
	bnez        t0, .InitLibrarySearchPath_label_59

	// *** Basic block 19

.InitLibrarySearchPath_label_134:

	// *** Basic block 20

.InitLibrarySearchPath_label_135:
	// Restored registers.
	ld s1, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InitLibrarySearchPath:
	.size InitLibrarySearchPath, .func_end_InitLibrarySearchPath-InitLibrarySearchPath

	.global LoaderInitFromFile
	.type LoaderInitFromFile, @function

LoaderInitFromFile:

	// *** Basic block 0

	.global NewELFReaderFile
	.global ELFReaderFileRead
	.global fprintf
	.global stderr
	.global StringInit
	.global strrchr
	.global StringAppendSegment
	.global StringAppend
	.global realpath
	.global DynamicLibraryRegistryInit
	.global VectorInit
	.global VectorAppend
	.global NewString
	.local InitLibrarySearchPath
	.global NewRegion
	.local FindDynamicSection
	.local LoadStaticSegments
	.local LoadDynamic
	.global memset
	.global LoaderNumErrors
	addi sp, sp, -16
	sd ra, 8(sp)
	sd s0, 0(sp)
	lui t0, 1
	addi t0, t0, 96
	sub sp, sp, t0
	addi t0, t0, 16
	add s0, sp, t0
	// Saved argument registers.
	sd a0, -24(s0)
	sd a3, -32(s0)
	sd a4, -40(s0)
	sd a2, -48(s0)
	sd a1, -56(s0)
	sd a5, -64(s0)
	// Local vars at offset -4184(s0)
	// Saved integer registers.
	sd s1, 16(sp)
	sd s2, 8(sp)
	// End of stack frame
	ld          t0, -24(s0)
	addi        t0, t0, 48
	ld          t1, -32(s0)
	sd          t1, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 56
	ld          t1, -40(s0)
	sd          t1, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 40
	lw          t1, -48(s0)
	sw          t1, 0(t0)
	ld          t0, -24(s0)
	addi        s1, t0, 144
	ld          a0, -56(s0)
	call        NewELFReaderFile

	// *** Basic block 1

	sd          a0, 0(s1)
	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          a0, 0(t0)
	mv          a2, x0
	mv          a1, x0
	call        ELFReaderFileRead

	// *** Basic block 2

	li          t0, -4096		// 0xfffffffffffff000
	add         s1, s0, t0
	sb          a0, -88(s1)
	lb          t0, -88(s1)
	not         t0, t0
	beqz        t0, .LoaderInitFromFile_label_96

	// *** Basic block 3

	mv          a0, x0

	// *** Basic block 4

.LoaderInitFromFile_label_93:
	// Restored registers.
	ld s1, 16(sp)
	ld s2, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.LoaderInitFromFile_label_96:
	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t0, 0(t0)
	addi        t0, t0, 0
	ld          t0, 0(t0)
	addi        t0, t0, 18
	lhu         t0, 0(t0)
	ld          t1, -24(s0)
	addi        t1, t1, 48
	ld          t1, 0(t1)
	addi        t1, t1, 0
	lw          t1, 0(t1)
	sub         t2, t0, t1
	snez        t2, t2
	beq         t0, t1, .LoaderInitFromFile_label_137

	// *** Basic block 6

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.10
	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t0, 0(t0)
	addi        t0, t0, 0
	ld          t0, 0(t0)
	addi        t0, t0, 18
	lhu         a2, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 48
	ld          t0, 0(t0)
	addi        t0, t0, 0
	lw          a3, 0(t0)
	call        fprintf

	// *** Basic block 7

	mv          a0, x0
	j           .LoaderInitFromFile_label_93

	// *** Basic block 8

.LoaderInitFromFile_label_137:
	ld          t0, -24(s0)
	addi        a0, t0, 64
	mv          a1, x0
	call        StringInit

	// *** Basic block 9

	ld          t0, -56(s0)
	addi        t0, t0, 16
	ld          a0, 0(t0)
	li          t0, 47		// 0x2f ASCII '/'
	mv          a1, t0
	call        strrchr

	// *** Basic block 10

	sd          a0, -80(s1)
	ld          t0, -80(s1)
	sub         t1, t0, x0
	snez        t1, t1
	beq         t0, x0, .LoaderInitFromFile_label_174

	// *** Basic block 11

	ld          t0, -24(s0)
	addi        a0, t0, 64
	ld          t0, -56(s0)
	addi        t0, t0, 16
	ld          a1, 0(t0)
	ld          t0, -80(s1)
	ld          t1, -56(s0)
	addi        t1, t1, 16
	ld          t1, 0(t1)
	sub         t0, t0, t1
	mv          a2, t0
	call        StringAppendSegment

	// *** Basic block 12

	j           .LoaderInitFromFile_label_182

	// *** Basic block 13

.LoaderInitFromFile_label_174:
	ld          t0, -24(s0)
	addi        a0, t0, 64
	lla         a1, .str.11
	call        StringAppend

	// *** Basic block 14

.LoaderInitFromFile_label_182:
	ld          t0, -24(s0)
	addi        t0, t0, 64
	addi        t0, t0, 16
	ld          a0, 0(t0)
	addi        a1, s1, -64
	call        realpath

	// *** Basic block 15

	sd          a0, -72(s1)
	ld          t0, -72(s1)
	sub         t1, t0, x0
	snez        t1, t1
	beq         t0, x0, .LoaderInitFromFile_label_205

	// *** Basic block 16

	ld          t0, -24(s0)
	addi        a0, t0, 104
	addi        a1, s1, -64
	call        StringInit

	// *** Basic block 17

	j           .LoaderInitFromFile_label_215

	// *** Basic block 18

.LoaderInitFromFile_label_205:
	ld          t0, -24(s0)
	addi        a0, t0, 104
	ld          t0, -24(s0)
	addi        t0, t0, 64
	addi        t0, t0, 16
	ld          a1, 0(t0)
	call        StringInit

	// *** Basic block 19

.LoaderInitFromFile_label_215:
	ld          t0, -24(s0)
	addi        a0, t0, 224
	call        DynamicLibraryRegistryInit

	// *** Basic block 20

	ld          t0, -24(s0)
	addi        a0, t0, 192
	call        VectorInit

	// *** Basic block 21

	ld          t0, -64(s0)
	sub         t1, t0, x0
	snez        t1, t1
	beq         t0, x0, .LoaderInitFromFile_label_238

	// *** Basic block 22

	ld          t0, -24(s0)
	addi        s2, t0, 192
	ld          a0, -64(s0)
	call        NewString

	// *** Basic block 23

	mv          a1, a0
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 24

.LoaderInitFromFile_label_238:
	ld          a0, -24(s0)
	call        InitLibrarySearchPath

	// *** Basic block 25

	ld          t0, -24(s0)
	addi        a0, t0, 0
	ld          t0, -56(s0)
	addi        t0, t0, 16
	ld          a1, 0(t0)
	call        StringInit

	// *** Basic block 26

	ld          t0, -24(s0)
	addi        a0, t0, 160
	call        VectorInit

	// *** Basic block 27

	ld          t0, -24(s0)
	addi        s2, t0, 160
	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t0, 0(t0)
	addi        t0, t0, 0
	ld          a0, 0(t0)
	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t0, 0(t0)
	addi        t0, t0, 48
	ld          a1, 0(t0)
	call        NewRegion

	// *** Basic block 28

	mv          a1, a0
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 29

	ld          t0, -24(s0)
	addi        t0, t0, 152
	ld          t1, -24(s0)
	addi        t1, t1, 144
	ld          t1, 0(t1)
	addi        t1, t1, 0
	ld          t1, 0(t1)
	addi        t1, t1, 24
	ld          t1, 0(t1)
	sd          t1, 0(t0)
	ld          t0, -24(s0)
	addi        s2, t0, 184
	ld          a0, -24(s0)
	call        FindDynamicSection

	// *** Basic block 30

	sd          a0, 0(s2)
	ld          t0, -24(s0)
	addi        t0, t0, 184
	ld          t0, 0(t0)
	sub         t1, t0, x0
	seqz        t1, t1
	bne         t0, x0, .LoaderInitFromFile_label_319

	// *** Basic block 31

	ld          t0, -24(s0)
	addi        t0, t0, 144
	ld          t0, 0(t0)
	addi        t0, t0, 0
	ld          t0, 0(t0)
	addi        t0, t0, 16
	lhu         t0, 0(t0)
	addi        t1, t0, -2
	snez        t1, t1
	li          t1, 2		// 0x2 ASCII \x2
	beq         t0, t1, .LoaderInitFromFile_label_311

	// *** Basic block 32

	mv          a0, x0
	j           .LoaderInitFromFile_label_93

	// *** Basic block 33

.LoaderInitFromFile_label_311:
	ld          a0, -24(s0)
	ld          a1, -56(s0)
	call        LoadStaticSegments

	// *** Basic block 34

	sb          a0, -88(s1)
	j           .LoaderInitFromFile_label_328

	// *** Basic block 35

.LoaderInitFromFile_label_319:
	ld          a0, -24(s0)
	lw          t0, -48(s0)
	andi        t0, t0, 2
	snez        a1, t0
	call        LoadDynamic

	// *** Basic block 36

	sb          a0, -88(s1)

	// *** Basic block 37

.LoaderInitFromFile_label_328:
	ld          t0, -24(s0)
	addi        a0, t0, 280
	li          t0, 24		// 0x18 ASCII \x18
	mv          a2, t0
	mv          a1, x0
	call        memset

	// *** Basic block 38

	lb          s2, -88(s1)
	beqz        s2, .LoaderInitFromFile_label_343

	// *** Basic block 39

	call        LoaderNumErrors

	// *** Basic block 40

	seqz        s2, a0

	// *** Basic block 41

.LoaderInitFromFile_label_343:
	mv          a0, s2
	j           .LoaderInitFromFile_label_93
.func_end_LoaderInitFromFile:
	.size LoaderInitFromFile, .func_end_LoaderInitFromFile-LoaderInitFromFile

	.global LoaderDestruct
	.type LoaderDestruct, @function

LoaderDestruct:

	// *** Basic block 0

	.global munmap
	.global DynamicLibraryRegistryInit
	.global VectorDestructWithContents
	.global RegionDestruct
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
	addi        t1, t1, 160
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoaderDestruct_label_60

	// *** Basic block 1

.LoaderDestruct_label_24:
	ld          t0, -24(s0)
	addi        t0, t0, 160
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
	ld          a0, 0(t0)
	ld          t0, -32(s0)
	addi        t0, t0, 8
	ld          a1, 0(t0)
	call        munmap

	// *** Basic block 2

.LoaderDestruct_label_48:
	ld          t0, -40(s0)
	addi        t0, t0, 1
	sd          t0, -40(s0)
	ld          t0, -40(s0)
	ld          t1, -24(s0)
	addi        t1, t1, 160
	addi        t1, t1, 8
	ld          t1, 0(t1)
	slt         t2, t0, t1
	bge         t0, t1, .LoaderDestruct_label_24

	// *** Basic block 3

.LoaderDestruct_label_60:
	ld          t0, -24(s0)
	addi        a0, t0, 224
	call        DynamicLibraryRegistryInit

	// *** Basic block 4

	ld          t0, -24(s0)
	addi        a0, t0, 160
	la          t0, RegionDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 5

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LoaderDestruct:
	.size LoaderDestruct, .func_end_LoaderDestruct-LoaderDestruct

.PCend:
	.data
kPrintSymbolTableOnStart:
	.type   kPrintSymbolTableOnStart,@object
	.global kPrintSymbolTableOnStart
	.size   kPrintSymbolTableOnStart,1
	.p2align  0
	.byte   1

load_addr:
	.type   load_addr,@object
	.local  load_addr
	.size   load_addr,8
	.p2align  3
	.long   0

	.type   num_errors,@object
	.local  num_errors
	.comm   num_errors,4,4

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "Loader error: %s\n"
	.type .str.1, @object
	.size .str.1, 18

.str.2:
	.asciz "Failed to open ELF file segment\n"
	.type .str.2, @object
	.size .str.2, 33

.str.3:
	.asciz "Failed to map in ELF segment: %s\n"
	.type .str.3, @object
	.size .str.3, 34

.str.4:
	.asciz "Failed to map in dynamic segment: %s\n"
	.type .str.4, @object
	.size .str.4, 38

.str.5:
	.asciz "Unable to find %s"
	.type .str.5, @object
	.size .str.5, 18

.str.6:
	.asciz "/usr/lib"
	.type .str.6, @object
	.size .str.6, 9

.str.7:
	.asciz "/lib"
	.type .str.7, @object
	.size .str.7, 5

.str.8:
	.asciz "LD_LIBRARY_PATH"
	.type .str.8, @object
	.size .str.8, 16

.str.9:
	.asciz "(null)"
	.type .str.9, @object
	.size .str.9, 1

.str.10:
	.asciz "Wrong architecture: got %d, need %d\n"
	.type .str.10, @object
	.size .str.10, 37

.str.11:
	.asciz "."
	.type .str.11, @object
	.size .str.11, 2

