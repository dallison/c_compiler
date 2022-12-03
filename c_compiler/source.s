	.file   "source.c"
	.text
	.option pic
.PCbegin:
	.global NewFile
	.type NewFile, @function

NewFile:

	// *** Basic block 0

	.global malloc
	.global StringInit
	.global VectorInit
	.local all_files
	.global VectorAppend
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
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        StringInit

	// *** Basic block 2

	addi        a0, s2, 40
	call        VectorInit

	// *** Basic block 3

	la          t0, all_files
	ld          s3, 8(t0)
	la          a0, all_files
	mv          a1, s2
	call        VectorAppend

	// *** Basic block 4

	li          t0, 4294967295		// 0xffffffff
	and         a0, s3, t0

	// *** Basic block 5

.NewFile_label_42:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewFile:
	.size NewFile, .func_end_NewFile-NewFile

	.local  FindFile
	.type FindFile, @function

FindFile:

	// *** Basic block 0

	.local file_map_initialized
	.global MapInitForCharPointerKeys
	.local file_map
	.global MapFindPointerKey
	.global NewFile
	.local all_files
	.global MapInsert
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	la          t0, file_map_initialized
	lb          t0, 0(t0)
	not         t0, t0
	beqz        t0, .FindFile_label_29

	// *** Basic block 1

	la          a0, file_map
	call        MapInitForCharPointerKeys

	// *** Basic block 2

	la          t0, file_map_initialized
	li          t1, 1		// 0x1 ASCII \x1
	sb          t1, 0(t0)

	// *** Basic block 3

.FindFile_label_29:
	la          a0, file_map
	mv          a1, s1
	call        MapFindPointerKey

	// *** Basic block 4

	mv          s2, a0
	beq         s2, x0, .FindFile_label_49

	// *** Basic block 5

	addi        t0, s2, -1
	li          t1, 4294967295		// 0xffffffff
	and         a0, t0, t1

	// *** Basic block 6

.FindFile_label_46:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.FindFile_label_49:
	mv          a0, s1
	call        NewFile

	// *** Basic block 8

	mv          s3, a0
	la          t0, all_files
	ld          t0, 0(t0)
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	ld          t0, 16(s4)
	sd          t0, -32(s0)
	addi        t0, s0, -32
	addi        t1, s3, 1
	sd          t1, 8(t0)
	la          a0, file_map
	addi        sp, sp, -16
	ld          t0, -32(s0)
	sd          t0, 0(sp)
	ld          t0, -24(s0)
	sd          t0, 8(sp)
	addi        a1, sp, 0
	call        MapInsert

	// *** Basic block 9

	mv          a0, s3
	j           .FindFile_label_46
.func_end_FindFile:
	.size FindFile, .func_end_FindFile-FindFile

	.global FileDestruct
	.type FileDestruct, @function

FileDestruct:

	// *** Basic block 0

	.global StringDestruct
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

	addi        a0, s1, 40
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorDestruct
.func_end_FileDestruct:
	.size FileDestruct, .func_end_FileDestruct-FileDestruct

	.global ClearAllFiles
	.type ClearAllFiles, @function

ClearAllFiles:

	// *** Basic block 0

	.local all_files
	.global FileDestruct
	.global free
	.global VectorDestruct
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
	mv          s1, x0
	la          t0, all_files
	ld          s2, 8(t0)
	bge         x0, s2, .ClearAllFiles_label_35

	// *** Basic block 1

	la          t0, all_files
	ld          s3, 0(t0)

	// *** Basic block 2

.ClearAllFiles_label_20:
	slli        t0, s1, 3
	add         t0, s3, t0
	ld          a0, 0(t0)
	call        FileDestruct

	// *** Basic block 3

	call        free

	// *** Basic block 4

.ClearAllFiles_label_31:
	addi        s1, s1, 1
	bge         s1, s2, .ClearAllFiles_label_20

	// *** Basic block 5

.ClearAllFiles_label_35:
	la          a0, all_files
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           VectorDestruct
.func_end_ClearAllFiles:
	.size ClearAllFiles, .func_end_ClearAllFiles-ClearAllFiles

	.local  ResetFiles
	.type ResetFiles, @function

