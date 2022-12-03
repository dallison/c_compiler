	.file   "/Users/dallison/Google Drive/c_compiler/libc/strncpy.c"
	.text
	.set __b0 0x0
	.set __b1 0x1
	.set __b2 0x2
	.set __b3 0x3
	.set __b4 0x4
	.set __b5 0x5
	.set __b6 0x6
	.set __b7 0x7
	.set __i0 0x8
	.set __i1 0xa
	.set __i2 0xc
	.set __i3 0xe
	.set __i4 0x10
	.set __i5 0x12
	.set __i6 0x14
	.set __i7 0x16
	.set __i8 0x18
	.set __i9 0x1a
	.set __i10 0x1c
	.set __i11 0x1e
	.set __i12 0x20
	.set __i13 0x22
	.set __i14 0x24
	.set __i15 0x26
	.set __l0 0x28
	.set __l1 0x2c
	.set __l2 0x30
	.set __l3 0x34
	.set __l4 0x38
	.set __l5 0x3c
	.set __l6 0x40
	.set __l7 0x44
	.set __x0 0x48
	.set __x1 0x50
	.set __x2 0x58
	.set __x3 0x60
	.set __f0 0x68
	.set __f1 0x6c
	.set __f2 0x70
	.set __f3 0x74
	.set __sp 0x78
	.set __fp 0x7a
	.set __result 0x7c
	.set __t0 0x7e
	.set __t1 0x7f
	.set __t2 0x80
	.set __t3 0x81
	.set __mem_src 0x82
	.set __mem_dest 0x84
	.set __mem_size 0x86


	.global strncpy
	.type strncpy, @function

strncpy:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @17 */ 	ldx          #0
	jsr          __arg_value2_i1			// dest
/* @21 */ 	ldx          #4
	jsr          __arg_value2_i2			// len
/* @25 */ 	ldx          #2
	jsr          __arg_value2_i3			// src
/* @26 */ 	lda         __i1
/* @27 */ 	sta         __i0
/* @29 */ 	lda         __i1+1
/* @30 */ 	sta         __i0+1
.strncpy_label_31:
/* @37 */ 	ldx          #1
/* @38 */ 	lda          #0
/* @39 */ 	cmp         __i2+1
/* @40 */ 	bcc         .strncpy_label_36
/* @41 */ 	bne         .strncpy_label_35
/* @42 */ 	lda          #0
/* @43 */ 	cmp         __i2
/* @44 */ 	bcc         .strncpy_label_36
.strncpy_label_35:
/* @45 */ 	dex         
.strncpy_label_36:
/* @46 */ 	stx         __b0
/* @48 */ 	txa         
/* @49 */ 	cmp          #0
/* @50 */ 	beq         .strncpy_label_78
/* @53 */ 	lda         (__i3)
/* @59 */ 	sta         __i4
/* @61 */ 	and          #128
/* @63 */ 	beq         .strncpy_label_62
/* @65 */ 	lda          #255
.strncpy_label_62:
/* @66 */ 	sta         __i4+1
/* @71 */ 	ldx          #1
/* @72 */ 	lda         __i4
/* @73 */ 	ora         __i4+1
/* @75 */ 	bne         .strncpy_label_70
/* @76 */ 	dex         
.strncpy_label_70:
/* @77 */ 	stx         __b0
.strncpy_label_78:
/* @80 */ 	lda         __b0
/* @82 */ 	beq         .strncpy_label_116
/* @85 */ 	lda         __i0
/* @86 */ 	sta         __i4
/* @87 */ 	lda         __i0+1
/* @88 */ 	sta         __i4+1
/* @89 */ 	lda          #__i0
/* @91 */ 	jsr         __rinc21
/* @94 */ 	lda         __i3
/* @95 */ 	sta         __i5
/* @96 */ 	lda         __i3+1
/* @97 */ 	sta         __i5+1
/* @98 */ 	lda          #__i3
/* @99 */ 	jsr         __rinc21
/* @104 */ 	lda         (__i5)
/* @111 */ 	sta         (__i4)
/* @112 */ 	lda          #__i2
/* @114 */ 	jsr         __rdec21
/* @115 */ 	bra         .strncpy_label_31
.strncpy_label_116:
.strncpy_label_117:
/* @118 */ 	lda         __i2
/* @119 */ 	ora         __i2+1
/* @121 */ 	beq         .strncpy_label_138
/* @124 */ 	lda         __i0
/* @125 */ 	sta         __i4
/* @126 */ 	lda         __i0+1
/* @127 */ 	sta         __i4+1
/* @128 */ 	lda          #__i0
/* @129 */ 	jsr         __rinc21
/* @132 */ 	lda          #0
/* @133 */ 	tay         
/* @134 */ 	sta         (__i4)
/* @135 */ 	lda          #__i2
/* @136 */ 	jsr         __rdec21
/* @137 */ 	bra         .strncpy_label_117
.strncpy_label_138:
/* @139 */ 	lda         #__i1
/* @141 */ 	jsr         __result2
/* @143 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_strncpy:
	.size strncpy, .func_end_strncpy-strncpy

	.data
	.section ".rodata", "aMS", @progbits
