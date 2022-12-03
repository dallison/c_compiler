	.file   "/Users/dallison/Google Drive/c_compiler/libc/fputc.c"
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


	.global fputc
	.type fputc, @function

fputc:
/* @13 */ 	stx         __result
/* @15 */ 	sty         __result+1
/* @16 */ 	ldx          #8
	jsr          __enter
	.byte        0x64,0x00,0x00		// Save mask i:4 b:3 l:0 x:0 f:0 
/* @22 */ 	ldx          #2
	jsr          __arg_value2_i4			// stream
/* @28 */ 	ldx          #0
	jsr          __arg_value1_b3			// c
/* @36 */ 	ldy          #2
/* @37 */ 	lda         (__i4), Y
/* @39 */ 	sta         __i0
/* @41 */ 	iny         
/* @42 */ 	lda         (__i4), Y
/* @44 */ 	sta         __i0+1
/* @47 */ 	lda         __i0
/* @49 */ 	bne         .fputc_label_119
/* @50 */ 	lda         __i0+1
/* @52 */ 	bne         .fputc_label_119
/* @54 */ 	lda          #__b3
/* @56 */ 	ldx          #4
/* @58 */ 	jsr         __set_var_value1
/* @59 */ 	ldx          #1
/* @61 */ 	jsr         __pushxy0
/* @63 */ 	ldx          #4
	jsr          __var_addr_i0			// ch
/* @66 */ 	jsr         __pushi0
/* @69 */ 	lda         (__i4)
/* @70 */ 	sta         __i0
/* @71 */ 	ldy          #1
/* @72 */ 	lda         (__i4), Y
/* @73 */ 	sta         __i0+1
/* @76 */ 	jsr         __pushi0
/* @77 */ 	ldx         #__i5
/* @78 */ 	ldy          #0
/* @79 */ 	jsr         write
/* @81 */ 	jsr         __incsp6
/* @83 */ 	lda         __i5
/* @84 */ 	cmp          #1
/* @85 */ 	bne         .fputc_label_94
/* @86 */ 	lda         __i5+1
/* @88 */ 	bne         .fputc_label_94
/* @91 */ 	lda         __b3
/* @92 */ 	sta         __b2
/* @93 */ 	bra         .fputc_label_98
.fputc_label_94:
/* @96 */ 	lda          #255
/* @97 */ 	sta         __b2
.fputc_label_98:
/* @102 */ 	lda         __b2
/* @103 */ 	sta         __i0
/* @105 */ 	and          #128
/* @107 */ 	beq         .fputc_label_106
/* @109 */ 	lda          #255
.fputc_label_106:
/* @110 */ 	sta         __i0+1
/* @112 */ 	ldx          #9
	jsr          __load_result
/* @113 */ 	lda         #__i0
/* @115 */ 	jsr         __result2
.fputc_label_116:
/* @117 */ 	ldy          #11
	jmp          __leave
.fputc_label_119:
/* @122 */ 	ldy          #10
/* @123 */ 	lda         (__i4), Y
/* @124 */ 	sta         __i0
/* @126 */ 	iny         
/* @127 */ 	lda         (__i4), Y
/* @128 */ 	sta         __i0+1
/* @131 */ 	ldy          #4
/* @132 */ 	lda         (__i4), Y
/* @133 */ 	sta         __i1
/* @135 */ 	iny         
/* @136 */ 	lda         (__i4), Y
/* @137 */ 	sta         __i1+1
/* @141 */ 	lda         __i0
/* @142 */ 	cmp         __i1
/* @143 */ 	bne         .fputc_label_163
/* @144 */ 	lda         __i0+1
/* @145 */ 	cmp         __i1+1
/* @146 */ 	bne         .fputc_label_163
/* @148 */ 	jsr         __pushi4
/* @149 */ 	ldx         #__i6
/* @150 */ 	ldy          #0
/* @151 */ 	jsr         fflush
/* @153 */ 	jsr         __incsp2
/* @154 */ 	lda         __i6
/* @155 */ 	ora         __i6+1
/* @157 */ 	beq         .fputc_label_162
/* @158 */ 	ldx          #9
	jsr          __load_result
