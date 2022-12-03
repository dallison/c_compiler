	.file   "/Users/dallison/Google Drive/c_compiler/libc/ftoa.c"
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


	.global FixFloat
	.type FixFloat, @function

FixFloat:
/* @16 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x20,0x00		// Save mask i:1 b:0 l:0 x:1 f:0 
/* @28 */ 	ldx          #0
	jsr          __arg_value2_i4			// printer
/* @30 */ 	ldx          #2
	jsr          __arg_addr_i1			// f
/* @33 */ 	lda         __i1
/* @34 */ 	sta         __i0
/* @35 */ 	lda         __i1+1
/* @36 */ 	sta         __i0+1
/* @37 */ 	jsr         __pushi4
/* @38 */ 	jsr         __pushi0
/* @39 */ 	jsr         __unpackIEEE754
/* @41 */ 	jsr         __incsp4
/* @42 */ 	lda          #0
/* @44 */ 	ldy          #6
/* @45 */ 	sta         (__i4), Y
/* @48 */ 	ldy          #1
/* @49 */ 	lda         (__i4), Y
/* @55 */ 	sta         __i0
/* @57 */ 	stz         __i0+1
/* @61 */ 	cmp          #255
/* @62 */ 	bne         .FixFloat_label_122
/* @63 */ 	lda         __i0+1
/* @65 */ 	bne         .FixFloat_label_122
/* @70 */ 	ldy          #5
/* @72 */ 	ldx          #3
.FixFloat_label_73:
/* @75 */ 	lda         (__i4), Y
/* @76 */ 	sta         __l0, X
/* @77 */ 	dey         
/* @78 */ 	dex         
/* @79 */ 	bpl         .FixFloat_label_73
/* @82 */ 	lda         __l0
/* @84 */ 	bne         .FixFloat_label_98
/* @85 */ 	lda         __l0+1
/* @86 */ 	cmp          #255
/* @87 */ 	bne         .FixFloat_label_98
/* @88 */ 	lda         __l0+2
/* @89 */ 	cmp          #255
/* @90 */ 	bne         .FixFloat_label_98
/* @91 */ 	lda         __l0+3
/* @92 */ 	cmp          #255
/* @93 */ 	bne         .FixFloat_label_98
/* @95 */ 	lda          #1
/* @96 */ 	ldy          #6
/* @97 */ 	sta         (__i4), Y
.FixFloat_label_98:
/* @101 */ 	ldy          #5
/* @102 */ 	ldx          #3
.FixFloat_label_103:
/* @104 */ 	lda         (__i4), Y
/* @105 */ 	sta         __l0, X
/* @106 */ 	dey         
/* @107 */ 	dex         
/* @108 */ 	bpl         .FixFloat_label_103
/* @110 */ 	lda         __l0
/* @111 */ 	ora         __l0+1
/* @112 */ 	ora         __l0+2
/* @113 */ 	ora         __l0+3
/* @114 */ 	bne         .FixFloat_label_118
/* @115 */ 	lda          #2
/* @116 */ 	ldy          #6
/* @117 */ 	sta         (__i4), Y
.FixFloat_label_118:
.FixFloat_label_119:
/* @120 */ 	ldy          #10
	jmp          __leave_void
.FixFloat_label_122:
/* @125 */ 	ldy          #1
/* @126 */ 	lda         (__i4), Y
/* @134 */ 	stz         __i0+1
/* @138 */ 	bne         .FixFloat_label_140
/* @139 */ 	bra         .FixFloat_label_119
.FixFloat_label_140:
/* @143 */ 	ldy          #5
/* @144 */ 	ldx          #3
.FixFloat_label_145:
/* @146 */ 	lda         (__i4), Y
/* @147 */ 	sta         __l0, X
/* @148 */ 	dey         
/* @149 */ 	dex         
/* @150 */ 	bpl         .FixFloat_label_145
/* @154 */ 	lda         __l0
/* @155 */ 	sta         __x0
/* @156 */ 	lda         __l0+1
/* @157 */ 	sta         __x0+1
/* @158 */ 	lda         __l0+2
/* @159 */ 	sta         __x0+2
/* @160 */ 	lda         __l0+3
/* @161 */ 	sta         __x0+3
/* @162 */ 	lda          #0
/* @164 */ 	ldy          #7
.FixFloat_label_165:
/* @166 */ 	sta         __x0, Y
/* @167 */ 	dey         
/* @168 */ 	cpy          #3
/* @169 */ 	bne         .FixFloat_label_165
/* @173 */ 	lda         __x0+3
/* @174 */ 	sta         __x1+7
/* @175 */ 	lda         __x0+2
/* @176 */ 	sta         __x1+6
/* @177 */ 	lda         __x0+1
/* @178 */ 	sta         __x1+5
/* @179 */ 	lda         __x0
/* @181 */ 	sta         __x1+4
/* @183 */ 	stz         __x1
/* @184 */ 	sta         __x1+1
/* @185 */ 	sta         __x1+2
/* @186 */ 	sta         __x1+3
/* @190 */ 	ldy          #22
/* @191 */ 	ldx          #7
.FixFloat_label_192:
/* @193 */ 	lda         __x1, X
/* @194 */ 	sta         (__i4), Y
/* @195 */ 	dey         
/* @196 */ 	dex         
/* @197 */ 	bpl         .FixFloat_label_192
/* @200 */ 	ldy          #1
/* @201 */ 	lda         (__i4), Y
/* @207 */ 	sta         __i0
/* @209 */ 	stz         __i0+1
/* @212 */ 	lda         __i0+1
/* @213 */ 	cmp          #0
/* @214 */ 	bcc         .FixFloat_label_211
/* @215 */ 	bne         .FixFloat_label_274
/* @216 */ 	lda         __i0
/* @217 */ 	cmp          #127
/* @218 */ 	bcs         .FixFloat_label_274
.FixFloat_label_211:
.FixFloat_label_220:
/* @223 */ 	ldy          #1
/* @224 */ 	lda         (__i4), Y
/* @230 */ 	sta         __i0
/* @232 */ 	stz         __i0+1
/* @235 */ 	lda         __i0+1
/* @236 */ 	cmp          #0
/* @237 */ 	bcc         .FixFloat_label_234
/* @238 */ 	bne         .FixFloat_label_272
/* @239 */ 	lda         __i0
/* @240 */ 	cmp          #126
/* @241 */ 	bcs         .FixFloat_label_272
.FixFloat_label_234:
/* @245 */ 	clc         
/* @246 */ 	lda         __i4
/* @247 */ 	adc          #7
/* @248 */ 	sta         __i0
/* @249 */ 	lda         __i4+1
/* @250 */ 	adc          #0
/* @251 */ 	sta         __i0+1
/* @254 */ 	jsr         __pushi0
/* @255 */ 	jsr         __RShift
/* @257 */ 	jsr         __incsp2
/* @260 */ 	clc         
/* @261 */ 	lda         __i4
/* @262 */ 	adc          #1
/* @263 */ 	sta         __i0
/* @264 */ 	lda         __i4+1
/* @265 */ 	adc          #0
/* @266 */ 	sta         __i0+1
/* @268 */ 	lda          #__i0
/* @270 */ 	jsr         __inc1
/* @271 */ 	bra         .FixFloat_label_220
.FixFloat_label_272:
/* @273 */ 	bra         .FixFloat_label_327
.FixFloat_label_274:
.FixFloat_label_275:
/* @278 */ 	ldy          #1
/* @279 */ 	lda         (__i4), Y
/* @285 */ 	sta         __i0
/* @287 */ 	stz         __i0+1
/* @290 */ 	lda         __i0+1
/* @291 */ 	cmp          #0
/* @292 */ 	bcc         .FixFloat_label_326
/* @293 */ 	bne         .FixFloat_label_289
/* @294 */ 	lda         __i0
/* @295 */ 	cmp          #127
/* @296 */ 	bcc         .FixFloat_label_326
.FixFloat_label_289:
/* @300 */ 	clc         
/* @301 */ 	lda         __i4
/* @302 */ 	adc          #7
/* @303 */ 	sta         __i0
/* @304 */ 	lda         __i4+1
/* @305 */ 	adc          #0
/* @306 */ 	sta         __i0+1
/* @309 */ 	jsr         __pushi0
/* @310 */ 	jsr         __LShift
/* @311 */ 	jsr         __incsp2
/* @314 */ 	clc         
/* @315 */ 	lda         __i4
/* @316 */ 	adc          #1
/* @317 */ 	sta         __i0
/* @318 */ 	lda         __i4+1
/* @319 */ 	adc          #0
/* @320 */ 	sta         __i0+1
/* @322 */ 	lda          #__i0
/* @324 */ 	jsr         __dec1
/* @325 */ 	bra         .FixFloat_label_275
.FixFloat_label_326:
.FixFloat_label_327:
/* @328 */ 	jmp         .FixFloat_label_119
.func_end_FixFloat:
	.size FixFloat, .func_end_FixFloat-FixFloat

	.global WriteZero
	.type WriteZero, @function

WriteZero:
/* @8 */ 	stx         __result
/* @10 */ 	sty         __result+1
/* @11 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
/* @18 */ 	ldx          #2
	jsr          __arg_value2_i1			// buf
/* @22 */ 	ldx          #4
	jsr          __arg_value2_i2			// size
/* @28 */ 	ldx          #0
	jsr          __arg_value2_i4			// precision