ResetFiles:

	// *** Basic block 0

	.local all_files
	.global FileDestruct
	.global free
	.global VectorClear
	.global MapClear
	.local file_map
	.local file_map_initialized
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
	mv          s1, x0
	la          t0, all_files
	ld          s2, 8(t0)
	bge         x0, s2, .ResetFiles_label_38

	// *** Basic block 1

	la          t0, all_files
	ld          s3, 0(t0)

	// *** Basic block 2

.ResetFiles_label_23:
	slli        t0, s1, 3
	add         t0, s3, t0
	ld          a0, 0(t0)
	call        FileDestruct

	// *** Basic block 3

	call        free

	// *** Basic block 4

.ResetFiles_label_34:
	addi        s1, s1, 1
	bge         s1, s2, .ResetFiles_label_23

	// *** Basic block 5

.ResetFiles_label_38:
	la          a0, all_files
	call        VectorClear

	// *** Basic block 6

	la          a0, file_map
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           MapClear
.func_end_ResetFiles:
	.size ResetFiles, .func_end_ResetFiles-ResetFiles

	.global NewSourceLocation
	.type NewSourceLocation, @function

NewSourceLocation:

	// *** Basic block 0

	.local FindFile
	.local all_files
	.global VectorAppend
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
	mv          s2, a2
	mv          s3, a3
	mv          s4, a1
	lwu         t0, 84(s1)
	li          t1, -1		// 0xffffffffffffffff
	bne         t0, t1, .NewSourceLocation_label_47

	// *** Basic block 1

	ld          a0, 16(s1)
	call        FindFile

	// *** Basic block 2

	sw          a0, 84(s1)

	// *** Basic block 3

.NewSourceLocation_label_47:
	lwu         s5, 84(s1)
	li          s6, 65535		// 0xffff
	slt         t0, s6, s2
	blt         s6, s2, .NewSourceLocation_label_58

	// *** Basic block 4

	slt         t0, s6, s5

	// *** Basic block 5

.NewSourceLocation_label_58:
	beqz        t0, .NewSourceLocation_label_65

	// *** Basic block 6

	li          a0, -2		// 0xfffffffffffffffe

	// *** Basic block 7

.NewSourceLocation_label_62:
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

	// *** Basic block 8

.NewSourceLocation_label_65:
	la          t0, all_files
	ld          t0, 0(t0)
	slli        t1, s5, 3
	add         t0, t0, t1
	ld          s6, 0(t0)
	addi        t0, s6, 40
	ld          s7, 8(t0)
	sub         s8, s3, s2
	li          t1, 1048573		// 0xffffd
	slt         t0, t1, s7
	blt         t1, s7, .NewSourceLocation_label_86

	// *** Basic block 9

	li          t1, 8191		// 0x1fff
	slt         t0, t1, s8

	// *** Basic block 10

.NewSourceLocation_label_86:
	beqz        t0, .NewSourceLocation_label_91

	// *** Basic block 11

	li          a0, -2		// 0xfffffffffffffffe
	j           .NewSourceLocation_label_62

	// *** Basic block 12

.NewSourceLocation_label_91:
	mv          s3, s4
	seqz        t0, s7
	beqz        s7, .NewSourceLocation_label_104

	// *** Basic block 13

	ld          t1, 40(s6)
	addi        t2, s7, -1
	slli        t2, t2, 3
	add         t1, t1, t2
	ld          t1, 0(t1)
	sub         t1, t1, s3
	snez        t0, t1

	// *** Basic block 14

.NewSourceLocation_label_104:
	beqz        t0, .NewSourceLocation_label_112

	// *** Basic block 15

	addi        a0, s6, 40
	mv          a1, s3
	call        VectorAppend

	// *** Basic block 16

	j           .NewSourceLocation_label_114

	// *** Basic block 17

.NewSourceLocation_label_112:
	addi        s7, s7, -1

	// *** Basic block 18

.NewSourceLocation_label_114:
	slli        t0, s5, 20
	mv          t1, s7
	or          t0, t0, t1
	slli        t1, s2, 49
	or          t0, t0, t1
	slli        t1, s8, 36
	or          s4, t0, t1
	mv          a0, s4
	j           .NewSourceLocation_label_62
