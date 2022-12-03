	.file   "linker_dynamic.c"
	.text
	.option pic
.PCbegin:
	.global DynamicLinkerInit
	.type DynamicLinkerInit, @function

DynamicLinkerInit:

	// *** Basic block 0

	.global printf
	.global abort
	.global VectorInit
	.global DynamicLibraryRegistryInit
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
	ld          s2, 88(a1)
	beq         s2, x0, .DynamicLinkerInit_label_41

	// *** Basic block 1

	j           .DynamicLinkerInit_label_58

	// *** Basic block 2

.DynamicLinkerInit_label_41:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 28		// 0x1c ASCII \x1c
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.DynamicLinkerInit_label_58:
	ld          t0, 24(s2)
	mv          a0, s1
	jalr         x1, t0, 0

	// *** Basic block 5

	addi        a0, s1, 8
	call        VectorInit

	// *** Basic block 6

	addi        a0, s1, 32
	call        VectorInit

	// *** Basic block 7

	addi        a0, s1, 56
	call        VectorInit

	// *** Basic block 8

	addi        a0, s1, 80
	call        VectorInit

	// *** Basic block 9

	addi        t0, s1, 104
	addi        a0, t0, 8
	call        VectorInit

	// *** Basic block 10

	addi        a0, s1, 136
	call        VectorInit

	// *** Basic block 11

	addi        a0, s1, 160
	call        VectorInit

	// *** Basic block 12

	addi        a0, s1, 184
	call        VectorInit

	// *** Basic block 13

	addi        a0, s1, 208
	call        VectorInit

	// *** Basic block 14

	sd          x0, 232(s1)
	sd          x0, 272(s1)
	sd          x0, 256(s1)
	sd          x0, 264(s1)
	addi        a0, s1, 320
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DynamicLibraryRegistryInit
.func_end_DynamicLinkerInit:
	.size DynamicLinkerInit, .func_end_DynamicLinkerInit-DynamicLinkerInit

	.global NewDynamicLinker
	.type NewDynamicLinker, @function

NewDynamicLinker:

	// *** Basic block 0

	.global malloc
	.global DynamicLinkerInit
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
	li          a0, 376		// 0x178
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        DynamicLinkerInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.NewDynamicLinker_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewDynamicLinker:
	.size NewDynamicLinker, .func_end_NewDynamicLinker-NewDynamicLinker

	.global DynamicLinkerDestruct
	.type DynamicLinkerDestruct, @function

DynamicLinkerDestruct:

	// *** Basic block 0

	.global VectorDestruct
	.global VectorInit
	.global DynamicLibraryRegistryDestruct
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
	addi        a0, s1, 8
	call        VectorDestruct

	// *** Basic block 1

	addi        a0, s1, 32
	call        VectorDestruct

	// *** Basic block 2

	addi        a0, s1, 56
	call        VectorDestruct

	// *** Basic block 3

	addi        a0, s1, 80
	call        VectorDestruct

	// *** Basic block 4

	addi        t0, s1, 104
	addi        a0, t0, 8
	call        VectorDestruct

	// *** Basic block 5

	addi        a0, s1, 136
	call        VectorDestruct

	// *** Basic block 6

	addi        a0, s1, 160
	call        VectorDestruct

	// *** Basic block 7

	addi        a0, s1, 184
	call        VectorDestruct

	// *** Basic block 8

	addi        a0, s1, 208
	call        VectorInit

	// *** Basic block 9

	addi        a0, s1, 320
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DynamicLibraryRegistryDestruct
.func_end_DynamicLinkerDestruct:
	.size DynamicLinkerDestruct, .func_end_DynamicLinkerDestruct-DynamicLinkerDestruct

	.global DynamicLinkerDelete
	.type DynamicLinkerDelete, @function

DynamicLinkerDelete:

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
	.global DynamicLinkerDestruct
	.global free
	mv          s1, a0
	call        DynamicLinkerDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_DynamicLinkerDelete:
	.size DynamicLinkerDelete, .func_end_DynamicLinkerDelete-DynamicLinkerDelete

	.global DynamicLinkerInventSymbols
	.type DynamicLinkerInventSymbols, @function

DynamicLinkerInventSymbols:

	// *** Basic block 0

	.global LinkerInventSymbol
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
	lla         a1, .str.4
	li          s3, 8		// 0x8 ASCII \x8
	mv          a2, s3
	call        LinkerInventSymbol

	// *** Basic block 1

	sd          a0, 240(s1)
	lla         a1, .str.5
	mv          a2, s3
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LinkerInventSymbol
.func_end_DynamicLinkerInventSymbols:
	.size DynamicLinkerInventSymbols, .func_end_DynamicLinkerInventSymbols-DynamicLinkerInventSymbols

	.global DynamicLinkerDefineSymbols
	.type DynamicLinkerDefineSymbols, @function

DynamicLinkerDefineSymbols:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 536(a0)
	ld          t1, 240(t0)
	ld          t2, 264(t0)
	ld          t2, 96(t2)
	sd          t2, 80(t1)
	ld          t1, 240(t0)
	li          t2, 1		// 0x1 ASCII \x1
	sb          t2, 48(t1)
	ld          t1, 240(t0)
	sb          t2, 49(t1)
	ld          t1, 248(t0)
	ld          t3, 304(t0)
	ld          t3, 96(t3)
	sd          t3, 80(t1)
	ld          t1, 248(t0)
	sb          t2, 48(t1)
	ld          t0, 248(t0)
	sb          t2, 49(t0)
	ret         
.func_end_DynamicLinkerDefineSymbols:
	.size DynamicLinkerDefineSymbols, .func_end_DynamicLinkerDefineSymbols-DynamicLinkerDefineSymbols

	.local  GetDataGOTOffset
	.type GetDataGOTOffset, @function

GetDataGOTOffset:

	// *** Basic block 0

	.global VectorAppend
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
	lw          t0, 88(s1)
	li          t1, -1		// 0xffffffffffffffff
	bne         t0, t1, .GetDataGOTOffset_label_32

	// *** Basic block 1

	addi        a0, s2, 8
	mv          a1, s1
	call        VectorAppend

	// *** Basic block 2

	ld          t0, 8(a0)
	addi        t0, t0, -1
	sw          t0, 88(s1)

	// *** Basic block 3

.GetDataGOTOffset_label_32:
	lw          t0, 88(s1)
	mv          a0, t0

	// *** Basic block 4

.GetDataGOTOffset_label_37:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GetDataGOTOffset:
	.size GetDataGOTOffset, .func_end_GetDataGOTOffset-GetDataGOTOffset

	.local  GetFunctionGOTOffset
	.type GetFunctionGOTOffset, @function

GetFunctionGOTOffset:

	// *** Basic block 0

	.global VectorAppend
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
	lw          t0, 88(s1)
	li          t1, -1		// 0xffffffffffffffff
	bne         t0, t1, .GetFunctionGOTOffset_label_35

	// *** Basic block 1

	addi        a0, s2, 80
	mv          a1, s1
	call        VectorAppend

	// *** Basic block 2

	ld          t0, 8(a0)
	addi        t0, t0, -1
	lw          t1, 0(s2)
	add         t0, t0, t1
	sw          t0, 88(s1)

	// *** Basic block 3

.GetFunctionGOTOffset_label_35:
	lw          t0, 88(s1)
	mv          a0, t0

	// *** Basic block 4

.GetFunctionGOTOffset_label_40:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GetFunctionGOTOffset:
	.size GetFunctionGOTOffset, .func_end_GetFunctionGOTOffset-GetFunctionGOTOffset

	.local  GetPLTOffset
	.type GetPLTOffset, @function

GetPLTOffset:

	// *** Basic block 0

	.global VectorAppend
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
	lw          t0, 92(s1)
	li          t1, -1		// 0xffffffffffffffff
	bne         t0, t1, .GetPLTOffset_label_36

	// *** Basic block 1

	addi        t0, s2, 104
	addi        a0, t0, 8
	mv          a1, s1
	call        VectorAppend

	// *** Basic block 2

	ld          t0, 8(a0)
	addi        t0, t0, -1
	lw          t1, 104(s2)
	add         t0, t0, t1
	sw          t0, 92(s1)

	// *** Basic block 3

.GetPLTOffset_label_36:
	lw          t0, 92(s1)
	mv          a0, t0

	// *** Basic block 4

.GetPLTOffset_label_41:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GetPLTOffset:
	.size GetPLTOffset, .func_end_GetPLTOffset-GetPLTOffset

	.local  AddGOTEntry
	.type AddGOTEntry, @function

AddGOTEntry:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 88(t0)
	ld          t1, 48(t1)
	mv          t0, t1
	jr          t0
.func_end_AddGOTEntry:
	.size AddGOTEntry, .func_end_AddGOTEntry-AddGOTEntry

	.local  BuildGlobalOffsetTableContents
	.type BuildGlobalOffsetTableContents, @function

BuildGlobalOffsetTableContents:

	// *** Basic block 0

	.local AddGOTEntry
	.global BufferAddSpace
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 72(sp)
	sd s2, 64(sp)
	sd s3, 56(sp)
	sd s4, 48(sp)
	sd s5, 40(sp)
	sd s6, 32(sp)
	sd s7, 24(sp)
	sd s8, 16(sp)
	sd s9, 8(sp)
	sd s10, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	ld          s4, 536(s1)
	addi        s5, s4, 8
	mv          s6, x0
	ld          s7, 8(s5)
	bge         x0, s7, .BuildGlobalOffsetTableContents_label_60

	// *** Basic block 1

	ld          s5, 0(s5)

	// *** Basic block 2

.BuildGlobalOffsetTableContents_label_40:
	slli        t0, s6, 3
	add         t0, s5, t0
	ld          a1, 0(t0)
	addi        a3, s4, 136
	mv          a4, x0
	mv          a2, s2
	mv          a0, s1
	call        AddGOTEntry

	// *** Basic block 3

.BuildGlobalOffsetTableContents_label_56:
	addi        s6, s6, 1
	bge         s6, s7, .BuildGlobalOffsetTableContents_label_40

	// *** Basic block 4

.BuildGlobalOffsetTableContents_label_60:
	addi        s5, s4, 32
	mv          s7, x0
	ld          s8, 8(s5)
	bge         x0, s8, .BuildGlobalOffsetTableContents_label_89

	// *** Basic block 5

	ld          s5, 0(s5)

	// *** Basic block 6

.BuildGlobalOffsetTableContents_label_70:
	slli        t0, s7, 3
	add         t0, s5, t0
	ld          a1, 0(t0)
	addi        a3, s4, 136
	li          t0, 2		// 0x2 ASCII \x2
	mv          a4, t0
	mv          a2, s2
	mv          a0, s1
	call        AddGOTEntry

	// *** Basic block 7

.BuildGlobalOffsetTableContents_label_85:
	addi        s7, s7, 1
	bge         s7, s8, .BuildGlobalOffsetTableContents_label_70

	// *** Basic block 8

.BuildGlobalOffsetTableContents_label_89:
	addi        s5, s4, 56
	mv          s8, x0
	ld          s9, 8(s5)
	bge         x0, s9, .BuildGlobalOffsetTableContents_label_131

	// *** Basic block 9

	ld          s5, 0(s5)

	// *** Basic block 10

.BuildGlobalOffsetTableContents_label_99:
	slli        t0, s8, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	addi        a3, s4, 136
	li          t0, 3		// 0x3 ASCII \x3
	mv          a4, t0
	mv          a2, s2
	mv          a1, s5
	mv          a0, s1
	call        AddGOTEntry

	// *** Basic block 11

	addi        a3, s4, 136
	li          t0, 2		// 0x2 ASCII \x2
	mv          a4, t0
	mv          a2, s2
	mv          a1, s5
	mv          a0, s1
	call        AddGOTEntry

	// *** Basic block 12

.BuildGlobalOffsetTableContents_label_127:
	addi        s8, s8, 1
	bge         s8, s9, .BuildGlobalOffsetTableContents_label_99

	// *** Basic block 13

.BuildGlobalOffsetTableContents_label_131:
	addi        a0, s3, 8
	lw          t0, 0(s4)
	lw          t1, 4(s4)
	mul         a1, t0, t1
	call        BufferAddSpace

	// *** Basic block 14

	addi        s5, s4, 80
	mv          s9, x0
	ld          s10, 8(s5)
	bge         x0, s10, .BuildGlobalOffsetTableContents_label_170

	// *** Basic block 15

	ld          s5, 0(s5)

	// *** Basic block 16

