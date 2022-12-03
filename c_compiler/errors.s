	.file   "errors.c"
	.text
	.option pic
.PCbegin:
	.global NumErrors
	.type NumErrors, @function

NumErrors:

	// *** Basic block 0

	.global compiler
	// Leaf procedure, no stack frame generated
	la          t0, compiler
	ld          t0, 0(t0)
	lw          a0, 40(t0)

	// *** Basic block 1

.NumErrors_label_10:
	ret         
.func_end_NumErrors:
	.size NumErrors, .func_end_NumErrors-NumErrors

	.global SetMaxErrors
	.type SetMaxErrors, @function

SetMaxErrors:

	// *** Basic block 0

	.global compiler
	// Leaf procedure, no stack frame generated
	la          t0, compiler
	ld          t0, 0(t0)
	sw          a0, 44(t0)
	ret         
.func_end_SetMaxErrors:
	.size SetMaxErrors, .func_end_SetMaxErrors-SetMaxErrors

	.global DisableWarning
	.type DisableWarning, @function

DisableWarning:

	// *** Basic block 0

	.global compiler
	.global SetInsert
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
	la          t0, compiler
	ld          t0, 0(t0)
	lb          t0, 49(t0)
	beqz        t0, .DisableWarning_label_18

	// *** Basic block 1

.DisableWarning_label_15:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.DisableWarning_label_18:
	la          t0, compiler
	ld          t0, 0(t0)
	addi        a0, t0, 56
	mv          a1, s1
	call        SetInsert

	// *** Basic block 3

	j           .DisableWarning_label_15
.func_end_DisableWarning:
	.size DisableWarning, .func_end_DisableWarning-DisableWarning

	.global EnableWarning
	.type EnableWarning, @function

EnableWarning:

	// *** Basic block 0

	.global SetRemove
	.global compiler
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	la          t1, compiler
	ld          t1, 0(t1)
	addi        a0, t1, 56
	mv          a1, t0
	j           SetRemove
.func_end_EnableWarning:
	.size EnableWarning, .func_end_EnableWarning-EnableWarning

	.local  IsWarningDisabled
	.type IsWarningDisabled, @function

IsWarningDisabled:

	// *** Basic block 0

	.global SetContains
	.global compiler
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	la          t1, compiler
	ld          t1, 0(t1)
	addi        a0, t1, 56
	mv          a1, t0
	j           SetContains
.func_end_IsWarningDisabled:
	.size IsWarningDisabled, .func_end_IsWarningDisabled-IsWarningDisabled

	.global VReportError
	.type VReportError, @function

VReportError:

	// *** Basic block 0

	.global vsnprintf
	.global fprintf
	.global stderr
	.global compiler
	.global exit
	addi sp, sp, -16
	sd ra, 8(sp)
	sd s0, 0(sp)
	lui t0, 1
	addi t0, t0, 32
	sub sp, sp, t0
	addi t0, t0, 16
	add s0, sp, t0
	// Local vars at offset -4112(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	li          t0, -4096		// 0xfffffffffffff000
	add         t0, s0, t0
	addi        s3, t0, -16
	li          a1, 4096		// 0x1000
	mv          a0, s3
	call        vsnprintf

	// *** Basic block 1

	bnez        s1, .VReportError_label_56

	// *** Basic block 2

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.1
	mv          a3, s3
	mv          a2, s2
	call        fprintf

	// *** Basic block 3

	j           .VReportError_label_71

	// *** Basic block 4

.VReportError_label_56:
	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.2
	mv          a4, s3
	mv          a3, s1
	mv          a2, s2
	call        fprintf

	// *** Basic block 5

.VReportError_label_71:
	la          t0, compiler
	ld          t0, 0(t0)
	lw          t1, 40(t0)
	addi        t2, t1, 1
	sw          t2, 40(t0)
	lw          t0, 44(t0)
	blt         t1, t0, .VReportError_label_94

	// *** Basic block 6

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.3
	call        fprintf

	// *** Basic block 7

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	call        exit

	// *** Basic block 8