/* @31 */ 	clc         
/* @32 */ 	lda         __i1
/* @33 */ 	adc         __i2
/* @34 */ 	sta         __i5
/* @36 */ 	lda         __i1+1
/* @37 */ 	adc         __i2+1
/* @38 */ 	sta         __i5+1
/* @42 */ 	sec         
/* @43 */ 	lda         __i5
/* @44 */ 	sbc          #1
/* @45 */ 	sta         __i6
/* @46 */ 	lda         __i5+1
/* @47 */ 	sbc          #0
/* @48 */ 	sta         __i6+1
/* @51 */ 	lda         __i6
/* @52 */ 	sta         __i0
/* @53 */ 	lda         __i6+1
/* @54 */ 	sta         __i0+1
/* @55 */ 	lda         __i0
/* @56 */ 	sta         __i3
/* @57 */ 	lda         __i0+1
/* @58 */ 	sta         __i3+1
/* @61 */ 	lda         __i3
/* @62 */ 	sta         __i5
/* @63 */ 	lda         __i3+1
/* @64 */ 	sta         __i5+1
/* @65 */ 	lda          #__i3
/* @67 */ 	jsr         __rdec21
/* @70 */ 	lda          #0
/* @71 */ 	tay         
/* @72 */ 	sta         (__i5)
.WriteZero_label_73:
/* @75 */ 	lda          #0
/* @76 */ 	cmp         __i4
/* @78 */ 	sbc         __i4+1
/* @79 */ 	bvc         .WriteZero_label_74
/* @81 */ 	eor          #128
.WriteZero_label_74:
/* @82 */ 	bpl         .WriteZero_label_99
/* @85 */ 	lda         __i3
/* @86 */ 	sta         __i0
/* @87 */ 	lda         __i3+1
/* @88 */ 	sta         __i0+1
/* @89 */ 	lda          #__i3
/* @90 */ 	jsr         __rdec21
/* @93 */ 	lda          #48
/* @94 */ 	ldy          #0
/* @95 */ 	sta         (__i0)
/* @96 */ 	lda          #__i4
/* @97 */ 	jsr         __rdec21
/* @98 */ 	bra         .WriteZero_label_73
.WriteZero_label_99:
/* @102 */ 	lda         __i3
/* @103 */ 	sta         __i0
/* @104 */ 	lda         __i3+1
/* @105 */ 	sta         __i0+1
/* @106 */ 	lda          #__i3
/* @107 */ 	jsr         __rdec21
/* @110 */ 	lda          #46
/* @111 */ 	ldy          #0
/* @112 */ 	sta         (__i0)
/* @115 */ 	lda         __i3
/* @116 */ 	sta         __i0
/* @117 */ 	lda         __i3+1
/* @118 */ 	sta         __i0+1
/* @119 */ 	lda          #__i3
/* @120 */ 	jsr         __rdec21
/* @123 */ 	lda          #48
/* @124 */ 	ldy          #0
/* @125 */ 	sta         (__i0)
/* @128 */ 	clc         
/* @129 */ 	lda         __i3
/* @130 */ 	adc          #1
/* @131 */ 	sta         __i0
/* @132 */ 	lda         __i3+1
/* @133 */ 	adc          #0
/* @134 */ 	sta         __i0+1
/* @136 */ 	lda         #__i0
/* @138 */ 	jsr         __result2
/* @140 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_WriteZero:
	.size WriteZero, .func_end_WriteZero-WriteZero

	.global WriteNanInf
	.type WriteNanInf, @function

WriteNanInf:
/* @11 */ 	stx         __result
/* @13 */ 	sty         __result+1
/* @14 */ 	ldx          #7
	jsr          __enter
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
/* @22 */ 	ldx          #4
	jsr          __arg_value2_i0			// buf
/* @26 */ 	ldx          #6
	jsr          __arg_value2_i1			// size
/* @32 */ 	ldx          #0
	jsr          __arg_value1_b0			// sign
/* @36 */ 	ldx          #2
	jsr          __arg_value2_i2			// v
/* @39 */ 	clc         
/* @41 */ 	lda         __i0
/* @42 */ 	adc         __i1
/* @43 */ 	sta         __i3
/* @45 */ 	lda         __i0+1
/* @46 */ 	adc         __i1+1
/* @47 */ 	sta         __i3+1
/* @51 */ 	sec         
/* @52 */ 	lda         __i3
/* @53 */ 	sbc          #4
/* @54 */ 	sta         __i7
/* @55 */ 	lda         __i3+1
/* @56 */ 	sbc          #0
/* @57 */ 	sta         __i7+1
/* @60 */ 	lda         __i7
/* @61 */ 	sta         __i5
/* @62 */ 	lda         __i7+1
/* @63 */ 	sta         __i5+1
/* @64 */ 	lda         __i5
/* @65 */ 	sta         __i6
/* @66 */ 	lda         __i5+1
/* @67 */ 	sta         __i6+1
/* @68 */ 	lda         __b0
/* @70 */ 	beq         .WriteNanInf_label_85
/* @73 */ 	lda         __i6
/* @74 */ 	sta         __i0
/* @75 */ 	lda         __i6+1
/* @76 */ 	sta         __i0+1
/* @77 */ 	lda          #__i6
/* @79 */ 	jsr         __rinc21
/* @82 */ 	lda          #45
/* @83 */ 	ldy          #0
/* @84 */ 	sta         (__i0)
.WriteNanInf_label_85:
/* @87 */ 	lda         __i2
/* @88 */ 	cmp          #1
/* @89 */ 	bne         .WriteNanInf_label_100
/* @90 */ 	lda         __i2+1
/* @92 */ 	bne         .WriteNanInf_label_100
/* @95 */ 	lda         #%lo(.str.17)
/* @96 */ 	sta         __i4
/* @97 */ 	lda         #%hi(.str.17)
/* @98 */ 	sta         __i4+1
/* @99 */ 	bra         .WriteNanInf_label_106
.WriteNanInf_label_100:
/* @102 */ 	lda         #%lo(.str.18)
/* @103 */ 	sta         __i4
/* @104 */ 	lda         #%hi(.str.18)
/* @105 */ 	sta         __i4+1
.WriteNanInf_label_106:
/* @109 */ 	jsr         __pushi4
/* @110 */ 	jsr         __pushi6
/* @112 */ 	ldx         #__i0
/* @113 */ 	ldy          #0
/* @114 */ 	jsr         strcpy
/* @116 */ 	jsr         __incsp4
/* @117 */ 	ldx          #8
	jsr          __load_result
/* @118 */ 	lda         #__i5
/* @120 */ 	jsr         __result2
/* @122 */ 	ldy          #10
	jmp          __leave
.func_end_WriteNanInf:
	.size WriteNanInf, .func_end_WriteNanInf-WriteNanInf

	.global PrintFixedPoint
	.type PrintFixedPoint, @function

PrintFixedPoint:
/* @19 */ 	stx         __result
/* @21 */ 	sty         __result+1
/* @22 */ 	ldx          #10
	jsr          __enter
	.byte        0x67,0x00,0x00		// Save mask i:7 b:3 l:0 x:0 f:0 
/* @36 */ 	ldx          #6
	jsr          __arg_value2_i4			// size
/* @40 */ 	ldx          #4
	jsr          __arg_value2_i5			// buf
/* @44 */ 	ldx          #2
	jsr          __arg_value2_i6			// precision
/* @58 */ 	ldx          #0
	jsr          __arg_value2_i9			// printer
/* @62 */ 	ldy          #6
/* @63 */ 	lda         (__i9), Y
/* @71 */ 	stz         __i0+1
/* @76 */ 	beq         .PrintFixedPoint_label_122
/* @77 */ 	jsr         __pushi4
/* @78 */ 	jsr         __pushi5
/* @81 */ 	ldy          #6
/* @82 */ 	lda         (__i9), Y
/* @88 */ 	sta         __i0
/* @90 */ 	and          #128
/* @92 */ 	beq         .PrintFixedPoint_label_91
/* @94 */ 	lda          #255
.PrintFixedPoint_label_91:
/* @95 */ 	sta         __i0+1
/* @98 */ 	jsr         __pushi0
/* @101 */ 	lda         (__i9)
/* @107 */ 	jsr         __pusha
/* @109 */ 	ldx         #__i10
/* @110 */ 	ldy          #0
/* @111 */ 	jsr         WriteNanInf
/* @113 */ 	jsr         __incsp8
/* @115 */ 	ldx          #11
	jsr          __load_result
/* @116 */ 	lda         #__i10
/* @118 */ 	jsr         __result2
.PrintFixedPoint_label_119:
/* @120 */ 	ldy          #13
	jmp          __leave
.PrintFixedPoint_label_122:
/* @125 */ 	clc         
/* @126 */ 	lda         __i9
/* @127 */ 	adc          #7
/* @128 */ 	sta         __i0
/* @129 */ 	lda         __i9+1
/* @130 */ 	adc          #0
/* @131 */ 	sta         __i0+1
/* @134 */ 	jsr         __pushi0
/* @136 */ 	ldx         #__b4
/* @137 */ 	ldy          #0
/* @138 */ 	jsr         __IsZero
/* @140 */ 	jsr         __incsp2
/* @142 */ 	lda         __b4
/* @144 */ 	beq         .PrintFixedPoint_label_159
/* @145 */ 	jsr         __pushi4
/* @146 */ 	jsr         __pushi5
/* @147 */ 	jsr         __pushi6
/* @149 */ 	ldx         #__i10
/* @150 */ 	ldy          #0
/* @151 */ 	jsr         WriteZero
/* @153 */ 	jsr         __incsp6
/* @155 */ 	ldx          #11
	jsr          __load_result
/* @156 */ 	lda         #__i10
/* @157 */ 	jsr         __result2
/* @158 */ 	bra         .PrintFixedPoint_label_119
.PrintFixedPoint_label_159:
/* @162 */ 	clc         
/* @163 */ 	lda         __i5
/* @164 */ 	adc         __i4
/* @165 */ 	sta         __i0
/* @166 */ 	lda         __i5+1
/* @167 */ 	adc         __i4+1
/* @168 */ 	sta         __i0+1
/* @172 */ 	sec         
/* @173 */ 	lda         __i0
/* @174 */ 	sbc          #1
/* @175 */ 	sta         __i1
/* @176 */ 	lda         __i0+1
/* @177 */ 	sbc          #0
/* @178 */ 	sta         __i1+1
/* @181 */ 	lda         __i1
/* @182 */ 	sta         __i7
/* @183 */ 	lda         __i1+1
/* @184 */ 	sta         __i7+1
/* @185 */ 	lda         __i7
/* @186 */ 	sta         __i8
/* @187 */ 	lda         __i7+1
/* @188 */ 	sta         __i8+1
/* @191 */ 	lda         __i8
/* @192 */ 	sta         __i0
/* @193 */ 	lda         __i8+1
/* @194 */ 	sta         __i0+1
/* @195 */ 	lda          #__i8
/* @197 */ 	jsr         __rdec21
/* @200 */ 	lda          #0
/* @201 */ 	tay         
/* @202 */ 	sta         (__i0)
/* @203 */ 	stz         __b3
/* @206 */ 	stz         __b0
/* @207 */ 	lda          #__b0
/* @209 */ 	ldx          #4
/* @211 */ 	jsr         __set_var_value1
.PrintFixedPoint_label_212:
/* @214 */ 	ldx          #4
	jsr          __var_value1_b0			// i
