	.file   "/Users/dallison/Google Drive/c_compiler/libc/strrchr.c"
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


	.global strrchr
	.type strrchr, @function

strrchr:
/* @8 */ 	stx         __result
/* @10 */ 	sty         __result+1
/* @11 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x21,0x00,0x00		// Save mask i:1 b:1 l:0 x:0 f:0 
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i2			// s
/* @22 */ 	ldx          #2
	jsr          __arg_value1_b0			// c
/* @23 */ 	lda         __i2
/* @24 */ 	sta         __i1
/* @26 */ 	lda         __i2+1
/* @27 */ 	sta         __i1+1
.strrchr_label_28:
/* @31 */ 	lda         (__i2)
/* @37 */ 	sta         __i3
/* @39 */ 	and          #128
/* @41 */ 	beq         .strrchr_label_40
/* @43 */ 	lda          #255
.strrchr_label_40:
/* @44 */ 	sta         __i3+1
/* @46 */ 	lda         __i3
/* @47 */ 	ora         __i3+1
/* @49 */ 	beq         .strrchr_label_54
/* @50 */ 	lda          #__i2
/* @52 */ 	jsr         __rinc21
/* @53 */ 	bra         .strrchr_label_28
.strrchr_label_54:
.strrchr_label_55:
/* @60 */ 	ldx          #0
/* @62 */ 	lda         __i2
/* @63 */ 	cmp         __i1
/* @64 */ 	lda         __i2+1
/* @65 */ 	sbc         __i1+1
/* @66 */ 	bvc         .strrchr_label_61
/* @67 */ 	eor          #128
.strrchr_label_61:
/* @68 */ 	bmi         .strrchr_label_59
/* @69 */ 	inx         
.strrchr_label_59:
/* @70 */ 	stx         __b1
/* @72 */ 	txa         
/* @73 */ 	cmp          #0
/* @74 */ 	beq         .strrchr_label_112
/* @77 */ 	lda         (__i2)
/* @83 */ 	sta         __i3
/* @84 */ 	and          #128
/* @86 */ 	beq         .strrchr_label_85
/* @87 */ 	lda          #255
.strrchr_label_85:
/* @88 */ 	sta         __i3+1
/* @91 */ 	lda         __b0
/* @92 */ 	sta         __i4
/* @93 */ 	and          #128
/* @95 */ 	beq         .strrchr_label_94
/* @96 */ 	lda          #255
.strrchr_label_94:
/* @97 */ 	sta         __i4+1
/* @103 */ 	ldx          #0
/* @104 */ 	lda         __i3
/* @105 */ 	cmp         __i4
/* @106 */ 	bne         .strrchr_label_102
/* @107 */ 	lda         __i3+1
/* @108 */ 	cmp         __i4+1
/* @109 */ 	beq         .strrchr_label_101
.strrchr_label_102:
/* @110 */ 	inx         
.strrchr_label_101:
/* @111 */ 	stx         __b1
.strrchr_label_112:
/* @114 */ 	lda         __b1
/* @116 */ 	beq         .strrchr_label_121
/* @117 */ 	lda          #__i2
/* @119 */ 	jsr         __rdec21
/* @120 */ 	bra         .strrchr_label_55
.strrchr_label_121:
/* @123 */ 	lda         __i2
/* @124 */ 	cmp         __i1
/* @125 */ 	lda         __i2+1
/* @126 */ 	sbc         __i1+1
/* @127 */ 	bvc         .strrchr_label_122
/* @128 */ 	eor          #128
.strrchr_label_122:
/* @129 */ 	bpl         .strrchr_label_134
/* @131 */ 	stz         __i0
/* @132 */ 	stz         __i0+1
/* @133 */ 	bra         .strrchr_label_140
.strrchr_label_134:
/* @136 */ 	lda         __i2
/* @137 */ 	sta         __i0
/* @138 */ 	lda         __i2+1
/* @139 */ 	sta         __i0+1
.strrchr_label_140:
/* @142 */ 	lda         #__i0
/* @144 */ 	jsr         __result2
/* @146 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_strrchr:
	.size strrchr, .func_end_strrchr-strrchr

	.data
	.section ".rodata", "aMS", @progbits
