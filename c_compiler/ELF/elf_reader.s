	.file   "elf_reader.c"
	.text
	.option pic
.PCbegin:
	.global NewELFReaderSection
	.type NewELFReaderSection, @function

NewELFReaderSection:
	.global malloc
	.global StringInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s2, 8(sp)
	// End of stack frame
	li          a0, 80
	call    malloc
	mv      s2, a0
	addi        a0, s2, 8
	lla         a1, .str.1
	call    StringInit
	sd          x0, 48(s2)
	sd          x0, 56(s2)
	sd          x0, 64(s2)
	sw          x0, 72(s2)
	mv      a0, s2
.NewELFReaderSection_label_44:
	ld s2, 8(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_NewELFReaderSection:
	.size NewELFReaderSection, .func_end_NewELFReaderSection-NewELFReaderSection

	.global ELFReaderSectionDelete
	.type ELFReaderSectionDelete, @function

ELFReaderSectionDelete:
	.global StringDestruct
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s2, 8(sp)
	// Register variable loads.
	mv   s2, a0		// section
	// End of stack frame
	addi        a0, s2, 8
	call    StringDestruct
	ld s2, 8(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFReaderSectionDelete:
	.size ELFReaderSectionDelete, .func_end_ELFReaderSectionDelete-ELFReaderSectionDelete

	.global ELFReaderFileInit
	.type ELFReaderFileInit, @function

ELFReaderFileInit:
	.global StringInit
	.global memset
	.global VectorInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s2, 8(sp)
	sd s3, 0(sp)
	// Register variable loads.
	mv   s2, a0		// elf
	mv   s3, a1		// filename
	// End of stack frame
	addi        a0, s2, 8
	ld          a1, 16(s3)
	call    StringInit
	addi        a0, s2, 0
	li          a2, 8
	mv      a1, x0
	call    memset
	addi        a0, s2, 56
	call    VectorInit
	addi        a0, s2, 80
	call    VectorInit
	sd          x0, 104(s2)
	ld s2, 8(sp)
	ld s3, 0(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFReaderFileInit:
	.size ELFReaderFileInit, .func_end_ELFReaderFileInit-ELFReaderFileInit

	.global NewELFReaderFile
	.type NewELFReaderFile, @function

NewELFReaderFile:
	.global malloc
	.global ELFReaderFileInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s2, 8(sp)
	sd s3, 0(sp)
	// Register variable loads.
	mv   s3, a0		// filename
	// End of stack frame
	li          a0, 240
	call    malloc
	mv      s2, a0
	mv      a1, s3
	mv      a0, s2
	call    ELFReaderFileInit
	mv      a0, s2
.NewELFReaderFile_label_21:
	ld s2, 8(sp)
	ld s3, 0(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_NewELFReaderFile:
	.size NewELFReaderFile, .func_end_NewELFReaderFile-NewELFReaderFile

	.global ReadFileContents
	.type ReadFileContents, @function

ReadFileContents:
	.global NewELFReaderSection
	.global VectorAppend
	.global StringSet
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Saved argument registers.
	sd a1, -24(s0)
	sd a2, -32(s0)
	// Local vars at offset -80(s0)
	// Saved integer registers.
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	sd s5, 0(sp)
	// Register variable loads.
	mv   s2, a0		// elf
	// End of stack frame
	ld          t0, -24(s0)
	sd          t0, 0(s2)
	ld          t0, -32(s0)
	sd          t0, 48(s2)
	ld          s3, -24(s0)
	add         t3, s3, x0
	lb          t3, 0(t3)
	addi        t4, t3, -127
	snez        t2, t4
	li          t4, 127
	bne         t3, t4, .ReadFileContents_label_59
	li          t3, 1
	add         t3, s3, t3
	lb          t3, 0(t3)
	addi        t3, t3, -69
	snez        t2, t3
.ReadFileContents_label_59:
	mv      t1, t2
	bnez        t2, .ReadFileContents_label_70
	li          t2, 1
	slli        t2, t2, 1
	add         t2, s3, t2
	lb          t2, 0(t2)
	addi        t2, t2, -76
	snez        t1, t2
.ReadFileContents_label_70:
	mv      t0, t1
	bnez        t1, .ReadFileContents_label_82
	li          t2, 3
	li          t3, 1
	mul         t2, t2, t3
	add         t2, s3, t2
	lb          t2, 0(t2)
	addi        t2, t2, -70
	snez        t0, t2
.ReadFileContents_label_82:
	beqz        t0, .ReadFileContents_label_90
	mv      a0, x0
.ReadFileContents_label_87:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 104(sp)
	ld s0, 96(sp)
	addi sp, sp, 112
	ret         
.ReadFileContents_label_90:
	ld          t2, 0(s2)
	ld          t2, 40(t2)
	add         t2, s3, t2
	sd          t2, -80(s0)
.ReadFileContents_label_99:
	lw          t2, -56(s0)
	ld          t3, 0(s2)
	lhu         t3, 60(t3)
	bge         t2, t3, .ReadFileContents_label_137
	call    NewELFReaderSection
	sd          a0, -64(s0)
	ld          t2, -64(s0)
	ld          t3, -80(s0)
	sd          t3, 0(t2)
	addi        a0, s2, 56
	ld          a1, -64(s0)
	call    VectorAppend
	li          t2, 64
	li          t3, 1
	mul         t2, t2, t3
	ld          t3, -80(s0)
	add         t2, t3, t2
	sd          t2, -80(s0)
.ReadFileContents_label_130:
	lw          t2, -56(s0)
	addi        t2, t2, 1
	sw          t2, -56(s0)
	j           .ReadFileContents_label_99
.ReadFileContents_label_137:
	addi        t2, s2, 56
	ld          t2, 0(t2)
	ld          t3, 0(s2)
	lhu         t3, 62(t3)
	slli        t3, t3, 3
	add         t2, t2, t3
	ld          t2, 0(t2)
	sd          t2, -40(s0)
	ld          t2, -40(s0)
	ld          t2, 0(t2)
	ld          t2, 24(t2)
	add         t2, s3, t2
	sd          t2, 104(s2)
.ReadFileContents_label_160:
	ld          t2, 64(s2)
	bge         s5, t2, .ReadFileContents_label_204
	addi        t2, s2, 56
	ld          t2, 0(t2)
	slli        t3, s5, 3
	add         t2, t2, t3
	ld          s4, 0(t2)
	addi        a0, s4, 8
	ld          t3, 104(s2)
	ld          t4, 0(s4)
	lwu         t4, 0(t4)
	add         a1, t3, t4
	call    StringSet
	ld          t2, 0(s4)
	ld          t2, 24(t2)
	add         t2, s3, t2
	sd          t2, 48(s4)
.ReadFileContents_label_198:
	addi        s5, s5, 1
	j           .ReadFileContents_label_160
.ReadFileContents_label_204:
	ld          t2, 0(s2)
	ld          t2, 32(t2)
	add         t2, s3, t2
	sd          t2, -72(s0)
.ReadFileContents_label_213:
	lw          t2, -52(s0)
	ld          t3, 0(s2)
	lhu         t3, 56(t3)
	bge         t2, t3, .ReadFileContents_label_245
	ld          t2, -72(s0)
	sd          t2, -48(s0)
	addi        a0, s2, 80
	ld          a1, -48(s0)
	call    VectorAppend
	li          t2, 56
	li          t3, 1
	mul         t2, t2, t3
	ld          t3, -72(s0)
	add         t2, t3, t2
	sd          t2, -72(s0)
.ReadFileContents_label_238:
	lw          t2, -52(s0)
	addi        t2, t2, 1
	sw          t2, -52(s0)
	j           .ReadFileContents_label_213
.ReadFileContents_label_245:
	li          a0, 1
	j           .ReadFileContents_label_87
.func_end_ReadFileContents:
	.size ReadFileContents, .func_end_ReadFileContents-ReadFileContents

	.global ELFReaderFileRead
	.type ELFReaderFileRead, @function

ELFReaderFileRead:
	.global stat
	.global open
	.global sysconf
	.global mmap
	.global close
	.global ReadFileContents
	.global munmap
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Saved argument registers.
	sd a2, -24(s0)
	// Local vars at offset -80(s0)
	// Saved integer registers.
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	sd s5, 0(sp)
	// Register variable loads.
	mv   s2, a1		// length
	mv   s3, a0		// elf
	// End of stack frame
	bnez        s2, .ELFReaderFileRead_label_53
	ld          a0, 24(s3)
	addi        a1, s3, 112
	call    stat
	sw          a0, -64(s0)
	lw          t0, -64(s0)
	beqz        t0, .ELFReaderFileRead_label_48
	mv      a0, x0
.ELFReaderFileRead_label_45:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 104(sp)
	ld s0, 96(sp)
	addi sp, sp, 112
	ret         
.ELFReaderFileRead_label_48:
	ld          s2, 160(s3)
.ELFReaderFileRead_label_53:
	ld          a0, 24(s3)
	mv      a1, x0
	call    open
	mv      s4, a0
	bge         s4, x0, .ELFReaderFileRead_label_72
	mv      a0, x0
	j           .ELFReaderFileRead_label_45
.ELFReaderFileRead_label_72:
	li          a0, 30
	call    sysconf
	sw          a0, -48(s0)
	lw          t0, -48(s0)
	addi        t0, t0, -1
	sw          t0, -76(s0)
	ld          t0, -24(s0)
	lw          t1, -76(s0)
	not         t1, t1
	and         t0, t0, t1
	sd          t0, -32(s0)
	ld          t0, -24(s0)
	lw          t1, -76(s0)
	and         t0, t0, t1
	sd          t0, -72(s0)
	ld          t0, -72(s0)
	add         t0, s2, t0
	sd          t0, -56(s0)
	ld          a1, -56(s0)
	ld          a5, -32(s0)
	mv      a4, s4
	li          a3, 2
	li          a2, 3
	mv      a0, x0
	call    mmap
	mv      s5, a0
	li          t0, -1
	bne         s5, t0, .ELFReaderFileRead_label_134
	mv      a0, s4
	call    close
	mv      a0, x0
	j           .ELFReaderFileRead_label_45
.ELFReaderFileRead_label_134:
	ld          t0, -72(s0)
	add         t0, s5, t0
	sd          t0, -40(s0)
	ld          a1, -40(s0)
	mv      a2, s2
	mv      a0, s3
	call    ReadFileContents
	sb          a0, -80(s0)
	lb          t0, -80(s0)
	not         t0, t0
	beqz        t0, .ELFReaderFileRead_label_158
	mv      a1, s2
	mv      a0, s5
	call    munmap
.ELFReaderFileRead_label_158:
	mv      a0, s4
	call    close
	lb          a0, -80(s0)
	j           .ELFReaderFileRead_label_45
.func_end_ELFReaderFileRead:
	.size ELFReaderFileRead, .func_end_ELFReaderFileRead-ELFReaderFileRead

	.global ELFReaderFileDestruct
	.type ELFReaderFileDestruct, @function

ELFReaderFileDestruct:
	.global VectorDestruct
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s2, 8(sp)
	// Register variable loads.
	mv   s2, a0		// elf
	// End of stack frame
	addi        a0, s2, 56
	call    VectorDestruct
	addi        a0, s2, 80
	call    VectorDestruct
	ld s2, 8(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFReaderFileDestruct:
	.size ELFReaderFileDestruct, .func_end_ELFReaderFileDestruct-ELFReaderFileDestruct

	.global ELFReaderFileDelete
	.type ELFReaderFileDelete, @function

ELFReaderFileDelete:
	.global ELFReaderFileDestruct
	.global free
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s2, 8(sp)
	// Register variable loads.
	mv   s2, a0		// elf
	// End of stack frame
	mv      a0, s2
	call    ELFReaderFileDestruct
	mv      a0, s2
	call    free
	ld s2, 8(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFReaderFileDelete:
	.size ELFReaderFileDelete, .func_end_ELFReaderFileDelete-ELFReaderFileDelete

	.global ELFReaderFileFindSection
	.type ELFReaderFileFindSection, @function

ELFReaderFileFindSection:
	.global StringEqual
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	sd s5, 0(sp)
	// Register variable loads.
	mv   s4, a0		// elf
	mv   s5, a1		// name
	// End of stack frame
.ELFReaderFileFindSection_label_7:
	ld          t0, 64(s4)
	bge         s2, t0, .ELFReaderFileFindSection_label_47
	addi        t0, s4, 56
	ld          t0, 0(t0)
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	addi        a0, s3, 8
	mv      a1, s5
	call    StringEqual
	beqz        a0, .ELFReaderFileFindSection_label_40
	mv      a0, s3
.ELFReaderFileFindSection_label_37:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.ELFReaderFileFindSection_label_40:
.ELFReaderFileFindSection_label_41:
	addi        s2, s2, 1
	j           .ELFReaderFileFindSection_label_7
.ELFReaderFileFindSection_label_47:
	mv      a0, x0
	j           .ELFReaderFileFindSection_label_37
.func_end_ELFReaderFileFindSection:
	.size ELFReaderFileFindSection, .func_end_ELFReaderFileFindSection-ELFReaderFileFindSection

	.global ELFReaderFileFindSectionsByType
	.type ELFReaderFileFindSectionsByType, @function

ELFReaderFileFindSectionsByType:
	.global VectorAppend
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Saved argument registers.
	sd a2, -24(s0)
	// Local vars at offset -24(s0)
	// Saved integer registers.
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// Register variable loads.
	mv   s4, a0		// elf
	mv   s5, a1		// type
	// End of stack frame
.ELFReaderFileFindSectionsByType_label_8:
	ld          t0, 64(s4)
	bge         s2, t0, .ELFReaderFileFindSectionsByType_label_51
	addi        t0, s4, 56
	ld          t0, 0(t0)
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	ld          t0, 0(s3)
	lwu         t0, 4(t0)
	bne         t0, s5, .ELFReaderFileFindSectionsByType_label_44
	ld          a0, -24(s0)
	mv      a1, s3
	call    VectorAppend
.ELFReaderFileFindSectionsByType_label_44:
.ELFReaderFileFindSectionsByType_label_45:
	addi        s2, s2, 1
	j           .ELFReaderFileFindSectionsByType_label_8
.ELFReaderFileFindSectionsByType_label_51:
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld ra, 56(sp)
	ld s0, 48(sp)
	addi sp, sp, 64
	ret         
.func_end_ELFReaderFileFindSectionsByType:
	.size ELFReaderFileFindSectionsByType, .func_end_ELFReaderFileFindSectionsByType-ELFReaderFileFindSectionsByType

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "(null)"
	.type .str.1, @object
	.size .str.1, 1