/* @218 */ 	lda         __b0
/* @219 */ 	sta         __i0
/* @220 */ 	and          #128
/* @222 */ 	beq         .PrintFixedPoint_label_221
/* @223 */ 	lda          #255
.PrintFixedPoint_label_221:
/* @224 */ 	sta         __i0+1
/* @227 */ 	lda         __i0
/* @228 */ 	cmp         __i6
/* @229 */ 	lda         __i0+1
/* @230 */ 	sbc         __i6+1
/* @231 */ 	bvc         .PrintFixedPoint_label_226
/* @232 */ 	eor          #128
.PrintFixedPoint_label_226:
/* @233 */ 	bpl         .PrintFixedPoint_label_286
/* @236 */ 	clc         
/* @237 */ 	lda         __i9
/* @238 */ 	adc          #7
/* @239 */ 	sta         __i0
/* @240 */ 	lda         __i9+1
/* @241 */ 	adc          #0
/* @242 */ 	sta         __i0+1
/* @245 */ 	jsr         __pushi0
/* @246 */ 	jsr         __MultiplyBy10
/* @247 */ 	jsr         __incsp2
/* @251 */ 	ldy          #30
/* @253 */ 	ldx          #7
.PrintFixedPoint_label_254:
/* @256 */ 	lda         (__i9), Y
/* @257 */ 	sta         __x0, X
/* @258 */ 	dey         
/* @259 */ 	dex         
/* @260 */ 	bpl         .PrintFixedPoint_label_254
/* @262 */ 	lda         __x0
/* @263 */ 	ora         __x0+1
/* @265 */ 	ora         __x0+2
/* @267 */ 	ora         __x0+3
/* @268 */ 	ora         __x0+4
/* @270 */ 	ora         __x0+5
/* @271 */ 	ora         __x0+6
/* @272 */ 	ora         __x0+7
/* @273 */ 	bne         .PrintFixedPoint_label_277
/* @274 */ 	lda          #__b3
/* @276 */ 	jsr         __rinc1
.PrintFixedPoint_label_277:
/* @280 */ 	ldx          #4
	jsr          __var_addr_i0			// i
/* @282 */ 	lda          #__i0
/* @284 */ 	jsr         __inc1
/* @285 */ 	bra         .PrintFixedPoint_label_212
.PrintFixedPoint_label_286:
/* @289 */ 	clc         
/* @290 */ 	lda         __i9
/* @291 */ 	adc          #7
/* @292 */ 	sta         __i0
/* @293 */ 	lda         __i9+1
/* @294 */ 	adc          #0
/* @295 */ 	sta         __i0+1
/* @298 */ 	jsr         __pushi0
/* @299 */ 	jsr         __Round
/* @300 */ 	jsr         __incsp2
/* @303 */ 	stz         __b0
/* @304 */ 	lda          #__b0
/* @305 */ 	ldx          #5
/* @306 */ 	jsr         __set_var_value1
.PrintFixedPoint_label_307:
/* @310 */ 	clc         
/* @311 */ 	lda         __i9
/* @312 */ 	adc          #7
/* @313 */ 	sta         __i0
/* @314 */ 	lda         __i9+1
/* @315 */ 	adc          #0
/* @316 */ 	sta         __i0+1
/* @320 */ 	clc         
/* @321 */ 	lda         __i0
/* @322 */ 	adc          #16
/* @323 */ 	sta         __i1
/* @324 */ 	lda         __i0+1
/* @325 */ 	adc          #0
/* @326 */ 	sta         __i1+1
/* @329 */ 	jsr         __pushi1
/* @331 */ 	ldx         #__b4
/* @332 */ 	ldy          #0
/* @333 */ 	jsr         __IsZeroHalf
/* @334 */ 	jsr         __incsp2
/* @339 */ 	lda         __b4
/* @341 */ 	beq         .PrintFixedPoint_label_338
/* @342 */ 	lda          #255
.PrintFixedPoint_label_338:
/* @343 */ 	inc          A
/* @348 */ 	beq         .PrintFixedPoint_label_434
/* @349 */ 	lda         __i6
/* @350 */ 	ora         __i6+1
/* @351 */ 	bne         .PrintFixedPoint_label_372
/* @354 */ 	lda         __i8
/* @355 */ 	sta         __i0
/* @356 */ 	lda         __i8+1
/* @357 */ 	sta         __i0+1
/* @358 */ 	lda          #__i8
/* @359 */ 	jsr         __rdec21
/* @362 */ 	lda          #46
/* @363 */ 	ldy          #0
/* @364 */ 	sta         (__i0)
/* @367 */ 	lda          #1
/* @368 */ 	sta         __b0
/* @369 */ 	lda          #__b0
/* @370 */ 	ldx          #5
/* @371 */ 	jsr         __set_var_value1
.PrintFixedPoint_label_372:
/* @375 */ 	clc         
/* @376 */ 	lda         __i9
/* @377 */ 	adc          #7
/* @378 */ 	sta         __i0
/* @379 */ 	lda         __i9+1
/* @380 */ 	adc          #0
/* @381 */ 	sta         __i0+1
/* @385 */ 	clc         
/* @386 */ 	lda         __i0
/* @387 */ 	adc          #16
/* @388 */ 	sta         __i1
/* @389 */ 	lda         __i0+1
/* @390 */ 	adc          #0
/* @391 */ 	sta         __i1+1
/* @394 */ 	jsr         __pushi1
/* @395 */ 	ldx          #6
	jsr         __var_addr_xy			// r
/* @396 */ 	jsr         __DivModBy10Half
/* @397 */ 	jsr         __incsp2
/* @400 */ 	lda         __i8
/* @401 */ 	sta         __i0
/* @402 */ 	lda         __i8+1
/* @403 */ 	sta         __i0+1
/* @404 */ 	lda          #__i8
/* @405 */ 	jsr         __rdec21
/* @407 */ 	ldx          #6
	jsr          __var_value1_b0			// r
/* @411 */ 	lda         __b0
/* @412 */ 	sta         __i1
/* @414 */ 	stz         __i1+1
/* @418 */ 	clc         
/* @420 */ 	adc          #48
/* @421 */ 	sta         __i2
/* @422 */ 	lda         __i1+1
/* @423 */ 	adc          #0
/* @424 */ 	sta         __i2+1
/* @429 */ 	lda         __i2
/* @430 */ 	sta         (__i0)
/* @431 */ 	lda          #__i6
/* @432 */ 	jsr         __rdec21
/* @433 */ 	jmp         .PrintFixedPoint_label_307
.PrintFixedPoint_label_434:
/* @435 */ 	stz         __b2
.PrintFixedPoint_label_436:
/* @438 */ 	lda         __b2
/* @439 */ 	sta         __i0
/* @441 */ 	stz         __i0+1
/* @444 */ 	lda         __b3
/* @445 */ 	sta         __i1
/* @446 */ 	and          #128
/* @448 */ 	beq         .PrintFixedPoint_label_447
/* @449 */ 	lda          #255
.PrintFixedPoint_label_447:
/* @450 */ 	sta         __i1+1
/* @454 */ 	lda         __i0+1
/* @455 */ 	cmp         __i1+1
/* @456 */ 	bcc         .PrintFixedPoint_label_453
/* @457 */ 	bne         .PrintFixedPoint_label_479
/* @458 */ 	lda         __i0
/* @459 */ 	cmp         __i1
/* @460 */ 	bcs         .PrintFixedPoint_label_479
.PrintFixedPoint_label_453:
/* @464 */ 	lda         __i8
/* @465 */ 	sta         __i0
/* @466 */ 	lda         __i8+1
/* @467 */ 	sta         __i0+1
/* @468 */ 	lda          #__i8
/* @469 */ 	jsr         __rdec21
/* @472 */ 	lda          #48
/* @473 */ 	ldy          #0
/* @474 */ 	sta         (__i0)
/* @476 */ 	lda          #__b2
/* @477 */ 	jsr         __rinc1
/* @478 */ 	bra         .PrintFixedPoint_label_436
.PrintFixedPoint_label_479:
/* @481 */ 	ldx          #5
	jsr          __var_value1_b0			// point_printed
/* @486 */ 	lda         __b0
/* @488 */ 	beq         .PrintFixedPoint_label_485
/* @489 */ 	lda          #255
.PrintFixedPoint_label_485:
/* @490 */ 	inc          A
/* @495 */ 	beq         .PrintFixedPoint_label_522
/* @498 */ 	lda         __i8
/* @499 */ 	sta         __i0
/* @500 */ 	lda         __i8+1
/* @501 */ 	sta         __i0+1
/* @502 */ 	lda          #__i8
/* @503 */ 	jsr         __rdec21
/* @506 */ 	lda          #46
/* @507 */ 	ldy          #0
/* @508 */ 	sta         (__i0)
/* @511 */ 	lda         __i8
/* @512 */ 	sta         __i0
/* @513 */ 	lda         __i8+1
/* @514 */ 	sta         __i0+1
/* @515 */ 	lda          #__i8
/* @516 */ 	jsr         __rdec21
/* @519 */ 	lda          #48
/* @520 */ 	ldy          #0
/* @521 */ 	sta         (__i0)
.PrintFixedPoint_label_522:
/* @525 */ 	lda         (__i9)
/* @530 */ 	beq         .PrintFixedPoint_label_544
/* @533 */ 	lda         __i8
/* @534 */ 	sta         __i0
/* @535 */ 	lda         __i8+1
/* @536 */ 	sta         __i0+1
/* @537 */ 	lda          #__i8
/* @538 */ 	jsr         __rdec21
/* @541 */ 	lda          #45
/* @542 */ 	ldy          #0
/* @543 */ 	sta         (__i0)
.PrintFixedPoint_label_544:
/* @547 */ 	clc         
/* @548 */ 	lda         __i8
/* @549 */ 	adc          #1
/* @550 */ 	sta         __i0
/* @551 */ 	lda         __i8+1
/* @552 */ 	adc          #0
/* @553 */ 	sta         __i0+1
/* @555 */ 	ldx          #11
	jsr          __load_result
