	.file   "dwarf.c"
	.text
	.option pic
.PCbegin:
	.local  CompareString
	.type CompareString, @function

CompareString:

	// *** Basic block 0

	.global strcmp
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, t0
	mv          t3, t1
	ld          t4, 0(t2)
	ld          t5, 0(t3)
	ld          a0, 16(t4)
	ld          a1, 16(t5)
	j           strcmp
.func_end_CompareString:
	.size CompareString, .func_end_CompareString-CompareString

	.global DwarfInit
	.type DwarfInit, @function

DwarfInit:

	// *** Basic block 0

	.global MapInitForStringKeys
	.global VectorInit
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
	call        MapInitForStringKeys

	// *** Basic block 1

	li          t0, 2		// 0x2 ASCII \x2
	sd          t0, 32(s1)
	addi        a0, s1, 40
	call        VectorInit

	// *** Basic block 2

	addi        a0, s1, 64
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorInit
.func_end_DwarfInit:
	.size DwarfInit, .func_end_DwarfInit-DwarfInit

	.local  DeleteDirectoryEntry
	.type DeleteDirectoryEntry, @function

DeleteDirectoryEntry:

	// *** Basic block 0

	.global StringDelete
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 0(t0)
	j           StringDelete
.func_end_DeleteDirectoryEntry:
	.size DeleteDirectoryEntry, .func_end_DeleteDirectoryEntry-DeleteDirectoryEntry

	.global DwarfDestruct
	.type DwarfDestruct, @function

DwarfDestruct:

	// *** Basic block 0

	.global MapDestructWithContents
	.local DeleteDirectoryEntry
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
	la          a1, DeleteDirectoryEntry
	call        MapDestructWithContents

	// *** Basic block 1

	addi        a0, s1, 40
	mv          a1, x0
	call        VectorDestructWithContents

	// *** Basic block 2

	addi        a0, s1, 64
	mv          a1, x0
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorDestructWithContents
.func_end_DwarfDestruct:
	.size DwarfDestruct, .func_end_DwarfDestruct-DwarfDestruct

	.global NewFileEntry
	.type NewFileEntry, @function

NewFileEntry:

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
	sd          s1, 0(s3)
	sw          s2, 8(s3)
	mv          a0, s3

	// *** Basic block 2

.NewFileEntry_label_23:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewFileEntry:
	.size NewFileEntry, .func_end_NewFileEntry-NewFileEntry

	.global FileEntryDestruct
	.type FileEntryDestruct, @function

FileEntryDestruct:

	// *** Basic block 0

	.global StringDelete
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 0(t0)
	j           StringDelete
.func_end_FileEntryDestruct:
	.size FileEntryDestruct, .func_end_FileEntryDestruct-FileEntryDestruct

	.global FileEntryDelete
	.type FileEntryDelete, @function

FileEntryDelete:

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
	.global FileEntryDestruct
	.global free
	mv          s1, a0
	call        FileEntryDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_FileEntryDelete:
	.size FileEntryDelete, .func_end_FileEntryDelete-FileEntryDelete

	.global DwarfAddFile
	.type DwarfAddFile, @function

