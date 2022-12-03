	.file   "debug.c"
	.text
	.option pic
.PCbegin:
	.local  CompareSignatures
	.type CompareSignatures, @function

CompareSignatures:

	// *** Basic block 0

	.global BufferCompare
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, t0
	mv          t3, t1
	ld          a0, 0(t2)
	ld          a1, 0(t3)
	j           BufferCompare
.func_end_CompareSignatures:
	.size CompareSignatures, .func_end_CompareSignatures-CompareSignatures

	.global DebugBuilderInit
	.type DebugBuilderInit, @function

DebugBuilderInit:

	// *** Basic block 0

	.global MapInitForCharPointerKeys
	.global MapInit
	.local CompareSignatures
	.global VectorInit
	.global BufferInit
	.global malloc
	.local DIEInit
	.local compile_unit_virtuals
	.global StringInit
	.local AddDIE
	.global DIEBuild
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
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	mv          s4, a3
	call        MapInitForCharPointerKeys

	// *** Basic block 1

	addi        a0, s1, 32
	la          t0, CompareSignatures
	mv          a1, t0
	call        MapInit

	// *** Basic block 2

	addi        a0, s1, 64
	call        VectorInit

	// *** Basic block 3

	addi        a0, s1, 88
	call        BufferInit

	// *** Basic block 4

	li          s5, 1		// 0x1 ASCII \x1
	sw          s5, 112(s1)
	addi        a0, s1, 120
	call        VectorInit

	// *** Basic block 5

	addi        a0, s1, 144
	call        VectorInit

	// *** Basic block 6

	addi        a0, s1, 272
	call        BufferInit

	// *** Basic block 7

	li          t0, 56		// 0x38 ASCII '8'
	mv          a0, t0
	call        malloc

	// *** Basic block 8

	sd          a0, 168(s1)
	ld          a0, 168(s1)
	la          a2, compile_unit_virtuals
	mv          a3, s5
	li          t0, 17		// 0x11 ASCII \x11
	mv          a1, t0
	call        DIEInit

	// *** Basic block 9

	addi        a0, s1, 176
	mv          a1, s2
	call        StringInit

	// *** Basic block 10

	sd          s3, 216(s1)
	addi        a0, s1, 224
	mv          a1, s4
	call        StringInit

	// *** Basic block 11

	ld          s5, 168(s1)
	mv          a1, s5
	mv          a0, s1
	call        AddDIE

	// *** Basic block 12

	mv          a1, s5
	mv          a0, s1
	call        DIEBuild

	// *** Basic block 13

	addi        a0, s1, 144
	ld          a1, 168(s1)
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorAppend
.func_end_DebugBuilderInit:
	.size DebugBuilderInit, .func_end_DebugBuilderInit-DebugBuilderInit

	.local  DebugAbbreviationMapDestruct
	.type DebugAbbreviationMapDestruct, @function

DebugAbbreviationMapDestruct:

	// *** Basic block 0

	.global BufferDelete
	.global free
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
	ld          a0, 0(s1)
	call        BufferDelete

	// *** Basic block 1

	ld          a0, 8(s1)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_DebugAbbreviationMapDestruct:
	.size DebugAbbreviationMapDestruct, .func_end_DebugAbbreviationMapDestruct-DebugAbbreviationMapDestruct

	.global DebugBuilderDestruct
	.type DebugBuilderDestruct, @function

DebugBuilderDestruct:

	// *** Basic block 0

	.global MapDestruct
	.global MapDestructWithContents
	.local DebugAbbreviationMapDestruct
	.global VectorDestruct
	.global BufferDestruct
	.global VectorDestructWithContents
	.global DIEDestruct
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
	call        MapDestruct

	// *** Basic block 1

	addi        a0, s1, 32
	la          t0, DebugAbbreviationMapDestruct
	mv          a1, t0
	call        MapDestructWithContents

	// *** Basic block 2

	addi        a0, s1, 64
	call        VectorDestruct

	// *** Basic block 3

	addi        a0, s1, 88
	call        BufferDestruct

	// *** Basic block 4

	addi        a0, s1, 120
	la          t0, DIEDestruct
	ld          a1, 0(t0)
	call        VectorDestructWithContents

	// *** Basic block 5

	addi        a0, s1, 144
	call        VectorDestruct

	// *** Basic block 6

	addi        a0, s1, 272
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BufferDestruct
.func_end_DebugBuilderDestruct:
	.size DebugBuilderDestruct, .func_end_DebugBuilderDestruct-DebugBuilderDestruct

	.local  AddDIE
	.type AddDIE, @function

AddDIE:

	// *** Basic block 0

	.global VectorAppend
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	addi        t2, t1, 120
	ld          t2, 8(t2)
	addi        t2, t2, 100
	sw          t2, 0(t0)
	addi        a0, t1, 120
	j           VectorAppend
.func_end_AddDIE:
	.size AddDIE, .func_end_AddDIE-AddDIE

	.global BuildDebugInfo
	.type BuildDebugInfo, @function

BuildDebugInfo:

	// *** Basic block 0

	.local NewSymbolDIE
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
	mv          s1, a0
	call        NewSymbolDIE

	// *** Basic block 1

	mv          s2, a0
	addi        a0, s1, 144
	mv          a1, s2
	call        VectorAppend

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.BuildDebugInfo_label_30:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BuildDebugInfo:
	.size BuildDebugInfo, .func_end_BuildDebugInfo-BuildDebugInfo

	.global BuildDebugInfoAfterCodegen
	.type BuildDebugInfoAfterCodegen, @function

BuildDebugInfoAfterCodegen:

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
	mv          s1, a0
	ld          s2, 128(a1)
	beq         s2, x0, .BuildDebugInfoAfterCodegen_label_25

	// *** Basic block 1

	j           .BuildDebugInfoAfterCodegen_label_42

	// *** Basic block 2

.BuildDebugInfoAfterCodegen_label_25:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 93		// 0x5d ASCII ']'
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.BuildDebugInfoAfterCodegen_label_42:
	ld          t0, 8(s2)
	ld          t0, 8(t0)
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_BuildDebugInfoAfterCodegen:
	.size BuildDebugInfoAfterCodegen, .func_end_BuildDebugInfoAfterCodegen-BuildDebugInfoAfterCodegen

	.global BuildTypeDebugInfo
	.type BuildTypeDebugInfo, @function

BuildTypeDebugInfo:

	// *** Basic block 0

	.local NewTypeRecordDIE
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
	call        NewTypeRecordDIE

	// *** Basic block 1

	mv          s2, a0
	ld          t0, 8(s2)
	ld          t0, 8(t0)
	mv          a1, s2
	mv          a0, s1
	jalr         x1, t0, 0

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.BuildTypeDebugInfo_label_29:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BuildTypeDebugInfo:
	.size BuildTypeDebugInfo, .func_end_BuildTypeDebugInfo-BuildTypeDebugInfo

	.global DIEBuild
	.type DIEBuild, @function

DIEBuild:

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
	mv          s1, a1
	mv          s2, a0
	beq         s1, x0, .DIEBuild_label_22

	// *** Basic block 1

	j           .DIEBuild_label_39

	// *** Basic block 2

.DIEBuild_label_22:
	lla         a0, .str.4
	lla         a1, .str.5
	lla         a3, .str.6
	li          t0, 106		// 0x6a ASCII 'j'
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.DIEBuild_label_39:
	ld          t0, 16(s1)
	beq         t0, x0, .DIEBuild_label_48

	// *** Basic block 5

.DIEBuild_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.DIEBuild_label_48:
	ld          t0, 8(s1)
	ld          t0, 8(t0)
	mv          a1, s1
	mv          a0, s2
	jalr         x1, t0, 0

	// *** Basic block 7

	j           .DIEBuild_label_45
.func_end_DIEBuild:
	.size DIEBuild, .func_end_DIEBuild-DIEBuild

	.global DIEPrint
	.type DIEPrint, @function

DIEPrint:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 8(t0)
	ld          t1, 16(t1)
	mv          t0, t1
	jr          t0
.func_end_DIEPrint:
	.size DIEPrint, .func_end_DIEPrint-DIEPrint

	.local  PrimitiveToDIE
	.type PrimitiveToDIE, @function

PrimitiveToDIE:

	// *** Basic block 0

	.local base_types
	.global printf
	.global abort
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
	mv          s2, x0

	// *** Basic block 1

.PrimitiveToDIE_label_21:
	slli        t0, s2, 4
	slli        t1, s2, 6
	add         s3, t0, t1
	la          t0, base_types
	add         t0, t0, s3
	ld          t0, 56(t0)
	mv          a0, s1
	jalr         x1, t0, 0

	// *** Basic block 2

	beqz        a0, .PrimitiveToDIE_label_41

	// *** Basic block 3

	la          t0, base_types
	add         a0, t0, s3

	// *** Basic block 4

.PrimitiveToDIE_label_38:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 5

.PrimitiveToDIE_label_41:

	// *** Basic block 6

.PrimitiveToDIE_label_42:
	addi        s2, s2, 1
	li          t0, 12		// 0xc ASCII \xc
	bge         s2, t0, .PrimitiveToDIE_label_21

	// *** Basic block 7

.PrimitiveToDIE_label_47:
	lla         a0, .str.19
	lla         a1, .str.20
	lla         a3, .str.21
	li          t0, 189		// 0xbd ASCII \xbd
	mv          a2, t0
	call        printf

	// *** Basic block 8

	call        abort

	// *** Basic block 9

	mv          a0, x0
	j           .PrimitiveToDIE_label_38
.func_end_PrimitiveToDIE:
	.size PrimitiveToDIE, .func_end_PrimitiveToDIE-PrimitiveToDIE

	.local  FindBaseType
	.type FindBaseType, @function

FindBaseType:

	// *** Basic block 0

	.local base_types
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
	mv          t0, a0
	mv          t1, x0

	// *** Basic block 1

.FindBaseType_label_21:
	slli        t2, t1, 4
	slli        t3, t1, 6
	add         s1, t2, t3
	la          t2, base_types
	add         t2, t2, s1
	ld          t2, 56(t2)
	bne         t2, t0, .FindBaseType_label_40

	// *** Basic block 2

	la          t0, base_types
	add         a0, t0, s1

	// *** Basic block 3

.FindBaseType_label_37:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.FindBaseType_label_40:

	// *** Basic block 5

.FindBaseType_label_41:
	addi        t1, t1, 1
	li          t0, 12		// 0xc ASCII \xc
	bge         t1, t0, .FindBaseType_label_21

	// *** Basic block 6

.FindBaseType_label_46:
	lla         a0, .str.22
	lla         a1, .str.23
	lla         a3, .str.24
	li          t0, 199		// 0xc7 ASCII \xc7
	mv          a2, t0
	call        printf

	// *** Basic block 7

	call        abort

	// *** Basic block 8

	mv          a0, x0
	j           .FindBaseType_label_37
.func_end_FindBaseType:
	.size FindBaseType, .func_end_FindBaseType-FindBaseType

	.local  DIEBaseDestruct
	.type DIEBaseDestruct, @function

DIEBaseDestruct:

	// *** Basic block 0

	.global VectorDestructWithContents
	.global DebugAttributeValueDestruct
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 24
	la          t1, DebugAttributeValueDestruct
	ld          a1, 0(t1)
	j           VectorDestructWithContents
.func_end_DIEBaseDestruct:
	.size DIEBaseDestruct, .func_end_DIEBaseDestruct-DIEBaseDestruct

	.global DIEDestruct
	.type DIEDestruct, @function

DIEDestruct:

	// *** Basic block 0

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
	bne         s1, x0, .DIEDestruct_label_14

	// *** Basic block 1

.DIEDestruct_label_11:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.DIEDestruct_label_14:
	ld          t0, 8(s1)
	ld          s2, 0(t0)
	beq         s2, x0, .DIEDestruct_label_24

	// *** Basic block 3

	mv          a0, s1
	jalr         x1, s2, 0

	// *** Basic block 4

.DIEDestruct_label_24:
	j           .DIEDestruct_label_11
.func_end_DIEDestruct:
	.size DIEDestruct, .func_end_DIEDestruct-DIEDestruct

	.global NamedDIEDestruct
	.type NamedDIEDestruct, @function

NamedDIEDestruct:

	// *** Basic block 0

	.global StringDestruct
	.local DIEBaseDestruct
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
	bne         s1, x0, .NamedDIEDestruct_label_16

	// *** Basic block 1

.NamedDIEDestruct_label_13:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.NamedDIEDestruct_label_16:
	mv          s2, s1
	addi        a0, s2, 56
	call        StringDestruct

	// *** Basic block 3

	mv          a0, s1
	call        DIEBaseDestruct

	// *** Basic block 4

	j           .NamedDIEDestruct_label_13
.func_end_NamedDIEDestruct:
	.size NamedDIEDestruct, .func_end_NamedDIEDestruct-NamedDIEDestruct

	.local  DIEInit
	.type DIEInit, @function

DIEInit:

	// *** Basic block 0

	.global VectorInit
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	sw          a1, 4(t0)
	sd          a2, 8(t0)
	addi        a0, t0, 24
	j           VectorInit
.func_end_DIEInit:
	.size DIEInit, .func_end_DIEInit-DIEInit

	.local  NamedDIEInit
	.type NamedDIEInit, @function

NamedDIEInit:

	// *** Basic block 0

	.local DIEInit
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
	mv          s2, a3
	mv          a3, a4
	call        DIEInit

	// *** Basic block 1

	addi        a0, s1, 56
	mv          a1, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringInit
.func_end_NamedDIEInit:
	.size NamedDIEInit, .func_end_NamedDIEInit-NamedDIEInit

	.local  NewSubrangeDIE
	.type NewSubrangeDIE, @function

NewSubrangeDIE:

	// *** Basic block 0

	.global malloc
	.local DIEInit
	.local subrange_virtuals
	.local FindBaseType
	.global TypeIsUnsignedLong
	.local AddDIE
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
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          a2, subrange_virtuals
	mv          a3, x0
	li          t0, 33		// 0x21 ASCII '!'
	mv          a1, t0
	mv          a0, s3
	call        DIEInit

	// *** Basic block 2

	sw          s1, 56(s3)
	la          t0, TypeIsUnsignedLong
	mv          a0, t0
	call        FindBaseType

	// *** Basic block 3

	sd          a0, 64(s3)
	mv          a1, s3
	mv          a0, s2
	call        AddDIE

	// *** Basic block 4

	mv          a0, s3

	// *** Basic block 5

.NewSubrangeDIE_label_52:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewSubrangeDIE:
	.size NewSubrangeDIE, .func_end_NewSubrangeDIE-NewSubrangeDIE

	.local  ArrayDIEDestruct
	.type ArrayDIEDestruct, @function

ArrayDIEDestruct:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local DIEBaseDestruct
	j           DIEBaseDestruct
.func_end_ArrayDIEDestruct:
	.size ArrayDIEDestruct, .func_end_ArrayDIEDestruct-ArrayDIEDestruct

	.local  NewArrayDIE
	.type NewArrayDIE, @function

NewArrayDIE:

	// *** Basic block 0

	.global malloc
	.local DIEInit
	.local array_virtuals
	.local NewSubrangeDIE
	.local AddDIE
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
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	la          a2, array_virtuals
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a1, t0
	mv          a0, s4
	call        DIEInit

	// *** Basic block 2

	mv          a1, s2
	mv          a0, s1
	call        NewSubrangeDIE

	// *** Basic block 3

	sd          a0, 56(s4)
	sd          s3, 64(s4)
	mv          a1, s4
	mv          a0, s1
	call        AddDIE

	// *** Basic block 4

	mv          a0, s4

	// *** Basic block 5

.NewArrayDIE_label_55:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewArrayDIE:
	.size NewArrayDIE, .func_end_NewArrayDIE-NewArrayDIE

	.local  NewPointerDIE
	.type NewPointerDIE, @function

NewPointerDIE:

	// *** Basic block 0

	.global malloc
	.local DIEInit
	.local pointer_virtuals
	.local AddDIE
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
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          a2, pointer_virtuals
	mv          a3, x0
	li          t0, 15		// 0xf ASCII \xf
	mv          a1, t0
	mv          a0, s3
	call        DIEInit

	// *** Basic block 2

	sd          s1, 56(s3)
	mv          a1, s3
	mv          a0, s2
	call        AddDIE

	// *** Basic block 3

	mv          a0, s3

	// *** Basic block 4

.NewPointerDIE_label_43:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewPointerDIE:
	.size NewPointerDIE, .func_end_NewPointerDIE-NewPointerDIE

	.local  LexicalScopeDestruct
	.type LexicalScopeDestruct, @function

LexicalScopeDestruct:

	// *** Basic block 0

	.global VectorDestruct
	.local DIEBaseDestruct
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
	mv          s2, s1
	addi        a0, s2, 56
	call        VectorDestruct

	// *** Basic block 1

	addi        a0, s2, 80
	call        VectorDestruct

	// *** Basic block 2

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEBaseDestruct
.func_end_LexicalScopeDestruct:
	.size LexicalScopeDestruct, .func_end_LexicalScopeDestruct-LexicalScopeDestruct

	.local  NewLexicalScope
	.type NewLexicalScope, @function

NewLexicalScope:

	// *** Basic block 0

	.global malloc
	.local DIEInit
	.local lexical_scope_virtuals
	.global VectorInit
	.global StringInit
	.global StringPrintf
	.local AddDIE
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
	mv          s2, a3
	mv          s3, a4
	mv          s4, a2
	mv          s5, a0
	li          a0, 192		// 0xc0 ASCII \xc0
	call        malloc

	// *** Basic block 1

	mv          s6, a0
	la          a2, lexical_scope_virtuals
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a1, s1
	mv          a0, s6
	call        DIEInit

	// *** Basic block 2

	addi        a0, s6, 56
	call        VectorInit

	// *** Basic block 3

	addi        a0, s6, 80
	call        VectorInit

	// *** Basic block 4

	addi        a0, s6, 112
	addi        t0, s2, 64
	ld          s7, 16(t0)
	mv          a1, s7
	call        StringInit

	// *** Basic block 5

	addi        a0, s6, 152
	mv          a1, x0
	call        StringInit

	// *** Basic block 6

	addi        a0, s6, 152
	lla         a1, .str.25
	addi        t0, s3, 64
	ld          a2, 16(t0)
	mv          a3, s7
	call        StringPrintf

	// *** Basic block 7

	sd          s4, 104(s6)
	mv          a1, s6
	mv          a0, s5
	call        AddDIE

	// *** Basic block 8

	mv          a0, s6

	// *** Basic block 9

.NewLexicalScope_label_91:
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
.func_end_NewLexicalScope:
	.size NewLexicalScope, .func_end_NewLexicalScope-NewLexicalScope

	.local  FunctionDIEDestruct
	.type FunctionDIEDestruct, @function

FunctionDIEDestruct:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local DIEBaseDestruct
	j           DIEBaseDestruct
.func_end_FunctionDIEDestruct:
	.size FunctionDIEDestruct, .func_end_FunctionDIEDestruct-FunctionDIEDestruct

	.local  NewFunctionDIE
	.type NewFunctionDIE, @function