/* @556 */ 	lda         #__i0
/* @557 */ 	jsr         __result2
/* @558 */ 	jmp         .PrintFixedPoint_label_119
.func_end_PrintFixedPoint:
	.size PrintFixedPoint, .func_end_PrintFixedPoint-PrintFixedPoint

	.global WriteExponent
	.type WriteExponent, @function

WriteExponent:
/* @17 */ 	stx         __result
/* @19 */ 	sty         __result+1
/* @20 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x42,0x00,0x00		// Save mask i:2 b:2 l:0 x:0 f:0 
/* @27 */ 	ldx          #0
	jsr          __arg_value2_i0			// exp
/* @35 */ 	ldx          #2
	jsr          __arg_value2_i2			// buf
/* @36 */ 	stz         __b1
/* @38 */ 	lda         __i0+1
/* @39 */ 	bpl         .WriteExponent_label_57
/* @42 */ 	sec         
/* @43 */ 	lda          #0
/* @44 */ 	sbc         __i0
/* @45 */ 	sta         __i3
/* @46 */ 	lda          #0
/* @47 */ 	sbc         __i0+1
/* @48 */ 	sta         __i3+1
/* @51 */ 	lda         __i3
/* @52 */ 	sta         __i0
/* @53 */ 	lda         __i3+1
/* @54 */ 	sta         __i0+1
/* @55 */ 	lda          #1
/* @56 */ 	sta         __b1
.WriteExponent_label_57:
/* @58 */ 	lda          #2
/* @59 */ 	sta         __i1
/* @61 */ 	stz         __i1+1
.WriteExponent_label_62:
/* @68 */ 	ldx          #1
/* @69 */ 	lda         __i0
/* @70 */ 	ora         __i0+1
/* @72 */ 	bne         .WriteExponent_label_67
/* @73 */ 	dex         
.WriteExponent_label_67:
/* @74 */ 	stx         __b3
/* @76 */ 	txa         
/* @77 */ 	bne         .WriteExponent_label_92
/* @80 */ 	ldx          #0
/* @82 */ 	txa         
/* @83 */ 	cmp         __i1
/* @85 */ 	sbc         __i1+1
/* @86 */ 	bvc         .WriteExponent_label_81
/* @88 */ 	eor          #128
.WriteExponent_label_81:
/* @89 */ 	bpl         .WriteExponent_label_79
/* @90 */ 	inx         
.WriteExponent_label_79:
/* @91 */ 	stx         __b3
.WriteExponent_label_92:
/* @94 */ 	lda         __b3
/* @96 */ 	beq         .WriteExponent_label_169
/* @99 */ 	lda          #10
/* @100 */ 	sta         __i3
/* @102 */ 	stz         __i3+1
/* @106 */ 	lda          #__i4
/* @107 */ 	ldx          #__i0
/* @108 */ 	ldy          #__i3
/* @110 */ 	jsr         __smod2
/* @113 */ 	lda         __i4
/* @114 */ 	sta         __b2
/* @117 */ 	lda         __i2
/* @118 */ 	sta         __i3
/* @119 */ 	lda         __i2+1
/* @120 */ 	sta         __i3+1
/* @121 */ 	lda          #__i2
/* @123 */ 	jsr         __rdec21
/* @125 */ 	lda         __b2
/* @126 */ 	sta         __i4
/* @128 */ 	stz         __i4+1
/* @132 */ 	clc         
/* @134 */ 	adc          #48
/* @135 */ 	sta         __i5
/* @136 */ 	lda         __i4+1
/* @137 */ 	adc          #0
/* @138 */ 	sta         __i5+1
/* @143 */ 	lda         __i5
/* @144 */ 	sta         (__i3)
/* @148 */ 	lda          #10
/* @149 */ 	sta         __i3
/* @151 */ 	stz         __i3+1
/* @155 */ 	lda          #__i4
/* @156 */ 	ldx          #__i0
/* @157 */ 	ldy          #__i3
/* @159 */ 	jsr         __sdiv2
/* @162 */ 	lda         __i4
/* @163 */ 	sta         __i0
/* @164 */ 	lda         __i4+1
/* @165 */ 	sta         __i0+1
/* @166 */ 	lda          #__i1
/* @167 */ 	jsr         __rdec21
/* @168 */ 	bra         .WriteExponent_label_62
.WriteExponent_label_169:
/* @172 */ 	lda         __i2
/* @173 */ 	sta         __i3
/* @174 */ 	lda         __i2+1
/* @175 */ 	sta         __i3+1
/* @176 */ 	lda          #__i2
/* @177 */ 	jsr         __rdec21
/* @178 */ 	lda         __b1
/* @180 */ 	beq         .WriteExponent_label_185
/* @182 */ 	lda          #45
/* @183 */ 	sta         __b0
/* @184 */ 	bra         .WriteExponent_label_189
.WriteExponent_label_185:
/* @187 */ 	lda          #43
/* @188 */ 	sta         __b0
.WriteExponent_label_189:
/* @194 */ 	lda         __b0
/* @195 */ 	sta         (__i3)
/* @196 */ 	lda         #__i2
/* @198 */ 	jsr         __result2
/* @200 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_WriteExponent:
	.size WriteExponent, .func_end_WriteExponent-WriteExponent

	.global CalculateExponent
	.type CalculateExponent, @function

CalculateExponent:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #39
	jsr          __enter
	.byte        0x22,0x00,0x00		// Save mask i:2 b:1 l:0 x:0 f:0 
/* @19 */ 	ldx          #0
	jsr          __arg_value2_i0			// v
/* @25 */ 	ldx          #35
	jsr          __var_addr_i1			// t
/* @30 */ 	lda          #32
/* @31 */ 	sta         __i2
/* @34 */ 	stz         __i2+1
/* @38 */ 	sta         __mem_size
/* @39 */ 	lda         __i2+1
/* @41 */ 	sta         __mem_size+1
/* @42 */ 	lda         __i0
/* @44 */ 	sta         __mem_src
/* @45 */ 	lda         __i0+1
/* @47 */ 	sta         __mem_src+1
/* @49 */ 	lda         __i1
/* @51 */ 	sta         __mem_dest
/* @52 */ 	lda         __i1+1
/* @54 */ 	sta         __mem_dest+1
/* @56 */ 	jsr         __builtin_memcpy
/* @57 */ 	stz         __i4
/* @58 */ 	stz         __i4+1
/* @64 */ 	clc         
/* @65 */ 	lda         __i1
/* @66 */ 	adc          #16
/* @67 */ 	sta         __i2
/* @68 */ 	lda         __i1+1
/* @69 */ 	adc          #0
/* @70 */ 	sta         __i2+1
/* @73 */ 	jsr         __pushi2
/* @75 */ 	ldx         #__b2
/* @76 */ 	ldy          #0
/* @77 */ 	jsr         __IsZeroHalf
/* @79 */ 	jsr         __incsp2
/* @81 */ 	lda         __b2
/* @83 */ 	beq         .CalculateExponent_label_122
.CalculateExponent_label_84:
/* @86 */ 	ldx          #35
	jsr          __var_addr_i5			// t
/* @90 */ 	clc         
/* @91 */ 	lda         __i5
/* @92 */ 	adc          #16
/* @93 */ 	sta         __i0
/* @94 */ 	lda         __i5+1
/* @95 */ 	adc          #0
/* @96 */ 	sta         __i0+1
/* @99 */ 	jsr         __pushi0
/* @101 */ 	ldx         #__b2
/* @102 */ 	ldy          #0
/* @103 */ 	jsr         __IsZeroHalf
/* @104 */ 	jsr         __incsp2
/* @106 */ 	lda         __b2
/* @108 */ 	beq         .CalculateExponent_label_120
/* @113 */ 	jsr         __pushi5
/* @114 */ 	jsr         __MultiplyBy10
/* @115 */ 	jsr         __incsp2
/* @116 */ 	lda          #__i4
/* @118 */ 	jsr         __rdec21
/* @119 */ 	bra         .CalculateExponent_label_84
.CalculateExponent_label_120:
/* @121 */ 	bra         .CalculateExponent_label_174
.CalculateExponent_label_122:
.CalculateExponent_label_123:
/* @125 */ 	ldx          #35
	jsr          __var_addr_i5			// t
/* @129 */ 	clc         
/* @130 */ 	lda         __i5
/* @131 */ 	adc          #16
/* @132 */ 	sta         __i0
/* @133 */ 	lda         __i5+1
/* @134 */ 	adc          #0
/* @135 */ 	sta         __i0+1
/* @138 */ 	jsr         __pushi0
/* @140 */ 	ldx         #__b2
/* @141 */ 	ldy          #0
/* @142 */ 	jsr         __IsLessThan10
/* @143 */ 	jsr         __incsp2
/* @148 */ 	lda         __b2
/* @150 */ 	beq         .CalculateExponent_label_147
/* @152 */ 	lda          #255
.CalculateExponent_label_147:
/* @153 */ 	inc          A
/* @158 */ 	beq         .CalculateExponent_label_173
/* @163 */ 	jsr         __pushi5
/* @165 */ 	ldx         #__b0
/* @166 */ 	ldy          #0
/* @167 */ 	jsr         __DivideBy10
/* @168 */ 	jsr         __incsp2
/* @169 */ 	lda          #__i4
/* @171 */ 	jsr         __rinc21
/* @172 */ 	bra         .CalculateExponent_label_123
.CalculateExponent_label_173:
.CalculateExponent_label_174:
/* @175 */ 	ldx          #40
	jsr          __load_result