.VReportError_label_94:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_VReportError:
	.size VReportError, .func_end_VReportError-VReportError

	.global ReportError
	.type ReportError, @function

ReportError:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global VReportError
	mv          t0, s0
	mv          a3, t0
	j           VReportError
.func_end_ReportError:
	.size ReportError, .func_end_ReportError-ReportError

	.global VReportWarning
	.type VReportWarning, @function

VReportWarning:

	// *** Basic block 0

	.local IsWarningDisabled
	.global snprintf
	.global compiler
	.global vsnprintf
	.global fprintf
	.global stderr
	.global exit
	addi sp, sp, -16
	sd ra, 8(sp)
	sd s0, 0(sp)
	lui t0, 1
	addi t0, t0, 144
	sub sp, sp, t0
	addi t0, t0, 16
	add s0, sp, t0
	// Local vars at offset -4176(s0)
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
	mv          s1, a2
	mv          s2, a3
	mv          s3, a4
	mv          s4, a1
	mv          s5, a0
	mv          a0, s1
	call        IsWarningDisabled

	// *** Basic block 1

	beqz        a0, .VReportWarning_label_45

	// *** Basic block 2

.VReportWarning_label_42:
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

	// *** Basic block 3

.VReportWarning_label_45:
	lla         s6, .str.4
	li          t0, -4096		// 0xfffffffffffff000
	add         s7, s0, t0
	addi        a0, s7, -80
	lla         a2, .str.5
	mv          a3, s1
	li          t0, 64		// 0x40 ASCII '@'
	mv          a1, t0
	call        snprintf

	// *** Basic block 4

	addi        s8, s7, -80
	la          t0, compiler
	ld          t0, 0(t0)
	lb          s9, 48(t0)
	beqz        s9, .VReportWarning_label_77

	// *** Basic block 5

	lla         s6, .str.6
	lla         s8, .str.7

	// *** Basic block 6

.VReportWarning_label_77:
	addi        s7, s7, -16
	mv          a3, s3
	mv          a2, s2
	li          t0, 4096		// 0x1000
	mv          a1, t0
	mv          a0, s7
	call        vsnprintf

	// *** Basic block 7

	bnez        s4, .VReportWarning_label_112

	// *** Basic block 8

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.8
	mv          a6, s8
	mv          a5, s7
	mv          a4, s5
	mv          a3, s1
	mv          a2, s6
	call        fprintf

	// *** Basic block 9

	j           .VReportWarning_label_133

	// *** Basic block 10

.VReportWarning_label_112:
	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.9
	mv          a7, s8
	mv          a6, s7
	mv          a5, s4
	mv          a4, s5
	mv          a3, s1
	mv          a2, s6
	call        fprintf

	// *** Basic block 11

.VReportWarning_label_133:
	beqz        s9, .VReportWarning_label_141

	// *** Basic block 12

	la          t0, compiler
	ld          t0, 0(t0)
	lw          t1, 40(t0)
	addi        t1, t1, 1
	sw          t1, 40(t0)

	// *** Basic block 13

.VReportWarning_label_141:
	la          t0, compiler
	ld          t0, 0(t0)
	lw          t1, 40(t0)
	lw          t0, 44(t0)
	blt         t1, t0, .VReportWarning_label_162

	// *** Basic block 14

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.10
	call        fprintf

	// *** Basic block 15

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	call        exit

	// *** Basic block 16

.VReportWarning_label_162:
	j           .VReportWarning_label_42
.func_end_VReportWarning:
	.size VReportWarning, .func_end_VReportWarning-VReportWarning

	.global ReportWarning
	.type ReportWarning, @function

ReportWarning:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global VReportWarning
	mv          t0, s0
	mv          a4, t0
	j           VReportWarning
.func_end_ReportWarning:
	.size ReportWarning, .func_end_ReportWarning-ReportWarning

	.global VReportNote
	.type VReportNote, @function