.func_end_NewSourceLocation:
	.size NewSourceLocation, .func_end_NewSourceLocation-NewSourceLocation

	.global DecodeSourceLocation
	.type DecodeSourceLocation, @function

DecodeSourceLocation:

	// *** Basic block 0

	.local all_files
	// Leaf procedure, no stack frame generated
	mv          t0, a4
	mv          t1, a3
	mv          t2, a2
	mv          t3, a0
	mv          t4, a1
	lw          t5, 0(t1)
	sw          t5, 0(t0)
	lw          a5, 0(t2)
	sw          a5, 0(t6)
	sw          x0, 0(t6)
	li          t6, -1		// 0xffffffffffffffff
	bne         t3, t6, .DecodeSourceLocation_label_52

	// *** Basic block 1

	lla         t6, .str.1
	sd          t6, 0(t4)

	// *** Basic block 2

.DecodeSourceLocation_label_49:
	ret         

	// *** Basic block 3

.DecodeSourceLocation_label_52:
	li          t6, -2		// 0xfffffffffffffffe
	bne         t3, t6, .DecodeSourceLocation_label_61

	// *** Basic block 4

	lla         t6, .str.2
	sd          t6, 0(t4)
	ret         

	// *** Basic block 5

.DecodeSourceLocation_label_61:
	srli        t6, t3, 20
	li          a0, 65535		// 0xffff
	and         a1, t6, a0
	mv          t6, t3
	li          a2, 1048575		// 0xfffff
	and         a3, t6, a2
	srli        t6, t3, 36
	li          a2, 8191		// 0x1fff
	and         a4, t6, a2
	srli        t6, t3, 49
	and         t6, t6, a0
	sw          t6, 0(t1)
	la          t6, all_files
	ld          t6, 8(t6)
	bge         a1, t6, .DecodeSourceLocation_label_113

	// *** Basic block 6

	la          t1, all_files
	ld          t1, 0(t1)
	slli        t3, a1, 3
	add         t1, t1, t3
	ld          t3, 0(t1)
	ld          t1, 16(t3)
	sd          t1, 0(t4)
	addi        t1, t3, 40
	ld          t1, 8(t1)
	bge         a3, t1, .DecodeSourceLocation_label_109

	// *** Basic block 7

	ld          t1, 40(t3)
	slli        t3, a3, 3
	add         t1, t1, t3
	ld          t1, 0(t1)
	sw          t1, 0(t2)

	// *** Basic block 8

.DecodeSourceLocation_label_109:
	add         t1, t5, a4
	sw          t1, 0(t0)
	j           .DecodeSourceLocation_label_117

	// *** Basic block 9

.DecodeSourceLocation_label_113:
	lla         t1, .str.3
	sd          t1, 0(t4)

	// *** Basic block 10

.DecodeSourceLocation_label_117:
	j           .DecodeSourceLocation_label_49
.func_end_DecodeSourceLocation:
	.size DecodeSourceLocation, .func_end_DecodeSourceLocation-DecodeSourceLocation

	.global SourceLocationNumbers
	.type SourceLocationNumbers, @function

SourceLocationNumbers:

	// *** Basic block 0

	.local all_files
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a2
	mv          t2, a3
	mv          t3, a0
	lw          t4, 0(t1)
	sw          t4, 0(t0)
	lw          t5, 0(t2)
	sw          t5, 0(t4)
	sw          x0, 0(t4)
	addi        t5, t3, 1
	seqz        t4, t5
	li          t5, -1		// 0xffffffffffffffff
	beq         t3, t5, .SourceLocationNumbers_label_42

	// *** Basic block 1

	addi        t5, t3, 2
	seqz        t4, t5

	// *** Basic block 2

.SourceLocationNumbers_label_42:
	beqz        t4, .SourceLocationNumbers_label_47

	// *** Basic block 3

.SourceLocationNumbers_label_44:
	ret         

	// *** Basic block 4