.BuildGlobalOffsetTableContents_label_149:
	slli        t0, s9, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	addi        a3, s4, 160
	li          t0, 1		// 0x1 ASCII \x1
	mv          a4, t0
	mv          a2, s3
	mv          a1, s5
	mv          a0, s1
	call        AddGOTEntry

	// *** Basic block 17

.BuildGlobalOffsetTableContents_label_166:
	addi        s9, s9, 1
	bge         s9, s10, .BuildGlobalOffsetTableContents_label_149

	// *** Basic block 18

.BuildGlobalOffsetTableContents_label_170:
	// Restored registers.
	ld s1, 72(sp)
	ld s2, 64(sp)
	ld s3, 56(sp)
	ld s4, 48(sp)
	ld s5, 40(sp)
	ld s6, 32(sp)
	ld s7, 24(sp)
	ld s8, 16(sp)
	ld s9, 8(sp)
	ld s10, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BuildGlobalOffsetTableContents:
	.size BuildGlobalOffsetTableContents, .func_end_BuildGlobalOffsetTableContents-BuildGlobalOffsetTableContents

	.local  GetSectionContentsBuffer
	.type GetSectionContentsBuffer, @function

GetSectionContentsBuffer:

	// *** Basic block 0

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
	sd s2, 0(sp)
	// End of stack frame
	addi        t0, a0, 40
	ld          t0, 8(t0)
	li          s1, 1		// 0x1 ASCII \x1
	bne         t0, s1, .GetSectionContentsBuffer_label_38

	// *** Basic block 1

	j           .GetSectionContentsBuffer_label_56

	// *** Basic block 2

.GetSectionContentsBuffer_label_38:
	lla         a0, .str.6
	lla         a1, .str.7
	lla         a3, .str.8
	li          t0, 190		// 0xbe ASCII \xbe
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.GetSectionContentsBuffer_label_56:
	ld          t0, 40(a0)
	ld          s2, 0(t0)
	lw          t0, 0(s2)
	bne         t0, s1, .GetSectionContentsBuffer_label_66

	// *** Basic block 5

	j           .GetSectionContentsBuffer_label_81

	// *** Basic block 6

.GetSectionContentsBuffer_label_66:
	lla         a0, .str.9
	lla         a1, .str.10
	lla         a3, .str.11
	li          t0, 192		// 0xc0 ASCII \xc0
	mv          a2, t0
	call        printf

	// *** Basic block 7

	call        abort

	// *** Basic block 8

.GetSectionContentsBuffer_label_81:
	ld          t0, 8(s2)
	ld          s1, 104(t0)
	lw          t0, 0(s1)
	bnez        t0, .GetSectionContentsBuffer_label_90

	// *** Basic block 9

	j           .GetSectionContentsBuffer_label_105

	// *** Basic block 10

.GetSectionContentsBuffer_label_90:
	lla         a0, .str.12
	lla         a1, .str.13
	lla         a3, .str.14
	li          t0, 193		// 0xc1 ASCII \xc1
	mv          a2, t0
	call        printf

	// *** Basic block 11

	call        abort

	// *** Basic block 12

.GetSectionContentsBuffer_label_105:
	lw          t0, 0(s1)
	bnez        t0, .GetSectionContentsBuffer_label_111

	// *** Basic block 13

	j           .GetSectionContentsBuffer_label_126

	// *** Basic block 14

.GetSectionContentsBuffer_label_111:
	lla         a0, .str.15
	lla         a1, .str.16
	lla         a3, .str.17
	li          t0, 195		// 0xc3 ASCII \xc3
	mv          a2, t0
	call        printf

	// *** Basic block 15

	call        abort

	// *** Basic block 16

.GetSectionContentsBuffer_label_126:
	addi        t0, s1, 8
	mv          a0, t0

	// *** Basic block 17

.GetSectionContentsBuffer_label_130:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GetSectionContentsBuffer:
	.size GetSectionContentsBuffer, .func_end_GetSectionContentsBuffer-GetSectionContentsBuffer

	.global DynamicLinkerFixupGOT
	.type DynamicLinkerFixupGOT, @function

DynamicLinkerFixupGOT:

	// *** Basic block 0

	.local GetSectionContentsBuffer
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
	ld          s2, 536(s1)
	ld          a0, 264(s2)
	call        GetSectionContentsBuffer

	// *** Basic block 1

	ld          t0, 88(s1)
	ld          s3, 56(t0)
	mv          s4, a0
	ld          t0, 272(s2)
	ld          s5, 96(t0)
	addi        s6, s2, 80
	mv          s7, x0
	ld          s8, 8(s6)
	bge         x0, s8, .DynamicLinkerFixupGOT_label_72

	// *** Basic block 2

	ld          s1, 0(s6)
	addi        t0, s2, 104
	lw          s2, 4(t0)

	// *** Basic block 3

.DynamicLinkerFixupGOT_label_50:
	slli        t0, s7, 3
	add         t0, s1, t0
	ld          s1, 0(t0)
	mv          a3, s2
	mv          a2, s5
	mv          a1, s4
	mv          a0, s1
	jalr         x1, s3, 0

	// *** Basic block 4

.DynamicLinkerFixupGOT_label_68:
	addi        s7, s7, 1
	bge         s7, s8, .DynamicLinkerFixupGOT_label_50

	// *** Basic block 5

.DynamicLinkerFixupGOT_label_72:
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
.func_end_DynamicLinkerFixupGOT:
	.size DynamicLinkerFixupGOT, .func_end_DynamicLinkerFixupGOT-DynamicLinkerFixupGOT

	.local  AddPLTEntry
	.type AddPLTEntry, @function

AddPLTEntry:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 88(t0)
	ld          t1, 64(t1)
	mv          t0, t1
	jr          t0
.func_end_AddPLTEntry:
	.size AddPLTEntry, .func_end_AddPLTEntry-AddPLTEntry

	.local  BuildPLTResolveSymbolEntry
	.type BuildPLTResolveSymbolEntry, @function

BuildPLTResolveSymbolEntry:

	// *** Basic block 0

	.global BufferAddSpace
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          t2, 536(t0)
	addi        a0, t1, 8
	addi        t3, t2, 104
	lw          t2, 104(t2)
	lw          t3, 4(t3)
	mul         a1, t2, t3
	j           BufferAddSpace
.func_end_BuildPLTResolveSymbolEntry:
	.size BuildPLTResolveSymbolEntry, .func_end_BuildPLTResolveSymbolEntry-BuildPLTResolveSymbolEntry

	.local  BuildProcedureLinkageTableContents
	.type BuildProcedureLinkageTableContents, @function

BuildProcedureLinkageTableContents:

	// *** Basic block 0

	.local BuildPLTResolveSymbolEntry
	.local AddPLTEntry
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
	ld          s3, 536(s1)
	call        BuildPLTResolveSymbolEntry

	// *** Basic block 1

	mv          s4, x0
	addi        t0, s3, 104
	addi        t1, t0, 8
	ld          s3, 8(t1)
	bge         x0, s3, .BuildProcedureLinkageTableContents_label_50

	// *** Basic block 2

	ld          s5, 8(t0)

	// *** Basic block 3

.BuildProcedureLinkageTableContents_label_34:
	slli        t0, s4, 3
	add         t0, s5, t0
	ld          a1, 0(t0)
	mv          a2, s2
	mv          a0, s1
	call        AddPLTEntry

	// *** Basic block 4

.BuildProcedureLinkageTableContents_label_46:
	addi        s4, s4, 1
	bge         s4, s3, .BuildProcedureLinkageTableContents_label_34

	// *** Basic block 5

.BuildProcedureLinkageTableContents_label_50:
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
.func_end_BuildProcedureLinkageTableContents:
	.size BuildProcedureLinkageTableContents, .func_end_BuildProcedureLinkageTableContents-BuildProcedureLinkageTableContents

	.global DynamicLinkerFixupPLT
	.type DynamicLinkerFixupPLT, @function

DynamicLinkerFixupPLT:

	// *** Basic block 0

	.local GetSectionContentsBuffer
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 72(sp)
	sd s2, 64(sp)
	sd s3, 56(sp)
	sd s4, 48(sp)
	sd s5, 40(sp)
	sd s6, 32(sp)
	sd s7, 24(sp)
	sd s8, 16(sp)
	sd s9, 8(sp)
	// End of stack frame
	mv          s1, a0
	ld          s2, 536(s1)
	addi        s3, s2, 104
	mv          s4, s2
	ld          s5, 272(s2)
	mv          a0, s5
	call        GetSectionContentsBuffer

	// *** Basic block 1

	mv          s6, a0
	ld          t0, 264(s2)
	ld          s2, 96(t0)
	ld          s7, 96(s5)
	ld          s5, 88(s1)
	ld          t0, 72(s5)
	mv          a3, s7
	mv          a2, s2
	mv          a1, s6
	mv          a0, s3
	jalr         x1, t0, 0

	// *** Basic block 2

	ld          s5, 80(s5)
	mv          s8, x0
	addi        t0, s3, 8
	ld          s9, 8(t0)
	bge         x0, s9, .DynamicLinkerFixupPLT_label_92

	// *** Basic block 3

	ld          s1, 8(s3)

	// *** Basic block 4

.DynamicLinkerFixupPLT_label_67:
	slli        t0, s8, 3
	add         t0, s1, t0
	ld          s1, 0(t0)
	mv          a5, s7
	mv          a4, s2
	mv          a3, s6
	mv          a2, s1
	mv          a1, s4
	mv          a0, s3
	jalr         x1, s5, 0

	// *** Basic block 5

.DynamicLinkerFixupPLT_label_88:
	addi        s8, s8, 1
	bge         s8, s9, .DynamicLinkerFixupPLT_label_67

	// *** Basic block 6

.DynamicLinkerFixupPLT_label_92:
	// Restored registers.
	ld s1, 72(sp)
	ld s2, 64(sp)
	ld s3, 56(sp)
	ld s4, 48(sp)
	ld s5, 40(sp)
	ld s6, 32(sp)
	ld s7, 24(sp)
	ld s8, 16(sp)
	ld s9, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DynamicLinkerFixupPLT:
	.size DynamicLinkerFixupPLT, .func_end_DynamicLinkerFixupPLT-DynamicLinkerFixupPLT

	.local  ProcessPossibleDynamicRelocation
	.type ProcessPossibleDynamicRelocation, @function

ProcessPossibleDynamicRelocation:

	// *** Basic block 0

	.global ObjectFileFindSymbol
	.local GetDataGOTOffset
	.local GetFunctionGOTOffset
	.local GetPLTOffset
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
	mv          t0, a1
	mv          s1, a2
	mv          s2, a0
	ld          a1, 16(s1)
	mv          a0, t0
	call        ObjectFileFindSymbol

	// *** Basic block 1

	mv          s3, a0
	ld          t0, 88(s2)
	ld          t0, 32(t0)
	ld          a0, 536(s2)
	la          t1, GetPLTOffset
	mv          a5, t1
	la          t1, GetFunctionGOTOffset
	mv          a4, t1
	la          t1, GetDataGOTOffset
	mv          a3, t1
	mv          a2, s1
	mv          a1, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_ProcessPossibleDynamicRelocation:
	.size ProcessPossibleDynamicRelocation, .func_end_ProcessPossibleDynamicRelocation-ProcessPossibleDynamicRelocation

	.global DynamicLinkerGatherDynamicRelocations
	.type DynamicLinkerGatherDynamicRelocations, @function

DynamicLinkerGatherDynamicRelocations:

	// *** Basic block 0

	.local ProcessPossibleDynamicRelocation
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
	// End of stack frame
	mv          s1, a0
	mv          s2, x0
	addi        t0, s1, 40
	ld          s3, 8(t0)
	bge         x0, s3, .DynamicLinkerGatherDynamicRelocations_label_56

	// *** Basic block 1

	ld          t0, 40(s1)

	// *** Basic block 2

.DynamicLinkerGatherDynamicRelocations_label_21:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	mv          s5, x0
	addi        t0, s4, 136
	ld          s6, 8(t0)
	bge         x0, s6, .DynamicLinkerGatherDynamicRelocations_label_51

	// *** Basic block 3

	ld          s7, 136(s4)

	// *** Basic block 4

