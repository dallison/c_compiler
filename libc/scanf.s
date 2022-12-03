	.file   "/Users/dallison/Google Drive/c_compiler/libc/scanf.c"
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


	.local  CollectFormat
	.type CollectFormat, @function

CollectFormat:
/* @28 */ 	stx         __result
/* @30 */ 	sty         __result+1
/* @31 */ 	ldx          #7
	jsr          __enter
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @36 */ 	ldx          #2
	jsr          __arg_value2_i0			// format
/* @40 */ 	ldx          #0
	jsr          __arg_value2_i1			// p
/* @44 */ 	lda          #0
/* @45 */ 	tay         
/* @46 */ 	sta         (__i0)
/* @47 */ 	lda          #255
/* @48 */ 	iny         
/* @49 */ 	sta         (__i0), Y
/* @52 */ 	iny         
/* @53 */ 	sta         (__i0), Y
/* @55 */ 	iny         
/* @56 */ 	lda          #0
.CollectFormat_label_57:
/* @59 */ 	sta         (__i0), Y
/* @60 */ 	iny         
/* @62 */ 	cpy          #5
/* @63 */ 	bne         .CollectFormat_label_57
/* @66 */ 	lda         (__i1)
/* @72 */ 	sta         __i3
/* @74 */ 	and          #128
/* @76 */ 	beq         .CollectFormat_label_75
/* @78 */ 	lda          #255
.CollectFormat_label_75:
/* @79 */ 	sta         __i3+1
/* @82 */ 	lda         __i3
/* @83 */ 	cmp          #42
/* @84 */ 	bne         .CollectFormat_label_95
/* @85 */ 	lda         __i3+1
/* @87 */ 	bne         .CollectFormat_label_95
/* @89 */ 	lda          #1
/* @90 */ 	ldy          #0
/* @91 */ 	sta         (__i0)
/* @92 */ 	lda          #__i1
/* @94 */ 	jsr         __rinc21
.CollectFormat_label_95:
/* @98 */ 	lda         (__i1)
/* @104 */ 	sta         __i3
/* @105 */ 	and          #128
/* @107 */ 	beq         .CollectFormat_label_106
/* @108 */ 	lda          #255
.CollectFormat_label_106:
/* @109 */ 	sta         __i3+1
/* @112 */ 	ldx         __i3
/* @113 */ 	ldy         __i3+1
/* @115 */ 	jsr         __builtin_isdigit
/* @117 */ 	stz         __i4+1
/* @121 */ 	cmp          #0
/* @122 */ 	beq         .CollectFormat_label_217
/* @123 */ 	stz         __i2
/* @124 */ 	stz         __i2+1
.CollectFormat_label_125:
/* @128 */ 	lda         (__i1)
/* @134 */ 	sta         __i3
/* @135 */ 	and          #128
/* @137 */ 	beq         .CollectFormat_label_136
/* @138 */ 	lda          #255
.CollectFormat_label_136:
/* @139 */ 	sta         __i3+1
/* @142 */ 	ldx         __i3
/* @143 */ 	ldy         __i3+1
/* @144 */ 	jsr         __builtin_isdigit
/* @146 */ 	stz         __i4+1
/* @150 */ 	cmp          #0
/* @151 */ 	beq         .CollectFormat_label_210
/* @154 */ 	lda          #__i3
/* @155 */ 	ldx          #__i2
/* @157 */ 	jsr         __smul2_10
/* @160 */ 	lda         __i1
/* @161 */ 	sta         __i4
/* @162 */ 	lda         __i1+1
/* @163 */ 	sta         __i4+1
/* @164 */ 	lda          #__i1
/* @165 */ 	jsr         __rinc21
/* @170 */ 	lda         (__i4)
/* @176 */ 	sta         __i4
/* @177 */ 	and          #128
/* @179 */ 	beq         .CollectFormat_label_178
/* @180 */ 	lda          #255
.CollectFormat_label_178:
/* @181 */ 	sta         __i4+1
/* @186 */ 	clc         
/* @187 */ 	lda         __i3
/* @188 */ 	adc         __i4
/* @189 */ 	sta         __i5
/* @190 */ 	lda         __i3+1
/* @191 */ 	adc         __i4+1
/* @192 */ 	sta         __i5+1
/* @196 */ 	sec         
/* @197 */ 	lda         __i5
/* @198 */ 	sbc          #48
/* @199 */ 	sta         __i3
/* @200 */ 	lda         __i5+1
/* @201 */ 	sbc          #0
/* @202 */ 	sta         __i3+1
/* @205 */ 	lda         __i3
/* @206 */ 	sta         __i2
/* @207 */ 	lda         __i3+1
/* @208 */ 	sta         __i2+1
/* @209 */ 	bra         .CollectFormat_label_125
.CollectFormat_label_210:
/* @211 */ 	lda         __i2
/* @212 */ 	ldy          #1
/* @213 */ 	sta         (__i0), Y
/* @214 */ 	lda         __i2+1
/* @215 */ 	iny         
/* @216 */ 	sta         (__i0), Y
.CollectFormat_label_217:
/* @220 */ 	lda         (__i1)
/* @221 */ 	sta         __b0
/* @225 */ 	cmp          #76
/* @226 */ 	bne         .CollectFormat_label_399
/* @400 */ 	jmp         .CollectFormat_label_351
.CollectFormat_label_399:
/* @230 */ 	lda         __b0
/* @231 */ 	cmp          #104
/* @232 */ 	beq         .CollectFormat_label_306
/* @236 */ 	lda         __b0
/* @237 */ 	cmp          #106
/* @238 */ 	bne         .CollectFormat_label_401
/* @402 */ 	jmp         .CollectFormat_label_371
.CollectFormat_label_401:
/* @242 */ 	lda         __b0
/* @243 */ 	cmp          #108
/* @244 */ 	beq         .CollectFormat_label_260
/* @248 */ 	lda         __b0
/* @249 */ 	cmp          #116
/* @250 */ 	bne         .CollectFormat_label_403
/* @404 */ 	jmp         .CollectFormat_label_381
.CollectFormat_label_403:
/* @254 */ 	lda         __b0
/* @255 */ 	cmp          #122
/* @256 */ 	bne         .CollectFormat_label_405
/* @406 */ 	jmp         .CollectFormat_label_361
.CollectFormat_label_405:
/* @259 */ 	jmp         .CollectFormat_label_391
.CollectFormat_label_260:
/* @263 */ 	ldy          #1
/* @264 */ 	lda         (__i1), Y
/* @270 */ 	sta         __i3
/* @271 */ 	and          #128
/* @273 */ 	beq         .CollectFormat_label_272
/* @274 */ 	lda          #255
.CollectFormat_label_272:
/* @275 */ 	sta         __i3+1
/* @278 */ 	lda         __i3
/* @279 */ 	cmp          #108
/* @280 */ 	bne         .CollectFormat_label_295
/* @281 */ 	lda         __i3+1
/* @283 */ 	bne         .CollectFormat_label_295
/* @285 */ 	lda          #4
/* @286 */ 	ldy          #3
/* @287 */ 	sta         (__i0), Y
/* @288 */ 	lda          #0
/* @290 */ 	iny         
/* @291 */ 	sta         (__i0), Y
/* @292 */ 	lda          #__i1
/* @293 */ 	jsr         __rinc21
/* @294 */ 	bra         .CollectFormat_label_302
.CollectFormat_label_295:
/* @296 */ 	lda          #3
/* @297 */ 	tay         
/* @298 */ 	sta         (__i0), Y
/* @299 */ 	lda          #0
/* @300 */ 	iny         
/* @301 */ 	sta         (__i0), Y
.CollectFormat_label_302:
/* @303 */ 	lda          #__i1
/* @304 */ 	jsr         __rinc21
/* @305 */ 	jmp         .CollectFormat_label_391
.CollectFormat_label_306:
/* @309 */ 	ldy          #1
/* @310 */ 	lda         (__i1), Y
/* @316 */ 	sta         __i3
/* @317 */ 	and          #128
/* @319 */ 	beq         .CollectFormat_label_318
/* @320 */ 	lda          #255
.CollectFormat_label_318:
/* @321 */ 	sta         __i3+1
/* @324 */ 	lda         __i3
/* @325 */ 	cmp          #104
/* @326 */ 	bne         .CollectFormat_label_340
/* @327 */ 	lda         __i3+1
/* @329 */ 	bne         .CollectFormat_label_340
/* @331 */ 	lda          #1
/* @332 */ 	ldy          #3
/* @333 */ 	sta         (__i0), Y
/* @334 */ 	dec          A
/* @335 */ 	iny         
/* @336 */ 	sta         (__i0), Y
/* @337 */ 	lda          #__i1
/* @338 */ 	jsr         __rinc21
/* @339 */ 	bra         .CollectFormat_label_347
.CollectFormat_label_340:
/* @341 */ 	lda          #2
/* @342 */ 	ldy          #3
/* @343 */ 	sta         (__i0), Y
/* @344 */ 	lda          #0
/* @345 */ 	iny         
/* @346 */ 	sta         (__i0), Y
.CollectFormat_label_347:
/* @348 */ 	lda          #__i1
/* @349 */ 	jsr         __rinc21
/* @350 */ 	bra         .CollectFormat_label_391
.CollectFormat_label_351:
/* @352 */ 	lda          #5
/* @353 */ 	ldy          #3
/* @354 */ 	sta         (__i0), Y
/* @355 */ 	lda          #0
/* @356 */ 	iny         
/* @357 */ 	sta         (__i0), Y
/* @358 */ 	lda          #__i1
/* @359 */ 	jsr         __rinc21
/* @360 */ 	bra         .CollectFormat_label_391
.CollectFormat_label_361:
/* @362 */ 	lda          #7
/* @363 */ 	ldy          #3
/* @364 */ 	sta         (__i0), Y
/* @365 */ 	lda          #0
/* @366 */ 	iny         
/* @367 */ 	sta         (__i0), Y
/* @368 */ 	lda          #__i1
/* @369 */ 	jsr         __rinc21
/* @370 */ 	bra         .CollectFormat_label_391
.CollectFormat_label_371:
/* @372 */ 	lda          #6
/* @373 */ 	ldy          #3
/* @374 */ 	sta         (__i0), Y
/* @375 */ 	lda          #0
/* @376 */ 	iny         
/* @377 */ 	sta         (__i0), Y
/* @378 */ 	lda          #__i1
/* @379 */ 	jsr         __rinc21
/* @380 */ 	bra         .CollectFormat_label_391
.CollectFormat_label_381:
/* @382 */ 	lda          #8
/* @383 */ 	ldy          #3
/* @384 */ 	sta         (__i0), Y
/* @385 */ 	lda          #0
/* @386 */ 	iny         
/* @387 */ 	sta         (__i0), Y
/* @388 */ 	lda          #__i1
/* @389 */ 	jsr         __rinc21
.CollectFormat_label_391:
/* @392 */ 	ldx          #8
	jsr          __load_result
/* @393 */ 	lda         #__i1
/* @395 */ 	jsr         __result2
/* @397 */ 	ldy          #10
	jmp          __leave
.func_end_CollectFormat:
	.size CollectFormat, .func_end_CollectFormat-CollectFormat

	.local  SkipInputWhiteSpace
	.type SkipInputWhiteSpace, @function

SkipInputWhiteSpace:
/* @5 */ 	ldx          #7
	jsr          __enter
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
/* @13 */ 	ldx          #0
	jsr          __arg_value2_i5			// get
/* @17 */ 	ldx          #4
	jsr          __arg_value2_i6			// data
/* @21 */ 	ldx          #2
	jsr          __arg_value2_i7			// unget
.SkipInputWhiteSpace_label_22:
/* @23 */ 	jsr         __pushi6
/* @25 */ 	ldx         #__i4
/* @26 */ 	ldy          #0
/* @29 */ 	jsr         .SkipInputWhiteSpace_label_27
/* @30 */ 	bra         .SkipInputWhiteSpace_label_28
.SkipInputWhiteSpace_label_27:
/* @31 */ 	jmp         (__i5)
.SkipInputWhiteSpace_label_28:
/* @33 */ 	jsr         __incsp2
/* @35 */ 	lda         __i4
/* @36 */ 	cmp          #255
/* @37 */ 	bne         .SkipInputWhiteSpace_label_34
/* @39 */ 	lda         __i4+1
/* @40 */ 	cmp          #255
/* @41 */ 	beq         .SkipInputWhiteSpace_label_97
.SkipInputWhiteSpace_label_34:
/* @45 */ 	ldx         __i4
/* @46 */ 	ldy         __i4+1
/* @48 */ 	jsr         __builtin_isspace
/* @49 */ 	sta         __i0
/* @50 */ 	stz         __i0+1
/* @54 */ 	lda         __i0+1
/* @56 */ 	and          #128
/* @57 */ 	ora         __i0
/* @65 */ 	beq         .SkipInputWhiteSpace_label_62
/* @67 */ 	lda          #255
.SkipInputWhiteSpace_label_62:
/* @68 */ 	inc          A
/* @73 */ 	beq         .SkipInputWhiteSpace_label_94
/* @74 */ 	jsr         __pushi6
/* @77 */ 	lda         __i4+1
/* @78 */ 	and          #128
/* @79 */ 	ora         __i4
/* @85 */ 	jsr         __pusha
/* @88 */ 	jsr         .SkipInputWhiteSpace_label_86
/* @89 */ 	bra         .SkipInputWhiteSpace_label_87
.SkipInputWhiteSpace_label_86:
/* @90 */ 	jmp         (__i7)
.SkipInputWhiteSpace_label_87:
/* @92 */ 	jsr         __incsp4
/* @93 */ 	bra         .SkipInputWhiteSpace_label_97
.SkipInputWhiteSpace_label_94:
/* @96 */ 	bra         .SkipInputWhiteSpace_label_22
.SkipInputWhiteSpace_label_97:
/* @98 */ 	ldy          #10
	jmp          __leave_void
.func_end_SkipInputWhiteSpace:
	.size SkipInputWhiteSpace, .func_end_SkipInputWhiteSpace-SkipInputWhiteSpace

	.local  WriteInt
	.type WriteInt, @function

WriteInt:
/* @8 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @12 */ 	ldx          #12
	jsr          __arg_value2_i0			// fmt
/* @16 */ 	ldx          #8
	jsr          __arg_value1_b0			// is_unsigned
/* @20 */ 	ldx          #10
	jsr          __arg_value2_i1			// ptr
/* @24 */ 	ldx          #0
	jsr          __arg_value8_x0			// v
/* @28 */ 	ldy          #3
/* @29 */ 	lda         (__i0), Y
/* @30 */ 	sta         __i2
/* @32 */ 	iny         
/* @33 */ 	lda         (__i0), Y
/* @35 */ 	sta         __i2+1
/* @39 */ 	lda         __i2
/* @40 */ 	sta         __b1
/* @42 */ 	lda         #__b1
/* @44 */ 	jsr         __jump_table1
/* @45 */ 	.short .WriteInt_label_54
/* @46 */ 	.short .WriteInt_label_87
/* @47 */ 	.short .WriteInt_label_96
/* @48 */ 	.short .WriteInt_label_128
/* @49 */ 	.short .WriteInt_label_181
/* @50 */ 	.short .WriteInt_label_55
/* @51 */ 	.short .WriteInt_label_205
/* @52 */ 	.short .WriteInt_label_219
/* @53 */ 	.short .WriteInt_label_233
.WriteInt_label_54:
.WriteInt_label_55:
/* @56 */ 	lda         __b0
/* @58 */ 	beq         .WriteInt_label_72
/* @60 */ 	lda         __x0
/* @61 */ 	sta         __i0
/* @62 */ 	lda         __x0+1
/* @63 */ 	sta         __i0+1
/* @66 */ 	lda         __i0
/* @67 */ 	sta         (__i1)
/* @68 */ 	lda         __i0+1
/* @69 */ 	ldy          #1
/* @70 */ 	sta         (__i1), Y
/* @71 */ 	bra         .WriteInt_label_85
.WriteInt_label_72:
/* @74 */ 	lda         __x0
/* @75 */ 	sta         __i0
/* @76 */ 	lda         __x0+1
/* @77 */ 	sta         __i0+1
/* @80 */ 	lda         __i0
/* @81 */ 	sta         (__i1)
/* @82 */ 	lda         __i0+1
/* @83 */ 	ldy          #1
/* @84 */ 	sta         (__i1), Y
.WriteInt_label_85:
/* @86 */ 	jmp         .WriteInt_label_257
.WriteInt_label_87:
/* @89 */ 	lda         __x0
/* @94 */ 	sta         (__i1)
/* @95 */ 	jmp         .WriteInt_label_257
.WriteInt_label_96:
/* @97 */ 	lda         __b0
/* @99 */ 	beq         .WriteInt_label_113
/* @101 */ 	lda         __x0
/* @102 */ 	sta         __i0
/* @103 */ 	lda         __x0+1
/* @104 */ 	sta         __i0+1
/* @107 */ 	lda         __i0
/* @108 */ 	sta         (__i1)
/* @109 */ 	lda         __i0+1
/* @110 */ 	ldy          #1
/* @111 */ 	sta         (__i1), Y
/* @112 */ 	bra         .WriteInt_label_126
.WriteInt_label_113:
/* @115 */ 	lda         __x0
/* @116 */ 	sta         __i0
/* @117 */ 	lda         __x0+1
/* @118 */ 	sta         __i0+1
/* @121 */ 	lda         __i0
/* @122 */ 	sta         (__i1)
/* @123 */ 	lda         __i0+1
/* @124 */ 	ldy          #1
/* @125 */ 	sta         (__i1), Y
.WriteInt_label_126:
/* @127 */ 	jmp         .WriteInt_label_257
.WriteInt_label_128:
/* @129 */ 	lda         __b0
/* @131 */ 	beq         .WriteInt_label_154
/* @133 */ 	lda         __x0
/* @134 */ 	sta         __l0
/* @135 */ 	lda         __x0+1
/* @136 */ 	sta         __l0+1
/* @138 */ 	lda         __x0+2
/* @139 */ 	sta         __l0+2
/* @140 */ 	lda         __x0+3
/* @141 */ 	sta         __l0+3
/* @144 */ 	ldy          #3
/* @145 */ 	ldx          #3
.WriteInt_label_146:
/* @148 */ 	lda         __l0, X
/* @149 */ 	sta         (__i1), Y
/* @150 */ 	dey         
/* @151 */ 	dex         
/* @152 */ 	bpl         .WriteInt_label_146
/* @153 */ 	bra         .WriteInt_label_179
.WriteInt_label_154:
/* @157 */ 	lda         __x0
/* @158 */ 	sta         __l0
/* @159 */ 	lda         __x0+1
/* @160 */ 	sta         __l0+1
/* @161 */ 	lda         __x0+2
/* @162 */ 	sta         __l0+2
/* @164 */ 	lda         __x0+7
/* @166 */ 	and          #128
/* @167 */ 	ora         __x0+3
/* @168 */ 	sta         __l0+3
/* @171 */ 	ldy          #3
/* @172 */ 	ldx          #3
.WriteInt_label_173:
/* @174 */ 	lda         __l0, X
/* @175 */ 	sta         (__i1), Y
/* @176 */ 	dey         
/* @177 */ 	dex         
/* @178 */ 	bpl         .WriteInt_label_173
.WriteInt_label_179:
/* @180 */ 	bra         .WriteInt_label_257
.WriteInt_label_181:
/* @182 */ 	lda         __b0
/* @184 */ 	beq         .WriteInt_label_194
/* @185 */ 	ldy          #7
/* @186 */ 	ldx          #7
.WriteInt_label_187:
/* @188 */ 	lda         __x0, X
/* @189 */ 	sta         (__i1), Y
/* @190 */ 	dey         
/* @191 */ 	dex         
/* @192 */ 	bpl         .WriteInt_label_187
/* @193 */ 	bra         .WriteInt_label_203
.WriteInt_label_194:
/* @195 */ 	ldy          #7
/* @196 */ 	ldx          #7
.WriteInt_label_197:
/* @198 */ 	lda         __x0, X
/* @199 */ 	sta         (__i1), Y
/* @200 */ 	dey         
/* @201 */ 	dex         
/* @202 */ 	bpl         .WriteInt_label_197
.WriteInt_label_203:
/* @204 */ 	bra         .WriteInt_label_257
.WriteInt_label_205:
/* @207 */ 	lda         __x0
/* @208 */ 	sta         __i0
/* @209 */ 	lda         __x0+1
/* @210 */ 	sta         __i0+1
/* @213 */ 	lda         __i0
/* @214 */ 	sta         (__i1)
/* @215 */ 	lda         __i0+1
/* @216 */ 	ldy          #1
/* @217 */ 	sta         (__i1), Y
/* @218 */ 	bra         .WriteInt_label_257
.WriteInt_label_219:
/* @221 */ 	lda         __x0
/* @222 */ 	sta         __i0
/* @223 */ 	lda         __x0+1
/* @224 */ 	sta         __i0+1
/* @227 */ 	lda         __i0
/* @228 */ 	sta         (__i1)
/* @229 */ 	lda         __i0+1
/* @230 */ 	ldy          #1
/* @231 */ 	sta         (__i1), Y
/* @232 */ 	bra         .WriteInt_label_257
.WriteInt_label_233:
/* @236 */ 	lda         __x0
/* @237 */ 	sta         __l0
/* @238 */ 	lda         __x0+1
/* @239 */ 	sta         __l0+1
/* @240 */ 	lda         __x0+2
/* @241 */ 	sta         __l0+2
/* @242 */ 	lda         __x0+7
/* @243 */ 	and          #128
/* @244 */ 	ora         __x0+3
/* @245 */ 	sta         __l0+3
/* @248 */ 	ldy          #3
/* @249 */ 	ldx          #3
.WriteInt_label_250:
/* @251 */ 	lda         __l0, X
/* @252 */ 	sta         (__i1), Y
/* @253 */ 	dey         
/* @254 */ 	dex         
/* @255 */ 	bpl         .WriteInt_label_250
.WriteInt_label_257:
/* @258 */ 	ldy          #8
	jmp          __leave_leaf_void_nomask
.func_end_WriteInt:
	.size WriteInt, .func_end_WriteInt-WriteInt

	.local  ConvertDecimal
	.type ConvertDecimal, @function

ConvertDecimal:
/* @36 */ 	stx         __result
/* @38 */ 	sty         __result+1
/* @39 */ 	ldx          #13
	jsr          __enter
	.byte        0xca,0x60,0x00		// Save mask i:10 b:6 l:0 x:3 f:0 
/* @47 */ 	ldx          #6
	jsr          __arg_value1_b2			// is_unsigned
/* @54 */ 	ldx          #12
	jsr          __arg_value2_i6			// fmt
/* @69 */ 	ldx          #0
	jsr          __arg_value2_i8			// get
/* @73 */ 	ldx          #8
	jsr          __arg_value2_i9			// data
/* @76 */ 	ldx          #4
	jsr          __arg_value2_i10			// base
/* @80 */ 	ldx          #2
	jsr          __arg_value2_i11			// unget
/* @86 */ 	ldx          #10
	jsr          __arg_value2_i12			// ptr
/* @91 */ 	stz         __i0
/* @92 */ 	stz         __i0+1
/* @93 */ 	lda          #__i0
/* @95 */ 	ldx          #9
/* @97 */ 	jsr         __set_var_value2
/* @100 */ 	ldy          #1
/* @101 */ 	lda         (__i6), Y
/* @102 */ 	sta         __i0
/* @104 */ 	iny         
/* @105 */ 	lda         (__i6), Y
/* @106 */ 	sta         __i0+1
/* @109 */ 	lda         __i0
/* @110 */ 	cmp          #255
/* @111 */ 	bne         .ConvertDecimal_label_122
/* @112 */ 	lda         __i0+1
/* @113 */ 	cmp          #255
/* @114 */ 	bne         .ConvertDecimal_label_122
/* @118 */ 	stz         __i4
/* @119 */ 	lda          #4
/* @120 */ 	sta         __i4+1
/* @121 */ 	bra         .ConvertDecimal_label_137
.ConvertDecimal_label_122:
/* @125 */ 	ldy          #1
/* @126 */ 	lda         (__i6), Y
/* @127 */ 	sta         __i0
/* @128 */ 	iny         
/* @129 */ 	lda         (__i6), Y
/* @130 */ 	sta         __i0+1
/* @133 */ 	lda         __i0
/* @134 */ 	sta         __i4
/* @135 */ 	lda         __i0+1
/* @136 */ 	sta         __i4+1
.ConvertDecimal_label_137:
/* @140 */ 	lda         __i4
/* @141 */ 	sta         __i5
/* @142 */ 	lda         __i4+1
/* @143 */ 	sta         __i5+1
/* @145 */ 	ldx          #7
/* @146 */ 	lda          #0
.ConvertDecimal_label_147:
/* @149 */ 	sta         __x1, X
/* @150 */ 	dex         
/* @151 */ 	bpl         .ConvertDecimal_label_147
/* @152 */ 	lda          #1
/* @153 */ 	sta         __b3
/* @156 */ 	stz         __b0
/* @157 */ 	lda          #__b0
/* @159 */ 	ldx          #4
/* @161 */ 	jsr         __set_var_value1
/* @164 */ 	stz         __b0
/* @165 */ 	lda          #__b0
/* @167 */ 	ldx          #5
/* @168 */ 	jsr         __set_var_value1
/* @171 */ 	stz         __b0
/* @172 */ 	lda          #__b0
/* @174 */ 	ldx          #6
/* @175 */ 	jsr         __set_var_value1
.ConvertDecimal_label_176:
/* @178 */ 	ldx          #9
	jsr          __var_value2_i0			// length
