	.file   "/Users/dallison/Google Drive/c_compiler/libc/strpbrk.c"
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


	.global strpbrk
	.type strpbrk, @function

strpbrk:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @13 */ 	ldx          #0
	jsr          __arg_value2_i0			// s1
/* @20 */ 	ldx          #2
	jsr          __arg_value2_i2			// s2
.strpbrk_label_21:
/* @24 */ 	lda         (__i0)
/* @30 */ 	sta         __i3
/* @32 */ 	and          #128
/* @34 */ 	beq         .strpbrk_label_33
/* @36 */ 	lda          #255
.strpbrk_label_33:
/* @38 */ 	sta         __i3+1
/* @40 */ 	lda         __i3
/* @41 */ 	ora         __i3+1
/* @43 */ 	beq         .strpbrk_label_122
/* @44 */ 	lda         __i2
/* @45 */ 	sta         __i1
/* @46 */ 	lda         __i2+1
/* @47 */ 	sta         __i1+1
.strpbrk_label_48:
/* @51 */ 	lda         (__i1)
/* @57 */ 	sta         __i3
/* @58 */ 	and          #128
/* @60 */ 	beq         .strpbrk_label_59
/* @61 */ 	lda          #255
.strpbrk_label_59:
/* @62 */ 	sta         __i3+1
/* @64 */ 	lda         __i3
/* @65 */ 	ora         __i3+1
/* @67 */ 	beq         .strpbrk_label_118
/* @70 */ 	lda         (__i0)
/* @76 */ 	sta         __i3
/* @77 */ 	and          #128
/* @79 */ 	beq         .strpbrk_label_78
/* @80 */ 	lda          #255
.strpbrk_label_78:
/* @81 */ 	sta         __i3+1
/* @84 */ 	lda         (__i1)
/* @90 */ 	sta         __i4
/* @91 */ 	and          #128
/* @93 */ 	beq         .strpbrk_label_92
/* @94 */ 	lda          #255
.strpbrk_label_92:
/* @95 */ 	sta         __i4+1
/* @99 */ 	lda         __i3
/* @100 */ 	cmp         __i4
/* @101 */ 	bne         .strpbrk_label_112
/* @102 */ 	lda         __i3+1
/* @103 */ 	cmp         __i4+1
/* @104 */ 	bne         .strpbrk_label_112
/* @106 */ 	lda         #__i0
/* @108 */ 	jsr         __result2
.strpbrk_label_109:
/* @110 */ 	ldy          #8
	jmp          __leave_leaf
.strpbrk_label_112:
/* @114 */ 	lda          #__i1
/* @116 */ 	jsr         __rinc21
/* @117 */ 	bra         .strpbrk_label_48
.strpbrk_label_118:
/* @119 */ 	lda          #__i0
/* @120 */ 	jsr         __rinc21
/* @121 */ 	bra         .strpbrk_label_21
.strpbrk_label_122:
/* @125 */ 	stz         __i3
/* @126 */ 	stz         __i3+1
/* @127 */ 	lda         #__i3
/* @128 */ 	jsr         __result2
/* @129 */ 	bra         .strpbrk_label_109
.func_end_strpbrk:
	.size strpbrk, .func_end_strpbrk-strpbrk

	.data
	.section ".rodata", "aMS", @progbits
