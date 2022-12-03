	.file   "elf_writer.c"
	.text
	.option pic
.PCbegin:
	.global ELFWriterFileInit
	.type ELFWriterFileInit, @function

ELFWriterFileInit:
	.global memset
	.global VectorInit
	.global BufferInit
	.global calloc
	.global VectorAppend
	.global BufferAppend
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Saved argument registers.
	sd a2, -24(s0)
	sd a3, -32(s0)
	sd a4, -40(s0)
	sd a5, -48(s0)
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	sd s5, 0(sp)
	// Register variable loads.
	mv   s2, a0		// elf
	mv   s4, a6		// is_little_endian
	mv   s5, a1		// type
	// End of stack frame
	addi        a0, s2, 0
	li          a2, 64
	mv      a1, x0
	call    memset
	addi        t0, s2, 0
	addi        t0, t0, 0
	add         t0, t0, x0
	li          t1, 127
	sb          t1, 0(t0)
	addi        t0, s2, 0
	addi        t0, t0, 0
	li          t1, 1
	add         t0, t0, t1
	li          t1, 69
	sb          t1, 0(t0)
	addi        t0, s2, 0
	addi        t0, t0, 0
	li          t1, 1
	slli        t1, t1, 1
	add         t0, t0, t1
	li          t1, 76
	sb          t1, 0(t0)
	addi        t0, s2, 0
	addi        t0, t0, 0
	li          t1, 3
	li          t2, 1
	mul         t1, t1, t2
	add         t0, t0, t1
	li          t1, 70
	sb          t1, 0(t0)
	addi        t0, s2, 0
	addi        t0, t0, 0
	li          t1, 1
	slli        t1, t1, 2
	add         t0, t0, t1
	lb          t2, -48(s0)
	bnez        t2, .ELFWriterFileInit_label_102
	li          t1, 2
	j           .ELFWriterFileInit_label_105
.ELFWriterFileInit_label_102:
	li          t1, 1
.ELFWriterFileInit_label_105:
	andi        t1, t1, 255
	sb          t1, 0(t0)
	addi        t0, s2, 0
	addi        t0, t0, 0
	li          t1, 5
	li          t2, 1
	mul         t1, t1, t2
	add         t0, t0, t1
	bnez        s4, .ELFWriterFileInit_label_122
	li          t1, 1
	j           .ELFWriterFileInit_label_125
.ELFWriterFileInit_label_122:
	li          t1, 2
.ELFWriterFileInit_label_125:
	andi        t1, t1, 255
	sb          t1, 0(t0)
	addi        t0, s2, 0
	addi        t0, t0, 0
	li          t1, 6
	li          t2, 1
	mul         t1, t1, t2
	add         t0, t0, t1
	li          t1, 1
	sb          t1, 0(t0)
	addi        t0, s2, 0
	addi        t0, t0, 0
	li          t1, 7
	li          t2, 1
	mul         t1, t1, t2
	add         t0, t0, t1
	sb          x0, 0(t0)
	addi        t0, s2, 0
	lw          t1, -32(s0)
	sw          t1, 48(t0)
	addi        t0, s2, 0
	li          t1, 1
	sw          t1, 20(t0)
	addi        t0, s2, 0
	li          t1, 64
	sd          t1, 32(t0)
	addi        a0, s2, 64
	call    VectorInit
	addi        a0, s2, 88
	call    VectorInit
	addi        a0, s2, 112
	call    BufferInit
	addi        a0, s2, 136
	call    VectorInit
	addi        a0, s2, 208
	call    BufferInit
	addi        a0, s2, 240
	call    VectorInit
	li          t0, -1
	sw          t0, 232(s2)
	li          a1, 1
	li          a0, 24
	call    calloc
	mv      s3, a0
	addi        a0, s2, 136
	mv      a1, s3
	call    VectorAppend
	addi        a0, s2, 112
	lla         a1, .str.1
	li          a2, 1
	call    BufferAppend
	addi        a0, s2, 208
	lla         a1, .str.2
	li          a2, 1
	call    BufferAppend
	ld          t0, -40(s0)
	sd          t0, 264(s2)
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 72(sp)
	ld s0, 64(sp)
	addi sp, sp, 80
	ret         
.func_end_ELFWriterFileInit:
	.size ELFWriterFileInit, .func_end_ELFWriterFileInit-ELFWriterFileInit

	.global ELFWriterFileDestruct
	.type ELFWriterFileDestruct, @function