/* @181 */ 	lda         __i0
/* @182 */ 	cmp         __i5
/* @183 */ 	lda         __i0+1
/* @184 */ 	sbc         __i5+1
/* @185 */ 	bvc         .ConvertDecimal_label_180
/* @187 */ 	eor          #128
.ConvertDecimal_label_180:
/* @188 */ 	bmi         .ConvertDecimal_label_1298
/* @1299 */ 	jmp         .ConvertDecimal_label_1220
.ConvertDecimal_label_1298:
/* @189 */ 	jsr         __pushi9
/* @190 */ 	ldx         #__i7
/* @191 */ 	ldy          #0
/* @194 */ 	jsr         .ConvertDecimal_label_192
/* @195 */ 	bra         .ConvertDecimal_label_193
.ConvertDecimal_label_192:
/* @196 */ 	jmp         (__i8)
.ConvertDecimal_label_193:
/* @198 */ 	jsr         __incsp2
/* @200 */ 	lda         __i7
/* @201 */ 	cmp          #255
/* @202 */ 	bne         .ConvertDecimal_label_229
/* @203 */ 	lda         __i7+1
/* @204 */ 	cmp          #255
/* @205 */ 	bne         .ConvertDecimal_label_229
/* @208 */ 	ldx          #9
	jsr          __var_value2_i0			// length
/* @211 */ 	lda          #0
/* @212 */ 	cmp         __i0
/* @214 */ 	sbc         __i0+1
/* @215 */ 	bvc         .ConvertDecimal_label_210
/* @216 */ 	eor          #128
.ConvertDecimal_label_210:
/* @217 */ 	bpl         .ConvertDecimal_label_1300
/* @1301 */ 	jmp         .ConvertDecimal_label_1221
.ConvertDecimal_label_1300:
/* @221 */ 	stz         __b0
/* @222 */ 	ldx          #14
	jsr          __load_result
/* @223 */ 	lda         #__b0
/* @225 */ 	jsr         __result1
.ConvertDecimal_label_226:
/* @227 */ 	ldy          #16
	jmp          __leave
.ConvertDecimal_label_229:
/* @231 */ 	ldx          #9
	jsr          __var_addr_i13			// length
/* @233 */ 	lda          #__i13
/* @235 */ 	jsr         __inc21
/* @239 */ 	lda         __b3
/* @240 */ 	sta         __b4
/* @244 */ 	beq         .ConvertDecimal_label_257
/* @248 */ 	ldx          #1
/* @249 */ 	lda         __i7
/* @250 */ 	cmp          #45
/* @251 */ 	bne         .ConvertDecimal_label_246
/* @252 */ 	lda         __i7+1
/* @254 */ 	beq         .ConvertDecimal_label_247
.ConvertDecimal_label_246:
/* @255 */ 	dex         
.ConvertDecimal_label_247:
/* @256 */ 	stx         __b4
.ConvertDecimal_label_257:
/* @259 */ 	lda         __b4
/* @261 */ 	beq         .ConvertDecimal_label_271
/* @264 */ 	lda          #1
/* @265 */ 	sta         __b0
/* @266 */ 	lda          #__b0
/* @267 */ 	ldx          #4
/* @268 */ 	jsr         __set_var_value1
/* @269 */ 	stz         __b3
/* @270 */ 	jmp         .ConvertDecimal_label_1218
.ConvertDecimal_label_271:
/* @275 */ 	lda         __b3
/* @276 */ 	sta         __b5
/* @280 */ 	beq         .ConvertDecimal_label_293
/* @284 */ 	ldx          #1
/* @285 */ 	lda         __i7
/* @286 */ 	cmp          #43
/* @287 */ 	bne         .ConvertDecimal_label_282
/* @288 */ 	lda         __i7+1
/* @290 */ 	beq         .ConvertDecimal_label_283
.ConvertDecimal_label_282:
/* @291 */ 	dex         
.ConvertDecimal_label_283:
/* @292 */ 	stx         __b5
.ConvertDecimal_label_293:
/* @295 */ 	lda         __b5
/* @297 */ 	beq         .ConvertDecimal_label_300
/* @298 */ 	stz         __b3
/* @299 */ 	jmp         .ConvertDecimal_label_1217
.ConvertDecimal_label_300:
/* @303 */ 	lda         __i10+1
/* @304 */ 	and          #128
/* @305 */ 	ora         __i10
/* @306 */ 	sta         __b6
/* @310 */ 	bne         .ConvertDecimal_label_1302
/* @1303 */ 	jmp         .ConvertDecimal_label_989
.ConvertDecimal_label_1302:
/* @313 */ 	lda         __b6
/* @314 */ 	cmp          #8
/* @315 */ 	beq         .ConvertDecimal_label_330
/* @319 */ 	lda         __b6
/* @320 */ 	cmp          #10
/* @321 */ 	bne         .ConvertDecimal_label_1304
/* @1305 */ 	jmp         .ConvertDecimal_label_489
.ConvertDecimal_label_1304:
/* @325 */ 	lda         __b6
/* @326 */ 	cmp          #16
/* @327 */ 	bne         .ConvertDecimal_label_1306
/* @1307 */ 	jmp         .ConvertDecimal_label_594
.ConvertDecimal_label_1306:
/* @329 */ 	jmp         .ConvertDecimal_label_1216
.ConvertDecimal_label_330:
/* @335 */ 	ldx          #0
/* @336 */ 	lda         __i7+1
/* @337 */ 	bmi         .ConvertDecimal_label_334
/* @338 */ 	inx         
.ConvertDecimal_label_334:
/* @339 */ 	stx         __b7
/* @341 */ 	txa         
/* @342 */ 	cmp          #0
/* @343 */ 	beq         .ConvertDecimal_label_357
/* @346 */ 	ldx          #0
/* @348 */ 	lda         __i7
/* @349 */ 	cmp          #8
/* @350 */ 	lda         __i7+1
/* @351 */ 	sbc          #0
/* @352 */ 	bvc         .ConvertDecimal_label_347
/* @353 */ 	eor          #128
.ConvertDecimal_label_347:
/* @354 */ 	bpl         .ConvertDecimal_label_345
/* @355 */ 	inx         
.ConvertDecimal_label_345:
/* @356 */ 	stx         __b7
.ConvertDecimal_label_357:
/* @359 */ 	lda         __b7
/* @361 */ 	bne         .ConvertDecimal_label_1308
/* @1309 */ 	jmp         .ConvertDecimal_label_462
.ConvertDecimal_label_1308:
/* @364 */ 	lda         __x1
/* @365 */ 	asl          A
/* @366 */ 	sta         __x0
/* @367 */ 	lda         __x1+1
/* @368 */ 	rol          A
/* @369 */ 	sta         __x0+1
/* @370 */ 	lda         __x1+2
/* @371 */ 	rol          A
/* @372 */ 	sta         __x0+2
/* @374 */ 	lda         __x1+3
/* @375 */ 	rol          A
/* @376 */ 	sta         __x0+3
/* @377 */ 	lda         __x1+4
/* @378 */ 	rol          A
/* @379 */ 	sta         __x0+4
/* @380 */ 	lda         __x1+5
/* @381 */ 	rol          A
/* @382 */ 	sta         __x0+5
/* @383 */ 	lda         __x1+6
/* @384 */ 	rol          A
/* @385 */ 	sta         __x0+6
/* @386 */ 	lda         __x1+7
/* @387 */ 	rol          A
/* @388 */ 	sta         __x0+7
/* @389 */ 	ldx          #2
.ConvertDecimal_label_390:
/* @391 */ 	asl         __x0
/* @392 */ 	rol         __x0+1
/* @393 */ 	rol         __x0+2
/* @394 */ 	rol         __x0+3
/* @395 */ 	rol         __x0+4
/* @396 */ 	rol         __x0+5
/* @397 */ 	rol         __x0+6
/* @398 */ 	rol         __x0+7
/* @399 */ 	dex         
/* @400 */ 	bne         .ConvertDecimal_label_390
/* @403 */ 	sec         
/* @404 */ 	lda         __i7
/* @405 */ 	sbc          #48
/* @406 */ 	sta         __i0
/* @407 */ 	lda         __i7+1
/* @408 */ 	sbc          #0
/* @409 */ 	sta         __i0+1
/* @413 */ 	lda         __i0
/* @414 */ 	sta         __x2
/* @415 */ 	lda         __i0+1
/* @416 */ 	sta         __x2+1
/* @417 */ 	lda          #0
/* @418 */ 	ldy          #7
.ConvertDecimal_label_419:
/* @420 */ 	sta         __x2, Y
/* @421 */ 	dey         
/* @422 */ 	cpy          #1
/* @423 */ 	bne         .ConvertDecimal_label_419
/* @428 */ 	lda         __x0
/* @429 */ 	ora         __x2
/* @430 */ 	sta         __x3
/* @431 */ 	lda         __x0+1
/* @432 */ 	ora         __x2+1
/* @433 */ 	sta         __x3+1
/* @434 */ 	lda         __x0+2
/* @435 */ 	ora         __x2+2
/* @436 */ 	sta         __x3+2
/* @437 */ 	lda         __x0+3
/* @438 */ 	ora         __x2+3
/* @439 */ 	sta         __x3+3
/* @440 */ 	lda         __x0+4
/* @441 */ 	ora         __x2+4
/* @442 */ 	sta         __x3+4
/* @443 */ 	lda         __x0+5
/* @444 */ 	ora         __x2+5
/* @445 */ 	sta         __x3+5
/* @446 */ 	lda         __x0+6
/* @447 */ 	ora         __x2+6
/* @448 */ 	sta         __x3+6
/* @449 */ 	lda         __x0+7
/* @450 */ 	ora         __x2+7
/* @451 */ 	sta         __x3+7
/* @454 */ 	ldx          #7
.ConvertDecimal_label_455:
/* @456 */ 	lda         __x3, X
/* @457 */ 	sta         __x1, X
/* @458 */ 	dex         
/* @459 */ 	bpl         .ConvertDecimal_label_455
/* @461 */ 	jmp         .ConvertDecimal_label_1216
.ConvertDecimal_label_462:
/* @463 */ 	jsr         __pushi9
/* @466 */ 	lda         __i7+1
/* @467 */ 	and          #128
/* @468 */ 	ora         __i7
/* @474 */ 	jsr         __pusha
/* @477 */ 	jsr         .ConvertDecimal_label_475
/* @478 */ 	bra         .ConvertDecimal_label_476
.ConvertDecimal_label_475:
/* @479 */ 	jmp         (__i11)
.ConvertDecimal_label_476:
/* @481 */ 	jsr         __incsp4
/* @485 */ 	lda          #__i13
/* @487 */ 	jsr         __dec21
/* @488 */ 	jmp         .ConvertDecimal_label_1221
.ConvertDecimal_label_489:
/* @491 */ 	ldx         __i7
/* @492 */ 	ldy         __i7+1
/* @494 */ 	jsr         __builtin_isdigit
/* @496 */ 	stz         __i0+1
/* @500 */ 	cmp          #0
/* @501 */ 	beq         .ConvertDecimal_label_570
/* @506 */ 	ldx          #7
.ConvertDecimal_label_507:
/* @508 */ 	lda         .lit.37, X
/* @509 */ 	sta         __x0, X
/* @510 */ 	dex         
/* @511 */ 	bpl         .ConvertDecimal_label_507
/* @515 */ 	lda          #__x2
/* @516 */ 	ldx          #__x1
/* @517 */ 	ldy          #__x0
/* @519 */ 	jsr         __umul8
/* @521 */ 	lda         __i7
/* @522 */ 	sta         __x0
/* @523 */ 	lda         __i7+1
/* @524 */ 	sta         __x0+1
/* @525 */ 	lda          #0
/* @526 */ 	ldy          #7
.ConvertDecimal_label_527:
/* @528 */ 	sta         __x0, Y
/* @529 */ 	dey         
/* @530 */ 	cpy          #1
/* @531 */ 	bne         .ConvertDecimal_label_527
/* @536 */ 	clc         
/* @537 */ 	ldy          #8
/* @538 */ 	ldx          #0
.ConvertDecimal_label_539:
/* @540 */ 	lda         __x2, X
/* @541 */ 	adc         __x0, X
/* @542 */ 	sta         __x3, X
/* @543 */ 	inx         
/* @544 */ 	dey         
/* @545 */ 	bne         .ConvertDecimal_label_539
/* @549 */ 	sec         
/* @551 */ 	ldy          #8
/* @552 */ 	ldx          #0
.ConvertDecimal_label_553:
/* @554 */ 	lda         __x3, X
/* @555 */ 	sbc         .lit.36, X
/* @556 */ 	sta         __x0, X
/* @557 */ 	inx         
/* @558 */ 	dey         
/* @559 */ 	bne         .ConvertDecimal_label_553
/* @562 */ 	ldx          #7
.ConvertDecimal_label_563:
/* @564 */ 	lda         __x0, X
/* @565 */ 	sta         __x1, X
/* @566 */ 	dex         
/* @567 */ 	bpl         .ConvertDecimal_label_563
/* @569 */ 	jmp         .ConvertDecimal_label_1216
.ConvertDecimal_label_570:
/* @571 */ 	jsr         __pushi9
/* @574 */ 	lda         __i7+1
/* @575 */ 	and          #128
/* @576 */ 	ora         __i7
/* @581 */ 	jsr         __pusha
/* @584 */ 	jsr         .ConvertDecimal_label_582
/* @585 */ 	bra         .ConvertDecimal_label_583
.ConvertDecimal_label_582:
/* @586 */ 	jmp         (__i11)
.ConvertDecimal_label_583:
/* @587 */ 	jsr         __incsp4
/* @591 */ 	lda          #__i13
/* @592 */ 	jsr         __dec21
/* @593 */ 	jmp         .ConvertDecimal_label_1221
.ConvertDecimal_label_594:
/* @596 */ 	lda         __i7
/* @597 */ 	cmp          #48
/* @598 */ 	bne         .ConvertDecimal_label_629
/* @599 */ 	lda         __i7+1
/* @601 */ 	bne         .ConvertDecimal_label_629
/* @604 */ 	ldx          #6
	jsr          __var_value1_b0			// prefix_done
/* @609 */ 	lda         __b0
/* @611 */ 	beq         .ConvertDecimal_label_608
/* @613 */ 	lda          #255
.ConvertDecimal_label_608:
/* @614 */ 	inc          A
/* @619 */ 	beq         .ConvertDecimal_label_627
/* @622 */ 	lda          #1
/* @623 */ 	sta         __b0
/* @624 */ 	lda          #__b0
/* @625 */ 	ldx          #5
/* @626 */ 	jsr         __set_var_value1
.ConvertDecimal_label_627:
/* @628 */ 	jmp         .ConvertDecimal_label_987
.ConvertDecimal_label_629:
/* @633 */ 	ldx          #5
	jsr          __var_value1_b0			// prefix_ok
/* @636 */ 	lda         __b0
/* @637 */ 	sta         __b6
/* @641 */ 	beq         .ConvertDecimal_label_676
/* @647 */ 	ldx          #1
/* @648 */ 	lda         __i7
/* @649 */ 	cmp          #88
/* @650 */ 	bne         .ConvertDecimal_label_645
/* @651 */ 	lda         __i7+1
/* @653 */ 	beq         .ConvertDecimal_label_646
.ConvertDecimal_label_645:
/* @654 */ 	dex         
.ConvertDecimal_label_646:
/* @655 */ 	stx         __b0
/* @657 */ 	txa         
/* @658 */ 	bne         .ConvertDecimal_label_671
/* @662 */ 	ldx          #1
/* @663 */ 	lda         __i7
/* @664 */ 	cmp          #120
/* @665 */ 	bne         .ConvertDecimal_label_660
/* @666 */ 	lda         __i7+1
/* @668 */ 	beq         .ConvertDecimal_label_661
.ConvertDecimal_label_660:
/* @669 */ 	dex         
.ConvertDecimal_label_661:
/* @670 */ 	stx         __b0
.ConvertDecimal_label_671:
/* @674 */ 	lda         __b0
/* @675 */ 	sta         __b6
.ConvertDecimal_label_676:
/* @678 */ 	lda         __b6
/* @680 */ 	beq         .ConvertDecimal_label_695
/* @683 */ 	stz         __b0
/* @684 */ 	lda          #__b0
/* @685 */ 	ldx          #5
/* @686 */ 	jsr         __set_var_value1
/* @689 */ 	lda          #1
/* @690 */ 	sta         __b0
/* @691 */ 	lda          #__b0
/* @692 */ 	ldx          #6
/* @693 */ 	jsr         __set_var_value1
/* @694 */ 	jmp         .ConvertDecimal_label_986
.ConvertDecimal_label_695:
/* @697 */ 	ldx         __i7
/* @698 */ 	ldy         __i7+1
/* @700 */ 	jsr         __builtin_isxdigit
/* @702 */ 	stz         __i0+1
/* @706 */ 	cmp          #0
/* @707 */ 	bne         .ConvertDecimal_label_1310
/* @1311 */ 	jmp         .ConvertDecimal_label_961
.ConvertDecimal_label_1310:
/* @709 */ 	ldx         __i7
/* @710 */ 	ldy         __i7+1
/* @712 */ 	jsr         __builtin_toupper
/* @713 */ 	sta         __i0
/* @714 */ 	stz         __i0+1
/* @716 */ 	lda          #__i0
/* @717 */ 	ldx          #7
/* @718 */ 	jsr         __set_var_value1
/* @720 */ 	ldx          #7
	jsr          __var_value1_b0			// uch
/* @724 */ 	lda         __b0
/* @725 */ 	sta         __i0
/* @726 */ 	and          #128
/* @728 */ 	beq         .ConvertDecimal_label_727
/* @729 */ 	lda          #255
.ConvertDecimal_label_727:
/* @730 */ 	sta         __i0+1
/* @733 */ 	lda          #65
/* @734 */ 	cmp         __i0
/* @735 */ 	lda          #0
/* @736 */ 	sbc         __i0+1
/* @737 */ 	bvc         .ConvertDecimal_label_732
/* @738 */ 	eor          #128
.ConvertDecimal_label_732:
/* @739 */ 	bmi         .ConvertDecimal_label_1312
/* @1313 */ 	jmp         .ConvertDecimal_label_861
.ConvertDecimal_label_1312:
/* @742 */ 	lda         __x1
/* @743 */ 	asl          A
/* @744 */ 	sta         __x0
/* @745 */ 	lda         __x1+1
/* @746 */ 	rol          A
/* @747 */ 	sta         __x0+1
/* @748 */ 	lda         __x1+2
/* @749 */ 	rol          A
/* @750 */ 	sta         __x0+2
/* @751 */ 	lda         __x1+3
/* @752 */ 	rol          A
/* @753 */ 	sta         __x0+3
/* @754 */ 	lda         __x1+4
/* @755 */ 	rol          A
/* @756 */ 	sta         __x0+4
/* @757 */ 	lda         __x1+5
/* @758 */ 	rol          A
/* @759 */ 	sta         __x0+5
/* @760 */ 	lda         __x1+6
/* @761 */ 	rol          A
/* @762 */ 	sta         __x0+6
/* @763 */ 	lda         __x1+7
/* @764 */ 	rol          A
/* @765 */ 	sta         __x0+7
/* @766 */ 	ldx          #3
.ConvertDecimal_label_767:
/* @768 */ 	asl         __x0
/* @769 */ 	rol         __x0+1
/* @770 */ 	rol         __x0+2
/* @771 */ 	rol         __x0+3
/* @772 */ 	rol         __x0+4
/* @773 */ 	rol         __x0+5
/* @774 */ 	rol         __x0+6
/* @775 */ 	rol         __x0+7
/* @776 */ 	dex         
/* @777 */ 	bne         .ConvertDecimal_label_767
/* @779 */ 	ldx          #7
	jsr          __var_value1_b0			// uch
/* @783 */ 	lda         __b0
/* @784 */ 	sta         __i0
/* @785 */ 	and          #128
/* @787 */ 	beq         .ConvertDecimal_label_786
/* @788 */ 	lda          #255
.ConvertDecimal_label_786:
/* @789 */ 	sta         __i0+1
/* @793 */ 	sec         
/* @794 */ 	lda         __i0
/* @795 */ 	sbc          #65
/* @796 */ 	sta         __i1
/* @797 */ 	lda         __i0+1
/* @798 */ 	sbc          #0
/* @799 */ 	sta         __i1+1
/* @803 */ 	clc         
/* @804 */ 	lda         __i1
/* @805 */ 	adc          #10
/* @806 */ 	sta         __i0
/* @807 */ 	lda         __i1+1
/* @808 */ 	adc          #0
/* @809 */ 	sta         __i0+1
/* @813 */ 	lda         __i0
/* @814 */ 	sta         __x2
/* @815 */ 	lda         __i0+1
/* @816 */ 	sta         __x2+1
/* @817 */ 	lda          #0
/* @818 */ 	ldy          #7
.ConvertDecimal_label_819:
/* @820 */ 	sta         __x2, Y
/* @821 */ 	dey         
/* @822 */ 	cpy          #1
/* @823 */ 	bne         .ConvertDecimal_label_819
/* @828 */ 	lda         __x0
/* @829 */ 	ora         __x2
/* @830 */ 	sta         __x3
/* @831 */ 	lda         __x0+1
/* @832 */ 	ora         __x2+1
/* @833 */ 	sta         __x3+1
/* @834 */ 	lda         __x0+2
/* @835 */ 	ora         __x2+2
/* @836 */ 	sta         __x3+2
/* @837 */ 	lda         __x0+3
/* @838 */ 	ora         __x2+3
/* @839 */ 	sta         __x3+3
/* @840 */ 	lda         __x0+4
/* @841 */ 	ora         __x2+4
/* @842 */ 	sta         __x3+4
/* @843 */ 	lda         __x0+5
/* @844 */ 	ora         __x2+5
/* @845 */ 	sta         __x3+5
/* @846 */ 	lda         __x0+6
/* @847 */ 	ora         __x2+6
/* @848 */ 	sta         __x3+6
/* @849 */ 	lda         __x0+7
/* @850 */ 	ora         __x2+7
/* @851 */ 	sta         __x3+7
/* @854 */ 	ldx          #7
.ConvertDecimal_label_855:
/* @856 */ 	lda         __x3, X
/* @857 */ 	sta         __x1, X
/* @858 */ 	dex         
/* @859 */ 	bpl         .ConvertDecimal_label_855
/* @860 */ 	jmp         .ConvertDecimal_label_959
.ConvertDecimal_label_861:
/* @864 */ 	lda         __x1
/* @865 */ 	asl          A
/* @866 */ 	sta         __x0
/* @867 */ 	lda         __x1+1
/* @868 */ 	rol          A
/* @869 */ 	sta         __x0+1
/* @870 */ 	lda         __x1+2
/* @871 */ 	rol          A
/* @872 */ 	sta         __x0+2
/* @873 */ 	lda         __x1+3
/* @874 */ 	rol          A
/* @875 */ 	sta         __x0+3
/* @876 */ 	lda         __x1+4
/* @877 */ 	rol          A
/* @878 */ 	sta         __x0+4
/* @879 */ 	lda         __x1+5
/* @880 */ 	rol          A
/* @881 */ 	sta         __x0+5
/* @882 */ 	lda         __x1+6
/* @883 */ 	rol          A
/* @884 */ 	sta         __x0+6
/* @885 */ 	lda         __x1+7
/* @886 */ 	rol          A
/* @887 */ 	sta         __x0+7
/* @888 */ 	ldx          #3
.ConvertDecimal_label_889:
/* @890 */ 	asl         __x0
/* @891 */ 	rol         __x0+1
/* @892 */ 	rol         __x0+2
/* @893 */ 	rol         __x0+3
/* @894 */ 	rol         __x0+4
/* @895 */ 	rol         __x0+5
/* @896 */ 	rol         __x0+6
/* @897 */ 	rol         __x0+7
/* @898 */ 	dex         
/* @899 */ 	bne         .ConvertDecimal_label_889
/* @902 */ 	sec         
/* @903 */ 	lda         __i7
/* @904 */ 	sbc          #48
/* @905 */ 	sta         __i0
/* @906 */ 	lda         __i7+1
/* @907 */ 	sbc          #0
/* @908 */ 	sta         __i0+1
/* @912 */ 	lda         __i0
/* @913 */ 	sta         __x2
/* @914 */ 	lda         __i0+1
/* @915 */ 	sta         __x2+1
/* @916 */ 	lda          #0
/* @917 */ 	ldy          #7
.ConvertDecimal_label_918:
/* @919 */ 	sta         __x2, Y
/* @920 */ 	dey         
/* @921 */ 	cpy          #1
/* @922 */ 	bne         .ConvertDecimal_label_918
/* @927 */ 	lda         __x0
/* @928 */ 	ora         __x2
/* @929 */ 	sta         __x3
/* @930 */ 	lda         __x0+1
/* @931 */ 	ora         __x2+1
/* @932 */ 	sta         __x3+1
/* @933 */ 	lda         __x0+2
/* @934 */ 	ora         __x2+2
/* @935 */ 	sta         __x3+2
/* @936 */ 	lda         __x0+3
/* @937 */ 	ora         __x2+3
/* @938 */ 	sta         __x3+3
/* @939 */ 	lda         __x0+4
/* @940 */ 	ora         __x2+4
/* @941 */ 	sta         __x3+4
/* @942 */ 	lda         __x0+5
/* @943 */ 	ora         __x2+5
/* @944 */ 	sta         __x3+5
/* @945 */ 	lda         __x0+6
/* @946 */ 	ora         __x2+6
/* @947 */ 	sta         __x3+6
/* @948 */ 	lda         __x0+7
/* @949 */ 	ora         __x2+7
/* @950 */ 	sta         __x3+7
/* @953 */ 	ldx          #7
.ConvertDecimal_label_954:
/* @955 */ 	lda         __x3, X
/* @956 */ 	sta         __x1, X
/* @957 */ 	dex         
/* @958 */ 	bpl         .ConvertDecimal_label_954
.ConvertDecimal_label_959:
/* @960 */ 	bra         .ConvertDecimal_label_985
.ConvertDecimal_label_961:
/* @962 */ 	jsr         __pushi9
/* @965 */ 	lda         __i7+1
/* @966 */ 	and          #128
/* @967 */ 	ora         __i7
/* @972 */ 	jsr         __pusha
/* @975 */ 	jsr         .ConvertDecimal_label_973
/* @976 */ 	bra         .ConvertDecimal_label_974
.ConvertDecimal_label_973:
/* @977 */ 	jmp         (__i11)
.ConvertDecimal_label_974:
/* @978 */ 	jsr         __incsp4
/* @982 */ 	lda          #__i13
/* @983 */ 	jsr         __dec21
/* @984 */ 	jmp         .ConvertDecimal_label_1221
.ConvertDecimal_label_985:
.ConvertDecimal_label_986:
.ConvertDecimal_label_987:
/* @988 */ 	jmp         .ConvertDecimal_label_1216
.ConvertDecimal_label_989:
/* @991 */ 	lda         __i7
/* @992 */ 	cmp          #48
/* @993 */ 	bne         .ConvertDecimal_label_1023
/* @994 */ 	lda         __i7+1
/* @996 */ 	bne         .ConvertDecimal_label_1023
/* @999 */ 	ldx          #6
	jsr          __var_value1_b0			// prefix_done
