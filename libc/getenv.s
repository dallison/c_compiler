	.file   "/Users/dallison/Google Drive/c_compiler/libc/getenv.c"
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


	.global getenv
	.type getenv, @function

getenv:
/* @12 */ 	stx         __result
/* @14 */ 	sty         __result+1
/* @15 */ 	ldx          #7
	jsr          __enter
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
/* @30 */ 	ldx          #0
	jsr          __arg_value2_i4			// var
/* @31 */ 	lda         environ+0
/* @32 */ 	sta         __i0
/* @34 */ 	lda         environ+1
/* @35 */ 	sta         __i0+1
.getenv_label_36:
/* @38 */ 	lda         __i0
/* @40 */ 	bne         .getenv_label_37
/* @41 */ 	lda         __i0+1
/* @43 */ 	bne         .getenv_label_248
/* @249 */ 	jmp         .getenv_label_239
.getenv_label_248:
.getenv_label_37:
/* @47 */ 	lda         __i0
/* @48 */ 	sta         __i5
/* @49 */ 	lda         __i0+1
/* @50 */ 	sta         __i5+1
/* @51 */ 	lda          #__i0
/* @53 */ 	ldx          #2
/* @55 */ 	jsr         __rinc2
/* @60 */ 	lda         (__i5)
/* @61 */ 	sta         __i6
/* @62 */ 	ldy          #1
/* @63 */ 	lda         (__i5), Y
/* @64 */ 	sta         __i6+1
/* @67 */ 	lda         __i6
/* @68 */ 	sta         __i1
/* @69 */ 	lda         __i6+1
/* @70 */ 	sta         __i1+1
/* @71 */ 	lda         __i1
/* @72 */ 	sta         __i2
/* @73 */ 	lda         __i1+1
/* @74 */ 	sta         __i2+1
.getenv_label_75:
/* @80 */ 	lda         (__i2)
/* @86 */ 	sta         __i5
/* @88 */ 	and          #128
/* @90 */ 	beq         .getenv_label_89
/* @92 */ 	lda          #255
.getenv_label_89:
/* @93 */ 	sta         __i5+1
/* @98 */ 	ldx          #1
/* @99 */ 	lda         __i5
/* @100 */ 	ora         __i5+1
/* @102 */ 	bne         .getenv_label_97
/* @103 */ 	dex         
.getenv_label_97:
/* @104 */ 	stx         __b0
/* @106 */ 	txa         
/* @107 */ 	cmp          #0
/* @108 */ 	beq         .getenv_label_136
/* @111 */ 	lda         (__i2)
/* @117 */ 	sta         __i5
/* @118 */ 	and          #128
/* @120 */ 	beq         .getenv_label_119
/* @121 */ 	lda          #255
.getenv_label_119:
/* @122 */ 	sta         __i5+1
/* @127 */ 	ldx          #0
/* @128 */ 	lda         __i5
/* @129 */ 	cmp          #61
/* @130 */ 	bne         .getenv_label_126
/* @131 */ 	lda         __i5+1
/* @133 */ 	beq         .getenv_label_125
.getenv_label_126:
/* @134 */ 	inx         
.getenv_label_125:
/* @135 */ 	stx         __b0
.getenv_label_136:
/* @138 */ 	lda         __b0
/* @140 */ 	beq         .getenv_label_145
/* @141 */ 	lda          #__i2
/* @143 */ 	jsr         __rinc21
/* @144 */ 	bra         .getenv_label_75
.getenv_label_145:
/* @148 */ 	lda         (__i2)
/* @154 */ 	sta         __i5
/* @155 */ 	and          #128
/* @157 */ 	beq         .getenv_label_156
/* @158 */ 	lda          #255
.getenv_label_156:
/* @159 */ 	sta         __i5+1
/* @162 */ 	lda         __i5
/* @163 */ 	cmp          #61
/* @164 */ 	bne         .getenv_label_171
/* @165 */ 	lda         __i5+1
/* @167 */ 	bne         .getenv_label_171
/* @169 */ 	lda          #__i2
/* @170 */ 	jsr         __rinc21
.getenv_label_171:
/* @174 */ 	sec         
/* @175 */ 	lda         __i2
/* @176 */ 	sbc         __i1
/* @177 */ 	sta         __i5
/* @178 */ 	lda         __i2+1
/* @179 */ 	sbc         __i1+1
/* @180 */ 	sta         __i5+1
/* @184 */ 	sec         
/* @187 */ 	ldy          #4
/* @188 */ 	ldx          #0
.getenv_label_189:
/* @191 */ 	lda         __i5, X
/* @192 */ 	sbc         .lit.4, X
/* @193 */ 	sta         __l0, X
/* @194 */ 	inx         
/* @195 */ 	dey         
/* @196 */ 	bne         .getenv_label_189
/* @199 */ 	lda         __l0
/* @200 */ 	sta         __i3
/* @201 */ 	lda         __l0+1
/* @202 */ 	sta         __i3+1
/* @204 */ 	lda         __i3
/* @206 */ 	sta         __mem_size
/* @207 */ 	lda         __i3+1
/* @209 */ 	sta         __mem_size+1
/* @210 */ 	lda         __i1
/* @212 */ 	sta         __mem_src
/* @213 */ 	lda         __i1+1
/* @215 */ 	sta         __mem_src+1
/* @216 */ 	lda         __i4
/* @218 */ 	sta         __mem_dest
/* @219 */ 	lda         __i4+1
/* @221 */ 	sta         __mem_dest+1
/* @223 */ 	jsr         __builtin_memcmp
/* @224 */ 	stx         __i5
/* @225 */ 	sty         __i5+1
/* @227 */ 	txa         
/* @228 */ 	ora         __i5+1
/* @229 */ 	bne         .getenv_label_237
/* @230 */ 	ldx          #8
	jsr          __load_result
/* @231 */ 	lda         #__i2
/* @233 */ 	jsr         __result2
.getenv_label_234:
/* @235 */ 	ldy          #10
	jmp          __leave
.getenv_label_237:
/* @238 */ 	jmp         .getenv_label_36
.getenv_label_239:
/* @242 */ 	stz         __i5
/* @243 */ 	stz         __i5+1
/* @244 */ 	ldx          #8
	jsr          __load_result
/* @245 */ 	lda         #__i5
/* @246 */ 	jsr         __result2
/* @247 */ 	bra         .getenv_label_234
.func_end_getenv:
	.size getenv, .func_end_getenv-getenv

	.data
	.type   environ,@object
	.global environ
	.comm   environ,2,1

	.section ".rodata", "aMS", @progbits
.lit.4:
	.byte 0x01
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.type .lit.4, @object
	.size .lit.4, 4