/* @159 */ 	lda         #__i6
/* @160 */ 	jsr         __result2
/* @161 */ 	bra         .fputc_label_116
.fputc_label_162:
.fputc_label_163:
/* @166 */ 	ldy          #2
/* @167 */ 	lda         (__i4), Y
/* @168 */ 	sta         __i0
/* @169 */ 	iny         
/* @170 */ 	lda         (__i4), Y
/* @171 */ 	sta         __i0+1
/* @174 */ 	lda         __i4
/* @175 */ 	sta         __i1
/* @176 */ 	lda         __i4+1
/* @177 */ 	sta         __i1+1
/* @181 */ 	clc         
/* @182 */ 	lda         __i1
/* @183 */ 	adc          #10
/* @184 */ 	sta         __i2
/* @185 */ 	lda         __i1+1
/* @186 */ 	adc          #0
/* @187 */ 	sta         __i2+1
/* @192 */ 	ldy          #10
/* @193 */ 	lda         (__i1), Y
/* @194 */ 	sta         __i3
/* @195 */ 	iny         
/* @196 */ 	lda         (__i1), Y
/* @197 */ 	sta         __i3+1
/* @199 */ 	lda          #__i2
/* @201 */ 	jsr         __inc21
/* @206 */ 	clc         
/* @207 */ 	lda         __i0
/* @208 */ 	adc         __i3
/* @209 */ 	sta         __i1
/* @210 */ 	lda         __i0+1
/* @211 */ 	adc         __i3+1
/* @212 */ 	sta         __i1+1
/* @215 */ 	lda         __b3
/* @216 */ 	sta         (__i1)
/* @222 */ 	sta         __i0
/* @223 */ 	and          #128
/* @225 */ 	beq         .fputc_label_224
/* @226 */ 	lda          #255
.fputc_label_224:
/* @227 */ 	sta         __i0+1
/* @232 */ 	ldx          #1
/* @233 */ 	lda         __i0
/* @234 */ 	cmp          #10
/* @235 */ 	bne         .fputc_label_230
/* @236 */ 	lda         __i0+1
/* @238 */ 	beq         .fputc_label_231
.fputc_label_230:
/* @239 */ 	dex         
.fputc_label_231:
/* @240 */ 	stx         __b4
/* @242 */ 	txa         
/* @243 */ 	cmp          #0
/* @244 */ 	beq         .fputc_label_268
/* @248 */ 	ldy          #12
/* @249 */ 	lda         (__i4), Y
/* @250 */ 	sta         __i0
/* @252 */ 	iny         
/* @253 */ 	lda         (__i4), Y
/* @254 */ 	sta         __i0+1
/* @259 */ 	ldx          #1
/* @260 */ 	lda         __i0
/* @261 */ 	cmp          #2
/* @262 */ 	bne         .fputc_label_257
/* @263 */ 	lda         __i0+1
/* @265 */ 	beq         .fputc_label_258
.fputc_label_257:
/* @266 */ 	dex         
.fputc_label_258:
/* @267 */ 	stx         __b4
.fputc_label_268:
/* @270 */ 	lda         __b4
/* @272 */ 	beq         .fputc_label_284
/* @273 */ 	jsr         __pushi4
/* @275 */ 	ldx         #__i7
/* @276 */ 	ldy          #0
/* @277 */ 	jsr         fflush
/* @278 */ 	jsr         __incsp2
/* @280 */ 	ldx          #9
	jsr          __load_result
/* @281 */ 	lda         #__i7
/* @282 */ 	jsr         __result2
/* @283 */ 	jmp         .fputc_label_116
.fputc_label_284:
/* @287 */ 	stz         __i0
/* @288 */ 	stz         __i0+1
/* @289 */ 	ldx          #9
	jsr          __load_result
/* @290 */ 	lda         #__i0
/* @291 */ 	jsr         __result2
/* @292 */ 	jmp         .fputc_label_116
.func_end_fputc:
	.size fputc, .func_end_fputc-fputc

	.global putchar
	.type putchar, @function

putchar:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @13 */ 	ldx          #0
	jsr          __arg_value1_b0			// c
/* @17 */ 	lda         stdout+0
/* @18 */ 	sta         __i0
/* @20 */ 	lda         stdout+1
/* @21 */ 	sta         __i0+1
/* @23 */ 	jsr         __pushi0
/* @24 */ 	lda         __b0
/* @26 */ 	jsr         __pusha
/* @28 */ 	ldx         #__i4
/* @29 */ 	ldy          #0
/* @30 */ 	jsr         fputc
/* @32 */ 	jsr         __incsp4
/* @34 */ 	ldx          #8
	jsr          __load_result
/* @35 */ 	lda         #__i4
/* @37 */ 	jsr         __result2
/* @39 */ 	ldy          #10
	jmp          __leave
.func_end_putchar:
	.size putchar, .func_end_putchar-putchar

	.data
	.section ".rodata", "aMS", @progbits