/* @1004 */ 	lda         __b0
/* @1006 */ 	beq         .ConvertDecimal_label_1003
/* @1007 */ 	lda          #255
.ConvertDecimal_label_1003:
/* @1008 */ 	inc          A
/* @1013 */ 	beq         .ConvertDecimal_label_1021
/* @1016 */ 	lda          #1
/* @1017 */ 	sta         __b0
/* @1018 */ 	lda          #__b0
/* @1019 */ 	ldx          #5
/* @1020 */ 	jsr         __set_var_value1
.ConvertDecimal_label_1021:
/* @1022 */ 	jmp         .ConvertDecimal_label_1214
.ConvertDecimal_label_1023:
/* @1027 */ 	ldx          #5
	jsr          __var_value1_b0			// prefix_ok
/* @1030 */ 	lda         __b0
/* @1031 */ 	sta         __b7
/* @1035 */ 	beq         .ConvertDecimal_label_1070
/* @1041 */ 	ldx          #1
/* @1042 */ 	lda         __i7
/* @1043 */ 	cmp          #88
/* @1044 */ 	bne         .ConvertDecimal_label_1039
/* @1045 */ 	lda         __i7+1
/* @1047 */ 	beq         .ConvertDecimal_label_1040
.ConvertDecimal_label_1039:
/* @1048 */ 	dex         
.ConvertDecimal_label_1040:
/* @1049 */ 	stx         __b0
/* @1051 */ 	txa         
/* @1052 */ 	bne         .ConvertDecimal_label_1065
/* @1056 */ 	ldx          #1
/* @1057 */ 	lda         __i7
/* @1058 */ 	cmp          #120
/* @1059 */ 	bne         .ConvertDecimal_label_1054
/* @1060 */ 	lda         __i7+1
/* @1062 */ 	beq         .ConvertDecimal_label_1055
.ConvertDecimal_label_1054:
/* @1063 */ 	dex         
.ConvertDecimal_label_1055:
/* @1064 */ 	stx         __b0
.ConvertDecimal_label_1065:
/* @1068 */ 	lda         __b0
/* @1069 */ 	sta         __b7
.ConvertDecimal_label_1070:
/* @1072 */ 	lda         __b7
/* @1074 */ 	beq         .ConvertDecimal_label_1093
/* @1077 */ 	stz         __b0
/* @1078 */ 	lda          #__b0
/* @1079 */ 	ldx          #5
/* @1080 */ 	jsr         __set_var_value1
/* @1083 */ 	lda          #1
/* @1084 */ 	sta         __b0
/* @1085 */ 	lda          #__b0
/* @1086 */ 	ldx          #6
/* @1087 */ 	jsr         __set_var_value1
/* @1088 */ 	lda          #16
/* @1089 */ 	sta         __i10
/* @1091 */ 	stz         __i10+1
/* @1092 */ 	jmp         .ConvertDecimal_label_1213
.ConvertDecimal_label_1093:
/* @1095 */ 	ldx          #5
	jsr          __var_value1_b0			// prefix_ok
/* @1097 */ 	lda         __b0
/* @1099 */ 	beq         .ConvertDecimal_label_1105
/* @1100 */ 	lda          #8
/* @1101 */ 	sta         __i10
/* @1103 */ 	stz         __i10+1
/* @1104 */ 	jmp         .ConvertDecimal_label_1212
.ConvertDecimal_label_1105:
/* @1107 */ 	ldx         __i7
/* @1108 */ 	ldy         __i7+1
/* @1109 */ 	jsr         __builtin_isdigit
/* @1111 */ 	stz         __i0+1
/* @1115 */ 	cmp          #0
/* @1116 */ 	beq         .ConvertDecimal_label_1187
/* @1121 */ 	ldx          #7
.ConvertDecimal_label_1122:
/* @1123 */ 	lda         .lit.37, X
/* @1124 */ 	sta         __x0, X
/* @1125 */ 	dex         
/* @1126 */ 	bpl         .ConvertDecimal_label_1122
/* @1130 */ 	lda          #__x2
/* @1131 */ 	ldx          #__x1
/* @1132 */ 	ldy          #__x0
/* @1133 */ 	jsr         __umul8
/* @1135 */ 	lda         __i7
/* @1136 */ 	sta         __x0
/* @1137 */ 	lda         __i7+1
/* @1138 */ 	sta         __x0+1
/* @1139 */ 	lda          #0
/* @1140 */ 	ldy          #7
.ConvertDecimal_label_1141:
/* @1142 */ 	sta         __x0, Y
/* @1143 */ 	dey         
/* @1144 */ 	cpy          #1
/* @1145 */ 	bne         .ConvertDecimal_label_1141
/* @1150 */ 	clc         
/* @1151 */ 	ldy          #8
/* @1152 */ 	ldx          #0
.ConvertDecimal_label_1153:
/* @1154 */ 	lda         __x2, X
/* @1155 */ 	adc         __x0, X
/* @1156 */ 	sta         __x3, X
/* @1157 */ 	inx         
/* @1158 */ 	dey         
/* @1159 */ 	bne         .ConvertDecimal_label_1153
/* @1163 */ 	sec         
/* @1165 */ 	ldy          #8
/* @1166 */ 	ldx          #0
.ConvertDecimal_label_1167:
/* @1168 */ 	lda         __x3, X
/* @1169 */ 	sbc         .lit.36, X
/* @1170 */ 	sta         __x0, X
/* @1171 */ 	inx         
/* @1172 */ 	dey         
/* @1173 */ 	bne         .ConvertDecimal_label_1167
/* @1176 */ 	ldx          #7
.ConvertDecimal_label_1177:
/* @1178 */ 	lda         __x0, X
/* @1179 */ 	sta         __x1, X
/* @1180 */ 	dex         
/* @1181 */ 	bpl         .ConvertDecimal_label_1177
/* @1182 */ 	lda          #10
/* @1183 */ 	sta         __i10
/* @1185 */ 	stz         __i10+1
/* @1186 */ 	bra         .ConvertDecimal_label_1211
.ConvertDecimal_label_1187:
/* @1188 */ 	jsr         __pushi9
/* @1191 */ 	lda         __i7+1
/* @1192 */ 	and          #128
/* @1193 */ 	ora         __i7
/* @1198 */ 	jsr         __pusha
/* @1201 */ 	jsr         .ConvertDecimal_label_1199
/* @1202 */ 	bra         .ConvertDecimal_label_1200
.ConvertDecimal_label_1199:
/* @1203 */ 	jmp         (__i11)
.ConvertDecimal_label_1200:
/* @1204 */ 	jsr         __incsp4
/* @1208 */ 	lda          #__i13
/* @1209 */ 	jsr         __dec21
/* @1210 */ 	bra         .ConvertDecimal_label_1221
.ConvertDecimal_label_1211:
.ConvertDecimal_label_1212:
.ConvertDecimal_label_1213:
.ConvertDecimal_label_1214:
.ConvertDecimal_label_1216:
.ConvertDecimal_label_1217:
.ConvertDecimal_label_1218:
/* @1219 */ 	jmp         .ConvertDecimal_label_176
.ConvertDecimal_label_1220:
.ConvertDecimal_label_1221:
/* @1223 */ 	ldx          #4
	jsr          __var_value1_b0			// negative
/* @1225 */ 	lda         __b0
/* @1227 */ 	beq         .ConvertDecimal_label_1248
/* @1230 */ 	sec         
/* @1231 */ 	ldy          #8
/* @1232 */ 	ldx          #0
.ConvertDecimal_label_1233:
/* @1234 */ 	lda          #0
/* @1235 */ 	sbc         __x1, X
/* @1236 */ 	sta         __x0, X
/* @1237 */ 	inx         
/* @1238 */ 	dey         
/* @1239 */ 	bne         .ConvertDecimal_label_1233
/* @1242 */ 	ldx          #7
.ConvertDecimal_label_1243:
/* @1244 */ 	lda         __x0, X
/* @1245 */ 	sta         __x1, X
/* @1246 */ 	dex         
/* @1247 */ 	bpl         .ConvertDecimal_label_1243
.ConvertDecimal_label_1248:
/* @1251 */ 	lda         (__i6)
/* @1259 */ 	beq         .ConvertDecimal_label_1256
/* @1260 */ 	lda          #255
.ConvertDecimal_label_1256:
/* @1261 */ 	inc          A
/* @1266 */ 	beq         .ConvertDecimal_label_1275
/* @1267 */ 	jsr         __pushi6
/* @1268 */ 	jsr         __pushi12
/* @1269 */ 	lda         __b2
/* @1270 */ 	jsr         __pusha
/* @1271 */ 	jsr         __pushx1
/* @1272 */ 	jsr         WriteInt
/* @1274 */ 	jsr         __incsp14
.ConvertDecimal_label_1275:
/* @1277 */ 	ldx          #9
	jsr          __var_value2_i0			// length
/* @1281 */ 	ldx          #0
/* @1284 */ 	txa         
/* @1285 */ 	cmp         __i0
/* @1287 */ 	sbc         __i0+1
/* @1288 */ 	bvc         .ConvertDecimal_label_1283
/* @1289 */ 	eor          #128
.ConvertDecimal_label_1283:
/* @1290 */ 	bpl         .ConvertDecimal_label_1280
/* @1291 */ 	inx         
.ConvertDecimal_label_1280:
/* @1292 */ 	stx         __b0
/* @1294 */ 	ldx          #14
	jsr          __load_result
/* @1295 */ 	lda         #__b0
/* @1296 */ 	jsr         __result1
/* @1297 */ 	jmp         .ConvertDecimal_label_226
.func_end_ConvertDecimal:
	.size ConvertDecimal, .func_end_ConvertDecimal-ConvertDecimal

	.local  ReadPointer
	.type ReadPointer, @function

ReadPointer:
/* @29 */ 	stx         __result
/* @31 */ 	sty         __result+1
/* @32 */ 	ldx          #18
	jsr          __enter
	.byte        0x6a,0x60,0x00		// Save mask i:10 b:3 l:0 x:3 f:0 
/* @38 */ 	ldx          #6
	jsr          __arg_value2_i5			// ptr
/* @45 */ 	ldx          #8
	jsr          __arg_value2_i7			// fmt
/* @63 */ 	ldx          #0
	jsr          __arg_value2_i10			// get
/* @66 */ 	ldx          #4
	jsr          __arg_value2_i11			// data
/* @72 */ 	ldx          #2
	jsr          __arg_value2_i12			// unget
/* @77 */ 	stz         __i0
/* @78 */ 	stz         __i0+1
/* @79 */ 	lda          #__i0
/* @81 */ 	ldx          #14
/* @83 */ 	jsr         __set_var_value2
/* @86 */ 	ldy          #1
/* @87 */ 	lda         (__i7), Y
/* @88 */ 	sta         __i0
/* @90 */ 	iny         
/* @91 */ 	lda         (__i7), Y
/* @92 */ 	sta         __i0+1
/* @95 */ 	lda         __i0
/* @96 */ 	cmp          #255
/* @97 */ 	bne         .ReadPointer_label_108
/* @98 */ 	lda         __i0+1
/* @99 */ 	cmp          #255
/* @100 */ 	bne         .ReadPointer_label_108
/* @103 */ 	lda          #255
/* @104 */ 	sta         __i4
/* @105 */ 	lda          #127
/* @106 */ 	sta         __i4+1
/* @107 */ 	bra         .ReadPointer_label_123
.ReadPointer_label_108:
/* @111 */ 	ldy          #1
/* @112 */ 	lda         (__i7), Y
/* @113 */ 	sta         __i0
/* @114 */ 	iny         
/* @115 */ 	lda         (__i7), Y
/* @116 */ 	sta         __i0+1
/* @119 */ 	lda         __i0
/* @120 */ 	sta         __i4
/* @121 */ 	lda         __i0+1
/* @122 */ 	sta         __i4+1
.ReadPointer_label_123:
/* @126 */ 	lda         __i4
/* @127 */ 	sta         __i6
/* @128 */ 	lda         __i4+1
/* @129 */ 	sta         __i6+1
/* @131 */ 	ldx          #7
/* @132 */ 	lda          #0
.ReadPointer_label_133:
/* @135 */ 	sta         __x1, X
/* @136 */ 	dex         
/* @137 */ 	bpl         .ReadPointer_label_133
/* @138 */ 	stz         __b2
/* @139 */ 	stz         __b3
/* @140 */ 	lda          #1
/* @141 */ 	sta         __i8
/* @142 */ 	dec          A
/* @143 */ 	stz         __i8+1
/* @146 */ 	stz         __b0
/* @147 */ 	lda          #__b0
/* @149 */ 	ldx          #4
/* @151 */ 	jsr         __set_var_value1
/* @154 */ 	lda         #%lo(.str.46)
/* @155 */ 	sta         __i0
/* @156 */ 	lda         #%hi(.str.46)
/* @157 */ 	sta         __i0+1
/* @158 */ 	lda          #7
/* @160 */ 	sta         __mem_size
/* @162 */ 	lda         __i0
/* @164 */ 	sta         __mem_src
/* @165 */ 	lda         __i0+1
/* @167 */ 	sta         __mem_src+1
/* @169 */ 	ldx          #11
	jsr          __var_addr_i0			// null_literal
/* @171 */ 	lda         __i0
/* @173 */ 	sta         __mem_dest
/* @174 */ 	lda         __i0+1
/* @176 */ 	sta         __mem_dest+1
/* @178 */ 	jsr         __copymem1
.ReadPointer_label_179:
/* @181 */ 	ldx          #14
	jsr          __var_value2_i0			// length
/* @184 */ 	lda         __i0
/* @185 */ 	cmp         __i6
/* @186 */ 	lda         __i0+1
/* @187 */ 	sbc         __i6+1
/* @188 */ 	bvc         .ReadPointer_label_183
/* @190 */ 	eor          #128
.ReadPointer_label_183:
/* @191 */ 	bmi         .ReadPointer_label_756
/* @757 */ 	jmp         .ReadPointer_label_709
.ReadPointer_label_756:
/* @192 */ 	jsr         __pushi11
/* @193 */ 	ldx         #__i9
/* @194 */ 	ldy          #0
/* @197 */ 	jsr         .ReadPointer_label_195
/* @198 */ 	bra         .ReadPointer_label_196
.ReadPointer_label_195:
/* @199 */ 	jmp         (__i10)
.ReadPointer_label_196:
/* @201 */ 	jsr         __incsp2
/* @203 */ 	lda         __i9
/* @204 */ 	cmp          #255
/* @205 */ 	bne         .ReadPointer_label_232
/* @206 */ 	lda         __i9+1
/* @207 */ 	cmp          #255
/* @208 */ 	bne         .ReadPointer_label_232
/* @211 */ 	ldx          #14
	jsr          __var_value2_i0			// length
/* @214 */ 	lda          #0
/* @215 */ 	cmp         __i0
/* @217 */ 	sbc         __i0+1
/* @218 */ 	bvc         .ReadPointer_label_213
/* @219 */ 	eor          #128
.ReadPointer_label_213:
/* @220 */ 	bpl         .ReadPointer_label_758
/* @759 */ 	jmp         .ReadPointer_label_709
.ReadPointer_label_758:
/* @224 */ 	stz         __b0
/* @225 */ 	ldx          #19
	jsr          __load_result
/* @226 */ 	lda         #__b0
/* @228 */ 	jsr         __result1
.ReadPointer_label_229:
/* @230 */ 	ldy          #21
	jmp          __leave
.ReadPointer_label_232:
/* @234 */ 	ldx          #14
	jsr          __var_addr_i13			// length
/* @236 */ 	lda          #__i13
/* @238 */ 	jsr         __inc21
/* @240 */ 	lda         __i9
/* @241 */ 	cmp          #40
/* @242 */ 	bne         .ReadPointer_label_255
/* @243 */ 	lda         __i9+1
/* @245 */ 	bne         .ReadPointer_label_255
/* @249 */ 	lda          #1
/* @250 */ 	sta         __b0
/* @251 */ 	lda          #__b0
/* @252 */ 	ldx          #4
/* @253 */ 	jsr         __set_var_value1
/* @254 */ 	jmp         .ReadPointer_label_179
.ReadPointer_label_255:
/* @257 */ 	lda         __i9
/* @258 */ 	cmp          #48
/* @259 */ 	bne         .ReadPointer_label_282
/* @260 */ 	lda         __i9+1
/* @262 */ 	bne         .ReadPointer_label_282
/* @267 */ 	lda         __b2
/* @269 */ 	beq         .ReadPointer_label_266
/* @271 */ 	lda          #255
.ReadPointer_label_266:
/* @272 */ 	inc          A
/* @277 */ 	beq         .ReadPointer_label_280
/* @278 */ 	lda          #1
/* @279 */ 	sta         __b3
.ReadPointer_label_280:
/* @281 */ 	jmp         .ReadPointer_label_707
.ReadPointer_label_282:
/* @286 */ 	lda         __b3
/* @287 */ 	sta         __b4
/* @291 */ 	beq         .ReadPointer_label_326
/* @297 */ 	ldx          #1
/* @298 */ 	lda         __i9
/* @299 */ 	cmp          #88
/* @300 */ 	bne         .ReadPointer_label_295
/* @301 */ 	lda         __i9+1
/* @303 */ 	beq         .ReadPointer_label_296
.ReadPointer_label_295:
/* @304 */ 	dex         
.ReadPointer_label_296:
/* @305 */ 	stx         __b0
/* @307 */ 	txa         
/* @308 */ 	bne         .ReadPointer_label_321
/* @312 */ 	ldx          #1
/* @313 */ 	lda         __i9
/* @314 */ 	cmp          #120
/* @315 */ 	bne         .ReadPointer_label_310
/* @316 */ 	lda         __i9+1
/* @318 */ 	beq         .ReadPointer_label_311
.ReadPointer_label_310:
/* @319 */ 	dex         
.ReadPointer_label_311:
/* @320 */ 	stx         __b0
.ReadPointer_label_321:
/* @324 */ 	lda         __b0
/* @325 */ 	sta         __b4
.ReadPointer_label_326:
/* @328 */ 	lda         __b4
/* @330 */ 	beq         .ReadPointer_label_335
/* @331 */ 	stz         __b3
/* @332 */ 	lda          #1
/* @333 */ 	sta         __b2
/* @334 */ 	jmp         .ReadPointer_label_706
.ReadPointer_label_335:
/* @337 */ 	ldx         __i9
/* @338 */ 	ldy         __i9+1
/* @340 */ 	jsr         __builtin_isxdigit
/* @342 */ 	stz         __i0+1
/* @346 */ 	cmp          #0
/* @347 */ 	bne         .ReadPointer_label_760
/* @761 */ 	jmp         .ReadPointer_label_605
.ReadPointer_label_760:
/* @349 */ 	ldx         __i9
/* @350 */ 	ldy         __i9+1
/* @352 */ 	jsr         __builtin_toupper
/* @353 */ 	sta         __i0
/* @354 */ 	stz         __i0+1
/* @356 */ 	lda          #__i0
/* @358 */ 	ldx          #12
/* @359 */ 	jsr         __set_var_value1
/* @361 */ 	ldx          #12
	jsr          __var_value1_b0			// uch
/* @365 */ 	lda         __b0
/* @366 */ 	sta         __i0
/* @367 */ 	and          #128
/* @369 */ 	beq         .ReadPointer_label_368
/* @370 */ 	lda          #255
.ReadPointer_label_368:
/* @371 */ 	sta         __i0+1
/* @374 */ 	lda          #65
/* @375 */ 	cmp         __i0
/* @376 */ 	lda          #0
/* @377 */ 	sbc         __i0+1
/* @378 */ 	bvc         .ReadPointer_label_373
/* @379 */ 	eor          #128
.ReadPointer_label_373:
/* @380 */ 	bmi         .ReadPointer_label_762
/* @763 */ 	jmp         .ReadPointer_label_505
.ReadPointer_label_762:
/* @383 */ 	lda         __x1
/* @384 */ 	asl          A
/* @385 */ 	sta         __x0
/* @386 */ 	lda         __x1+1
/* @387 */ 	rol          A
/* @388 */ 	sta         __x0+1
/* @389 */ 	lda         __x1+2
/* @390 */ 	rol          A
/* @391 */ 	sta         __x0+2
/* @393 */ 	lda         __x1+3
/* @394 */ 	rol          A
/* @395 */ 	sta         __x0+3
/* @396 */ 	lda         __x1+4
/* @397 */ 	rol          A
/* @398 */ 	sta         __x0+4
/* @400 */ 	lda         __x1+5
/* @401 */ 	rol          A
/* @402 */ 	sta         __x0+5
/* @404 */ 	lda         __x1+6
/* @405 */ 	rol          A
/* @406 */ 	sta         __x0+6
/* @407 */ 	lda         __x1+7
/* @408 */ 	rol          A
/* @409 */ 	sta         __x0+7
/* @410 */ 	ldx          #3
.ReadPointer_label_411:
/* @412 */ 	asl         __x0
/* @413 */ 	rol         __x0+1
/* @414 */ 	rol         __x0+2
/* @415 */ 	rol         __x0+3
/* @416 */ 	rol         __x0+4
/* @417 */ 	rol         __x0+5
/* @418 */ 	rol         __x0+6
/* @419 */ 	rol         __x0+7
/* @420 */ 	dex         
/* @421 */ 	bne         .ReadPointer_label_411
/* @423 */ 	ldx          #12
	jsr          __var_value1_b0			// uch
/* @427 */ 	lda         __b0
/* @428 */ 	sta         __i0
/* @429 */ 	and          #128
/* @431 */ 	beq         .ReadPointer_label_430
/* @432 */ 	lda          #255
.ReadPointer_label_430:
/* @433 */ 	sta         __i0+1
/* @437 */ 	sec         
/* @438 */ 	lda         __i0
/* @439 */ 	sbc          #65
/* @440 */ 	sta         __i1
/* @441 */ 	lda         __i0+1
/* @442 */ 	sbc          #0
/* @443 */ 	sta         __i1+1
/* @447 */ 	clc         
/* @448 */ 	lda         __i1
/* @449 */ 	adc          #10
/* @450 */ 	sta         __i0
/* @451 */ 	lda         __i1+1
/* @452 */ 	adc          #0
/* @453 */ 	sta         __i0+1
/* @457 */ 	lda         __i0
/* @458 */ 	sta         __x2
/* @459 */ 	lda         __i0+1
/* @460 */ 	sta         __x2+1
/* @461 */ 	lda          #0
/* @462 */ 	ldy          #7
.ReadPointer_label_463:
/* @464 */ 	sta         __x2, Y
/* @465 */ 	dey         
/* @466 */ 	cpy          #1
/* @467 */ 	bne         .ReadPointer_label_463
/* @472 */ 	lda         __x0
/* @473 */ 	ora         __x2
/* @474 */ 	sta         __x3
/* @475 */ 	lda         __x0+1
/* @476 */ 	ora         __x2+1
/* @477 */ 	sta         __x3+1
/* @478 */ 	lda         __x0+2
/* @479 */ 	ora         __x2+2
/* @480 */ 	sta         __x3+2
/* @481 */ 	lda         __x0+3
/* @482 */ 	ora         __x2+3
/* @483 */ 	sta         __x3+3
/* @484 */ 	lda         __x0+4
/* @485 */ 	ora         __x2+4
/* @486 */ 	sta         __x3+4
/* @487 */ 	lda         __x0+5
/* @488 */ 	ora         __x2+5
/* @489 */ 	sta         __x3+5
/* @490 */ 	lda         __x0+6
/* @491 */ 	ora         __x2+6
/* @492 */ 	sta         __x3+6
/* @493 */ 	lda         __x0+7
/* @494 */ 	ora         __x2+7
/* @495 */ 	sta         __x3+7
/* @498 */ 	ldx          #7
.ReadPointer_label_499:
/* @500 */ 	lda         __x3, X
/* @501 */ 	sta         __x1, X
/* @502 */ 	dex         
/* @503 */ 	bpl         .ReadPointer_label_499
/* @504 */ 	jmp         .ReadPointer_label_603
.ReadPointer_label_505:
/* @508 */ 	lda         __x1
/* @509 */ 	asl          A
/* @510 */ 	sta         __x0
/* @511 */ 	lda         __x1+1
/* @512 */ 	rol          A
/* @513 */ 	sta         __x0+1
/* @514 */ 	lda         __x1+2
/* @515 */ 	rol          A
/* @516 */ 	sta         __x0+2
/* @517 */ 	lda         __x1+3
/* @518 */ 	rol          A
/* @519 */ 	sta         __x0+3
/* @520 */ 	lda         __x1+4
/* @521 */ 	rol          A
/* @522 */ 	sta         __x0+4
/* @523 */ 	lda         __x1+5
/* @524 */ 	rol          A
/* @525 */ 	sta         __x0+5
/* @526 */ 	lda         __x1+6
/* @527 */ 	rol          A
/* @528 */ 	sta         __x0+6
/* @529 */ 	lda         __x1+7
/* @530 */ 	rol          A
/* @531 */ 	sta         __x0+7
/* @532 */ 	ldx          #3
.ReadPointer_label_533:
/* @534 */ 	asl         __x0
/* @535 */ 	rol         __x0+1
/* @536 */ 	rol         __x0+2
/* @537 */ 	rol         __x0+3
/* @538 */ 	rol         __x0+4
/* @539 */ 	rol         __x0+5
/* @540 */ 	rol         __x0+6
/* @541 */ 	rol         __x0+7
/* @542 */ 	dex         
/* @543 */ 	bne         .ReadPointer_label_533
/* @546 */ 	sec         
/* @547 */ 	lda         __i9
/* @548 */ 	sbc          #48
/* @549 */ 	sta         __i0
/* @550 */ 	lda         __i9+1
/* @551 */ 	sbc          #0
/* @552 */ 	sta         __i0+1
/* @556 */ 	lda         __i0
/* @557 */ 	sta         __x2
/* @558 */ 	lda         __i0+1
/* @559 */ 	sta         __x2+1
/* @560 */ 	lda          #0
/* @561 */ 	ldy          #7
.ReadPointer_label_562:
/* @563 */ 	sta         __x2, Y
/* @564 */ 	dey         
/* @565 */ 	cpy          #1
/* @566 */ 	bne         .ReadPointer_label_562
/* @571 */ 	lda         __x0
/* @572 */ 	ora         __x2
/* @573 */ 	sta         __x3
/* @574 */ 	lda         __x0+1
/* @575 */ 	ora         __x2+1
/* @576 */ 	sta         __x3+1
/* @577 */ 	lda         __x0+2
/* @578 */ 	ora         __x2+2
/* @579 */ 	sta         __x3+2
/* @580 */ 	lda         __x0+3
/* @581 */ 	ora         __x2+3
/* @582 */ 	sta         __x3+3
/* @583 */ 	lda         __x0+4
/* @584 */ 	ora         __x2+4
/* @585 */ 	sta         __x3+4
/* @586 */ 	lda         __x0+5
/* @587 */ 	ora         __x2+5
/* @588 */ 	sta         __x3+5
/* @589 */ 	lda         __x0+6
/* @590 */ 	ora         __x2+6
/* @591 */ 	sta         __x3+6
/* @592 */ 	lda         __x0+7
/* @593 */ 	ora         __x2+7
/* @594 */ 	sta         __x3+7
/* @597 */ 	ldx          #7
.ReadPointer_label_598:
/* @599 */ 	lda         __x3, X
/* @600 */ 	sta         __x1, X
/* @601 */ 	dex         
/* @602 */ 	bpl         .ReadPointer_label_598
.ReadPointer_label_603:
/* @604 */ 	jmp         .ReadPointer_label_705
.ReadPointer_label_605:
/* @607 */ 	ldx          #4
	jsr          __var_value1_b0			// is_null
