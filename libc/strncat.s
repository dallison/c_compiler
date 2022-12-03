	.file   "/Users/dallison/Google Drive/c_compiler/libc/strncat.c"
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


	.global strncat
	.type strncat, @function

strncat:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @17 */ 	ldx          #0
	jsr          __arg_value2_i1			// dest
/* @21 */ 	ldx          #4
	jsr          __arg_value2_i2			// n
/* @25 */ 	ldx          #2
	jsr          __arg_value2_i3			// src
/* @26 */ 	lda         __i1
/* @27 */ 	sta         __i0
/* @29 */ 	lda         __i1+1
/* @30 */ 	sta         __i0+1
.strncat_label_31:
/* @34 */ 	lda         (__i0)
/* @40 */ 	sta         __i4
/* @42 */ 	and          #128
/* @44 */ 	beq         .strncat_label_43
/* @46 */ 	lda          #255
.strncat_label_43:
/* @47 */ 	sta         __i4+1
/* @49 */ 	lda         __i4
/* @50 */ 	ora         __i4+1
/* @52 */ 	beq         .strncat_label_57
/* @53 */ 	lda          #__i0
/* @55 */ 	jsr         __rinc21
/* @56 */ 	bra         .strncat_label_31
.strncat_label_57:
.strncat_label_58:
/* @63 */ 	lda         __i2
/* @64 */ 	sta         __i4
/* @65 */ 	lda         __i2+1
/* @66 */ 	sta         __i4+1
/* @67 */ 	lda          #__i2
/* @69 */ 	jsr         __rdec21
/* @74 */ 	ldx          #1
/* @75 */ 	lda          #0
/* @76 */ 	cmp         __i4+1
/* @77 */ 	bcc         .strncat_label_73
/* @78 */ 	bne         .strncat_label_72
/* @79 */ 	lda          #0
/* @80 */ 	cmp         __i4
/* @81 */ 	bcc         .strncat_label_73
.strncat_label_72:
/* @82 */ 	dex         
.strncat_label_73:
/* @83 */ 	stx         __b0
/* @85 */ 	txa         
/* @86 */ 	cmp          #0
/* @87 */ 	beq         .strncat_label_113
/* @90 */ 	lda         (__i3)
/* @96 */ 	sta         __i4
/* @97 */ 	and          #128
/* @99 */ 	beq         .strncat_label_98
/* @100 */ 	lda          #255
.strncat_label_98:
/* @101 */ 	sta         __i4+1
/* @106 */ 	ldx          #1
/* @107 */ 	lda         __i4
/* @108 */ 	ora         __i4+1
/* @110 */ 	bne         .strncat_label_105
/* @111 */ 	dex         
.strncat_label_105:
/* @112 */ 	stx         __b0
.strncat_label_113:
/* @115 */ 	lda         __b0
/* @117 */ 	beq         .strncat_label_147
/* @120 */ 	lda         __i0
/* @121 */ 	sta         __i4
/* @122 */ 	lda         __i0+1
/* @123 */ 	sta         __i4+1
/* @124 */ 	lda          #__i0
/* @125 */ 	jsr         __rinc21
/* @128 */ 	lda         __i3
/* @129 */ 	sta         __i5
/* @130 */ 	lda         __i3+1
/* @131 */ 	sta         __i5+1
/* @132 */ 	lda          #__i3
/* @133 */ 	jsr         __rinc21
/* @138 */ 	lda         (__i5)
/* @145 */ 	sta         (__i4)
/* @146 */ 	bra         .strncat_label_58
.strncat_label_147:
/* @148 */ 	lda          #0
/* @149 */ 	tay         
/* @150 */ 	sta         (__i0)
/* @151 */ 	lda         #__i1
/* @153 */ 	jsr         __result2
/* @155 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_strncat:
	.size strncat, .func_end_strncat-strncat

	.data
	.section ".rodata", "aMS", @progbits