.DynamicLinkerGatherDynamicRelocations_label_35:
	slli        t0, s5, 3
	add         t0, s7, t0
	ld          a2, 0(t0)
	mv          a1, s4
	mv          a0, s1
	call        ProcessPossibleDynamicRelocation

	// *** Basic block 5

.DynamicLinkerGatherDynamicRelocations_label_47:
	addi        s5, s5, 1
	bge         s5, s6, .DynamicLinkerGatherDynamicRelocations_label_35

	// *** Basic block 6

.DynamicLinkerGatherDynamicRelocations_label_51:

	// *** Basic block 7

.DynamicLinkerGatherDynamicRelocations_label_52:
	addi        s2, s2, 1
	bge         s2, s3, .DynamicLinkerGatherDynamicRelocations_label_21

	// *** Basic block 8

.DynamicLinkerGatherDynamicRelocations_label_56:
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DynamicLinkerGatherDynamicRelocations:
	.size DynamicLinkerGatherDynamicRelocations, .func_end_DynamicLinkerGatherDynamicRelocations-DynamicLinkerGatherDynamicRelocations

	.local  AllocateDynamicRelocations
	.type AllocateDynamicRelocations, @function

AllocateDynamicRelocations:

	// *** Basic block 0

	.global BufferAddSpace
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          t2, 536(t0)
	addi        t3, t2, 136
	ld          t3, 8(t3)
	addi        t2, t2, 184
	ld          t2, 8(t2)
	add         t2, t3, t2
	slli        t3, t2, 3
	slli        t2, t2, 4
	add         t4, t3, t2
	addi        a0, t1, 8
	mv          a1, t4
	j           BufferAddSpace
.func_end_AllocateDynamicRelocations:
	.size AllocateDynamicRelocations, .func_end_AllocateDynamicRelocations-AllocateDynamicRelocations

	.local  AllocatePLTRelocations
	.type AllocatePLTRelocations, @function

AllocatePLTRelocations:

	// *** Basic block 0

	.global BufferAddSpace
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          t2, 536(t0)
	addi        a0, t1, 8
	addi        t2, t2, 160
	ld          t2, 8(t2)
	slli        t3, t2, 3
	slli        t2, t2, 4
	add         a1, t3, t2
	j           BufferAddSpace
.func_end_AllocatePLTRelocations:
	.size AllocatePLTRelocations, .func_end_AllocatePLTRelocations-AllocatePLTRelocations

	.global DynamicLinkerBuildDynamicRelocations
	.type DynamicLinkerBuildDynamicRelocations, @function

DynamicLinkerBuildDynamicRelocations:

	// *** Basic block 0

	.global ELFWriterInitRelocation
	.global printf
	.global abort
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 72(sp)
	sd s2, 64(sp)
	sd s3, 56(sp)
	sd s4, 48(sp)
	sd s5, 40(sp)
	sd s6, 32(sp)
	sd s7, 24(sp)
	sd s8, 16(sp)
	sd s9, 8(sp)
	// End of stack frame
	ld          s1, 536(a0)
	ld          t0, 280(s1)
	ld          t0, 40(t0)
	ld          t1, 0(t0)
	ld          t0, 8(t1)
	ld          t0, 104(t0)
	addi        s2, t0, 8
	ld          t0, 256(s1)
	ld          t0, 40(t0)
	ld          t1, 0(t0)
	ld          s3, 8(t1)
	mv          s4, x0
	mv          s5, x0
	addi        t0, s1, 184
	ld          s6, 8(t0)
	bge         x0, s6, .DynamicLinkerBuildDynamicRelocations_label_111

	// *** Basic block 1

	ld          s7, 184(s1)
	ld          s3, 128(s3)

	// *** Basic block 2

.DynamicLinkerBuildDynamicRelocations_label_69:
	slli        t0, s5, 3
	add         t0, s7, t0
	ld          s7, 0(t0)
	ld          t0, 0(s2)
	slli        t1, s4, 3
	slli        t2, s4, 4
	add         t1, t1, t2
	add         s8, t0, t1
	ld          t0, 64(s7)
	ld          t1, 48(s7)
	ld          t1, 56(t1)
	add         s9, t0, t1
	lw          a4, 56(s7)
	mv          a3, x0
	mv          a2, x0
	mv          a1, s9
	mv          a0, s8
	call        ELFWriterInitRelocation

	// *** Basic block 3

	addi        s4, s4, 1

	// *** Basic block 4

.DynamicLinkerBuildDynamicRelocations_label_107:
	addi        s5, s5, 1
	bge         s5, s6, .DynamicLinkerBuildDynamicRelocations_label_69

	// *** Basic block 5

.DynamicLinkerBuildDynamicRelocations_label_111:
	mv          s3, x0
	addi        t0, s1, 136
	ld          s6, 8(t0)
	bge         x0, s6, .DynamicLinkerBuildDynamicRelocations_label_185

	// *** Basic block 6

	ld          t0, 136(s1)

	// *** Basic block 7

.DynamicLinkerBuildDynamicRelocations_label_120:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s1, 0(t0)
	ld          t0, 0(s2)
	slli        t1, s4, 3
	slli        t2, s4, 4
	add         t1, t1, t2
	add         s2, t0, t1
	ld          s7, 40(s1)
	lw          s8, 100(s7)
	li          t0, -1		// 0xffffffffffffffff
	bne         s8, t0, .DynamicLinkerBuildDynamicRelocations_label_143

	// *** Basic block 8

	lw          s8, 96(s7)
	j           .DynamicLinkerBuildDynamicRelocations_label_144

	// *** Basic block 9

.DynamicLinkerBuildDynamicRelocations_label_143:

	// *** Basic block 10

.DynamicLinkerBuildDynamicRelocations_label_144:
	beq         s8, t0, .DynamicLinkerBuildDynamicRelocations_label_150

	// *** Basic block 11

	j           .DynamicLinkerBuildDynamicRelocations_label_165

	// *** Basic block 12

.DynamicLinkerBuildDynamicRelocations_label_150:
	lla         a0, .str.18
	lla         a1, .str.19
	lla         a3, .str.20
	li          t0, 367		// 0x16f
	mv          a2, t0
	call        printf

	// *** Basic block 13

	call        abort

	// *** Basic block 14

.DynamicLinkerBuildDynamicRelocations_label_165:
	ld          t0, 64(s1)
	add         a1, t0, s3
	lw          a4, 56(s1)
	mv          a3, x0
	mv          a2, s8
	mv          a0, s2
	call        ELFWriterInitRelocation

	// *** Basic block 15

	addi        s4, s4, 1

	// *** Basic block 16

.DynamicLinkerBuildDynamicRelocations_label_181:
	addi        s3, s3, 1
	bge         s3, s6, .DynamicLinkerBuildDynamicRelocations_label_120

	// *** Basic block 17

.DynamicLinkerBuildDynamicRelocations_label_185:
	// Restored registers.
	ld s1, 72(sp)
	ld s2, 64(sp)
	ld s3, 56(sp)
	ld s4, 48(sp)
	ld s5, 40(sp)
	ld s6, 32(sp)
	ld s7, 24(sp)
	ld s8, 16(sp)
	ld s9, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DynamicLinkerBuildDynamicRelocations:
	.size DynamicLinkerBuildDynamicRelocations, .func_end_DynamicLinkerBuildDynamicRelocations-DynamicLinkerBuildDynamicRelocations

	.global DynamicLinkerBuildPLTRelocations
	.type DynamicLinkerBuildPLTRelocations, @function

DynamicLinkerBuildPLTRelocations:

	// *** Basic block 0

	.global ELFWriterInitRelocation
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
	// End of stack frame
	ld          t0, 536(a0)
	ld          t1, 288(t0)
	ld          t1, 40(t1)
	ld          t2, 0(t1)
	ld          t1, 8(t2)
	ld          t1, 104(t1)
	addi        t2, t1, 8
	ld          t1, 264(t0)
	ld          t1, 40(t1)
	ld          t3, 0(t1)
	ld          t1, 8(t3)
	mv          s1, x0
	addi        t3, t0, 160
	ld          s2, 8(t3)
	bge         x0, s2, .DynamicLinkerBuildPLTRelocations_label_108

	// *** Basic block 1

	ld          t0, 160(t0)
	ld          s3, 128(t1)

	// *** Basic block 2

.DynamicLinkerBuildPLTRelocations_label_59:
	slli        t1, s1, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	ld          t0, 0(t2)
	slli        t1, s1, 3
	slli        t2, s1, 4
	add         t1, t1, t2
	add         s5, t0, t1
	ld          s6, 40(s4)
	lw          s7, 100(s6)
	li          t0, -1		// 0xffffffffffffffff
	bne         s7, t0, .DynamicLinkerBuildPLTRelocations_label_84

	// *** Basic block 3

	lw          s7, 96(s6)
	j           .DynamicLinkerBuildPLTRelocations_label_85

	// *** Basic block 4

.DynamicLinkerBuildPLTRelocations_label_84:

	// *** Basic block 5

.DynamicLinkerBuildPLTRelocations_label_85:
	ld          t0, 64(s4)
	add         a1, t0, s3
	lw          a4, 56(s4)
	mv          a3, x0
	mv          a2, s7
	mv          a0, s5
	call        ELFWriterInitRelocation

	// *** Basic block 6

.DynamicLinkerBuildPLTRelocations_label_104:
	addi        s1, s1, 1
	bge         s1, s2, .DynamicLinkerBuildPLTRelocations_label_59

	// *** Basic block 7

.DynamicLinkerBuildPLTRelocations_label_108:
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DynamicLinkerBuildPLTRelocations:
	.size DynamicLinkerBuildPLTRelocations, .func_end_DynamicLinkerBuildPLTRelocations-DynamicLinkerBuildPLTRelocations

	.local  NewELFSection
	.type NewELFSection, @function

NewELFSection:

	// *** Basic block 0

	.global calloc
	.global StringInit
	.global NewVector
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
	mv          s4, a3
	mv          s5, a4
	li          a1, 1		// 0x1 ASCII \x1
	li          a0, 152		// 0x98 ASCII \x98
	call        calloc

	// *** Basic block 1

	mv          s6, a0
	mv          a1, s1
	mv          a0, s6
	call        StringInit

	// *** Basic block 2

	addi        t0, s6, 40
	sw          s2, 4(t0)
	addi        t0, s6, 40
	sd          s3, 8(t0)
	addi        t0, s6, 40
	sd          s4, 48(t0)
	addi        t0, s6, 40
	sd          x0, 16(t0)
	sd          s5, 104(s6)
	call        NewVector

	// *** Basic block 3

	sd          a0, 120(s6)
	sw          x0, 112(s6)
	sd          x0, 128(s6)
	sd          x0, 144(s6)
	mv          a0, s6

	// *** Basic block 4

.NewELFSection_label_72:
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
.func_end_NewELFSection:
	.size NewELFSection, .func_end_NewELFSection-NewELFSection

	.local  NewDynamicLinkerGroup
	.type NewDynamicLinkerGroup, @function

NewDynamicLinkerGroup:

	// *** Basic block 0

	.global NewSectionGroup
	.global VectorAppend
	.global NewGroupedSection
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
	mv          s2, a2
	mv          s3, a0
	addi        t0, s1, 40
	lwu         a1, 4(t0)
	ld          a2, 8(t0)
	ld          a3, 48(t0)
	mv          a0, s1
	call        NewSectionGroup

	// *** Basic block 1

	mv          s4, a0
	addi        s5, s4, 40
	mv          a0, s1
	call        NewGroupedSection

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s5
	call        VectorAppend

	// *** Basic block 3

	sd          s2, 64(s4)
	mv          a1, s4
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 4

	addi        a0, s3, 248
	mv          a1, s4
	call        VectorAppend

	// *** Basic block 5

	mv          a0, s4

	// *** Basic block 6

.NewDynamicLinkerGroup_label_61:
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
.func_end_NewDynamicLinkerGroup:
	.size NewDynamicLinkerGroup, .func_end_NewDynamicLinkerGroup-NewDynamicLinkerGroup

	.local  AddGlobalOffsetTable
	.type AddGlobalOffsetTable, @function