/* @609 */ 	lda         __b0
/* @611 */ 	beq         .ReadPointer_label_677
/* @613 */ 	lda         __i8
/* @614 */ 	cmp          #6
/* @615 */ 	bne         .ReadPointer_label_627
/* @616 */ 	lda         __i8+1
/* @618 */ 	bne         .ReadPointer_label_627
/* @620 */ 	ldx          #7
/* @621 */ 	lda          #0
.ReadPointer_label_622:
/* @623 */ 	sta         __x1, X
/* @624 */ 	dex         
/* @625 */ 	bpl         .ReadPointer_label_622
/* @626 */ 	bra         .ReadPointer_label_709
.ReadPointer_label_627:
/* @629 */ 	ldx          #11
	jsr          __var_addr_i0			// null_literal
/* @633 */ 	clc         
/* @634 */ 	lda         __i0
/* @635 */ 	adc         __i8
/* @636 */ 	sta         __i1
/* @637 */ 	lda         __i0+1
/* @638 */ 	adc         __i8+1
/* @639 */ 	sta         __i1+1
/* @644 */ 	lda         (__i1)
/* @650 */ 	sta         __i0
/* @651 */ 	and          #128
/* @653 */ 	beq         .ReadPointer_label_652
/* @654 */ 	lda          #255
.ReadPointer_label_652:
/* @655 */ 	sta         __i0+1
/* @658 */ 	lda         __i9
/* @659 */ 	cmp         __i0
/* @660 */ 	bne         .ReadPointer_label_657
/* @661 */ 	lda         __i9+1
/* @662 */ 	cmp         __i0+1
/* @663 */ 	beq         .ReadPointer_label_672
.ReadPointer_label_657:
/* @667 */ 	stz         __b0
/* @668 */ 	ldx          #19
	jsr          __load_result
/* @669 */ 	lda         #__b0
/* @670 */ 	jsr         __result1
/* @671 */ 	jmp         .ReadPointer_label_229
.ReadPointer_label_672:
/* @673 */ 	lda          #__i8
/* @675 */ 	jsr         __rinc21
/* @676 */ 	bra         .ReadPointer_label_704
.ReadPointer_label_677:
/* @678 */ 	jsr         __pushi11
/* @681 */ 	lda         __i9+1
/* @682 */ 	and          #128
/* @683 */ 	ora         __i9
/* @689 */ 	jsr         __pusha
/* @692 */ 	jsr         .ReadPointer_label_690
/* @693 */ 	bra         .ReadPointer_label_691
.ReadPointer_label_690:
/* @694 */ 	jmp         (__i12)
.ReadPointer_label_691:
/* @696 */ 	jsr         __incsp4
/* @700 */ 	lda          #__i13
/* @702 */ 	jsr         __dec21
/* @703 */ 	bra         .ReadPointer_label_709
.ReadPointer_label_704:
.ReadPointer_label_705:
.ReadPointer_label_706:
.ReadPointer_label_707:
/* @708 */ 	jmp         .ReadPointer_label_179
.ReadPointer_label_709:
/* @712 */ 	lda         (__i7)
/* @720 */ 	beq         .ReadPointer_label_717
/* @721 */ 	lda          #255
.ReadPointer_label_717:
/* @722 */ 	inc          A
/* @727 */ 	beq         .ReadPointer_label_733
/* @728 */ 	lda         __x1
/* @729 */ 	sta         (__i5)
/* @730 */ 	lda         __x1+1
/* @731 */ 	ldy          #1
/* @732 */ 	sta         (__i5), Y
.ReadPointer_label_733:
/* @735 */ 	ldx          #14
	jsr          __var_value2_i0			// length
/* @739 */ 	ldx          #0
/* @742 */ 	txa         
/* @743 */ 	cmp         __i0
/* @745 */ 	sbc         __i0+1
/* @746 */ 	bvc         .ReadPointer_label_741
/* @747 */ 	eor          #128
.ReadPointer_label_741:
/* @748 */ 	bpl         .ReadPointer_label_738
/* @749 */ 	inx         
.ReadPointer_label_738:
/* @750 */ 	stx         __b0
/* @752 */ 	ldx          #19
	jsr          __load_result
/* @753 */ 	lda         #__b0
/* @754 */ 	jsr         __result1
/* @755 */ 	jmp         .ReadPointer_label_229
.func_end_ReadPointer:
	.size ReadPointer, .func_end_ReadPointer-ReadPointer

	.local  ReadChars
	.type ReadChars, @function

ReadChars:
/* @13 */ 	stx         __result
/* @15 */ 	sty         __result+1
/* @16 */ 	ldx          #9
	jsr          __enter
	.byte        0x09,0x00,0x00		// Save mask i:9 b:0 l:0 x:0 f:0 
/* @20 */ 	ldx          #4
	jsr          __arg_value2_i5			// data
/* @24 */ 	ldx          #8
	jsr          __arg_value2_i6			// fmt
/* @31 */ 	ldx          #6
	jsr          __arg_value2_i8			// ptr
/* @46 */ 	ldy          #1
/* @47 */ 	lda         (__i6), Y
/* @48 */ 	sta         __i0
/* @50 */ 	iny         
/* @51 */ 	lda         (__i6), Y
/* @52 */ 	sta         __i0+1
/* @55 */ 	lda         __i0
/* @56 */ 	cmp          #255
/* @57 */ 	bne         .ReadChars_label_68
/* @58 */ 	lda         __i0+1
/* @59 */ 	cmp          #255
/* @60 */ 	bne         .ReadChars_label_68
/* @63 */ 	lda          #1
/* @64 */ 	sta         __i4
/* @65 */ 	dec          A
/* @66 */ 	stz         __i4+1
/* @67 */ 	bra         .ReadChars_label_76
.ReadChars_label_68:
/* @70 */ 	ldy          #1
/* @71 */ 	lda         (__i6), Y
/* @72 */ 	sta         __i4
/* @73 */ 	iny         
/* @74 */ 	lda         (__i6), Y
/* @75 */ 	sta         __i4+1
.ReadChars_label_76:
/* @78 */ 	lda          #__i4
/* @80 */ 	ldx          #5
/* @82 */ 	jsr         __set_var_value2
/* @83 */ 	lda         __i8
/* @84 */ 	sta         __i7
/* @85 */ 	lda         __i8+1
/* @86 */ 	sta         __i7+1
/* @87 */ 	lda         __i8
/* @88 */ 	sta         __i9
/* @89 */ 	lda         __i8+1
/* @90 */ 	sta         __i9+1
/* @91 */ 	stz         __i10
/* @92 */ 	stz         __i10+1
/* @93 */ 	stz         __i11
/* @94 */ 	stz         __i11+1
.ReadChars_label_95:
/* @97 */ 	ldx          #5
	jsr          __var_value2_i0			// numchars
/* @100 */ 	lda         __i11
/* @101 */ 	cmp         __i0
/* @102 */ 	lda         __i11+1
/* @103 */ 	sbc         __i0+1
/* @104 */ 	bvc         .ReadChars_label_99
/* @106 */ 	eor          #128
.ReadChars_label_99:
/* @107 */ 	bmi         .ReadChars_label_309
/* @310 */ 	jmp         .ReadChars_label_217
.ReadChars_label_309:
/* @109 */ 	ldx          #0
	jsr          __arg_value2_i0			// get
/* @110 */ 	jsr         __pushi5
/* @111 */ 	ldx         #__i12
/* @112 */ 	ldy          #0
/* @117 */ 	jsr         .ReadChars_label_115
/* @118 */ 	bra         .ReadChars_label_116
.ReadChars_label_115:
/* @119 */ 	jmp         (__i0)
.ReadChars_label_116:
/* @121 */ 	jsr         __incsp2
/* @123 */ 	lda         __i12
/* @124 */ 	cmp          #255
/* @125 */ 	bne         .ReadChars_label_122
/* @126 */ 	lda         __i12+1
/* @127 */ 	cmp          #255
/* @128 */ 	beq         .ReadChars_label_217
.ReadChars_label_122:
/* @131 */ 	lda          #__i10
/* @133 */ 	jsr         __rinc21
/* @136 */ 	lda         (__i6)
/* @144 */ 	beq         .ReadChars_label_141
/* @146 */ 	lda          #255
.ReadChars_label_141:
/* @147 */ 	inc          A
/* @152 */ 	beq         .ReadChars_label_212
/* @156 */ 	ldy          #3
/* @157 */ 	lda         (__i6), Y
/* @158 */ 	sta         __i0
/* @160 */ 	iny         
/* @161 */ 	lda         (__i6), Y
/* @162 */ 	sta         __i0+1
/* @165 */ 	lda         __i0
/* @166 */ 	cmp          #3
/* @167 */ 	bne         .ReadChars_label_190
/* @168 */ 	lda         __i0+1
/* @170 */ 	bne         .ReadChars_label_190
/* @174 */ 	lda         __i9
/* @175 */ 	sta         __i0
/* @176 */ 	lda         __i9+1
/* @177 */ 	sta         __i0+1
/* @178 */ 	lda          #__i9
/* @179 */ 	ldx          #2
/* @181 */ 	jsr         __rinc2
/* @184 */ 	lda         __i12
/* @185 */ 	sta         (__i0)
/* @186 */ 	lda         __i12+1
/* @187 */ 	ldy          #1
/* @188 */ 	sta         (__i0), Y
/* @189 */ 	bra         .ReadChars_label_211
.ReadChars_label_190:
/* @193 */ 	lda         __i7
/* @194 */ 	sta         __i0
/* @195 */ 	lda         __i7+1
/* @196 */ 	sta         __i0+1
/* @197 */ 	lda          #__i7
/* @198 */ 	jsr         __rinc21
/* @201 */ 	lda         __i12
/* @202 */ 	sta         __i1
/* @203 */ 	lda         __i12+1
/* @204 */ 	sta         __i1+1
/* @209 */ 	lda         __i1
/* @210 */ 	sta         (__i0)
.ReadChars_label_211:
.ReadChars_label_212:
/* @214 */ 	lda          #__i11
/* @215 */ 	jsr         __rinc21
/* @216 */ 	jmp         .ReadChars_label_95
.ReadChars_label_217:
/* @220 */ 	lda         (__i6)
/* @225 */ 	beq         .ReadChars_label_248
/* @229 */ 	ldx          #0
/* @231 */ 	txa         
/* @232 */ 	cmp         __i10
/* @234 */ 	sbc         __i10+1
/* @235 */ 	bvc         .ReadChars_label_230
/* @236 */ 	eor          #128
.ReadChars_label_230:
/* @237 */ 	bpl         .ReadChars_label_228
/* @238 */ 	inx         
.ReadChars_label_228:
/* @239 */ 	stx         __b0
/* @241 */ 	ldx          #10
	jsr          __load_result
/* @242 */ 	lda         #__b0
/* @244 */ 	jsr         __result1
.ReadChars_label_245:
/* @246 */ 	ldy          #12
	jmp          __leave
.ReadChars_label_248:
/* @251 */ 	ldy          #3
/* @252 */ 	lda         (__i6), Y
/* @253 */ 	sta         __i0
/* @254 */ 	iny         
/* @255 */ 	lda         (__i6), Y
/* @256 */ 	sta         __i0+1
/* @259 */ 	lda         __i0
/* @260 */ 	cmp          #3
/* @261 */ 	bne         .ReadChars_label_283
/* @262 */ 	lda         __i0+1
/* @264 */ 	bne         .ReadChars_label_283
/* @267 */ 	lda         __i9
/* @268 */ 	cmp         __i8
/* @269 */ 	bne         .ReadChars_label_281
/* @270 */ 	lda         __i9+1
/* @271 */ 	cmp         __i8+1
/* @272 */ 	bne         .ReadChars_label_281
/* @276 */ 	stz         __b0
/* @277 */ 	ldx          #10
	jsr          __load_result
/* @278 */ 	lda         #__b0
/* @279 */ 	jsr         __result1
/* @280 */ 	bra         .ReadChars_label_245
.ReadChars_label_281:
/* @282 */ 	bra         .ReadChars_label_300
.ReadChars_label_283:
/* @285 */ 	lda         __i7
/* @286 */ 	cmp         __i8
/* @287 */ 	bne         .ReadChars_label_299
/* @288 */ 	lda         __i7+1
/* @289 */ 	cmp         __i8+1
/* @290 */ 	bne         .ReadChars_label_299
/* @294 */ 	stz         __b0
/* @295 */ 	ldx          #10
	jsr          __load_result
/* @296 */ 	lda         #__b0
/* @297 */ 	jsr         __result1
/* @298 */ 	bra         .ReadChars_label_245
.ReadChars_label_299:
.ReadChars_label_300:
/* @303 */ 	lda          #1
/* @304 */ 	sta         __b0
/* @305 */ 	ldx          #10
	jsr          __load_result
/* @306 */ 	lda         #__b0
/* @307 */ 	jsr         __result1
/* @308 */ 	bra         .ReadChars_label_245
.func_end_ReadChars:
	.size ReadChars, .func_end_ReadChars-ReadChars

	.local  ReadString
	.type ReadString, @function

ReadString:
/* @14 */ 	stx         __result
/* @16 */ 	sty         __result+1
/* @17 */ 	ldx          #9
	jsr          __enter
	.byte        0x09,0x00,0x00		// Save mask i:9 b:0 l:0 x:0 f:0 
/* @22 */ 	ldx          #2
	jsr          __arg_value2_i5			// unget
/* @26 */ 	ldx          #8
	jsr          __arg_value2_i6			// fmt
/* @33 */ 	ldx          #6
	jsr          __arg_value2_i8			// ptr
/* @50 */ 	ldy          #1
/* @51 */ 	lda         (__i6), Y
/* @52 */ 	sta         __i0
/* @54 */ 	iny         
/* @55 */ 	lda         (__i6), Y
/* @56 */ 	sta         __i0+1
/* @59 */ 	lda         __i0
/* @60 */ 	cmp          #255
/* @61 */ 	bne         .ReadString_label_72
/* @62 */ 	lda         __i0+1
/* @63 */ 	cmp          #255
/* @64 */ 	bne         .ReadString_label_72
/* @67 */ 	lda          #255
/* @68 */ 	sta         __i4
/* @69 */ 	lda          #127
/* @70 */ 	sta         __i4+1
/* @71 */ 	bra         .ReadString_label_80
.ReadString_label_72:
/* @74 */ 	ldy          #1
/* @75 */ 	lda         (__i6), Y
/* @76 */ 	sta         __i4
/* @77 */ 	iny         
/* @78 */ 	lda         (__i6), Y
/* @79 */ 	sta         __i4+1
.ReadString_label_80:
/* @82 */ 	lda          #__i4
/* @84 */ 	ldx          #5
/* @86 */ 	jsr         __set_var_value2
/* @87 */ 	lda         __i8
/* @88 */ 	sta         __i7
/* @89 */ 	lda         __i8+1
/* @90 */ 	sta         __i7+1
/* @91 */ 	lda         __i8
/* @92 */ 	sta         __i9
/* @93 */ 	lda         __i8+1
/* @94 */ 	sta         __i9+1
/* @95 */ 	stz         __i10
/* @96 */ 	stz         __i10+1
/* @97 */ 	stz         __i11
/* @98 */ 	stz         __i11+1
.ReadString_label_99:
/* @101 */ 	ldx          #5
	jsr          __var_value2_i0			// numchars
/* @104 */ 	lda         __i11
/* @105 */ 	cmp         __i0
/* @106 */ 	lda         __i11+1
/* @107 */ 	sbc         __i0+1
/* @108 */ 	bvc         .ReadString_label_103
/* @110 */ 	eor          #128
.ReadString_label_103:
/* @111 */ 	bmi         .ReadString_label_364
/* @365 */ 	jmp         .ReadString_label_262
.ReadString_label_364:
/* @113 */ 	ldx          #0
	jsr          __arg_value2_i0			// get
/* @115 */ 	ldx          #4
	jsr          __arg_value2_i1			// data
/* @118 */ 	jsr         __pushi1
/* @119 */ 	ldx         #__i12
/* @120 */ 	ldy          #0
/* @125 */ 	jsr         .ReadString_label_123
/* @126 */ 	bra         .ReadString_label_124
.ReadString_label_123:
/* @127 */ 	jmp         (__i0)
.ReadString_label_124:
/* @129 */ 	jsr         __incsp2
/* @131 */ 	lda         __i12
/* @132 */ 	cmp          #255
/* @133 */ 	bne         .ReadString_label_130
/* @134 */ 	lda         __i12+1
/* @135 */ 	cmp          #255
/* @136 */ 	bne         .ReadString_label_366
/* @367 */ 	jmp         .ReadString_label_262
.ReadString_label_366:
.ReadString_label_130:
/* @140 */ 	ldx         __i12
/* @141 */ 	ldy         __i12+1
/* @143 */ 	jsr         __builtin_isspace
/* @145 */ 	stz         __i0+1
/* @149 */ 	cmp          #0
/* @150 */ 	beq         .ReadString_label_175
/* @152 */ 	ldx          #4
	jsr          __arg_value2_i0			// data
/* @155 */ 	jsr         __pushi0
/* @158 */ 	lda         __i12+1
/* @159 */ 	and          #128
/* @160 */ 	ora         __i12
/* @166 */ 	jsr         __pusha
/* @169 */ 	jsr         .ReadString_label_167
/* @170 */ 	bra         .ReadString_label_168
.ReadString_label_167:
/* @171 */ 	jmp         (__i5)
.ReadString_label_168:
/* @173 */ 	jsr         __incsp4
/* @174 */ 	bra         .ReadString_label_262
.ReadString_label_175:
/* @176 */ 	lda          #__i10
/* @178 */ 	jsr         __rinc21
/* @181 */ 	lda         (__i6)
/* @189 */ 	beq         .ReadString_label_186
/* @191 */ 	lda          #255
.ReadString_label_186:
/* @192 */ 	inc          A
/* @197 */ 	beq         .ReadString_label_257
/* @201 */ 	ldy          #3
/* @202 */ 	lda         (__i6), Y
/* @203 */ 	sta         __i0
/* @205 */ 	iny         
/* @206 */ 	lda         (__i6), Y
/* @207 */ 	sta         __i0+1
/* @210 */ 	lda         __i0
/* @211 */ 	cmp          #3
/* @212 */ 	bne         .ReadString_label_235
/* @213 */ 	lda         __i0+1
/* @215 */ 	bne         .ReadString_label_235
/* @219 */ 	lda         __i9
/* @220 */ 	sta         __i0
/* @221 */ 	lda         __i9+1
/* @222 */ 	sta         __i0+1
/* @223 */ 	lda          #__i9
/* @224 */ 	ldx          #2
/* @226 */ 	jsr         __rinc2
/* @229 */ 	lda         __i12
/* @230 */ 	sta         (__i0)
/* @231 */ 	lda         __i12+1
/* @232 */ 	ldy          #1
/* @233 */ 	sta         (__i0), Y
/* @234 */ 	bra         .ReadString_label_256
.ReadString_label_235:
/* @238 */ 	lda         __i7
/* @239 */ 	sta         __i0
/* @240 */ 	lda         __i7+1
/* @241 */ 	sta         __i0+1
/* @242 */ 	lda          #__i7
/* @243 */ 	jsr         __rinc21
/* @246 */ 	lda         __i12
/* @247 */ 	sta         __i1
/* @248 */ 	lda         __i12+1
/* @249 */ 	sta         __i1+1
/* @254 */ 	lda         __i1
/* @255 */ 	sta         (__i0)
.ReadString_label_256:
.ReadString_label_257:
/* @259 */ 	lda          #__i11
/* @260 */ 	jsr         __rinc21
/* @261 */ 	jmp         .ReadString_label_99
.ReadString_label_262:
/* @265 */ 	lda         (__i6)
/* @270 */ 	beq         .ReadString_label_293
/* @274 */ 	ldx          #0
/* @276 */ 	txa         
/* @277 */ 	cmp         __i10
/* @279 */ 	sbc         __i10+1
/* @280 */ 	bvc         .ReadString_label_275
/* @281 */ 	eor          #128
.ReadString_label_275:
/* @282 */ 	bpl         .ReadString_label_273
/* @283 */ 	inx         
.ReadString_label_273:
/* @284 */ 	stx         __b0
/* @286 */ 	ldx          #10
	jsr          __load_result
/* @287 */ 	lda         #__b0
/* @289 */ 	jsr         __result1
.ReadString_label_290:
/* @291 */ 	ldy          #12
	jmp          __leave
.ReadString_label_293:
/* @296 */ 	ldy          #3
/* @297 */ 	lda         (__i6), Y
/* @298 */ 	sta         __i0
/* @299 */ 	iny         
/* @300 */ 	lda         (__i6), Y
/* @301 */ 	sta         __i0+1
/* @304 */ 	lda         __i0
/* @305 */ 	cmp          #3
/* @306 */ 	bne         .ReadString_label_335
/* @307 */ 	lda         __i0+1
/* @309 */ 	bne         .ReadString_label_335
/* @312 */ 	lda         __i9
/* @313 */ 	cmp         __i8
/* @314 */ 	bne         .ReadString_label_326
/* @315 */ 	lda         __i9+1
/* @316 */ 	cmp         __i8+1
/* @317 */ 	bne         .ReadString_label_326
/* @321 */ 	stz         __b0
/* @322 */ 	ldx          #10
	jsr          __load_result
/* @323 */ 	lda         #__b0
/* @324 */ 	jsr         __result1
/* @325 */ 	bra         .ReadString_label_290
.ReadString_label_326:
/* @327 */ 	ldy          #1
/* @328 */ 	lda          #0
.ReadString_label_329:
/* @331 */ 	sta         (__i9), Y
/* @332 */ 	dey         
/* @333 */ 	bpl         .ReadString_label_329
/* @334 */ 	bra         .ReadString_label_355
.ReadString_label_335:
/* @337 */ 	lda         __i7
/* @338 */ 	cmp         __i8
/* @339 */ 	bne         .ReadString_label_351
/* @340 */ 	lda         __i7+1
/* @341 */ 	cmp         __i8+1
/* @342 */ 	bne         .ReadString_label_351
/* @346 */ 	stz         __b0
/* @347 */ 	ldx          #10
	jsr          __load_result
/* @348 */ 	lda         #__b0
/* @349 */ 	jsr         __result1
/* @350 */ 	bra         .ReadString_label_290
.ReadString_label_351:
/* @352 */ 	lda          #0
/* @353 */ 	tay         
/* @354 */ 	sta         (__i7)
.ReadString_label_355:
/* @358 */ 	lda          #1
/* @359 */ 	sta         __b0
/* @360 */ 	ldx          #10
	jsr          __load_result
/* @361 */ 	lda         #__b0
/* @362 */ 	jsr         __result1
/* @363 */ 	bra         .ReadString_label_290
.func_end_ReadString:
	.size ReadString, .func_end_ReadString-ReadString

	.local  ParseScanSet
	.type ParseScanSet, @function

ParseScanSet:
/* @13 */ 	stx         __result
/* @15 */ 	sty         __result+1
/* @16 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @25 */ 	ldx          #2
	jsr          __arg_value2_i2			// set
