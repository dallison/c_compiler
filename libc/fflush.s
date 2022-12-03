	.file   "/Users/dallison/Google Drive/c_compiler/libc/fflush.c"
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


	.global fflush
	.type fflush, @function

fflush:
/* @10 */ 	stx         __result
/* @12 */ 	sty         __result+1
/* @13 */ 	ldx          #7
	jsr          __enter
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i4			// stream
/* @27 */ 	ldy          #2
/* @28 */ 	lda         (__i4), Y
/* @30 */ 	sta         __i0
/* @32 */ 	iny         
/* @33 */ 	lda         (__i4), Y
/* @34 */ 	sta         __i0+1
/* @37 */ 	lda         __i0
/* @39 */ 	bne         .fflush_label_36
/* @40 */ 	lda         __i0+1
/* @42 */ 	beq         .fflush_label_133
.fflush_label_36:
/* @47 */ 	ldy          #10
/* @48 */ 	lda         (__i4), Y
/* @49 */ 	sta         __i0
/* @51 */ 	iny         
/* @52 */ 	lda         (__i4), Y
/* @53 */ 	sta         __i0+1
/* @56 */ 	lda         __i0
/* @57 */ 	sta         __i5
/* @58 */ 	lda         __i0+1
/* @59 */ 	sta         __i5+1
/* @60 */ 	dey         
/* @61 */ 	lda          #0
.fflush_label_62:
/* @64 */ 	sta         (__i4), Y
/* @65 */ 	iny         
/* @67 */ 	cpy          #12
/* @68 */ 	bne         .fflush_label_62
/* @69 */ 	jsr         __pushi5
/* @72 */ 	ldy          #2
/* @73 */ 	lda         (__i4), Y
/* @74 */ 	sta         __i0
/* @75 */ 	iny         
/* @76 */ 	lda         (__i4), Y
/* @77 */ 	sta         __i0+1
/* @80 */ 	jsr         __pushi0
/* @83 */ 	lda         (__i4)
/* @84 */ 	sta         __i0
/* @85 */ 	ldy          #1
/* @86 */ 	lda         (__i4), Y
/* @87 */ 	sta         __i0+1
/* @90 */ 	jsr         __pushi0
/* @91 */ 	ldx         #__i6
/* @92 */ 	ldy          #0
/* @93 */ 	jsr         write
/* @95 */ 	jsr         __incsp6
/* @97 */ 	lda          #0
/* @98 */ 	cmp         __i6
/* @100 */ 	sbc         __i6+1
/* @101 */ 	bvc         .fflush_label_96
/* @103 */ 	eor          #128
.fflush_label_96:
/* @104 */ 	bmi         .fflush_label_132
/* @105 */ 	lda         __i6
/* @106 */ 	ora         __i6+1
/* @107 */ 	bne         .fflush_label_113
/* @108 */ 	lda          #1
/* @110 */ 	ldy          #16
/* @111 */ 	sta         (__i4), Y
/* @112 */ 	bra         .fflush_label_118
.fflush_label_113:
/* @114 */ 	lda          #1
/* @116 */ 	ldy          #17
/* @117 */ 	sta         (__i4), Y
.fflush_label_118:
/* @121 */ 	lda          #255
/* @122 */ 	sta         __i0
/* @124 */ 	sta         __i0+1
/* @125 */ 	ldx          #8
	jsr          __load_result
/* @126 */ 	lda         #__i0
/* @128 */ 	jsr         __result2
.fflush_label_129:
/* @130 */ 	ldy          #10
	jmp          __leave
.fflush_label_132:
.fflush_label_133:
/* @136 */ 	stz         __i0
/* @137 */ 	stz         __i0+1
/* @138 */ 	ldx          #8
	jsr          __load_result
/* @139 */ 	lda         #__i0
/* @140 */ 	jsr         __result2
/* @141 */ 	bra         .fflush_label_129
.func_end_fflush:
	.size fflush, .func_end_fflush-fflush

	.data
	.section ".rodata", "aMS", @progbits