AddGlobalOffsetTable:

	// *** Basic block 0

	.global NewELFWriterSectionContents
	.local NewELFSection
	.local BuildGlobalOffsetTableContents
	.global BufferAlignLength
	.local NewDynamicLinkerGroup
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
	mv          s2, a1
	mv          a0, x0
	call        NewELFWriterSectionContents

	// *** Basic block 1

	mv          s3, a0
	lla         a0, .str.21
	mv          a4, s3
	li          s4, 8		// 0x8 ASCII \x8
	mv          a3, s4
	li          s5, 3		// 0x3 ASCII \x3
	mv          a2, s5
	li          s6, 1		// 0x1 ASCII \x1
	mv          a1, s6
	call        NewELFSection

	// *** Basic block 2

	mv          s7, a0
	mv          a0, x0
	call        NewELFWriterSectionContents

	// *** Basic block 3

	mv          s8, a0
	lla         a0, .str.22
	mv          a4, s8
	mv          a3, s4
	mv          a2, s5
	mv          a1, s6
	call        NewELFSection

	// *** Basic block 4

	mv          s5, a0
	addi        t0, s7, 40
	ld          t1, 536(s1)
	lw          t1, 4(t1)
	sd          t1, 56(t0)
	addi        t0, s5, 40
	sd          t1, 56(t0)
	mv          a2, s8
	mv          a1, s3
	mv          a0, s1
	call        BuildGlobalOffsetTableContents

	// *** Basic block 5

	addi        a0, s3, 8
	mv          a1, s4
	call        BufferAlignLength

	// *** Basic block 6

	addi        a0, s8, 8
	mv          a1, s4
	call        BufferAlignLength

	// *** Basic block 7

	addi        a2, s1, 304
	mv          a1, s7
	mv          a0, s1
	call        NewDynamicLinkerGroup

	// *** Basic block 8

	sd          a0, 0(s2)
	addi        a2, s1, 304
	mv          a1, s5
	mv          a0, s1
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
	j           NewDynamicLinkerGroup
.func_end_AddGlobalOffsetTable:
	.size AddGlobalOffsetTable, .func_end_AddGlobalOffsetTable-AddGlobalOffsetTable

	.local  AddProcedureLinkageTable
	.type AddProcedureLinkageTable, @function

AddProcedureLinkageTable:

	// *** Basic block 0

	.global NewELFWriterSectionContents
	.local NewELFSection
	.local BuildProcedureLinkageTableContents
	.global BufferAlignLength
	.local NewDynamicLinkerGroup
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
	mv          a0, x0
	call        NewELFWriterSectionContents

	// *** Basic block 1

	mv          s2, a0
	lla         a0, .str.23
	mv          a4, s2
	li          s3, 8		// 0x8 ASCII \x8
	mv          a3, s3
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        NewELFSection

	// *** Basic block 2

	mv          s4, a0
	addi        t0, s4, 40
	ld          t1, 536(s1)
	addi        t1, t1, 104
	lw          t1, 4(t1)
	sd          t1, 56(t0)
	mv          a1, s2
	mv          a0, s1
	call        BuildProcedureLinkageTableContents

	// *** Basic block 3

	addi        a0, s2, 8
	mv          a1, s3
	call        BufferAlignLength

	// *** Basic block 4

	addi        a2, s1, 272
	mv          a1, s4
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewDynamicLinkerGroup
.func_end_AddProcedureLinkageTable:
	.size AddProcedureLinkageTable, .func_end_AddProcedureLinkageTable-AddProcedureLinkageTable

	.local  AddDynamicRelocationsSection
	.type AddDynamicRelocationsSection, @function

AddDynamicRelocationsSection:

	// *** Basic block 0

	.global NewELFWriterSectionContents
	.local NewELFSection
	.local AllocateDynamicRelocations
	.local NewDynamicLinkerGroup
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
	mv          a0, x0
	call        NewELFWriterSectionContents

	// *** Basic block 1

	mv          s2, a0
	lla         a0, .str.24
	mv          a4, s2
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	call        NewELFSection

	// *** Basic block 2

	mv          s3, a0
	addi        t0, s3, 40
	li          t1, 24		// 0x18 ASCII \x18
	sd          t1, 56(t0)
	mv          a1, s2
	mv          a0, s1
	call        AllocateDynamicRelocations

	// *** Basic block 3

	addi        a2, s1, 272
	mv          a1, s3
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewDynamicLinkerGroup
.func_end_AddDynamicRelocationsSection:
	.size AddDynamicRelocationsSection, .func_end_AddDynamicRelocationsSection-AddDynamicRelocationsSection

	.local  AddPLTRelocationsSection
	.type AddPLTRelocationsSection, @function

AddPLTRelocationsSection:

	// *** Basic block 0

	.global NewELFWriterSectionContents
	.local NewELFSection
	.local AllocatePLTRelocations
	.local NewDynamicLinkerGroup
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
	mv          a0, x0
	call        NewELFWriterSectionContents

	// *** Basic block 1

	mv          s2, a0
	lla         a0, .str.25
	mv          a4, s2
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	call        NewELFSection

	// *** Basic block 2

	mv          s3, a0
	addi        t0, s3, 40
	li          t1, 24		// 0x18 ASCII \x18
	sd          t1, 56(t0)
	mv          a1, s2
	mv          a0, s1
	call        AllocatePLTRelocations

	// *** Basic block 3

	addi        a2, s1, 272
	mv          a1, s3
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewDynamicLinkerGroup
.func_end_AddPLTRelocationsSection:
	.size AddPLTRelocationsSection, .func_end_AddPLTRelocationsSection-AddPLTRelocationsSection

	.local  AddDataRelocationsSection
	.type AddDataRelocationsSection, @function

AddDataRelocationsSection:

	// *** Basic block 0

	.global NewELFWriterSectionContents
	.local NewELFSection
	.global AllocateDataRelocations
	.local NewDynamicLinkerGroup
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
	mv          a0, x0
	call        NewELFWriterSectionContents

	// *** Basic block 1

	mv          s2, a0
	lla         a0, .str.26
	mv          a4, s2
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	call        NewELFSection

	// *** Basic block 2

	mv          s3, a0
	addi        t0, s3, 40
	li          t1, 24		// 0x18 ASCII \x18
	sd          t1, 56(t0)
	mv          a1, s2
	mv          a0, s1
	call        AllocateDataRelocations

	// *** Basic block 3

	addi        a2, s1, 272
	mv          a1, s3
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewDynamicLinkerGroup
.func_end_AddDataRelocationsSection:
	.size AddDataRelocationsSection, .func_end_AddDataRelocationsSection-AddDataRelocationsSection

	.local  AddInterpreterSection
	.type AddInterpreterSection, @function

AddInterpreterSection:

	// *** Basic block 0

	.global NewELFWriterSectionContents
	.local NewELFSection
	.global BufferAppend
	.local NewDynamicLinkerGroup
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
	mv          a0, x0
	call        NewELFWriterSectionContents

	// *** Basic block 1

	mv          s2, a0
	lla         a0, .str.27
	mv          a4, s2
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	call        NewELFSection

	// *** Basic block 2

	mv          s3, a0
	addi        a0, s2, 8
	addi        t0, s1, 120
	ld          a1, 16(t0)
	ld          t0, 24(t0)
	addi        a2, t0, 1
	call        BufferAppend

	// *** Basic block 3

	addi        a2, s1, 368
	mv          a1, s3
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewDynamicLinkerGroup
.func_end_AddInterpreterSection:
	.size AddInterpreterSection, .func_end_AddInterpreterSection-AddInterpreterSection

	.local  WriteDynamicSectionEntryWithValue
	.type WriteDynamicSectionEntryWithValue, @function

WriteDynamicSectionEntryWithValue:

	// *** Basic block 0

	.global BufferAppend
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -32(s0)
	// End of stack frame
	mv          t0, a1
	mv          t1, a2
	sd          t0, -32(s0)
	addi        t2, s0, -32
	sd          t1, 8(t2)
	addi        a1, s0, -32
	li          a2, 16		// 0x10 ASCII \x10
	call        BufferAppend

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_WriteDynamicSectionEntryWithValue:
	.size WriteDynamicSectionEntryWithValue, .func_end_WriteDynamicSectionEntryWithValue-WriteDynamicSectionEntryWithValue

	.local  CreateDynamicSectionContents
	.type CreateDynamicSectionContents, @function

CreateDynamicSectionContents:

	// *** Basic block 0

	.local WriteDynamicSectionEntryWithValue
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
	mv          s3, x0
	ld          s4, 536(s1)
	addi        t0, s4, 208
	ld          s5, 8(t0)
	bge         x0, s5, .CreateDynamicSectionContents_label_64

	// *** Basic block 1

	ld          s6, 208(s4)

	// *** Basic block 2

.CreateDynamicSectionContents_label_45:
	slli        t0, s3, 3
	add         t0, s6, t0
	ld          s6, 0(t0)
	mv          a2, s6
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 3

.CreateDynamicSectionContents_label_60:
	addi        s3, s3, 1
	bge         s3, s5, .CreateDynamicSectionContents_label_45

	// *** Basic block 4

.CreateDynamicSectionContents_label_64:
	ld          s4, 232(s4)
	beqz        s4, .CreateDynamicSectionContents_label_77

	// *** Basic block 5

	mv          a2, s4
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 6

.CreateDynamicSectionContents_label_77:
	lw          s4, 568(s1)
	li          t0, -1		// 0xffffffffffffffff
	beq         s4, t0, .CreateDynamicSectionContents_label_93

	// *** Basic block 7

	mv          a2, s4
	li          t0, 14		// 0xe ASCII \xe
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 8

.CreateDynamicSectionContents_label_93:
	mv          a2, x0
	li          t0, 1879047925		// 0x6ffffef5
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 9

	mv          a2, x0
	li          t0, 5		// 0x5 ASCII \x5
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 10

	mv          a2, x0
	li          t0, 10		// 0xa ASCII \xa
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 11

	mv          a2, x0
	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 12

	li          s1, 24		// 0x18 ASCII \x18
	mv          a2, s1
	li          t0, 11		// 0xb ASCII \xb
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 13

	mv          a2, x0
	li          s4, 7		// 0x7 ASCII \x7
	mv          a1, s4
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 14

	mv          a2, x0
	li          s5, 8		// 0x8 ASCII \x8
	mv          a1, s5
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 15

	mv          a2, x0
	li          t0, 1879048185		// 0x6ffffff9
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 16

	mv          a2, s1
	li          t0, 9		// 0x9 ASCII \x9
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 17

	mv          a2, x0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 18

	mv          a2, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 19

	mv          a2, s4
	li          t0, 20		// 0x14 ASCII \x14
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 20

	mv          a2, x0
	li          t0, 23		// 0x17 ASCII \x17
	mv          a1, t0
	mv          a0, s2
	call        WriteDynamicSectionEntryWithValue

	// *** Basic block 21

	mv          a2, x0
	mv          a1, x0
	mv          a0, s2
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
	j           WriteDynamicSectionEntryWithValue
.func_end_CreateDynamicSectionContents:
	.size CreateDynamicSectionContents, .func_end_CreateDynamicSectionContents-CreateDynamicSectionContents

	.local  FixupDynamicSectionEntryValue
	.type FixupDynamicSectionEntryValue, @function

FixupDynamicSectionEntryValue:

	// *** Basic block 0

	.global printf
	.global abort
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
	mv          s4, x0
	ld          t0, 8(s1)
	bge         x0, t0, .FixupDynamicSectionEntryValue_label_51

	// *** Basic block 1

.FixupDynamicSectionEntryValue_label_29:
	ld          t0, 0(s1)
	add         s5, t0, s4
	ld          t0, 0(s5)
	beqz        t0, .FixupDynamicSectionEntryValue_label_51

	// *** Basic block 2

.FixupDynamicSectionEntryValue_label_36:
	bne         t0, s2, .FixupDynamicSectionEntryValue_label_45

	// *** Basic block 3

	sd          s3, 8(s5)

	// *** Basic block 4

.FixupDynamicSectionEntryValue_label_42:
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

	// *** Basic block 5

.FixupDynamicSectionEntryValue_label_45:
	addi        s4, s4, 16
	ld          t0, 8(s1)
	blt         s4, t0, .FixupDynamicSectionEntryValue_label_29

	// *** Basic block 6

.FixupDynamicSectionEntryValue_label_51:
	lla         a0, .str.28
	lla         a1, .str.29
	lla         a3, .str.30
	li          t0, 664		// 0x298
	mv          a2, t0
	call        printf

	// *** Basic block 7

	call        abort

	// *** Basic block 8

	j           .FixupDynamicSectionEntryValue_label_42