/* @29 */ 	ldx          #0
	jsr          __arg_value2_i3			// p
/* @30 */ 	stz         __i0
/* @31 */ 	stz         __i0+1
/* @34 */ 	clc         
/* @35 */ 	lda         __i2
/* @36 */ 	adc          #0
/* @37 */ 	sta         __i4
/* @38 */ 	lda         __i2+1
/* @39 */ 	adc          #0
/* @40 */ 	sta         __i4+1
/* @43 */ 	lda         __i4
/* @44 */ 	sta         __i1
/* @45 */ 	lda         __i4+1
/* @46 */ 	sta         __i1+1
/* @49 */ 	lda         (__i3)
/* @55 */ 	sta         __i4
/* @57 */ 	and          #128
/* @59 */ 	beq         .ParseScanSet_label_58
/* @61 */ 	lda          #255
.ParseScanSet_label_58:
/* @62 */ 	sta         __i4+1
/* @65 */ 	lda         __i4
/* @66 */ 	cmp          #94
/* @67 */ 	bne         .ParseScanSet_label_79
/* @68 */ 	lda         __i4+1
/* @70 */ 	bne         .ParseScanSet_label_79
/* @72 */ 	lda          #1
/* @74 */ 	ldy          #32
/* @75 */ 	sta         (__i2), Y
/* @76 */ 	lda          #__i3
/* @78 */ 	jsr         __rinc21
.ParseScanSet_label_79:
.ParseScanSet_label_80:
/* @83 */ 	lda         (__i3)
/* @89 */ 	sta         __i4
/* @90 */ 	and          #128
/* @92 */ 	beq         .ParseScanSet_label_91
/* @93 */ 	lda          #255
.ParseScanSet_label_91:
/* @94 */ 	sta         __i4+1
/* @96 */ 	lda         __i4
/* @97 */ 	ora         __i4+1
/* @99 */ 	beq         .ParseScanSet_label_181
/* @102 */ 	lda         (__i3)
/* @108 */ 	sta         __i4
/* @109 */ 	and          #128
/* @111 */ 	beq         .ParseScanSet_label_110
/* @112 */ 	lda          #255
.ParseScanSet_label_110:
/* @113 */ 	sta         __i4+1
/* @116 */ 	lda         __i4
/* @117 */ 	cmp          #93
/* @118 */ 	bne         .ParseScanSet_label_149
/* @119 */ 	lda         __i4+1
/* @121 */ 	bne         .ParseScanSet_label_149
/* @123 */ 	lda         __i0
/* @124 */ 	ora         __i0+1
/* @125 */ 	bne         .ParseScanSet_label_144
/* @128 */ 	lda         __i1
/* @129 */ 	sta         __i4
/* @130 */ 	lda         __i1+1
/* @131 */ 	sta         __i4+1
/* @132 */ 	lda          #__i1
/* @133 */ 	jsr         __rinc21
/* @136 */ 	lda          #93
/* @137 */ 	ldy          #0
/* @138 */ 	sta         (__i4)
/* @139 */ 	lda          #__i3
/* @140 */ 	jsr         __rinc21
/* @141 */ 	lda          #__i0
/* @142 */ 	jsr         __rinc21
/* @143 */ 	bra         .ParseScanSet_label_148
.ParseScanSet_label_144:
/* @145 */ 	lda          #__i3
/* @146 */ 	jsr         __rinc21
/* @147 */ 	bra         .ParseScanSet_label_181
.ParseScanSet_label_148:
.ParseScanSet_label_149:
/* @152 */ 	lda         __i1
/* @153 */ 	sta         __i4
/* @154 */ 	lda         __i1+1
/* @155 */ 	sta         __i4+1
/* @156 */ 	lda          #__i1
/* @157 */ 	jsr         __rinc21
/* @160 */ 	lda         __i3
/* @161 */ 	sta         __i5
/* @162 */ 	lda         __i3+1
/* @163 */ 	sta         __i5+1
/* @164 */ 	lda          #__i3
/* @165 */ 	jsr         __rinc21
/* @170 */ 	lda         (__i5)
/* @177 */ 	sta         (__i4)
/* @178 */ 	lda          #__i0
/* @179 */ 	jsr         __rinc21
/* @180 */ 	bra         .ParseScanSet_label_80
.ParseScanSet_label_181:
/* @184 */ 	lda         __i1
/* @185 */ 	sta         __i4
/* @186 */ 	lda         __i1+1
/* @187 */ 	sta         __i4+1
/* @188 */ 	lda          #__i1
/* @189 */ 	jsr         __rinc21
/* @192 */ 	lda          #0
/* @193 */ 	tay         
/* @194 */ 	sta         (__i4)
/* @195 */ 	lda         #__i3
/* @197 */ 	jsr         __result2
/* @199 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_ParseScanSet:
	.size ParseScanSet, .func_end_ParseScanSet-ParseScanSet

	.local  InScanSet
	.type InScanSet, @function

InScanSet:
/* @11 */ 	stx         __result
/* @13 */ 	sty         __result+1
/* @14 */ 	ldx          #8
	jsr          __enter_leaf
	.byte        0x21,0x00,0x00		// Save mask i:1 b:1 l:0 x:0 f:0 
/* @21 */ 	ldx          #2
	jsr          __arg_value2_i1			// set
/* @27 */ 	ldx          #0
	jsr          __arg_value1_b1			// ch
/* @36 */ 	clc         
/* @37 */ 	lda         __i1
/* @38 */ 	adc          #0
/* @39 */ 	sta         __i2
/* @40 */ 	lda         __i1+1
/* @41 */ 	adc          #0
/* @42 */ 	sta         __i2+1
/* @45 */ 	lda         __i2
/* @46 */ 	sta         __i0
/* @47 */ 	lda         __i2+1
/* @48 */ 	sta         __i0+1
/* @49 */ 	stz         __b0
.InScanSet_label_50:
/* @53 */ 	lda         (__i0)
/* @59 */ 	sta         __i2
/* @61 */ 	and          #128
/* @63 */ 	beq         .InScanSet_label_62
/* @65 */ 	lda          #255
.InScanSet_label_62:
/* @66 */ 	sta         __i2+1
/* @68 */ 	lda         __i2
/* @69 */ 	ora         __i2+1
/* @71 */ 	bne         .InScanSet_label_290
/* @291 */ 	jmp         .InScanSet_label_259
.InScanSet_label_290:
/* @74 */ 	lda         __b1
/* @75 */ 	sta         __i2
/* @76 */ 	and          #128
/* @78 */ 	beq         .InScanSet_label_77
/* @79 */ 	lda          #255
.InScanSet_label_77:
/* @80 */ 	sta         __i2+1
/* @83 */ 	lda         (__i0)
/* @89 */ 	sta         __i3
/* @90 */ 	and          #128
/* @92 */ 	beq         .InScanSet_label_91
/* @93 */ 	lda          #255
.InScanSet_label_91:
/* @94 */ 	sta         __i3+1
/* @98 */ 	lda         __i2
/* @99 */ 	cmp         __i3
/* @100 */ 	bne         .InScanSet_label_108
/* @101 */ 	lda         __i2+1
/* @102 */ 	cmp         __i3+1
/* @103 */ 	bne         .InScanSet_label_108
/* @105 */ 	lda          #1
/* @106 */ 	sta         __b0
/* @107 */ 	jmp         .InScanSet_label_259
.InScanSet_label_108:
/* @111 */ 	ldy          #1
/* @112 */ 	lda         (__i0), Y
/* @118 */ 	sta         __i2
/* @119 */ 	and          #128
/* @121 */ 	beq         .InScanSet_label_120
/* @122 */ 	lda          #255
.InScanSet_label_120:
/* @123 */ 	sta         __i2+1
/* @126 */ 	lda         __i2
/* @127 */ 	cmp          #45
/* @128 */ 	beq         .InScanSet_label_292
/* @293 */ 	jmp         .InScanSet_label_253
.InScanSet_label_292:
/* @129 */ 	lda         __i2+1
/* @131 */ 	beq         .InScanSet_label_294
/* @295 */ 	jmp         .InScanSet_label_253
.InScanSet_label_294:
/* @135 */ 	lda         (__i0)
/* @136 */ 	sta         __b2
/* @138 */ 	lda          #__b2
/* @140 */ 	ldx          #4
/* @142 */ 	jsr         __set_var_value1
/* @146 */ 	ldy          #2
/* @147 */ 	lda         (__i0), Y
/* @148 */ 	sta         __b2
/* @150 */ 	lda          #__b2
/* @152 */ 	ldx          #5
/* @153 */ 	jsr         __set_var_value1
/* @155 */ 	ldx          #4
	jsr          __var_value1_b2			// start
/* @157 */ 	lda          #__b2
/* @159 */ 	ldx          #6
/* @160 */ 	jsr         __set_var_value1
.InScanSet_label_161:
/* @163 */ 	ldx          #6
	jsr          __var_value1_b2			// c
/* @167 */ 	lda         __b2
/* @168 */ 	sta         __i2
/* @169 */ 	and          #128
/* @171 */ 	beq         .InScanSet_label_170
/* @172 */ 	lda          #255
.InScanSet_label_170:
/* @173 */ 	sta         __i2+1
/* @175 */ 	ldx          #5
	jsr          __var_value1_b2			// end
/* @179 */ 	lda         __b2
/* @180 */ 	sta         __i3
/* @181 */ 	and          #128
/* @183 */ 	beq         .InScanSet_label_182
/* @184 */ 	lda          #255
.InScanSet_label_182:
/* @185 */ 	sta         __i3+1
/* @189 */ 	clc         
/* @190 */ 	lda         __i3
/* @191 */ 	adc          #1
/* @192 */ 	sta         __i4
/* @193 */ 	lda         __i3+1
/* @194 */ 	adc          #0
/* @195 */ 	sta         __i4+1
/* @199 */ 	lda         __i2
/* @200 */ 	cmp         __i4
/* @201 */ 	bne         .InScanSet_label_198
/* @202 */ 	lda         __i2+1
/* @203 */ 	cmp         __i4+1
/* @204 */ 	beq         .InScanSet_label_249
.InScanSet_label_198:
/* @207 */ 	ldx          #6
	jsr          __var_value1_b2			// c
/* @211 */ 	lda         __b2
/* @212 */ 	sta         __i2
/* @213 */ 	and          #128
/* @215 */ 	beq         .InScanSet_label_214
/* @216 */ 	lda          #255
.InScanSet_label_214:
/* @217 */ 	sta         __i2+1
/* @220 */ 	lda         __b1
/* @221 */ 	sta         __i3
/* @222 */ 	and          #128
/* @224 */ 	beq         .InScanSet_label_223
/* @225 */ 	lda          #255
.InScanSet_label_223:
/* @226 */ 	sta         __i3+1
/* @230 */ 	lda         __i2
/* @231 */ 	cmp         __i3
/* @232 */ 	bne         .InScanSet_label_240
/* @233 */ 	lda         __i2+1
/* @234 */ 	cmp         __i3+1
/* @235 */ 	bne         .InScanSet_label_240
/* @237 */ 	lda          #1
/* @238 */ 	sta         __b0
/* @239 */ 	bra         .InScanSet_label_249
.InScanSet_label_240:
/* @243 */ 	ldx          #6
	jsr          __var_addr_i2			// c
/* @245 */ 	lda          #__i2
/* @247 */ 	jsr         __inc1
/* @248 */ 	bra         .InScanSet_label_161
.InScanSet_label_249:
/* @250 */ 	lda         __b0
/* @251 */ 	bne         .InScanSet_label_259
.InScanSet_label_253:
/* @255 */ 	lda          #__i0
/* @257 */ 	jsr         __rinc21
/* @258 */ 	jmp         .InScanSet_label_50
.InScanSet_label_259:
/* @263 */ 	ldy          #32
/* @264 */ 	lda         (__i1), Y
/* @269 */ 	beq         .InScanSet_label_286
/* @273 */ 	lda         __b0
/* @275 */ 	beq         .InScanSet_label_272
/* @276 */ 	lda          #255
.InScanSet_label_272:
/* @277 */ 	inc          A
/* @278 */ 	sta         __b2
/* @280 */ 	lda         #__b2
/* @282 */ 	jsr         __result1
.InScanSet_label_283:
/* @284 */ 	ldy          #11
	jmp          __leave_leaf
.InScanSet_label_286:
/* @287 */ 	lda         #__b0
/* @288 */ 	jsr         __result1
/* @289 */ 	bra         .InScanSet_label_283
.func_end_InScanSet:
	.size InScanSet, .func_end_InScanSet-InScanSet

	.local  ReadScanSet
	.type ReadScanSet, @function

ReadScanSet:
/* @14 */ 	stx         __result
/* @16 */ 	sty         __result+1
/* @17 */ 	ldx          #9
	jsr          __enter
	.byte        0x29,0x00,0x00		// Save mask i:9 b:1 l:0 x:0 f:0 
/* @22 */ 	ldx          #2
	jsr          __arg_value2_i5			// unget
/* @26 */ 	ldx          #10
	jsr          __arg_value2_i6			// fmt
/* @33 */ 	ldx          #6
	jsr          __arg_value2_i8			// ptr
/* @52 */ 	ldy          #1
/* @53 */ 	lda         (__i6), Y
/* @54 */ 	sta         __i0
/* @56 */ 	iny         
/* @57 */ 	lda         (__i6), Y
/* @58 */ 	sta         __i0+1
/* @61 */ 	lda         __i0
/* @62 */ 	cmp          #255
/* @63 */ 	bne         .ReadScanSet_label_74
/* @64 */ 	lda         __i0+1
/* @65 */ 	cmp          #255
/* @66 */ 	bne         .ReadScanSet_label_74
/* @69 */ 	lda          #255
/* @70 */ 	sta         __i4
/* @71 */ 	lda          #127
/* @72 */ 	sta         __i4+1
/* @73 */ 	bra         .ReadScanSet_label_82
.ReadScanSet_label_74:
/* @76 */ 	ldy          #1
/* @77 */ 	lda         (__i6), Y
/* @78 */ 	sta         __i4
/* @79 */ 	iny         
/* @80 */ 	lda         (__i6), Y
/* @81 */ 	sta         __i4+1
.ReadScanSet_label_82:
/* @84 */ 	lda          #__i4
/* @86 */ 	ldx          #5
/* @88 */ 	jsr         __set_var_value2
/* @89 */ 	lda         __i8
/* @90 */ 	sta         __i7
/* @91 */ 	lda         __i8+1
/* @92 */ 	sta         __i7+1
/* @93 */ 	lda         __i8
/* @94 */ 	sta         __i9
/* @95 */ 	lda         __i8+1
/* @96 */ 	sta         __i9+1
/* @97 */ 	stz         __i10
/* @98 */ 	stz         __i10+1
/* @99 */ 	stz         __i11
/* @100 */ 	stz         __i11+1
.ReadScanSet_label_101:
/* @103 */ 	ldx          #5
	jsr          __var_value2_i0			// numchars
/* @106 */ 	lda         __i11
/* @107 */ 	cmp         __i0
/* @108 */ 	lda         __i11+1
/* @109 */ 	sbc         __i0+1
/* @110 */ 	bvc         .ReadScanSet_label_105
/* @112 */ 	eor          #128
.ReadScanSet_label_105:
/* @113 */ 	bmi         .ReadScanSet_label_388
/* @389 */ 	jmp         .ReadScanSet_label_286
.ReadScanSet_label_388:
/* @115 */ 	ldx          #0
	jsr          __arg_value2_i0			// get
/* @117 */ 	ldx          #4
	jsr          __arg_value2_i1			// data
/* @120 */ 	jsr         __pushi1
/* @121 */ 	ldx         #__i12
/* @122 */ 	ldy          #0
/* @127 */ 	jsr         .ReadScanSet_label_125
/* @128 */ 	bra         .ReadScanSet_label_126
.ReadScanSet_label_125:
/* @129 */ 	jmp         (__i0)
.ReadScanSet_label_126:
/* @131 */ 	jsr         __incsp2
/* @133 */ 	lda         __i12
/* @134 */ 	cmp          #255
/* @135 */ 	bne         .ReadScanSet_label_132
/* @136 */ 	lda         __i12+1
/* @137 */ 	cmp          #255
/* @138 */ 	bne         .ReadScanSet_label_390
/* @391 */ 	jmp         .ReadScanSet_label_286
.ReadScanSet_label_390:
.ReadScanSet_label_132:
/* @142 */ 	ldx          #8
	jsr          __arg_value2_i0			// set
/* @145 */ 	jsr         __pushi0
/* @148 */ 	lda         __i12+1
/* @149 */ 	and          #128
/* @150 */ 	ora         __i12
/* @156 */ 	jsr         __pusha
/* @158 */ 	ldx         #__b2
/* @159 */ 	ldy          #0
/* @160 */ 	jsr         InScanSet
/* @162 */ 	jsr         __incsp4
/* @167 */ 	lda         __b2
/* @169 */ 	beq         .ReadScanSet_label_166
/* @171 */ 	lda          #255
.ReadScanSet_label_166:
/* @172 */ 	inc          A
/* @177 */ 	beq         .ReadScanSet_label_200
/* @179 */ 	ldx          #4
	jsr          __arg_value2_i0			// data
/* @182 */ 	jsr         __pushi0
/* @185 */ 	lda         __i12+1
/* @186 */ 	and          #128
/* @187 */ 	ora         __i12
/* @192 */ 	jsr         __pusha
/* @195 */ 	jsr         .ReadScanSet_label_193
/* @196 */ 	bra         .ReadScanSet_label_194
.ReadScanSet_label_193:
/* @197 */ 	jmp         (__i5)
.ReadScanSet_label_194:
/* @198 */ 	jsr         __incsp4
/* @199 */ 	bra         .ReadScanSet_label_286
.ReadScanSet_label_200:
/* @201 */ 	lda          #__i10
/* @203 */ 	jsr         __rinc21
/* @206 */ 	lda         (__i6)
/* @214 */ 	beq         .ReadScanSet_label_211
/* @215 */ 	lda          #255
.ReadScanSet_label_211:
/* @216 */ 	inc          A
/* @221 */ 	beq         .ReadScanSet_label_281
/* @225 */ 	ldy          #3
/* @226 */ 	lda         (__i6), Y
/* @227 */ 	sta         __i0
/* @229 */ 	iny         
/* @230 */ 	lda         (__i6), Y
/* @231 */ 	sta         __i0+1
/* @234 */ 	lda         __i0
/* @235 */ 	cmp          #3
/* @236 */ 	bne         .ReadScanSet_label_259
/* @237 */ 	lda         __i0+1
/* @239 */ 	bne         .ReadScanSet_label_259
/* @243 */ 	lda         __i9
/* @244 */ 	sta         __i0
/* @245 */ 	lda         __i9+1
/* @246 */ 	sta         __i0+1
/* @247 */ 	lda          #__i9
/* @248 */ 	ldx          #2
/* @250 */ 	jsr         __rinc2
/* @253 */ 	lda         __i12
/* @254 */ 	sta         (__i0)
/* @255 */ 	lda         __i12+1
/* @256 */ 	ldy          #1
/* @257 */ 	sta         (__i0), Y
/* @258 */ 	bra         .ReadScanSet_label_280
.ReadScanSet_label_259:
/* @262 */ 	lda         __i7
/* @263 */ 	sta         __i0
/* @264 */ 	lda         __i7+1
/* @265 */ 	sta         __i0+1
/* @266 */ 	lda          #__i7
/* @267 */ 	jsr         __rinc21
/* @270 */ 	lda         __i12
/* @271 */ 	sta         __i1
/* @272 */ 	lda         __i12+1
/* @273 */ 	sta         __i1+1
/* @278 */ 	lda         __i1
/* @279 */ 	sta         (__i0)
.ReadScanSet_label_280:
.ReadScanSet_label_281:
/* @283 */ 	lda          #__i11
/* @284 */ 	jsr         __rinc21
/* @285 */ 	jmp         .ReadScanSet_label_101
.ReadScanSet_label_286:
/* @289 */ 	lda         (__i6)
/* @294 */ 	beq         .ReadScanSet_label_317
/* @298 */ 	ldx          #0
/* @300 */ 	txa         
/* @301 */ 	cmp         __i10
/* @303 */ 	sbc         __i10+1
/* @304 */ 	bvc         .ReadScanSet_label_299
/* @305 */ 	eor          #128
.ReadScanSet_label_299:
/* @306 */ 	bpl         .ReadScanSet_label_297
/* @307 */ 	inx         
.ReadScanSet_label_297:
/* @308 */ 	stx         __b0
/* @310 */ 	ldx          #10
	jsr          __load_result
/* @311 */ 	lda         #__b0
/* @313 */ 	jsr         __result1
.ReadScanSet_label_314:
/* @315 */ 	ldy          #12
	jmp          __leave
.ReadScanSet_label_317:
/* @320 */ 	ldy          #3
/* @321 */ 	lda         (__i6), Y
/* @322 */ 	sta         __i0
/* @323 */ 	iny         
/* @324 */ 	lda         (__i6), Y
/* @325 */ 	sta         __i0+1
/* @328 */ 	lda         __i0
/* @329 */ 	cmp          #3
/* @330 */ 	bne         .ReadScanSet_label_359
/* @331 */ 	lda         __i0+1
/* @333 */ 	bne         .ReadScanSet_label_359
/* @336 */ 	lda         __i9
/* @337 */ 	cmp         __i8
/* @338 */ 	bne         .ReadScanSet_label_350
/* @339 */ 	lda         __i9+1
/* @340 */ 	cmp         __i8+1
/* @341 */ 	bne         .ReadScanSet_label_350
/* @345 */ 	stz         __b0
/* @346 */ 	ldx          #10
	jsr          __load_result
/* @347 */ 	lda         #__b0
/* @348 */ 	jsr         __result1
/* @349 */ 	bra         .ReadScanSet_label_314
.ReadScanSet_label_350:
/* @351 */ 	ldy          #1
/* @352 */ 	lda          #0
.ReadScanSet_label_353:
/* @355 */ 	sta         (__i9), Y
/* @356 */ 	dey         
/* @357 */ 	bpl         .ReadScanSet_label_353
/* @358 */ 	bra         .ReadScanSet_label_379
.ReadScanSet_label_359:
/* @361 */ 	lda         __i7
/* @362 */ 	cmp         __i8
/* @363 */ 	bne         .ReadScanSet_label_375
/* @364 */ 	lda         __i7+1
/* @365 */ 	cmp         __i8+1
/* @366 */ 	bne         .ReadScanSet_label_375
/* @370 */ 	stz         __b0
/* @371 */ 	ldx          #10
	jsr          __load_result
/* @372 */ 	lda         #__b0
/* @373 */ 	jsr         __result1
/* @374 */ 	bra         .ReadScanSet_label_314
.ReadScanSet_label_375:
/* @376 */ 	lda          #0
/* @377 */ 	tay         
/* @378 */ 	sta         (__i7)
.ReadScanSet_label_379:
/* @382 */ 	lda          #1
/* @383 */ 	sta         __b0
/* @384 */ 	ldx          #10
	jsr          __load_result
/* @385 */ 	lda         #__b0
/* @386 */ 	jsr         __result1
/* @387 */ 	bra         .ReadScanSet_label_314
.func_end_ReadScanSet:
	.size ReadScanSet, .func_end_ReadScanSet-ReadScanSet

	.local  ReadDouble
	.type ReadDouble, @function

ReadDouble:
/* @29 */ 	stx         __result
/* @31 */ 	sty         __result+1
/* @32 */ 	ldx          #60
	jsr          __enter
	.byte        0xaa,0x20,0x01		// Save mask i:10 b:5 l:0 x:1 f:1 
/* @47 */ 	ldx          #6
	jsr          __arg_value2_i4			// ptr
/* @61 */ 	ldx          #0
	jsr          __arg_value2_i6			// get
/* @65 */ 	ldx          #4
	jsr          __arg_value2_i7			// data
/* @89 */ 	lda          #32
/* @91 */ 	sta         __mem_size
/* @93 */ 	ldx          #56
	jsr          __var_addr_i0			// fx
/* @95 */ 	lda         __i0
/* @97 */ 	sta         __mem_dest
/* @98 */ 	lda         __i0+1
/* @100 */ 	sta         __mem_dest+1
/* @102 */ 	jsr         __zeromem1
/* @104 */ 	lda          #16
/* @105 */ 	sta         __mem_size
/* @107 */ 	ldx          #19
	jsr          __var_addr_i0			// n