/* @176 */ 	lda         #__i4
/* @178 */ 	jsr         __result2
/* @180 */ 	ldy          #42
	jmp          __leave
.func_end_CalculateExponent:
	.size CalculateExponent, .func_end_CalculateExponent-CalculateExponent

	.global PrintFixedPointScientific
	.type PrintFixedPointScientific, @function

PrintFixedPointScientific:
/* @19 */ 	stx         __result
/* @21 */ 	sty         __result+1
/* @22 */ 	ldx          #13
	jsr          __enter
	.byte        0x49,0x00,0x00		// Save mask i:9 b:2 l:0 x:0 f:0 
/* @38 */ 	ldx          #8
	jsr          __arg_value2_i4			// size
/* @42 */ 	ldx          #6
	jsr          __arg_value2_i5			// buf
/* @50 */ 	ldx          #2
	jsr          __arg_value2_i8			// precision
/* @56 */ 	ldx          #4
	jsr          __arg_value2_i10			// exp
/* @68 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @74 */ 	ldy          #6
/* @75 */ 	lda         (__i0), Y
/* @84 */ 	stz         __i0+1
/* @89 */ 	beq         .PrintFixedPointScientific_label_143
/* @90 */ 	jsr         __pushi4
/* @91 */ 	jsr         __pushi5
/* @93 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @98 */ 	ldy          #6
/* @99 */ 	lda         (__i0), Y
/* @105 */ 	sta         __i0
/* @107 */ 	and          #128
/* @109 */ 	beq         .PrintFixedPointScientific_label_108
/* @111 */ 	lda          #255
.PrintFixedPointScientific_label_108:
/* @112 */ 	sta         __i0+1
/* @115 */ 	jsr         __pushi0
/* @117 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @122 */ 	lda         (__i0)
/* @128 */ 	jsr         __pusha
/* @130 */ 	ldx         #__i12
/* @131 */ 	ldy          #0
/* @132 */ 	jsr         WriteNanInf
/* @134 */ 	jsr         __incsp8
/* @136 */ 	ldx          #14
	jsr          __load_result
/* @137 */ 	lda         #__i12
/* @139 */ 	jsr         __result2
.PrintFixedPointScientific_label_140:
/* @141 */ 	ldy          #16
	jmp          __leave
.PrintFixedPointScientific_label_143:
/* @145 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @149 */ 	clc         
/* @150 */ 	lda         __i0
/* @151 */ 	adc          #7
/* @152 */ 	sta         __i1
/* @153 */ 	lda         __i0+1
/* @154 */ 	adc          #0
/* @155 */ 	sta         __i1+1
/* @158 */ 	jsr         __pushi1
/* @160 */ 	ldx         #__b3
/* @161 */ 	ldy          #0
/* @162 */ 	jsr         __IsZero
/* @164 */ 	jsr         __incsp2
/* @166 */ 	lda         __b3
/* @168 */ 	bne         .PrintFixedPointScientific_label_620
/* @621 */ 	jmp         .PrintFixedPointScientific_label_278
.PrintFixedPointScientific_label_620:
/* @171 */ 	clc         
/* @172 */ 	lda         __i5
/* @173 */ 	adc         __i4
/* @174 */ 	sta         __i0
/* @175 */ 	lda         __i5+1
/* @176 */ 	adc         __i4+1
/* @177 */ 	sta         __i0+1
/* @181 */ 	sec         
/* @182 */ 	lda         __i0
/* @183 */ 	sbc          #1
/* @184 */ 	sta         __i1
/* @185 */ 	lda         __i0+1
/* @186 */ 	sbc          #0
/* @187 */ 	sta         __i1+1
/* @190 */ 	lda         __i1
/* @191 */ 	sta         __i6
/* @192 */ 	lda         __i1+1
/* @193 */ 	sta         __i6+1
/* @196 */ 	lda         __i6
/* @197 */ 	sta         __i0
/* @198 */ 	lda         __i6+1
/* @199 */ 	sta         __i0+1
/* @200 */ 	lda          #__i6
/* @202 */ 	jsr         __rdec21
/* @205 */ 	lda          #0
/* @206 */ 	tay         
/* @207 */ 	sta         (__i0)
/* @208 */ 	jsr         __pushi6
/* @209 */ 	ldx          #0
/* @211 */ 	jsr         __pushxy0
/* @212 */ 	ldx         #__i7
/* @213 */ 	ldy          #0
/* @214 */ 	jsr         WriteExponent
/* @216 */ 	jsr         __incsp4
/* @219 */ 	lda         __i7
/* @220 */ 	sta         __i0
/* @221 */ 	lda         __i7+1
/* @222 */ 	sta         __i0+1
/* @223 */ 	lda          #__i7
/* @224 */ 	jsr         __rdec21
/* @227 */ 	lda          #101
/* @228 */ 	ldy          #0
/* @229 */ 	sta         (__i0)
/* @232 */ 	sec         
/* @233 */ 	lda         __i7
/* @234 */ 	sbc         __i5
/* @235 */ 	sta         __i0
/* @236 */ 	lda         __i7+1
/* @237 */ 	sbc         __i5+1
/* @238 */ 	sta         __i0+1
/* @242 */ 	clc         
/* @245 */ 	ldy          #4
/* @246 */ 	ldx          #0
.PrintFixedPointScientific_label_247:
/* @249 */ 	lda         __i0, X
/* @250 */ 	adc         .lit.53, X
/* @251 */ 	sta         __l0, X
/* @252 */ 	inx         
/* @253 */ 	dey         
/* @254 */ 	bne         .PrintFixedPointScientific_label_247
/* @258 */ 	lda         __l0
/* @259 */ 	sta         __i0
/* @260 */ 	lda         __l0+1
/* @261 */ 	sta         __i0+1
/* @264 */ 	jsr         __pushi0
/* @265 */ 	jsr         __pushi5
/* @266 */ 	jsr         __pushi8
/* @268 */ 	ldx         #__i12
/* @269 */ 	ldy          #0
/* @270 */ 	jsr         WriteZero
/* @272 */ 	jsr         __incsp6
/* @274 */ 	ldx          #14
	jsr          __load_result
/* @275 */ 	lda         #__i12
/* @276 */ 	jsr         __result2
/* @277 */ 	jmp         .PrintFixedPointScientific_label_140
.PrintFixedPointScientific_label_278:
/* @281 */ 	sec         
/* @282 */ 	lda         __i8
/* @283 */ 	sbc         __i10
/* @284 */ 	sta         __i0
/* @285 */ 	lda         __i8+1
/* @286 */ 	sbc         __i10+1
/* @287 */ 	sta         __i0+1
/* @290 */ 	lda         __i0
/* @291 */ 	sta         __i9
/* @292 */ 	lda         __i0+1
/* @293 */ 	sta         __i9+1
/* @296 */ 	bpl         .PrintFixedPointScientific_label_332
/* @297 */ 	lda         __i9
/* @298 */ 	sta         __i11
/* @299 */ 	lda         __i9+1
/* @300 */ 	sta         __i11+1
.PrintFixedPointScientific_label_301:
/* @303 */ 	lda         __i11+1
/* @304 */ 	bpl         .PrintFixedPointScientific_label_330
/* @306 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @310 */ 	clc         
/* @311 */ 	lda         __i0
/* @312 */ 	adc          #7
/* @313 */ 	sta         __i1
/* @314 */ 	lda         __i0+1
/* @315 */ 	adc          #0
/* @316 */ 	sta         __i1+1
/* @319 */ 	jsr         __pushi1
/* @321 */ 	ldx         #__b0
/* @322 */ 	ldy          #0
/* @323 */ 	jsr         __DivideBy10
/* @324 */ 	jsr         __incsp2
/* @326 */ 	lda          #__i11
/* @328 */ 	jsr         __rinc21
/* @329 */ 	bra         .PrintFixedPointScientific_label_301
.PrintFixedPointScientific_label_330:
/* @331 */ 	bra         .PrintFixedPointScientific_label_380
.PrintFixedPointScientific_label_332:
/* @335 */ 	stz         __i12
/* @336 */ 	stz         __i12+1
/* @337 */ 	lda          #__i12
/* @339 */ 	ldx          #5
/* @341 */ 	jsr         __set_var_value2
.PrintFixedPointScientific_label_342:
/* @344 */ 	ldx          #5
	jsr          __var_value2_i12			// i
/* @347 */ 	lda         __i12
/* @348 */ 	cmp         __i9
/* @349 */ 	lda         __i12+1
/* @350 */ 	sbc         __i9+1
/* @351 */ 	bvc         .PrintFixedPointScientific_label_346
/* @352 */ 	eor          #128
.PrintFixedPointScientific_label_346:
/* @353 */ 	bpl         .PrintFixedPointScientific_label_379
/* @355 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @359 */ 	clc         
/* @360 */ 	lda         __i0
/* @361 */ 	adc          #7
/* @362 */ 	sta         __i1
/* @363 */ 	lda         __i0+1
/* @364 */ 	adc          #0
/* @365 */ 	sta         __i1+1
/* @368 */ 	jsr         __pushi1
/* @369 */ 	jsr         __MultiplyBy10
/* @370 */ 	jsr         __incsp2
/* @373 */ 	ldx          #5
	jsr          __var_addr_i0			// i
/* @375 */ 	lda          #__i0
/* @377 */ 	jsr         __inc21
/* @378 */ 	bra         .PrintFixedPointScientific_label_342
.PrintFixedPointScientific_label_379:
.PrintFixedPointScientific_label_380:
/* @382 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @386 */ 	clc         
/* @387 */ 	lda         __i0
/* @388 */ 	adc          #7
/* @389 */ 	sta         __i1
/* @390 */ 	lda         __i0+1
/* @391 */ 	adc          #0
/* @392 */ 	sta         __i1+1
/* @395 */ 	jsr         __pushi1
/* @396 */ 	jsr         __Round
/* @397 */ 	jsr         __incsp2
/* @400 */ 	clc         
/* @401 */ 	lda         __i5
/* @402 */ 	adc         __i4
/* @403 */ 	sta         __i0
/* @404 */ 	lda         __i5+1
/* @405 */ 	adc         __i4+1
/* @406 */ 	sta         __i0+1
/* @410 */ 	sec         
/* @411 */ 	lda         __i0
/* @412 */ 	sbc          #1
/* @413 */ 	sta         __i1
/* @414 */ 	lda         __i0+1
/* @415 */ 	sbc          #0
/* @416 */ 	sta         __i1+1
/* @418 */ 	lda          #__i1
/* @420 */ 	ldx          #7
/* @421 */ 	jsr         __set_var_value2
/* @423 */ 	ldx          #7
	jsr          __var_value2_i0			// end