NewFunctionDIE:

	// *** Basic block 0

	.global malloc
	.local NamedDIEInit
	.local function_virtuals
	.local NewLexicalScope
	.local AddDIE
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
	mv          s1, a2
	mv          s2, a1
	mv          s3, a0
	mv          s4, a3
	mv          s5, a4
	li          a0, 112		// 0x70 ASCII 'p'
	call        malloc

	// *** Basic block 1

	mv          s6, a0
	la          a2, function_virtuals
	li          t0, 1		// 0x1 ASCII \x1
	mv          a4, t0
	mv          a3, s1
	li          t0, 46		// 0x2e ASCII '.'
	mv          a1, t0
	mv          a0, s6
	call        NamedDIEInit

	// *** Basic block 2

	sd          s2, 96(s6)
	mv          a4, s5
	mv          a3, s4
	mv          a2, x0
	mv          a1, x0
	mv          a0, s3
	call        NewLexicalScope

	// *** Basic block 3

	sd          a0, 104(s6)
	mv          a1, s6
	mv          a0, s3
	call        AddDIE

	// *** Basic block 4

	mv          a0, s6

	// *** Basic block 5

.NewFunctionDIE_label_69:
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
.func_end_NewFunctionDIE:
	.size NewFunctionDIE, .func_end_NewFunctionDIE-NewFunctionDIE

	.local  StructDIEDestruct
	.type StructDIEDestruct, @function

StructDIEDestruct:

	// *** Basic block 0

	.global VectorDestruct
	.global NamedDIEDestruct
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
	mv          t0, s1
	addi        a0, t0, 104
	call        VectorDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NamedDIEDestruct
.func_end_StructDIEDestruct:
	.size StructDIEDestruct, .func_end_StructDIEDestruct-StructDIEDestruct

	.local  NewStructDIE
	.type NewStructDIE, @function

NewStructDIE:

	// *** Basic block 0

	.global malloc
	.local NamedDIEInit
	.local struct_virtuals
	.global StringInit
	.global VectorInit
	.local AddDIE
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
	mv          s2, a0
	li          a0, 128		// 0x80 ASCII \x80
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	ld          s5, 32(s1)
	lb          t0, 80(s5)
	beqz        t0, .NewStructDIE_label_42

	// *** Basic block 2

	li          s4, 23		// 0x17 ASCII \x17
	j           .NewStructDIE_label_44

	// *** Basic block 3

.NewStructDIE_label_42:
	li          s4, 19		// 0x13 ASCII \x13

	// *** Basic block 4

.NewStructDIE_label_44:
	la          a2, struct_virtuals
	ld          t0, 8(s5)
	ld          s5, 16(t0)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a4, t0
	mv          a3, s5
	mv          a1, s4
	mv          a0, s3
	call        NamedDIEInit

	// *** Basic block 5

	addi        a0, s3, 56
	mv          a1, s5
	call        StringInit

	// *** Basic block 6

	addi        a0, s3, 104
	call        VectorInit

	// *** Basic block 7

	lw          t0, 20(s1)
	sw          t0, 96(s3)
	mv          a1, s3
	mv          a0, s2
	call        AddDIE

	// *** Basic block 8

	mv          a0, s3

	// *** Basic block 9

.NewStructDIE_label_83:
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
.func_end_NewStructDIE:
	.size NewStructDIE, .func_end_NewStructDIE-NewStructDIE

	.local  EnumDIEDestruct
	.type EnumDIEDestruct, @function

EnumDIEDestruct:

	// *** Basic block 0

	.global VectorDestruct
	.global NamedDIEDestruct
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
	mv          t0, s1
	addi        a0, t0, 96
	call        VectorDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NamedDIEDestruct
.func_end_EnumDIEDestruct:
	.size EnumDIEDestruct, .func_end_EnumDIEDestruct-EnumDIEDestruct

	.local  NewEnumDIE
	.type NewEnumDIE, @function

NewEnumDIE:

	// *** Basic block 0

	.global malloc
	.local NamedDIEInit
	.local enum_virtuals
	.global VectorInit
	.local AddDIE
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
	li          a0, 120		// 0x78 ASCII 'x'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          a2, enum_virtuals
	ld          t0, 32(s1)
	ld          t0, 8(t0)
	ld          a3, 16(t0)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a4, t0
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	mv          a0, s3
	call        NamedDIEInit

	// *** Basic block 2

	addi        a0, s3, 96
	call        VectorInit

	// *** Basic block 3

	mv          a1, s3
	mv          a0, s2
	call        AddDIE

	// *** Basic block 4

	mv          a0, s3

	// *** Basic block 5

.NewEnumDIE_label_58:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewEnumDIE:
	.size NewEnumDIE, .func_end_NewEnumDIE-NewEnumDIE

	.local  FindTag
	.type FindTag, @function

FindTag:

	// *** Basic block 0

	.global MapFind
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -32(s0)
	// End of stack frame
	mv          t0, a1
	sd          x0, -32(s0)
	sd          t0, -32(s0)
	ld          a1, -32(s0)
	call        MapFind

	// *** Basic block 1


	// *** Basic block 2

.FindTag_label_23:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_FindTag:
	.size FindTag, .func_end_FindTag-FindTag

	.local  LexicalScopeVisitor
	.type LexicalScopeVisitor, @function

LexicalScopeVisitor:

	// *** Basic block 0

	.local NewLexicalScope
	.global VectorAppend
	.local NewSymbolDIE
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
	mv          s2, a3
	mv          s3, a1
	lw          s4, 0(s1)
	li          t0, 41		// 0x29 ASCII ')'
	bne         s4, t0, .LexicalScopeVisitor_label_96

	// *** Basic block 1

	bnez        s2, .LexicalScopeVisitor_label_79

	// *** Basic block 2

	lw          t0, 16(s3)
	bge         x0, t0, .LexicalScopeVisitor_label_73

	// *** Basic block 3

	mv          s5, s1
	ld          a0, 0(s3)
	ld          a2, 8(s3)
	ld          a3, 64(s5)
	ld          a4, 72(s5)
	li          t0, 11		// 0xb ASCII \xb
	mv          a1, t0
	call        NewLexicalScope

	// *** Basic block 4

	mv          s6, a0
	ld          t0, 8(s3)
	addi        a0, t0, 80
	mv          a1, s6
	call        VectorAppend

	// *** Basic block 5

	sd          s6, 8(s3)

	// *** Basic block 6

.LexicalScopeVisitor_label_73:
	lw          t0, 16(s3)
	addi        t0, t0, 1
	sw          t0, 16(s3)
	j           .LexicalScopeVisitor_label_94

	// *** Basic block 7

.LexicalScopeVisitor_label_79:
	lw          t0, 16(s3)
	addi        t1, t0, -1
	sw          t1, 16(s3)
	bge         x0, t0, .LexicalScopeVisitor_label_93

	// *** Basic block 8

	ld          t0, 8(s3)
	ld          t0, 104(t0)
	sd          t0, 8(s3)

	// *** Basic block 9

.LexicalScopeVisitor_label_93:

	// *** Basic block 10

.LexicalScopeVisitor_label_94:
	j           .LexicalScopeVisitor_label_130

	// *** Basic block 11

.LexicalScopeVisitor_label_96:
	li          t0, 82		// 0x52 ASCII 'R'
	bne         s4, t0, .LexicalScopeVisitor_label_129

	// *** Basic block 12

	bnez        s2, .LexicalScopeVisitor_label_128

	// *** Basic block 13

	mv          s4, s1
	ld          a0, 0(s3)
	ld          a1, 56(s4)
	li          t0, 52		// 0x34 ASCII '4'
	mv          a2, t0
	call        NewSymbolDIE

	// *** Basic block 14

	mv          s7, a0
	ld          t0, 8(s3)
	addi        a0, t0, 56
	mv          a1, s7
	call        VectorAppend

	// *** Basic block 15

	ld          t0, 56(s4)
	sd          s7, 128(t0)

	// *** Basic block 16

.LexicalScopeVisitor_label_128:

	// *** Basic block 17

.LexicalScopeVisitor_label_129:

	// *** Basic block 18

.LexicalScopeVisitor_label_130:
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
.func_end_LexicalScopeVisitor:
	.size LexicalScopeVisitor, .func_end_LexicalScopeVisitor-LexicalScopeVisitor

	.local  BuildLexicalScopes
	.type BuildLexicalScopes, @function

BuildLexicalScopes:

	// *** Basic block 0

	.global ASTNodeVisit
	.local LexicalScopeVisitor
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -48(s0)
	// End of stack frame
	mv          t0, a0
	mv          t1, a2
	mv          t2, a1
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          t0, -48(s0)
	addi        t3, s0, -48
	ld          t4, 104(t1)
	sd          t4, 8(t3)
	addi        t3, s0, -48
	sw          x0, 16(t3)
	addi        t3, t2, 32
	ld          a0, 40(t3)
	addi        a3, s0, -48
	mv          a2, x0
	la          a1, LexicalScopeVisitor
	call        ASTNodeVisit

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BuildLexicalScopes:
	.size BuildLexicalScopes, .func_end_BuildLexicalScopes-BuildLexicalScopes

	.local  NewTypeRecordDIE
	.type NewTypeRecordDIE, @function