/* @109 */ 	lda         __i0
/* @110 */ 	sta         __mem_dest
/* @111 */ 	lda         __i0+1
/* @112 */ 	sta         __mem_dest+1
/* @113 */ 	jsr         __zeromem1
/* @115 */ 	ldx          #3
/* @116 */ 	lda          #0
.ReadDouble_label_117:
/* @119 */ 	sta         __f1, X
/* @120 */ 	dex         
/* @121 */ 	bpl         .ReadDouble_label_117
/* @122 */ 	stz         __b2
/* @123 */ 	stz         __b3
/* @124 */ 	jsr         __pushi7
/* @125 */ 	ldx         #__i5
/* @126 */ 	ldy          #0
/* @129 */ 	jsr         .ReadDouble_label_127
/* @130 */ 	bra         .ReadDouble_label_128
.ReadDouble_label_127:
/* @131 */ 	jmp         (__i6)
.ReadDouble_label_128:
/* @133 */ 	jsr         __incsp2
/* @135 */ 	lda         __i5
/* @136 */ 	cmp          #255
/* @137 */ 	bne         .ReadDouble_label_134
/* @138 */ 	lda         __i5+1
/* @139 */ 	cmp          #255
/* @140 */ 	bne         .ReadDouble_label_1106
/* @1107 */ 	jmp         .ReadDouble_label_1101
.ReadDouble_label_1106:
.ReadDouble_label_134:
/* @144 */ 	lda         __i5
/* @145 */ 	cmp          #45
/* @146 */ 	bne         .ReadDouble_label_171
/* @147 */ 	lda         __i5+1
/* @149 */ 	bne         .ReadDouble_label_171
/* @151 */ 	jsr         __pushi7
/* @152 */ 	ldx         #__i5
/* @153 */ 	ldy          #0
/* @156 */ 	jsr         .ReadDouble_label_154
/* @157 */ 	bra         .ReadDouble_label_155
.ReadDouble_label_154:
/* @158 */ 	jmp         (__i6)
.ReadDouble_label_155:
/* @159 */ 	jsr         __incsp2
/* @161 */ 	lda         __i5
/* @162 */ 	cmp          #255
/* @163 */ 	bne         .ReadDouble_label_160
/* @164 */ 	lda         __i5+1
/* @165 */ 	cmp          #255
/* @166 */ 	bne         .ReadDouble_label_1108
/* @1109 */ 	jmp         .ReadDouble_label_1101
.ReadDouble_label_1108:
.ReadDouble_label_160:
/* @169 */ 	lda          #128
/* @170 */ 	sta         __b2
.ReadDouble_label_171:
/* @173 */ 	lda         __i5
/* @174 */ 	cmp          #43
/* @175 */ 	bne         .ReadDouble_label_198
/* @176 */ 	lda         __i5+1
/* @178 */ 	bne         .ReadDouble_label_198
/* @180 */ 	jsr         __pushi7
/* @181 */ 	ldx         #__i5
/* @182 */ 	ldy          #0
/* @185 */ 	jsr         .ReadDouble_label_183
/* @186 */ 	bra         .ReadDouble_label_184
.ReadDouble_label_183:
/* @187 */ 	jmp         (__i6)
.ReadDouble_label_184:
/* @188 */ 	jsr         __incsp2
/* @190 */ 	lda         __i5
/* @191 */ 	cmp          #255
/* @192 */ 	bne         .ReadDouble_label_189
/* @193 */ 	lda         __i5+1
/* @194 */ 	cmp          #255
/* @195 */ 	bne         .ReadDouble_label_1110
/* @1111 */ 	jmp         .ReadDouble_label_1101
.ReadDouble_label_1110:
.ReadDouble_label_189:
.ReadDouble_label_198:
/* @199 */ 	stz         __i8
/* @200 */ 	stz         __i8+1
.ReadDouble_label_201:
/* @205 */ 	ldx         __i5
/* @206 */ 	ldy         __i5+1
/* @208 */ 	jsr         __builtin_isdigit
/* @209 */ 	sta         __i0
/* @210 */ 	stz         __i0+1
/* @213 */ 	lda         __i0+1
/* @215 */ 	and          #128
/* @216 */ 	ora         __i0
/* @217 */ 	sta         __b4
/* @221 */ 	beq         .ReadDouble_label_234
/* @225 */ 	ldx          #0
/* @226 */ 	lda         __i5
/* @227 */ 	cmp          #46
/* @228 */ 	bne         .ReadDouble_label_224
/* @229 */ 	lda         __i5+1
/* @231 */ 	beq         .ReadDouble_label_223
.ReadDouble_label_224:
/* @232 */ 	inx         
.ReadDouble_label_223:
/* @233 */ 	stx         __b4
.ReadDouble_label_234:
/* @236 */ 	lda         __b4
/* @238 */ 	beq         .ReadDouble_label_251
/* @242 */ 	ldx          #0
/* @243 */ 	lda         __i5
/* @244 */ 	cmp          #101
/* @245 */ 	bne         .ReadDouble_label_241
/* @246 */ 	lda         __i5+1
/* @248 */ 	beq         .ReadDouble_label_240
.ReadDouble_label_241:
/* @249 */ 	inx         
.ReadDouble_label_240:
/* @250 */ 	stx         __b4
.ReadDouble_label_251:
/* @253 */ 	lda         __b4
/* @255 */ 	beq         .ReadDouble_label_268
/* @259 */ 	ldx          #0
/* @260 */ 	lda         __i5
/* @261 */ 	cmp          #69
/* @262 */ 	bne         .ReadDouble_label_258
/* @263 */ 	lda         __i5+1
/* @265 */ 	beq         .ReadDouble_label_257
.ReadDouble_label_258:
/* @266 */ 	inx         
.ReadDouble_label_257:
/* @267 */ 	stx         __b4
.ReadDouble_label_268:
/* @270 */ 	lda         __b4
/* @272 */ 	bne         .ReadDouble_label_1112
/* @1113 */ 	jmp         .ReadDouble_label_371
.ReadDouble_label_1112:
/* @275 */ 	sec         
/* @276 */ 	lda         __i5
/* @277 */ 	sbc          #48
/* @278 */ 	sta         __i0
/* @279 */ 	lda         __i5+1
/* @280 */ 	sbc          #0
/* @281 */ 	sta         __i0+1
/* @285 */ 	lda         __i0
/* @286 */ 	sta         __x0
/* @287 */ 	lda         __i0+1
/* @288 */ 	sta         __x0+1
/* @289 */ 	lda          #0
/* @291 */ 	ldy          #7
.ReadDouble_label_292:
/* @293 */ 	sta         __x0, Y
/* @294 */ 	dey         
/* @295 */ 	cpy          #1
/* @296 */ 	bne         .ReadDouble_label_292
/* @299 */ 	ldx          #19
	jsr          __var_addr_i12			// n
/* @302 */ 	ldy          #7
/* @303 */ 	ldx          #7
.ReadDouble_label_304:
/* @305 */ 	lda         __x0, X
/* @306 */ 	sta         (__i12), Y
/* @307 */ 	dey         
/* @308 */ 	dex         
/* @309 */ 	bpl         .ReadDouble_label_304
/* @311 */ 	ldx          #56
	jsr          __var_addr_i13			// fx
/* @315 */ 	clc         
/* @316 */ 	lda         __i13
/* @317 */ 	adc          #16
/* @318 */ 	sta         __i0
/* @319 */ 	lda         __i13+1
/* @320 */ 	adc          #0
/* @321 */ 	sta         __i0+1
/* @324 */ 	jsr         __pushi0
/* @325 */ 	jsr         __MultiplyBy10Half
/* @326 */ 	jsr         __incsp2
/* @331 */ 	jsr         __pushi12
/* @337 */ 	clc         
/* @338 */ 	lda         __i13
/* @339 */ 	adc          #16
/* @340 */ 	sta         __i0
/* @341 */ 	lda         __i13+1
/* @342 */ 	adc          #0
/* @343 */ 	sta         __i0+1
/* @346 */ 	jsr         __pushi0
/* @347 */ 	jsr         __AddHalf
/* @349 */ 	jsr         __incsp4
/* @350 */ 	jsr         __pushi7
/* @351 */ 	ldx         #__i5
/* @352 */ 	ldy          #0
/* @355 */ 	jsr         .ReadDouble_label_353
/* @356 */ 	bra         .ReadDouble_label_354
.ReadDouble_label_353:
/* @357 */ 	jmp         (__i6)
.ReadDouble_label_354:
/* @358 */ 	jsr         __incsp2
/* @360 */ 	lda         __i5
/* @361 */ 	cmp          #255
/* @362 */ 	bne         .ReadDouble_label_359
/* @363 */ 	lda         __i5+1
/* @364 */ 	cmp          #255
/* @365 */ 	bne         .ReadDouble_label_1114
/* @1115 */ 	jmp         .ReadDouble_label_1101
.ReadDouble_label_1114:
.ReadDouble_label_359:
/* @368 */ 	lda          #1
/* @369 */ 	sta         __b3
/* @370 */ 	jmp         .ReadDouble_label_201
.ReadDouble_label_371:
/* @373 */ 	lda         __i5
/* @374 */ 	cmp          #46
/* @375 */ 	beq         .ReadDouble_label_1116
/* @1117 */ 	jmp         .ReadDouble_label_577
.ReadDouble_label_1116:
/* @376 */ 	lda         __i5+1
/* @378 */ 	beq         .ReadDouble_label_1118
/* @1119 */ 	jmp         .ReadDouble_label_577
.ReadDouble_label_1118:
/* @380 */ 	jsr         __pushi7
/* @381 */ 	ldx         #__i5
/* @382 */ 	ldy          #0
/* @385 */ 	jsr         .ReadDouble_label_383
/* @386 */ 	bra         .ReadDouble_label_384
.ReadDouble_label_383:
/* @387 */ 	jmp         (__i6)
.ReadDouble_label_384:
/* @388 */ 	jsr         __incsp2
/* @390 */ 	lda         __i5
/* @391 */ 	cmp          #255
/* @392 */ 	bne         .ReadDouble_label_389
/* @393 */ 	lda         __i5+1
/* @394 */ 	cmp          #255
/* @395 */ 	bne         .ReadDouble_label_1120
/* @1121 */ 	jmp         .ReadDouble_label_1101
.ReadDouble_label_1120:
.ReadDouble_label_389:
.ReadDouble_label_398:
/* @402 */ 	ldx         __i5
/* @403 */ 	ldy         __i5+1
/* @404 */ 	jsr         __builtin_isdigit
/* @405 */ 	sta         __i0
/* @406 */ 	stz         __i0+1
/* @409 */ 	lda         __i0+1
/* @410 */ 	and          #128
/* @411 */ 	ora         __i0
/* @412 */ 	sta         __b5
/* @416 */ 	beq         .ReadDouble_label_429
/* @420 */ 	ldx          #0
/* @421 */ 	lda         __i5
/* @422 */ 	cmp          #101
/* @423 */ 	bne         .ReadDouble_label_419
/* @424 */ 	lda         __i5+1
/* @426 */ 	beq         .ReadDouble_label_418
.ReadDouble_label_419:
/* @427 */ 	inx         
.ReadDouble_label_418:
/* @428 */ 	stx         __b5
.ReadDouble_label_429:
/* @431 */ 	lda         __b5
/* @433 */ 	beq         .ReadDouble_label_446
/* @437 */ 	ldx          #0
/* @438 */ 	lda         __i5
/* @439 */ 	cmp          #69
/* @440 */ 	bne         .ReadDouble_label_436
/* @441 */ 	lda         __i5+1
/* @443 */ 	beq         .ReadDouble_label_435
.ReadDouble_label_436:
/* @444 */ 	inx         
.ReadDouble_label_435:
/* @445 */ 	stx         __b5
.ReadDouble_label_446:
/* @448 */ 	lda         __b5
/* @450 */ 	bne         .ReadDouble_label_1122
/* @1123 */ 	jmp         .ReadDouble_label_550
.ReadDouble_label_1122:
/* @453 */ 	sec         
/* @454 */ 	lda         __i5
/* @455 */ 	sbc          #48
/* @456 */ 	sta         __i0
/* @457 */ 	lda         __i5+1
/* @458 */ 	sbc          #0
/* @459 */ 	sta         __i0+1
/* @463 */ 	lda         __i0
/* @464 */ 	sta         __x0
/* @465 */ 	lda         __i0+1
/* @466 */ 	sta         __x0+1
/* @467 */ 	lda          #0
/* @468 */ 	ldy          #7
.ReadDouble_label_469:
/* @470 */ 	sta         __x0, Y
/* @471 */ 	dey         
/* @472 */ 	cpy          #1
/* @473 */ 	bne         .ReadDouble_label_469
/* @476 */ 	ldx          #19
	jsr          __var_addr_i12			// n
/* @479 */ 	ldy          #7
/* @480 */ 	ldx          #7
.ReadDouble_label_481:
/* @482 */ 	lda         __x0, X
/* @483 */ 	sta         (__i12), Y
/* @484 */ 	dey         
/* @485 */ 	dex         
/* @486 */ 	bpl         .ReadDouble_label_481
/* @488 */ 	ldx          #56
	jsr          __var_addr_i13			// fx
/* @492 */ 	clc         
/* @493 */ 	lda         __i13
/* @494 */ 	adc          #16
/* @495 */ 	sta         __i0
/* @496 */ 	lda         __i13+1
/* @497 */ 	adc          #0
/* @498 */ 	sta         __i0+1
/* @501 */ 	jsr         __pushi0
/* @502 */ 	jsr         __MultiplyBy10Half
/* @503 */ 	jsr         __incsp2
/* @508 */ 	jsr         __pushi12
/* @514 */ 	clc         
/* @515 */ 	lda         __i13
/* @516 */ 	adc          #16
/* @517 */ 	sta         __i0
/* @518 */ 	lda         __i13+1
/* @519 */ 	adc          #0
/* @520 */ 	sta         __i0+1
/* @523 */ 	jsr         __pushi0
/* @524 */ 	jsr         __AddHalf
/* @525 */ 	jsr         __incsp4
/* @526 */ 	jsr         __pushi7
/* @527 */ 	ldx         #__i5
/* @528 */ 	ldy          #0
/* @531 */ 	jsr         .ReadDouble_label_529
/* @532 */ 	bra         .ReadDouble_label_530
.ReadDouble_label_529:
/* @533 */ 	jmp         (__i6)
.ReadDouble_label_530:
/* @534 */ 	jsr         __incsp2
/* @536 */ 	lda         __i5
/* @537 */ 	cmp          #255
/* @538 */ 	bne         .ReadDouble_label_535
/* @539 */ 	lda         __i5+1
/* @540 */ 	cmp          #255
/* @541 */ 	bne         .ReadDouble_label_1124
/* @1125 */ 	jmp         .ReadDouble_label_1101
.ReadDouble_label_1124:
.ReadDouble_label_535:
/* @544 */ 	lda          #1
/* @545 */ 	sta         __b3
/* @546 */ 	lda          #__i8
/* @548 */ 	jsr         __rinc21
/* @549 */ 	jmp         .ReadDouble_label_398
.ReadDouble_label_550:
/* @551 */ 	stz         __i9
/* @552 */ 	stz         __i9+1
.ReadDouble_label_553:
/* @555 */ 	lda         __i9
/* @556 */ 	cmp         __i8
/* @557 */ 	lda         __i9+1
/* @558 */ 	sbc         __i8+1
/* @559 */ 	bvc         .ReadDouble_label_554
/* @560 */ 	eor          #128
.ReadDouble_label_554:
/* @561 */ 	bpl         .ReadDouble_label_576
/* @563 */ 	ldx          #56
	jsr          __var_addr_i0			// fx
/* @566 */ 	jsr         __pushi0
/* @568 */ 	ldx         #__b0
/* @569 */ 	ldy          #0
/* @570 */ 	jsr         __DivideBy10
/* @571 */ 	jsr         __incsp2
/* @573 */ 	lda          #__i9
/* @574 */ 	jsr         __rinc21
/* @575 */ 	bra         .ReadDouble_label_553
.ReadDouble_label_576:
.ReadDouble_label_577:
/* @583 */ 	ldx          #1
/* @584 */ 	lda         __i5
/* @585 */ 	cmp          #101
/* @586 */ 	bne         .ReadDouble_label_581
/* @587 */ 	lda         __i5+1
/* @589 */ 	beq         .ReadDouble_label_582
.ReadDouble_label_581:
/* @590 */ 	dex         
.ReadDouble_label_582:
/* @591 */ 	stx         __b5
/* @593 */ 	txa         
/* @594 */ 	bne         .ReadDouble_label_607
/* @598 */ 	ldx          #1
/* @599 */ 	lda         __i5
/* @600 */ 	cmp          #69
/* @601 */ 	bne         .ReadDouble_label_596
/* @602 */ 	lda         __i5+1
/* @604 */ 	beq         .ReadDouble_label_597
.ReadDouble_label_596:
/* @605 */ 	dex         
.ReadDouble_label_597:
/* @606 */ 	stx         __b5
.ReadDouble_label_607:
/* @609 */ 	lda         __b5
/* @611 */ 	bne         .ReadDouble_label_1126
/* @1127 */ 	jmp         .ReadDouble_label_840
.ReadDouble_label_1126:
/* @612 */ 	jsr         __pushi7
/* @613 */ 	ldx         #__i5
/* @614 */ 	ldy          #0
/* @617 */ 	jsr         .ReadDouble_label_615
/* @618 */ 	bra         .ReadDouble_label_616
.ReadDouble_label_615:
/* @619 */ 	jmp         (__i6)
.ReadDouble_label_616:
/* @620 */ 	jsr         __incsp2
/* @622 */ 	lda         __i5
/* @623 */ 	cmp          #255
/* @624 */ 	bne         .ReadDouble_label_621
/* @625 */ 	lda         __i5+1
/* @626 */ 	cmp          #255
/* @627 */ 	bne         .ReadDouble_label_1128
/* @1129 */ 	jmp         .ReadDouble_label_1101
.ReadDouble_label_1128:
.ReadDouble_label_621:
/* @634 */ 	ldx          #1
/* @635 */ 	lda         __i5
/* @636 */ 	cmp          #45
/* @637 */ 	bne         .ReadDouble_label_632
/* @638 */ 	lda         __i5+1
/* @640 */ 	beq         .ReadDouble_label_633
.ReadDouble_label_632:
/* @641 */ 	dex         
.ReadDouble_label_633:
/* @642 */ 	stx         __b0
/* @644 */ 	lda          #__b0
/* @646 */ 	ldx          #20
/* @648 */ 	jsr         __set_var_value1
/* @654 */ 	ldx          #1
/* @655 */ 	lda         __i5
/* @656 */ 	cmp          #43
/* @657 */ 	bne         .ReadDouble_label_652
/* @658 */ 	lda         __i5+1
/* @660 */ 	beq         .ReadDouble_label_653
.ReadDouble_label_652:
/* @661 */ 	dex         
.ReadDouble_label_653:
/* @662 */ 	stx         __b6
/* @664 */ 	txa         
/* @665 */ 	bne         .ReadDouble_label_678
/* @669 */ 	ldx          #1
/* @670 */ 	lda         __i5
/* @671 */ 	cmp          #45
/* @672 */ 	bne         .ReadDouble_label_667
/* @673 */ 	lda         __i5+1
/* @675 */ 	beq         .ReadDouble_label_668
.ReadDouble_label_667:
/* @676 */ 	dex         
.ReadDouble_label_668:
/* @677 */ 	stx         __b6
.ReadDouble_label_678:
/* @680 */ 	lda         __b6
/* @682 */ 	beq         .ReadDouble_label_701
/* @683 */ 	jsr         __pushi7
/* @684 */ 	ldx         #__i5
/* @685 */ 	ldy          #0
/* @688 */ 	jsr         .ReadDouble_label_686
/* @689 */ 	bra         .ReadDouble_label_687
.ReadDouble_label_686:
/* @690 */ 	jmp         (__i6)
.ReadDouble_label_687:
/* @691 */ 	jsr         __incsp2
/* @693 */ 	lda         __i5
/* @694 */ 	cmp          #255
/* @695 */ 	bne         .ReadDouble_label_692
/* @696 */ 	lda         __i5+1
/* @697 */ 	cmp          #255
/* @698 */ 	bne         .ReadDouble_label_1130
/* @1131 */ 	jmp         .ReadDouble_label_1101
.ReadDouble_label_1130:
.ReadDouble_label_692:
.ReadDouble_label_701:
/* @702 */ 	stz         __i10
/* @703 */ 	stz         __i10+1
.ReadDouble_label_704:
/* @706 */ 	ldx         __i5
/* @707 */ 	ldy         __i5+1
/* @708 */ 	jsr         __builtin_isdigit
/* @710 */ 	stz         __i0+1
/* @714 */ 	cmp          #0
/* @715 */ 	beq         .ReadDouble_label_767
/* @718 */ 	lda          #__i0
/* @719 */ 	ldx          #__i10
/* @721 */ 	jsr         __smul2_10
/* @725 */ 	clc         
/* @726 */ 	lda         __i0
/* @727 */ 	adc         __i5
/* @728 */ 	sta         __i1
/* @729 */ 	lda         __i0+1
/* @730 */ 	adc         __i5+1
/* @731 */ 	sta         __i1+1
/* @735 */ 	sec         
/* @736 */ 	lda         __i1
/* @737 */ 	sbc          #48
/* @738 */ 	sta         __i0
/* @739 */ 	lda         __i1+1
/* @740 */ 	sbc          #0
/* @741 */ 	sta         __i0+1
/* @744 */ 	lda         __i0
/* @745 */ 	sta         __i10
/* @746 */ 	lda         __i0+1
/* @747 */ 	sta         __i10+1
/* @748 */ 	jsr         __pushi7
/* @749 */ 	ldx         #__i5
/* @750 */ 	ldy          #0
/* @753 */ 	jsr         .ReadDouble_label_751
/* @754 */ 	bra         .ReadDouble_label_752
.ReadDouble_label_751:
/* @755 */ 	jmp         (__i6)
.ReadDouble_label_752:
/* @756 */ 	jsr         __incsp2
/* @758 */ 	lda         __i5
/* @759 */ 	cmp          #255
/* @760 */ 	bne         .ReadDouble_label_757
/* @761 */ 	lda         __i5+1
/* @762 */ 	cmp          #255
/* @763 */ 	bne         .ReadDouble_label_1132
/* @1133 */ 	jmp         .ReadDouble_label_1101
.ReadDouble_label_1132:
.ReadDouble_label_757:
/* @766 */ 	bra         .ReadDouble_label_704
.ReadDouble_label_767:
/* @769 */ 	ldx          #20
	jsr          __var_value1_b0			// negative_exp
/* @771 */ 	lda         __b0
/* @773 */ 	beq         .ReadDouble_label_801
/* @774 */ 	stz         __i11
/* @775 */ 	stz         __i11+1
.ReadDouble_label_776:
/* @778 */ 	lda         __i11
/* @779 */ 	cmp         __i10
/* @780 */ 	lda         __i11+1
/* @781 */ 	sbc         __i10+1
/* @782 */ 	bvc         .ReadDouble_label_777
/* @783 */ 	eor          #128
.ReadDouble_label_777:
/* @784 */ 	bpl         .ReadDouble_label_799
/* @786 */ 	ldx          #56
	jsr          __var_addr_i0			// fx
/* @789 */ 	jsr         __pushi0
/* @791 */ 	ldx         #__b0
/* @792 */ 	ldy          #0
/* @793 */ 	jsr         __DivideBy10
/* @794 */ 	jsr         __incsp2
/* @796 */ 	lda          #__i11
/* @797 */ 	jsr         __rinc21
/* @798 */ 	bra         .ReadDouble_label_776
.ReadDouble_label_799:
/* @800 */ 	bra         .ReadDouble_label_839
.ReadDouble_label_801:
/* @804 */ 	stz         __i0
/* @805 */ 	stz         __i0+1
/* @806 */ 	lda          #__i0
/* @808 */ 	ldx          #22
/* @810 */ 	jsr         __set_var_value2
.ReadDouble_label_811:
/* @813 */ 	ldx          #22
	jsr          __var_value2_i0			// i
/* @816 */ 	lda         __i0
/* @817 */ 	cmp         __i10
/* @818 */ 	lda         __i0+1
/* @819 */ 	sbc         __i10+1
/* @820 */ 	bvc         .ReadDouble_label_815
/* @821 */ 	eor          #128
.ReadDouble_label_815:
/* @822 */ 	bpl         .ReadDouble_label_838
/* @824 */ 	ldx          #56
	jsr          __var_addr_i0			// fx
/* @827 */ 	jsr         __pushi0
/* @828 */ 	jsr         __MultiplyBy10
/* @829 */ 	jsr         __incsp2
/* @832 */ 	ldx          #22
	jsr          __var_addr_i0			// i
/* @834 */ 	lda          #__i0
/* @836 */ 	jsr         __inc21
/* @837 */ 	bra         .ReadDouble_label_811
.ReadDouble_label_838:
.ReadDouble_label_839:
.ReadDouble_label_840:
/* @842 */ 	ldx          #2
	jsr          __arg_value2_i0			// unget
/* @843 */ 	jsr         __pushi7
/* @846 */ 	lda         __i5+1
/* @847 */ 	and          #128
/* @848 */ 	ora         __i5
/* @854 */ 	jsr         __pusha
/* @859 */ 	jsr         .ReadDouble_label_857
/* @860 */ 	bra         .ReadDouble_label_858
.ReadDouble_label_857:
/* @861 */ 	jmp         (__i0)
.ReadDouble_label_858:
/* @862 */ 	jsr         __incsp4
/* @865 */ 	lda          #127
/* @866 */ 	sta         __i0
/* @868 */ 	stz         __i0+1
/* @869 */ 	lda          #__i0
/* @871 */ 	ldx          #24
/* @872 */ 	jsr         __set_var_value2
/* @874 */ 	ldx          #56
	jsr          __var_addr_i12			// fx
/* @877 */ 	jsr         __pushi12
/* @879 */ 	ldx         #__b6
/* @880 */ 	ldy          #0
/* @881 */ 	jsr         __IsZero
/* @882 */ 	jsr         __incsp2
/* @884 */ 	lda         __b6
/* @886 */ 	beq         .ReadDouble_label_897
/* @889 */ 	stz         __b0
/* @890 */ 	ldx          #61
	jsr          __load_result
/* @891 */ 	lda         #__b0
/* @893 */ 	jsr         __result1
.ReadDouble_label_894:
/* @895 */ 	ldy          #63
	jmp          __leave
.ReadDouble_label_897:
/* @903 */ 	clc         
/* @904 */ 	lda         __i12
/* @905 */ 	adc          #16
/* @906 */ 	sta         __i0
/* @907 */ 	lda         __i12+1
/* @908 */ 	adc          #0
/* @909 */ 	sta         __i0+1
/* @912 */ 	jsr         __pushi0
/* @914 */ 	ldx         #__b6
/* @915 */ 	ldy          #0
/* @916 */ 	jsr         __IsZeroHalf
/* @917 */ 	jsr         __incsp2
/* @919 */ 	lda         __b6
/* @921 */ 	beq         .ReadDouble_label_974
.ReadDouble_label_922:
/* @924 */ 	ldx          #56
	jsr          __var_addr_i13			// fx
