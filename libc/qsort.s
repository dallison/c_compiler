	.file   "/Users/dallison/Google Drive/c_compiler/libc/qsort.c"
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


	.local  Swap
	.type Swap, @function

Swap:
/* @4 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @11 */ 	ldx          #0
	jsr          __arg_value2_i1			// p
/* @17 */ 	ldx          #2
	jsr          __arg_value2_i3			// q
/* @23 */ 	ldx          #4
	jsr          __arg_value2_i5			// size
/* @27 */ 	lda         __i1
/* @28 */ 	sta         __i0
/* @30 */ 	lda         __i1+1
/* @31 */ 	sta         __i0+1
/* @32 */ 	lda         __i3
/* @33 */ 	sta         __i2
/* @34 */ 	lda         __i3+1
/* @35 */ 	sta         __i2+1
/* @36 */ 	stz         __i4
/* @37 */ 	stz         __i4+1
.Swap_label_38:
/* @40 */ 	lda         __i4+1
/* @41 */ 	cmp         __i5+1
/* @42 */ 	bcc         .Swap_label_39
/* @43 */ 	bne         .Swap_label_75
/* @44 */ 	lda         __i4
/* @45 */ 	cmp         __i5
/* @46 */ 	bcs         .Swap_label_75
.Swap_label_39:
/* @50 */ 	lda         (__i0)
/* @55 */ 	sta         __b0
/* @58 */ 	lda         (__i2)
/* @63 */ 	sta         (__i0)
/* @64 */ 	lda         __b0
/* @65 */ 	sta         (__i2)
/* @66 */ 	lda          #__i0
/* @68 */ 	jsr         __rinc21
/* @69 */ 	lda          #__i2
/* @70 */ 	jsr         __rinc21
/* @72 */ 	lda          #__i4
/* @73 */ 	jsr         __rinc21
/* @74 */ 	bra         .Swap_label_38
.Swap_label_75:
/* @76 */ 	ldy          #8
	jmp          __leave_leaf_void
.func_end_Swap:
	.size Swap, .func_end_Swap-Swap

	.local  Quicksort
	.type Quicksort, @function

Quicksort:
/* @6 */ 	ldx          #7
	jsr          __enter
	.byte        0x09,0x00,0x00		// Save mask i:9 b:0 l:0 x:0 f:0 
/* @12 */ 	ldx          #0
	jsr          __arg_value2_i4			// base
/* @16 */ 	ldx          #6
	jsr          __arg_value2_i5			// last
/* @23 */ 	ldx          #2
	jsr          __arg_value2_i7			// size
/* @31 */ 	ldx          #8
	jsr          __arg_value2_i10			// compar
/* @35 */ 	ldx          #4
	jsr          __arg_value2_i11			// first