/* @425 */ 	ldx          #7
	jsr          __var_addr_i1			// end
/* @427 */ 	lda          #__i1
/* @429 */ 	jsr         __dec21
/* @432 */ 	lda          #0
/* @433 */ 	tay         
/* @434 */ 	sta         (__i0)
/* @436 */ 	ldx          #7
	jsr          __var_value2_i0			// end
/* @439 */ 	jsr         __pushi0
/* @440 */ 	jsr         __pushi10
/* @441 */ 	ldx          #9
	jsr         __var_addr_xy			// p
/* @442 */ 	jsr         WriteExponent
/* @443 */ 	jsr         __incsp4
/* @445 */ 	ldx          #9
	jsr          __var_value2_i0			// p
/* @447 */ 	ldx          #9
	jsr          __var_addr_i1			// p
/* @449 */ 	lda          #__i1
/* @450 */ 	jsr         __dec21
/* @453 */ 	lda          #101
/* @454 */ 	ldy          #0
/* @455 */ 	sta         (__i0)
.PrintFixedPointScientific_label_456:
/* @458 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @462 */ 	clc         
/* @463 */ 	lda         __i0
/* @464 */ 	adc          #7
/* @465 */ 	sta         __i1
/* @466 */ 	lda         __i0+1
/* @467 */ 	adc          #0
/* @468 */ 	sta         __i1+1
/* @472 */ 	clc         
/* @473 */ 	lda         __i1
/* @474 */ 	adc          #16
/* @475 */ 	sta         __i0
/* @476 */ 	lda         __i1+1
/* @477 */ 	adc          #0
/* @478 */ 	sta         __i0+1
/* @481 */ 	jsr         __pushi0
/* @483 */ 	ldx         #__b3
/* @484 */ 	ldy          #0
/* @485 */ 	jsr         __IsZeroHalf
/* @486 */ 	jsr         __incsp2
/* @491 */ 	lda         __b3
/* @493 */ 	beq         .PrintFixedPointScientific_label_490
/* @494 */ 	lda          #255
.PrintFixedPointScientific_label_490:
/* @495 */ 	inc          A
/* @500 */ 	beq         .PrintFixedPointScientific_label_577
/* @501 */ 	lda         __i8
/* @502 */ 	ora         __i8+1
/* @503 */ 	bne         .PrintFixedPointScientific_label_516
/* @505 */ 	ldx          #9
	jsr          __var_value2_i0			// p
/* @507 */ 	ldx          #9
	jsr          __var_addr_i1			// p
/* @509 */ 	lda          #__i1
/* @510 */ 	jsr         __dec21
/* @513 */ 	lda          #46
/* @514 */ 	ldy          #0
/* @515 */ 	sta         (__i0)
.PrintFixedPointScientific_label_516:
/* @518 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @522 */ 	clc         
/* @523 */ 	lda         __i0
/* @524 */ 	adc          #7
/* @525 */ 	sta         __i1
/* @526 */ 	lda         __i0+1
/* @527 */ 	adc          #0
/* @528 */ 	sta         __i1+1
/* @532 */ 	clc         
/* @533 */ 	lda         __i1
/* @534 */ 	adc          #16
/* @535 */ 	sta         __i0
/* @536 */ 	lda         __i1+1
/* @537 */ 	adc          #0
/* @538 */ 	sta         __i0+1
/* @541 */ 	jsr         __pushi0
/* @542 */ 	ldx         #__b2
/* @543 */ 	ldy          #0
/* @544 */ 	jsr         __DivModBy10Half
/* @545 */ 	jsr         __incsp2
/* @547 */ 	ldx          #9
	jsr          __var_value2_i0			// p
/* @549 */ 	ldx          #9
	jsr          __var_addr_i1			// p
/* @551 */ 	lda          #__i1
/* @552 */ 	jsr         __dec21
/* @554 */ 	lda         __b2
/* @555 */ 	sta         __i1
/* @557 */ 	stz         __i1+1
/* @561 */ 	clc         
/* @563 */ 	adc          #48
/* @564 */ 	sta         __i2
/* @565 */ 	lda         __i1+1
/* @566 */ 	adc          #0
/* @567 */ 	sta         __i2+1
/* @572 */ 	lda         __i2
/* @573 */ 	sta         (__i0)
/* @574 */ 	lda          #__i8
/* @575 */ 	jsr         __rdec21
/* @576 */ 	jmp         .PrintFixedPointScientific_label_456
.PrintFixedPointScientific_label_577:
/* @579 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @584 */ 	lda         (__i0)
/* @589 */ 	beq         .PrintFixedPointScientific_label_602
/* @591 */ 	ldx          #9
	jsr          __var_value2_i0			// p
/* @593 */ 	ldx          #9
	jsr          __var_addr_i1			// p
/* @595 */ 	lda          #__i1
/* @596 */ 	jsr         __dec21
/* @599 */ 	lda          #45
/* @600 */ 	ldy          #0
/* @601 */ 	sta         (__i0)
.PrintFixedPointScientific_label_602:
/* @604 */ 	ldx          #9
	jsr          __var_value2_i0			// p
/* @608 */ 	clc         
/* @609 */ 	lda         __i0
/* @610 */ 	adc          #1
/* @611 */ 	sta         __i1
/* @612 */ 	lda         __i0+1
/* @613 */ 	adc          #0
/* @614 */ 	sta         __i1+1
/* @616 */ 	ldx          #14
	jsr          __load_result
/* @617 */ 	lda         #__i1
/* @618 */ 	jsr         __result2
/* @619 */ 	jmp         .PrintFixedPointScientific_label_140
.func_end_PrintFixedPointScientific:
	.size PrintFixedPointScientific, .func_end_PrintFixedPointScientific-PrintFixedPointScientific

	.global PrintFixedPointGeneral
	.type PrintFixedPointGeneral, @function

PrintFixedPointGeneral:
/* @22 */ 	stx         __result
/* @24 */ 	sty         __result+1
/* @25 */ 	ldx          #9
	jsr          __enter
	.byte        0x49,0x00,0x00		// Save mask i:9 b:2 l:0 x:0 f:0 
/* @37 */ 	ldx          #6
	jsr          __arg_value2_i5			// size
/* @41 */ 	ldx          #4
	jsr          __arg_value2_i6			// buf
/* @51 */ 	ldx          #2
	jsr          __arg_value2_i9			// precision
/* @61 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @67 */ 	ldy          #6
/* @68 */ 	lda         (__i0), Y
/* @76 */ 	stz         __i0+1
/* @81 */ 	beq         .PrintFixedPointGeneral_label_135
/* @82 */ 	jsr         __pushi5
/* @83 */ 	jsr         __pushi6
/* @85 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @90 */ 	ldy          #6
/* @91 */ 	lda         (__i0), Y
/* @97 */ 	sta         __i0
/* @99 */ 	and          #128
/* @101 */ 	beq         .PrintFixedPointGeneral_label_100
/* @103 */ 	lda          #255
.PrintFixedPointGeneral_label_100:
/* @104 */ 	sta         __i0+1
/* @107 */ 	jsr         __pushi0
/* @109 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @114 */ 	lda         (__i0)
/* @120 */ 	jsr         __pusha
/* @122 */ 	ldx         #__i12
/* @123 */ 	ldy          #0
/* @124 */ 	jsr         WriteNanInf
/* @126 */ 	jsr         __incsp8
/* @128 */ 	ldx          #10
	jsr          __load_result
/* @129 */ 	lda         #__i12
/* @131 */ 	jsr         __result2
.PrintFixedPointGeneral_label_132:
/* @133 */ 	ldy          #12
	jmp          __leave
.PrintFixedPointGeneral_label_135:
/* @137 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @141 */ 	clc         
/* @142 */ 	lda         __i0
/* @143 */ 	adc          #7
/* @144 */ 	sta         __i1
/* @145 */ 	lda         __i0+1
/* @146 */ 	adc          #0
/* @147 */ 	sta         __i1+1
/* @150 */ 	jsr         __pushi1
/* @152 */ 	ldx         #__b3
/* @153 */ 	ldy          #0
/* @154 */ 	jsr         __IsZero
/* @156 */ 	jsr         __incsp2
/* @158 */ 	lda         __b3
/* @160 */ 	beq         .PrintFixedPointGeneral_label_207
/* @163 */ 	clc         
/* @164 */ 	lda         __i6
/* @165 */ 	adc         __i5
/* @166 */ 	sta         __i0
/* @167 */ 	lda         __i6+1
/* @168 */ 	adc         __i5+1
/* @169 */ 	sta         __i0+1
/* @173 */ 	sec         
/* @174 */ 	lda         __i0
/* @175 */ 	sbc          #1
/* @176 */ 	sta         __i1
/* @177 */ 	lda         __i0+1
/* @178 */ 	sbc          #0
/* @179 */ 	sta         __i1+1
/* @182 */ 	lda         __i1
/* @183 */ 	sta         __i7
/* @184 */ 	lda         __i1+1
/* @185 */ 	sta         __i7+1
/* @188 */ 	lda         __i7
/* @189 */ 	sta         __i0
/* @190 */ 	lda         __i7+1
/* @191 */ 	sta         __i0+1
/* @192 */ 	lda          #__i7
/* @194 */ 	jsr         __rdec21
/* @197 */ 	lda          #0
/* @198 */ 	tay         
/* @199 */ 	sta         (__i0)
/* @200 */ 	lda          #48
/* @202 */ 	sta         (__i7)
/* @203 */ 	ldx          #10
	jsr          __load_result