.func_end_FixupDynamicSectionEntryValue:
	.size FixupDynamicSectionEntryValue, .func_end_FixupDynamicSectionEntryValue-FixupDynamicSectionEntryValue

	.global DynamicLinkerFixupDynamicSectionContents
	.type DynamicLinkerFixupDynamicSectionContents, @function

DynamicLinkerFixupDynamicSectionContents:

	// *** Basic block 0

	.global ELFWriterFindSection
	.global printf
	.global abort
	.local FixupDynamicSectionEntryValue
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
	lla         a1, .str.31
	call        ELFWriterFindSection

	// *** Basic block 1

	mv          s2, a0
	beq         s2, x0, .DynamicLinkerFixupDynamicSectionContents_label_73

	// *** Basic block 2

	j           .DynamicLinkerFixupDynamicSectionContents_label_90

	// *** Basic block 3

.DynamicLinkerFixupDynamicSectionContents_label_73:
	lla         a0, .str.32
	lla         a1, .str.33
	lla         a3, .str.34
	li          t0, 671		// 0x29f
	mv          a2, t0
	call        printf

	// *** Basic block 4

	call        abort

	// *** Basic block 5

.DynamicLinkerFixupDynamicSectionContents_label_90:
	ld          t0, 104(s2)
	ld          t0, 8(t0)
	ld          s3, 0(t0)
	addi        s4, s3, 8
	lla         a1, .str.35
	mv          a0, s1
	call        ELFWriterFindSection

	// *** Basic block 6

	mv          s3, a0
	beq         s3, x0, .DynamicLinkerFixupDynamicSectionContents_label_111

	// *** Basic block 7

	j           .DynamicLinkerFixupDynamicSectionContents_label_126

	// *** Basic block 8

.DynamicLinkerFixupDynamicSectionContents_label_111:
	lla         a0, .str.36
	lla         a1, .str.37
	lla         a3, .str.38
	li          t0, 681		// 0x2a9
	mv          a2, t0
	call        printf

	// *** Basic block 9

	call        abort

	// *** Basic block 10

.DynamicLinkerFixupDynamicSectionContents_label_126:
	addi        s2, s3, 40
	ld          a2, 16(s2)
	li          t0, 5		// 0x5 ASCII \x5
	mv          a1, t0
	mv          a0, s4
	call        FixupDynamicSectionEntryValue

	// *** Basic block 11

	ld          a2, 32(s2)
	li          t0, 10		// 0xa ASCII \xa
	mv          a1, t0
	mv          a0, s4
	call        FixupDynamicSectionEntryValue

	// *** Basic block 12

	lla         a1, .str.39
	mv          a0, s1
	call        ELFWriterFindSection

	// *** Basic block 13

	mv          s2, a0
	beq         s2, x0, .DynamicLinkerFixupDynamicSectionContents_label_158

	// *** Basic block 14

	j           .DynamicLinkerFixupDynamicSectionContents_label_173

	// *** Basic block 15

.DynamicLinkerFixupDynamicSectionContents_label_158:
	lla         a0, .str.40
	lla         a1, .str.41
	lla         a3, .str.42
	li          t0, 687		// 0x2af
	mv          a2, t0
	call        printf

	// *** Basic block 16

	call        abort

	// *** Basic block 17

.DynamicLinkerFixupDynamicSectionContents_label_173:
	addi        t0, s2, 40
	ld          a2, 16(t0)
	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	mv          a0, s4
	call        FixupDynamicSectionEntryValue

	// *** Basic block 18

	lla         a1, .str.43
	mv          a0, s1
	call        ELFWriterFindSection

	// *** Basic block 19

	mv          s3, a0
	beq         s3, x0, .DynamicLinkerFixupDynamicSectionContents_label_196

	// *** Basic block 20

	j           .DynamicLinkerFixupDynamicSectionContents_label_211

	// *** Basic block 21

.DynamicLinkerFixupDynamicSectionContents_label_196:
	lla         a0, .str.44
	lla         a1, .str.45
	lla         a3, .str.46
	li          t0, 692		// 0x2b4
	mv          a2, t0
	call        printf

	// *** Basic block 22

	call        abort

	// *** Basic block 23

.DynamicLinkerFixupDynamicSectionContents_label_211:
	addi        s2, s3, 40
	ld          a2, 16(s2)
	li          t0, 7		// 0x7 ASCII \x7
	mv          a1, t0
	mv          a0, s4
	call        FixupDynamicSectionEntryValue

	// *** Basic block 24

	ld          s2, 32(s2)
	mv          a2, s2
	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	mv          a0, s4
	call        FixupDynamicSectionEntryValue

	// *** Basic block 25

	li          t0, 24		// 0x18 ASCII \x18
	div         a2, s2, t0
	li          t0, 1879048185		// 0x6ffffff9
	mv          a1, t0
	mv          a0, s4
	call        FixupDynamicSectionEntryValue

	// *** Basic block 26

	lla         a1, .str.47
	mv          a0, s1
	call        ELFWriterFindSection

	// *** Basic block 27

	mv          s2, a0
	beq         s2, x0, .DynamicLinkerFixupDynamicSectionContents_label_253

	// *** Basic block 28

	j           .DynamicLinkerFixupDynamicSectionContents_label_268

	// *** Basic block 29

.DynamicLinkerFixupDynamicSectionContents_label_253:
	lla         a0, .str.48
	lla         a1, .str.49
	lla         a3, .str.50
	li          t0, 700		// 0x2bc
	mv          a2, t0
	call        printf

	// *** Basic block 30

	call        abort

	// *** Basic block 31

.DynamicLinkerFixupDynamicSectionContents_label_268:
	addi        t0, s2, 40
	ld          a2, 16(t0)
	li          t0, 1879047925		// 0x6ffffef5
	mv          a1, t0
	mv          a0, s4
	call        FixupDynamicSectionEntryValue

	// *** Basic block 32

	lla         a1, .str.51
	mv          a0, s1
	call        ELFWriterFindSection

	// *** Basic block 33

	mv          s3, a0
	beq         s3, x0, .DynamicLinkerFixupDynamicSectionContents_label_291

	// *** Basic block 34

	j           .DynamicLinkerFixupDynamicSectionContents_label_306

	// *** Basic block 35

.DynamicLinkerFixupDynamicSectionContents_label_291:
	lla         a0, .str.52
	lla         a1, .str.53
	lla         a3, .str.54
	li          t0, 705		// 0x2c1
	mv          a2, t0
	call        printf

	// *** Basic block 36

	call        abort

	// *** Basic block 37

.DynamicLinkerFixupDynamicSectionContents_label_306:
	addi        t0, s3, 40
	ld          a2, 16(t0)
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s4
	call        FixupDynamicSectionEntryValue

	// *** Basic block 38

	lla         a1, .str.55
	mv          a0, s1
	call        ELFWriterFindSection

	// *** Basic block 39

	mv          s2, a0
	beq         s2, x0, .DynamicLinkerFixupDynamicSectionContents_label_329

	// *** Basic block 40

	j           .DynamicLinkerFixupDynamicSectionContents_label_344

	// *** Basic block 41

.DynamicLinkerFixupDynamicSectionContents_label_329:
	lla         a0, .str.56
	lla         a1, .str.57
	lla         a3, .str.58
	li          t0, 712		// 0x2c8
	mv          a2, t0
	call        printf

	// *** Basic block 42

	call        abort

	// *** Basic block 43

.DynamicLinkerFixupDynamicSectionContents_label_344:
	addi        s1, s2, 40
	ld          a2, 16(s1)
	li          t0, 23		// 0x17 ASCII \x17
	mv          a1, t0
	mv          a0, s4
	call        FixupDynamicSectionEntryValue

	// *** Basic block 44

	ld          a2, 32(s1)
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s4
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           FixupDynamicSectionEntryValue
.func_end_DynamicLinkerFixupDynamicSectionContents:
	.size DynamicLinkerFixupDynamicSectionContents, .func_end_DynamicLinkerFixupDynamicSectionContents-DynamicLinkerFixupDynamicSectionContents

	.local  AddDynamicSection
	.type AddDynamicSection, @function

AddDynamicSection:

	// *** Basic block 0

	.global NewELFWriterSectionContents
	.local NewELFSection
	.local CreateDynamicSectionContents
	.local NewDynamicLinkerGroup
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
	mv          a0, x0
	call        NewELFWriterSectionContents

	// *** Basic block 1

	mv          s2, a0
	lla         a0, .str.59
	mv          a4, s2
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	call        NewELFSection

	// *** Basic block 2

	mv          s3, a0
	ld          t0, 536(s1)
	sd          t0, 144(s3)
	addi        t0, s3, 40
	li          t1, 16		// 0x10 ASCII \x10
	sd          t1, 56(t0)
	addi        a1, s2, 8
	mv          a0, s1
	call        CreateDynamicSectionContents

	// *** Basic block 3

	addi        a2, s1, 336
	mv          a1, s3
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewDynamicLinkerGroup
.func_end_AddDynamicSection:
	.size AddDynamicSection, .func_end_AddDynamicSection-AddDynamicSection

	.local  CompareSymbolFixup
	.type CompareSymbolFixup, @function

CompareSymbolFixup:

	// *** Basic block 0

	.local num_gnu_buckets
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	lwu         t2, 4(t0)
	la          t3, num_gnu_buckets
	lw          t3, 0(t3)
	rem         t2, t2, t3
	lwu         t4, 4(t1)
	rem         t3, t4, t3
	sub         t2, t2, t3
	mv          a0, t2

	// *** Basic block 1

.CompareSymbolFixup_label_27:
	ret         
.func_end_CompareSymbolFixup:
	.size CompareSymbolFixup, .func_end_CompareSymbolFixup-CompareSymbolFixup

	.local  SortDynamicSymbolTable
	.type SortDynamicSymbolTable, @function

SortDynamicSymbolTable:

	// *** Basic block 0

	.local num_gnu_buckets
	.global printf
	.global abort
	.global qsort
	.local CompareSymbolFixup
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -40(s0)
	// Saved integer registers.
	sd s1, 48(sp)
	sd s2, 40(sp)
	sd s3, 32(sp)
	sd s4, 24(sp)
	sd s5, 16(sp)
	sd s6, 8(sp)
	// End of stack frame
	mv          s1, a0
	la          t0, num_gnu_buckets
	lw          t0, 0(t0)
	bge         x0, t0, .SortDynamicSymbolTable_label_29

	// *** Basic block 1

	j           .SortDynamicSymbolTable_label_47

	// *** Basic block 2

.SortDynamicSymbolTable_label_29:
	lla         a0, .str.60
	lla         a1, .str.61
	lla         a3, .str.62
	li          t0, 795		// 0x31b
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.SortDynamicSymbolTable_label_47:
	ld          t0, 8(s1)
	li          s2, 24		// 0x18 ASCII \x18
	div         s3, t0, s2
	ld          s4, 0(s1)
	li          s5, -1		// 0xffffffffffffffff
	li          s6, 1		// 0x1 ASCII \x1
	li          t0, 1		// 0x1 ASCII \x1
	bge         t0, s3, .SortDynamicSymbolTable_label_85

	// *** Basic block 5

.SortDynamicSymbolTable_label_63:
	slli        t0, s6, 3
	slli        t1, s6, 4
	add         t0, t0, t1
	add         s1, s4, t0
	ld          t0, 8(s1)
	ld          t0, 0(t0)
	lhu         t0, 6(t0)
	beqz        t0, .SortDynamicSymbolTable_label_80

	// *** Basic block 6

	mv          s5, s6
	j           .SortDynamicSymbolTable_label_85

	// *** Basic block 7

.SortDynamicSymbolTable_label_80:

	// *** Basic block 8

.SortDynamicSymbolTable_label_81:
	addi        s6, s6, 1
	bge         s6, s3, .SortDynamicSymbolTable_label_63

	// *** Basic block 9

.SortDynamicSymbolTable_label_85:
	li          t0, -1		// 0xffffffffffffffff
	bne         s5, t0, .SortDynamicSymbolTable_label_97

	// *** Basic block 10

	addi        t0, s3, -1
	mv          a0, t0

	// *** Basic block 11