DwarfAddFile:

	// *** Basic block 0

	.global StringLastIndexOf
	.global NewString
	.global StringInitFromSegment
	.global MapFindPointerKey
	.global MapInsert
	.global StringSetString
	.global VectorAppend
	.global NewFileEntry
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -80(s0)
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
	mv          s3, x0
	lla         a1, .str.1
	mv          a0, s1
	call        StringLastIndexOf

	// *** Basic block 1

	mv          s4, a0
	mv          a0, x0
	call        NewString

	// *** Basic block 2

	mv          s5, a0
	li          t0, -1		// 0xffffffffffffffff
	beq         s4, t0, .DwarfAddFile_label_118

	// *** Basic block 3

	addi        a0, s0, -80
	ld          s6, 16(s1)
	mv          a2, s4
	mv          a1, s6
	call        StringInitFromSegment

	// *** Basic block 4

	addi        a1, s0, -80
	mv          a0, s2
	call        MapFindPointerKey

	// *** Basic block 5

	mv          s7, a0
	bne         s7, x0, .DwarfAddFile_label_103

	// *** Basic block 6

	addi        t0, s0, -80
	ld          a0, 16(t0)
	call        NewString

	// *** Basic block 7

	mv          s8, a0
	ld          t0, 32(s2)
	addi        s3, t0, -1
	sd          s8, -40(s0)
	addi        t0, s0, -40
	ld          t1, 32(s2)
	addi        t1, t1, 1
	sd          t1, 32(s2)
	sd          t1, 8(t0)
	addi        sp, sp, -16
	ld          t0, -40(s0)
	sd          t0, 0(sp)
	ld          t0, -32(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	mv          a0, s2
	call        MapInsert

	// *** Basic block 8

	j           .DwarfAddFile_label_105

	// *** Basic block 9

.DwarfAddFile_label_103:
	addi        s3, s7, -1

	// *** Basic block 10

.DwarfAddFile_label_105:
	add         t0, s6, s4
	addi        a1, t0, 1
	ld          t0, 24(s1)
	sub         t0, t0, s4
	addi        a2, t0, -1
	mv          a0, s5
	call        StringInitFromSegment

	// *** Basic block 11

	j           .DwarfAddFile_label_124

	// *** Basic block 12

.DwarfAddFile_label_118:
	mv          a1, s1
	mv          a0, s5
	call        StringSetString

	// *** Basic block 13

.DwarfAddFile_label_124:
	addi        s6, s2, 40
	sext.w      a1, s3
	mv          a0, s5
	call        NewFileEntry

	// *** Basic block 14

	mv          a1, a0
	mv          a0, s6
	call        VectorAppend

	// *** Basic block 15

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
.func_end_DwarfAddFile:
	.size DwarfAddFile, .func_end_DwarfAddFile-DwarfAddFile

	.global DwarfAddLocation
	.type DwarfAddLocation, @function

DwarfAddLocation:

	// *** Basic block 0

	.global VectorLast
	.global VectorAppend
	.global NewLocationEntry
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
	mv          s2, a4
	mv          s3, a1
	mv          s4, a2
	mv          s5, a3
	addi        t0, s1, 64
	ld          t0, 8(t0)
	bge         x0, t0, .DwarfAddLocation_label_44

	// *** Basic block 1

	addi        a0, s1, 64
	call        VectorLast

	// *** Basic block 2

	mv          s6, a0
	ld          t0, 16(s6)
	bne         t0, s2, .DwarfAddLocation_label_43

	// *** Basic block 3

.DwarfAddLocation_label_40:
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

	// *** Basic block 4

.DwarfAddLocation_label_43:

	// *** Basic block 5

.DwarfAddLocation_label_44:
	addi        s7, s1, 64
	mv          a3, s2
	mv          a2, s5
	mv          a1, s4
	mv          a0, s3
	call        NewLocationEntry

	// *** Basic block 6

	mv          a1, a0
	mv          a0, s7
	call        VectorAppend

	// *** Basic block 7

	j           .DwarfAddLocation_label_40
.func_end_DwarfAddLocation:
	.size DwarfAddLocation, .func_end_DwarfAddLocation-DwarfAddLocation

	.local  WriteULEB128
	.type WriteULEB128, @function

WriteULEB128:

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
	mv          s1, a0
	mv          s2, a1

	// *** Basic block 1

.WriteULEB128_label_14:
	andi        s3, s1, 127
	srai        s1, s1, 7
	beqz        s1, .WriteULEB128_label_24

	// *** Basic block 2

	ori         s3, s3, -128

	// *** Basic block 3

.WriteULEB128_label_24:
	mv          a1, s3
	mv          a0, s2
	call        BufferAppendByte

	// *** Basic block 4

.WriteULEB128_label_30:
	bnez        s1, .WriteULEB128_label_14

	// *** Basic block 5

.WriteULEB128_label_32:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_WriteULEB128:
	.size WriteULEB128, .func_end_WriteULEB128-WriteULEB128

	.local  WriteSLEB128
	.type WriteSLEB128, @function

WriteSLEB128:

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
	mv          s1, a0
	mv          s2, a1
	li          s3, 1		// 0x1 ASCII \x1

	// *** Basic block 1

.WriteSLEB128_label_21:
	andi        s4, s1, 127
	srli        s1, s1, 7
	seqz        t0, s1
	bnez        s1, .WriteSLEB128_label_34

	// *** Basic block 2

	andi        t1, s4, 64
	seqz        t0, t1

	// *** Basic block 3

.WriteSLEB128_label_34:
	bnez        t0, .WriteSLEB128_label_44

	// *** Basic block 4

	addi        t1, s1, 1
	seqz        t0, t1
	li          t1, -1		// 0xffffffffffffffff
	bne         s1, t1, .WriteSLEB128_label_43

	// *** Basic block 5

	andi        t1, s4, 64
	snez        t0, t1

	// *** Basic block 6

.WriteSLEB128_label_43:

	// *** Basic block 7

.WriteSLEB128_label_44:
	beqz        t0, .WriteSLEB128_label_49

	// *** Basic block 8

	mv          s3, x0
	j           .WriteSLEB128_label_51

	// *** Basic block 9

.WriteSLEB128_label_49:
	ori         s4, s4, -128

	// *** Basic block 10

.WriteSLEB128_label_51:
	mv          a1, s4
	mv          a0, s2
	call        BufferAppendByte

	// *** Basic block 11

	bnez        s3, .WriteSLEB128_label_21

	// *** Basic block 12

.WriteSLEB128_label_58:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_WriteSLEB128:
	.size WriteSLEB128, .func_end_WriteSLEB128-WriteSLEB128

	.local  EmitDirectory
	.type EmitDirectory, @function

EmitDirectory:

	// *** Basic block 0

	.global BufferAppend
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	ld          t2, 0(t0)
	mv          t3, t1
	ld          a1, 16(t2)
	ld          t2, 24(t2)
	addi        a2, t2, 1
	mv          a0, t3
	j           BufferAppend
.func_end_EmitDirectory:
	.size EmitDirectory, .func_end_EmitDirectory-EmitDirectory

	.global DwarfBuildDebugLineContents
	.type DwarfBuildDebugLineContents, @function

DwarfBuildDebugLineContents:

	// *** Basic block 0

	.global BufferAddSpace
	.global BufferAppendHalfLE
	.global BufferAppendByte
	.global BufferAppend
	.local standard_opcode_lengths
	.global MapTraverse
	.local EmitDirectory
	.local WriteULEB128
	.local WriteSLEB128
	.global BufferAppendLongLE
	.global BufferAlignLength
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -16(s0)
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
	mv          s2, a0
	li          s3, 4		// 0x4 ASCII \x4
	mv          a1, s3
	mv          a0, s1
	call        BufferAddSpace

	// *** Basic block 1

	mv          a1, s3
	mv          a0, s1
	call        BufferAppendHalfLE

	// *** Basic block 2

	mv          a1, s3
	mv          a0, s1
	call        BufferAddSpace

	// *** Basic block 3

	lbu         a1, 88(s2)
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 4

	li          s3, 1		// 0x1 ASCII \x1
	mv          a1, s3
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 5

	mv          a1, s3
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 6

	lb          a1, 89(s2)
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 7

	lbu         a1, 90(s2)
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 8

	li          t0, 13		// 0xd ASCII \xd
	mv          a1, t0
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 9

	lla         a1, standard_opcode_lengths
	li          t0, 12		// 0xc ASCII \xc
	mv          a2, t0
	mv          a0, s1
	call        BufferAppend

	// *** Basic block 10

	mv          a2, s1
	la          t0, EmitDirectory
	mv          a1, t0
	mv          a0, s2
	call        MapTraverse

	// *** Basic block 11

	mv          a1, x0
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 12

	mv          s3, x0
	addi        t0, s2, 40
	ld          s4, 8(t0)
	bge         x0, s4, .DwarfBuildDebugLineContents_label_173

	// *** Basic block 13

	ld          s5, 40(s2)

	// *** Basic block 14

.DwarfBuildDebugLineContents_label_136:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	ld          t0, 0(s5)
	ld          a1, 16(t0)
	ld          t0, 24(t0)
	addi        a2, t0, 1
	mv          a0, s1
	call        BufferAppend

	// *** Basic block 15

	lw          a0, 8(s5)
	mv          a1, s1
	call        WriteULEB128

	// *** Basic block 16

	mv          a1, s1
	mv          a0, x0
	call        WriteULEB128

	// *** Basic block 17

	mv          a1, s1
	mv          a0, x0
	call        WriteULEB128

	// *** Basic block 18

.DwarfBuildDebugLineContents_label_169:
	addi        s3, s3, 1
	bge         s3, s4, .DwarfBuildDebugLineContents_label_136

	// *** Basic block 19

.DwarfBuildDebugLineContents_label_173:
	mv          a1, x0
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 20

	ld          s4, 0(s1)
	addi        s5, s4, 6
	ld          s6, 8(s1)
	li          t0, 4294967295		// 0xffffffff
	and         s7, s6, t0
	addi        t0, s7, -10
	sw          t0, 0(s5)
	mv          s5, x0
	mv          s8, x0
	addi        t0, s2, 64
	ld          t0, 8(t0)
	bge         x0, t0, .DwarfBuildDebugLineContents_label_339

	// *** Basic block 21

.DwarfBuildDebugLineContents_label_198:
	ld          t0, 64(s2)
	slli        t1, s8, 3
	add         t0, t0, t1
	ld          s9, 0(t0)
	bne         s5, x0, .DwarfBuildDebugLineContents_label_262

	// *** Basic block 22

	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 23

	lw          t0, 4(s9)
	addi        a0, t0, -1
	mv          a1, s1
	call        WriteSLEB128

	// *** Basic block 24

	mv          a1, x0
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 25

	mv          a1, s1
	li          t0, 9		// 0x9 ASCII \x9
	mv          a0, t0
	call        WriteULEB128

	// *** Basic block 26

	li          s10, 2		// 0x2 ASCII \x2
	mv          a1, s10
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 27

	sw          s6, 92(s2)
	mv          a1, x0
	mv          a0, s1
	call        BufferAppendLongLE

	// *** Basic block 28

	mv          a1, s10
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 29

	ld          t0, 16(s9)
	lbu         t1, 88(s2)
	div         a0, t0, t1
	mv          a1, s1
	call        WriteULEB128

	// *** Basic block 30

	j           .DwarfBuildDebugLineContents_label_330

	// *** Basic block 31

.DwarfBuildDebugLineContents_label_262:
	lw          t0, 4(s9)
	lw          t1, 4(s5)
	sub         s6, t0, t1
	ld          t0, 16(s9)
	ld          t1, 16(s5)
	sub         t0, t0, t1
	lbu         t1, 88(s2)
	div         t0, t0, t1
	sext.w      s10, t0
	lb          t0, 89(s2)
	sub         t0, s6, t0
	lbu         t1, 90(s2)
	mul         t1, t1, s10
	add         t0, t0, t1
	addi        s11, t0, 13
	li          t0, 255		// 0xff
	bge         t0, s11, .DwarfBuildDebugLineContents_label_321

	// *** Basic block 32

	beqz        s6, .DwarfBuildDebugLineContents_label_305

	// *** Basic block 33

	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 34

	mv          a1, s1
	mv          a0, s6
	call        WriteSLEB128

	// *** Basic block 35

.DwarfBuildDebugLineContents_label_305:
	beqz        s10, .DwarfBuildDebugLineContents_label_319

	// *** Basic block 36

	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 37

	mv          a1, s1
	mv          a0, s10
	call        WriteULEB128

	// *** Basic block 38

.DwarfBuildDebugLineContents_label_319:
	j           .DwarfBuildDebugLineContents_label_329

	// *** Basic block 39

.DwarfBuildDebugLineContents_label_321:
	slli        t0, s11, 56
	srai        a1, t0, 56
	mv          a0, s1
	call        BufferAppendByte

	// *** Basic block 40

.DwarfBuildDebugLineContents_label_329:

	// *** Basic block 41

.DwarfBuildDebugLineContents_label_330:
	mv          s5, s9

	// *** Basic block 42

.DwarfBuildDebugLineContents_label_332:
	addi        s8, s8, 1
	addi        t0, s2, 64
	ld          t0, 8(t0)
	bge         s8, t0, .DwarfBuildDebugLineContents_label_198

	// *** Basic block 43

.DwarfBuildDebugLineContents_label_339:
	addi        t0, s7, -4
	sw          t0, 0(s4)
	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	mv          a0, s1
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
	j           BufferAlignLength
.func_end_DwarfBuildDebugLineContents:
	.size DwarfBuildDebugLineContents, .func_end_DwarfBuildDebugLineContents-DwarfBuildDebugLineContents

	.global DwarfDebugLineRelocation
	.type DwarfDebugLineRelocation, @function

DwarfDebugLineRelocation:

	// *** Basic block 0

	.global NewAssemblerRelocation
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a2
	mv          t2, a3
	mv          t3, a0
	lw          a3, 92(t3)
	mv          a4, x0
	mv          a2, t2
	mv          a1, t1
	mv          a0, t0
	j           NewAssemblerRelocation
.func_end_DwarfDebugLineRelocation:
	.size DwarfDebugLineRelocation, .func_end_DwarfDebugLineRelocation-DwarfDebugLineRelocation

.PCend:
	.data
standard_opcode_lengths:
	.type   standard_opcode_lengths,@object
	.local  standard_opcode_lengths
	.size   standard_opcode_lengths,12
	.p2align  0
	.byte   0
	.byte   1
	.byte   1
	.byte   1
	.byte   1
	.byte   0
	.byte   0
	.byte   0
	.byte   1
	.byte   0
	.byte   0
	.byte   1

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "/"
	.type .str.1, @object
	.size .str.1, 2