/* @204 */ 	lda         #__i7
/* @205 */ 	jsr         __result2
/* @206 */ 	bra         .PrintFixedPointGeneral_label_132
.PrintFixedPointGeneral_label_207:
/* @209 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @213 */ 	clc         
/* @214 */ 	lda         __i0
/* @215 */ 	adc          #7
/* @216 */ 	sta         __i1
/* @217 */ 	lda         __i0+1
/* @218 */ 	adc          #0
/* @219 */ 	sta         __i1+1
/* @222 */ 	jsr         __pushi1
/* @223 */ 	ldx         #__i8
/* @224 */ 	ldy          #0
/* @225 */ 	jsr         CalculateExponent
/* @226 */ 	jsr         __incsp2
/* @227 */ 	stz         __b2
/* @232 */ 	ldx          #0
/* @234 */ 	lda         __i8
/* @235 */ 	cmp         __i9
/* @236 */ 	lda         __i8+1
/* @237 */ 	sbc         __i9+1
/* @238 */ 	bvc         .PrintFixedPointGeneral_label_233
/* @239 */ 	eor          #128
.PrintFixedPointGeneral_label_233:
/* @240 */ 	bpl         .PrintFixedPointGeneral_label_231
/* @241 */ 	inx         
.PrintFixedPointGeneral_label_231:
/* @242 */ 	stx         __b3
/* @244 */ 	txa         
/* @245 */ 	cmp          #0
/* @246 */ 	beq         .PrintFixedPointGeneral_label_260
/* @249 */ 	ldx          #0
/* @251 */ 	lda         __i8
/* @252 */ 	cmp          #252
/* @253 */ 	lda         __i8+1
/* @254 */ 	sbc          #255
/* @255 */ 	bvc         .PrintFixedPointGeneral_label_250
/* @256 */ 	eor          #128
.PrintFixedPointGeneral_label_250:
/* @257 */ 	bmi         .PrintFixedPointGeneral_label_248
/* @258 */ 	inx         
.PrintFixedPointGeneral_label_248:
/* @259 */ 	stx         __b3
.PrintFixedPointGeneral_label_260:
/* @262 */ 	lda         __b3
/* @264 */ 	beq         .PrintFixedPointGeneral_label_301
/* @265 */ 	lda          #1
/* @266 */ 	sta         __b2
/* @267 */ 	jsr         __pushi5
/* @268 */ 	jsr         __pushi6
/* @271 */ 	clc         
/* @272 */ 	lda         __i8
/* @273 */ 	adc          #1
/* @274 */ 	sta         __i0
/* @275 */ 	lda         __i8+1
/* @276 */ 	adc          #0
/* @277 */ 	sta         __i0+1
/* @281 */ 	sec         
/* @282 */ 	lda         __i9
/* @283 */ 	sbc         __i0
/* @284 */ 	sta         __i1
/* @285 */ 	lda         __i9+1
/* @286 */ 	sbc         __i0+1
/* @287 */ 	sta         __i1+1
/* @290 */ 	jsr         __pushi1
/* @292 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @295 */ 	jsr         __pushi0
/* @296 */ 	ldx         #__i10
/* @297 */ 	ldy          #0
/* @298 */ 	jsr         PrintFixedPoint
/* @299 */ 	jsr         __incsp8
/* @300 */ 	bra         .PrintFixedPointGeneral_label_327
.PrintFixedPointGeneral_label_301:
/* @302 */ 	jsr         __pushi5
/* @303 */ 	jsr         __pushi6
/* @304 */ 	jsr         __pushi8
/* @307 */ 	sec         
/* @308 */ 	lda         __i9
/* @309 */ 	sbc          #1
/* @310 */ 	sta         __i0
/* @311 */ 	lda         __i9+1
/* @312 */ 	sbc          #0
/* @313 */ 	sta         __i0+1
/* @316 */ 	jsr         __pushi0
/* @318 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @321 */ 	jsr         __pushi0
/* @322 */ 	ldx         #__i10
/* @323 */ 	ldy          #0
/* @324 */ 	jsr         PrintFixedPointScientific
/* @326 */ 	jsr         __incsp10
.PrintFixedPointGeneral_label_327:
/* @328 */ 	lda         __b2
/* @330 */ 	beq         .PrintFixedPointGeneral_label_415
/* @333 */ 	clc         
/* @334 */ 	lda         __i6
/* @335 */ 	adc         __i5
/* @336 */ 	sta         __i0
/* @337 */ 	lda         __i6+1
/* @338 */ 	adc         __i5+1
/* @339 */ 	sta         __i0+1
/* @343 */ 	sec         
/* @344 */ 	lda         __i0
/* @345 */ 	sbc          #2
/* @346 */ 	sta         __i1
/* @347 */ 	lda         __i0+1
/* @348 */ 	sbc          #0
/* @349 */ 	sta         __i1+1
/* @352 */ 	lda         __i1
/* @353 */ 	sta         __i11
/* @354 */ 	lda         __i1+1
/* @355 */ 	sta         __i11+1
.PrintFixedPointGeneral_label_356:
/* @359 */ 	lda         (__i11)
/* @365 */ 	sta         __i0
/* @366 */ 	and          #128
/* @368 */ 	beq         .PrintFixedPointGeneral_label_367
/* @369 */ 	lda          #255
.PrintFixedPointGeneral_label_367:
/* @370 */ 	sta         __i0+1
/* @373 */ 	lda         __i0
/* @374 */ 	cmp          #48
/* @375 */ 	bne         .PrintFixedPointGeneral_label_386
/* @376 */ 	lda         __i0+1
/* @378 */ 	bne         .PrintFixedPointGeneral_label_386
/* @380 */ 	lda          #0
/* @381 */ 	tay         
/* @382 */ 	sta         (__i11)
/* @383 */ 	lda          #__i11
/* @384 */ 	jsr         __rdec21
/* @385 */ 	bra         .PrintFixedPointGeneral_label_356
.PrintFixedPointGeneral_label_386:
/* @389 */ 	lda         (__i11)
/* @395 */ 	sta         __i0
/* @396 */ 	and          #128
/* @398 */ 	beq         .PrintFixedPointGeneral_label_397
/* @399 */ 	lda          #255
.PrintFixedPointGeneral_label_397:
/* @400 */ 	sta         __i0+1
/* @403 */ 	lda         __i0
/* @404 */ 	cmp          #46
/* @405 */ 	bne         .PrintFixedPointGeneral_label_413
/* @406 */ 	lda         __i0+1
/* @408 */ 	bne         .PrintFixedPointGeneral_label_413
/* @410 */ 	lda          #0
/* @411 */ 	tay         
/* @412 */ 	sta         (__i11)
.PrintFixedPointGeneral_label_413:
/* @414 */ 	jmp         .PrintFixedPointGeneral_label_633
.PrintFixedPointGeneral_label_415:
/* @418 */ 	clc         
/* @419 */ 	lda         __i6
/* @420 */ 	adc         __i5
/* @421 */ 	sta         __i0
/* @422 */ 	lda         __i6+1
/* @423 */ 	adc         __i5+1
/* @424 */ 	sta         __i0+1
/* @428 */ 	sec         
/* @429 */ 	lda         __i0
/* @430 */ 	sbc          #5
/* @431 */ 	sta         __i1
/* @432 */ 	lda         __i0+1
/* @433 */ 	sbc          #0
/* @434 */ 	sta         __i1+1
/* @436 */ 	lda          #__i1
/* @438 */ 	ldx          #5
/* @440 */ 	jsr         __set_var_value2
/* @442 */ 	ldx          #5
	jsr          __var_value2_i0			// e
/* @447 */ 	lda         (__i0)
/* @453 */ 	sta         __i0
/* @454 */ 	and          #128
/* @456 */ 	beq         .PrintFixedPointGeneral_label_455
/* @457 */ 	lda          #255
.PrintFixedPointGeneral_label_455:
/* @458 */ 	sta         __i0+1
/* @461 */ 	lda         __i0
/* @462 */ 	cmp          #101
/* @463 */ 	bne         .PrintFixedPointGeneral_label_460
/* @464 */ 	lda         __i0+1
/* @466 */ 	beq         .PrintFixedPointGeneral_label_474
.PrintFixedPointGeneral_label_460:
/* @469 */ 	ldx          #5
	jsr          __var_addr_i0			// e
/* @471 */ 	lda          #__i0
/* @473 */ 	jsr         __dec21
.PrintFixedPointGeneral_label_474:
/* @476 */ 	ldx          #5
	jsr          __var_value2_i0			// e
/* @480 */ 	sec         
/* @481 */ 	lda         __i0
/* @482 */ 	sbc          #1
/* @483 */ 	sta         __i1
/* @484 */ 	lda         __i0+1
/* @485 */ 	sbc          #0
/* @486 */ 	sta         __i1+1
/* @489 */ 	lda         __i1
/* @490 */ 	sta         __i4
/* @491 */ 	lda         __i1+1
/* @492 */ 	sta         __i4+1
.PrintFixedPointGeneral_label_493:
/* @496 */ 	lda         (__i4)
/* @502 */ 	sta         __i0
/* @503 */ 	and          #128
/* @505 */ 	beq         .PrintFixedPointGeneral_label_504
/* @506 */ 	lda          #255
.PrintFixedPointGeneral_label_504:
/* @507 */ 	sta         __i0+1
/* @510 */ 	lda         __i0
/* @511 */ 	cmp          #48
/* @512 */ 	bne         .PrintFixedPointGeneral_label_523
/* @513 */ 	lda         __i0+1
/* @515 */ 	bne         .PrintFixedPointGeneral_label_523
/* @517 */ 	lda          #0
/* @518 */ 	tay         
/* @519 */ 	sta         (__i4)
/* @520 */ 	lda          #__i4
/* @521 */ 	jsr         __rdec21
/* @522 */ 	bra         .PrintFixedPointGeneral_label_493
.PrintFixedPointGeneral_label_523:
/* @526 */ 	lda         (__i4)
/* @532 */ 	sta         __i0
/* @533 */ 	and          #128
/* @535 */ 	beq         .PrintFixedPointGeneral_label_534
/* @536 */ 	lda          #255
.PrintFixedPointGeneral_label_534:
/* @537 */ 	sta         __i0+1
/* @540 */ 	lda         __i0
/* @541 */ 	cmp          #46
/* @542 */ 	bne         .PrintFixedPointGeneral_label_550
/* @543 */ 	lda         __i0+1
/* @545 */ 	bne         .PrintFixedPointGeneral_label_550
/* @547 */ 	lda          #0
/* @548 */ 	tay         
/* @549 */ 	sta         (__i4)
.PrintFixedPointGeneral_label_550:
/* @551 */ 	lda          #__i4
/* @553 */ 	jsr         __rinc21
/* @555 */ 	ldx          #5
	jsr          __var_value2_i0			// e
