	.file   "/Users/dallison/Google Drive/c_compiler/libc/strstr.c"
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


	.global strstr
	.type strstr, @function

strstr:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i1			// haystack
/* @22 */ 	ldx          #2
	jsr          __arg_value2_i3			// needle
/* @25 */ 	lda         __i1
/* @26 */ 	sta         __i0
/* @28 */ 	lda         __i1+1
/* @29 */ 	sta         __i0+1
/* @30 */ 	lda         __i3
/* @31 */ 	sta         __i2
/* @32 */ 	lda         __i3+1
/* @33 */ 	sta         __i2+1
.strstr_label_34:
/* @37 */ 	lda         (__i0)
/* @43 */ 	sta         __i1
/* @45 */ 	and          #128
/* @47 */ 	beq         .strstr_label_46
/* @49 */ 	lda          #255
.strstr_label_46:
/* @50 */ 	sta         __i1+1
/* @52 */ 	lda         __i1
/* @53 */ 	ora         __i1+1
/* @55 */ 	bne         .strstr_label_192
/* @193 */ 	jmp         .strstr_label_184
.strstr_label_192:
/* @56 */ 	lda         __i0
/* @57 */ 	sta         __i4
/* @58 */ 	lda         __i0+1
/* @59 */ 	sta         __i4+1
.strstr_label_60:
/* @65 */ 	lda         __i0
/* @66 */ 	sta         __i1
/* @67 */ 	lda         __i0+1
/* @68 */ 	sta         __i1+1
/* @69 */ 	lda          #__i0
/* @71 */ 	jsr         __rinc21
/* @76 */ 	lda         (__i1)
/* @82 */ 	sta         __i1
/* @83 */ 	and          #128
/* @85 */ 	beq         .strstr_label_84
/* @86 */ 	lda          #255
.strstr_label_84:
/* @87 */ 	sta         __i1+1
/* @90 */ 	lda         (__i2)
/* @96 */ 	sta         __i5
/* @97 */ 	and          #128
/* @99 */ 	beq         .strstr_label_98
/* @100 */ 	lda          #255
.strstr_label_98:
/* @101 */ 	sta         __i5+1
/* @107 */ 	ldx          #1
/* @108 */ 	lda         __i1
/* @109 */ 	cmp         __i5
/* @110 */ 	bne         .strstr_label_105
/* @111 */ 	lda         __i1+1
/* @112 */ 	cmp         __i5+1
/* @113 */ 	beq         .strstr_label_106
.strstr_label_105:
/* @114 */ 	dex         
.strstr_label_106:
/* @115 */ 	stx         __b0
/* @117 */ 	txa         
/* @118 */ 	cmp          #0
/* @119 */ 	beq         .strstr_label_145
/* @122 */ 	lda         (__i2)
/* @128 */ 	sta         __i1
/* @129 */ 	and          #128
/* @131 */ 	beq         .strstr_label_130
/* @132 */ 	lda          #255
.strstr_label_130:
/* @133 */ 	sta         __i1+1
/* @138 */ 	ldx          #1
/* @139 */ 	lda         __i1
/* @140 */ 	ora         __i1+1
/* @142 */ 	bne         .strstr_label_137
/* @143 */ 	dex         
.strstr_label_137:
/* @144 */ 	stx         __b0
.strstr_label_145:
/* @147 */ 	lda         __b0
/* @149 */ 	beq         .strstr_label_153
/* @150 */ 	lda          #__i2
/* @151 */ 	jsr         __rinc21
/* @152 */ 	bra         .strstr_label_60
.strstr_label_153:
/* @156 */ 	lda         (__i2)
/* @162 */ 	sta         __i1
/* @163 */ 	and          #128
/* @165 */ 	beq         .strstr_label_164
/* @166 */ 	lda          #255
.strstr_label_164:
/* @167 */ 	sta         __i1+1
/* @169 */ 	lda         __i1
/* @170 */ 	ora         __i1+1
/* @171 */ 	bne         .strstr_label_178
/* @172 */ 	lda         #__i4
/* @174 */ 	jsr         __result2
.strstr_label_175:
/* @176 */ 	ldy          #8
	jmp          __leave_leaf
.strstr_label_178:
/* @179 */ 	lda         __i3
/* @180 */ 	sta         __i2
/* @181 */ 	lda         __i3+1
/* @182 */ 	sta         __i2+1
/* @183 */ 	jmp         .strstr_label_34
.strstr_label_184:
/* @187 */ 	stz         __i1
/* @188 */ 	stz         __i1+1
/* @189 */ 	lda         #__i1
/* @190 */ 	jsr         __result2
/* @191 */ 	bra         .strstr_label_175
.func_end_strstr:
	.size strstr, .func_end_strstr-strstr

	.data
	.section ".rodata", "aMS", @progbits
