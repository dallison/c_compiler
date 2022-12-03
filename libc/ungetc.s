	.file   "/Users/dallison/Google Drive/c_compiler/libc/ungetc.c"
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


	.global ungetc
	.type ungetc, @function

ungetc:
/* @12 */ 	stx         __result
/* @14 */ 	sty         __result+1
/* @15 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @19 */ 	ldx          #0
	jsr          __arg_value2_i0			// ch
/* @23 */ 	ldx          #2
	jsr          __arg_value2_i1			// stream
/* @25 */ 	lda         __i0
/* @26 */ 	cmp          #255
/* @27 */ 	bne         .ungetc_label_44
/* @28 */ 	lda         __i0+1
/* @29 */ 	cmp          #255
/* @30 */ 	bne         .ungetc_label_44
/* @34 */ 	lda          #255
/* @35 */ 	sta         __i2
/* @37 */ 	sta         __i2+1
/* @38 */ 	lda         #__i2
/* @40 */ 	jsr         __result2
.ungetc_label_41:
/* @42 */ 	ldy          #8
	jmp          __leave_leaf
.ungetc_label_44:
/* @48 */ 	ldy          #15
/* @49 */ 	lda         (__i1), Y
/* @55 */ 	sta         __i2
/* @57 */ 	and          #128
/* @59 */ 	beq         .ungetc_label_58
/* @61 */ 	lda          #255
.ungetc_label_58:
/* @62 */ 	sta         __i2+1
/* @65 */ 	lda         __i2
/* @66 */ 	cmp          #10
/* @67 */ 	bne         .ungetc_label_81
/* @68 */ 	lda         __i2+1
/* @70 */ 	bne         .ungetc_label_81
/* @74 */ 	lda          #255
/* @75 */ 	sta         __i2
/* @77 */ 	sta         __i2+1
/* @78 */ 	lda         #__i2
/* @79 */ 	jsr         __result2
/* @80 */ 	bra         .ungetc_label_41
.ungetc_label_81:
/* @82 */ 	lda          #0
/* @84 */ 	ldy          #16
/* @85 */ 	sta         (__i1), Y
/* @88 */ 	clc         
/* @89 */ 	lda         __i1
/* @90 */ 	adc          #18
/* @91 */ 	sta         __i2
/* @92 */ 	lda         __i1+1
/* @93 */ 	adc          #0
/* @94 */ 	sta         __i2+1
/* @97 */ 	lda         __i1
/* @98 */ 	sta         __i3
/* @99 */ 	lda         __i1+1
/* @100 */ 	sta         __i3+1
/* @104 */ 	clc         
/* @105 */ 	lda         __i3
/* @106 */ 	adc          #15
/* @107 */ 	sta         __i4
/* @108 */ 	lda         __i3+1
/* @109 */ 	adc          #0
/* @110 */ 	sta         __i4+1
/* @115 */ 	dey         
/* @116 */ 	lda         (__i3), Y
/* @117 */ 	sta         __b0
/* @119 */ 	lda          #__i4
/* @121 */ 	jsr         __inc1
/* @126 */ 	clc         
/* @127 */ 	lda         __i2
/* @128 */ 	adc         __b0
/* @129 */ 	sta         __i3
/* @130 */ 	lda         __i2+1
/* @131 */ 	adc         __b0+1
/* @132 */ 	sta         __i3+1
/* @135 */ 	lda         __i0
/* @136 */ 	sta         __i2
/* @137 */ 	lda         __i0+1
/* @138 */ 	sta         __i2+1
/* @143 */ 	lda         __i2
/* @144 */ 	sta         (__i3)
/* @145 */ 	lda         #__i0
/* @146 */ 	jsr         __result2
/* @147 */ 	bra         .ungetc_label_41
.func_end_ungetc:
	.size ungetc, .func_end_ungetc-ungetc

	.data
	.section ".rodata", "aMS", @progbits