VReportNote:

	// *** Basic block 0

	.global vsnprintf
	.global fprintf
	.global stderr
	addi sp, sp, -16
	sd ra, 8(sp)
	sd s0, 0(sp)
	lui t0, 1
	addi t0, t0, 32
	sub sp, sp, t0
	addi t0, t0, 16
	add s0, sp, t0
	// Local vars at offset -4112(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	li          t0, -4096		// 0xfffffffffffff000
	add         t0, s0, t0
	addi        s3, t0, -16
	li          a1, 4096		// 0x1000
	mv          a0, s3
	call        vsnprintf

	// *** Basic block 1

	bne         s1, x0, .VReportNote_label_51

	// *** Basic block 2

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.11
	mv          a2, s3
	call        fprintf

	// *** Basic block 3

	j           .VReportNote_label_82

	// *** Basic block 4

.VReportNote_label_51:
	bnez        s2, .VReportNote_label_66

	// *** Basic block 5

	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.12
	mv          a3, s3
	mv          a2, s1
	call        fprintf

	// *** Basic block 6

	j           .VReportNote_label_81

	// *** Basic block 7

.VReportNote_label_66:
	la          t0, stderr
	ld          a0, 0(t0)
	lla         a1, .str.13
	mv          a4, s3
	mv          a3, s2
	mv          a2, s1
	call        fprintf

	// *** Basic block 8

.VReportNote_label_81:

	// *** Basic block 9

.VReportNote_label_82:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_VReportNote:
	.size VReportNote, .func_end_VReportNote-VReportNote

	.global ReportNote
	.type ReportNote, @function

ReportNote:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global VReportNote
	mv          t0, s0
	mv          a3, t0
	j           VReportNote
.func_end_ReportNote:
	.size ReportNote, .func_end_ReportNote-ReportNote

	.global FatalError
	.type FatalError, @function

FatalError:

	// *** Basic block 0

	addi sp, sp, -80
	// Saved return address (offset 8) and frame pointer (offset 0)
	sd ra, 8(sp)
	sd s0, 0(sp)
	addi s0, sp, 16
	// varargs function with 0 declared args
	sd a0, 0(s0)
	sd a1, 8(s0)
	sd a2, 16(s0)
	sd a3, 24(s0)
	sd a4, 32(s0)
	sd a5, 40(s0)
	sd a6, 48(s0)
	sd a7, 56(s0)
	// Local vars at offset -16(s0)
	// End of stack frame
	.global vfprintf
	.global stderr
	.global abort
	mv          t0, a0
	mv          t1, s0
	la          t2, stderr
	ld          a0, 0(t2)
	mv          a2, t1
	mv          a1, t0
	call        vfprintf

	// *** Basic block 1

	// Restored registers.
	addi sp, s0, 64
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           abort
.func_end_FatalError:
	.size FatalError, .func_end_FatalError-FatalError

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "error: %s: %s\n"
	.type .str.1, @object
	.size .str.1, 15

.str.2:
	.asciz "error: %s:%d: %s\n"
	.type .str.2, @object
	.size .str.2, 18

.str.3:
	.asciz "Too many errors; terminated\n"
	.type .str.3, @object
	.size .str.3, 29

.str.4:
	.asciz "warning"
	.type .str.4, @object
	.size .str.4, 8

.str.5:
	.asciz "-W%s"
	.type .str.5, @object
	.size .str.5, 5

.str.6:
	.asciz "error"
	.type .str.6, @object
	.size .str.6, 6

.str.7:
	.asciz "-Werror"
	.type .str.7, @object
	.size .str.7, 8

.str.8:
	.asciz "%s[%s]: %s: %s [%s]\n"
	.type .str.8, @object
	.size .str.8, 21

.str.9:
	.asciz "%s[%s]: %s:%d: %s [%s]\n"
	.type .str.9, @object
	.size .str.9, 24

.str.10:
	.asciz "Too many errors; terminated\n"
	.type .str.10, @object
	.size .str.10, 29

.str.11:
	.asciz "    note: %s\n"
	.type .str.11, @object
	.size .str.11, 14

.str.12:
	.asciz "    note: %s: %s\n"
	.type .str.12, @object
	.size .str.12, 18

.str.13:
	.asciz "    note: %s:%d: %s\n"
	.type .str.13, @object
	.size .str.13, 21