/* @559 */ 	sec         
/* @560 */ 	lda         __i0
/* @561 */ 	sbc          #1
/* @562 */ 	sta         __i1
/* @563 */ 	lda         __i0+1
/* @564 */ 	sbc          #0
/* @565 */ 	sta         __i1+1
/* @568 */ 	lda         __i4
/* @569 */ 	cmp         __i1
/* @570 */ 	lda         __i4+1
/* @571 */ 	sbc         __i1+1
/* @572 */ 	bvc         .PrintFixedPointGeneral_label_567
/* @573 */ 	eor          #128
.PrintFixedPointGeneral_label_567:
/* @574 */ 	bpl         .PrintFixedPointGeneral_label_629
.PrintFixedPointGeneral_label_575:
/* @577 */ 	ldx          #5
	jsr          __var_value2_i0			// e
/* @582 */ 	lda         (__i0)
/* @588 */ 	sta         __i0
/* @589 */ 	and          #128
/* @591 */ 	beq         .PrintFixedPointGeneral_label_590
/* @592 */ 	lda          #255
.PrintFixedPointGeneral_label_590:
/* @593 */ 	sta         __i0+1
/* @595 */ 	lda         __i0
/* @596 */ 	ora         __i0+1
/* @598 */ 	beq         .PrintFixedPointGeneral_label_628
/* @601 */ 	lda         __i4
/* @602 */ 	sta         __i0
/* @603 */ 	lda         __i4+1
/* @604 */ 	sta         __i0+1
/* @605 */ 	lda          #__i4
/* @606 */ 	jsr         __rinc21
/* @608 */ 	ldx          #5
	jsr          __var_value2_i1			// e
/* @610 */ 	ldx          #5
	jsr          __var_addr_i2			// e
/* @612 */ 	lda          #__i2
/* @614 */ 	jsr         __inc21
/* @619 */ 	lda         (__i1)
/* @626 */ 	sta         (__i0)
/* @627 */ 	bra         .PrintFixedPointGeneral_label_575
.PrintFixedPointGeneral_label_628:
.PrintFixedPointGeneral_label_629:
/* @630 */ 	lda          #0
/* @631 */ 	tay         
/* @632 */ 	sta         (__i4)
.PrintFixedPointGeneral_label_633:
/* @634 */ 	ldx          #10
	jsr          __load_result
/* @635 */ 	lda         #__i10
/* @636 */ 	jsr         __result2
/* @637 */ 	jmp         .PrintFixedPointGeneral_label_132
.func_end_PrintFixedPointGeneral:
	.size PrintFixedPointGeneral, .func_end_PrintFixedPointGeneral-PrintFixedPointGeneral

	.global __PrintFloatFormat
	.type __PrintFloatFormat, @function

__PrintFloatFormat:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #46
	jsr          __enter
	.byte        0x05,0x00,0x00		// Save mask i:5 b:0 l:0 x:0 f:0 
/* @18 */ 	ldx          #0
	jsr          __arg_value4_f0			// f
/* @22 */ 	ldx          #8
	jsr          __arg_value2_i4			// size
/* @26 */ 	ldx          #6
	jsr          __arg_value2_i5			// buf
/* @30 */ 	ldx          #4
	jsr          __arg_value2_i6			// precision
/* @32 */ 	lda          #39
/* @34 */ 	sta         __mem_size
/* @36 */ 	ldx          #42
	jsr          __var_addr_i7			// printer
/* @38 */ 	lda         __i7
/* @40 */ 	sta         __mem_dest
/* @42 */ 	lda         __i7+1
/* @44 */ 	sta         __mem_dest+1
/* @46 */ 	jsr         __zeromem1
/* @47 */ 	jsr         __pushf0
/* @52 */ 	jsr         __pushi7
/* @53 */ 	jsr         FixFloat
/* @55 */ 	jsr         __incsp6
/* @56 */ 	jsr         __pushi4
/* @57 */ 	jsr         __pushi5
/* @58 */ 	jsr         __pushi6
/* @63 */ 	jsr         __pushi7
/* @65 */ 	ldx         #__i8
/* @66 */ 	ldy          #0
/* @67 */ 	jsr         PrintFixedPoint
/* @69 */ 	jsr         __incsp8
/* @71 */ 	ldx          #47
	jsr          __load_result
/* @72 */ 	lda         #__i8
/* @74 */ 	jsr         __result2
/* @76 */ 	ldy          #49
	jmp          __leave
.func_end___PrintFloatFormat:
	.size __PrintFloatFormat, .func_end___PrintFloatFormat-__PrintFloatFormat

	.global __PrintScientificFormat
	.type __PrintScientificFormat, @function

__PrintScientificFormat:
/* @9 */ 	stx         __result
/* @11 */ 	sty         __result+1
/* @12 */ 	ldx          #46
	jsr          __enter
	.byte        0x06,0x00,0x00		// Save mask i:6 b:0 l:0 x:0 f:0 
/* @21 */ 	ldx          #0
	jsr          __arg_value4_f0			// f
/* @27 */ 	ldx          #8
	jsr          __arg_value2_i5			// size
/* @31 */ 	ldx          #6
	jsr          __arg_value2_i6			// buf
/* @35 */ 	ldx          #4
	jsr          __arg_value2_i7			// precision
/* @37 */ 	lda          #39
/* @39 */ 	sta         __mem_size
/* @41 */ 	ldx          #42
	jsr          __var_addr_i8			// printer
/* @43 */ 	lda         __i8
/* @45 */ 	sta         __mem_dest
/* @47 */ 	lda         __i8+1
/* @49 */ 	sta         __mem_dest+1
/* @51 */ 	jsr         __zeromem1
/* @52 */ 	jsr         __pushf0
/* @57 */ 	jsr         __pushi8
/* @58 */ 	jsr         FixFloat
/* @60 */ 	jsr         __incsp6
/* @66 */ 	clc         
/* @67 */ 	lda         __i8
/* @68 */ 	adc          #7
/* @69 */ 	sta         __i0
/* @70 */ 	lda         __i8+1
/* @71 */ 	adc          #0
/* @72 */ 	sta         __i0+1
/* @75 */ 	jsr         __pushi0
/* @76 */ 	ldx         #__i4
/* @77 */ 	ldy          #0
/* @78 */ 	jsr         CalculateExponent
/* @80 */ 	jsr         __incsp2
/* @81 */ 	jsr         __pushi5
/* @82 */ 	jsr         __pushi6
/* @83 */ 	jsr         __pushi4
/* @84 */ 	jsr         __pushi7
/* @89 */ 	jsr         __pushi8
/* @91 */ 	ldx         #__i9
/* @92 */ 	ldy          #0
/* @93 */ 	jsr         PrintFixedPointScientific
/* @95 */ 	jsr         __incsp10
/* @97 */ 	ldx          #47
	jsr          __load_result
/* @98 */ 	lda         #__i9
/* @100 */ 	jsr         __result2
/* @102 */ 	ldy          #49
	jmp          __leave
.func_end___PrintScientificFormat:
	.size __PrintScientificFormat, .func_end___PrintScientificFormat-__PrintScientificFormat

	.global __PrintGeneralFormat
	.type __PrintGeneralFormat, @function

__PrintGeneralFormat:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #46
	jsr          __enter
	.byte        0x05,0x00,0x00		// Save mask i:5 b:0 l:0 x:0 f:0 
/* @18 */ 	ldx          #0
	jsr          __arg_value4_f0			// f
/* @22 */ 	ldx          #8
	jsr          __arg_value2_i4			// size
/* @26 */ 	ldx          #6
	jsr          __arg_value2_i5			// buf
/* @30 */ 	ldx          #4
	jsr          __arg_value2_i6			// precision
/* @32 */ 	lda          #39
/* @34 */ 	sta         __mem_size
/* @36 */ 	ldx          #42
	jsr          __var_addr_i7			// printer
/* @38 */ 	lda         __i7
/* @40 */ 	sta         __mem_dest
/* @42 */ 	lda         __i7+1
/* @44 */ 	sta         __mem_dest+1
/* @46 */ 	jsr         __zeromem1
/* @47 */ 	jsr         __pushf0
/* @52 */ 	jsr         __pushi7
/* @53 */ 	jsr         FixFloat
/* @55 */ 	jsr         __incsp6
/* @56 */ 	jsr         __pushi4
/* @57 */ 	jsr         __pushi5
/* @58 */ 	jsr         __pushi6
/* @63 */ 	jsr         __pushi7
/* @65 */ 	ldx         #__i8
/* @66 */ 	ldy          #0
/* @67 */ 	jsr         PrintFixedPointGeneral
/* @69 */ 	jsr         __incsp8
/* @71 */ 	ldx          #47
	jsr          __load_result
/* @72 */ 	lda         #__i8
/* @74 */ 	jsr         __result2
/* @76 */ 	ldy          #49
	jmp          __leave
.func_end___PrintGeneralFormat:
	.size __PrintGeneralFormat, .func_end___PrintGeneralFormat-__PrintGeneralFormat

	.data
	.section ".rodata", "aMS", @progbits
.str.17:
	.asciz "nan"
	.type .str.17, @object
	.size .str.17, 4

.str.18:
	.asciz "inf"
	.type .str.18, @object
	.size .str.18, 4

.lit.53:
	.byte 0x01
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.type .lit.53, @object
	.size .lit.53, 4

