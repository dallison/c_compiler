	.file   "/Users/dallison/Google Drive/c_compiler/libc/strchr.c"
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


	.global strchr
	.type strchr, @function

strchr:
/* @8 */ 	stx         __result
/* @10 */ 	sty         __result+1
/* @11 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x20,0x00,0x00		// Save mask i:0 b:1 l:0 x:0 f:0 
/* @15 */ 	ldx          #0
	jsr          __arg_value2_i1			// s
/* @19 */ 	ldx          #2
	jsr          __arg_value1_b0			// c
.strchr_label_20:
/* @25 */ 	lda         (__i1)
/* @31 */ 	sta         __i2
/* @33 */ 	and          #128
/* @35 */ 	beq         .strchr_label_34
/* @37 */ 	lda          #255
.strchr_label_34:
/* @39 */ 	sta         __i2+1
/* @44 */ 	ldx          #1
/* @45 */ 	lda         __i2
/* @46 */ 	ora         __i2+1
/* @48 */ 	bne         .strchr_label_43
/* @49 */ 	dex         
.strchr_label_43:
/* @50 */ 	stx         __b1
/* @52 */ 	txa         
/* @53 */ 	cmp          #0
/* @54 */ 	beq         .strchr_label_92
/* @57 */ 	lda         (__i1)
/* @63 */ 	sta         __i2
/* @64 */ 	and          #128
/* @66 */ 	beq         .strchr_label_65
/* @67 */ 	lda          #255
.strchr_label_65:
/* @68 */ 	sta         __i2+1
/* @71 */ 	lda         __b0
/* @72 */ 	sta         __i3
/* @73 */ 	and          #128
/* @75 */ 	beq         .strchr_label_74
/* @76 */ 	lda          #255
.strchr_label_74:
/* @77 */ 	sta         __i3+1
/* @83 */ 	ldx          #0
/* @84 */ 	lda         __i2
/* @85 */ 	cmp         __i3
/* @86 */ 	bne         .strchr_label_82
/* @87 */ 	lda         __i2+1
/* @88 */ 	cmp         __i3+1
/* @89 */ 	beq         .strchr_label_81
.strchr_label_82:
/* @90 */ 	inx         
.strchr_label_81:
/* @91 */ 	stx         __b1
.strchr_label_92:
/* @94 */ 	lda         __b1
/* @96 */ 	beq         .strchr_label_101
/* @97 */ 	lda          #__i1
/* @99 */ 	jsr         __rinc21
/* @100 */ 	bra         .strchr_label_20
.strchr_label_101:
/* @104 */ 	lda         (__i1)
/* @110 */ 	sta         __i2
/* @111 */ 	and          #128
/* @113 */ 	beq         .strchr_label_112
/* @114 */ 	lda          #255
.strchr_label_112:
/* @115 */ 	sta         __i2+1
/* @117 */ 	lda         __i2
/* @118 */ 	ora         __i2+1
/* @119 */ 	bne         .strchr_label_124
/* @121 */ 	stz         __i0
/* @122 */ 	stz         __i0+1
/* @123 */ 	bra         .strchr_label_130
.strchr_label_124:
/* @126 */ 	lda         __i1
/* @127 */ 	sta         __i0
/* @128 */ 	lda         __i1+1
/* @129 */ 	sta         __i0+1
.strchr_label_130:
/* @132 */ 	lda         #__i0
/* @134 */ 	jsr         __result2
/* @136 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_strchr:
	.size strchr, .func_end_strchr-strchr

	.data
	.section ".rodata", "aMS", @progbits