/* @38 */ 	lda         __i11
/* @39 */ 	cmp         __i5
/* @41 */ 	lda         __i11+1
/* @42 */ 	sbc         __i5+1
/* @43 */ 	bvc         .Quicksort_label_36
/* @45 */ 	eor          #128
.Quicksort_label_36:
/* @46 */ 	bmi         .Quicksort_label_351
/* @352 */ 	jmp         .Quicksort_label_348
.Quicksort_label_351:
/* @49 */ 	clc         
/* @50 */ 	lda         __i11
/* @51 */ 	adc         __i5
/* @52 */ 	sta         __i0
/* @53 */ 	lda         __i11+1
/* @54 */ 	adc         __i5+1
/* @55 */ 	sta         __i0+1
/* @60 */ 	lsr          A
/* @61 */ 	sta         __i1+1
/* @62 */ 	lda         __i0
/* @63 */ 	ror          A
/* @64 */ 	sta         __i1
/* @68 */ 	sta         __i6
/* @69 */ 	lda         __i1+1
/* @70 */ 	sta         __i6+1
/* @73 */ 	lda          #__i0
/* @74 */ 	ldx          #__i6
/* @75 */ 	ldy          #__i7
/* @77 */ 	jsr         __umod2
/* @81 */ 	sec         
/* @82 */ 	lda         __i6
/* @83 */ 	sbc         __i0
/* @84 */ 	sta         __i1
/* @85 */ 	lda         __i6+1
/* @86 */ 	sbc         __i0+1
/* @87 */ 	sta         __i1+1
/* @90 */ 	lda         __i1
/* @91 */ 	sta         __i6
/* @92 */ 	lda         __i1+1
/* @93 */ 	sta         __i6+1
/* @94 */ 	lda         __i11
/* @95 */ 	sta         __i8
/* @96 */ 	lda         __i11+1
/* @97 */ 	sta         __i8+1
/* @98 */ 	lda         __i5
/* @99 */ 	sta         __i9
/* @100 */ 	lda         __i5+1
/* @101 */ 	sta         __i9+1
.Quicksort_label_102:
.Quicksort_label_103:
/* @106 */ 	clc         
/* @107 */ 	lda         __i4
/* @108 */ 	adc         __i6
/* @109 */ 	sta         __i0
/* @110 */ 	lda         __i4+1
/* @111 */ 	adc         __i6+1
/* @112 */ 	sta         __i0+1
/* @115 */ 	jsr         __pushi0
/* @118 */ 	clc         
/* @119 */ 	lda         __i4
/* @120 */ 	adc         __i8
/* @121 */ 	sta         __i0
/* @122 */ 	lda         __i4+1
/* @123 */ 	adc         __i8+1
/* @124 */ 	sta         __i0+1
/* @127 */ 	jsr         __pushi0
/* @129 */ 	ldx         #__i12
/* @130 */ 	ldy          #0
/* @133 */ 	jsr         .Quicksort_label_131
/* @134 */ 	bra         .Quicksort_label_132
.Quicksort_label_131:
/* @135 */ 	jmp         (__i10)
.Quicksort_label_132:
/* @137 */ 	jsr         __incsp4
/* @140 */ 	lda         __i12+1
/* @141 */ 	bpl         .Quicksort_label_158
/* @144 */ 	clc         
/* @145 */ 	lda         __i8
/* @146 */ 	adc         __i7
/* @147 */ 	sta         __i0
/* @148 */ 	lda         __i8+1
/* @149 */ 	adc         __i7+1
/* @150 */ 	sta         __i0+1
/* @153 */ 	lda         __i0
/* @154 */ 	sta         __i8
/* @155 */ 	lda         __i0+1
/* @156 */ 	sta         __i8+1
/* @157 */ 	bra         .Quicksort_label_103
.Quicksort_label_158:
.Quicksort_label_159:
/* @162 */ 	clc         
/* @163 */ 	lda         __i4
/* @164 */ 	adc         __i6
/* @165 */ 	sta         __i0
/* @166 */ 	lda         __i4+1
/* @167 */ 	adc         __i6+1
/* @168 */ 	sta         __i0+1
/* @171 */ 	jsr         __pushi0
/* @174 */ 	clc         
/* @175 */ 	lda         __i4
/* @176 */ 	adc         __i9
/* @177 */ 	sta         __i0
/* @178 */ 	lda         __i4+1
/* @179 */ 	adc         __i9+1
/* @180 */ 	sta         __i0+1
/* @183 */ 	jsr         __pushi0
/* @185 */ 	ldx         #__i12
/* @186 */ 	ldy          #0
/* @189 */ 	jsr         .Quicksort_label_187
/* @190 */ 	bra         .Quicksort_label_188
.Quicksort_label_187:
/* @191 */ 	jmp         (__i10)
.Quicksort_label_188:
/* @192 */ 	jsr         __incsp4
/* @195 */ 	lda          #0
/* @196 */ 	cmp         __i12
/* @198 */ 	sbc         __i12+1
/* @199 */ 	bvc         .Quicksort_label_194
/* @200 */ 	eor          #128
.Quicksort_label_194:
/* @201 */ 	bpl         .Quicksort_label_218
/* @204 */ 	sec         
/* @205 */ 	lda         __i9
/* @206 */ 	sbc         __i7
/* @207 */ 	sta         __i0
/* @208 */ 	lda         __i9+1
/* @209 */ 	sbc         __i7+1
/* @210 */ 	sta         __i0+1
/* @213 */ 	lda         __i0
/* @214 */ 	sta         __i9
/* @215 */ 	lda         __i0+1
/* @216 */ 	sta         __i9+1
/* @217 */ 	bra         .Quicksort_label_159
.Quicksort_label_218:
/* @220 */ 	lda         __i9
/* @221 */ 	cmp         __i8
/* @222 */ 	lda         __i9+1
/* @223 */ 	sbc         __i8+1
/* @224 */ 	bvc         .Quicksort_label_219
/* @225 */ 	eor          #128
.Quicksort_label_219:
/* @226 */ 	bpl         .Quicksort_label_353
/* @354 */ 	jmp         .Quicksort_label_322
.Quicksort_label_353:
/* @228 */ 	lda         __i8
/* @229 */ 	cmp         __i9
/* @230 */ 	bne         .Quicksort_label_227
/* @231 */ 	lda         __i8+1
/* @232 */ 	cmp         __i9+1
/* @233 */ 	beq         .Quicksort_label_291
.Quicksort_label_227:
/* @236 */ 	lda         __i6
/* @237 */ 	cmp         __i8
/* @238 */ 	bne         .Quicksort_label_248
/* @239 */ 	lda         __i6+1
/* @240 */ 	cmp         __i8+1
/* @241 */ 	bne         .Quicksort_label_248
/* @243 */ 	lda         __i9
/* @244 */ 	sta         __i6
/* @245 */ 	lda         __i9+1
/* @246 */ 	sta         __i6+1
/* @247 */ 	bra         .Quicksort_label_262
.Quicksort_label_248:
/* @250 */ 	lda         __i6
/* @251 */ 	cmp         __i9
/* @252 */ 	bne         .Quicksort_label_261
/* @253 */ 	lda         __i6+1
/* @254 */ 	cmp         __i9+1
/* @255 */ 	bne         .Quicksort_label_261
/* @257 */ 	lda         __i8
/* @258 */ 	sta         __i6
/* @259 */ 	lda         __i8+1
/* @260 */ 	sta         __i6+1
.Quicksort_label_261:
.Quicksort_label_262:
/* @263 */ 	jsr         __pushi7
/* @266 */ 	clc         
/* @267 */ 	lda         __i4
/* @268 */ 	adc         __i9
/* @269 */ 	sta         __i0
/* @270 */ 	lda         __i4+1
/* @271 */ 	adc         __i9+1
/* @272 */ 	sta         __i0+1
/* @275 */ 	jsr         __pushi0
/* @278 */ 	clc         
/* @279 */ 	lda         __i4
/* @280 */ 	adc         __i8
/* @281 */ 	sta         __i0
/* @282 */ 	lda         __i4+1
/* @283 */ 	adc         __i8+1
/* @284 */ 	sta         __i0+1
/* @287 */ 	jsr         __pushi0
/* @288 */ 	jsr         Swap
/* @290 */ 	jsr         __incsp6
.Quicksort_label_291:
/* @294 */ 	clc         
/* @295 */ 	lda         __i8
/* @296 */ 	adc         __i7
/* @297 */ 	sta         __i0
/* @298 */ 	lda         __i8+1
/* @299 */ 	adc         __i7+1
/* @300 */ 	sta         __i0+1
/* @303 */ 	lda         __i0
/* @304 */ 	sta         __i8
/* @305 */ 	lda         __i0+1
/* @306 */ 	sta         __i8+1
/* @309 */ 	sec         
/* @310 */ 	lda         __i9
/* @311 */ 	sbc         __i7
/* @312 */ 	sta         __i0
/* @313 */ 	lda         __i9+1
/* @314 */ 	sbc         __i7+1
/* @315 */ 	sta         __i0+1
/* @318 */ 	lda         __i0
/* @319 */ 	sta         __i9
/* @320 */ 	lda         __i0+1
/* @321 */ 	sta         __i9+1
.Quicksort_label_322:
/* @325 */ 	lda         __i9
/* @326 */ 	cmp         __i8
/* @327 */ 	lda         __i9+1
/* @328 */ 	sbc         __i8+1
/* @329 */ 	bvc         .Quicksort_label_324
/* @330 */ 	eor          #128
.Quicksort_label_324:
/* @331 */ 	bmi         .Quicksort_label_355
/* @356 */ 	jmp         .Quicksort_label_102
.Quicksort_label_355:
/* @333 */ 	jsr         __pushi10
/* @334 */ 	jsr         __pushi9
/* @335 */ 	jsr         __pushi11
/* @336 */ 	jsr         __pushi7
/* @337 */ 	jsr         __pushi4
/* @338 */ 	jsr         Quicksort
/* @340 */ 	jsr         __incsp10
/* @341 */ 	jsr         __pushi10
/* @342 */ 	jsr         __pushi5
/* @343 */ 	jsr         __pushi8
/* @344 */ 	jsr         __pushi7
/* @345 */ 	jsr         __pushi4
/* @346 */ 	jsr         Quicksort
/* @347 */ 	jsr         __incsp10
.Quicksort_label_348:
/* @349 */ 	ldy          #10
	jmp          __leave_void