.SourceLocationNumbers_label_47:
	srli        t4, t3, 20
	li          t5, 65535		// 0xffff
	and         t6, t4, t5
	mv          t4, t3
	li          a0, 1048575		// 0xfffff
	and         a1, t4, a0
	srli        t4, t3, 49
	and         t4, t4, t5
	sw          t4, 0(t2)
	la          t4, all_files
	ld          t4, 8(t4)
	bge         t6, t4, .SourceLocationNumbers_label_89

	// *** Basic block 5

	la          t2, all_files
	ld          t2, 0(t2)
	slli        t4, t6, 3
	add         t2, t2, t4
	ld          t4, 0(t2)
	addi        t2, t4, 40
	ld          t2, 8(t2)
	bge         a1, t2, .SourceLocationNumbers_label_87

	// *** Basic block 6

	ld          t2, 40(t4)
	slli        t4, a1, 3
	add         t2, t2, t4
	ld          t2, 0(t2)
	sw          t2, 0(t1)

	// *** Basic block 7

.SourceLocationNumbers_label_87:
	sw          t6, 0(t0)

	// *** Basic block 8

.SourceLocationNumbers_label_89:
	j           .SourceLocationNumbers_label_44
.func_end_SourceLocationNumbers:
	.size SourceLocationNumbers, .func_end_SourceLocationNumbers-SourceLocationNumbers

	.global NewSourceFromFile
	.type NewSourceFromFile, @function

NewSourceFromFile:

	// *** Basic block 0

	.global malloc
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
	mv          s1, a0
	mv          s2, a1
	li          a0, 128		// 0x80 ASCII \x80
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	mv          a1, s1
	mv          a0, s3
	call        StringInit

	// *** Basic block 2

	addi        a0, s3, 40
	mv          a1, s1
	call        StringInit

	// *** Basic block 3

	sw          x0, 80(s3)
	sd          s2, 104(s3)
	sw          x0, 96(s3)
	li          t0, -1		// 0xffffffffffffffff
	sw          t0, 84(s3)
	sd          x0, 88(s3)
	sd          x0, 120(s3)
	mv          a0, s3

	// *** Basic block 4

.NewSourceFromFile_label_52:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewSourceFromFile:
	.size NewSourceFromFile, .func_end_NewSourceFromFile-NewSourceFromFile

	.global NewSourceFromString
	.type NewSourceFromString, @function

NewSourceFromString:

	// *** Basic block 0

	.global malloc
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
	mv          s1, a0
	mv          s2, a1
	li          a0, 128		// 0x80 ASCII \x80
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	mv          a1, s1
	mv          a0, s3
	call        StringInit

	// *** Basic block 2

	addi        a0, s3, 40
	mv          a1, s1
	call        StringInit

	// *** Basic block 3

	sw          x0, 80(s3)
	sd          s2, 104(s3)
	addi        t0, s3, 104
	sd          x0, 8(t0)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 96(s3)
	li          t0, -1		// 0xffffffffffffffff
	sw          t0, 84(s3)
	sd          x0, 88(s3)
	sd          x0, 120(s3)
	mv          a0, s3

	// *** Basic block 4

.NewSourceFromString_label_58:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewSourceFromString:
	.size NewSourceFromString, .func_end_NewSourceFromString-NewSourceFromString

	.global SourceDestruct
	.type SourceDestruct, @function