.SortDynamicSymbolTable_label_94:
	// Restored registers.
	ld s1, 48(sp)
	ld s2, 40(sp)
	ld s3, 32(sp)
	ld s4, 24(sp)
	ld s5, 16(sp)
	ld s6, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 12

.SortDynamicSymbolTable_label_97:
	addi        s1, s5, 1
	bge         s1, s3, .SortDynamicSymbolTable_label_153

	// *** Basic block 13

.SortDynamicSymbolTable_label_102:
	slli        t0, s1, 3
	slli        t1, s1, 4
	add         t0, t0, t1
	add         s6, s4, t0
	ld          t1, 8(s6)
	ld          t1, 0(t1)
	lhu         t1, 6(t1)
	bnez        t1, .SortDynamicSymbolTable_label_148

	// *** Basic block 14

	ld          t1, 0(s6)
	ld          t2, 0(t1)
	sd          t2, -40(s0)
	ld          t2, 8(t1)
	sd          t2, -32(s0)
	ld          t1, 16(t1)
	sd          t1, -24(s0)
	add         t0, s4, t0
	slli        t1, s5, 3
	slli        t2, s5, 4
	add         t1, t1, t2
	add         t2, s4, t1
	ld          t2, 0(t2)
	ld          t3, 0(t2)
	sd          t3, 0(t0)
	ld          t3, 8(t2)
	sd          t3, 8(t0)
	ld          t2, 16(t2)
	sd          t2, 16(t0)
	add         t0, s4, t1
	ld          t1, -40(s0)
	sd          t1, 0(t0)
	ld          t1, -32(s0)
	sd          t1, 8(t0)
	ld          t1, -24(s0)
	sd          t1, 16(t0)
	addi        s5, s5, 1

	// *** Basic block 15

.SortDynamicSymbolTable_label_148:

	// *** Basic block 16

.SortDynamicSymbolTable_label_149:
	addi        s1, s1, 1
	bge         s1, s3, .SortDynamicSymbolTable_label_102

	// *** Basic block 17

.SortDynamicSymbolTable_label_153:
	slli        t0, s5, 3
	slli        t1, s5, 4
	add         t0, t0, t1
	add         a0, s4, t0
	sub         a1, s3, s5
	la          t0, CompareSymbolFixup
	mv          a3, t0
	mv          a2, s2
	call        qsort

	// *** Basic block 18

	mv          a0, s5
	j           .SortDynamicSymbolTable_label_94
.func_end_SortDynamicSymbolTable:
	.size SortDynamicSymbolTable, .func_end_SortDynamicSymbolTable-SortDynamicSymbolTable

	.local  AddSymbolListToDynamicSymbolTable
	.type AddSymbolListToDynamicSymbolTable, @function

AddSymbolListToDynamicSymbolTable:

	// *** Basic block 0

	.global BufferAppend
	.global DynamicLoaderGNUHash
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -40(s0)
	// Saved integer registers.
	sd s1, 64(sp)
	sd s2, 56(sp)
	sd s3, 48(sp)
	sd s4, 40(sp)
	sd s5, 32(sp)
	sd s6, 24(sp)
	sd s7, 16(sp)
	sd s8, 8(sp)
	// End of stack frame
	mv          t0, a0
	mv          t1, a1
	mv          s1, x0
	ld          s2, 8(t0)
	bge         x0, s2, .AddSymbolListToDynamicSymbolTable_label_91

	// *** Basic block 1

	ld          t2, 0(t0)
	ld          s3, 8(t1)
	ld          t3, 8(s3)
	li          t4, 4294967295		// 0xffffffff
	and         s4, t3, t4
	ld          s5, 0(t1)

	// *** Basic block 2

.AddSymbolListToDynamicSymbolTable_label_39:
	slli        t0, s1, 3
	add         t0, t2, t0
	ld          s6, 0(t0)
	addi        s7, s6, 8
	ld          s8, 24(s7)
	seqz        t0, s8
	beqz        s8, .AddSymbolListToDynamicSymbolTable_label_53

	// *** Basic block 3

	lb          t0, 49(s6)

	// *** Basic block 4

.AddSymbolListToDynamicSymbolTable_label_53:
	bnez        t0, .AddSymbolListToDynamicSymbolTable_label_87

	// *** Basic block 5

.AddSymbolListToDynamicSymbolTable_label_55:
	ld          s7, 16(s7)
	addi        a2, s8, 1
	mv          a1, s7
	mv          a0, s3
	call        BufferAppend

	// *** Basic block 6

	sw          s4, -40(s0)
	addi        t0, s0, -40
	sd          s6, 8(t0)
	addi        s3, s0, -40
	mv          a0, s7
	call        DynamicLoaderGNUHash

	// *** Basic block 7

	sw          a0, 4(s3)
	addi        a1, s0, -40
	li          t0, 24		// 0x18 ASCII \x18
	mv          a2, t0
	mv          a0, s5
	call        BufferAppend

	// *** Basic block 8

.AddSymbolListToDynamicSymbolTable_label_87:
	addi        s1, s1, 1
	bge         s1, s2, .AddSymbolListToDynamicSymbolTable_label_39

	// *** Basic block 9

.AddSymbolListToDynamicSymbolTable_label_91:
	// Restored registers.
	ld s1, 64(sp)
	ld s2, 56(sp)
	ld s3, 48(sp)
	ld s4, 40(sp)
	ld s5, 32(sp)
	ld s6, 24(sp)
	ld s7, 16(sp)
	ld s8, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AddSymbolListToDynamicSymbolTable:
	.size AddSymbolListToDynamicSymbolTable, .func_end_AddSymbolListToDynamicSymbolTable-AddSymbolListToDynamicSymbolTable

	.local  NextPowerOf2
	.type NextPowerOf2, @function

NextPowerOf2:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	srli        t0, a0, 1
	or          t1, a0, t0
	srli        t0, a0, 2
	or          t1, a0, t0
	srli        t0, a0, 4
	or          t1, a0, t0
	srli        t0, a0, 8
	or          t1, a0, t0
	srli        t0, a0, 16
	or          t1, a0, t0
	srli        t0, a0, 32
	or          t1, a0, t0
	addi        t0, a0, 1
	mv          a0, t0

	// *** Basic block 1

.NextPowerOf2_label_27:
	ret         
.func_end_NextPowerOf2:
	.size NextPowerOf2, .func_end_NextPowerOf2-NextPowerOf2

	.local  DebugPrintHashTable
	.type DebugPrintHashTable, @function

DebugPrintHashTable:

	// *** Basic block 0

	.global printf
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
	mv          t0, a0
	mv          s1, a1
	ld          s2, 0(t0)
	lla         a0, .str.63
	lwu         s3, 0(s2)
	mv          a1, s3
	call        printf

	// *** Basic block 1

	lla         a0, .str.64
	lwu         s4, 4(s2)
	mv          a1, s4
	call        printf

	// *** Basic block 2

	lla         a0, .str.65
	lwu         a1, 12(s2)
	call        printf

	// *** Basic block 3

	lla         a0, .str.66
	lwu         s5, 8(s2)
	mv          a1, s5
	call        printf

	// *** Basic block 4

	addi        s6, s2, 16
	slli        t0, s5, 3
	add         s2, s6, t0
	slli        t0, s3, 2
	add         s7, s2, t0
	mv          s8, x0
	bge         x0, s5, .DebugPrintHashTable_label_85

	// *** Basic block 5

.DebugPrintHashTable_label_69:
	lla         a0, .str.67
	slli        t0, s8, 3
	add         t0, s6, t0
	ld          a2, 0(t0)
	mv          a1, s8
	call        printf

	// *** Basic block 6

.DebugPrintHashTable_label_81:
	addi        s8, s8, 1
	bge         s8, s5, .DebugPrintHashTable_label_69

	// *** Basic block 7

.DebugPrintHashTable_label_85:
	mv          s5, x0
	bge         x0, s3, .DebugPrintHashTable_label_105

	// *** Basic block 8

.DebugPrintHashTable_label_90:
	lla         a0, .str.68
	slli        t0, s5, 2
	add         t0, s2, t0
	lwu         a2, 0(t0)
	mv          a1, s5
	call        printf

	// *** Basic block 9

.DebugPrintHashTable_label_101:
	addi        s5, s5, 1
	bge         s5, s3, .DebugPrintHashTable_label_90

	// *** Basic block 10

.DebugPrintHashTable_label_105:
	mv          s2, x0
	sub         s3, s1, s4
	bge         x0, s3, .DebugPrintHashTable_label_126

	// *** Basic block 11

.DebugPrintHashTable_label_111:
	lla         a0, .str.69
	slli        t0, s2, 2
	add         t0, s7, t0
	lwu         a2, 0(t0)
	mv          a1, s2
	call        printf

	// *** Basic block 12

.DebugPrintHashTable_label_122:
	addi        s2, s2, 1
	bge         s2, s3, .DebugPrintHashTable_label_111

	// *** Basic block 13

.DebugPrintHashTable_label_126:
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
.func_end_DebugPrintHashTable:
	.size DebugPrintHashTable, .func_end_DebugPrintHashTable-DebugPrintHashTable

	.local  WriteHeader
	.type WriteHeader, @function

WriteHeader:

	// *** Basic block 0

	.local num_gnu_buckets
	.local NextPowerOf2
	.global BufferAppend
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
	la          t0, num_gnu_buckets
	lw          t0, 0(t0)
	sw          t0, 0(s1)
	sext.w      t0, a3
	sw          t0, 4(s1)
	ld          t0, 8(a1)
	li          t1, 24		// 0x18 ASCII \x18
	div         t2, t0, t1
	sub         t0, t2, a3
	slli        t1, t0, 2
	slli        t0, t0, 3
	add         t2, t1, t0
	li          t0, 64		// 0x40 ASCII '@'
	div         a0, t2, t0
	call        NextPowerOf2

	// *** Basic block 1

	sext.w      t0, a0
	sw          t0, 8(s1)
	li          t0, 26		// 0x1a ASCII \x1a
	sw          t0, 12(s1)
	li          t0, 16		// 0x10 ASCII \x10
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BufferAppend
.func_end_WriteHeader:
	.size WriteHeader, .func_end_WriteHeader-WriteHeader

	.local  WriteBloomFilter
	.type WriteBloomFilter, @function

WriteBloomFilter:

	// *** Basic block 0

	.global BufferAddSpace
	.global memset
	.global DynamicLoaderBloomBits64
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
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	mv          s3, a2
	ld          s4, 8(s1)
	lwu         s5, 8(a3)
	slli        s6, s5, 3
	mv          a1, s6
	mv          a0, s1
	call        BufferAddSpace

	// *** Basic block 1

	ld          t0, 0(s1)
	add         s7, t0, s4
	mv          a2, s6
	mv          a1, x0
	mv          a0, s7
	call        memset

	// *** Basic block 2

	ld          t0, 8(s2)
	li          t1, 24		// 0x18 ASCII \x18
	div         s4, t0, t1
	mv          s6, s3
	bge         s6, s4, .WriteBloomFilter_label_81

	// *** Basic block 3

	ld          s1, 0(s2)

	// *** Basic block 4

.WriteBloomFilter_label_58:
	slli        t0, s6, 3
	slli        t1, s6, 4
	add         t0, t0, t1
	add         s2, s1, t0
	lwu         a0, 4(s2)
	li          t0, 64		// 0x40 ASCII '@'
	div         t0, a0, t0
	rem         s1, t0, s5
	call        DynamicLoaderBloomBits64

	// *** Basic block 5

	slli        t0, s1, 3
	add         t0, s7, t0
	ld          t1, 0(t0)
	or          t1, t1, a0
	sd          t1, 0(t0)

	// *** Basic block 6

.WriteBloomFilter_label_77:
	addi        s6, s6, 1
	bge         s6, s4, .WriteBloomFilter_label_58

	// *** Basic block 7

.WriteBloomFilter_label_81:
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_WriteBloomFilter:
	.size WriteBloomFilter, .func_end_WriteBloomFilter-WriteBloomFilter

	.local  WriteHashTable
	.type WriteHashTable, @function