/* @928 */ 	clc         
/* @929 */ 	lda         __i13
/* @930 */ 	adc          #16
/* @931 */ 	sta         __i0
/* @932 */ 	lda         __i13+1
/* @933 */ 	adc          #0
/* @934 */ 	sta         __i0+1
/* @937 */ 	jsr         __pushi0
/* @939 */ 	ldx         #__b6
/* @940 */ 	ldy          #0
/* @941 */ 	jsr         __IsOneHalf
/* @942 */ 	jsr         __incsp2
/* @947 */ 	lda         __b6
/* @949 */ 	beq         .ReadDouble_label_946
/* @951 */ 	lda          #255
.ReadDouble_label_946:
/* @952 */ 	inc          A
/* @957 */ 	beq         .ReadDouble_label_972
/* @962 */ 	jsr         __pushi13
/* @963 */ 	jsr         __LShift
/* @964 */ 	jsr         __incsp2
/* @966 */ 	ldx          #24
	jsr          __var_addr_i0			// exp
/* @968 */ 	lda          #__i0
/* @970 */ 	jsr         __dec21
/* @971 */ 	bra         .ReadDouble_label_922
.ReadDouble_label_972:
/* @973 */ 	bra         .ReadDouble_label_1024
.ReadDouble_label_974:
.ReadDouble_label_975:
/* @977 */ 	ldx          #56
	jsr          __var_addr_i13			// fx
/* @981 */ 	clc         
/* @982 */ 	lda         __i13
/* @983 */ 	adc          #16
/* @984 */ 	sta         __i0
/* @985 */ 	lda         __i13+1
/* @986 */ 	adc          #0
/* @987 */ 	sta         __i0+1
/* @990 */ 	jsr         __pushi0
/* @992 */ 	ldx         #__b6
/* @993 */ 	ldy          #0
/* @994 */ 	jsr         __IsOneHalf
/* @995 */ 	jsr         __incsp2
/* @1000 */ 	lda         __b6
/* @1002 */ 	beq         .ReadDouble_label_999
/* @1003 */ 	lda          #255
.ReadDouble_label_999:
/* @1004 */ 	inc          A
/* @1009 */ 	beq         .ReadDouble_label_1023
/* @1014 */ 	jsr         __pushi13
/* @1015 */ 	jsr         __RShift
/* @1016 */ 	jsr         __incsp2
/* @1018 */ 	ldx          #24
	jsr          __var_addr_i0			// exp
/* @1020 */ 	lda          #__i0
/* @1021 */ 	jsr         __inc21
/* @1022 */ 	bra         .ReadDouble_label_975
.ReadDouble_label_1023:
.ReadDouble_label_1024:
/* @1031 */ 	ldy          #15
/* @1032 */ 	ldx          #7
.ReadDouble_label_1033:
/* @1034 */ 	lda         (__i12), Y
/* @1035 */ 	sta         __x0, X
/* @1036 */ 	dey         
/* @1037 */ 	dex         
/* @1038 */ 	bpl         .ReadDouble_label_1033
/* @1041 */ 	ldx          #7
.ReadDouble_label_1042:
/* @1043 */ 	lda         __x0, X
/* @1044 */ 	sta         __x1, X
/* @1045 */ 	dex         
/* @1046 */ 	bpl         .ReadDouble_label_1042
/* @1047 */ 	jsr         __pushx1
/* @1049 */ 	ldx          #24
	jsr          __var_value2_i0			// exp
/* @1052 */ 	jsr         __pushi0
/* @1055 */ 	lda         __b2
/* @1056 */ 	sta         __i0
/* @1057 */ 	and          #128
/* @1059 */ 	beq         .ReadDouble_label_1058
/* @1060 */ 	lda          #255
.ReadDouble_label_1058:
/* @1061 */ 	sta         __i0+1
/* @1064 */ 	jsr         __pushi0
/* @1065 */ 	ldx         #__f1
/* @1066 */ 	ldy          #0
/* @1067 */ 	jsr         __packIEEE754
/* @1069 */ 	jsr         __incsp12
/* @1071 */ 	ldx          #8
	jsr          __arg_value2_i0			// fmt
/* @1076 */ 	lda         (__i0)
/* @1084 */ 	beq         .ReadDouble_label_1081
/* @1085 */ 	lda          #255
.ReadDouble_label_1081:
/* @1086 */ 	inc          A
/* @1091 */ 	beq         .ReadDouble_label_1100
/* @1092 */ 	ldy          #3
/* @1093 */ 	ldx          #3
.ReadDouble_label_1094:
/* @1095 */ 	lda         __f1, X
/* @1096 */ 	sta         (__i4), Y
/* @1097 */ 	dey         
/* @1098 */ 	dex         
/* @1099 */ 	bpl         .ReadDouble_label_1094
.ReadDouble_label_1100:
.ReadDouble_label_1101:
/* @1102 */ 	ldx          #61
	jsr          __load_result
/* @1103 */ 	lda         #__b3
/* @1104 */ 	jsr         __result1
/* @1105 */ 	jmp         .ReadDouble_label_894
.func_end_ReadDouble:
	.size ReadDouble, .func_end_ReadDouble-ReadDouble

	.local  Scanf
	.type Scanf, @function

Scanf:
/* @43 */ 	stx         __result
/* @45 */ 	sty         __result+1
/* @46 */ 	ldx          #45
	jsr          __enter
	.byte        0xa9,0x00,0x00		// Save mask i:9 b:5 l:0 x:0 f:0 
/* @63 */ 	ldx          #6
	jsr          __arg_value2_i0			// format
/* @76 */ 	ldx          #4
	jsr          __arg_value2_i7			// data
/* @80 */ 	ldx          #2
	jsr          __arg_value2_i8			// unget
/* @84 */ 	ldx          #0
	jsr          __arg_value2_i9			// get
/* @91 */ 	lda         __i0
/* @92 */ 	sta         __i10
/* @93 */ 	lda         __i0+1
/* @94 */ 	sta         __i10+1
/* @95 */ 	stz         __i5
/* @96 */ 	stz         __i5+1
/* @97 */ 	stz         __b2
.Scanf_label_98:
/* @101 */ 	lda         (__i10)
/* @107 */ 	sta         __i0
/* @109 */ 	and          #128
/* @111 */ 	beq         .Scanf_label_110
/* @113 */ 	lda          #255
.Scanf_label_110:
/* @114 */ 	sta         __i0+1
/* @116 */ 	lda         __i0
/* @117 */ 	ora         __i0+1
/* @119 */ 	bne         .Scanf_label_1131
/* @1132 */ 	jmp         .Scanf_label_1087
.Scanf_label_1131:
/* @122 */ 	lda         (__i10)
/* @128 */ 	sta         __i0
/* @129 */ 	and          #128
/* @131 */ 	beq         .Scanf_label_130
/* @132 */ 	lda          #255
.Scanf_label_130:
/* @133 */ 	sta         __i0+1
/* @136 */ 	lda         __i0
/* @137 */ 	cmp          #37
/* @138 */ 	beq         .Scanf_label_1133
/* @1134 */ 	jmp         .Scanf_label_953
.Scanf_label_1133:
/* @139 */ 	lda         __i0+1
/* @141 */ 	beq         .Scanf_label_1135
/* @1136 */ 	jmp         .Scanf_label_953
.Scanf_label_1135:
/* @143 */ 	lda          #__i10
/* @145 */ 	jsr         __rinc21
/* @147 */ 	ldx          #8
	jsr          __var_addr_i11			// fmt
/* @150 */ 	jsr         __pushi11
/* @151 */ 	jsr         __pushi10
/* @152 */ 	ldx         #__i10
/* @153 */ 	ldy          #0
/* @154 */ 	jsr         CollectFormat
/* @156 */ 	jsr         __incsp4
/* @157 */ 	stz         __i6
/* @158 */ 	stz         __i6+1
/* @164 */ 	lda         (__i11)
/* @172 */ 	beq         .Scanf_label_169
/* @173 */ 	lda          #255
.Scanf_label_169:
/* @174 */ 	inc          A
/* @179 */ 	beq         .Scanf_label_205
/* @182 */ 	ldx          #8
	jsr          __arg_addr_i0			// ap
/* @184 */ 	lda         __i0
/* @186 */ 	sta         __mem_src
/* @187 */ 	lda         __i0+1
/* @189 */ 	sta         __mem_src+1
/* @192 */ 	lda         #__i0
/* @194 */ 	sta         __mem_dest
/* @196 */ 	stz         __mem_dest+1
/* @198 */ 	jsr         __builtin_va_arg2
/* @201 */ 	lda         __i0
/* @202 */ 	sta         __i6
/* @203 */ 	lda         __i0+1
/* @204 */ 	sta         __i6+1
.Scanf_label_205:
/* @210 */ 	lda         (__i10)
/* @216 */ 	sta         __i0
/* @217 */ 	and          #128
/* @219 */ 	beq         .Scanf_label_218
/* @220 */ 	lda          #255
.Scanf_label_218:
/* @221 */ 	sta         __i0+1
/* @226 */ 	ldx          #0
/* @227 */ 	lda         __i0
/* @228 */ 	cmp          #91
/* @229 */ 	bne         .Scanf_label_225
/* @230 */ 	lda         __i0+1
/* @232 */ 	beq         .Scanf_label_224
.Scanf_label_225:
/* @233 */ 	inx         
.Scanf_label_224:
/* @234 */ 	stx         __b4
/* @236 */ 	txa         
/* @237 */ 	cmp          #0
/* @238 */ 	beq         .Scanf_label_266
/* @241 */ 	lda         (__i10)
/* @247 */ 	sta         __i0
/* @248 */ 	and          #128
/* @250 */ 	beq         .Scanf_label_249
/* @251 */ 	lda          #255
.Scanf_label_249:
/* @252 */ 	sta         __i0+1
/* @257 */ 	ldx          #0
/* @258 */ 	lda         __i0
/* @259 */ 	cmp          #99
/* @260 */ 	bne         .Scanf_label_256
/* @261 */ 	lda         __i0+1
/* @263 */ 	beq         .Scanf_label_255
.Scanf_label_256:
/* @264 */ 	inx         
.Scanf_label_255:
/* @265 */ 	stx         __b4
.Scanf_label_266:
/* @268 */ 	lda         __b4
/* @270 */ 	beq         .Scanf_label_298
/* @273 */ 	lda         (__i10)
/* @279 */ 	sta         __i0
/* @280 */ 	and          #128
/* @282 */ 	beq         .Scanf_label_281
/* @283 */ 	lda          #255
.Scanf_label_281:
/* @284 */ 	sta         __i0+1
/* @289 */ 	ldx          #0
/* @290 */ 	lda         __i0
/* @291 */ 	cmp          #110
/* @292 */ 	bne         .Scanf_label_288
/* @293 */ 	lda         __i0+1
/* @295 */ 	beq         .Scanf_label_287
.Scanf_label_288:
/* @296 */ 	inx         
.Scanf_label_287:
/* @297 */ 	stx         __b4
.Scanf_label_298:
/* @300 */ 	lda         __b4
/* @302 */ 	beq         .Scanf_label_309
/* @303 */ 	jsr         __pushi7
/* @304 */ 	jsr         __pushi8
/* @305 */ 	jsr         __pushi9
/* @306 */ 	jsr         SkipInputWhiteSpace
/* @308 */ 	jsr         __incsp6
.Scanf_label_309:
/* @312 */ 	lda         (__i10)
/* @313 */ 	sta         __b5
/* @316 */ 	sec         
/* @318 */ 	sbc          #100
/* @319 */ 	bvc         .Scanf_label_315
/* @320 */ 	eor          #128
.Scanf_label_315:
/* @321 */ 	bmi         .Scanf_label_389
/* @324 */ 	lda         __b5
/* @325 */ 	cmp          #100
/* @326 */ 	bne         .Scanf_label_1137
/* @1138 */ 	jmp         .Scanf_label_520
.Scanf_label_1137:
/* @330 */ 	lda         __b5
/* @331 */ 	cmp          #101
/* @332 */ 	bne         .Scanf_label_1139
/* @1140 */ 	jmp         .Scanf_label_905
.Scanf_label_1139:
/* @336 */ 	lda         __b5
/* @337 */ 	cmp          #102
/* @338 */ 	bne         .Scanf_label_1141
/* @1142 */ 	jmp         .Scanf_label_906
.Scanf_label_1141:
/* @342 */ 	lda         __b5
/* @343 */ 	cmp          #103
/* @344 */ 	bne         .Scanf_label_1143
/* @1144 */ 	jmp         .Scanf_label_907
.Scanf_label_1143:
/* @348 */ 	lda         __b5
/* @349 */ 	cmp          #105
/* @350 */ 	bne         .Scanf_label_1145
/* @1146 */ 	jmp         .Scanf_label_596
.Scanf_label_1145:
/* @354 */ 	lda         __b5
/* @355 */ 	cmp          #110
/* @356 */ 	bne         .Scanf_label_1147
/* @1148 */ 	jmp         .Scanf_label_841
.Scanf_label_1147:
/* @360 */ 	lda         __b5
/* @361 */ 	cmp          #111
/* @362 */ 	bne         .Scanf_label_1149
/* @1150 */ 	jmp         .Scanf_label_638
.Scanf_label_1149:
/* @366 */ 	lda         __b5
/* @367 */ 	cmp          #112
/* @368 */ 	bne         .Scanf_label_1151
/* @1152 */ 	jmp         .Scanf_label_803
.Scanf_label_1151:
/* @372 */ 	lda         __b5
/* @373 */ 	cmp          #115
/* @374 */ 	bne         .Scanf_label_1153
/* @1154 */ 	jmp         .Scanf_label_726
.Scanf_label_1153:
/* @378 */ 	lda         __b5
/* @379 */ 	cmp          #117
/* @380 */ 	bne         .Scanf_label_1155
/* @1156 */ 	jmp         .Scanf_label_521
.Scanf_label_1155:
/* @384 */ 	lda         __b5
/* @385 */ 	cmp          #120
/* @386 */ 	bne         .Scanf_label_1157
/* @1158 */ 	jmp         .Scanf_label_682
.Scanf_label_1157:
/* @388 */ 	jmp         .Scanf_label_949
.Scanf_label_389:
/* @392 */ 	lda         __b5
/* @393 */ 	cmp          #37
/* @394 */ 	beq         .Scanf_label_451
/* @398 */ 	lda         __b5
/* @399 */ 	cmp          #65
/* @400 */ 	bne         .Scanf_label_1159
/* @1160 */ 	jmp         .Scanf_label_908
.Scanf_label_1159:
/* @404 */ 	lda         __b5
/* @405 */ 	cmp          #69
/* @406 */ 	bne         .Scanf_label_1161
/* @1162 */ 	jmp         .Scanf_label_909
.Scanf_label_1161:
/* @410 */ 	lda         __b5
/* @411 */ 	cmp          #70
/* @412 */ 	bne         .Scanf_label_1163
/* @1164 */ 	jmp         .Scanf_label_910
.Scanf_label_1163:
/* @416 */ 	lda         __b5
/* @417 */ 	cmp          #71
/* @418 */ 	bne         .Scanf_label_1165
/* @1166 */ 	jmp         .Scanf_label_911
.Scanf_label_1165:
/* @422 */ 	lda         __b5
/* @423 */ 	cmp          #79
/* @424 */ 	bne         .Scanf_label_1167
/* @1168 */ 	jmp         .Scanf_label_639
.Scanf_label_1167:
/* @428 */ 	lda         __b5
/* @429 */ 	cmp          #88
/* @430 */ 	bne         .Scanf_label_1169
/* @1170 */ 	jmp         .Scanf_label_683
.Scanf_label_1169:
/* @434 */ 	lda         __b5
/* @435 */ 	cmp          #91
/* @436 */ 	bne         .Scanf_label_1171
/* @1172 */ 	jmp         .Scanf_label_850
.Scanf_label_1171:
/* @440 */ 	lda         __b5
/* @441 */ 	cmp          #97
/* @442 */ 	bne         .Scanf_label_1173
/* @1174 */ 	jmp         .Scanf_label_904
.Scanf_label_1173:
/* @446 */ 	lda         __b5
/* @447 */ 	cmp          #99
/* @448 */ 	bne         .Scanf_label_1175
/* @1176 */ 	jmp         .Scanf_label_765
.Scanf_label_1175:
/* @450 */ 	jmp         .Scanf_label_949
.Scanf_label_451:
/* @452 */ 	jsr         __pushi7
/* @454 */ 	ldx         #__i12
/* @455 */ 	ldy          #0
/* @458 */ 	jsr         .Scanf_label_456
/* @459 */ 	bra         .Scanf_label_457
.Scanf_label_456:
/* @460 */ 	jmp         (__i9)
.Scanf_label_457:
/* @462 */ 	jsr         __incsp2
/* @465 */ 	lda         __i12
/* @466 */ 	sta         __b3
/* @470 */ 	sta         __i0
/* @471 */ 	and          #128
/* @473 */ 	beq         .Scanf_label_472
/* @474 */ 	lda          #255
.Scanf_label_472:
/* @475 */ 	sta         __i0+1
/* @478 */ 	lda         __i0
/* @479 */ 	cmp          #255
/* @480 */ 	bne         .Scanf_label_488
/* @481 */ 	lda         __i0+1
/* @482 */ 	cmp          #255
/* @483 */ 	bne         .Scanf_label_488
/* @485 */ 	lda          #1
/* @486 */ 	sta         __b2
/* @487 */ 	jmp         .Scanf_label_1088
.Scanf_label_488:
/* @491 */ 	lda         __b3
/* @492 */ 	sta         __i0
/* @493 */ 	and          #128
/* @495 */ 	beq         .Scanf_label_494
/* @496 */ 	lda          #255
.Scanf_label_494:
/* @497 */ 	sta         __i0+1
/* @500 */ 	lda         __i0
/* @501 */ 	cmp          #37
/* @502 */ 	bne         .Scanf_label_499
/* @503 */ 	lda         __i0+1
/* @505 */ 	beq         .Scanf_label_518
.Scanf_label_499:
/* @507 */ 	jsr         __pushi7
/* @508 */ 	lda         __b3
/* @510 */ 	jsr         __pusha
/* @513 */ 	jsr         .Scanf_label_511
/* @514 */ 	bra         .Scanf_label_512
.Scanf_label_511:
/* @515 */ 	jmp         (__i8)
.Scanf_label_512:
/* @516 */ 	jsr         __incsp4
/* @517 */ 	jmp         .Scanf_label_1088
.Scanf_label_518:
/* @519 */ 	jmp         .Scanf_label_951
.Scanf_label_520:
.Scanf_label_521:
/* @526 */ 	jsr         __pushi11
/* @527 */ 	jsr         __pushi6
/* @528 */ 	jsr         __pushi7
/* @531 */ 	lda         (__i10)
/* @537 */ 	sta         __i0
/* @538 */ 	and          #128
/* @540 */ 	beq         .Scanf_label_539
/* @541 */ 	lda          #255
.Scanf_label_539:
/* @542 */ 	sta         __i0+1
/* @548 */ 	ldx          #1
/* @549 */ 	lda         __i0
/* @550 */ 	cmp          #117
/* @551 */ 	bne         .Scanf_label_546
/* @552 */ 	lda         __i0+1
/* @554 */ 	beq         .Scanf_label_547
.Scanf_label_546:
/* @555 */ 	dex         
.Scanf_label_547:
/* @559 */ 	txa         
/* @560 */ 	jsr         __pusha
/* @562 */ 	ldx          #10
/* @564 */ 	jsr         __pushxy0
/* @565 */ 	jsr         __pushi8
/* @566 */ 	jsr         __pushi9
/* @568 */ 	ldx         #__b6
/* @569 */ 	ldy          #0
/* @570 */ 	jsr         ConvertDecimal
/* @572 */ 	jsr         __incsp14
/* @577 */ 	lda         __b6
/* @579 */ 	beq         .Scanf_label_576
/* @580 */ 	lda          #255
.Scanf_label_576:
/* @581 */ 	inc          A
/* @586 */ 	beq         .Scanf_label_590
/* @587 */ 	lda          #1
/* @588 */ 	sta         __b2
/* @589 */ 	jmp         .Scanf_label_1088
.Scanf_label_590:
/* @591 */ 	lda          #__i5
/* @592 */ 	jsr         __rinc21
/* @593 */ 	lda          #__i10
/* @594 */ 	jsr         __rinc21
/* @595 */ 	jmp         .Scanf_label_951
.Scanf_label_596:
/* @601 */ 	jsr         __pushi11
/* @602 */ 	jsr         __pushi6
/* @603 */ 	jsr         __pushi7
/* @604 */ 	lda          #0
/* @605 */ 	jsr         __pusha
/* @606 */ 	ldx          #0
/* @607 */ 	jsr         __pushxy0
/* @608 */ 	jsr         __pushi8
/* @609 */ 	jsr         __pushi9
/* @611 */ 	ldx         #__b6
/* @612 */ 	ldy          #0
/* @613 */ 	jsr         ConvertDecimal
/* @614 */ 	jsr         __incsp14
/* @619 */ 	lda         __b6
/* @621 */ 	beq         .Scanf_label_618
/* @622 */ 	lda          #255
.Scanf_label_618:
/* @623 */ 	inc          A
/* @628 */ 	beq         .Scanf_label_632
/* @629 */ 	lda          #1
/* @630 */ 	sta         __b2
/* @631 */ 	jmp         .Scanf_label_1088
.Scanf_label_632:
/* @633 */ 	lda          #__i5
/* @634 */ 	jsr         __rinc21
/* @635 */ 	lda          #__i10
/* @636 */ 	jsr         __rinc21
/* @637 */ 	jmp         .Scanf_label_951
.Scanf_label_638:
.Scanf_label_639:
/* @644 */ 	jsr         __pushi11
/* @645 */ 	jsr         __pushi6
/* @646 */ 	jsr         __pushi7
/* @647 */ 	lda          #1
/* @648 */ 	jsr         __pusha
/* @650 */ 	ldx          #8
/* @651 */ 	jsr         __pushxy0
/* @652 */ 	jsr         __pushi8
/* @653 */ 	jsr         __pushi9
/* @655 */ 	ldx         #__b6
/* @656 */ 	ldy          #0
/* @657 */ 	jsr         ConvertDecimal
/* @658 */ 	jsr         __incsp14
/* @663 */ 	lda         __b6
/* @665 */ 	beq         .Scanf_label_662
/* @666 */ 	lda          #255
.Scanf_label_662:
/* @667 */ 	inc          A
/* @672 */ 	beq         .Scanf_label_676
/* @673 */ 	lda          #1
/* @674 */ 	sta         __b2
/* @675 */ 	jmp         .Scanf_label_1088
.Scanf_label_676:
/* @677 */ 	lda          #__i5
/* @678 */ 	jsr         __rinc21
/* @679 */ 	lda          #__i10
/* @680 */ 	jsr         __rinc21
/* @681 */ 	jmp         .Scanf_label_951
.Scanf_label_682:
.Scanf_label_683:
/* @688 */ 	jsr         __pushi11
/* @689 */ 	jsr         __pushi6
/* @690 */ 	jsr         __pushi7
/* @691 */ 	lda          #1
/* @692 */ 	jsr         __pusha
/* @694 */ 	ldx          #16
/* @695 */ 	jsr         __pushxy0
/* @696 */ 	jsr         __pushi8
/* @697 */ 	jsr         __pushi9
/* @699 */ 	ldx         #__b6
/* @700 */ 	ldy          #0
/* @701 */ 	jsr         ConvertDecimal
/* @702 */ 	jsr         __incsp14
/* @707 */ 	lda         __b6
/* @709 */ 	beq         .Scanf_label_706
/* @710 */ 	lda          #255
.Scanf_label_706:
/* @711 */ 	inc          A
/* @716 */ 	beq         .Scanf_label_720
/* @717 */ 	lda          #1
/* @718 */ 	sta         __b2
/* @719 */ 	jmp         .Scanf_label_1088
.Scanf_label_720:
/* @721 */ 	lda          #__i5
/* @722 */ 	jsr         __rinc21
/* @723 */ 	lda          #__i10
/* @724 */ 	jsr         __rinc21
/* @725 */ 	jmp         .Scanf_label_951
.Scanf_label_726:
/* @731 */ 	jsr         __pushi11
/* @732 */ 	jsr         __pushi6
/* @733 */ 	jsr         __pushi7
/* @734 */ 	jsr         __pushi8
/* @735 */ 	jsr         __pushi9
/* @737 */ 	ldx         #__b6
/* @738 */ 	ldy          #0
/* @739 */ 	jsr         ReadString
/* @741 */ 	jsr         __incsp10
/* @746 */ 	lda         __b6
/* @748 */ 	beq         .Scanf_label_745
/* @749 */ 	lda          #255
.Scanf_label_745:
/* @750 */ 	inc          A
/* @755 */ 	beq         .Scanf_label_759
/* @756 */ 	lda          #1
/* @757 */ 	sta         __b2
/* @758 */ 	jmp         .Scanf_label_1088
.Scanf_label_759:
/* @760 */ 	lda          #__i5
/* @761 */ 	jsr         __rinc21
/* @762 */ 	lda          #__i10
/* @763 */ 	jsr         __rinc21
/* @764 */ 	jmp         .Scanf_label_951
.Scanf_label_765:
/* @770 */ 	jsr         __pushi11
/* @771 */ 	jsr         __pushi6
/* @772 */ 	jsr         __pushi7
/* @773 */ 	jsr         __pushi8
/* @774 */ 	jsr         __pushi9
/* @776 */ 	ldx         #__b6
/* @777 */ 	ldy          #0
/* @778 */ 	jsr         ReadChars
/* @779 */ 	jsr         __incsp10
/* @784 */ 	lda         __b6
/* @786 */ 	beq         .Scanf_label_783
/* @787 */ 	lda          #255
.Scanf_label_783:
/* @788 */ 	inc          A
/* @793 */ 	beq         .Scanf_label_797
/* @794 */ 	lda          #1
/* @795 */ 	sta         __b2
/* @796 */ 	jmp         .Scanf_label_1088
.Scanf_label_797:
/* @798 */ 	lda          #__i5
/* @799 */ 	jsr         __rinc21
/* @800 */ 	lda          #__i10
/* @801 */ 	jsr         __rinc21
/* @802 */ 	jmp         .Scanf_label_951
.Scanf_label_803:
/* @808 */ 	jsr         __pushi11
/* @809 */ 	jsr         __pushi6
/* @810 */ 	jsr         __pushi7
/* @811 */ 	jsr         __pushi8
/* @812 */ 	jsr         __pushi9
/* @814 */ 	ldx         #__b6
/* @815 */ 	ldy          #0
/* @816 */ 	jsr         ReadPointer
/* @817 */ 	jsr         __incsp10
/* @822 */ 	lda         __b6
/* @824 */ 	beq         .Scanf_label_821
/* @825 */ 	lda          #255
.Scanf_label_821:
/* @826 */ 	inc          A
/* @831 */ 	beq         .Scanf_label_835
/* @832 */ 	lda          #1
/* @833 */ 	sta         __b2
/* @834 */ 	jmp         .Scanf_label_1088
.Scanf_label_835:
/* @836 */ 	lda          #__i5
/* @837 */ 	jsr         __rinc21
/* @838 */ 	lda          #__i10
/* @839 */ 	jsr         __rinc21
/* @840 */ 	jmp         .Scanf_label_951
.Scanf_label_841:
/* @842 */ 	lda         __i5
/* @843 */ 	sta         (__i6)
/* @844 */ 	lda         __i5+1
/* @845 */ 	ldy          #1
/* @846 */ 	sta         (__i6), Y
/* @847 */ 	lda          #__i10
/* @848 */ 	jsr         __rinc21
/* @849 */ 	jmp         .Scanf_label_951
.Scanf_label_850:
/* @851 */ 	lda          #__i10
/* @852 */ 	jsr         __rinc21
/* @854 */ 	ldx          #41
	jsr          __var_addr_i12			// set