SourceDestruct:

	// *** Basic block 0

	.global fclose
	.global StringDestruct
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
	lw          t0, 96(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .SourceDestruct_label_24

	// *** Basic block 2

	j           .SourceDestruct_label_30

	// *** Basic block 3

.SourceDestruct_label_24:
	ld          a0, 104(s1)
	call        fclose

	// *** Basic block 4

	j           .SourceDestruct_label_40

	// *** Basic block 5

.SourceDestruct_label_30:
	ld          s2, 104(s1)
	mv          a0, s2
	call        StringDestruct

	// *** Basic block 6

	mv          a0, s2
	call        free

	// *** Basic block 7

	j           .SourceDestruct_label_40

	// *** Basic block 8

.SourceDestruct_label_40:
	mv          a0, s1
	call        StringDestruct

	// *** Basic block 9

	addi        a0, s1, 40
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           StringDestruct
.func_end_SourceDestruct:
	.size SourceDestruct, .func_end_SourceDestruct-SourceDestruct

	.global SourceDelete
	.type SourceDelete, @function

SourceDelete:

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
	.global SourceDestruct
	.global free
	mv          s1, a0
	call        SourceDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_SourceDelete:
	.size SourceDelete, .func_end_SourceDelete-SourceDelete

	.global SourceRewind
	.type SourceRewind, @function

SourceRewind:

	// *** Basic block 0

	.global rewind
	.local ResetFiles
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
	lw          t0, 96(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .SourceRewind_label_27

	// *** Basic block 2

	j           .SourceRewind_label_33

	// *** Basic block 3

.SourceRewind_label_27:
	ld          a0, 104(s1)
	call        rewind

	// *** Basic block 4

	j           .SourceRewind_label_38

	// *** Basic block 5

.SourceRewind_label_33:
	addi        t0, s1, 104
	sd          x0, 8(t0)
	j           .SourceRewind_label_38

	// *** Basic block 6

.SourceRewind_label_38:
	sw          x0, 80(s1)
	li          t0, -1		// 0xffffffffffffffff
	sw          t0, 84(s1)
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ResetFiles
.func_end_SourceRewind:
	.size SourceRewind, .func_end_SourceRewind-SourceRewind

	.global SourceEof
	.type SourceEof, @function

SourceEof:

	// *** Basic block 0

	.global feof
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
	lw          t0, 96(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .SourceEof_label_24

	// *** Basic block 2

	j           .SourceEof_label_35

	// *** Basic block 3

.SourceEof_label_24:
	ld          a0, 104(s1)
	call        feof

	// *** Basic block 4

	slli        t0, a0, 56
	srai        a0, t0, 56

	// *** Basic block 5

.SourceEof_label_32:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.SourceEof_label_35:
	addi        t0, s1, 104
	ld          t0, 8(t0)
	ld          t1, 104(s1)
	ld          t1, 24(t1)
	slt         a0, t1, t0
	j           .SourceEof_label_32
.func_end_SourceEof:
	.size SourceEof, .func_end_SourceEof-SourceEof

	.global SourceGetChar
	.type SourceGetChar, @function

SourceGetChar:

	// *** Basic block 0

	.global fgetc
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	lw          t1, 96(t0)
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 1

	j           .SourceGetChar_label_26

	// *** Basic block 2

	j           .SourceGetChar_label_37

	// *** Basic block 3

.SourceGetChar_label_26:
	ld          a0, 104(t0)
	j           fgetc

	// *** Basic block 6

.SourceGetChar_label_37:
	addi        t1, t0, 104
	ld          t1, 8(t1)
	ld          t2, 104(t0)
	ld          t2, 24(t2)
	bge         t2, t1, .SourceGetChar_label_49

	// *** Basic block 7

	li          a0, -1		// 0xffffffffffffffff
	ret         

	// *** Basic block 8

.SourceGetChar_label_49:
	addi        t1, t0, 104
	ld          t2, 104(t0)
	ld          t2, 16(t2)
	ld          t3, 8(t1)
	addi        t3, t3, 1
	sd          t3, 8(t1)
	add         t1, t2, t3
	lb          a0, 0(t1)
	ret         
.func_end_SourceGetChar:
	.size SourceGetChar, .func_end_SourceGetChar-SourceGetChar

	.global SourceReadLine
	.type SourceReadLine, @function

SourceReadLine:

	// *** Basic block 0

	.global SourceEof
	.global SourceGetChar
	.global StringAppendChar
	.global StringDestruct
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Local vars at offset -64(s0)
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
	mv          s2, a1
	call        SourceEof

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .SourceReadLine_label_251

	// *** Basic block 2

.SourceReadLine_label_48:
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	ld          s3, 24(s2)
	ld          s4, 16(s2)

	// *** Basic block 3

.SourceReadLine_label_67:
	mv          a0, s1
	call        SourceGetChar

	// *** Basic block 4

	addi        t0, s0, -64
	ld          s5, 16(t0)
	mv          s6, a0
	li          t0, -1		// 0xffffffffffffffff
	beq         s6, t0, .SourceReadLine_label_96

	// *** Basic block 5

.SourceReadLine_label_80:
	li          t0, 10		// 0xa ASCII \xa
	beq         s6, t0, .SourceReadLine_label_96

	// *** Basic block 6

.SourceReadLine_label_86:
	addi        a0, s0, -64
	slli        t0, s6, 56
	srai        a1, t0, 56
	call        StringAppendChar

	// *** Basic block 7

.SourceReadLine_label_94:
	j           .SourceReadLine_label_67

	// *** Basic block 8

.SourceReadLine_label_96:
	lw          t0, 80(s1)
	addi        t0, t0, 1
	sw          t0, 80(s1)
	mv          s7, x0
	addi        t0, s0, -64
	ld          s8, 24(t0)
	bge         x0, s8, .SourceReadLine_label_225

	// *** Basic block 9

	addi        s1, s8, -2

	// *** Basic block 10

.SourceReadLine_label_110:
	add         t0, s5, s7
	lb          s9, 0(t0)
	addi        t1, s9, -63
	seqz        t0, t1
	li          t1, 63		// 0x3f ASCII '?'
	bne         s9, t1, .SourceReadLine_label_122

	// *** Basic block 11

	slt         t0, s7, s1

	// *** Basic block 12

.SourceReadLine_label_122:
	beqz        t0, .SourceReadLine_label_129

	// *** Basic block 13

	addi        t1, s7, 1
	add         t1, s5, t1
	lb          t1, 0(t1)
	addi        t1, t1, -63
	seqz        t0, t1

	// *** Basic block 14

.SourceReadLine_label_129:
	beqz        t0, .SourceReadLine_label_215

	// *** Basic block 15

	addi        s7, s7, 2
	add         t0, s5, s7
	lb          t0, 0(t0)
	li          t1, 33		// 0x21 ASCII '!'
	beq         t0, t1, .SourceReadLine_label_198

	// *** Basic block 16

	li          t1, 39		// 0x27 ASCII '''
	beq         t0, t1, .SourceReadLine_label_186

	// *** Basic block 17

	li          t1, 40		// 0x28 ASCII '('
	beq         t0, t1, .SourceReadLine_label_190

	// *** Basic block 18

	li          t1, 41		// 0x29 ASCII ')'
	beq         t0, t1, .SourceReadLine_label_194

	// *** Basic block 19

	li          t1, 45		// 0x2d ASCII '-'
	beq         t0, t1, .SourceReadLine_label_210

	// *** Basic block 20

	li          t1, 47		// 0x2f ASCII '/'
	beq         t0, t1, .SourceReadLine_label_182

	// *** Basic block 21

	li          t1, 60		// 0x3c ASCII '<'
	beq         t0, t1, .SourceReadLine_label_202

	// *** Basic block 22

	li          t1, 61		// 0x3d ASCII '='
	beq         t0, t1, .SourceReadLine_label_179

	// *** Basic block 23

	li          t1, 62		// 0x3e ASCII '>'
	beq         t0, t1, .SourceReadLine_label_206

	// *** Basic block 24

	j           .SourceReadLine_label_214

	// *** Basic block 25

.SourceReadLine_label_179:
	li          s9, 35		// 0x23 ASCII '#'
	j           .SourceReadLine_label_214

	// *** Basic block 26

.SourceReadLine_label_182:
	li          s9, 92		// 0x5c ASCII '\'
	j           .SourceReadLine_label_214

	// *** Basic block 27

.SourceReadLine_label_186:
	li          s9, 94		// 0x5e ASCII '^'
	j           .SourceReadLine_label_214

	// *** Basic block 28

.SourceReadLine_label_190:
	li          s9, 91		// 0x5b ASCII '['
	j           .SourceReadLine_label_214

	// *** Basic block 29

.SourceReadLine_label_194:
	li          s9, 93		// 0x5d ASCII ']'
	j           .SourceReadLine_label_214

	// *** Basic block 30

.SourceReadLine_label_198:
	li          s9, 124		// 0x7c ASCII '|'
	j           .SourceReadLine_label_214

	// *** Basic block 31

.SourceReadLine_label_202:
	li          s9, 123		// 0x7b ASCII '{'
	j           .SourceReadLine_label_214

	// *** Basic block 32

.SourceReadLine_label_206:
	li          s9, 125		// 0x7d ASCII '}'
	j           .SourceReadLine_label_214

	// *** Basic block 33

.SourceReadLine_label_210:
	li          s9, 126		// 0x7e ASCII '~'
	j           .SourceReadLine_label_214

	// *** Basic block 34

.SourceReadLine_label_214:

	// *** Basic block 35

.SourceReadLine_label_215:
	mv          a1, s9
	mv          a0, s2
	call        StringAppendChar

	// *** Basic block 36

.SourceReadLine_label_221:
	addi        s7, s7, 1
	bge         s7, s8, .SourceReadLine_label_110

	// *** Basic block 37

.SourceReadLine_label_225:
	bge         x0, s3, .SourceReadLine_label_246

	// *** Basic block 38

	addi        s1, s3, -1
	add         t0, s4, s1
	lb          t0, 0(t0)
	li          t1, 92		// 0x5c ASCII '\'
	bne         t0, t1, .SourceReadLine_label_245

	// *** Basic block 39

	ld          t0, 16(s2)
	add         t0, t0, s1
	li          t1, 32		// 0x20 ASCII ' '
	sb          t1, 0(t0)
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 40

	j           .SourceReadLine_label_48

	// *** Basic block 41

.SourceReadLine_label_245:

	// *** Basic block 42

.SourceReadLine_label_246:
	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 43

	j           .SourceReadLine_label_251

	// *** Basic block 44

.SourceReadLine_label_251:
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
.func_end_SourceReadLine:
	.size SourceReadLine, .func_end_SourceReadLine-SourceReadLine

	.global SourcePrintLocation
	.type SourcePrintLocation, @function

SourcePrintLocation:

	// *** Basic block 0

	.global DecodeSourceLocation
	.global printf
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -48(s0)
	// End of stack frame
	addi        a1, s0, -48
	addi        a2, s0, -40
	addi        a3, s0, -36
	addi        a4, s0, -32
	call        DecodeSourceLocation

	// *** Basic block 1

	lla         a0, .str.4
	ld          a1, -48(s0)
	lw          a2, -40(s0)
	call        printf

	// *** Basic block 2

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SourcePrintLocation:
	.size SourcePrintLocation, .func_end_SourcePrintLocation-SourcePrintLocation

	.global SourceTraverseFiles
	.type SourceTraverseFiles, @function

SourceTraverseFiles:

	// *** Basic block 0

	.local all_files
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
	mv          s3, x0
	la          t0, all_files
	ld          s4, 8(t0)
	bge         x0, s4, .SourceTraverseFiles_label_40

	// *** Basic block 1

	la          t0, all_files
	ld          s5, 0(t0)

	// *** Basic block 2

.SourceTraverseFiles_label_24:
	sext.w      a0, s3
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          a1, 0(t0)
	mv          a2, s2
	jalr         x1, s1, 0

	// *** Basic block 3

.SourceTraverseFiles_label_36:
	addi        s3, s3, 1
	bge         s3, s4, .SourceTraverseFiles_label_24

	// *** Basic block 4

.SourceTraverseFiles_label_40:
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
.func_end_SourceTraverseFiles:
	.size SourceTraverseFiles, .func_end_SourceTraverseFiles-SourceTraverseFiles

.PCend:
	.data
file_map_initialized:
	.type   file_map_initialized,@object
	.local  file_map_initialized
	.size   file_map_initialized,1
	.p2align  0
	.byte   0

	.type   all_files,@object
	.local  all_files
	.comm   all_files,24,8

	.type   file_map,@object
	.local  file_map
	.comm   file_map,32,8

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "command-line"
	.type .str.1, @object
	.size .str.1, 13

.str.2:
	.asciz "<unknown>"
	.type .str.2, @object
	.size .str.2, 10

.str.3:
	.asciz "<unknown>"
	.type .str.3, @object
	.size .str.3, 10

.str.4:
	.asciz "%s:%d\n"
	.type .str.4, @object
	.size .str.4, 7