WriteHashTable:

	// *** Basic block 0

	.global BufferAddSpace
	.local num_gnu_buckets
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 72(sp)
	sd s2, 64(sp)
	sd s3, 56(sp)
	sd s4, 48(sp)
	sd s5, 40(sp)
	sd s6, 32(sp)
	sd s7, 24(sp)
	sd s8, 16(sp)
	sd s9, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	mv          s3, a2
	ld          s4, 8(s1)
	lwu         t0, 0(a3)
	slli        t1, t0, 2
	add         s5, s4, t1
	ld          t1, 8(s2)
	li          t2, 24		// 0x18 ASCII \x18
	div         s6, t1, t2
	sub         t1, s6, s3
	add         t0, t0, t1
	slli        a1, t0, 2
	mv          a0, s1
	call        BufferAddSpace

	// *** Basic block 1

	ld          t0, 0(s2)
	la          t1, num_gnu_buckets
	lw          t1, 0(t1)
	ld          t2, 0(s1)
	add         s7, t2, s4
	ld          t2, 0(s1)
	add         s4, t2, s5
	mv          s5, x0
	sext.w      t2, s3
	sw          t2, 0(s7)
	li          s8, -1		// 0xffffffffffffffff
	mv          s9, s3
	bge         s9, s6, .WriteHashTable_label_109

	// *** Basic block 2

.WriteHashTable_label_69:
	slli        t2, s9, 3
	slli        t3, s9, 4
	add         t2, t2, t3
	add         s1, t0, t2
	lwu         t0, 4(s1)
	andi        s1, t0, -2
	rem         s5, t0, t1
	beq         s5, s5, .WriteHashTable_label_99

	// *** Basic block 3

	li          t0, -1		// 0xffffffffffffffff
	beq         s8, t0, .WriteHashTable_label_93

	// *** Basic block 4

	slli        t1, s8, 2
	add         t1, s4, t1
	lwu         t2, 0(t1)
	ori         t2, t2, 1
	sw          t2, 0(t1)

	// *** Basic block 5

.WriteHashTable_label_93:
	slli        t1, s5, 2
	add         t1, s7, t1
	and         t0, s9, t0
	sw          t0, 0(t1)

	// *** Basic block 6

.WriteHashTable_label_99:
	sub         t0, s9, s3
	sext.w      s8, t0
	slli        t0, s8, 2
	add         t0, s4, t0
	sw          s1, 0(t0)

	// *** Basic block 7

.WriteHashTable_label_105:
	addi        s9, s9, 1
	bge         s9, s6, .WriteHashTable_label_69

	// *** Basic block 8

.WriteHashTable_label_109:
	li          t0, -1		// 0xffffffffffffffff
	beq         s8, t0, .WriteHashTable_label_119

	// *** Basic block 9

	slli        t0, s8, 2
	add         t0, s4, t0
	lwu         t1, 0(t0)
	ori         t1, t1, 1
	sw          t1, 0(t0)

	// *** Basic block 10

.WriteHashTable_label_119:
	// Restored registers.
	ld s1, 72(sp)
	ld s2, 64(sp)
	ld s3, 56(sp)
	ld s4, 48(sp)
	ld s5, 40(sp)
	ld s6, 32(sp)
	ld s7, 24(sp)
	ld s8, 16(sp)
	ld s9, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_WriteHashTable:
	.size WriteHashTable, .func_end_WriteHashTable-WriteHashTable

	.local  CreateGNUHashTable
	.type CreateGNUHashTable, @function

CreateGNUHashTable:

	// *** Basic block 0

	.local WriteHeader
	.local WriteBloomFilter
	.local WriteHashTable
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	addi        t0, s0, -32
	mv          a3, s3
	mv          a2, s2
	mv          a1, s1
	mv          a0, t0
	call        WriteHeader

	// *** Basic block 1

	ld          t0, -32(s0)
	sd          t0, -48(s0)
	ld          t0, -24(s0)
	sd          t0, -40(s0)
	addi        a3, s0, -48
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        WriteBloomFilter

	// *** Basic block 2

	addi        a3, s0, -48
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        WriteHashTable

	// *** Basic block 3

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CreateGNUHashTable:
	.size CreateGNUHashTable, .func_end_CreateGNUHashTable-CreateGNUHashTable

	.global DynamicLinkerFixupDynamicSymbolTable
	.type DynamicLinkerFixupDynamicSymbolTable, @function

DynamicLinkerFixupDynamicSymbolTable:

	// *** Basic block 0

	.global ELFSymbolInit
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
	li          s2, 24		// 0x18 ASCII \x18
	ld          t0, 8(s1)
	li          s3, 24		// 0x18 ASCII \x18
	bge         s3, t0, .DynamicLinkerFixupDynamicSymbolTable_label_77

	// *** Basic block 1

.DynamicLinkerFixupDynamicSymbolTable_label_27:
	ld          t0, 0(s1)
	add         s4, t0, s2
	ld          s5, 8(s4)
	mv          s6, s4
	ld          t0, 0(s5)
	lbu         t1, 4(t0)
	andi        s7, t1, 15
	srli        s8, t1, 4
	lwu         a1, 0(s4)
	lhu         a2, 6(t0)
	ld          a5, 16(t0)
	ld          a6, 80(s5)
	mv          a4, s8
	mv          a3, s7
	mv          a0, s6
	call        ELFSymbolInit

	// *** Basic block 2

	sext.w      t0, s2
	div         t0, t0, s3
	sw          t0, 100(s5)
	addi        s2, s2, 24
	ld          t0, 8(s1)
	blt         s2, t0, .DynamicLinkerFixupDynamicSymbolTable_label_27

	// *** Basic block 3

.DynamicLinkerFixupDynamicSymbolTable_label_77:
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
.func_end_DynamicLinkerFixupDynamicSymbolTable:
	.size DynamicLinkerFixupDynamicSymbolTable, .func_end_DynamicLinkerFixupDynamicSymbolTable-DynamicLinkerFixupDynamicSymbolTable

	.local  Basename
	.type Basename, @function

Basename:

	// *** Basic block 0

	.global strrchr
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          t0, a0
	ld          s1, 16(t0)
	li          a1, 47		// 0x2f ASCII '/'
	mv          a0, s1
	call        strrchr

	// *** Basic block 1

	mv          s1, a0
	beq         s1, x0, .Basename_label_26

	// *** Basic block 2

	addi        s1, s1, 1
	j           .Basename_label_27

	// *** Basic block 3

.Basename_label_26:

	// *** Basic block 4

.Basename_label_27:
	mv          a0, s1

	// *** Basic block 5

.Basename_label_30:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Basename:
	.size Basename, .func_end_Basename-Basename

	.local  InsertDynamicStrings
	.type InsertDynamicStrings, @function

InsertDynamicStrings:

	// *** Basic block 0

	.local Basename
	.global BufferAppend
	.global strlen
	.global VectorAppend
	.global StringInit
	.global StringAppend
	.global StringDestruct
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -56(s0)
	// Saved integer registers.
	sd s1, 64(sp)
	sd s2, 56(sp)
	sd s3, 48(sp)
	sd s4, 40(sp)
	sd s5, 32(sp)
	sd s6, 24(sp)
	sd s7, 16(sp)
	sd s8, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a2
	ld          t0, 104(a1)
	addi        t0, t0, 8
	ld          t0, 8(t0)
	sw          t0, 568(s1)
	call        Basename

	// *** Basic block 1

	mv          s3, a0
	addi        s4, s2, 8
	mv          a0, s3
	call        strlen

	// *** Basic block 2

	addi        a2, a0, 1
	mv          a1, s3
	mv          a0, s4
	call        BufferAppend

	// *** Basic block 3

	mv          s4, x0
	addi        t0, s1, 496
	ld          t0, 8(t0)
	bge         x0, t0, .InsertDynamicStrings_label_105

	// *** Basic block 4

.InsertDynamicStrings_label_65:
	ld          t0, 496(s1)
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	ld          t0, 536(s1)
	addi        a0, t0, 208
	addi        t0, s2, 8
	ld          a1, 8(t0)
	call        VectorAppend

	// *** Basic block 5

	mv          a0, s3
	call        Basename

	// *** Basic block 6

	mv          s3, a0
	addi        s5, s2, 8
	mv          a0, s3
	call        strlen

	// *** Basic block 7

	addi        a2, a0, 1
	mv          a1, s3
	mv          a0, s5
	call        BufferAppend

	// *** Basic block 8

.InsertDynamicStrings_label_98:
	addi        s4, s4, 1
	addi        t0, s1, 496
	ld          t0, 8(t0)
	bge         s4, t0, .InsertDynamicStrings_label_65

	// *** Basic block 9

.InsertDynamicStrings_label_105:
	addi        a0, s0, -56
	mv          a1, x0
	call        StringInit

	// *** Basic block 10

	lla         s5, .str.70
	mv          s6, x0
	addi        t0, s1, 96
	ld          s7, 8(t0)
	bge         x0, s7, .InsertDynamicStrings_label_146

	// *** Basic block 11

	ld          s8, 96(s1)

	// *** Basic block 12

.InsertDynamicStrings_label_124:
	slli        t0, s6, 3
	add         t0, s8, t0
	ld          s8, 0(t0)
	addi        a0, s0, -56
	mv          a1, s5
	call        StringAppend

	// *** Basic block 13

	addi        a0, s0, -56
	ld          a1, 16(s8)
	call        StringAppend

	// *** Basic block 15

.InsertDynamicStrings_label_142:
	addi        s6, s6, 1
	bge         s6, s7, .InsertDynamicStrings_label_124

	// *** Basic block 16

.InsertDynamicStrings_label_146:
	ld          t0, 536(s1)
	addi        t1, s2, 8
	ld          t1, 8(t1)
	sd          t1, 232(t0)
	addi        a0, s2, 8
	addi        t0, s0, -56
	ld          a1, 16(t0)
	addi        t0, s0, -56
	ld          t0, 24(t0)
	addi        a2, t0, 1
	call        BufferAppend

	// *** Basic block 17

	addi        a0, s0, -56
	call        StringDestruct

	// *** Basic block 18

	// Restored registers.
	ld s1, 64(sp)
	ld s2, 56(sp)
	ld s3, 48(sp)
	ld s4, 40(sp)
	ld s5, 32(sp)
	ld s6, 24(sp)
	ld s7, 16(sp)
	ld s8, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_InsertDynamicStrings:
	.size InsertDynamicStrings, .func_end_InsertDynamicStrings-InsertDynamicStrings

	.local  AddDynamicSymbolTable
	.type AddDynamicSymbolTable, @function

AddDynamicSymbolTable:

	// *** Basic block 0

	.global NewELFWriterSectionContents
	.local NewELFSection
	.global BufferAppend
	.local InsertDynamicStrings
	.local NewDynamicLinkerGroup
	.global memset
	.global HashTableTraverse
	.local AddSymbolListToDynamicSymbolTable
	.local num_gnu_buckets
	.local SortDynamicSymbolTable
	.local CreateGNUHashTable
	.global BufferAlignLength
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -56(s0)
	// Saved integer registers.
	sd s1, 80(sp)
	sd s2, 72(sp)
	sd s3, 64(sp)
	sd s4, 56(sp)
	sd s5, 48(sp)
	sd s6, 40(sp)
	sd s7, 32(sp)
	sd s8, 24(sp)
	sd s9, 16(sp)
	sd s10, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          a0, x0
	call        NewELFWriterSectionContents

	// *** Basic block 1

	mv          s2, a0
	lla         a0, .str.72
	mv          a4, s2
	li          s3, 8		// 0x8 ASCII \x8
	mv          a3, s3
	li          t0, 34		// 0x22 ASCII '"'
	mv          a2, t0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	call        NewELFSection

	// *** Basic block 2

	mv          s4, a0
	addi        a0, s2, 8
	lla         a1, .str.73
	li          s5, 1		// 0x1 ASCII \x1
	mv          a2, s5
	call        BufferAppend

	// *** Basic block 3

	mv          a2, s2
	mv          a1, s4
	mv          a0, s1
	call        InsertDynamicStrings

	// *** Basic block 4

	mv          a0, x0
	call        NewELFWriterSectionContents

	// *** Basic block 5

	mv          s6, a0
	lla         a0, .str.74
	mv          a4, s6
	mv          a3, s3
	li          s7, 2		// 0x2 ASCII \x2
	mv          a2, s7
	li          t0, 11		// 0xb ASCII \xb
	mv          a1, t0
	call        NewELFSection

	// *** Basic block 6

	mv          s8, a0
	addi        t0, s8, 40
	li          s9, 24		// 0x18 ASCII \x18
	sd          s9, 56(t0)
	addi        a2, s1, 272
	mv          a1, s8
	mv          a0, s1
	call        NewDynamicLinkerGroup

	// *** Basic block 7

	sd          x0, -56(s0)
	sd          x0, -48(s0)
	addi        t0, s6, 8
	sd          t0, -56(s0)
	addi        t0, s0, -56
	addi        t1, s2, 8
	sd          t1, 8(t0)
	addi        a0, s0, -40
	mv          a2, s9
	mv          a1, x0
	call        memset

	// *** Basic block 8

	ld          a0, -56(s0)
	addi        a1, s0, -40
	mv          a2, s9
	call        BufferAppend

	// *** Basic block 9

	addi        a0, s1, 160
	addi        a2, s0, -56
	la          t0, AddSymbolListToDynamicSymbolTable
	mv          a1, t0
	call        HashTableTraverse

	// *** Basic block 10

	addi        a2, s1, 272
	mv          a1, s4
	mv          a0, s1
	call        NewDynamicLinkerGroup

	// *** Basic block 11

	mv          a0, x0
	call        NewELFWriterSectionContents

	// *** Basic block 12

	mv          s9, a0
	addi        s10, s9, 8
	lla         a0, .str.75
	mv          a4, s9
	mv          a3, s3
	mv          a2, s7
	li          t0, 1879048182		// 0x6ffffff6
	mv          a1, t0
	call        NewELFSection

	// *** Basic block 13

	mv          s7, a0
	addi        a2, s1, 272
	mv          a1, s7
	mv          a0, s1
	call        NewDynamicLinkerGroup

	// *** Basic block 14

	addi        t0, s1, 160
	ld          t0, 56(t0)
	srai        t0, t0, 2
	la          t1, num_gnu_buckets
	sw          t0, 0(t1)
	la          t0, num_gnu_buckets
	lw          t0, 0(t0)
	bnez        t0, .AddDynamicSymbolTable_label_202

	// *** Basic block 15

	la          t0, num_gnu_buckets
	sw          s5, 0(t0)

	// *** Basic block 16