/* @857 */ 	jsr         __pushi12
/* @858 */ 	jsr         __pushi10
/* @859 */ 	ldx         #__i10
/* @860 */ 	ldy          #0
/* @861 */ 	jsr         ParseScanSet
/* @862 */ 	jsr         __incsp4
/* @867 */ 	jsr         __pushi11
/* @872 */ 	jsr         __pushi12
/* @873 */ 	jsr         __pushi6
/* @874 */ 	jsr         __pushi7
/* @875 */ 	jsr         __pushi8
/* @876 */ 	jsr         __pushi9
/* @878 */ 	ldx         #__b6
/* @879 */ 	ldy          #0
/* @880 */ 	jsr         ReadScanSet
/* @882 */ 	jsr         __incsp12
/* @887 */ 	lda         __b6
/* @889 */ 	beq         .Scanf_label_886
/* @890 */ 	lda          #255
.Scanf_label_886:
/* @891 */ 	inc          A
/* @896 */ 	beq         .Scanf_label_900
/* @897 */ 	lda          #1
/* @898 */ 	sta         __b2
/* @899 */ 	jmp         .Scanf_label_1088
.Scanf_label_900:
/* @901 */ 	lda          #__i5
/* @902 */ 	jsr         __rinc21
/* @903 */ 	bra         .Scanf_label_951
.Scanf_label_904:
.Scanf_label_905:
.Scanf_label_906:
.Scanf_label_907:
.Scanf_label_908:
.Scanf_label_909:
.Scanf_label_910:
.Scanf_label_911:
/* @916 */ 	jsr         __pushi11
/* @917 */ 	jsr         __pushi6
/* @918 */ 	jsr         __pushi7
/* @919 */ 	jsr         __pushi8
/* @920 */ 	jsr         __pushi9
/* @922 */ 	ldx         #__b6
/* @923 */ 	ldy          #0
/* @924 */ 	jsr         ReadDouble
/* @925 */ 	jsr         __incsp10
/* @930 */ 	lda         __b6
/* @932 */ 	beq         .Scanf_label_929
/* @933 */ 	lda          #255
.Scanf_label_929:
/* @934 */ 	inc          A
/* @939 */ 	beq         .Scanf_label_943
/* @940 */ 	lda          #1
/* @941 */ 	sta         __b2
/* @942 */ 	jmp         .Scanf_label_1088
.Scanf_label_943:
/* @944 */ 	lda          #__i5
/* @945 */ 	jsr         __rinc21
/* @946 */ 	lda          #__i10
/* @947 */ 	jsr         __rinc21
/* @948 */ 	bra         .Scanf_label_951
.Scanf_label_949:
.Scanf_label_951:
/* @952 */ 	jmp         .Scanf_label_1085
.Scanf_label_953:
/* @956 */ 	lda         (__i10)
/* @962 */ 	sta         __i0
/* @963 */ 	and          #128
/* @965 */ 	beq         .Scanf_label_964
/* @966 */ 	lda          #255
.Scanf_label_964:
/* @967 */ 	sta         __i0+1
/* @970 */ 	ldx         __i0
/* @971 */ 	ldy         __i0+1
/* @973 */ 	jsr         __builtin_isspace
/* @975 */ 	stz         __i1+1
/* @979 */ 	cmp          #0
/* @980 */ 	beq         .Scanf_label_1018
.Scanf_label_981:
/* @984 */ 	lda         (__i10)
/* @990 */ 	sta         __i0
/* @991 */ 	and          #128
/* @993 */ 	beq         .Scanf_label_992
/* @994 */ 	lda          #255
.Scanf_label_992:
/* @995 */ 	sta         __i0+1
/* @998 */ 	ldx         __i0
/* @999 */ 	ldy         __i0+1
/* @1000 */ 	jsr         __builtin_isspace
/* @1002 */ 	stz         __i1+1
/* @1006 */ 	cmp          #0
/* @1007 */ 	beq         .Scanf_label_1011
/* @1008 */ 	lda          #__i10
/* @1009 */ 	jsr         __rinc21
/* @1010 */ 	bra         .Scanf_label_981
.Scanf_label_1011:
/* @1012 */ 	jsr         __pushi7
/* @1013 */ 	jsr         __pushi8
/* @1014 */ 	jsr         __pushi9
/* @1015 */ 	jsr         SkipInputWhiteSpace
/* @1016 */ 	jsr         __incsp6
/* @1017 */ 	bra         .Scanf_label_1084
.Scanf_label_1018:
/* @1019 */ 	jsr         __pushi7
/* @1020 */ 	ldx         #__i4
/* @1021 */ 	ldy          #0
/* @1024 */ 	jsr         .Scanf_label_1022
/* @1025 */ 	bra         .Scanf_label_1023
.Scanf_label_1022:
/* @1026 */ 	jmp         (__i9)
.Scanf_label_1023:
/* @1027 */ 	jsr         __incsp2
/* @1029 */ 	lda         __i4
/* @1030 */ 	cmp          #255
/* @1031 */ 	bne         .Scanf_label_1039
/* @1032 */ 	lda         __i4+1
/* @1033 */ 	cmp          #255
/* @1034 */ 	bne         .Scanf_label_1039
/* @1036 */ 	lda          #1
/* @1037 */ 	sta         __b2
/* @1038 */ 	bra         .Scanf_label_1087
.Scanf_label_1039:
/* @1042 */ 	lda         (__i10)
/* @1048 */ 	sta         __i0
/* @1049 */ 	and          #128
/* @1051 */ 	beq         .Scanf_label_1050
/* @1052 */ 	lda          #255
.Scanf_label_1050:
/* @1053 */ 	sta         __i0+1
/* @1056 */ 	lda         __i4
/* @1057 */ 	cmp         __i0
/* @1058 */ 	bne         .Scanf_label_1055
/* @1059 */ 	lda         __i4+1
/* @1060 */ 	cmp         __i0+1
/* @1061 */ 	beq         .Scanf_label_1081
.Scanf_label_1055:
/* @1063 */ 	jsr         __pushi7
/* @1066 */ 	lda         __i4+1
/* @1067 */ 	and          #128
/* @1068 */ 	ora         __i4
/* @1073 */ 	jsr         __pusha
/* @1076 */ 	jsr         .Scanf_label_1074
/* @1077 */ 	bra         .Scanf_label_1075
.Scanf_label_1074:
/* @1078 */ 	jmp         (__i8)
.Scanf_label_1075:
/* @1079 */ 	jsr         __incsp4
/* @1080 */ 	bra         .Scanf_label_1088
.Scanf_label_1081:
/* @1082 */ 	lda          #__i10
/* @1083 */ 	jsr         __rinc21
.Scanf_label_1084:
.Scanf_label_1085:
/* @1086 */ 	jmp         .Scanf_label_98
.Scanf_label_1087:
.Scanf_label_1088:
/* @1092 */ 	lda         __b2
/* @1093 */ 	sta         __b0
/* @1097 */ 	beq         .Scanf_label_1108
/* @1101 */ 	ldx          #1
/* @1102 */ 	lda         __i5
/* @1103 */ 	ora         __i5+1
/* @1105 */ 	beq         .Scanf_label_1100
/* @1106 */ 	dex         
.Scanf_label_1100:
/* @1107 */ 	stx         __b0
.Scanf_label_1108:
/* @1110 */ 	lda         __b0
/* @1112 */ 	beq         .Scanf_label_1126
/* @1115 */ 	lda          #255
/* @1116 */ 	sta         __i0
/* @1118 */ 	sta         __i0+1
/* @1119 */ 	ldx          #46
	jsr          __load_result
/* @1120 */ 	lda         #__i0
/* @1122 */ 	jsr         __result2
.Scanf_label_1123:
/* @1124 */ 	ldy          #48
	jmp          __leave
.Scanf_label_1126:
/* @1127 */ 	ldx          #46
	jsr          __load_result
/* @1128 */ 	lda         #__i5
/* @1129 */ 	jsr         __result2
/* @1130 */ 	bra         .Scanf_label_1123
.func_end_Scanf:
	.size Scanf, .func_end_Scanf-Scanf

	.local  FILEGet
	.type FILEGet, @function

FILEGet:
/* @3 */ 	stx         __result
/* @5 */ 	sty         __result+1
/* @6 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @14 */ 	ldx          #0
	jsr          __arg_value2_i1			// data
/* @16 */ 	lda         __i1
/* @17 */ 	sta         __i0
/* @19 */ 	lda         __i1+1
/* @20 */ 	sta         __i0+1
/* @21 */ 	jsr         __pushi0
/* @23 */ 	ldx         #__i4
/* @24 */ 	ldy          #0
/* @25 */ 	jsr         fgetc
/* @27 */ 	jsr         __incsp2
/* @29 */ 	ldx          #8
	jsr          __load_result
/* @30 */ 	lda         #__i4
/* @32 */ 	jsr         __result2
/* @34 */ 	ldy          #10
	jmp          __leave
.func_end_FILEGet:
	.size FILEGet, .func_end_FILEGet-FILEGet

	.local  FILEUnget
	.type FILEUnget, @function

FILEUnget:
/* @4 */ 	ldx          #7
	jsr          __enter_nomask
/* @12 */ 	ldx          #2
	jsr          __arg_value2_i1			// data
/* @16 */ 	ldx          #0
	jsr          __arg_value1_b0			// ch
/* @18 */ 	lda         __i1
/* @19 */ 	sta         __i0
/* @21 */ 	lda         __i1+1
/* @22 */ 	sta         __i0+1
/* @23 */ 	jsr         __pushi0
/* @26 */ 	lda         __b0
/* @27 */ 	sta         __i2
/* @29 */ 	and          #128
/* @31 */ 	beq         .FILEUnget_label_30
/* @33 */ 	lda          #255
.FILEUnget_label_30:
/* @34 */ 	sta         __i2+1
/* @37 */ 	jsr         __pushi2
/* @39 */ 	ldx         #__i2
/* @40 */ 	ldy          #0
/* @41 */ 	jsr         ungetc
/* @43 */ 	jsr         __incsp4
/* @44 */ 	ldy          #10
	jmp          __leave_void_nomask
.func_end_FILEUnget:
	.size FILEUnget, .func_end_FILEUnget-FILEUnget

	.local  StringGet
	.type StringGet, @function

StringGet:
/* @5 */ 	stx         __result
/* @7 */ 	sty         __result+1
/* @8 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @15 */ 	ldx          #0
	jsr          __arg_value2_i1			// data
/* @17 */ 	lda         __i1
/* @18 */ 	sta         __i0
/* @20 */ 	lda         __i1+1
/* @21 */ 	sta         __i0+1
/* @24 */ 	lda         __i0
/* @25 */ 	sta         __i2
/* @26 */ 	lda         __i0+1
/* @27 */ 	sta         __i2+1
/* @31 */ 	clc         
/* @32 */ 	lda         __i2
/* @33 */ 	adc          #0
/* @34 */ 	sta         __i3
/* @35 */ 	lda         __i2+1
/* @36 */ 	adc          #0
/* @37 */ 	sta         __i3+1
/* @42 */ 	lda         (__i2)
/* @43 */ 	sta         __i4
/* @44 */ 	ldy          #1
/* @45 */ 	lda         (__i2), Y
/* @46 */ 	sta         __i4+1
/* @48 */ 	lda          #__i3
/* @50 */ 	jsr         __inc21
/* @55 */ 	lda         (__i4)
/* @61 */ 	sta         __i2
/* @63 */ 	and          #128
/* @65 */ 	beq         .StringGet_label_64
/* @67 */ 	lda          #255
.StringGet_label_64:
/* @68 */ 	sta         __i2+1
/* @70 */ 	lda         #__i2
/* @72 */ 	jsr         __result2
/* @74 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_StringGet:
	.size StringGet, .func_end_StringGet-StringGet

	.local  StringUnget
	.type StringUnget, @function

StringUnget:
/* @3 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @10 */ 	ldx          #2
	jsr          __arg_value2_i1			// data
/* @12 */ 	lda         __i1
/* @13 */ 	sta         __i0
/* @15 */ 	lda         __i1+1
/* @16 */ 	sta         __i0+1
/* @19 */ 	clc         
/* @20 */ 	lda         __i0
/* @21 */ 	adc          #0
/* @22 */ 	sta         __i2
/* @23 */ 	lda         __i0+1
/* @24 */ 	adc          #0
/* @25 */ 	sta         __i2+1
/* @27 */ 	lda          #__i2
/* @29 */ 	jsr         __dec21
/* @30 */ 	ldy          #8
	jmp          __leave_leaf_void_nomask
.func_end_StringUnget:
	.size StringUnget, .func_end_StringUnget-StringUnget

	.global scanf
	.type scanf, @function

scanf:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #9
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @22 */ 	ldx          #5
	jsr          __var_addr_i0			// ap
/* @24 */ 	ldx          #0
	jsr          __arg_addr_i1			// format
/* @28 */ 	clc         
/* @30 */ 	ldy          #0
/* @31 */ 	lda         __i1
/* @32 */ 	adc          #2
/* @33 */ 	sta         (__i0)
/* @35 */ 	iny         
/* @36 */ 	lda         __i1+1
/* @37 */ 	adc          #0
/* @38 */ 	sta         (__i0), Y
/* @40 */ 	ldx          #5
	jsr          __var_value2_i0			// ap
/* @43 */ 	jsr         __pushi0
/* @45 */ 	ldx          #0
	jsr          __arg_value2_i0			// format
/* @48 */ 	jsr         __pushi0
/* @51 */ 	lda         stdin+0
/* @52 */ 	sta         __i0
/* @53 */ 	lda         stdin+1
/* @54 */ 	sta         __i0+1
/* @56 */ 	jsr         __pushi0
/* @59 */ 	lda         #%lo(FILEUnget)
/* @60 */ 	sta         __i0
/* @61 */ 	lda         #%hi(FILEUnget)
/* @62 */ 	sta         __i0+1
/* @64 */ 	jsr         __pushi0
/* @67 */ 	lda         #%lo(FILEGet)
/* @68 */ 	sta         __i0
/* @69 */ 	lda         #%hi(FILEGet)
/* @70 */ 	sta         __i0+1
/* @72 */ 	jsr         __pushi0
/* @73 */ 	ldx         #__i4
/* @74 */ 	ldy          #0
/* @75 */ 	jsr         Scanf
/* @77 */ 	jsr         __incsp10
/* @78 */ 	ldx          #10
	jsr          __load_result
/* @79 */ 	lda         #__i4
/* @81 */ 	jsr         __result2
/* @83 */ 	ldy          #12
	jmp          __leave
.func_end_scanf:
	.size scanf, .func_end_scanf-scanf

	.global vscanf
	.type vscanf, @function

vscanf:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @18 */ 	ldx          #2
	jsr          __arg_value2_i0			// arg
/* @22 */ 	ldx          #0
	jsr          __arg_value2_i1			// format
/* @23 */ 	jsr         __pushi0
/* @24 */ 	jsr         __pushi1
/* @28 */ 	lda         stdin+0
/* @29 */ 	sta         __i2
/* @31 */ 	lda         stdin+1
/* @32 */ 	sta         __i2+1
/* @34 */ 	jsr         __pushi2
/* @37 */ 	lda         #%lo(FILEUnget)
/* @38 */ 	sta         __i2
/* @39 */ 	lda         #%hi(FILEUnget)
/* @40 */ 	sta         __i2+1
/* @42 */ 	jsr         __pushi2
/* @45 */ 	lda         #%lo(FILEGet)
/* @46 */ 	sta         __i2
/* @47 */ 	lda         #%hi(FILEGet)
/* @48 */ 	sta         __i2+1
/* @50 */ 	jsr         __pushi2
/* @52 */ 	ldx         #__i4
/* @53 */ 	ldy          #0
/* @54 */ 	jsr         Scanf
/* @56 */ 	jsr         __incsp10
/* @58 */ 	ldx          #8
	jsr          __load_result
/* @59 */ 	lda         #__i4
/* @61 */ 	jsr         __result2
/* @63 */ 	ldy          #10
	jmp          __leave
.func_end_vscanf:
	.size vscanf, .func_end_vscanf-vscanf

	.global fscanf
	.type fscanf, @function

fscanf:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #9
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @23 */ 	ldx          #0
	jsr          __arg_value2_i0			// stream
/* @25 */ 	ldx          #5
	jsr          __var_addr_i1			// ap
/* @27 */ 	ldx          #2
	jsr          __arg_addr_i2			// format
/* @31 */ 	clc         
/* @33 */ 	ldy          #0
/* @34 */ 	lda         __i2
/* @35 */ 	adc          #2
/* @36 */ 	sta         (__i1)
/* @38 */ 	iny         
/* @39 */ 	lda         __i2+1
/* @40 */ 	adc          #0
/* @41 */ 	sta         (__i1), Y
/* @43 */ 	ldx          #5
	jsr          __var_value2_i1			// ap
/* @46 */ 	jsr         __pushi1
/* @48 */ 	ldx          #2
	jsr          __arg_value2_i1			// format
/* @51 */ 	jsr         __pushi1
/* @52 */ 	jsr         __pushi0
/* @55 */ 	lda         #%lo(FILEUnget)
/* @56 */ 	sta         __i1
/* @57 */ 	lda         #%hi(FILEUnget)
/* @58 */ 	sta         __i1+1
/* @60 */ 	jsr         __pushi1
/* @63 */ 	lda         #%lo(FILEGet)
/* @64 */ 	sta         __i1
/* @65 */ 	lda         #%hi(FILEGet)
/* @66 */ 	sta         __i1+1
/* @68 */ 	jsr         __pushi1
/* @69 */ 	ldx         #__i4
/* @70 */ 	ldy          #0
/* @71 */ 	jsr         Scanf
/* @73 */ 	jsr         __incsp10
/* @74 */ 	ldx          #10
	jsr          __load_result
/* @75 */ 	lda         #__i4
/* @77 */ 	jsr         __result2
/* @79 */ 	ldy          #12
	jmp          __leave
.func_end_fscanf:
	.size fscanf, .func_end_fscanf-fscanf

	.global vfscanf
	.type vfscanf, @function

vfscanf:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @17 */ 	ldx          #4
	jsr          __arg_value2_i0			// arg
/* @21 */ 	ldx          #2
	jsr          __arg_value2_i1			// format
/* @25 */ 	ldx          #0
	jsr          __arg_value2_i2			// stream
/* @26 */ 	jsr         __pushi0
/* @27 */ 	jsr         __pushi1
/* @28 */ 	jsr         __pushi2
/* @32 */ 	lda         #%lo(FILEUnget)
/* @33 */ 	sta         __i3
/* @35 */ 	lda         #%hi(FILEUnget)
/* @36 */ 	sta         __i3+1
/* @38 */ 	jsr         __pushi3
/* @41 */ 	lda         #%lo(FILEGet)
/* @42 */ 	sta         __i3
/* @43 */ 	lda         #%hi(FILEGet)
/* @44 */ 	sta         __i3+1
/* @46 */ 	jsr         __pushi3
/* @48 */ 	ldx         #__i4
/* @49 */ 	ldy          #0
/* @50 */ 	jsr         Scanf
/* @52 */ 	jsr         __incsp10
/* @54 */ 	ldx          #8
	jsr          __load_result
/* @55 */ 	lda         #__i4
/* @57 */ 	jsr         __result2
/* @59 */ 	ldy          #10
	jmp          __leave
.func_end_vfscanf:
	.size vfscanf, .func_end_vfscanf-vfscanf

	.global sscanf
	.type sscanf, @function

sscanf:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #11
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @23 */ 	ldx          #0
	jsr          __arg_value2_i0			// s
/* @27 */ 	ldx          #5
	jsr          __var_addr_i1			// ap
/* @29 */ 	ldx          #2
	jsr          __arg_addr_i2			// format
/* @33 */ 	clc         
/* @35 */ 	ldy          #0
/* @36 */ 	lda         __i2
/* @37 */ 	adc          #2
/* @38 */ 	sta         (__i1)
/* @40 */ 	iny         
/* @41 */ 	lda         __i2+1
/* @42 */ 	adc          #0
/* @43 */ 	sta         (__i1), Y
/* @44 */ 	lda          #2
/* @46 */ 	sta         __mem_size
/* @48 */ 	ldx          #7
	jsr          __var_addr_i1			// data
/* @50 */ 	lda         __i1
/* @52 */ 	sta         __mem_dest
/* @53 */ 	lda         __i1+1
/* @55 */ 	sta         __mem_dest+1
/* @57 */ 	jsr         __zeromem1
/* @58 */ 	lda          #__i0
/* @60 */ 	ldx          #7
/* @62 */ 	jsr         __set_var_value2
/* @64 */ 	ldx          #5
	jsr          __var_value2_i2			// ap
/* @67 */ 	jsr         __pushi2
/* @69 */ 	ldx          #2
	jsr          __arg_value2_i2			// format
/* @72 */ 	jsr         __pushi2
/* @77 */ 	jsr         __pushi1
/* @80 */ 	lda         #%lo(StringUnget)
/* @81 */ 	sta         __i1
/* @82 */ 	lda         #%hi(StringUnget)
/* @83 */ 	sta         __i1+1
/* @85 */ 	jsr         __pushi1
/* @88 */ 	lda         #%lo(StringGet)
/* @89 */ 	sta         __i1
/* @90 */ 	lda         #%hi(StringGet)
/* @91 */ 	sta         __i1+1
/* @93 */ 	jsr         __pushi1
/* @94 */ 	ldx         #__i4
/* @95 */ 	ldy          #0
/* @96 */ 	jsr         Scanf
/* @98 */ 	jsr         __incsp10
/* @99 */ 	ldx          #12
	jsr          __load_result
/* @100 */ 	lda         #__i4
/* @102 */ 	jsr         __result2
/* @104 */ 	ldy          #14
	jmp          __leave
.func_end_sscanf:
	.size sscanf, .func_end_sscanf-sscanf

	.global vsscanf
	.type vsscanf, @function

vsscanf:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #9
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @19 */ 	ldx          #0
	jsr          __arg_value2_i0			// s
/* @23 */ 	ldx          #4
	jsr          __arg_value2_i1			// arg
/* @27 */ 	ldx          #2
	jsr          __arg_value2_i2			// format
/* @29 */ 	lda          #2
/* @31 */ 	sta         __mem_size
/* @33 */ 	ldx          #5
	jsr          __var_addr_i3			// data
/* @36 */ 	lda         __i3
/* @38 */ 	sta         __mem_dest
/* @40 */ 	lda         __i3+1
/* @42 */ 	sta         __mem_dest+1
/* @44 */ 	jsr         __zeromem1
/* @45 */ 	lda          #__i0
/* @47 */ 	ldx          #5
/* @49 */ 	jsr         __set_var_value2
/* @50 */ 	jsr         __pushi1
/* @51 */ 	jsr         __pushi2
/* @56 */ 	jsr         __pushi3
/* @59 */ 	lda         #%lo(StringUnget)
/* @60 */ 	sta         __i3
/* @61 */ 	lda         #%hi(StringUnget)
/* @62 */ 	sta         __i3+1
/* @64 */ 	jsr         __pushi3
/* @67 */ 	lda         #%lo(StringGet)
/* @68 */ 	sta         __i3
/* @69 */ 	lda         #%hi(StringGet)
/* @70 */ 	sta         __i3+1
/* @72 */ 	jsr         __pushi3
/* @74 */ 	ldx         #__i4
/* @75 */ 	ldy          #0
/* @76 */ 	jsr         Scanf
/* @78 */ 	jsr         __incsp10
/* @80 */ 	ldx          #10
	jsr          __load_result
/* @81 */ 	lda         #__i4
/* @83 */ 	jsr         __result2
/* @85 */ 	ldy          #12
	jmp          __leave
.func_end_vsscanf:
	.size vsscanf, .func_end_vsscanf-vsscanf

	.data
	.section ".rodata", "aMS", @progbits
.lit.36:
	.byte 0x30
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.type .lit.36, @object
	.size .lit.36, 8

.lit.37:
	.byte 0x0a
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.type .lit.37, @object
	.size .lit.37, 8

.str.46:
	.asciz "(null)"
	.type .str.46, @object
	.size .str.46, 7