ELFWriterFileDestruct:
	.global VectorDestructWithContents
	.global ELFWriterSectionDestruct
	.global ELFWriterSegmentDestruct
	.global BufferDestruct
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
	addi        a0, s2, 64
	la          t1, ELFWriterSectionDestruct
	ld          a1, 0(t1)
	call    VectorDestructWithContents
	addi        a0, s2, 88
	la          t1, ELFWriterSegmentDestruct
	ld          a1, 0(t1)
	call    VectorDestructWithContents
	addi        a0, s2, 112
	call    BufferDestruct
	addi        a0, s2, 136
	mv      a1, x0
	call    VectorDestructWithContents
	addi        a0, s2, 208
	call    BufferDestruct
	addi        a0, s2, 240
	mv      a1, x0
	call    VectorDestructWithContents
	ld s2, 8(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFWriterFileDestruct:
	.size ELFWriterFileDestruct, .func_end_ELFWriterFileDestruct-ELFWriterFileDestruct

	.global ELFWriterAddSectionFixupByName
	.type ELFWriterAddSectionFixupByName, @function

ELFWriterAddSectionFixupByName:
	.global malloc
	.global VectorAppend
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Saved argument registers.
	sd a0, -24(s0)
	// Local vars at offset -24(s0)
	// Saved integer registers.
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// Register variable loads.
	mv   s3, a1		// field
	mv   s4, a2		// from
	mv   s5, a3		// to
	// End of stack frame
	li          a0, 24
	call    malloc
	mv      s2, a0
	sw          s3, 0(s2)
	sd          s4, 8(s2)
	sd          s5, 16(s2)
	ld          t0, -24(s0)
	addi        a0, t0, 240
	mv      a1, s2
	call    VectorAppend
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld ra, 56(sp)
	ld s0, 48(sp)
	addi sp, sp, 64
	ret         
.func_end_ELFWriterAddSectionFixupByName:
	.size ELFWriterAddSectionFixupByName, .func_end_ELFWriterAddSectionFixupByName-ELFWriterAddSectionFixupByName

	.global ELFWriterAddSectionFixup
	.type ELFWriterAddSectionFixup, @function

ELFWriterAddSectionFixup:
	.global ELFWriterAddSectionFixupByName
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
	mv   s2, a0		// elf
	mv   s3, a1		// field
	mv   s4, a2		// from
	mv   s5, a3		// to
	// End of stack frame
	ld          a2, 16(s4)
	ld          a3, 16(s5)
	mv      a1, s3
	mv      a0, s2
	call    ELFWriterAddSectionFixupByName
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.func_end_ELFWriterAddSectionFixup:
	.size ELFWriterAddSectionFixup, .func_end_ELFWriterAddSectionFixup-ELFWriterAddSectionFixup

	.global ELFWriterFixupSections
	.type ELFWriterFixupSections, @function

ELFWriterFixupSections:
	.global ELFWriterFindSection
	.global printf
	.global abort
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -24(s0)
	// Saved integer registers.
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// Register variable loads.
	mv   s3, a0		// elf
	// End of stack frame
.ELFWriterFixupSections_label_15:
	ld          t0, 248(s3)
	bge         s2, t0, .ELFWriterFixupSections_label_115
	addi        t0, s3, 240
	ld          t0, 0(t0)
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	ld          a1, 8(s4)
	mv      a0, s3
	call    ELFWriterFindSection
	mv      s5, a0
	ld          a1, 16(s4)
	mv      a0, s3
	call    ELFWriterFindSection
	sd          a0, -24(s0)
.ELFWriterFixupSections_label_53:
	snez        t0, s5
	beqz        s5, .ELFWriterFixupSections_label_61
	ld          t1, -24(s0)
	snez        t0, t1
.ELFWriterFixupSections_label_61:
	not         t0, t0
	beqz        t0, .ELFWriterFixupSections_label_70
	lla         a0, .str.3
	call    printf
	call    abort
.ELFWriterFixupSections_label_70:
.ELFWriterFixupSections_label_71:
.ELFWriterFixupSections_label_72:
	lw          t0, 0(s4)
	blt         t0, x0, .ELFWriterFixupSections_label_108
	li          t1, 1
	blt         t1, t0, .ELFWriterFixupSections_label_108
	addi        t0, t0, 0
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12
	j           .ELFWriterFixupSections_label_92
	j           .ELFWriterFixupSections_label_100
.ELFWriterFixupSections_label_92:
	addi        t0, s5, 40
	ld          t1, -24(s0)
	lw          t1, 112(t1)
	sw          t1, 44(t0)
	j           .ELFWriterFixupSections_label_108
.ELFWriterFixupSections_label_100:
	addi        t0, s5, 40
	ld          t1, -24(s0)
	lw          t1, 112(t1)
	sw          t1, 40(t0)
	j           .ELFWriterFixupSections_label_108
.ELFWriterFixupSections_label_108:
.ELFWriterFixupSections_label_109:
	addi        s2, s2, 1
	j           .ELFWriterFixupSections_label_15
.ELFWriterFixupSections_label_115:
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld ra, 56(sp)
	ld s0, 48(sp)
	addi sp, sp, 64
	ret         
.func_end_ELFWriterFixupSections:
	.size ELFWriterFixupSections, .func_end_ELFWriterFixupSections-ELFWriterFixupSections

	.global ELFWriterSectionContentsInit
	.type ELFWriterSectionContentsInit, @function

ELFWriterSectionContentsInit:
	.global BufferInit
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
	mv   s2, a0		// contents
	mv   s3, a1		// location
	// End of stack frame
	sw          s3, 0(s2)
	addi        t0, s2, 8
	addi        a0, t0, 0
	call    BufferInit
	sd          x0, 32(s2)
	ld s2, 8(sp)
	ld s3, 0(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFWriterSectionContentsInit:
	.size ELFWriterSectionContentsInit, .func_end_ELFWriterSectionContentsInit-ELFWriterSectionContentsInit

	.global NewELFWriterSectionContents
	.type NewELFWriterSectionContents, @function

NewELFWriterSectionContents:
	.global malloc
	.global ELFWriterSectionContentsInit
	.global VectorInit
	.global BufferInit
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
	mv   s3, a0		// location
	// End of stack frame
	li          a0, 40
	call    malloc
	mv      s2, a0
	mv      a1, s3
	mv      a0, s2
	call    ELFWriterSectionContentsInit
	blt         s3, x0, .NewELFWriterSectionContents_label_65
	li          t0, 3
	blt         t0, s3, .NewELFWriterSectionContents_label_65
	addi        t0, s3, 0
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12
	j           .NewELFWriterSectionContents_label_56
	j           .NewELFWriterSectionContents_label_43
	j           .NewELFWriterSectionContents_label_49
	j           .NewELFWriterSectionContents_label_63
.NewELFWriterSectionContents_label_43:
	addi        t0, s2, 8
	sd          x0, 0(t0)
	j           .NewELFWriterSectionContents_label_65
.NewELFWriterSectionContents_label_49:
	addi        t0, s2, 8
	addi        a0, t0, 0
	call    VectorInit
	j           .NewELFWriterSectionContents_label_65
.NewELFWriterSectionContents_label_56:
	addi        t0, s2, 8
	addi        a0, t0, 0
	call    BufferInit
	j           .NewELFWriterSectionContents_label_65
.NewELFWriterSectionContents_label_63:
	j           .NewELFWriterSectionContents_label_65
.NewELFWriterSectionContents_label_65:
	mv      a0, s2
.NewELFWriterSectionContents_label_68:
	ld s2, 8(sp)
	ld s3, 0(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_NewELFWriterSectionContents:
	.size NewELFWriterSectionContents, .func_end_NewELFWriterSectionContents-NewELFWriterSectionContents

	.global ELFWriterSectionContentsDestruct
	.type ELFWriterSectionContentsDestruct, @function

ELFWriterSectionContentsDestruct:
	.global VectorDestruct
	.global BufferDestruct
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s2, 8(sp)
	// Register variable loads.
	mv   s2, a0		// contents
	// End of stack frame
	lw          t0, 0(s2)
	blt         t0, x0, .ELFWriterSectionContentsDestruct_label_48
	li          t1, 3
	blt         t1, t0, .ELFWriterSectionContentsDestruct_label_48
	addi        t0, t0, 0
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12
	j           .ELFWriterSectionContentsDestruct_label_39
	j           .ELFWriterSectionContentsDestruct_label_29
	j           .ELFWriterSectionContentsDestruct_label_31
	j           .ELFWriterSectionContentsDestruct_label_46
.ELFWriterSectionContentsDestruct_label_29:
	j           .ELFWriterSectionContentsDestruct_label_48
.ELFWriterSectionContentsDestruct_label_31:
	addi        t0, s2, 8
	addi        a0, t0, 0
	call    VectorDestruct
	j           .ELFWriterSectionContentsDestruct_label_48
.ELFWriterSectionContentsDestruct_label_39:
	addi        t0, s2, 8
	addi        a0, t0, 0
	call    BufferDestruct
	j           .ELFWriterSectionContentsDestruct_label_48
.ELFWriterSectionContentsDestruct_label_46:
	j           .ELFWriterSectionContentsDestruct_label_48
.ELFWriterSectionContentsDestruct_label_48:
	ld s2, 8(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFWriterSectionContentsDestruct:
	.size ELFWriterSectionContentsDestruct, .func_end_ELFWriterSectionContentsDestruct-ELFWriterSectionContentsDestruct

	.local  AddBufferedString
	.type AddBufferedString, @function

AddBufferedString:
	.global BufferAppend
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
	// Register variable loads.
	mv   s3, a0		// buffer
	mv   s4, a1		// str
	// End of stack frame
	ld          s2, 8(s3)
	ld          a1, 16(s4)
	ld          t1, 24(s4)
	addi        a2, t1, 1
	mv      a0, s3
	call    BufferAppend
	mv      a0, s2
.AddBufferedString_label_31:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.func_end_AddBufferedString:
	.size AddBufferedString, .func_end_AddBufferedString-AddBufferedString

	.global ELFWriterSectionContentsGetLength
	.type ELFWriterSectionContentsGetLength, @function

ELFWriterSectionContentsGetLength:
	.global ELFWriterSectionContentsGetLength
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
	// Register variable loads.
	mv   s2, a0		// contents
	// End of stack frame
	lw          t0, 0(s2)
	blt         t0, x0, .ELFWriterSectionContentsGetLength_label_86
	li          t1, 3
	blt         t1, t0, .ELFWriterSectionContentsGetLength_label_86
	addi        t0, t0, 0
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12
	j           .ELFWriterSectionContentsGetLength_label_30
	j           .ELFWriterSectionContentsGetLength_label_40
	j           .ELFWriterSectionContentsGetLength_label_46
	j           .ELFWriterSectionContentsGetLength_label_80
.ELFWriterSectionContentsGetLength_label_30:
	addi        t0, s2, 8
	ld          a0, 8(t0)
.ELFWriterSectionContentsGetLength_label_37:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.ELFWriterSectionContentsGetLength_label_40:
	ld          a0, 32(s2)
	j           .ELFWriterSectionContentsGetLength_label_37
.ELFWriterSectionContentsGetLength_label_46:
.ELFWriterSectionContentsGetLength_label_47:
	addi        t0, s2, 8
	ld          t0, 8(t0)
	bge         s3, t0, .ELFWriterSectionContentsGetLength_label_76
	addi        t0, s2, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          a0, 0(t0)
	call    ELFWriterSectionContentsGetLength
	add         s4, s4, a0
.ELFWriterSectionContentsGetLength_label_70:
	addi        s3, s3, 1
	j           .ELFWriterSectionContentsGetLength_label_47
.ELFWriterSectionContentsGetLength_label_76:
	mv      a0, s4
	j           .ELFWriterSectionContentsGetLength_label_37
.ELFWriterSectionContentsGetLength_label_80:
	ld          a0, 32(s2)
	j           .ELFWriterSectionContentsGetLength_label_37
.ELFWriterSectionContentsGetLength_label_86:
	j           .ELFWriterSectionContentsGetLength_label_37
.func_end_ELFWriterSectionContentsGetLength:
	.size ELFWriterSectionContentsGetLength, .func_end_ELFWriterSectionContentsGetLength-ELFWriterSectionContentsGetLength

	.global ELFWriterSectionContentsWrite
	.type ELFWriterSectionContentsWrite, @function

ELFWriterSectionContentsWrite:
	.global fwrite
	.global ELFWriterSectionContentsWrite
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
	// Register variable loads.
	mv   s2, a0		// contents
	mv   s4, a1		// fp
	// End of stack frame
	lw          t0, 0(s2)
	blt         t0, x0, .ELFWriterSectionContentsWrite_label_105
	li          t1, 3
	blt         t1, t0, .ELFWriterSectionContentsWrite_label_105
	addi        t0, t0, 0
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12
	j           .ELFWriterSectionContentsWrite_label_31
	j           .ELFWriterSectionContentsWrite_label_56
	j           .ELFWriterSectionContentsWrite_label_73
	j           .ELFWriterSectionContentsWrite_label_103
.ELFWriterSectionContentsWrite_label_31:
	addi        t0, s2, 8
	ld          a0, 0(t0)
	addi        t1, s2, 8
	ld          a1, 8(t1)
	mv      a3, s4
	li          a2, 1
	call    fwrite
	j           .ELFWriterSectionContentsWrite_label_105
.ELFWriterSectionContentsWrite_label_56:
	ld          a0, 8(s2)
	ld          a1, 32(s2)
	mv      a3, s4
	li          a2, 1
	call    fwrite
	j           .ELFWriterSectionContentsWrite_label_105
.ELFWriterSectionContentsWrite_label_73:
.ELFWriterSectionContentsWrite_label_74:
	addi        t0, s2, 8
	ld          t0, 8(t0)
	bge         s3, t0, .ELFWriterSectionContentsWrite_label_101
	addi        t0, s2, 8
	addi        t0, t0, 0
	ld          t0, 0(t0)
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          a0, 0(t0)
	mv      a1, s4
	call    ELFWriterSectionContentsWrite
.ELFWriterSectionContentsWrite_label_95:
	addi        s3, s3, 1
	j           .ELFWriterSectionContentsWrite_label_74
.ELFWriterSectionContentsWrite_label_101:
	j           .ELFWriterSectionContentsWrite_label_105
.ELFWriterSectionContentsWrite_label_103:
	j           .ELFWriterSectionContentsWrite_label_105
.ELFWriterSectionContentsWrite_label_105:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.func_end_ELFWriterSectionContentsWrite:
	.size ELFWriterSectionContentsWrite, .func_end_ELFWriterSectionContentsWrite-ELFWriterSectionContentsWrite

	.global ELFWriterFindSection
	.type ELFWriterFindSection, @function

ELFWriterFindSection:
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
.ELFWriterFindSection_label_7:
	ld          t0, 72(s4)
	bge         s2, t0, .ELFWriterFindSection_label_47
	addi        t0, s4, 64
	ld          t0, 0(t0)
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
	addi        a0, s3, 0
	mv      a1, s5
	call    StringEqual
	beqz        a0, .ELFWriterFindSection_label_40
	mv      a0, s3
.ELFWriterFindSection_label_37:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.ELFWriterFindSection_label_40:
.ELFWriterFindSection_label_41:
	addi        s2, s2, 1
	j           .ELFWriterFindSection_label_7
.ELFWriterFindSection_label_47:
	mv      a0, x0
	j           .ELFWriterFindSection_label_37
.func_end_ELFWriterFindSection:
	.size ELFWriterFindSection, .func_end_ELFWriterFindSection-ELFWriterFindSection

	.local  CreateRelocationSections
	.type CreateRelocationSections, @function

CreateRelocationSections:
	.global StringInit
	.global StringPrintf
	.global ELFWriterAddStandardSection
	.global ELFWriterAddSectionFixup
	.global VectorAppend
	.global StringDestruct
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Saved argument registers.
	sd a2, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -80(s0)
	// Saved integer registers.
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	sd s5, 0(sp)
	// Register variable loads.
	mv   s4, a0		// elf
	// End of stack frame
	ld          t0, 72(s4)
	sd          t0, -40(s0)
.CreateRelocationSections_label_28:
	ld          t0, -40(s0)
	bge         s5, t0, .CreateRelocationSections_label_151
	addi        t0, s4, 64
	ld          t0, 0(t0)
	slli        t1, s5, 3
	add         t0, t0, t1
	ld          s2, 0(t0)
	ld          t1, 120(s2)
	snez        t0, t1
	beqz        t1, .CreateRelocationSections_label_55
	ld          t1, 120(s2)
	ld          t1, 8(t1)
	snez        t0, t1
.CreateRelocationSections_label_55:
	beqz        t0, .CreateRelocationSections_label_144
	addi        a0, s0, -80
	lla         a1, .str.4
	call    StringInit
	addi        a0, s0, -80
	lla         a1, .str.5
	ld          a2, 16(s2)
	call    StringPrintf
	ld          a1, -64(s0)
	mv      a3, x0
	li          a2, 4
	mv      a0, s4
	call    ELFWriterAddStandardSection
	mv      s3, a0
	ld          a3, -24(s0)
	mv      a2, s3
	li          a1, 1
	mv      a0, s4
	call    ELFWriterAddSectionFixup
	mv      a3, s2
	mv      a2, s3
	mv      a1, x0
	mv      a0, s4
	call    ELFWriterAddSectionFixup
	addi        t0, s3, 40
	li          t1, 24
	sd          t1, 56(t0)
	ld          t0, 120(s2)
	sd          t0, 120(s3)
	sd          x0, 120(s2)
	ld          a0, -32(s0)
	mv      a1, s3
	call    VectorAppend
	addi        a0, s0, -80
	call    StringDestruct
.CreateRelocationSections_label_144:
.CreateRelocationSections_label_145:
	addi        s5, s5, 1
	j           .CreateRelocationSections_label_28
.CreateRelocationSections_label_151:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 104(sp)
	ld s0, 96(sp)
	addi sp, sp, 112
	ret         
.func_end_CreateRelocationSections:
	.size CreateRelocationSections, .func_end_CreateRelocationSections-CreateRelocationSections

	.local  AlignTo
	.type AlignTo, @function

AlignTo:
	// Local vars at offset -16(s0)
	// Register variable loads.
	mv   t3, a1		// p2
	mv   t4, a0		// v
	// End of stack frame
	addi        t0, t3, -1
	add         t0, t4, t0
	addi        t1, t3, -1
	not         t1, t1
	and         a0, t0, t1
.AlignTo_label_13:
	ret         
.func_end_AlignTo:
	.size AlignTo, .func_end_AlignTo-AlignTo

	.local  Pad
	.type Pad, @function

Pad:
	.global memset
	.global fwrite
	addi sp, sp, -16
	sd ra, 8(sp)
	sd s0, 0(sp)
	lui t0, 1
	addi t0, t0, 48
	sub sp, sp, t0
	add s0, sp, t0
	// Local vars at offset -4120(s0)
	// Saved integer registers.
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// Register variable loads.
	mv   s2, a0		// size
	// Local variable at offset 0 from local vars base
	li t0, 4120
	add s4, s0, t0		// buf
	mv   s5, a1		// fp
	// End of stack frame
	li          a2, 4096
	li          a1, 218
	mv      a0, s4
	call    memset
.Pad_label_21:
	bge         x0, s2, .Pad_label_56
	li          t1, 4096
	bge         t1, s2, .Pad_label_36
	li          t0, 4096
	j           .Pad_label_38
.Pad_label_36:
	mv      t0, s2
.Pad_label_38:
	mv      s3, t0
	mv      a3, s5
	li          a2, 1
	mv      a1, s3
	mv      a0, s4
	call    fwrite
	sub         s2, s2, s3
	j           .Pad_label_21
.Pad_label_56:
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld ra, 4152(sp)
	ld s0, 4144(sp)
	lui t0, 1
	addi t0, t0, 64
dd sp, sp, t0
	ret         
.func_end_Pad:
	.size Pad, .func_end_Pad-Pad

	.local  WriteSectionHeaders
	.type WriteSectionHeaders, @function

WriteSectionHeaders:
	.global ELFWriterSectionContentsGetLength
	.global printf
	.global abort
	.local AlignTo
	.global fwrite
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Saved argument registers.
	sd a4, -24(s0)
	sd a6, -32(s0)
	sd a5, -40(s0)
	sd a1, -48(s0)
	sd a2, -56(s0)
	sd a3, -64(s0)
	// Local vars at offset -112(s0)
	// Saved integer registers.
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	sd s5, 0(sp)
	// Register variable loads.
	mv   s4, a0		// elf
	// End of stack frame
	ld          t0, -40(s0)
	li          t1, 56
	mul         t0, t0, t1
	addi        t0, t0, 64
	ld          t1, 72(s4)
	li          t2, 64
	mul         t1, t1, t2
	add         s5, t0, t1
	sd          x0, -104(s0)
.WriteSectionHeaders_label_45:
	ld          t0, -96(s0)
	ld          t1, 72(s4)
	bge         t0, t1, .WriteSectionHeaders_label_243
	addi        t0, s4, 64
	ld          t0, 0(t0)
	ld          t1, -96(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          s2, 0(t0)
	ld          t0, 104(s2)
	beqz        t0, .WriteSectionHeaders_label_77
	ld          a0, 104(s2)
	call    ELFWriterSectionContentsGetLength
	mv      s3, a0
	j           .WriteSectionHeaders_label_169
.WriteSectionHeaders_label_77:
	ld          t0, -48(s0)
	bne         s2, t0, .WriteSectionHeaders_label_90
	ld          t0, 144(s4)
	li          t1, 24
	mul         s3, t0, t1
	j           .WriteSectionHeaders_label_168
.WriteSectionHeaders_label_90:
	ld          t0, -56(s0)
	bne         s2, t0, .WriteSectionHeaders_label_101
	ld          s3, 120(s4)
	j           .WriteSectionHeaders_label_167
.WriteSectionHeaders_label_101:
	ld          t0, -64(s0)
	bne         s2, t0, .WriteSectionHeaders_label_112
	ld          s3, 216(s4)
	j           .WriteSectionHeaders_label_166
.WriteSectionHeaders_label_112:
.WriteSectionHeaders_label_113:
	ld          t0, -112(s0)
	ld          t1, -24(s0)
	ld          t1, 8(t1)
	bge         t0, t1, .WriteSectionHeaders_label_152
	ld          t0, -24(s0)
	ld          t0, 0(t0)
	ld          t1, -112(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	bne         t0, s2, .WriteSectionHeaders_label_144
	ld          t0, 120(s2)
	ld          t0, 8(t0)
	li          t1, 24
	mul         s3, t0, t1
	li          t0, 1
	sb          t0, -72(s0)
	j           .WriteSectionHeaders_label_152
.WriteSectionHeaders_label_144:
.WriteSectionHeaders_label_145:
	ld          t0, -112(s0)
	addi        t0, t0, 1
	sd          t0, -112(s0)
	j           .WriteSectionHeaders_label_113
.WriteSectionHeaders_label_152:
.WriteSectionHeaders_label_153:
	lb          t0, -72(s0)
	not         t0, t0
	beqz        t0, .WriteSectionHeaders_label_163
	lla         a0, .str.6
	call    printf
	call    abort
.WriteSectionHeaders_label_163:
.WriteSectionHeaders_label_164:
.WriteSectionHeaders_label_165:
.WriteSectionHeaders_label_166:
.WriteSectionHeaders_label_167:
.WriteSectionHeaders_label_168:
.WriteSectionHeaders_label_169:
	lwu         t0, 40(s2)
	beqz        t0, .WriteSectionHeaders_label_202
	ld          a1, 88(s2)
	mv      a0, s5
	call    AlignTo
	sd          a0, -88(s0)
	ld          t0, -88(s0)
	sub         t0, t0, s5
	sd          t0, -80(s0)
	addi        t0, s2, 40
	ld          t1, -88(s0)
	sd          t1, 24(t0)
	ld          t0, -104(s0)
	beqz        t0, .WriteSectionHeaders_label_201
	ld          t0, -104(s0)
	ld          t1, -80(s0)
	sd          t1, 136(t0)
.WriteSectionHeaders_label_201:
.WriteSectionHeaders_label_202:
	addi        t0, s2, 40
	sd          s3, 32(t0)
	addi        a0, s2, 40
	ld          a3, -32(s0)
	li          a2, 1
	li          a1, 64
	call    fwrite
	lwu         t0, 44(s2)
	li          t1, 8
	beq         t0, t1, .WriteSectionHeaders_label_234
	ld          t0, -80(s0)
	add         t0, s3, t0
	add         s5, s5, t0
.WriteSectionHeaders_label_234:
	sd          s2, -104(s0)
.WriteSectionHeaders_label_236:
	ld          t0, -96(s0)
	addi        t0, t0, 1
	sd          t0, -96(s0)
	j           .WriteSectionHeaders_label_45
.WriteSectionHeaders_label_243:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 136(sp)
	ld s0, 128(sp)
	addi sp, sp, 144
	ret         
.func_end_WriteSectionHeaders:
	.size WriteSectionHeaders, .func_end_WriteSectionHeaders-WriteSectionHeaders

	.local  WriteSectionContents
	.type WriteSectionContents, @function

WriteSectionContents:
	.global ELFWriterSectionContentsWrite
	.global fwrite
	.global printf
	.global abort
	.local Pad
	addi sp, sp, -144
	// Saved return address (offset 136) and frame pointer (offset 128)
	sd ra, 136(sp)
	sd s0, 128(sp)
	addi s0, sp, 144
	// Saved argument registers.
	sd a4, -24(s0)
	sd a1, -32(s0)
	sd a2, -40(s0)
	sd a3, -48(s0)
	// Local vars at offset -104(s0)
	// Saved integer registers.
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// Register variable loads.
	mv   s3, a0		// elf
	mv   s4, a5		// fp
	// End of stack frame
.WriteSectionContents_label_20:
	ld          t0, -96(s0)
	ld          t1, 72(s3)
	bge         t0, t1, .WriteSectionContents_label_262
	addi        t0, s3, 64
	ld          t0, 0(t0)
	ld          t1, -96(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          s2, 0(t0)
	ld          t0, 104(s2)
	beqz        t0, .WriteSectionContents_label_56
	ld          a0, 104(s2)
	mv      a1, s4
	call    ELFWriterSectionContentsWrite
	j           .WriteSectionContents_label_242
.WriteSectionContents_label_56:
	ld          t0, -32(s0)
	bne         s2, t0, .WriteSectionContents_label_103
.WriteSectionContents_label_62:
	ld          t0, -88(s0)
	ld          t1, 144(s3)
	bge         t0, t1, .WriteSectionContents_label_101
	addi        t0, s3, 136
	ld          t0, 0(t0)
	ld          t1, -88(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -64(s0)
	ld          a0, -64(s0)
	mv      a3, s4
	li          a2, 1
	li          a1, 24
	call    fwrite
.WriteSectionContents_label_94:
	ld          t0, -88(s0)
	addi        t0, t0, 1
	sd          t0, -88(s0)
	j           .WriteSectionContents_label_62
.WriteSectionContents_label_101:
	j           .WriteSectionContents_label_241
.WriteSectionContents_label_103:
	ld          t0, -40(s0)
	bne         s2, t0, .WriteSectionContents_label_128
	ld          s5, 120(s3)
	ld          a0, 112(s3)
	mv      a3, s4
	li          a2, 1
	mv      a1, s5
	call    fwrite
	j           .WriteSectionContents_label_240
.WriteSectionContents_label_128:
	ld          t0, -48(s0)
	bne         s2, t0, .WriteSectionContents_label_152
	ld          s5, 216(s3)
	ld          a0, 208(s3)
	mv      a3, s4
	li          a2, 1
	mv      a1, s5
	call    fwrite
	j           .WriteSectionContents_label_239
.WriteSectionContents_label_152:
.WriteSectionContents_label_153:
	ld          t0, -80(s0)
	ld          t1, -24(s0)
	ld          t1, 8(t1)
	bge         t0, t1, .WriteSectionContents_label_225
	ld          t0, -24(s0)
	ld          t0, 0(t0)
	ld          t1, -80(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	bne         t0, s2, .WriteSectionContents_label_217
.WriteSectionContents_label_173:
	ld          t0, -104(s0)
	ld          t1, 120(s2)
	ld          t1, 8(t1)
	bge         t0, t1, .WriteSectionContents_label_212
	ld          t0, 120(s2)
	ld          t0, 0(t0)
	ld          t1, -104(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          t0, 0(t0)
	sd          t0, -72(s0)
	ld          a0, -72(s0)
	mv      a3, s4
	li          a2, 1
	li          a1, 24
	call    fwrite
.WriteSectionContents_label_205:
	ld          t0, -104(s0)
	addi        t0, t0, 1
	sd          t0, -104(s0)
	j           .WriteSectionContents_label_173
.WriteSectionContents_label_212:
	li          t0, 1
	sb          t0, -56(s0)
	j           .WriteSectionContents_label_225
.WriteSectionContents_label_217:
.WriteSectionContents_label_218:
	ld          t0, -80(s0)
	addi        t0, t0, 1
	sd          t0, -80(s0)
	j           .WriteSectionContents_label_153
.WriteSectionContents_label_225:
.WriteSectionContents_label_226:
	lb          t0, -56(s0)
	not         t0, t0
	beqz        t0, .WriteSectionContents_label_236
	lla         a0, .str.7
	call    printf
	call    abort
.WriteSectionContents_label_236:
.WriteSectionContents_label_237:
.WriteSectionContents_label_238:
.WriteSectionContents_label_239:
.WriteSectionContents_label_240:
.WriteSectionContents_label_241:
.WriteSectionContents_label_242:
	ld          t0, 136(s2)
	beqz        t0, .WriteSectionContents_label_254
	ld          a0, 136(s2)
	mv      a1, s4
	call    Pad
.WriteSectionContents_label_254:
.WriteSectionContents_label_255:
	ld          t0, -96(s0)
	addi        t0, t0, 1
	sd          t0, -96(s0)
	j           .WriteSectionContents_label_20
.WriteSectionContents_label_262:
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld ra, 136(sp)
	ld s0, 128(sp)
	addi sp, sp, 144
	ret         
.func_end_WriteSectionContents:
	.size WriteSectionContents, .func_end_WriteSectionContents-WriteSectionContents

	.local  WriteProgramHeaders
	.type WriteProgramHeaders, @function

WriteProgramHeaders:
	.global fseek
	.global memset
	.global fwrite
	addi sp, sp, -160
	// Saved return address (offset 152) and frame pointer (offset 144)
	sd ra, 152(sp)
	sd s0, 144(sp)
	addi s0, sp, 160
	// Saved argument registers.
	sd a2, -24(s0)
	sd a1, -32(s0)
	// Local vars at offset -128(s0)
	// Saved integer registers.
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	sd s5, 0(sp)
	// Register variable loads.
	mv   s2, a0		// elf
	// End of stack frame
	ld          t0, 96(s2)
	bge         x0, t0, .WriteProgramHeaders_label_274
	ld          a0, -24(s0)
	ld          a1, 32(s2)
	li          a2, 2
	call    fseek
	addi        a0, s0, -128
	li          a2, 56
	mv      a1, x0
	call    memset
	addi        t0, s0, -128
	ld          t1, 32(s2)
	sd          t1, 8(t0)
	addi        t0, s0, -128
	ld          t1, -32(s0)
	li          t2, 56
	mul         t1, t1, t2
	sd          t1, 32(t0)
	addi        t0, s0, -128
	ld          t1, -96(s0)
	sd          t1, 40(t0)
	addi        t0, s0, -128
	li          t1, 8
	sd          t1, 48(t0)
	addi        t0, s0, -128
	li          t1, 5
	sw          t1, 4(t0)
	addi        t0, s0, -128
	li          t1, 6
	sw          t1, 0(t0)
	addi        t0, s2, 88
	ld          t0, 0(t0)
	add         t0, t0, x0
	ld          t0, 0(t0)
	sd          t0, -64(s0)
	ld          t0, -64(s0)
	addi        t0, t0, 56
	ld          t0, 0(t0)
	add         t0, t0, x0
	ld          t0, 0(t0)
	sd          t0, -48(s0)
	addi        t0, s0, -128
	ld          t1, -48(s0)
	ld          t1, 128(t1)
	ld          t2, -64(s0)
	ld          t2, 48(t2)
	addi        t2, t2, -1
	not         t2, t2
	and         t1, t1, t2
	ld          t2, 32(s2)
	add         t1, t1, t2
	sd          t1, 16(t0)
	addi        t0, s0, -128
	ld          t1, -112(s0)
	sd          t1, 24(t0)
	addi        a0, s0, -128
	ld          a3, -24(s0)
	li          a2, 1
	li          a1, 56
	call    fwrite
.WriteProgramHeaders_label_156:
	ld          t0, 96(s2)
	bge         s5, t0, .WriteProgramHeaders_label_272
	addi        t0, s2, 88
	ld          t0, 0(t0)
	slli        t1, s5, 3
	add         t0, t0, t1
	ld          s3, 0(t0)
.WriteProgramHeaders_label_173:
	ld          t0, -72(s0)
	ld          t1, 64(s3)
	bge         t0, t1, .WriteProgramHeaders_label_252
	addi        t0, s3, 56
	ld          t0, 0(t0)
	ld          t1, -72(s0)
	slli        t1, t1, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	ld          t0, 72(s4)
	addi        t1, s3, 0
	ld          t2, 40(s3)
	add         t0, t2, t0
	sd          t0, 40(t1)
	lwu         t0, 44(s4)
	li          t1, 8
	beq         t0, t1, .WriteProgramHeaders_label_215
	ld          t0, 72(s4)
	addi        t1, s3, 0
	ld          t2, 32(s3)
	add         t0, t2, t0
	sd          t0, 32(t1)
.WriteProgramHeaders_label_215:
	lb          t0, -56(s0)
	not         t0, t0
	beqz        t0, .WriteProgramHeaders_label_244
	addi        t0, s3, 0
	ld          t1, 64(s4)
	sd          t1, 8(t0)
	addi        t0, s3, 0
	ld          t1, 128(s4)
	sd          t1, 16(t0)
	addi        t0, s3, 0
	ld          t1, 16(s3)
	sd          t1, 24(t0)
	li          t0, 1
	sb          t0, -56(s0)
	ld          t0, 64(s4)
	sd          t0, -40(s0)
.WriteProgramHeaders_label_244:
.WriteProgramHeaders_label_245:
	ld          t0, -72(s0)
	addi        t0, t0, 1
	sd          t0, -72(s0)
	j           .WriteProgramHeaders_label_173
.WriteProgramHeaders_label_252:
	addi        a0, s3, 0
	ld          a3, -24(s0)
	li          a2, 1
	li          a1, 56
	call    fwrite
.WriteProgramHeaders_label_266:
	addi        s5, s5, 1
	j           .WriteProgramHeaders_label_156
.WriteProgramHeaders_label_272:
	j           .WriteProgramHeaders_label_283
.WriteProgramHeaders_label_274:
	addi        t0, s2, 0
	sd          x0, 32(t0)
.WriteProgramHeaders_label_283:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 152(sp)
	ld s0, 144(sp)
	addi sp, sp, 160
	ret         
.func_end_WriteProgramHeaders:
	.size WriteProgramHeaders, .func_end_WriteProgramHeaders-WriteProgramHeaders

	.global ELFWriterFileWrite
	.type ELFWriterFileWrite, @function

ELFWriterFileWrite:
	.global ELFWriterAddStandardSection
	.global ELFWriterAddSectionFixup
	.local CreateRelocationSections
	.global ELFWriterFixupSections
	.global fseek
	.local WriteSectionHeaders
	.local WriteSectionContents
	.local WriteProgramHeaders
	.global rewind
	.global fwrite
	.global VectorDestruct
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -56(s0)
	// Saved integer registers.
	sd s1, 32(sp)
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	sd s5, 0(sp)
	// Register variable loads.
	mv   s2, a0		// elf
	mv   s4, a1		// fp
	// End of stack frame
	lla         a1, .str.8
	mv      a3, x0
	li          a2, 2
	mv      a0, s2
	call    ELFWriterAddStandardSection
	mv      s3, a0
	lla         a1, .str.9
	li          a3, 32
	li          a2, 3
	mv      a0, s2
	call    ELFWriterAddStandardSection
	mv      s5, a0
	lla         a1, .str.10
	li          a3, 32
	li          a2, 3
	mv      a0, s2
	call    ELFWriterAddStandardSection
	sd          a0, -32(s0)
	addi        t0, s3, 40
	li          t1, 24
	sd          t1, 56(t0)
	mv      a3, s5
	mv      a2, s3
	li          a1, 1
	mv      a0, s2
	call    ELFWriterAddSectionFixup
	addi        t0, s3, 40
	lw          t1, 232(s2)
	addi        t1, t1, 1
	sw          t1, 44(t0)
	sd          x0, 0(s0)
	sd          x0, 8(s0)
	sd          x0, 16(s0)
	addi        t0, s0, -56
	sd          x0, 0(t0)
	addi        a1, s0, -56
	mv      a2, s3
	mv      a0, s2
	call    CreateRelocationSections
	mv      a0, s2
	call    ELFWriterFixupSections
	ld          t1, 96(s2)
	bnez        t1, .ELFWriterFileWrite_label_154
	mv      t0, x0
	j           .ELFWriterFileWrite_label_160
.ELFWriterFileWrite_label_154:
	ld          t1, 96(s2)
	addi        t0, t1, 1
.ELFWriterFileWrite_label_160:
	sd          t0, -24(s0)
	addi        t1, s2, 0
	ld          t2, -24(s0)
	li          t3, 56
	mul         t2, t2, t3
	addi        t2, t2, 64
	sd          t2, 40(t1)
	ld          a1, 40(s2)
	li          a2, 2
	mv      a0, s4
	call    fseek
	ld          a3, -32(s0)
	addi        a4, s0, -56
	ld          a5, -24(s0)
	mv      a6, s4
	mv      a2, s5
	mv      a1, s3
	mv      a0, s2
	call    WriteSectionHeaders
	ld          t1, 264(s2)
	beqz        t1, .ELFWriterFileWrite_label_212
	ld          s1, 264(s2)
	mv      a0, s2
	jalr     x1, s1, 0
.ELFWriterFileWrite_label_212:
	ld          a3, -32(s0)
	addi        a4, s0, -56
	mv      a5, s4
	mv      a2, s5
	mv      a1, s3
	mv      a0, s2
	call    WriteSectionContents
	ld          a1, -24(s0)
	mv      a2, s4
	mv      a0, s2
	call    WriteProgramHeaders
	mv      a0, s4
	call    rewind
	addi        a0, s2, 0
	mv      a3, s4
	li          a2, 1
	li          a1, 64
	call    fwrite
	addi        a0, s0, -56
	call    VectorDestruct
	ld s1, 32(sp)
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 88(sp)
	ld s0, 80(sp)
	addi sp, sp, 96
	ret         
.func_end_ELFWriterFileWrite:
	.size ELFWriterFileWrite, .func_end_ELFWriterFileWrite-ELFWriterFileWrite

	.global NewELFWriterFileFromFile
	.type NewELFWriterFileFromFile, @function

NewELFWriterFileFromFile:
	.global malloc
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s2, 8(sp)
	// End of stack frame
	li          a0, 272
	call    malloc
	mv      s2, a0
	mv      a0, s2
.NewELFWriterFileFromFile_label_13:
	ld s2, 8(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_NewELFWriterFileFromFile:
	.size NewELFWriterFileFromFile, .func_end_NewELFWriterFileFromFile-NewELFWriterFileFromFile

	.global ELFWriterSectionDestruct
	.type ELFWriterSectionDestruct, @function

ELFWriterSectionDestruct:
	.global StringDestruct
	.global VectorDelete
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s2, 8(sp)
	// Register variable loads.
	mv   s2, a0		// sect
	// End of stack frame
	addi        a0, s2, 0
	call    StringDestruct
	ld          t0, 120(s2)
	beqz        t0, .ELFWriterSectionDestruct_label_21
	ld          a0, 120(s2)
	call    VectorDelete
.ELFWriterSectionDestruct_label_21:
	ld s2, 8(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFWriterSectionDestruct:
	.size ELFWriterSectionDestruct, .func_end_ELFWriterSectionDestruct-ELFWriterSectionDestruct

	.global ELFWriterSectionDelete
	.type ELFWriterSectionDelete, @function

ELFWriterSectionDelete:
	.global ELFWriterSectionDestruct
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
	mv   s2, a0		// sect
	// End of stack frame
	mv      a0, s2
	call    ELFWriterSectionDestruct
	mv      a0, s2
	call    free
	ld s2, 8(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFWriterSectionDelete:
	.size ELFWriterSectionDelete, .func_end_ELFWriterSectionDelete-ELFWriterSectionDelete

	.global ELFWriterAddSection
	.type ELFWriterAddSection, @function

ELFWriterAddSection:
	.global calloc
	.global StringInit
	.global ELFWriterAddSectionName
	.global NewVector
	.global VectorAppend
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Saved argument registers.
	sd a5, -24(s0)
	sd a2, -32(s0)
	sd a3, -40(s0)
	sd a4, -48(s0)
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// Register variable loads.
	mv   s3, a0		// elf
	mv   s4, a1		// name
	mv   s5, a6		// address
	// End of stack frame
	li          a1, 1
	li          a0, 152
	call    calloc
	mv      s2, a0
	beqz        s4, .ELFWriterAddSection_label_59
	addi        a0, s2, 0
	ld          a1, 16(s4)
	call    StringInit
	addi        s1, s2, 40
	ld          a1, 16(s4)
	mv      a0, s3
	call    ELFWriterAddSectionName
	sw          a0, 0(s1)
	j           .ELFWriterAddSection_label_68
.ELFWriterAddSection_label_59:
	addi        a0, s2, 0
	lla         a1, .str.11
	call    StringInit
.ELFWriterAddSection_label_68:
	addi        t0, s2, 40
	lw          t1, -32(s0)
	sw          t1, 4(t0)
	addi        t0, s2, 40
	ld          t1, -40(s0)
	sd          t1, 8(t0)
	addi        t0, s2, 40
	ld          t1, -48(s0)
	sd          t1, 48(t0)
	addi        t0, s2, 40
	sd          s5, 16(t0)
	addi        t0, s2, 40
	sw          x0, 44(t0)
	ld          t0, -24(s0)
	sd          t0, 104(s2)
	call    NewVector
	sd          a0, 120(s2)
	ld          t0, 72(s3)
	sw          t0, 112(s2)
	sd          s5, 128(s2)
	sd          x0, 136(s2)
	sd          x0, 144(s2)
	addi        a0, s3, 64
	mv      a1, s2
	call    VectorAppend
	mv      a0, s2
.ELFWriterAddSection_label_122:
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld ra, 88(sp)
	ld s0, 80(sp)
	addi sp, sp, 96
	ret         
.func_end_ELFWriterAddSection:
	.size ELFWriterAddSection, .func_end_ELFWriterAddSection-ELFWriterAddSection

	.global ELFWriterAddStandardSection
	.type ELFWriterAddStandardSection, @function

ELFWriterAddStandardSection:
	.global calloc
	.global StringInit
	.global ELFWriterAddSectionName
	.global NewVector
	.global VectorAppend
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Saved argument registers.
	sd a3, -24(s0)
	// Local vars at offset -24(s0)
	// Saved integer registers.
	sd s1, 48(sp)
	sd s2, 40(sp)
	sd s3, 32(sp)
	sd s4, 24(sp)
	sd s5, 16(sp)
	sd s6, 8(sp)
	// Register variable loads.
	mv   s3, a1		// name
	mv   s4, a0		// elf
	mv   s5, a2		// type
	// End of stack frame
	li          a1, 1
	li          a0, 152
	call    calloc
	mv      s2, a0
	addi        a0, s2, 0
	mv      a1, s3
	call    StringInit
	addi        s1, s2, 40
	bnez        s3, .ELFWriterAddStandardSection_label_48
	mv      s6, x0
	j           .ELFWriterAddStandardSection_label_56
.ELFWriterAddStandardSection_label_48:
	mv      a1, s3
	mv      a0, s4
	call    ELFWriterAddSectionName
	mv      s6, a0
.ELFWriterAddStandardSection_label_56:
	sw          s6, 0(s1)
	addi        t0, s2, 40
	sw          s5, 4(t0)
	addi        t0, s2, 40
	lw          t1, -24(s0)
	sd          t1, 8(t0)
	addi        t0, s2, 40
	li          t1, 8
	sd          t1, 48(t0)
	addi        t0, s2, 40
	sw          x0, 44(t0)
	sd          x0, 104(s2)
	ld          t0, 72(s4)
	sw          t0, 112(s2)
	call    NewVector
	sd          a0, 120(s2)
	sd          x0, 144(s2)
	addi        a0, s4, 64
	mv      a1, s2
	call    VectorAppend
	mv      a0, s2
.ELFWriterAddStandardSection_label_98:
	ld s1, 48(sp)
	ld s2, 40(sp)
	ld s3, 32(sp)
	ld s4, 24(sp)
	ld s5, 16(sp)
	ld s6, 8(sp)
	ld ra, 72(sp)
	ld s0, 64(sp)
	addi sp, sp, 80
	ret         
.func_end_ELFWriterAddStandardSection:
	.size ELFWriterAddStandardSection, .func_end_ELFWriterAddStandardSection-ELFWriterAddStandardSection

	.global ELFSymbolInit
	.type ELFSymbolInit, @function

ELFSymbolInit:
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 40)
	sd s0, 40(sp)
	addi s0, sp, 48
	// Saved argument registers.
	sd a6, -24(s0)
	sd a3, -32(s0)
	sd a4, -40(s0)
	// Local vars at offset -40(s0)
	// Register variable loads.
	mv   t3, a0		// sym
	mv   t4, a1		// name_offset
	mv   t5, a2		// section_index
	mv   t6, a5		// size
	// End of stack frame
	sw          t4, 0(t3)
	sb          x0, 5(t3)
	sd          t6, 16(t3)
	ld          t0, -24(s0)
	sd          t0, 8(t3)
	lw          t0, -32(s0)
	lw          t1, -40(s0)
	slli        t1, t1, 4
	or          t0, t0, t1
	andi        t0, t0, 255
	sb          t0, 4(t3)
	ld s0, 40(sp)
	addi sp, sp, 48
	ret         
.func_end_ELFSymbolInit:
	.size ELFSymbolInit, .func_end_ELFSymbolInit-ELFSymbolInit

	.global NewELFSymbol
	.type NewELFSymbol, @function

NewELFSymbol:
	.global calloc
	.global ELFSymbolInit
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Saved argument registers.
	sd a3, -24(s0)
	sd a4, -32(s0)
	sd a5, -40(s0)
	// Local vars at offset -40(s0)
	// Saved integer registers.
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// Register variable loads.
	mv   s3, a0		// name_offset
	mv   s4, a1		// section_index
	mv   s5, a2		// symbol_type
	// End of stack frame
	li          a1, 1
	li          a0, 24
	call    calloc
	mv      s2, a0
	lw          a4, -24(s0)
	ld          a5, -32(s0)
	ld          a6, -40(s0)
	mv      a3, s5
	mv      a2, s4
	mv      a1, s3
	mv      a0, s2
	call    ELFSymbolInit
	mv      a0, s2
.NewELFSymbol_label_49:
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld ra, 72(sp)
	ld s0, 64(sp)
	addi sp, sp, 80
	ret         
.func_end_NewELFSymbol:
	.size NewELFSymbol, .func_end_NewELFSymbol-NewELFSymbol

	.global ELFWriterAddSymbol
	.type ELFWriterAddSymbol, @function

ELFWriterAddSymbol:
	.global NewELFSymbol
	.global ELFWriterAddString
	.global VectorAppend
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Saved argument registers.
	sd a4, -24(s0)
	sd a5, -32(s0)
	sd a6, -40(s0)
	sd a7, -48(s0)
	sd a1, -56(s0)
	// Local vars at offset -56(s0)
	// Saved integer registers.
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// Register variable loads.
	mv   s2, a0		// elf
	mv   s4, a2		// section_index
	mv   s5, a3		// symbol_type
	// End of stack frame
	ld          a1, -56(s0)
	mv      a0, s2
	call    ELFWriterAddString
	lw          a3, -24(s0)
	ld          a4, -32(s0)
	ld          a5, -40(s0)
	mv      a2, s5
	mv      a1, s4
	call    NewELFSymbol
	mv      s3, a0
	ld          t0, -48(s0)
	ld          t1, 144(s2)
	sw          t1, 0(t0)
	addi        a0, s2, 136
	mv      a1, s3
	call    VectorAppend
	mv      a0, s3
.ELFWriterAddSymbol_label_60:
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld ra, 88(sp)
	ld s0, 80(sp)
	addi sp, sp, 96
	ret         
.func_end_ELFWriterAddSymbol:
	.size ELFWriterAddSymbol, .func_end_ELFWriterAddSymbol-ELFWriterAddSymbol

	.global ELFWriterAddSectionSymbol
	.type ELFWriterAddSectionSymbol, @function

ELFWriterAddSectionSymbol:
	.global calloc
	.global ELFWriterAddString
	.global VectorAppend
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
	mv   s3, a0		// elf
	mv   s4, a1		// name
	mv   s5, a2		// index
	// End of stack frame
	li          a1, 1
	li          a0, 24
	call    calloc
	mv      s2, a0
	mv      a1, s4
	mv      a0, s3
	call    ELFWriterAddString
	sw          a0, 0(s2)
	lbu         t0, 4(s2)
	ori         t0, t0, 3
	sb          t0, 4(s2)
	lbu         t0, 4(s2)
	sb          t0, 4(s2)
	addi        a0, s3, 136
	mv      a1, s2
	call    VectorAppend
	mv      a0, s2
.ELFWriterAddSectionSymbol_label_54:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.func_end_ELFWriterAddSectionSymbol:
	.size ELFWriterAddSectionSymbol, .func_end_ELFWriterAddSectionSymbol-ELFWriterAddSectionSymbol

	.global ELFWriterAddFileSymbol
	.type ELFWriterAddFileSymbol, @function

ELFWriterAddFileSymbol:
	.global calloc
	.global ELFWriterAddString
	.global VectorAppend
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
	// Register variable loads.
	mv   s3, a0		// elf
	mv   s4, a1		// filename
	// End of stack frame
	li          a1, 1
	li          a0, 24
	call    calloc
	mv      s2, a0
	mv      a1, s4
	mv      a0, s3
	call    ELFWriterAddString
	sw          a0, 0(s2)
	lbu         t0, 4(s2)
	ori         t0, t0, 4
	sb          t0, 4(s2)
	lbu         t0, 4(s2)
	sb          t0, 4(s2)
	addi        a0, s3, 136
	mv      a1, s2
	call    VectorAppend
	mv      a0, s2
.ELFWriterAddFileSymbol_label_52:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.func_end_ELFWriterAddFileSymbol:
	.size ELFWriterAddFileSymbol, .func_end_ELFWriterAddFileSymbol-ELFWriterAddFileSymbol

	.global ELFWriterInitRelocation
	.type ELFWriterInitRelocation, @function

ELFWriterInitRelocation:
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 24)
	sd s0, 24(sp)
	addi s0, sp, 32
	// Saved argument registers.
	sd a3, -24(s0)
	// Local vars at offset -24(s0)
	// Register variable loads.
	mv   t3, a0		// r
	mv   t4, a1		// offset
	mv   t5, a2		// symbol_index
	mv   t6, a4		// type
	// End of stack frame
	sd          t4, 0(t3)
	slli        t0, t5, 32
	or          t0, t0, t6
	sd          t0, 8(t3)
	ld          t0, -24(s0)
	sd          t0, 16(t3)
	ld s0, 24(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFWriterInitRelocation:
	.size ELFWriterInitRelocation, .func_end_ELFWriterInitRelocation-ELFWriterInitRelocation

	.global ELFWriterAddRelocationWithAddend
	.type ELFWriterAddRelocationWithAddend, @function

ELFWriterAddRelocationWithAddend:
	.global malloc
	.global ELFWriterInitRelocation
	.global printf
	.global abort
	.global ELFWriterInsertRelocation
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Saved argument registers.
	sd a5, -24(s0)
	sd a0, -32(s0)
	sd a1, -40(s0)
	// Local vars at offset -40(s0)
	// Saved integer registers.
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// Register variable loads.
	mv   s3, a3		// symbol_index
	mv   s4, a2		// offset
	mv   s5, a4		// addend
	// End of stack frame
	li          a0, 24
	call    malloc
	mv      s2, a0
	lw          a4, -24(s0)
	mv      a3, s5
	mv      a2, s3
	mv      a1, s4
	mv      a0, s2
	call    ELFWriterInitRelocation
.ELFWriterAddRelocationWithAddend_label_38:
	addi        t0, s3, 1
	snez        t0, t0
	not         t0, t0
	beqz        t0, .ELFWriterAddRelocationWithAddend_label_50
	lla         a0, .str.12
	call    printf
	call    abort
.ELFWriterAddRelocationWithAddend_label_50:
.ELFWriterAddRelocationWithAddend_label_51:
.ELFWriterAddRelocationWithAddend_label_52:
	ld          a0, -32(s0)
	lw          a1, -40(s0)
	mv      a2, s2
	call    ELFWriterInsertRelocation
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld ra, 72(sp)
	ld s0, 64(sp)
	addi sp, sp, 80
	ret         
.func_end_ELFWriterAddRelocationWithAddend:
	.size ELFWriterAddRelocationWithAddend, .func_end_ELFWriterAddRelocationWithAddend-ELFWriterAddRelocationWithAddend

	.global ELFWriterAddRelocation
	.type ELFWriterAddRelocation, @function

ELFWriterAddRelocation:
	.global ELFWriterAddRelocationWithAddend
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Saved argument registers.
	sd a4, -24(s0)
	// Local vars at offset -24(s0)
	// Saved integer registers.
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// Register variable loads.
	mv   s2, a0		// elf
	mv   s3, a1		// section_index
	mv   s4, a2		// offset
	mv   s5, a3		// symbol_index
	// End of stack frame
	lw          a5, -24(s0)
	mv      a4, x0
	mv      a3, s5
	mv      a2, s4
	mv      a1, s3
	mv      a0, s2
	call    ELFWriterAddRelocationWithAddend
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld ra, 56(sp)
	ld s0, 48(sp)
	addi sp, sp, 64
	ret         
.func_end_ELFWriterAddRelocation:
	.size ELFWriterAddRelocation, .func_end_ELFWriterAddRelocation-ELFWriterAddRelocation

	.global ELFWriterInsertRelocation
	.type ELFWriterInsertRelocation, @function

ELFWriterInsertRelocation:
	.global VectorAppend
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
	mv   s3, a0		// elf
	mv   s4, a1		// section_index
	mv   s5, a2		// reloc
	// End of stack frame
	addi        t0, s3, 64
	ld          t0, 0(t0)
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          s2, 0(t0)
	ld          a0, 120(s2)
	mv      a1, s5
	call    VectorAppend
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.func_end_ELFWriterInsertRelocation:
	.size ELFWriterInsertRelocation, .func_end_ELFWriterInsertRelocation-ELFWriterInsertRelocation

	.global ELFWriterAddString
	.type ELFWriterAddString, @function

ELFWriterAddString:
	.local AddBufferedString
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
	mv   s3, a1		// str
	// End of stack frame
	addi        a0, s2, 112
	mv      a1, s3
	call    AddBufferedString
.ELFWriterAddString_label_16:
	ld s2, 8(sp)
	ld s3, 0(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFWriterAddString:
	.size ELFWriterAddString, .func_end_ELFWriterAddString-ELFWriterAddString

	.global ELFWriterAddRawString
	.type ELFWriterAddRawString, @function

ELFWriterAddRawString:
	.global strlen
	.global BufferAppend
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
	mv   s3, a0		// elf
	mv   s5, a1		// str
	// End of stack frame
	ld          s2, 120(s3)
	mv      a0, s5
	call    strlen
	addi        s4, a0, 1
	addi        a0, s3, 112
	mv      a2, s4
	mv      a1, s5
	call    BufferAppend
	mv      a0, s2
.ELFWriterAddRawString_label_34:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.func_end_ELFWriterAddRawString:
	.size ELFWriterAddRawString, .func_end_ELFWriterAddRawString-ELFWriterAddRawString

	.global ELFWriterAddSectionName
	.type ELFWriterAddSectionName, @function

ELFWriterAddSectionName:
	.global strlen
	.global BufferAppend
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
	mv   s3, a0		// elf
	mv   s5, a1		// name
	// End of stack frame
	ld          s2, 216(s3)
	mv      a0, s5
	call    strlen
	addi        s4, a0, 1
	addi        a0, s3, 208
	mv      a2, s4
	mv      a1, s5
	call    BufferAppend
	mv      a0, s2
.ELFWriterAddSectionName_label_34:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld s5, 0(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.func_end_ELFWriterAddSectionName:
	.size ELFWriterAddSectionName, .func_end_ELFWriterAddSectionName-ELFWriterAddSectionName

	.global NewELFWriterSegment
	.type NewELFWriterSegment, @function

NewELFWriterSegment:
	.global malloc
	.global VectorInit
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Saved argument registers.
	sd a1, -24(s0)
	// Local vars at offset -24(s0)
	// Saved integer registers.
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// Register variable loads.
	mv   s4, a0		// type
	mv   s5, a2		// alignment
	// End of stack frame
	li          a0, 80
	call    malloc
	mv      s3, a0
	addi        s2, s3, 0
	sw          s4, 0(s2)
	sd          s5, 48(s2)
	sd          x0, 32(s2)
	sd          x0, 40(s2)
	sd          x0, 8(s2)
	ld          t0, 16(s2)
	sd          t0, 24(s2)
	sd          x0, 0(t0)
	lw          t0, -24(s0)
	sw          t0, 4(s2)
	addi        a0, s3, 56
	call    VectorInit
	mv      a0, s3
.NewELFWriterSegment_label_58:
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld ra, 56(sp)
	ld s0, 48(sp)
	addi sp, sp, 64
	ret         
.func_end_NewELFWriterSegment:
	.size NewELFWriterSegment, .func_end_NewELFWriterSegment-NewELFWriterSegment

	.global ELFWriterSegmentDestruct
	.type ELFWriterSegmentDestruct, @function

ELFWriterSegmentDestruct:
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
	mv   s2, a0		// segment
	// End of stack frame
	addi        a0, s2, 56
	call    VectorDestruct
	ld s2, 8(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFWriterSegmentDestruct:
	.size ELFWriterSegmentDestruct, .func_end_ELFWriterSegmentDestruct-ELFWriterSegmentDestruct

	.global ELFWriterSegmentDelete
	.type ELFWriterSegmentDelete, @function

ELFWriterSegmentDelete:
	.global ELFWriterSegmentDestruct
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
	mv   s2, a0		// segment
	// End of stack frame
	mv      a0, s2
	call    ELFWriterSegmentDestruct
	mv      a0, s2
	call    free
	ld s2, 8(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFWriterSegmentDelete:
	.size ELFWriterSegmentDelete, .func_end_ELFWriterSegmentDelete-ELFWriterSegmentDelete

	.global ELFWriterSegmentAddSection
	.type ELFWriterSegmentAddSection, @function

ELFWriterSegmentAddSection:
	.global VectorAppend
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
	mv   s2, a0		// segment
	mv   s3, a1		// section
	// End of stack frame
	addi        a0, s2, 56
	mv      a1, s3
	call    VectorAppend
	ld s2, 8(sp)
	ld s3, 0(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_ELFWriterSegmentAddSection:
	.size ELFWriterSegmentAddSection, .func_end_ELFWriterSegmentAddSection-ELFWriterSegmentAddSection

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "(null)"
	.type .str.1, @object
	.size .str.1, 1

.str.2:
	.asciz "(null)"
	.type .str.2, @object
	.size .str.2, 1

.str.3:
	.asciz "Assertion failed: from!=NULL&&to!=NULL"
	.type .str.3, @object
	.size .str.3, 39

.str.4:
	.asciz "(null)"
	.type .str.4, @object
	.size .str.4, 1

.str.5:
	.asciz ".rela%s"
	.type .str.5, @object
	.size .str.5, 8

.str.6:
	.asciz "Assertion failed: section_found"
	.type .str.6, @object
	.size .str.6, 32

.str.7:
	.asciz "Assertion failed: section_found"
	.type .str.7, @object
	.size .str.7, 32

.str.8:
	.asciz ".symtab"
	.type .str.8, @object
	.size .str.8, 8

.str.9:
	.asciz ".strtab"
	.type .str.9, @object
	.size .str.9, 8

.str.10:
	.asciz ".shstrtab"
	.type .str.10, @object
	.size .str.10, 10

.str.11:
	.asciz "(null)"
	.type .str.11, @object
	.size .str.11, 1

.str.12:
	.asciz "Assertion failed: symbol_index!=-1"
	.type .str.12, @object
	.size .str.12, 35