.AddDynamicSymbolTable_label_202:
	ld          s1, -56(s0)
	mv          a0, s1
	call        SortDynamicSymbolTable

	// *** Basic block 17

	mv          s2, a0
	mv          a2, s2
	mv          a1, s10
	mv          a0, s1
	call        CreateGNUHashTable

	// *** Basic block 18

	mv          a1, s3
	mv          a0, s1
	call        BufferAlignLength

	// *** Basic block 19

	addi        t0, s0, -56
	ld          a0, 8(t0)
	mv          a1, s3
	call        BufferAlignLength

	// *** Basic block 20

	mv          a1, s3
	mv          a0, s10
	call        BufferAlignLength

	// *** Basic block 21

	// Restored registers.
	ld s1, 80(sp)
	ld s2, 72(sp)
	ld s3, 64(sp)
	ld s4, 56(sp)
	ld s5, 48(sp)
	ld s6, 40(sp)
	ld s7, 32(sp)
	ld s8, 24(sp)
	ld s9, 16(sp)
	ld s10, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AddDynamicSymbolTable:
	.size AddDynamicSymbolTable, .func_end_AddDynamicSymbolTable-AddDynamicSymbolTable

	.global DynamicLinkerCreateDynamicLinkerGroups
	.type DynamicLinkerCreateDynamicLinkerGroups, @function

DynamicLinkerCreateDynamicLinkerGroups:

	// *** Basic block 0

	.local AddDynamicSymbolTable
	.local AddGlobalOffsetTable
	.local AddProcedureLinkageTable
	.local AddDynamicSection
	.local AddDynamicRelocationsSection
	.local AddPLTRelocationsSection
	.local AddInterpreterSection
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
	ld          s2, 536(s1)
	call        AddDynamicSymbolTable

	// *** Basic block 1

	addi        a1, s2, 256
	addi        a2, s2, 264
	mv          a0, s1
	call        AddGlobalOffsetTable

	// *** Basic block 2

	mv          a0, s1
	call        AddProcedureLinkageTable

	// *** Basic block 3

	sd          a0, 272(s2)
	mv          a0, s1
	call        AddDynamicSection

	// *** Basic block 4

	sd          a0, 304(s2)
	mv          a0, s1
	call        AddDynamicRelocationsSection

	// *** Basic block 5

	sd          a0, 280(s2)
	mv          a0, s1
	call        AddPLTRelocationsSection

	// *** Basic block 6

	sd          a0, 288(s2)
	lb          t0, 528(s1)
	not         t0, t0
	beqz        t0, .DynamicLinkerCreateDynamicLinkerGroups_label_66

	// *** Basic block 7

	mv          a0, s1
	call        AddInterpreterSection

	// *** Basic block 8

	sd          a0, 312(s2)

	// *** Basic block 9

.DynamicLinkerCreateDynamicLinkerGroups_label_66:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DynamicLinkerCreateDynamicLinkerGroups:
	.size DynamicLinkerCreateDynamicLinkerGroups, .func_end_DynamicLinkerCreateDynamicLinkerGroups-DynamicLinkerCreateDynamicLinkerGroups

.PCend:
	.data
	.type   num_gnu_buckets,@object
	.local  num_gnu_buckets
	.comm   num_gnu_buckets,4,4

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.1, @object
	.size .str.1, 30

.str.2:
	.asciz "(null)"
	.type .str.2, @object
	.size .str.2, 1

.str.3:
	.asciz "linker->arch != NULL"
	.type .str.3, @object
	.size .str.3, 21

.str.4:
	.asciz "_GLOBAL_OFFSET_TABLE_"
	.type .str.4, @object
	.size .str.4, 22

.str.5:
	.asciz "_DYNAMIC_"
	.type .str.5, @object
	.size .str.5, 10

.str.6:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.6, @object
	.size .str.6, 30

.str.7:
	.asciz "(null)"
	.type .str.7, @object
	.size .str.7, 1

.str.8:
	.asciz "group->components.length == 1"
	.type .str.8, @object
	.size .str.8, 30

.str.9:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.9, @object
	.size .str.9, 30

.str.10:
	.asciz "(null)"
	.type .str.10, @object
	.size .str.10, 1

.str.11:
	.asciz "sect->source == kGroupedSectionNew"
	.type .str.11, @object
	.size .str.11, 35

.str.12:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.12, @object
	.size .str.12, 30

.str.13:
	.asciz "(null)"
	.type .str.13, @object
	.size .str.13, 1

.str.14:
	.asciz "sect->section.new->contents->data_location == kSectionContentsBuffered"
	.type .str.14, @object
	.size .str.14, 71

.str.15:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.15, @object
	.size .str.15, 30

.str.16:
	.asciz "(null)"
	.type .str.16, @object
	.size .str.16, 1

.str.17:
	.asciz "contents->data_location == kSectionContentsBuffered"
	.type .str.17, @object
	.size .str.17, 52

.str.18:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.18, @object
	.size .str.18, 30

.str.19:
	.asciz "(null)"
	.type .str.19, @object
	.size .str.19, 1

.str.20:
	.asciz "symbol_index != -1"
	.type .str.20, @object
	.size .str.20, 19

.str.21:
	.asciz ".got"
	.type .str.21, @object
	.size .str.21, 5

.str.22:
	.asciz ".got.plt"
	.type .str.22, @object
	.size .str.22, 9

.str.23:
	.asciz ".plt"
	.type .str.23, @object
	.size .str.23, 5

.str.24:
	.asciz ".rela.dyn"
	.type .str.24, @object
	.size .str.24, 10

.str.25:
	.asciz ".rela.plt"
	.type .str.25, @object
	.size .str.25, 10

.str.26:
	.asciz ".rela.data"
	.type .str.26, @object
	.size .str.26, 11

.str.27:
	.asciz ".interp"
	.type .str.27, @object
	.size .str.27, 8

.str.28:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.28, @object
	.size .str.28, 30

.str.29:
	.asciz "(null)"
	.type .str.29, @object
	.size .str.29, 1

.str.30:
	.asciz "false"
	.type .str.30, @object
	.size .str.30, 6

.str.31:
	.asciz ".dynamic"
	.type .str.31, @object
	.size .str.31, 9

.str.32:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.32, @object
	.size .str.32, 30

.str.33:
	.asciz "(null)"
	.type .str.33, @object
	.size .str.33, 1

.str.34:
	.asciz "dynamic != NULL"
	.type .str.34, @object
	.size .str.34, 16

.str.35:
	.asciz ".dynstr"
	.type .str.35, @object
	.size .str.35, 8

.str.36:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.36, @object
	.size .str.36, 30

.str.37:
	.asciz "(null)"
	.type .str.37, @object
	.size .str.37, 1

.str.38:
	.asciz "strtab != NULL"
	.type .str.38, @object
	.size .str.38, 15

.str.39:
	.asciz ".dynsym"
	.type .str.39, @object
	.size .str.39, 8

.str.40:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.40, @object
	.size .str.40, 30

.str.41:
	.asciz "(null)"
	.type .str.41, @object
	.size .str.41, 1

.str.42:
	.asciz "symtab != NULL"
	.type .str.42, @object
	.size .str.42, 15

.str.43:
	.asciz ".rela.dyn"
	.type .str.43, @object
	.size .str.43, 10

.str.44:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.44, @object
	.size .str.44, 30

.str.45:
	.asciz "(null)"
	.type .str.45, @object
	.size .str.45, 1

.str.46:
	.asciz "rela != NULL"
	.type .str.46, @object
	.size .str.46, 13

.str.47:
	.asciz ".gnu_hash"
	.type .str.47, @object
	.size .str.47, 10

.str.48:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.48, @object
	.size .str.48, 30

.str.49:
	.asciz "(null)"
	.type .str.49, @object
	.size .str.49, 1

.str.50:
	.asciz "hash != NULL"
	.type .str.50, @object
	.size .str.50, 13

.str.51:
	.asciz ".got.plt"
	.type .str.51, @object
	.size .str.51, 9

.str.52:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.52, @object
	.size .str.52, 30

.str.53:
	.asciz "(null)"
	.type .str.53, @object
	.size .str.53, 1

.str.54:
	.asciz "got_plt != NULL"
	.type .str.54, @object
	.size .str.54, 16

.str.55:
	.asciz ".rela.plt"
	.type .str.55, @object
	.size .str.55, 10

.str.56:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.56, @object
	.size .str.56, 30

.str.57:
	.asciz "(null)"
	.type .str.57, @object
	.size .str.57, 1

.str.58:
	.asciz "jmp_rel != NULL"
	.type .str.58, @object
	.size .str.58, 16

.str.59:
	.asciz ".dynamic"
	.type .str.59, @object
	.size .str.59, 9

.str.60:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.60, @object
	.size .str.60, 30

.str.61:
	.asciz "(null)"
	.type .str.61, @object
	.size .str.61, 1

.str.62:
	.asciz "num_gnu_buckets > 0"
	.type .str.62, @object
	.size .str.62, 20

.str.63:
	.asciz "nbuckets: %d\n"
	.type .str.63, @object
	.size .str.63, 14

.str.64:
	.asciz "symoffset: %d\n"
	.type .str.64, @object
	.size .str.64, 15

.str.65:
	.asciz "bloom_shift: %d\n"
	.type .str.65, @object
	.size .str.65, 17

.str.66:
	.asciz "bloom_size: %d\n"
	.type .str.66, @object
	.size .str.66, 16

.str.67:
	.asciz "bloom[%d] = %llx\n"
	.type .str.67, @object
	.size .str.67, 18

.str.68:
	.asciz "bucket[%d]: %d\n"
	.type .str.68, @object
	.size .str.68, 16

.str.69:
	.asciz "chain[%d]: %x\n"
	.type .str.69, @object
	.size .str.69, 15

.str.70:
	.asciz "(null)"
	.type .str.70, @object
	.size .str.70, 1

.str.71:
	.asciz ":"
	.type .str.71, @object
	.size .str.71, 2

.str.72:
	.asciz ".dynstr"
	.type .str.72, @object
	.size .str.72, 8

.str.73:
	.asciz "(null)"
	.type .str.73, @object
	.size .str.73, 1

.str.74:
	.asciz ".dynsym"
	.type .str.74, @object
	.size .str.74, 8

.str.75:
	.asciz ".gnu_hash"
	.type .str.75, @object
	.size .str.75, 10