NewTypeRecordDIE:

	// *** Basic block 0

	.local NewArrayDIE
	.local NewTypeRecordDIE
	.local NewPointerDIE
	.global TypeIsVoid
	.local NewFunctionDIE
	.local NewSymbolDIE
	.global VectorAppend
	.local BuildLexicalScopes
	.global TypeIsStructOrUnion
	.local FindTag
	.local NewStructDIE
	.local NewMemberDIE
	.global MapInsert
	.global TypeIsEnum
	.local NewEnumDIE
	.local NewEnumConstDIE
	.local PrimitiveToDIE
	.global printf
	.global abort
	.global malloc
	.local DIEInit
	.local cv_virtuals
	sd          a0, -0(s0)	// Spilled @441
	addi sp, sp, -160
	// Saved return address (offset 152) and frame pointer (offset 144)
	sd ra, 152(sp)
	sd s0, 144(sp)
	addi s0, sp, 160
	// Local vars at offset -48(s0)
	// Spilled register region: 16 bytes at -64(s0) to -48(s0)
	// Saved integer registers.
	sd s1, 88(sp)
	sd s2, 80(sp)
	sd s3, 72(sp)
	sd s4, 64(sp)
	sd s5, 56(sp)
	sd s6, 48(sp)
	sd s7, 40(sp)
	sd s8, 32(sp)
	sd s9, 24(sp)
	sd s10, 16(sp)
	sd s11, 8(sp)
	// End of stack frame
	mv          s1, a1
	sd          s1, -64(s0)	// Spilled @49
	mv          s2, a0
	mv          s3, x0
	lw          t0, 16(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .NewTypeRecordDIE_label_188

	// *** Basic block 2

	j           .NewTypeRecordDIE_label_88

	// *** Basic block 3

	j           .NewTypeRecordDIE_label_69

	// *** Basic block 4

	j           .NewTypeRecordDIE_label_102

	// *** Basic block 5

.NewTypeRecordDIE_label_69:
	lw          s7, 32(s1)
	ld          a1, 24(s1)
	mv          a0, s2
	call        NewTypeRecordDIE

	// *** Basic block 6

	mv          a2, a0
	mv          a1, s7
	mv          a0, s2
	call        NewArrayDIE

	// *** Basic block 7

	mv          s3, a0
	j           .NewTypeRecordDIE_label_353

	// *** Basic block 8

.NewTypeRecordDIE_label_88:
	ld          a1, 24(s1)
	mv          a0, s2
	call        NewTypeRecordDIE

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s2
	call        NewPointerDIE

	// *** Basic block 10

	mv          s3, a0
	j           .NewTypeRecordDIE_label_353

	// *** Basic block 11

.NewTypeRecordDIE_label_102:
	addi        s7, s1, 32
	ld          s10, 40(s7)
	mv          a0, x0
	ld          s4, 24(s1)
	mv          a0, s4
	call        TypeIsVoid

	// *** Basic block 12

	not         t0, a0
	beqz        t0, .NewTypeRecordDIE_label_122

	// *** Basic block 13

	mv          a1, s4
	mv          a0, s2
	call        NewTypeRecordDIE

	// *** Basic block 14


	// *** Basic block 15

.NewTypeRecordDIE_label_122:
	ld          t0, 32(s1)
	ld          a2, 16(t0)
	ld          a3, 64(s10)
	ld          a4, 72(s10)
	mv          a1, a0
	mv          a0, s2
	call        NewFunctionDIE

	// *** Basic block 16

	mv          s3, a0
	mv          s4, s3
	mv          s10, x0
	addi        t0, s7, 8
	ld          s5, 8(t0)
	bge         x0, s5, .NewTypeRecordDIE_label_179

	// *** Basic block 17

	ld          s7, 8(s7)

	// *** Basic block 18

.NewTypeRecordDIE_label_151:
	slli        t0, s10, 3
	add         t0, s7, t0
	ld          s7, 0(t0)
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	mv          a1, s7
	mv          a0, s2
	call        NewSymbolDIE

	// *** Basic block 19

	mv          s8, a0
	ld          t0, 104(s4)
	addi        a0, t0, 56
	mv          a1, s8
	call        VectorAppend

	// *** Basic block 20

	sd          s8, 128(s7)

	// *** Basic block 21

.NewTypeRecordDIE_label_175:
	addi        s10, s10, 1
	bge         s10, s5, .NewTypeRecordDIE_label_151

	// *** Basic block 22

.NewTypeRecordDIE_label_179:
	mv          a2, s4
	mv          a1, s1
	mv          a0, s2
	call        BuildLexicalScopes

	// *** Basic block 23

	j           .NewTypeRecordDIE_label_353

	// *** Basic block 24

.NewTypeRecordDIE_label_188:
	mv          a0, s1
	call        TypeIsStructOrUnion

	// *** Basic block 25

	beqz        a0, .NewTypeRecordDIE_label_268

	// *** Basic block 26

	ld          s4, 32(s1)
	ld          t0, 8(s4)
	ld          s5, 16(t0)
	mv          a1, s5
	mv          a0, s2
	call        FindTag

	// *** Basic block 27

	mv          s3, a0
	bne         s3, x0, .NewTypeRecordDIE_label_266

	// *** Basic block 28

	mv          a1, s1
	mv          a0, s2
	call        NewStructDIE

	// *** Basic block 29

	mv          s3, a0
	mv          s6, x0
	addi        t0, s4, 16
	ld          s7, 8(t0)
	bge         x0, s7, .NewTypeRecordDIE_label_243

	// *** Basic block 30

	ld          s4, 16(s4)

	// *** Basic block 31

.NewTypeRecordDIE_label_222:
	slli        t0, s6, 3
	add         t0, s4, t0
	ld          a1, 0(t0)
	mv          a0, s2
	call        NewMemberDIE

	// *** Basic block 32

	mv          s4, a0
	sd          s4, -64(s0)	// Spilled @230
	mv          s8, s3
	sd          s8, -56(s0)	// Spilled @232
	addi        a0, s8, 104
	mv          a1, s4
	call        VectorAppend

	// *** Basic block 33

.NewTypeRecordDIE_label_239:
	addi        s6, s6, 1
	bge         s6, s7, .NewTypeRecordDIE_label_222

	// *** Basic block 34

.NewTypeRecordDIE_label_243:
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          s5, -48(s0)
	addi        t0, s0, -48
	sd          s3, 8(t0)
	addi        sp, sp, -16
	ld          t0, -48(s0)
	sd          t0, 0(sp)
	ld          t0, -40(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s2
	call        MapInsert

	// *** Basic block 35

	addi        sp, sp, 16

	// *** Basic block 36

.NewTypeRecordDIE_label_266:
	j           .NewTypeRecordDIE_label_351

	// *** Basic block 37

.NewTypeRecordDIE_label_268:
	mv          a0, s1
	call        TypeIsEnum

	// *** Basic block 38

	beqz        a0, .NewTypeRecordDIE_label_345

	// *** Basic block 39

	ld          s5, 32(s1)
	ld          t0, 8(s5)
	ld          s7, 16(t0)
	mv          a1, s7
	mv          a0, s2
	call        FindTag

	// *** Basic block 40

	mv          s3, a0
	bne         s3, x0, .NewTypeRecordDIE_label_343

	// *** Basic block 41

	mv          a1, s1
	mv          a0, s2
	call        NewEnumDIE

	// *** Basic block 42

	mv          s3, a0
	mv          s9, x0
	addi        t0, s5, 16
	ld          s10, 8(t0)
	bge         x0, s10, .NewTypeRecordDIE_label_323

	// *** Basic block 43

	ld          s5, 16(s5)

	// *** Basic block 44

.NewTypeRecordDIE_label_302:
	slli        t0, s9, 3
	add         t0, s5, t0
	ld          a1, 0(t0)
	mv          a0, s2
	call        NewEnumConstDIE

	// *** Basic block 45

	mv          s5, a0
	sd          s5, -56(s0)	// Spilled @310
	mv          s11, s3
	addi        a0, s11, 96
	mv          a1, s5
	call        VectorAppend

	// *** Basic block 46

.NewTypeRecordDIE_label_319:
	addi        s9, s9, 1
	bge         s9, s10, .NewTypeRecordDIE_label_302

	// *** Basic block 47

.NewTypeRecordDIE_label_323:
	sd          x0, -32(s0)
	sd          x0, -24(s0)
	sd          s7, -32(s0)
	addi        t0, s0, -32
	sd          s3, 8(t0)
	addi        sp, sp, -16
	ld          t0, -32(s0)
	sd          t0, 0(sp)
	ld          t0, -24(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s2
	call        MapInsert

	// *** Basic block 49

.NewTypeRecordDIE_label_343:
	j           .NewTypeRecordDIE_label_350

	// *** Basic block 50

.NewTypeRecordDIE_label_345:
	mv          a0, s1
	call        PrimitiveToDIE

	// *** Basic block 51

	mv          s3, a0

	// *** Basic block 52

.NewTypeRecordDIE_label_350:

	// *** Basic block 53

.NewTypeRecordDIE_label_351:
	j           .NewTypeRecordDIE_label_353

	// *** Basic block 54

.NewTypeRecordDIE_label_353:
	beq         s3, x0, .NewTypeRecordDIE_label_358

	// *** Basic block 55

	j           .NewTypeRecordDIE_label_373

	// *** Basic block 56

.NewTypeRecordDIE_label_358:
	lla         a0, .str.26
	lla         a1, .str.27
	lla         a3, .str.28
	li          t0, 513		// 0x201
	mv          a2, t0
	call        printf

	// *** Basic block 57

	call        abort

	// *** Basic block 58

.NewTypeRecordDIE_label_373:
	mv          s5, s1
	lw          t0, 12(s5)
	andi        t0, t0, 4
	snez        s7, t0

	// *** Basic block 59

.NewTypeRecordDIE_label_381:
	beqz        s7, .NewTypeRecordDIE_label_404

	// *** Basic block 60

	j           .NewTypeRecordDIE_label_384

	// *** Basic block 61

.NewTypeRecordDIE_label_384:
	li          t0, 64		// 0x40 ASCII '@'
	mv          a0, t0
	call        malloc

	// *** Basic block 62

	mv          s5, a0
	la          a2, cv_virtuals
	mv          a3, x0
	li          t0, 38		// 0x26 ASCII '&'
	mv          a1, t0
	mv          a0, s5
	call        DIEInit

	// *** Basic block 63

	sd          s3, 56(s5)
	mv          s3, s5

	// *** Basic block 64

.NewTypeRecordDIE_label_404:
	mv          s7, s1
	bne         s7, x0, .NewTypeRecordDIE_label_413

	// *** Basic block 65

	mv          s1, x0
	sd          s1, -64(s0)	// Spilled @410
	j           .NewTypeRecordDIE_label_419

	// *** Basic block 66

.NewTypeRecordDIE_label_413:
	lw          t0, 12(s7)
	andi        t0, t0, 8
	snez        s1, t0
	j           .NewTypeRecordDIE_label_419

	// *** Basic block 67

.NewTypeRecordDIE_label_419:
	beqz        s1, .NewTypeRecordDIE_label_440

	// *** Basic block 68

	li          t0, 64		// 0x40 ASCII '@'
	mv          a0, t0
	call        malloc

	// *** Basic block 69

	mv          s1, a0
	la          a2, cv_virtuals
	mv          a3, x0
	li          t0, 53		// 0x35 ASCII '5'
	mv          a1, t0
	mv          a0, s1
	call        DIEInit

	// *** Basic block 70

	sd          s3, 56(s1)
	mv          s3, s1

	// *** Basic block 71

.NewTypeRecordDIE_label_440:
	mv          a0, s3

	// *** Basic block 72

.NewTypeRecordDIE_label_443:
	// Restored registers.
	ld s1, 88(sp)
	ld s2, 80(sp)
	ld s3, 72(sp)
	ld s4, 64(sp)
	ld s5, 56(sp)
	ld s6, 48(sp)
	ld s7, 40(sp)
	ld s8, 32(sp)
	ld s9, 24(sp)
	ld s10, 16(sp)
	ld s11, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewTypeRecordDIE:
	.size NewTypeRecordDIE, .func_end_NewTypeRecordDIE-NewTypeRecordDIE

	.local  NewSymbolDIE
	.type NewSymbolDIE, @function

NewSymbolDIE:

	// *** Basic block 0

	.local NewTypeRecordDIE
	.global malloc
	.local NamedDIEInit
	.local symbol_virtuals
	.local AddDIE
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
	mv          s2, a0
	mv          s3, a2
	ld          t0, 40(s1)
	lw          t0, 16(t0)
	addi        t0, t0, -3
	seqz        t1, t0

	// *** Basic block 1

.NewSymbolDIE_label_38:
	beqz        t1, .NewSymbolDIE_label_53

	// *** Basic block 2

	j           .NewSymbolDIE_label_41

	// *** Basic block 3

.NewSymbolDIE_label_41:
	mv          a1, t0
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           NewTypeRecordDIE

	// *** Basic block 5

.NewSymbolDIE_label_50:
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

.NewSymbolDIE_label_53:
	li          t1, 128		// 0x80 ASCII \x80
	mv          a0, t1
	call        malloc

	// *** Basic block 7

	mv          s4, a0
	la          a2, symbol_virtuals
	ld          a3, 16(s1)
	mv          a4, x0
	mv          a1, s3
	mv          a0, s4
	call        NamedDIEInit

	// *** Basic block 8

	lb          t1, 56(s1)
	slli        t2, t1, 63
	srai        t2, t2, 63
	not         t2, t2
	sb          t2, 105(s4)
	slli        t1, t1, 61
	srai        t1, t1, 63
	sb          t1, 104(s4)
	mv          a1, t0
	mv          a0, s2
	call        NewTypeRecordDIE

	// *** Basic block 9

	sd          a0, 96(s4)
	sw          x0, 112(s4)
	mv          a1, s4
	mv          a0, s2
	call        AddDIE

	// *** Basic block 10

	mv          a0, s4
	j           .NewSymbolDIE_label_50
.func_end_NewSymbolDIE:
	.size NewSymbolDIE, .func_end_NewSymbolDIE-NewSymbolDIE

	.local  NewMemberDIE
	.type NewMemberDIE, @function

NewMemberDIE:

	// *** Basic block 0

	.global malloc
	.local NamedDIEInit
	.local member_virtuals
	.global StructMemberIsBitField
	.local NewTypeRecordDIE
	.local AddDIE
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
	mv          s2, a0
	li          a0, 144		// 0x90 ASCII \x90
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          a2, member_virtuals
	ld          s4, 0(s1)
	ld          a3, 16(s4)
	mv          a4, x0
	li          t0, 13		// 0xd ASCII \xd
	mv          a1, t0
	mv          a0, s3
	call        NamedDIEInit

	// *** Basic block 2

	sb          x0, 105(s3)
	sb          x0, 104(s3)
	lw          t0, 8(s1)
	sw          t0, 128(s3)
	mv          a0, s1
	call        StructMemberIsBitField

	// *** Basic block 3

	sb          a0, 132(s3)
	lw          t0, 12(s1)
	sw          t0, 136(s3)
	lw          t0, 16(s1)
	sw          t0, 140(s3)
	ld          a1, 40(s4)
	mv          a0, s2
	call        NewTypeRecordDIE

	// *** Basic block 4

	sd          a0, 96(s3)
	mv          a1, s3
	mv          a0, s2
	call        AddDIE

	// *** Basic block 5

	mv          a0, s3

	// *** Basic block 6

.NewMemberDIE_label_87:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewMemberDIE:
	.size NewMemberDIE, .func_end_NewMemberDIE-NewMemberDIE

	.local  NewEnumConstDIE
	.type NewEnumConstDIE, @function

NewEnumConstDIE:

	// *** Basic block 0

	.global malloc
	.local NamedDIEInit
	.local enum_const_virtuals
	.local AddDIE
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
	li          a0, 104		// 0x68 ASCII 'h'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          a2, enum_const_virtuals
	ld          a3, 16(s1)
	mv          a4, x0
	li          t0, 40		// 0x28 ASCII '('
	mv          a1, t0
	mv          a0, s3
	call        NamedDIEInit

	// *** Basic block 2

	ld          t0, 112(s1)
	sd          t0, 96(s3)
	mv          a1, s3
	mv          a0, s2
	call        AddDIE

	// *** Basic block 3

	mv          a0, s3

	// *** Basic block 4

.NewEnumConstDIE_label_51:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewEnumConstDIE:
	.size NewEnumConstDIE, .func_end_NewEnumConstDIE-NewEnumConstDIE

	.local  NewAbbreviation
	.type NewAbbreviation, @function

NewAbbreviation:

	// *** Basic block 0

	.global malloc
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
	li          a0, 16		// 0x10 ASCII \x10
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	lw          t0, 112(s1)
	addi        t0, t0, 1
	sw          t0, 112(s1)
	sw          t0, 0(s3)
	sd          s2, 8(s3)
	mv          a0, s3

	// *** Basic block 2

.NewAbbreviation_label_30:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewAbbreviation:
	.size NewAbbreviation, .func_end_NewAbbreviation-NewAbbreviation

	.local  GetOrCreateAbbbreviation
	.type GetOrCreateAbbbreviation, @function

GetOrCreateAbbbreviation:

	// *** Basic block 0

	.global MapFind
	.local NewAbbreviation
	.global VectorAppend
	.global MapInsert
	.global BufferDelete
	.global printf
	.global abort
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
	mv          s1, a1
	mv          s2, a0
	sd          x0, -48(s0)
	sd          s1, -48(s0)
	addi        a0, s2, 32
	ld          a1, -48(s0)
	call        MapFind

	// *** Basic block 1

	mv          s3, a0
	bne         s3, x0, .GetOrCreateAbbbreviation_label_73

	// *** Basic block 2

	mv          a1, s1
	mv          a0, s2
	call        NewAbbreviation

	// *** Basic block 3

	mv          s3, a0
	addi        a0, s2, 64
	mv          a1, s3
	call        VectorAppend

	// *** Basic block 4

	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          s1, -40(s0)
	addi        t0, s0, -40
	sd          s3, 8(t0)
	addi        a0, s2, 32
	addi        sp, sp, -16
	ld          t0, -40(s0)
	sd          t0, 0(sp)
	ld          t0, -32(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 5

	j           .GetOrCreateAbbbreviation_label_77

	// *** Basic block 6

.GetOrCreateAbbbreviation_label_73:
	mv          a0, s1
	call        BufferDelete

	// *** Basic block 7

.GetOrCreateAbbbreviation_label_77:
	beq         s3, x0, .GetOrCreateAbbbreviation_label_82

	// *** Basic block 8

	j           .GetOrCreateAbbbreviation_label_99

	// *** Basic block 9

.GetOrCreateAbbbreviation_label_82:
	lla         a0, .str.29
	lla         a1, .str.30
	lla         a3, .str.31
	li          t0, 612		// 0x264
	mv          a2, t0
	call        printf

	// *** Basic block 10

	call        abort

	// *** Basic block 11

.GetOrCreateAbbbreviation_label_99:
	mv          a0, s3

	// *** Basic block 12

.GetOrCreateAbbbreviation_label_102:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GetOrCreateAbbbreviation:
	.size GetOrCreateAbbbreviation, .func_end_GetOrCreateAbbbreviation-GetOrCreateAbbbreviation

	.global NewDebugAttributeValue
	.type NewDebugAttributeValue, @function

NewDebugAttributeValue:

	// *** Basic block 0

	.global malloc
	.global BufferInit
	.global StringInit
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
	li          a0, 56		// 0x38 ASCII '8'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	sw          s1, 4(s3)
	sw          s2, 0(s3)
	li          t0, 1		// 0x1 ASCII \x1
	beq         s1, t0, .NewDebugAttributeValue_label_86

	// *** Basic block 2

	li          t0, 3		// 0x3 ASCII \x3
	beq         s1, t0, .NewDebugAttributeValue_label_77

	// *** Basic block 3

	li          t0, 4		// 0x4 ASCII \x4
	beq         s1, t0, .NewDebugAttributeValue_label_78

	// *** Basic block 4

	li          t0, 8		// 0x8 ASCII \x8
	beq         s1, t0, .NewDebugAttributeValue_label_84

	// *** Basic block 5

	li          t0, 9		// 0x9 ASCII \x9
	beq         s1, t0, .NewDebugAttributeValue_label_79

	// *** Basic block 6

	li          t0, 10		// 0xa ASCII \xa
	beq         s1, t0, .NewDebugAttributeValue_label_76

	// *** Basic block 7

	li          t0, 14		// 0xe ASCII \xe
	beq         s1, t0, .NewDebugAttributeValue_label_85

	// *** Basic block 8

	li          t0, 255		// 0xff
	beq         s1, t0, .NewDebugAttributeValue_label_87

	// *** Basic block 9

.NewDebugAttributeValue_label_72:
	sd          x0, 16(s3)
	j           .NewDebugAttributeValue_label_94

	// *** Basic block 10

.NewDebugAttributeValue_label_76:

	// *** Basic block 11

.NewDebugAttributeValue_label_77:

	// *** Basic block 12

.NewDebugAttributeValue_label_78:

	// *** Basic block 13

.NewDebugAttributeValue_label_79:
	addi        a0, s3, 16
	call        BufferInit

	// *** Basic block 14

	j           .NewDebugAttributeValue_label_94

	// *** Basic block 15

.NewDebugAttributeValue_label_84:

	// *** Basic block 16

.NewDebugAttributeValue_label_85:

	// *** Basic block 17

.NewDebugAttributeValue_label_86:

	// *** Basic block 18

.NewDebugAttributeValue_label_87:
	addi        a0, s3, 16
	mv          a1, x0
	call        StringInit

	// *** Basic block 19

	j           .NewDebugAttributeValue_label_94

	// *** Basic block 20

.NewDebugAttributeValue_label_94:
	mv          a0, s3

	// *** Basic block 21

.NewDebugAttributeValue_label_97:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewDebugAttributeValue:
	.size NewDebugAttributeValue, .func_end_NewDebugAttributeValue-NewDebugAttributeValue

	.global DebugAttributeValueDestruct
	.type DebugAttributeValueDestruct, @function

DebugAttributeValueDestruct:

	// *** Basic block 0

	.global BufferDestruct
	.global StringDestruct
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
	lw          s2, 4(s1)
	li          t0, 1		// 0x1 ASCII \x1
	beq         s2, t0, .DebugAttributeValueDestruct_label_72

	// *** Basic block 1

	li          t0, 3		// 0x3 ASCII \x3
	beq         s2, t0, .DebugAttributeValueDestruct_label_63

	// *** Basic block 2

	li          t0, 4		// 0x4 ASCII \x4
	beq         s2, t0, .DebugAttributeValueDestruct_label_64

	// *** Basic block 3

	li          t0, 8		// 0x8 ASCII \x8
	beq         s2, t0, .DebugAttributeValueDestruct_label_70

	// *** Basic block 4

	li          t0, 9		// 0x9 ASCII \x9
	beq         s2, t0, .DebugAttributeValueDestruct_label_65

	// *** Basic block 5

	li          t0, 10		// 0xa ASCII \xa
	beq         s2, t0, .DebugAttributeValueDestruct_label_62

	// *** Basic block 6

	li          t0, 14		// 0xe ASCII \xe
	beq         s2, t0, .DebugAttributeValueDestruct_label_71

	// *** Basic block 7

	li          t0, 255		// 0xff
	beq         s2, t0, .DebugAttributeValueDestruct_label_73

	// *** Basic block 8

.DebugAttributeValueDestruct_label_60:
	j           .DebugAttributeValueDestruct_label_78

	// *** Basic block 9

.DebugAttributeValueDestruct_label_62:

	// *** Basic block 10

.DebugAttributeValueDestruct_label_63:

	// *** Basic block 11

.DebugAttributeValueDestruct_label_64:

	// *** Basic block 12

.DebugAttributeValueDestruct_label_65:
	addi        a0, s1, 16
	call        BufferDestruct

	// *** Basic block 13

	j           .DebugAttributeValueDestruct_label_78

	// *** Basic block 14

.DebugAttributeValueDestruct_label_70:

	// *** Basic block 15

.DebugAttributeValueDestruct_label_71:

	// *** Basic block 16

.DebugAttributeValueDestruct_label_72:

	// *** Basic block 17

.DebugAttributeValueDestruct_label_73:
	addi        a0, s1, 16
	call        StringDestruct

	// *** Basic block 18

	j           .DebugAttributeValueDestruct_label_78

	// *** Basic block 19

.DebugAttributeValueDestruct_label_78:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DebugAttributeValueDestruct:
	.size DebugAttributeValueDestruct, .func_end_DebugAttributeValueDestruct-DebugAttributeValueDestruct

	.local  AddAttributeAndValue
	.type AddAttributeAndValue, @function

AddAttributeAndValue:

	// *** Basic block 0

	.global VectorAppend
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	addi        a0, t0, 24
	mv          a1, a2
	j           VectorAppend
.func_end_AddAttributeAndValue:
	.size AddAttributeAndValue, .func_end_AddAttributeAndValue-AddAttributeAndValue

	.local  SignedConstant
	.type SignedConstant, @function

SignedConstant:

	// *** Basic block 0

	.global NewDebugAttributeValue
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
	mv          s2, a0
	mv          s3, s1
	bge         s1, x0, .SignedConstant_label_32

	// *** Basic block 1

	neg         s3, s3

	// *** Basic block 2

.SignedConstant_label_32:
	li          t0, 256		// 0x100
	bge         s3, t0, .SignedConstant_label_46

	// *** Basic block 3

	li          t0, 11		// 0xb ASCII \xb
	mv          a1, t0
	mv          a0, s2
	call        NewDebugAttributeValue

	// *** Basic block 4

	mv          s4, a0
	j           .SignedConstant_label_82

	// *** Basic block 5

.SignedConstant_label_46:
	li          t0, 65536		// 0x10000
	bge         s3, t0, .SignedConstant_label_59

	// *** Basic block 6

	li          t0, 5		// 0x5 ASCII \x5
	mv          a1, t0
	mv          a0, s2
	call        NewDebugAttributeValue

	// *** Basic block 7

	mv          s4, a0
	j           .SignedConstant_label_81

	// *** Basic block 8

.SignedConstant_label_59:
	li          t0, 4294967296		// 0x100000000
	bge         s3, t0, .SignedConstant_label_72

	// *** Basic block 9

	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	mv          a0, s2
	call        NewDebugAttributeValue

	// *** Basic block 10

	mv          s4, a0
	j           .SignedConstant_label_80

	// *** Basic block 11

.SignedConstant_label_72:
	li          t0, 7		// 0x7 ASCII \x7
	mv          a1, t0
	mv          a0, s2
	call        NewDebugAttributeValue

	// *** Basic block 12

	mv          s4, a0

	// *** Basic block 13

.SignedConstant_label_80:

	// *** Basic block 14

.SignedConstant_label_81:

	// *** Basic block 15

.SignedConstant_label_82:
	lw          t0, 4(s4)
	li          t1, 5		// 0x5 ASCII \x5
	blt         t0, t1, .SignedConstant_label_140

	// *** Basic block 16

	li          t1, 11		// 0xb ASCII \xb
	blt         t1, t0, .SignedConstant_label_140

	// *** Basic block 17

	addi        t0, t0, -5
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 18

	j           .SignedConstant_label_115

	// *** Basic block 19

	j           .SignedConstant_label_124

	// *** Basic block 20

	j           .SignedConstant_label_133

	// *** Basic block 21

	j           .SignedConstant_label_140

	// *** Basic block 22

	j           .SignedConstant_label_140

	// *** Basic block 23

	j           .SignedConstant_label_140

	// *** Basic block 24

	j           .SignedConstant_label_106

	// *** Basic block 25

.SignedConstant_label_106:
	andi        t0, s1, 255
	sb          t0, 16(s4)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 8(s4)
	j           .SignedConstant_label_142

	// *** Basic block 26

.SignedConstant_label_115:
	addi        t0, s4, 16
	li          t1, 65535		// 0xffff
	and         t1, s1, t1
	sh          t1, 0(t0)
	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 8(s4)
	j           .SignedConstant_label_142

	// *** Basic block 27

.SignedConstant_label_124:
	li          t0, 4294967295		// 0xffffffff
	and         t0, s1, t0
	sw          t0, 16(s4)
	li          t0, 4		// 0x4 ASCII \x4
	sw          t0, 8(s4)
	j           .SignedConstant_label_142

	// *** Basic block 28

.SignedConstant_label_133:
	sd          s1, 16(s4)
	li          t0, 8		// 0x8 ASCII \x8
	sw          t0, 8(s4)
	j           .SignedConstant_label_142

	// *** Basic block 29

.SignedConstant_label_140:
	j           .SignedConstant_label_142

	// *** Basic block 30

.SignedConstant_label_142:
	mv          a0, s4

	// *** Basic block 31

.SignedConstant_label_145:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SignedConstant:
	.size SignedConstant, .func_end_SignedConstant-SignedConstant

	.local  UnsignedConstant
	.type UnsignedConstant, @function

UnsignedConstant:

	// *** Basic block 0

	.global NewDebugAttributeValue
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
	li          t0, 256		// 0x100
	bge         s1, t0, .UnsignedConstant_label_38

	// *** Basic block 1

	li          t0, 11		// 0xb ASCII \xb
	mv          a1, t0
	mv          a0, s2
	call        NewDebugAttributeValue

	// *** Basic block 2

	mv          s3, a0
	j           .UnsignedConstant_label_74

	// *** Basic block 3

.UnsignedConstant_label_38:
	li          t0, 65536		// 0x10000
	bge         s1, t0, .UnsignedConstant_label_51

	// *** Basic block 4

	li          t0, 5		// 0x5 ASCII \x5
	mv          a1, t0
	mv          a0, s2
	call        NewDebugAttributeValue

	// *** Basic block 5

	mv          s3, a0
	j           .UnsignedConstant_label_73

	// *** Basic block 6

.UnsignedConstant_label_51:
	li          t0, 4294967296		// 0x100000000
	bge         s1, t0, .UnsignedConstant_label_64

	// *** Basic block 7

	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	mv          a0, s2
	call        NewDebugAttributeValue

	// *** Basic block 8

	mv          s3, a0
	j           .UnsignedConstant_label_72

	// *** Basic block 9

.UnsignedConstant_label_64:
	li          t0, 7		// 0x7 ASCII \x7
	mv          a1, t0
	mv          a0, s2
	call        NewDebugAttributeValue

	// *** Basic block 10

	mv          s3, a0

	// *** Basic block 11

.UnsignedConstant_label_72:

	// *** Basic block 12

.UnsignedConstant_label_73:

	// *** Basic block 13

.UnsignedConstant_label_74:
	lw          t0, 4(s3)
	li          t1, 5		// 0x5 ASCII \x5
	blt         t0, t1, .UnsignedConstant_label_133

	// *** Basic block 14

	li          t1, 11		// 0xb ASCII \xb
	blt         t1, t0, .UnsignedConstant_label_133

	// *** Basic block 15

	addi        t0, t0, -5
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 16

	j           .UnsignedConstant_label_108

	// *** Basic block 17

	j           .UnsignedConstant_label_117

	// *** Basic block 18

	j           .UnsignedConstant_label_126

	// *** Basic block 19

	j           .UnsignedConstant_label_133

	// *** Basic block 20

	j           .UnsignedConstant_label_133

	// *** Basic block 21

	j           .UnsignedConstant_label_133

	// *** Basic block 22

	j           .UnsignedConstant_label_99

	// *** Basic block 23

.UnsignedConstant_label_99:
	andi        t0, s1, 255
	sb          t0, 16(s3)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 8(s3)
	j           .UnsignedConstant_label_135

	// *** Basic block 24

.UnsignedConstant_label_108:
	addi        t0, s3, 16
	li          t1, 65535		// 0xffff
	and         t1, s1, t1
	sh          t1, 0(t0)
	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 8(s3)
	j           .UnsignedConstant_label_135

	// *** Basic block 25

.UnsignedConstant_label_117:
	li          t0, 4294967295		// 0xffffffff
	and         t0, s1, t0
	sw          t0, 16(s3)
	li          t0, 4		// 0x4 ASCII \x4
	sw          t0, 8(s3)
	j           .UnsignedConstant_label_135

	// *** Basic block 26

.UnsignedConstant_label_126:
	sd          s1, 16(s3)
	li          t0, 8		// 0x8 ASCII \x8
	sw          t0, 8(s3)
	j           .UnsignedConstant_label_135

	// *** Basic block 27

.UnsignedConstant_label_133:
	j           .UnsignedConstant_label_135

	// *** Basic block 28

.UnsignedConstant_label_135:
	mv          a0, s3

	// *** Basic block 29

.UnsignedConstant_label_138:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_UnsignedConstant:
	.size UnsignedConstant, .func_end_UnsignedConstant-UnsignedConstant

	.local  AddressConstant
	.type AddressConstant, @function

AddressConstant:

	// *** Basic block 0

	.global NewDebugAttributeValue
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
	mv          s1, a2
	call        NewDebugAttributeValue

	// *** Basic block 1

	mv          s2, a0
	addi        a0, s2, 16
	mv          a1, s1
	call        StringInit

	// *** Basic block 2

	li          t0, 8		// 0x8 ASCII \x8
	sw          t0, 8(s2)
	mv          a0, s2

	// *** Basic block 3

.AddressConstant_label_34:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AddressConstant:
	.size AddressConstant, .func_end_AddressConstant-AddressConstant

	.local  StringConstant
	.type StringConstant, @function

StringConstant:

	// *** Basic block 0

	.global strlen
	.global NewDebugAttributeValue
	.global StringSet
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
	mv          s2, a0
	li          s3, 8		// 0x8 ASCII \x8
	mv          a0, s1
	call        strlen

	// *** Basic block 1

	addi        s4, a0, 1
	li          s5, 4		// 0x4 ASCII \x4
	bge         s5, s4, .StringConstant_label_32

	// *** Basic block 2

	li          s3, 14		// 0xe ASCII \xe

	// *** Basic block 3

.StringConstant_label_32:
	mv          a1, s3
	mv          a0, s2
	call        NewDebugAttributeValue

	// *** Basic block 4

	mv          s4, a0
	li          t0, 8		// 0x8 ASCII \x8
	bne         s3, t0, .StringConstant_label_54

	// *** Basic block 5

	mv          a0, s1
	call        strlen

	// *** Basic block 6

	sext.w      t0, a0
	addi        t0, t0, 1
	sw          t0, 8(s4)
	j           .StringConstant_label_58

	// *** Basic block 7

.StringConstant_label_54:
	sw          s5, 8(s4)

	// *** Basic block 8

.StringConstant_label_58:
	addi        a0, s4, 16
	mv          a1, s1
	call        StringSet

	// *** Basic block 9

	mv          a0, s4

	// *** Basic block 10

.StringConstant_label_66:
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
.func_end_StringConstant:
	.size StringConstant, .func_end_StringConstant-StringConstant

	.local  DIEReference
	.type DIEReference, @function

DIEReference:

	// *** Basic block 0

	.global NewDebugAttributeValue
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
	li          a1, 19		// 0x13 ASCII \x13
	call        NewDebugAttributeValue

	// *** Basic block 1

	mv          s2, a0
	sd          s1, 16(s2)
	li          t0, 8		// 0x8 ASCII \x8
	sw          t0, 8(s2)
	mv          a0, s2

	// *** Basic block 2

.DIEReference_label_28:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DIEReference:
	.size DIEReference, .func_end_DIEReference-DIEReference

	.local  Flag
	.type Flag, @function

Flag:

	// *** Basic block 0

	.global NewDebugAttributeValue
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
	li          a1, 12		// 0xc ASCII \xc
	call        NewDebugAttributeValue

	// *** Basic block 1

	mv          s2, a0
	sb          s1, 16(s2)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 8(s2)
	mv          a0, s2

	// *** Basic block 2

.Flag_label_29:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Flag:
	.size Flag, .func_end_Flag-Flag

	.global DebugAttributeValueDelete
	.type DebugAttributeValueDelete, @function

DebugAttributeValueDelete:

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
	.global DebugAttributeValueDestruct
	.global free
	mv          s1, a0
	call        DebugAttributeValueDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_DebugAttributeValueDelete:
	.size DebugAttributeValueDelete, .func_end_DebugAttributeValueDelete-DebugAttributeValueDelete

	.local  FlushAccumulator
	.type FlushAccumulator, @function

FlushAccumulator:

	// *** Basic block 0

	.global fprintf
	.global BufferClear
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
	ld          s2, 264(s1)
	mv          s3, x0
	addi        t0, s1, 272
	ld          s4, 8(t0)
	bge         x0, s4, .FlushAccumulator_label_45

	// *** Basic block 1

	ld          s5, 272(s1)

	// *** Basic block 2

.FlushAccumulator_label_28:
	lla         a1, .str.32
	add         t0, s5, s3
	lb          t0, 0(t0)
	andi        a2, t0, -1
	mv          a0, s2
	call        fprintf

	// *** Basic block 3

.FlushAccumulator_label_41:
	addi        s3, s3, 1
	bge         s3, s4, .FlushAccumulator_label_28

	// *** Basic block 4

.FlushAccumulator_label_45:
	addi        a0, s1, 272
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BufferClear
.func_end_FlushAccumulator:
	.size FlushAccumulator, .func_end_FlushAccumulator-FlushAccumulator

	.local  WriteUnsignedLEB128
	.type WriteUnsignedLEB128, @function

WriteUnsignedLEB128:

	// *** Basic block 0

	.global BufferAppendByte
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

	// *** Basic block 1

.WriteUnsignedLEB128_label_14:
	andi        s3, s1, 127
	srli        s1, s1, 7
	beqz        s1, .WriteUnsignedLEB128_label_24

	// *** Basic block 2

	ori         s3, s3, -128

	// *** Basic block 3

.WriteUnsignedLEB128_label_24:
	mv          a1, s3
	mv          a0, s2
	call        BufferAppendByte

	// *** Basic block 4

.WriteUnsignedLEB128_label_30:
	bnez        s1, .WriteUnsignedLEB128_label_14

	// *** Basic block 5

.WriteUnsignedLEB128_label_32:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_WriteUnsignedLEB128:
	.size WriteUnsignedLEB128, .func_end_WriteUnsignedLEB128-WriteUnsignedLEB128

	.local  WriteSignedLEB128
	.type WriteSignedLEB128, @function

WriteSignedLEB128:

	// *** Basic block 0

	.global BufferAppendByte
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
	mv          s2, a0
	li          s3, 1		// 0x1 ASCII \x1

	// *** Basic block 1

.WriteSignedLEB128_label_22:
	andi        s4, s1, 127
	srai        s1, s1, 7
	seqz        t0, s1
	bnez        s1, .WriteSignedLEB128_label_35

	// *** Basic block 2

	andi        t1, s4, 64
	seqz        t0, t1

	// *** Basic block 3

.WriteSignedLEB128_label_35:
	bnez        t0, .WriteSignedLEB128_label_47

	// *** Basic block 4

	addi        t1, s1, 1
	seqz        t0, t1
	li          t1, -1		// 0xffffffffffffffff
	bne         s1, t1, .WriteSignedLEB128_label_46

	// *** Basic block 5

	andi        t1, s4, 64
	addi        t1, t1, -64
	seqz        t0, t1

	// *** Basic block 6

.WriteSignedLEB128_label_46:

	// *** Basic block 7

.WriteSignedLEB128_label_47:
	beqz        t0, .WriteSignedLEB128_label_52

	// *** Basic block 8

	mv          s3, x0
	j           .WriteSignedLEB128_label_54

	// *** Basic block 9

.WriteSignedLEB128_label_52:
	ori         s4, s4, -128

	// *** Basic block 10

.WriteSignedLEB128_label_54:
	mv          a1, s4
	mv          a0, s2
	call        BufferAppendByte

	// *** Basic block 11

	bnez        s3, .WriteSignedLEB128_label_22

	// *** Basic block 12

.WriteSignedLEB128_label_61:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_WriteSignedLEB128:
	.size WriteSignedLEB128, .func_end_WriteSignedLEB128-WriteSignedLEB128

	.local  WriteByte
	.type WriteByte, @function

WriteByte:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global BufferAppendByte
	j           BufferAppendByte
.func_end_WriteByte:
	.size WriteByte, .func_end_WriteByte-WriteByte

	.local  DebugAttributeValueEmit
	.type DebugAttributeValueEmit, @function

DebugAttributeValueEmit:

	// *** Basic block 0

	.local WriteByte
	.local FlushAccumulator
	.global fprintf
	.global BufferAppend
	.global printf
	.global abort
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
	addi        s3, s1, 272
	lw          s4, 4(s2)
	li          t0, 1		// 0x1 ASCII \x1
	beq         s4, t0, .DebugAttributeValueEmit_label_319

	// *** Basic block 1

	li          t0, 5		// 0x5 ASCII \x5
	beq         s4, t0, .DebugAttributeValueEmit_label_129

	// *** Basic block 2

	li          t0, 6		// 0x6 ASCII \x6
	beq         s4, t0, .DebugAttributeValueEmit_label_148

	// *** Basic block 3

	li          t0, 7		// 0x7 ASCII \x7
	beq         s4, t0, .DebugAttributeValueEmit_label_183

	// *** Basic block 4

	li          t0, 8		// 0x8 ASCII \x8
	beq         s4, t0, .DebugAttributeValueEmit_label_274

	// *** Basic block 5

	li          t0, 11		// 0xb ASCII \xb
	beq         s4, t0, .DebugAttributeValueEmit_label_121

	// *** Basic block 6

	li          t0, 12		// 0xc ASCII \xc
	beq         s4, t0, .DebugAttributeValueEmit_label_266

	// *** Basic block 7

	li          t0, 14		// 0xe ASCII \xe
	beq         s4, t0, .DebugAttributeValueEmit_label_290

	// *** Basic block 8

	li          t0, 19		// 0x13 ASCII \x13
	beq         s4, t0, .DebugAttributeValueEmit_label_250

	// *** Basic block 9

	li          t0, 255		// 0xff
	beq         s4, t0, .DebugAttributeValueEmit_label_335

	// *** Basic block 10

.DebugAttributeValueEmit_label_102:
	lla         a0, .str.38
	lla         a1, .str.39
	lla         a3, .str.40
	li          t0, 881		// 0x371
	mv          a2, t0
	call        printf

	// *** Basic block 11

	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           abort

	// *** Basic block 13

.DebugAttributeValueEmit_label_121:
	lbu         a1, 16(s2)
	mv          a0, s3
	call        WriteByte

	// *** Basic block 14

	j           .DebugAttributeValueEmit_label_351

	// *** Basic block 15

.DebugAttributeValueEmit_label_129:
	lhu         s4, 16(s2)
	andi        t0, s4, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 16

	srli        t0, s4, 8
	andi        t0, t0, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 17

	j           .DebugAttributeValueEmit_label_351

	// *** Basic block 18

.DebugAttributeValueEmit_label_148:
	lwu         s4, 16(s2)
	andi        t0, s4, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 19

	srli        t0, s4, 8
	andi        t0, t0, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 20

	srli        t0, s4, 16
	andi        t0, t0, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 21

	srli        t0, s4, 24
	andi        t0, t0, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 22

	j           .DebugAttributeValueEmit_label_351

	// *** Basic block 23

.DebugAttributeValueEmit_label_183:
	ld          s4, 16(s2)
	andi        t0, s4, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 24

	srli        t0, s4, 8
	andi        t0, t0, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 25

	srli        t0, s4, 16
	andi        t0, t0, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 26

	srli        t0, s4, 24
	andi        t0, t0, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 27

	srli        t0, s4, 32
	andi        t0, t0, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 28

	srli        t0, s4, 40
	andi        t0, t0, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 29

	srli        t0, s4, 48
	andi        t0, t0, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 30

	srli        t0, s4, 56
	andi        t0, t0, 255
	slli        t0, t0, 56
	srai        a1, t0, 56
	mv          a0, s3
	call        WriteByte

	// *** Basic block 31

	j           .DebugAttributeValueEmit_label_351

	// *** Basic block 32

.DebugAttributeValueEmit_label_250:
	mv          a0, s1
	call        FlushAccumulator

	// *** Basic block 33

	ld          a0, 264(s1)
	lla         a1, .str.33
	ld          t0, 16(s2)
	lw          a2, 0(t0)
	call        fprintf

	// *** Basic block 34

	j           .DebugAttributeValueEmit_label_351

	// *** Basic block 35

.DebugAttributeValueEmit_label_266:
	lb          a1, 16(s2)
	mv          a0, s3
	call        WriteByte

	// *** Basic block 36

	j           .DebugAttributeValueEmit_label_351

	// *** Basic block 37

.DebugAttributeValueEmit_label_274:
	mv          a0, s1
	call        FlushAccumulator

	// *** Basic block 38

	ld          a0, 264(s1)
	lla         a1, .str.34
	addi        t0, s2, 16
	ld          a2, 16(t0)
	call        fprintf

	// *** Basic block 39

	j           .DebugAttributeValueEmit_label_351

	// *** Basic block 40

.DebugAttributeValueEmit_label_290:
	mv          a0, s1
	call        FlushAccumulator

	// *** Basic block 41

	addi        t0, s1, 88
	ld          s4, 8(t0)
	ld          a0, 264(s1)
	lla         a1, .str.35
	mv          a2, s4
	call        fprintf

	// *** Basic block 42

	addi        a0, s1, 88
	addi        t0, s2, 16
	ld          a1, 16(t0)
	ld          t0, 24(t0)
	addi        a2, t0, 1
	call        BufferAppend

	// *** Basic block 43

	j           .DebugAttributeValueEmit_label_351

	// *** Basic block 44

.DebugAttributeValueEmit_label_319:
	mv          a0, s1
	call        FlushAccumulator

	// *** Basic block 45

	ld          a0, 264(s1)
	lla         a1, .str.36
	addi        t0, s2, 16
	ld          a2, 16(t0)
	call        fprintf

	// *** Basic block 46

	j           .DebugAttributeValueEmit_label_351

	// *** Basic block 47

.DebugAttributeValueEmit_label_335:
	mv          a0, s1
	call        FlushAccumulator

	// *** Basic block 48

	ld          a0, 264(s1)
	lla         a1, .str.37
	addi        t0, s2, 16
	ld          a2, 16(t0)
	call        fprintf

	// *** Basic block 49

	j           .DebugAttributeValueEmit_label_351

	// *** Basic block 50

.DebugAttributeValueEmit_label_351:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DebugAttributeValueEmit:
	.size DebugAttributeValueEmit, .func_end_DebugAttributeValueEmit-DebugAttributeValueEmit

	.local  GenerateAbbreviationSignature
	.type GenerateAbbreviationSignature, @function

GenerateAbbreviationSignature:

	// *** Basic block 0

	.local WriteUnsignedLEB128
	.local WriteByte
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
	mv          s2, a0
	lw          a1, 4(s2)
	mv          a0, s1
	call        WriteUnsignedLEB128

	// *** Basic block 1

	lb          a1, 48(s2)
	mv          a0, s1
	call        WriteByte

	// *** Basic block 2

	mv          s3, x0
	addi        t0, s2, 24
	ld          s4, 8(t0)
	bge         x0, s4, .GenerateAbbreviationSignature_label_76

	// *** Basic block 3

	ld          s5, 24(s2)

	// *** Basic block 4

.GenerateAbbreviationSignature_label_40:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          s2, 0(t0)
	lw          a1, 0(s2)
	mv          a0, s1
	call        WriteUnsignedLEB128

	// *** Basic block 5

	lw          s2, 4(s2)
	li          t0, 255		// 0xff
	bne         s2, t0, .GenerateAbbreviationSignature_label_65

	// *** Basic block 6

	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	mv          a0, s1
	call        WriteUnsignedLEB128

	// *** Basic block 7

	j           .GenerateAbbreviationSignature_label_71

	// *** Basic block 8

.GenerateAbbreviationSignature_label_65:
	mv          a1, s2
	mv          a0, s1
	call        WriteUnsignedLEB128

	// *** Basic block 9

.GenerateAbbreviationSignature_label_71:

	// *** Basic block 10

.GenerateAbbreviationSignature_label_72:
	addi        s3, s3, 1
	bge         s3, s4, .GenerateAbbreviationSignature_label_40

	// *** Basic block 11

.GenerateAbbreviationSignature_label_76:
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
.func_end_GenerateAbbreviationSignature:
	.size GenerateAbbreviationSignature, .func_end_GenerateAbbreviationSignature-GenerateAbbreviationSignature

	.global DIEAllocateAbbreviation
	.type DIEAllocateAbbreviation, @function

DIEAllocateAbbreviation:

	// *** Basic block 0

	.global NewBuffer
	.local GenerateAbbreviationSignature
	.local GetOrCreateAbbbreviation
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
	call        NewBuffer

	// *** Basic block 1

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        GenerateAbbreviationSignature

	// *** Basic block 2

	mv          a1, s3
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GetOrCreateAbbbreviation
.func_end_DIEAllocateAbbreviation:
	.size DIEAllocateAbbreviation, .func_end_DIEAllocateAbbreviation-DIEAllocateAbbreviation

	.local  DIEBaseEmit
	.type DIEBaseEmit, @function

DIEBaseEmit:

	// *** Basic block 0

	.local FlushAccumulator
	.global fprintf
	.global DW_TAGString
	.local WriteUnsignedLEB128
	.local DebugAttributeValueEmit
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
	call        FlushAccumulator

	// *** Basic block 1

	ld          s3, 264(s1)
	lla         s4, .str.41
	lw          s5, 0(s2)
	lw          a0, 4(s2)
	call        DW_TAGString

	// *** Basic block 2

	ld          t0, 16(s2)
	lw          s6, 0(t0)
	mv          a4, s6
	mv          a3, a0
	mv          a2, s5
	mv          a1, s4
	mv          a0, s3
	call        fprintf

	// *** Basic block 3

	addi        a0, s1, 272
	mv          a1, s6
	call        WriteUnsignedLEB128

	// *** Basic block 4

	mv          s3, x0
	addi        t0, s2, 24
	ld          s4, 8(t0)
	bge         x0, s4, .DIEBaseEmit_label_79

	// *** Basic block 5

	ld          s5, 24(s2)

	// *** Basic block 6

.DIEBaseEmit_label_66:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        DebugAttributeValueEmit

	// *** Basic block 7

.DIEBaseEmit_label_75:
	addi        s3, s3, 1
	bge         s3, s4, .DIEBaseEmit_label_66

	// *** Basic block 8

.DIEBaseEmit_label_79:
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
.func_end_DIEBaseEmit:
	.size DIEBaseEmit, .func_end_DIEBaseEmit-DIEBaseEmit

	.local  DIEEmit
	.type DIEEmit, @function

DIEEmit:

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
	mv          s1, a1
	mv          s2, a0
	beq         s1, x0, .DIEEmit_label_24

	// *** Basic block 1

	j           .DIEEmit_label_41

	// *** Basic block 2

.DIEEmit_label_24:
	lla         a0, .str.42
	lla         a1, .str.43
	lla         a3, .str.44
	li          t0, 918		// 0x396
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.DIEEmit_label_41:
	lb          t0, 49(s1)
	beqz        t0, .DIEEmit_label_48

	// *** Basic block 5

.DIEEmit_label_45:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.DIEEmit_label_48:
	ld          t0, 8(s1)
	ld          t0, 24(t0)
	mv          a1, s1
	mv          a0, s2
	jalr         x1, t0, 0

	// *** Basic block 7

	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 49(s1)
	j           .DIEEmit_label_45
.func_end_DIEEmit:
	.size DIEEmit, .func_end_DIEEmit-DIEEmit

	.local  BaseTypeDIEBuild
	.type BaseTypeDIEBuild, @function

BaseTypeDIEBuild:

	// *** Basic block 0

	.local AddAttributeAndValue
	.local SignedConstant
	.local StringConstant
	.global DIEAllocateAbbreviation
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
	mv          s3, s1
	lw          a1, 76(s3)
	li          a0, 11		// 0xb ASCII \xb
	call        SignedConstant

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 2

	lw          a1, 72(s3)
	li          t0, 62		// 0x3e ASCII '>'
	mv          a0, t0
	call        SignedConstant

	// *** Basic block 3

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 4

	ld          a1, 64(s3)
	li          t0, 3		// 0x3 ASCII \x3
	mv          a0, t0
	call        StringConstant

	// *** Basic block 5

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 6

	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEAllocateAbbreviation
.func_end_BaseTypeDIEBuild:
	.size BaseTypeDIEBuild, .func_end_BaseTypeDIEBuild-BaseTypeDIEBuild

	.local  DoIndent
	.type DoIndent, @function

DoIndent:

	// *** Basic block 0

	.global printf
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
	mv          t0, s1
	addi        s1, s1, -1
	bge         x0, t0, .DoIndent_label_26

	// *** Basic block 1

.DoIndent_label_16:
	lla         a0, .str.45
	call        printf

	// *** Basic block 2

	mv          t0, s1
	blt         x0, t0, .DoIndent_label_16

	// *** Basic block 3

.DoIndent_label_26:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DoIndent:
	.size DoIndent, .func_end_DoIndent-DoIndent

	.local  BaseTypeDIEPrint
	.type BaseTypeDIEPrint, @function

BaseTypeDIEPrint:

	// *** Basic block 0

	.local DoIndent
	.global printf
	.global DW_TAGString
	.global DW_ATEString
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
	mv          a0, a1
	call        DoIndent

	// *** Basic block 1

	mv          s2, s1
	lla         s3, .str.46
	lw          a0, 4(s1)
	call        DW_TAGString

	// *** Basic block 2

	lw          a0, 72(s2)
	call        DW_ATEString

	// *** Basic block 3

	lw          a3, 76(s2)
	ld          a4, 64(s2)
	mv          a2, a0
	mv          a1, a0
	mv          a0, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           printf
.func_end_BaseTypeDIEPrint:
	.size BaseTypeDIEPrint, .func_end_BaseTypeDIEPrint-BaseTypeDIEPrint

	.local  BaseTypeDIEEmit
	.type BaseTypeDIEEmit, @function

BaseTypeDIEEmit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local DIEBaseEmit
	j           DIEBaseEmit
.func_end_BaseTypeDIEEmit:
	.size BaseTypeDIEEmit, .func_end_BaseTypeDIEEmit-BaseTypeDIEEmit

	.local  EmitEndOfChildren
	.type EmitEndOfChildren, @function

EmitEndOfChildren:

	// *** Basic block 0

	.local WriteByte
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        a0, t0, 272
	mv          a1, x0
	j           WriteByte
.func_end_EmitEndOfChildren:
	.size EmitEndOfChildren, .func_end_EmitEndOfChildren-EmitEndOfChildren

	.local  SubrangeDIEBuild
	.type SubrangeDIEBuild, @function

SubrangeDIEBuild:

	// *** Basic block 0

	.local AddAttributeAndValue
	.local DIEReference
	.global DIEAllocateAbbreviation
	.global DIEBuild
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
	mv          t0, s1
	ld          s3, 64(t0)
	mv          a1, s3
	li          a0, 73		// 0x49 ASCII 'I'
	call        DIEReference

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 2

	mv          a1, s1
	mv          a0, s2
	call        DIEAllocateAbbreviation

	// *** Basic block 3

	mv          a1, s3
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEBuild
.func_end_SubrangeDIEBuild:
	.size SubrangeDIEBuild, .func_end_SubrangeDIEBuild-SubrangeDIEBuild

	.local  SubrangeDIEPrint
	.type SubrangeDIEPrint, @function

SubrangeDIEPrint:

	// *** Basic block 0

	.local DoIndent
	.global printf
	.global DW_TAGString
	.global DIEPrint
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
	mv          s2, a0
	mv          a0, s1
	call        DoIndent

	// *** Basic block 1

	mv          s3, s2
	lla         s4, .str.47
	lw          a0, 4(s2)
	call        DW_TAGString

	// *** Basic block 2

	lw          a2, 56(s3)
	mv          a1, a0
	mv          a0, s4
	call        printf

	// *** Basic block 3

	ld          a0, 64(s3)
	addi        a1, s1, 1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEPrint
.func_end_SubrangeDIEPrint:
	.size SubrangeDIEPrint, .func_end_SubrangeDIEPrint-SubrangeDIEPrint

	.local  SubrangeDIEEmit
	.type SubrangeDIEEmit, @function

SubrangeDIEEmit:

	// *** Basic block 0

	.local DIEBaseEmit
	.local DIEEmit
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
	call        DIEBaseEmit

	// *** Basic block 1

	mv          s3, s2
	ld          a1, 64(s3)
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEEmit
.func_end_SubrangeDIEEmit:
	.size SubrangeDIEEmit, .func_end_SubrangeDIEEmit-SubrangeDIEEmit

	.local  ArrayDIEBuild
	.type ArrayDIEBuild, @function

ArrayDIEBuild:

	// *** Basic block 0

	.global DIEBuild
	.local AddAttributeAndValue
	.local DIEReference
	.global DIEAllocateAbbreviation
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
	mv          s2, a0
	mv          s3, s1
	ld          s4, 64(s3)
	mv          a1, s4
	call        DIEBuild

	// *** Basic block 1

	mv          a1, s4
	li          t0, 73		// 0x49 ASCII 'I'
	mv          a0, t0
	call        DIEReference

	// *** Basic block 2

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 3

	mv          a1, s1
	mv          a0, s2
	call        DIEAllocateAbbreviation

	// *** Basic block 4

	ld          a1, 56(s3)
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEBuild
.func_end_ArrayDIEBuild:
	.size ArrayDIEBuild, .func_end_ArrayDIEBuild-ArrayDIEBuild

	.local  ArrayDIEPrint
	.type ArrayDIEPrint, @function

ArrayDIEPrint:

	// *** Basic block 0

	.local DoIndent
	.global printf
	.global DW_TAGString
	.global DIEPrint
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
	mv          s2, a0
	mv          a0, s1
	call        DoIndent

	// *** Basic block 1

	mv          s3, s2
	lla         s4, .str.48
	lw          a0, 4(s2)
	call        DW_TAGString

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s4
	call        printf

	// *** Basic block 3

	ld          a0, 64(s3)
	addi        s4, s1, 1
	mv          a1, s4
	call        DIEPrint

	// *** Basic block 4

	ld          a0, 56(s3)
	mv          a1, s4
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEPrint
.func_end_ArrayDIEPrint:
	.size ArrayDIEPrint, .func_end_ArrayDIEPrint-ArrayDIEPrint

	.local  ArrayDIEEmit
	.type ArrayDIEEmit, @function

ArrayDIEEmit:

	// *** Basic block 0

	.local DIEBaseEmit
	.local DIEEmit
	.local EmitEndOfChildren
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
	mv          t0, a1
	mv          s1, a0
	mv          s2, t0
	call        DIEBaseEmit

	// *** Basic block 1

	ld          a1, 56(s2)
	mv          a0, s1
	call        DIEEmit

	// *** Basic block 2

	mv          a0, s1
	call        EmitEndOfChildren

	// *** Basic block 3

	ld          a1, 64(s2)
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEEmit
.func_end_ArrayDIEEmit:
	.size ArrayDIEEmit, .func_end_ArrayDIEEmit-ArrayDIEEmit

	.local  PointerDIEBuild
	.type PointerDIEBuild, @function

PointerDIEBuild:

	// *** Basic block 0

	.global DIEBuild
	.local AddAttributeAndValue
	.local DIEReference
	.local UnsignedConstant
	.global compiler
	.global DIEAllocateAbbreviation
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
	mv          t0, s1
	ld          s3, 56(t0)
	mv          a1, s3
	call        DIEBuild

	// *** Basic block 1

	mv          a1, s3
	li          t0, 73		// 0x49 ASCII 'I'
	mv          a0, t0
	call        DIEReference

	// *** Basic block 2

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 3

	la          t0, compiler
	ld          t0, 0(t0)
	lw          a1, 1080(t0)
	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        UnsignedConstant

	// *** Basic block 4

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 5

	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEAllocateAbbreviation
.func_end_PointerDIEBuild:
	.size PointerDIEBuild, .func_end_PointerDIEBuild-PointerDIEBuild

	.local  PointerDIEPrint
	.type PointerDIEPrint, @function

PointerDIEPrint:

	// *** Basic block 0

	.local DoIndent
	.global printf
	.global DW_TAGString
	.global DIEPrint
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
	mv          s2, a0
	mv          a0, s1
	call        DoIndent

	// *** Basic block 1

	mv          s3, s2
	lla         s4, .str.49
	lw          a0, 4(s2)
	call        DW_TAGString

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s4
	call        printf

	// *** Basic block 3

	ld          a0, 56(s3)
	addi        a1, s1, 1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEPrint
.func_end_PointerDIEPrint:
	.size PointerDIEPrint, .func_end_PointerDIEPrint-PointerDIEPrint

	.local  PointerDIEEmit
	.type PointerDIEEmit, @function

PointerDIEEmit:

	// *** Basic block 0

	.local DIEBaseEmit
	.local DIEEmit
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
	call        DIEBaseEmit

	// *** Basic block 1

	mv          s3, s2
	ld          a1, 56(s3)
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEEmit
.func_end_PointerDIEEmit:
	.size PointerDIEEmit, .func_end_PointerDIEEmit-PointerDIEEmit

	.local  LexicalScopeDIEBuild
	.type LexicalScopeDIEBuild, @function

LexicalScopeDIEBuild:

	// *** Basic block 0

	.local AddAttributeAndValue
	.local AddressConstant
	.global DIEAllocateAbbreviation
	.global DIEBuild
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
	mv          s3, s1
	addi        t0, s3, 112
	ld          a2, 16(t0)
	li          a1, 1		// 0x1 ASCII \x1
	li          a0, 17		// 0x11 ASCII \x11
	call        AddressConstant

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 2

	addi        t0, s3, 152
	ld          a2, 16(t0)
	li          t0, 255		// 0xff
	mv          a1, t0
	li          t0, 18		// 0x12 ASCII \x12
	mv          a0, t0
	call        AddressConstant

	// *** Basic block 3

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 4

	mv          a1, s1
	mv          a0, s2
	call        DIEAllocateAbbreviation

	// *** Basic block 5

	mv          s4, x0
	addi        t0, s3, 56
	ld          s5, 8(t0)
	bge         x0, s5, .LexicalScopeDIEBuild_label_89

	// *** Basic block 6

	ld          s1, 56(s3)

	// *** Basic block 7

.LexicalScopeDIEBuild_label_76:
	slli        t0, s4, 3
	add         t0, s1, t0
	ld          a1, 0(t0)
	mv          a0, s2
	call        DIEBuild

	// *** Basic block 8

.LexicalScopeDIEBuild_label_85:
	addi        s4, s4, 1
	bge         s4, s5, .LexicalScopeDIEBuild_label_76

	// *** Basic block 9

.LexicalScopeDIEBuild_label_89:
	mv          s1, x0
	addi        t0, s3, 80
	ld          s5, 8(t0)
	bge         x0, s5, .LexicalScopeDIEBuild_label_110

	// *** Basic block 10

	ld          s6, 80(s3)

	// *** Basic block 11

.LexicalScopeDIEBuild_label_98:
	slli        t0, s1, 3
	add         t0, s6, t0
	ld          a1, 0(t0)
	mv          a0, s2
	call        DIEBuild

	// *** Basic block 12

.LexicalScopeDIEBuild_label_106:
	addi        s1, s1, 1
	bge         s1, s5, .LexicalScopeDIEBuild_label_98

	// *** Basic block 13

.LexicalScopeDIEBuild_label_110:
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
.func_end_LexicalScopeDIEBuild:
	.size LexicalScopeDIEBuild, .func_end_LexicalScopeDIEBuild-LexicalScopeDIEBuild

	.local  LexicalScopeDIEPrint
	.type LexicalScopeDIEPrint, @function

LexicalScopeDIEPrint:

	// *** Basic block 0

	.local DoIndent
	.global printf
	.global DW_TAGString
	.global DIEPrint
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
	mv          a0, s1
	call        DoIndent

	// *** Basic block 1

	addi        s3, s1, 1
	addi        s4, s1, 1
	mv          s5, s2
	lla         s6, .str.50
	lw          a0, 4(s2)
	call        DW_TAGString

	// *** Basic block 2

	addi        t0, s5, 112
	ld          a2, 16(t0)
	addi        t0, s5, 152
	ld          a3, 16(t0)
	mv          a1, a0
	mv          a0, s6
	call        printf

	// *** Basic block 3

	mv          s6, x0
	addi        t0, s5, 56
	ld          s7, 8(t0)
	bge         x0, s7, .LexicalScopeDIEPrint_label_74

	// *** Basic block 4

	ld          s1, 56(s5)

	// *** Basic block 5

.LexicalScopeDIEPrint_label_61:
	slli        t0, s6, 3
	add         t0, s1, t0
	ld          a0, 0(t0)
	mv          a1, s3
	call        DIEPrint

	// *** Basic block 6

.LexicalScopeDIEPrint_label_70:
	addi        s6, s6, 1
	bge         s6, s7, .LexicalScopeDIEPrint_label_61

	// *** Basic block 7

.LexicalScopeDIEPrint_label_74:
	mv          s1, x0
	addi        t0, s5, 80
	ld          s2, 8(t0)
	bge         x0, s2, .LexicalScopeDIEPrint_label_95

	// *** Basic block 8

	ld          s3, 80(s5)

	// *** Basic block 9

.LexicalScopeDIEPrint_label_83:
	slli        t0, s1, 3
	add         t0, s3, t0
	ld          a0, 0(t0)
	mv          a1, s4
	call        DIEPrint

	// *** Basic block 10

.LexicalScopeDIEPrint_label_91:
	addi        s1, s1, 1
	bge         s1, s2, .LexicalScopeDIEPrint_label_83

	// *** Basic block 11

.LexicalScopeDIEPrint_label_95:
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
.func_end_LexicalScopeDIEPrint:
	.size LexicalScopeDIEPrint, .func_end_LexicalScopeDIEPrint-LexicalScopeDIEPrint

	.local  LexicalScopeDIEEmit
	.type LexicalScopeDIEEmit, @function

LexicalScopeDIEEmit:

	// *** Basic block 0

	.local DIEBaseEmit
	.local DIEEmit
	.local EmitEndOfChildren
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
	call        DIEBaseEmit

	// *** Basic block 1

	mv          s3, s2
	mv          s4, x0
	addi        t0, s3, 56
	ld          s5, 8(t0)
	bge         x0, s5, .LexicalScopeDIEEmit_label_46

	// *** Basic block 2

	ld          s2, 56(s3)

	// *** Basic block 3

.LexicalScopeDIEEmit_label_33:
	slli        t0, s4, 3
	add         t0, s2, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        DIEEmit

	// *** Basic block 4

.LexicalScopeDIEEmit_label_42:
	addi        s4, s4, 1
	bge         s4, s5, .LexicalScopeDIEEmit_label_33

	// *** Basic block 5

.LexicalScopeDIEEmit_label_46:
	mv          s2, x0
	addi        t0, s3, 80
	ld          s5, 8(t0)
	bge         x0, s5, .LexicalScopeDIEEmit_label_67

	// *** Basic block 6

	ld          s6, 80(s3)

	// *** Basic block 7

.LexicalScopeDIEEmit_label_55:
	slli        t0, s2, 3
	add         t0, s6, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        DIEEmit

	// *** Basic block 8

.LexicalScopeDIEEmit_label_63:
	addi        s2, s2, 1
	bge         s2, s5, .LexicalScopeDIEEmit_label_55

	// *** Basic block 9

.LexicalScopeDIEEmit_label_67:
	mv          a0, s1
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
	j           EmitEndOfChildren
.func_end_LexicalScopeDIEEmit:
	.size LexicalScopeDIEEmit, .func_end_LexicalScopeDIEEmit-LexicalScopeDIEEmit

	.local  FunctionDIEBuild
	.type FunctionDIEBuild, @function

FunctionDIEBuild:

	// *** Basic block 0

	.global DIEBuild
	.local AddAttributeAndValue
	.local StringConstant
	.local DIEReference
	.local AddressConstant
	.global DIEAllocateAbbreviation
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
	mv          s3, s1
	ld          s4, 96(s3)
	beq         s4, x0, .FunctionDIEBuild_label_44

	// *** Basic block 1

	mv          a1, s4
	mv          a0, s2
	call        DIEBuild

	// *** Basic block 2

.FunctionDIEBuild_label_44:
	addi        t0, s3, 56
	ld          a1, 16(t0)
	li          t0, 3		// 0x3 ASCII \x3
	mv          a0, t0
	call        StringConstant

	// *** Basic block 3

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 4

	mv          a1, s4
	li          t0, 73		// 0x49 ASCII 'I'
	mv          a0, t0
	call        DIEReference

	// *** Basic block 5

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 6

	ld          s4, 104(s3)
	addi        t0, s4, 112
	ld          a2, 16(t0)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        AddressConstant

	// *** Basic block 7

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 8

	addi        t0, s4, 152
	ld          a2, 16(t0)
	li          t0, 255		// 0xff
	mv          a1, t0
	li          t0, 18		// 0x12 ASCII \x12
	mv          a0, t0
	call        AddressConstant

	// *** Basic block 9

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 10

	mv          a1, s1
	mv          a0, s2
	call        DIEAllocateAbbreviation

	// *** Basic block 11

	mv          s5, x0
	addi        t0, s4, 56
	ld          s6, 8(t0)
	bge         x0, s6, .FunctionDIEBuild_label_137

	// *** Basic block 12

	ld          s1, 56(s4)

	// *** Basic block 13

.FunctionDIEBuild_label_125:
	slli        t0, s5, 3
	add         t0, s1, t0
	ld          a1, 0(t0)
	mv          a0, s2
	call        DIEBuild

	// *** Basic block 14

.FunctionDIEBuild_label_133:
	addi        s5, s5, 1
	bge         s5, s6, .FunctionDIEBuild_label_125

	// *** Basic block 15

.FunctionDIEBuild_label_137:
	mv          s1, x0
	addi        t0, s4, 80
	ld          s3, 8(t0)
	bge         x0, s3, .FunctionDIEBuild_label_158

	// *** Basic block 16

	ld          s4, 80(s4)

	// *** Basic block 17

.FunctionDIEBuild_label_146:
	slli        t0, s1, 3
	add         t0, s4, t0
	ld          a1, 0(t0)
	mv          a0, s2
	call        DIEBuild

	// *** Basic block 18

.FunctionDIEBuild_label_154:
	addi        s1, s1, 1
	bge         s1, s3, .FunctionDIEBuild_label_146

	// *** Basic block 19

.FunctionDIEBuild_label_158:
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
.func_end_FunctionDIEBuild:
	.size FunctionDIEBuild, .func_end_FunctionDIEBuild-FunctionDIEBuild

	.local  FunctionDIEPrint
	.type FunctionDIEPrint, @function

FunctionDIEPrint:

	// *** Basic block 0

	.local DoIndent
	.global printf
	.global DW_TAGString
	.global DIEPrint
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
	mv          s1, a1
	mv          s2, a0
	mv          a0, s1
	call        DoIndent

	// *** Basic block 1

	addi        s3, s1, 1
	addi        s4, s1, 1
	mv          s5, s2
	lla         s6, .str.51
	lw          a0, 4(s2)
	call        DW_TAGString

	// *** Basic block 2

	addi        t0, s5, 56
	ld          a2, 16(t0)
	ld          s7, 104(s5)
	addi        t0, s7, 112
	ld          a3, 16(t0)
	addi        t0, s7, 152
	ld          a4, 16(t0)
	mv          a1, a0
	mv          a0, s6
	call        printf

	// *** Basic block 3

	mv          s6, x0
	addi        t0, s7, 56
	ld          s8, 8(t0)
	bge         x0, s8, .FunctionDIEPrint_label_82

	// *** Basic block 4

	ld          s1, 56(s7)

	// *** Basic block 5

.FunctionDIEPrint_label_69:
	slli        t0, s6, 3
	add         t0, s1, t0
	ld          a0, 0(t0)
	mv          a1, s3
	call        DIEPrint

	// *** Basic block 6

.FunctionDIEPrint_label_78:
	addi        s6, s6, 1
	bge         s6, s8, .FunctionDIEPrint_label_69

	// *** Basic block 7

.FunctionDIEPrint_label_82:
	mv          s1, x0
	addi        t0, s7, 80
	ld          s2, 8(t0)
	bge         x0, s2, .FunctionDIEPrint_label_103

	// *** Basic block 8

	ld          s3, 80(s7)

	// *** Basic block 9

.FunctionDIEPrint_label_91:
	slli        t0, s1, 3
	add         t0, s3, t0
	ld          a0, 0(t0)
	mv          a1, s4
	call        DIEPrint

	// *** Basic block 10

.FunctionDIEPrint_label_99:
	addi        s1, s1, 1
	bge         s1, s2, .FunctionDIEPrint_label_91

	// *** Basic block 11

.FunctionDIEPrint_label_103:
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
.func_end_FunctionDIEPrint:
	.size FunctionDIEPrint, .func_end_FunctionDIEPrint-FunctionDIEPrint

	.local  FunctionDIEEmit
	.type FunctionDIEEmit, @function

FunctionDIEEmit:

	// *** Basic block 0

	.local DIEBaseEmit
	.local DIEEmit
	.local EmitEndOfChildren
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
	call        DIEBaseEmit

	// *** Basic block 1

	mv          s3, s2
	ld          s4, 104(s3)
	mv          s5, x0
	addi        t0, s4, 56
	ld          s6, 8(t0)
	bge         x0, s6, .FunctionDIEEmit_label_51

	// *** Basic block 2

	ld          s2, 56(s4)

	// *** Basic block 3

.FunctionDIEEmit_label_38:
	slli        t0, s5, 3
	add         t0, s2, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        DIEEmit

	// *** Basic block 4

.FunctionDIEEmit_label_47:
	addi        s5, s5, 1
	bge         s5, s6, .FunctionDIEEmit_label_38

	// *** Basic block 5

.FunctionDIEEmit_label_51:
	mv          s2, x0
	addi        t0, s4, 80
	ld          s6, 8(t0)
	bge         x0, s6, .FunctionDIEEmit_label_72

	// *** Basic block 6

	ld          s4, 80(s4)

	// *** Basic block 7

.FunctionDIEEmit_label_60:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        DIEEmit

	// *** Basic block 8

.FunctionDIEEmit_label_68:
	addi        s2, s2, 1
	bge         s2, s6, .FunctionDIEEmit_label_60

	// *** Basic block 9

.FunctionDIEEmit_label_72:
	mv          a0, s1
	call        EmitEndOfChildren

	// *** Basic block 10

	ld          t0, 96(s3)
	beq         t0, x0, .FunctionDIEEmit_label_87

	// *** Basic block 11

	mv          a1, t0
	mv          a0, s1
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
	j           DIEEmit

	// *** Basic block 12

.FunctionDIEEmit_label_87:
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
.func_end_FunctionDIEEmit:
	.size FunctionDIEEmit, .func_end_FunctionDIEEmit-FunctionDIEEmit

	.local  StructDIEBuild
	.type StructDIEBuild, @function

StructDIEBuild:

	// *** Basic block 0

	.local AddAttributeAndValue
	.local StringConstant
	.local UnsignedConstant
	.global DIEAllocateAbbreviation
	.global DIEBuild
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
	mv          s2, a0
	mv          s3, s1
	addi        t0, s3, 56
	ld          a1, 16(t0)
	li          a0, 3		// 0x3 ASCII \x3
	call        StringConstant

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 2

	lw          a1, 96(s3)
	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        UnsignedConstant

	// *** Basic block 3

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 4

	mv          a1, s1
	mv          a0, s2
	call        DIEAllocateAbbreviation

	// *** Basic block 5

	mv          s4, x0
	addi        t0, s3, 104
	ld          s5, 8(t0)
	bge         x0, s5, .StructDIEBuild_label_80

	// *** Basic block 6

	ld          s1, 104(s3)

	// *** Basic block 7

.StructDIEBuild_label_68:
	slli        t0, s4, 3
	add         t0, s1, t0
	ld          a1, 0(t0)
	mv          a0, s2
	call        DIEBuild

	// *** Basic block 8

.StructDIEBuild_label_76:
	addi        s4, s4, 1
	bge         s4, s5, .StructDIEBuild_label_68

	// *** Basic block 9

.StructDIEBuild_label_80:
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
.func_end_StructDIEBuild:
	.size StructDIEBuild, .func_end_StructDIEBuild-StructDIEBuild

	.local  StructDIEPrint
	.type StructDIEPrint, @function

StructDIEPrint:

	// *** Basic block 0

	.local DoIndent
	.global printf
	.global DW_TAGString
	.global DIEPrint
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
	mv          a0, s1
	call        DoIndent

	// *** Basic block 1

	addi        s3, s1, 1
	mv          s4, s2
	lla         s5, .str.52
	lw          a0, 4(s2)
	call        DW_TAGString

	// *** Basic block 2

	addi        t0, s4, 56
	ld          a2, 16(t0)
	lw          a3, 96(s4)
	mv          a1, a0
	mv          a0, s5
	call        printf

	// *** Basic block 3

	mv          s5, x0
	addi        t0, s4, 104
	ld          s6, 8(t0)
	bge         x0, s6, .StructDIEPrint_label_71

	// *** Basic block 4

	ld          s1, 104(s4)

	// *** Basic block 5

.StructDIEPrint_label_58:
	slli        t0, s5, 3
	add         t0, s1, t0
	ld          a0, 0(t0)
	mv          a1, s3
	call        DIEPrint

	// *** Basic block 6

.StructDIEPrint_label_67:
	addi        s5, s5, 1
	bge         s5, s6, .StructDIEPrint_label_58

	// *** Basic block 7

.StructDIEPrint_label_71:
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
.func_end_StructDIEPrint:
	.size StructDIEPrint, .func_end_StructDIEPrint-StructDIEPrint

	.local  StructDIEEmit
	.type StructDIEEmit, @function

StructDIEEmit:

	// *** Basic block 0

	.local DIEBaseEmit
	.global DIEBuild
	.local EmitEndOfChildren
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
	call        DIEBaseEmit

	// *** Basic block 1

	mv          s3, s2
	mv          s4, x0
	addi        t0, s3, 104
	ld          s5, 8(t0)
	bge         x0, s5, .StructDIEEmit_label_45

	// *** Basic block 2

	ld          s2, 104(s3)

	// *** Basic block 3

.StructDIEEmit_label_32:
	slli        t0, s4, 3
	add         t0, s2, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        DIEBuild

	// *** Basic block 4

.StructDIEEmit_label_41:
	addi        s4, s4, 1
	bge         s4, s5, .StructDIEEmit_label_32

	// *** Basic block 5

.StructDIEEmit_label_45:
	mv          a0, s1
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           EmitEndOfChildren
.func_end_StructDIEEmit:
	.size StructDIEEmit, .func_end_StructDIEEmit-StructDIEEmit

	.local  EnumDIEBuild
	.type EnumDIEBuild, @function

EnumDIEBuild:

	// *** Basic block 0

	.local AddAttributeAndValue
	.local StringConstant
	.local UnsignedConstant
	.global DIEAllocateAbbreviation
	.global DIEBuild
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
	mv          s2, a0
	mv          s3, s1
	addi        t0, s3, 56
	ld          a1, 16(t0)
	li          a0, 3		// 0x3 ASCII \x3
	call        StringConstant

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 2

	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        UnsignedConstant

	// *** Basic block 3

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 4

	mv          a1, s1
	mv          a0, s2
	call        DIEAllocateAbbreviation

	// *** Basic block 5

	mv          s4, x0
	addi        t0, s3, 96
	ld          s5, 8(t0)
	bge         x0, s5, .EnumDIEBuild_label_80

	// *** Basic block 6

	ld          s1, 96(s3)

	// *** Basic block 7

.EnumDIEBuild_label_68:
	slli        t0, s4, 3
	add         t0, s1, t0
	ld          a1, 0(t0)
	mv          a0, s2
	call        DIEBuild

	// *** Basic block 8

.EnumDIEBuild_label_76:
	addi        s4, s4, 1
	bge         s4, s5, .EnumDIEBuild_label_68

	// *** Basic block 9

.EnumDIEBuild_label_80:
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
.func_end_EnumDIEBuild:
	.size EnumDIEBuild, .func_end_EnumDIEBuild-EnumDIEBuild

	.local  EnumDIEPrint
	.type EnumDIEPrint, @function

EnumDIEPrint:

	// *** Basic block 0

	.local DoIndent
	.global printf
	.global DW_TAGString
	.global DIEPrint
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
	mv          a0, s1
	call        DoIndent

	// *** Basic block 1

	addi        s3, s1, 1
	mv          s4, s2
	lla         s5, .str.53
	lw          a0, 4(s2)
	call        DW_TAGString

	// *** Basic block 2

	addi        t0, s4, 56
	ld          a2, 16(t0)
	mv          a1, a0
	mv          a0, s5
	call        printf

	// *** Basic block 3

	mv          s5, x0
	addi        t0, s4, 96
	ld          s6, 8(t0)
	bge         x0, s6, .EnumDIEPrint_label_66

	// *** Basic block 4

	ld          s1, 96(s4)

	// *** Basic block 5

.EnumDIEPrint_label_53:
	slli        t0, s5, 3
	add         t0, s1, t0
	ld          a0, 0(t0)
	mv          a1, s3
	call        DIEPrint

	// *** Basic block 6

.EnumDIEPrint_label_62:
	addi        s5, s5, 1
	bge         s5, s6, .EnumDIEPrint_label_53

	// *** Basic block 7

.EnumDIEPrint_label_66:
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
.func_end_EnumDIEPrint:
	.size EnumDIEPrint, .func_end_EnumDIEPrint-EnumDIEPrint

	.local  EnumDIEEmit
	.type EnumDIEEmit, @function

EnumDIEEmit:

	// *** Basic block 0

	.local DIEBaseEmit
	.local DIEEmit
	.local EmitEndOfChildren
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
	call        DIEBaseEmit

	// *** Basic block 1

	mv          s3, s2
	mv          s4, x0
	addi        t0, s3, 96
	ld          s5, 8(t0)
	bge         x0, s5, .EnumDIEEmit_label_45

	// *** Basic block 2

	ld          s2, 96(s3)

	// *** Basic block 3

.EnumDIEEmit_label_32:
	slli        t0, s4, 3
	add         t0, s2, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        DIEEmit

	// *** Basic block 4

.EnumDIEEmit_label_41:
	addi        s4, s4, 1
	bge         s4, s5, .EnumDIEEmit_label_32

	// *** Basic block 5

.EnumDIEEmit_label_45:
	mv          a0, s1
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           EmitEndOfChildren
.func_end_EnumDIEEmit:
	.size EnumDIEEmit, .func_end_EnumDIEEmit-EnumDIEEmit

	.local  CVDIEBuild
	.type CVDIEBuild, @function

CVDIEBuild:

	// *** Basic block 0

	.global DIEBuild
	.local AddAttributeAndValue
	.local DIEReference
	.global DIEAllocateAbbreviation
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
	mv          t0, s1
	ld          s3, 56(t0)
	mv          a1, s3
	call        DIEBuild

	// *** Basic block 1

	mv          a1, s3
	li          t0, 73		// 0x49 ASCII 'I'
	mv          a0, t0
	call        DIEReference

	// *** Basic block 2

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 3

	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEAllocateAbbreviation
.func_end_CVDIEBuild:
	.size CVDIEBuild, .func_end_CVDIEBuild-CVDIEBuild

	.local  CVDIEPrint
	.type CVDIEPrint, @function

CVDIEPrint:

	// *** Basic block 0

	.local DoIndent
	.global printf
	.global DW_TAGString
	.global DIEPrint
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
	mv          s2, a0
	mv          a0, s1
	call        DoIndent

	// *** Basic block 1

	mv          s3, s2
	lla         s4, .str.54
	lw          a0, 4(s2)
	call        DW_TAGString

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s4
	call        printf

	// *** Basic block 3

	ld          a0, 56(s3)
	addi        a1, s1, 1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEPrint
.func_end_CVDIEPrint:
	.size CVDIEPrint, .func_end_CVDIEPrint-CVDIEPrint

	.local  CVDIEEmit
	.type CVDIEEmit, @function

CVDIEEmit:

	// *** Basic block 0

	.local DIEBaseEmit
	.local DIEEmit
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
	call        DIEBaseEmit

	// *** Basic block 1

	mv          s3, s2
	ld          a1, 56(s3)
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEEmit
.func_end_CVDIEEmit:
	.size CVDIEEmit, .func_end_CVDIEEmit-CVDIEEmit

	.local  SymbolDIEBuild
	.type SymbolDIEBuild, @function

SymbolDIEBuild:

	// *** Basic block 0

	.local AddAttributeAndValue
	.local StringConstant
	.global DIEBuild
	.local DIEReference
	.local Flag
	.global DIEAllocateAbbreviation
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
	mv          s2, a0
	mv          s3, s1
	addi        t0, s3, 56
	ld          a1, 16(t0)
	li          a0, 3		// 0x3 ASCII \x3
	call        StringConstant

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 2

	ld          s4, 96(s3)
	mv          a1, s4
	mv          a0, s2
	call        DIEBuild

	// *** Basic block 3

	mv          a1, s4
	li          t0, 73		// 0x49 ASCII 'I'
	mv          a0, t0
	call        DIEReference

	// *** Basic block 4

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 5

	lb          a1, 104(s3)
	li          t0, 63		// 0x3f ASCII '?'
	mv          a0, t0
	call        Flag

	// *** Basic block 6

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 7

	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEAllocateAbbreviation
.func_end_SymbolDIEBuild:
	.size SymbolDIEBuild, .func_end_SymbolDIEBuild-SymbolDIEBuild

	.local  SymbolDIEPrint
	.type SymbolDIEPrint, @function

SymbolDIEPrint:

	// *** Basic block 0

	.local DoIndent
	.global printf
	.global DW_TAGString
	.global DIEPrint
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
	mv          s2, a0
	mv          a0, s1
	call        DoIndent

	// *** Basic block 1

	mv          s3, s2
	lla         s4, .str.55
	lw          a0, 4(s2)
	call        DW_TAGString

	// *** Basic block 2

	addi        t0, s3, 56
	ld          a2, 16(t0)
	mv          a1, a0
	mv          a0, s4
	call        printf

	// *** Basic block 3

	addi        s4, s3, 112
	lw          t0, 112(s3)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 4

	j           .SymbolDIEPrint_label_88

	// *** Basic block 5

	j           .SymbolDIEPrint_label_94

	// *** Basic block 6

	j           .SymbolDIEPrint_label_79

	// *** Basic block 7

	j           .SymbolDIEPrint_label_60

	// *** Basic block 8

.SymbolDIEPrint_label_60:
	lla         s2, .str.56
	ld          a1, 8(s4)
	bne         a1, x0, .SymbolDIEPrint_label_72

	// *** Basic block 9

	lla         a1, .str.57
	j           .SymbolDIEPrint_label_73

	// *** Basic block 10

.SymbolDIEPrint_label_72:

	// *** Basic block 11

.SymbolDIEPrint_label_73:
	mv          a0, s2
	call        printf

	// *** Basic block 12

	j           .SymbolDIEPrint_label_103

	// *** Basic block 13

.SymbolDIEPrint_label_79:
	lla         a0, .str.58
	lw          a1, 8(s4)
	call        printf

	// *** Basic block 14

	j           .SymbolDIEPrint_label_103

	// *** Basic block 15

.SymbolDIEPrint_label_88:
	lla         a0, .str.59
	call        printf

	// *** Basic block 16

	j           .SymbolDIEPrint_label_103

	// *** Basic block 17

.SymbolDIEPrint_label_94:
	lla         a0, .str.60
	lw          a1, 8(s4)
	call        printf

	// *** Basic block 18

	j           .SymbolDIEPrint_label_103

	// *** Basic block 19

.SymbolDIEPrint_label_103:
	ld          a0, 96(s3)
	addi        a1, s1, 1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEPrint
.func_end_SymbolDIEPrint:
	.size SymbolDIEPrint, .func_end_SymbolDIEPrint-SymbolDIEPrint

	.local  SymbolDIEEmit
	.type SymbolDIEEmit, @function

SymbolDIEEmit:

	// *** Basic block 0

	.local DIEBaseEmit
	.local DIEEmit
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
	call        DIEBaseEmit

	// *** Basic block 1

	mv          s3, s2
	ld          a1, 96(s3)
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEEmit
.func_end_SymbolDIEEmit:
	.size SymbolDIEEmit, .func_end_SymbolDIEEmit-SymbolDIEEmit

	.global VariableDIESetRegister
	.type VariableDIESetRegister, @function

VariableDIESetRegister:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	bne         t0, x0, .VariableDIESetRegister_label_19

	// *** Basic block 1

.VariableDIESetRegister_label_16:
	ret         

	// *** Basic block 2

.VariableDIESetRegister_label_19:
	mv          t2, t0
	li          t3, 1		// 0x1 ASCII \x1
	sw          t3, 112(t2)
	addi        t3, t2, 112
	sw          t1, 8(t3)
	j           .VariableDIESetRegister_label_16
.func_end_VariableDIESetRegister:
	.size VariableDIESetRegister, .func_end_VariableDIESetRegister-VariableDIESetRegister

	.global VariableDIESetStackOffset
	.type VariableDIESetStackOffset, @function

VariableDIESetStackOffset:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	bne         t0, x0, .VariableDIESetStackOffset_label_19

	// *** Basic block 1

.VariableDIESetStackOffset_label_16:
	ret         

	// *** Basic block 2

.VariableDIESetStackOffset_label_19:
	mv          t2, t0
	li          t3, 2		// 0x2 ASCII \x2
	sw          t3, 112(t2)
	addi        t3, t2, 112
	sw          t1, 8(t3)
	j           .VariableDIESetStackOffset_label_16
.func_end_VariableDIESetStackOffset:
	.size VariableDIESetStackOffset, .func_end_VariableDIESetStackOffset-VariableDIESetStackOffset

	.global VariableDIESetStatic
	.type VariableDIESetStatic, @function

VariableDIESetStatic:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	bne         t0, x0, .VariableDIESetStatic_label_19

	// *** Basic block 1

.VariableDIESetStatic_label_16:
	ret         

	// *** Basic block 2

.VariableDIESetStatic_label_19:
	mv          t2, t0
	li          t3, 3		// 0x3 ASCII \x3
	sw          t3, 112(t2)
	addi        t3, t2, 112
	sd          t1, 8(t3)
	j           .VariableDIESetStatic_label_16
.func_end_VariableDIESetStatic:
	.size VariableDIESetStatic, .func_end_VariableDIESetStatic-VariableDIESetStatic

	.local  MemberDIEBuild
	.type MemberDIEBuild, @function

MemberDIEBuild:

	// *** Basic block 0

	.local AddAttributeAndValue
	.local StringConstant
	.global DIEBuild
	.local DIEReference
	.local Flag
	.local UnsignedConstant
	.global DIEAllocateAbbreviation
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
	mv          s2, a0
	mv          s3, s1
	addi        t0, s3, 56
	ld          a1, 16(t0)
	li          a0, 3		// 0x3 ASCII \x3
	call        StringConstant

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 2

	ld          s4, 96(s3)
	mv          a1, s4
	mv          a0, s2
	call        DIEBuild

	// *** Basic block 3

	mv          a1, s4
	li          t0, 73		// 0x49 ASCII 'I'
	mv          a0, t0
	call        DIEReference

	// *** Basic block 4

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 5

	lb          a1, 104(s3)
	li          t0, 63		// 0x3f ASCII '?'
	mv          a0, t0
	call        Flag

	// *** Basic block 6

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 7

	lw          a1, 128(s3)
	li          t0, 56		// 0x38 ASCII '8'
	mv          a0, t0
	call        UnsignedConstant

	// *** Basic block 8

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 9

	lb          t0, 132(s3)
	beqz        t0, .MemberDIEBuild_label_142

	// *** Basic block 10

	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        UnsignedConstant

	// *** Basic block 11

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 12

	lw          a1, 136(s3)
	li          t0, 12		// 0xc ASCII \xc
	mv          a0, t0
	call        UnsignedConstant

	// *** Basic block 13

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 14

	lw          a1, 140(s3)
	li          s4, 13		// 0xd ASCII \xd
	mv          a0, s4
	call        UnsignedConstant

	// *** Basic block 15

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 16

.MemberDIEBuild_label_142:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEAllocateAbbreviation
.func_end_MemberDIEBuild:
	.size MemberDIEBuild, .func_end_MemberDIEBuild-MemberDIEBuild

	.local  MemberDIEPrint
	.type MemberDIEPrint, @function

MemberDIEPrint:

	// *** Basic block 0

	.local SymbolDIEPrint
	.local DoIndent
	.global printf
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
	mv          t0, a0
	mv          s1, a1
	mv          s2, t0
	mv          a0, s2
	call        SymbolDIEPrint

	// *** Basic block 1

	mv          a0, s1
	call        DoIndent

	// *** Basic block 2

	lla         a0, .str.61
	lw          a1, 128(s2)
	lb          s3, 132(s2)
	mv          a2, s3
	call        printf

	// *** Basic block 3

	beqz        s3, .MemberDIEPrint_label_56

	// *** Basic block 4

	mv          a0, s1
	call        DoIndent

	// *** Basic block 5

	lla         a0, .str.62
	lw          a1, 136(s2)
	lw          a2, 140(s2)
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           printf

	// *** Basic block 6

.MemberDIEPrint_label_56:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_MemberDIEPrint:
	.size MemberDIEPrint, .func_end_MemberDIEPrint-MemberDIEPrint

	.local  MemberDIEEmit
	.type MemberDIEEmit, @function

MemberDIEEmit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local SymbolDIEEmit
	j           SymbolDIEEmit
.func_end_MemberDIEEmit:
	.size MemberDIEEmit, .func_end_MemberDIEEmit-MemberDIEEmit

	.local  EnumConstDIEBuild
	.type EnumConstDIEBuild, @function

EnumConstDIEBuild:

	// *** Basic block 0

	.local AddAttributeAndValue
	.local StringConstant
	.local SignedConstant
	.global DIEAllocateAbbreviation
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
	mv          s3, s1
	addi        t0, s3, 56
	ld          a1, 16(t0)
	li          a0, 3		// 0x3 ASCII \x3
	call        StringConstant

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 2

	ld          a1, 96(s3)
	li          t0, 28		// 0x1c ASCII \x1c
	mv          a0, t0
	call        SignedConstant

	// *** Basic block 3

	mv          a2, a0
	mv          a1, s1
	mv          a0, s2
	call        AddAttributeAndValue

	// *** Basic block 4

	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEAllocateAbbreviation
.func_end_EnumConstDIEBuild:
	.size EnumConstDIEBuild, .func_end_EnumConstDIEBuild-EnumConstDIEBuild

	.local  EnumConstDIEPrint
	.type EnumConstDIEPrint, @function

EnumConstDIEPrint:

	// *** Basic block 0

	.local DoIndent
	.global printf
	.global DW_TAGString
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
	mv          a0, a1
	call        DoIndent

	// *** Basic block 1

	mv          s2, s1
	lla         s3, .str.63
	lw          a0, 4(s1)
	call        DW_TAGString

	// *** Basic block 2

	ld          a2, 96(s2)
	mv          a1, a0
	mv          a0, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           printf
.func_end_EnumConstDIEPrint:
	.size EnumConstDIEPrint, .func_end_EnumConstDIEPrint-EnumConstDIEPrint

	.local  EnumConstDIEEmit
	.type EnumConstDIEEmit, @function

EnumConstDIEEmit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local DIEBaseEmit
	j           DIEBaseEmit
.func_end_EnumConstDIEEmit:
	.size EnumConstDIEEmit, .func_end_EnumConstDIEEmit-EnumConstDIEEmit

	.local  DecodeUnsignedLEB128
	.type DecodeUnsignedLEB128, @function

DecodeUnsignedLEB128:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a0
	sd          x0, 0(t0)
	mv          t2, x0
	ld          t3, 0(t0)

	// *** Basic block 1

.DecodeUnsignedLEB128_label_20:
	mv          t4, t1
	addi        t1, t1, 1
	lb          t5, 0(t4)
	andi        t6, t5, 127
	sll         t6, t6, t2
	or          t3, t3, t6
	sd          t3, 0(t0)
	andi        t3, t5, -128
	beqz        t3, .DecodeUnsignedLEB128_label_36

	// *** Basic block 2

.DecodeUnsignedLEB128_label_33:
	j           .DecodeUnsignedLEB128_label_20

	// *** Basic block 3

.DecodeUnsignedLEB128_label_36:
	mv          a0, t1

	// *** Basic block 4

.DecodeUnsignedLEB128_label_39:
	ret         
.func_end_DecodeUnsignedLEB128:
	.size DecodeUnsignedLEB128, .func_end_DecodeUnsignedLEB128-DecodeUnsignedLEB128

	.local  DebugAbbreviationPrint
	.type DebugAbbreviationPrint, @function

DebugAbbreviationPrint:

	// *** Basic block 0

	.local DoIndent
	.local DecodeUnsignedLEB128
	.global printf
	.global DW_TAGString
	.global DW_ATString
	.global DW_FORMString
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -48(s0)
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
	mv          s1, a1
	mv          s2, a0
	mv          a0, s1
	call        DoIndent

	// *** Basic block 1

	addi        s3, s1, 1
	ld          t0, 8(s2)
	ld          s4, 0(t0)
	ld          t0, 8(t0)
	add         s5, s4, t0
	addi        a1, s0, -48
	mv          a0, s4
	call        DecodeUnsignedLEB128

	// *** Basic block 2

	mv          s4, a0
	mv          t0, s4
	addi        s4, s4, 1
	lb          s6, 0(t0)
	lla         s7, .str.64
	lw          s8, 0(s2)
	ld          a0, -48(s0)
	call        DW_TAGString

	// *** Basic block 3

	beqz        s6, .DebugAbbreviationPrint_label_61

	// *** Basic block 4

	lla         a3, .str.65
	j           .DebugAbbreviationPrint_label_64

	// *** Basic block 5

.DebugAbbreviationPrint_label_61:
	lla         a3, .str.66

	// *** Basic block 6

.DebugAbbreviationPrint_label_64:
	mv          a2, a0
	mv          a1, s8
	mv          a0, s7
	call        printf

	// *** Basic block 7

	bgeu        s4, s5, .DebugAbbreviationPrint_label_112

	// *** Basic block 8

.DebugAbbreviationPrint_label_77:
	addi        a1, s0, -40
	mv          a0, s4
	call        DecodeUnsignedLEB128

	// *** Basic block 9

	mv          s4, a0
	addi        a1, s0, -32
	mv          a0, s4
	call        DecodeUnsignedLEB128

	// *** Basic block 10

	mv          s4, a0
	mv          a0, s3
	call        DoIndent

	// *** Basic block 11

	lla         s1, .str.67
	ld          a0, -40(s0)
	call        DW_ATString

	// *** Basic block 12

	ld          a0, -32(s0)
	call        DW_FORMString

	// *** Basic block 13

	mv          a2, a0
	mv          a1, a0
	mv          a0, s1
	call        printf

	// *** Basic block 14

	bltu        s4, s5, .DebugAbbreviationPrint_label_77

	// *** Basic block 15

.DebugAbbreviationPrint_label_112:
	lla         a0, .str.68
	call        printf

	// *** Basic block 16

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
.func_end_DebugAbbreviationPrint:
	.size DebugAbbreviationPrint, .func_end_DebugAbbreviationPrint-DebugAbbreviationPrint

	.local  DebugAbbreviationEmit
	.type DebugAbbreviationEmit, @function

DebugAbbreviationEmit:

	// *** Basic block 0

	.local WriteUnsignedLEB128
	.local WriteByte
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
	addi        s2, t0, 272
	lw          a1, 0(s1)
	mv          a0, s2
	call        WriteUnsignedLEB128

	// *** Basic block 1

	mv          s3, x0
	ld          s4, 8(s1)
	ld          s5, 8(s4)
	bge         x0, s5, .DebugAbbreviationEmit_label_44

	// *** Basic block 2

	ld          s1, 0(s4)

	// *** Basic block 3

.DebugAbbreviationEmit_label_33:
	add         t0, s1, s3
	lb          a1, 0(t0)
	mv          a0, s2
	call        WriteByte

	// *** Basic block 4

.DebugAbbreviationEmit_label_40:
	addi        s3, s3, 1
	bge         s3, s5, .DebugAbbreviationEmit_label_33

	// *** Basic block 5

.DebugAbbreviationEmit_label_44:
	mv          a1, x0
	mv          a0, s2
	call        WriteUnsignedLEB128

	// *** Basic block 6

	mv          a1, x0
	mv          a0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           WriteUnsignedLEB128
.func_end_DebugAbbreviationEmit:
	.size DebugAbbreviationEmit, .func_end_DebugAbbreviationEmit-DebugAbbreviationEmit

	.global DebugBuilderEmitAbbreviations
	.type DebugBuilderEmitAbbreviations, @function

DebugBuilderEmitAbbreviations:

	// *** Basic block 0

	.global fprintf
	.local DebugAbbreviationEmit
	.local WriteByte
	.local FlushAccumulator
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
	ld          a0, 264(s1)
	lla         a1, .str.69
	call        fprintf

	// *** Basic block 1

	mv          s2, x0
	addi        t0, s1, 64
	ld          s3, 8(t0)
	bge         x0, s3, .DebugBuilderEmitAbbreviations_label_48

	// *** Basic block 2

	ld          s4, 64(s1)

	// *** Basic block 3

.DebugBuilderEmitAbbreviations_label_35:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          a1, 0(t0)
	mv          a0, s1
	call        DebugAbbreviationEmit

	// *** Basic block 4

.DebugBuilderEmitAbbreviations_label_44:
	addi        s2, s2, 1
	bge         s2, s3, .DebugBuilderEmitAbbreviations_label_35

	// *** Basic block 5

.DebugBuilderEmitAbbreviations_label_48:
	addi        a0, s1, 272
	mv          a1, x0
	call        WriteByte

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
	j           FlushAccumulator
.func_end_DebugBuilderEmitAbbreviations:
	.size DebugBuilderEmitAbbreviations, .func_end_DebugBuilderEmitAbbreviations-DebugBuilderEmitAbbreviations

	.local  CompileUnitDIEBuild
	.type CompileUnitDIEBuild, @function

CompileUnitDIEBuild:

	// *** Basic block 0

	.local AddAttributeAndValue
	.local StringConstant
	.local UnsignedConstant
	.local AddressConstant
	.global DIEAllocateAbbreviation
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
	ld          a1, 216(s1)
	li          a0, 37		// 0x25 ASCII '%'
	call        StringConstant

	// *** Basic block 1

	mv          a2, a0
	mv          a1, s2
	mv          a0, s1
	call        AddAttributeAndValue

	// *** Basic block 2

	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	li          t0, 19		// 0x13 ASCII \x13
	mv          a0, t0
	call        UnsignedConstant

	// *** Basic block 3

	mv          a2, a0
	mv          a1, s2
	mv          a0, s1
	call        AddAttributeAndValue

	// *** Basic block 4

	addi        t0, s1, 176
	ld          a1, 16(t0)
	li          t0, 3		// 0x3 ASCII \x3
	mv          a0, t0
	call        StringConstant

	// *** Basic block 5

	mv          a2, a0
	mv          a1, s2
	mv          a0, s1
	call        AddAttributeAndValue

	// *** Basic block 6

	addi        t0, s1, 224
	ld          a1, 16(t0)
	li          t0, 27		// 0x1b ASCII \x1b
	mv          a0, t0
	call        StringConstant

	// *** Basic block 7

	mv          a2, a0
	mv          a1, s2
	mv          a0, s1
	call        AddAttributeAndValue

	// *** Basic block 8

	lla         a2, .str.70
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        AddressConstant

	// *** Basic block 9

	mv          a2, a0
	mv          a1, s2
	mv          a0, s1
	call        AddAttributeAndValue

	// *** Basic block 10

	lla         a2, .str.71
	li          t0, 255		// 0xff
	mv          a1, t0
	li          t0, 18		// 0x12 ASCII \x12
	mv          a0, t0
	call        AddressConstant

	// *** Basic block 11

	mv          a2, a0
	mv          a1, s2
	mv          a0, s1
	call        AddAttributeAndValue

	// *** Basic block 12

	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           DIEAllocateAbbreviation
.func_end_CompileUnitDIEBuild:
	.size CompileUnitDIEBuild, .func_end_CompileUnitDIEBuild-CompileUnitDIEBuild

	.local  CompileUnitDIEPrint
	.type CompileUnitDIEPrint, @function

CompileUnitDIEPrint:

	// *** Basic block 0

	.global printf
	// Leaf procedure, no stack frame generated
	lla         a0, .str.72
	j           printf
.func_end_CompileUnitDIEPrint:
	.size CompileUnitDIEPrint, .func_end_CompileUnitDIEPrint-CompileUnitDIEPrint

	.local  CompileUnitDIEEmit
	.type CompileUnitDIEEmit, @function

CompileUnitDIEEmit:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.local DIEBaseEmit
	j           DIEBaseEmit
.func_end_CompileUnitDIEEmit:
	.size CompileUnitDIEEmit, .func_end_CompileUnitDIEEmit-CompileUnitDIEEmit

	.global DebugBuilderEmitDebugInfo
	.type DebugBuilderEmitDebugInfo, @function

DebugBuilderEmitDebugInfo:

	// *** Basic block 0

	.local FlushAccumulator
	.global fprintf
	.global printf
	.global abort
	.local DIEEmit
	.local EmitEndOfChildren
	.global strlen
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
	call        FlushAccumulator

	// *** Basic block 1

	ld          s2, 264(s1)
	lla         a1, .str.73
	mv          a0, s2
	call        fprintf

	// *** Basic block 2

	lla         a1, .str.74
	mv          a0, s2
	call        fprintf

	// *** Basic block 3

	mv          s3, x0
	addi        t0, s1, 144
	ld          s4, 8(t0)
	bge         x0, s4, .DebugBuilderEmitDebugInfo_label_97

	// *** Basic block 4

	ld          s5, 144(s1)

	// *** Basic block 5

.DebugBuilderEmitDebugInfo_label_56:
	mv          a0, s1
	call        FlushAccumulator

	// *** Basic block 6

	slli        t0, s3, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	lw          t0, 0(s5)
	blt         t0, x0, .DebugBuilderEmitDebugInfo_label_70

	// *** Basic block 7

	j           .DebugBuilderEmitDebugInfo_label_87

	// *** Basic block 8

.DebugBuilderEmitDebugInfo_label_70:
	lla         a0, .str.75
	lla         a1, .str.76
	lla         a3, .str.77
	li          t0, 1455		// 0x5af
	mv          a2, t0
	call        printf

	// *** Basic block 9

	call        abort

	// *** Basic block 10

.DebugBuilderEmitDebugInfo_label_87:
	mv          a1, s5
	mv          a0, s1
	call        DIEEmit

	// *** Basic block 11

.DebugBuilderEmitDebugInfo_label_93:
	addi        s3, s3, 1
	bge         s3, s4, .DebugBuilderEmitDebugInfo_label_56

	// *** Basic block 12

.DebugBuilderEmitDebugInfo_label_97:
	mv          a0, s1
	call        EmitEndOfChildren

	// *** Basic block 13

	mv          a0, s1
	call        FlushAccumulator

	// *** Basic block 14

	lla         a1, .str.78
	mv          a0, s2
	call        fprintf

	// *** Basic block 15

	addi        s2, s1, 88
	ld          s4, 264(s1)
	lla         a1, .str.79
	mv          a0, s4
	call        fprintf

	// *** Basic block 16

	mv          s5, x0
	ld          t0, 8(s2)
	bge         x0, t0, .DebugBuilderEmitDebugInfo_label_146

	// *** Basic block 17

.DebugBuilderEmitDebugInfo_label_126:
	lla         a1, .str.80
	ld          s6, 0(s2)
	add         a2, s6, s5
	mv          a0, s4
	call        fprintf

	// *** Basic block 18

	ld          t0, 0(s2)
	add         a0, t0, s5
	call        strlen

	// *** Basic block 19

	addi        t0, a0, 1
	add         s5, s5, t0
	ld          t0, 8(s2)
	blt         s5, t0, .DebugBuilderEmitDebugInfo_label_126

	// *** Basic block 20

.DebugBuilderEmitDebugInfo_label_146:
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
.func_end_DebugBuilderEmitDebugInfo:
	.size DebugBuilderEmitDebugInfo, .func_end_DebugBuilderEmitDebugInfo-DebugBuilderEmitDebugInfo

.PCend:
	.data
compile_unit_virtuals:
	.type   compile_unit_virtuals,@object
	.local  compile_unit_virtuals
	.size   compile_unit_virtuals,32
	.p2align  3
	.global DIEBaseDestruct
	.long    DIEBaseDestruct
	.global CompileUnitDIEBuild
	.long    CompileUnitDIEBuild
	.global CompileUnitDIEPrint
	.long    CompileUnitDIEPrint
	.global CompileUnitDIEEmit
	.long    CompileUnitDIEEmit

base_type_virtuals:
	.type   base_type_virtuals,@object
	.local  base_type_virtuals
	.size   base_type_virtuals,32
	.p2align  3
	.word   0
	.space  4
	.global BaseTypeDIEBuild
	.long    BaseTypeDIEBuild
	.global BaseTypeDIEPrint
	.long    BaseTypeDIEPrint
	.global BaseTypeDIEEmit
	.long    BaseTypeDIEEmit

base_types:
	.type   base_types,@object
	.local  base_types
	.size   base_types,960
	.p2align  3
	.word   0
	.word   36
	.global base_type_virtuals
	.long    base_type_virtuals
	.space  40
	.global TypeIsChar
	.long    TypeIsChar
	.long    .str.7
	.word   6
	.word   1
	.word   1
	.word   36
	.global base_type_virtuals
	.long    base_type_virtuals
	.space  40
	.global TypeIsShort
	.long    TypeIsShort
	.long    .str.8
	.word   5
	.word   2
	.word   2
	.word   36
	.global base_type_virtuals
	.long    base_type_virtuals
	.space  40
	.global TypeIsInt
	.long    TypeIsInt
	.long    .str.9
	.word   5
	.word   4
	.word   3
	.word   36
	.global base_type_virtuals
	.long    base_type_virtuals
	.space  40
	.global TypeIsLong
	.long    TypeIsLong
	.long    .str.10
	.word   5
	.word   8
	.word   4
	.word   36
	.global base_type_virtuals
	.long    base_type_virtuals
	.space  40
	.global TypeIsUnsignedChar
	.long    TypeIsUnsignedChar
	.long    .str.11
	.word   8
	.word   1
	.word   5
	.word   36
	.global base_type_virtuals
	.long    base_type_virtuals
	.space  40
	.global TypeIsUnsignedShort
	.long    TypeIsUnsignedShort
	.long    .str.12
	.word   7
	.word   2
	.word   6
	.word   36
	.global base_type_virtuals
	.long    base_type_virtuals
	.space  40
	.global TypeIsUnsignedInt
	.long    TypeIsUnsignedInt
	.long    .str.13
	.word   7
	.word   4
	.word   7
	.word   36
	.global base_type_virtuals
	.long    base_type_virtuals
	.space  40
	.global TypeIsUnsignedLong
	.long    TypeIsUnsignedLong
	.long    .str.14
	.word   7
	.word   8
	.word   8
	.word   36
	.global base_type_virtuals
	.long    base_type_virtuals
	.space  40
	.global TypeIsFloat
	.long    TypeIsFloat
	.long    .str.15
	.word   4
	.word   4
	.word   9
	.word   36
	.global base_type_virtuals
	.long    base_type_virtuals
	.space  40
	.global TypeIsDouble
	.long    TypeIsDouble
	.long    .str.16
	.word   4
	.word   8
	.word   10
	.word   36
	.global base_type_virtuals
	.long    base_type_virtuals
	.space  40
	.global TypeIsBool
	.long    TypeIsBool
	.long    .str.17
	.word   2
	.word   1
	.word   11
	.word   38
	.global base_type_virtuals
	.long    base_type_virtuals
	.space  40
	.global TypeIsVoid
	.long    TypeIsVoid
	.long    .str.18
	.space  8

subrange_virtuals:
	.type   subrange_virtuals,@object
	.local  subrange_virtuals
	.size   subrange_virtuals,32
	.p2align  3
	.global DIEBaseDestruct
	.long    DIEBaseDestruct
	.global SubrangeDIEBuild
	.long    SubrangeDIEBuild
	.global SubrangeDIEPrint
	.long    SubrangeDIEPrint
	.global SubrangeDIEEmit
	.long    SubrangeDIEEmit

array_virtuals:
	.type   array_virtuals,@object
	.local  array_virtuals
	.size   array_virtuals,32
	.p2align  3
	.global ArrayDIEDestruct
	.long    ArrayDIEDestruct
	.global ArrayDIEBuild
	.long    ArrayDIEBuild
	.global ArrayDIEPrint
	.long    ArrayDIEPrint
	.global ArrayDIEEmit
	.long    ArrayDIEEmit

pointer_virtuals:
	.type   pointer_virtuals,@object
	.local  pointer_virtuals
	.size   pointer_virtuals,32
	.p2align  3
	.global DIEBaseDestruct
	.long    DIEBaseDestruct
	.global PointerDIEBuild
	.long    PointerDIEBuild
	.global PointerDIEPrint
	.long    PointerDIEPrint
	.global PointerDIEEmit
	.long    PointerDIEEmit

lexical_scope_virtuals:
	.type   lexical_scope_virtuals,@object
	.local  lexical_scope_virtuals
	.size   lexical_scope_virtuals,32
	.p2align  3
	.global LexicalScopeDestruct
	.long    LexicalScopeDestruct
	.global LexicalScopeDIEBuild
	.long    LexicalScopeDIEBuild
	.global LexicalScopeDIEPrint
	.long    LexicalScopeDIEPrint
	.global LexicalScopeDIEEmit
	.long    LexicalScopeDIEEmit

function_virtuals:
	.type   function_virtuals,@object
	.local  function_virtuals
	.size   function_virtuals,32
	.p2align  3
	.global FunctionDIEDestruct
	.long    FunctionDIEDestruct
	.global FunctionDIEBuild
	.long    FunctionDIEBuild
	.global FunctionDIEPrint
	.long    FunctionDIEPrint
	.global FunctionDIEEmit
	.long    FunctionDIEEmit

struct_virtuals:
	.type   struct_virtuals,@object
	.local  struct_virtuals
	.size   struct_virtuals,32
	.p2align  3
	.global StructDIEDestruct
	.long    StructDIEDestruct
	.global StructDIEBuild
	.long    StructDIEBuild
	.global StructDIEPrint
	.long    StructDIEPrint
	.global StructDIEEmit
	.long    StructDIEEmit

enum_virtuals:
	.type   enum_virtuals,@object
	.local  enum_virtuals
	.size   enum_virtuals,32
	.p2align  3
	.global EnumDIEDestruct
	.long    EnumDIEDestruct
	.global EnumDIEBuild
	.long    EnumDIEBuild
	.global EnumDIEPrint
	.long    EnumDIEPrint
	.global EnumDIEEmit
	.long    EnumDIEEmit

cv_virtuals:
	.type   cv_virtuals,@object
	.local  cv_virtuals
	.size   cv_virtuals,32
	.p2align  3
	.global DIEBaseDestruct
	.long    DIEBaseDestruct
	.global CVDIEBuild
	.long    CVDIEBuild
	.global CVDIEPrint
	.long    CVDIEPrint
	.global CVDIEEmit
	.long    CVDIEEmit

symbol_virtuals:
	.type   symbol_virtuals,@object
	.local  symbol_virtuals
	.size   symbol_virtuals,32
	.p2align  3
	.global NamedDIEDestruct
	.long    NamedDIEDestruct
	.global SymbolDIEBuild
	.long    SymbolDIEBuild
	.global SymbolDIEPrint
	.long    SymbolDIEPrint
	.global SymbolDIEEmit
	.long    SymbolDIEEmit

member_virtuals:
	.type   member_virtuals,@object
	.local  member_virtuals
	.size   member_virtuals,32
	.p2align  3
	.global NamedDIEDestruct
	.long    NamedDIEDestruct
	.global MemberDIEBuild
	.long    MemberDIEBuild
	.global MemberDIEPrint
	.long    MemberDIEPrint
	.global MemberDIEEmit
	.long    MemberDIEEmit

enum_const_virtuals:
	.type   enum_const_virtuals,@object
	.local  enum_const_virtuals
	.size   enum_const_virtuals,32
	.p2align  3
	.global NamedDIEDestruct
	.long    NamedDIEDestruct
	.global EnumConstDIEBuild
	.long    EnumConstDIEBuild
	.global EnumConstDIEPrint
	.long    EnumConstDIEPrint
	.global EnumConstDIEEmit
	.long    EnumConstDIEEmit

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.1, @object
	.size .str.1, 30

.str.2:
	.asciz "debug.c"
	.type .str.2, @object
	.size .str.2, 8

.str.3:
	.asciz "die != NULL"
	.type .str.3, @object
	.size .str.3, 12

.str.4:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.4, @object
	.size .str.4, 30

.str.5:
	.asciz "debug.c"
	.type .str.5, @object
	.size .str.5, 8

.str.6:
	.asciz "die != NULL"
	.type .str.6, @object
	.size .str.6, 12

.str.7:
	.asciz "char"
	.type .str.7, @object
	.size .str.7, 5

.str.8:
	.asciz "short"
	.type .str.8, @object
	.size .str.8, 6

.str.9:
	.asciz "int"
	.type .str.9, @object
	.size .str.9, 4

.str.10:
	.asciz "long"
	.type .str.10, @object
	.size .str.10, 5

.str.11:
	.asciz "unsigned char"
	.type .str.11, @object
	.size .str.11, 14

.str.12:
	.asciz "unsigned short"
	.type .str.12, @object
	.size .str.12, 15

.str.13:
	.asciz "unsigned int"
	.type .str.13, @object
	.size .str.13, 13

.str.14:
	.asciz "unsigned long"
	.type .str.14, @object
	.size .str.14, 14

.str.15:
	.asciz "float"
	.type .str.15, @object
	.size .str.15, 6

.str.16:
	.asciz "double"
	.type .str.16, @object
	.size .str.16, 7

.str.17:
	.asciz "bool"
	.type .str.17, @object
	.size .str.17, 5

.str.18:
	.asciz "void"
	.type .str.18, @object
	.size .str.18, 5

.str.19:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.19, @object
	.size .str.19, 30

.str.20:
	.asciz "debug.c"
	.type .str.20, @object
	.size .str.20, 8

.str.21:
	.asciz "false"
	.type .str.21, @object
	.size .str.21, 6

.str.22:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.22, @object
	.size .str.22, 30

.str.23:
	.asciz "debug.c"
	.type .str.23, @object
	.size .str.23, 8

.str.24:
	.asciz "false"
	.type .str.24, @object
	.size .str.24, 6

.str.25:
	.asciz "%s-%s"
	.type .str.25, @object
	.size .str.25, 6

.str.26:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.26, @object
	.size .str.26, 30

.str.27:
	.asciz "debug.c"
	.type .str.27, @object
	.size .str.27, 8

.str.28:
	.asciz "die != NULL"
	.type .str.28, @object
	.size .str.28, 12

.str.29:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.29, @object
	.size .str.29, 30

.str.30:
	.asciz "debug.c"
	.type .str.30, @object
	.size .str.30, 8

.str.31:
	.asciz "abbrev != NULL"
	.type .str.31, @object
	.size .str.31, 15

.str.32:
	.asciz "\t.byte 0x%02x\n"
	.type .str.32, @object
	.size .str.32, 15

.str.33:
	.asciz "\t.word .DW_DIE%d\n"
	.type .str.33, @object
	.size .str.33, 18

.str.34:
	.asciz "\t.string \"%s\"\n"
	.type .str.34, @object
	.size .str.34, 15

.str.35:
	.asciz "\t.word 0x%zx\n"
	.type .str.35, @object
	.size .str.35, 14

.str.36:
	.asciz "\t.long %s\n"
	.type .str.36, @object
	.size .str.36, 11

.str.37:
	.asciz "\t.word %s\n"
	.type .str.37, @object
	.size .str.37, 11

.str.38:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.38, @object
	.size .str.38, 30

.str.39:
	.asciz "debug.c"
	.type .str.39, @object
	.size .str.39, 8

.str.40:
	.asciz "false"
	.type .str.40, @object
	.size .str.40, 6

.str.41:
	.asciz ".DW_DIE%d:\t\t// %s (abbrev %d)\n"
	.type .str.41, @object
	.size .str.41, 31

.str.42:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.42, @object
	.size .str.42, 30

.str.43:
	.asciz "debug.c"
	.type .str.43, @object
	.size .str.43, 8

.str.44:
	.asciz "die != NULL"
	.type .str.44, @object
	.size .str.44, 12

.str.45:
	.asciz "  "
	.type .str.45, @object
	.size .str.45, 3

.str.46:
	.asciz "%s, encoding: %s, byte_size: %d, name: %s\n"
	.type .str.46, @object
	.size .str.46, 43

.str.47:
	.asciz "%s: upper_bound: %d\n"
	.type .str.47, @object
	.size .str.47, 21

.str.48:
	.asciz "%s:\n"
	.type .str.48, @object
	.size .str.48, 5

.str.49:
	.asciz "%s:\n"
	.type .str.49, @object
	.size .str.49, 5

.str.50:
	.asciz "%s: low_pc: %s, high_pc: %s\n"
	.type .str.50, @object
	.size .str.50, 29

.str.51:
	.asciz "%s: %s: low_pc: %s, high_pc: %s\n"
	.type .str.51, @object
	.size .str.51, 33

.str.52:
	.asciz "%s: %s, byte_size: %d\n"
	.type .str.52, @object
	.size .str.52, 23

.str.53:
	.asciz "%s: %s\n"
	.type .str.53, @object
	.size .str.53, 8

.str.54:
	.asciz "%s:\n"
	.type .str.54, @object
	.size .str.54, 5

.str.55:
	.asciz "%s: %s "
	.type .str.55, @object
	.size .str.55, 8

.str.56:
	.asciz "%s\n"
	.type .str.56, @object
	.size .str.56, 4

.str.57:
	.asciz "null"
	.type .str.57, @object
	.size .str.57, 5

.str.58:
	.asciz "offset %d\n"
	.type .str.58, @object
	.size .str.58, 11

.str.59:
	.asciz "<unknown>\n"
	.type .str.59, @object
	.size .str.59, 11

.str.60:
	.asciz "reg %d\n"
	.type .str.60, @object
	.size .str.60, 8

.str.61:
	.asciz "  byte_offset: %d, is_bitfield: %d\n"
	.type .str.61, @object
	.size .str.61, 36

.str.62:
	.asciz "  bit_offset: %d, bit_size: %d\n"
	.type .str.62, @object
	.size .str.62, 32

.str.63:
	.asciz "%s: value: %" PRId64 "\n"
	.type .str.63, @object
	.size .str.63, 17

.str.64:
	.asciz "%d: %s [%s_children]\n"
	.type .str.64, @object
	.size .str.64, 22

.str.65:
	.asciz "has"
	.type .str.65, @object
	.size .str.65, 4

.str.66:
	.asciz "no"
	.type .str.66, @object
	.size .str.66, 3

.str.67:
	.asciz "%s %s\n"
	.type .str.67, @object
	.size .str.67, 7

.str.68:
	.asciz "\n"
	.type .str.68, @object
	.size .str.68, 2

.str.69:
	.asciz "\n\t.section .debug_abbrev,\"\",@progbits\n"
	.type .str.69, @object
	.size .str.69, 39

.str.70:
	.asciz ".PCbegin"
	.type .str.70, @object
	.size .str.70, 9

.str.71:
	.asciz ".PCend-.PCbegin"
	.type .str.71, @object
	.size .str.71, 16

.str.72:
	.asciz "compile_unit\n"
	.type .str.72, @object
	.size .str.72, 14

.str.73:
	.asciz "\n\t.section .debug_info,\"\",@progbits\n"
	.type .str.73, @object
	.size .str.73, 37

.str.74:
	.asciz "\t.word .DW_info_end\n"
	.type .str.74, @object
	.size .str.74, 21

.str.75:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.75, @object
	.size .str.75, 30

.str.76:
	.asciz "debug.c"
	.type .str.76, @object
	.size .str.76, 8

.str.77:
	.asciz "die->id >= 0"
	.type .str.77, @object
	.size .str.77, 13

.str.78:
	.asciz ".DW_info_end:\n"
	.type .str.78, @object
	.size .str.78, 15

.str.79:
	.asciz "\n\t.section .debug_str,\"MS\",@progbits\n"
	.type .str.79, @object
	.size .str.79, 38

.str.80:
	.asciz "\t.string \"%s\"\n"
	.type .str.80, @object
	.size .str.80, 15