.func_end_Quicksort:
	.size Quicksort, .func_end_Quicksort-Quicksort

	.global qsort
	.type qsort, @function

qsort:
/* @6 */ 	ldx          #7
	jsr          __enter
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @11 */ 	ldx          #6
	jsr          __arg_value2_i0			// compar
/* @15 */ 	ldx          #2
	jsr          __arg_value2_i1			// nmemb
/* @19 */ 	ldx          #4
	jsr          __arg_value2_i2			// size
/* @23 */ 	ldx          #0
	jsr          __arg_value2_i3			// base
/* @24 */ 	jsr         __pushi0
/* @27 */ 	lda          #__i4
/* @28 */ 	ldx          #__i1
/* @29 */ 	ldy          #__i2
/* @31 */ 	jsr         __umul2
/* @35 */ 	sec         
/* @37 */ 	lda         __i4
/* @38 */ 	sbc         __i2
/* @39 */ 	sta         __i5
/* @41 */ 	lda         __i4+1
/* @42 */ 	sbc         __i2+1
/* @43 */ 	sta         __i5+1
/* @46 */ 	jsr         __pushi5
/* @47 */ 	ldx          #0
/* @49 */ 	jsr         __pushxy0
/* @50 */ 	jsr         __pushi2
/* @51 */ 	jsr         __pushi3
/* @52 */ 	jsr         Quicksort
/* @54 */ 	jsr         __incsp10
/* @55 */ 	ldy          #10
	jmp          __leave_void
.func_end_qsort:
	.size qsort, .func_end_qsort-qsort

	.data
	.section ".rodata", "aMS", @progbits
