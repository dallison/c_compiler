	.file   "/Users/dallison/Google Drive/c_compiler/libc/strcmp.c"
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


	.global strcmp
	.type strcmp, @function

strcmp:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @13 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @17 */ 	ldx          #2
	jsr          __arg_value2_i1			// b
.strcmp_label_18:
/* @21 */ 	lda         (__i0)
/* @27 */ 	sta         __i2
/* @29 */ 	and          #128
/* @31 */ 	beq         .strcmp_label_30
/* @33 */ 	lda          #255
.strcmp_label_30:
/* @35 */ 	sta         __i2+1
/* @37 */ 	lda         __i2
/* @38 */ 	ora         __i2+1
/* @40 */ 	beq         .strcmp_label_86
/* @43 */ 	lda         (__i0)
/* @49 */ 	sta         __i2
/* @50 */ 	and          #128
/* @52 */ 	beq         .strcmp_label_51
/* @53 */ 	lda          #255
.strcmp_label_51:
/* @54 */ 	sta         __i2+1
/* @57 */ 	lda         (__i1)
/* @63 */ 	sta         __i3
/* @64 */ 	and          #128
/* @66 */ 	beq         .strcmp_label_65
/* @67 */ 	lda          #255
.strcmp_label_65:
/* @68 */ 	sta         __i3+1
/* @72 */ 	lda         __i2
/* @73 */ 	cmp         __i3
/* @74 */ 	bne         .strcmp_label_86
/* @75 */ 	lda         __i2+1
/* @76 */ 	cmp         __i3+1
/* @77 */ 	bne         .strcmp_label_86
/* @80 */ 	lda          #__i0
/* @82 */ 	jsr         __rinc21
/* @83 */ 	lda          #__i1
/* @84 */ 	jsr         __rinc21
/* @85 */ 	bra         .strcmp_label_18
.strcmp_label_86:
/* @89 */ 	lda         (__i0)
/* @95 */ 	sta         __i2
/* @96 */ 	and          #128
/* @98 */ 	beq         .strcmp_label_97
/* @99 */ 	lda          #255
.strcmp_label_97:
/* @100 */ 	sta         __i2+1
/* @103 */ 	lda         (__i1)
/* @109 */ 	sta         __i3
/* @110 */ 	and          #128
/* @112 */ 	beq         .strcmp_label_111
/* @113 */ 	lda          #255
.strcmp_label_111:
/* @114 */ 	sta         __i3+1
/* @119 */ 	sec         
/* @120 */ 	lda         __i2
/* @121 */ 	sbc         __i3
/* @122 */ 	sta         __i4
/* @123 */ 	lda         __i2+1
/* @124 */ 	sbc         __i3+1
/* @125 */ 	sta         __i4+1
/* @127 */ 	lda         #__i4
/* @129 */ 	jsr         __result2
/* @131 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_strcmp:
	.size strcmp, .func_end_strcmp-strcmp

	.data
	.section ".rodata", "aMS", @progbits
