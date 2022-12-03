	.file   "/Users/dallison/Google Drive/c_compiler/libc/strspn.c"
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


	.global strspn
	.type strspn, @function

strspn:
/* @8 */ 	stx         __result
/* @10 */ 	sty         __result+1
/* @11 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i1			// s1
/* @26 */ 	ldx          #2
	jsr          __arg_value2_i3			// s2
/* @27 */ 	stz         __i0
/* @28 */ 	stz         __i0+1
.strspn_label_29:
/* @32 */ 	lda         (__i1)
/* @38 */ 	sta         __i4
/* @40 */ 	and          #128
/* @42 */ 	beq         .strspn_label_41
/* @44 */ 	lda          #255
.strspn_label_41:
/* @45 */ 	sta         __i4+1
/* @47 */ 	lda         __i4
/* @48 */ 	ora         __i4+1
/* @50 */ 	beq         .strspn_label_142
/* @51 */ 	stz         __b0
/* @52 */ 	lda         __i3
/* @53 */ 	sta         __i2
/* @54 */ 	lda         __i3+1
/* @55 */ 	sta         __i2+1
.strspn_label_56:
/* @59 */ 	lda         (__i2)
/* @65 */ 	sta         __i4
/* @66 */ 	and          #128
/* @68 */ 	beq         .strspn_label_67
/* @69 */ 	lda          #255
.strspn_label_67:
/* @70 */ 	sta         __i4+1
/* @72 */ 	lda         __i4
/* @73 */ 	ora         __i4+1
/* @75 */ 	beq         .strspn_label_123
/* @78 */ 	lda         (__i1)
/* @84 */ 	sta         __i4
/* @85 */ 	and          #128
/* @87 */ 	beq         .strspn_label_86
/* @88 */ 	lda          #255
.strspn_label_86:
/* @89 */ 	sta         __i4+1
/* @92 */ 	lda         (__i2)
/* @98 */ 	sta         __i5
/* @99 */ 	and          #128
/* @101 */ 	beq         .strspn_label_100
/* @102 */ 	lda          #255
.strspn_label_100:
/* @103 */ 	sta         __i5+1
/* @107 */ 	lda         __i4
/* @108 */ 	cmp         __i5
/* @109 */ 	bne         .strspn_label_117
/* @110 */ 	lda         __i4+1
/* @111 */ 	cmp         __i5+1
/* @112 */ 	bne         .strspn_label_117
/* @114 */ 	lda          #1
/* @115 */ 	sta         __b0
/* @116 */ 	bra         .strspn_label_123
.strspn_label_117:
/* @119 */ 	lda          #__i2
/* @121 */ 	jsr         __rinc21
/* @122 */ 	bra         .strspn_label_56
.strspn_label_123:
/* @127 */ 	lda         __b0
/* @129 */ 	beq         .strspn_label_126
/* @130 */ 	lda          #255
.strspn_label_126:
/* @131 */ 	inc          A
/* @135 */ 	bne         .strspn_label_142
/* @137 */ 	lda          #__i0
/* @138 */ 	jsr         __rinc21
/* @139 */ 	lda          #__i1
/* @140 */ 	jsr         __rinc21
/* @141 */ 	bra         .strspn_label_29
.strspn_label_142:
/* @143 */ 	lda         #__i0
/* @145 */ 	jsr         __result2
/* @147 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_strspn:
	.size strspn, .func_end_strspn-strspn

	.data
	.section ".rodata", "aMS", @progbits
