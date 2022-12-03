	.file   "/Users/dallison/Google Drive/c_compiler/libc/fopen.c"
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


	.global fopen
	.type fopen, @function

fopen:
/* @31 */ 	stx         __result
/* @33 */ 	sty         __result+1
/* @34 */ 	ldx          #7
	jsr          __enter
	.byte        0x05,0x00,0x00		// Save mask i:5 b:0 l:0 x:0 f:0 
/* @45 */ 	ldx          #2
	jsr          __arg_value2_i0			// mode
/* @51 */ 	ldx          #0
	jsr          __arg_value2_i7			// filename
/* @54 */ 	stz         __i4
/* @56 */ 	stz         __i4+1
/* @57 */ 	lda         __i0
/* @58 */ 	sta         __i5
/* @59 */ 	lda         __i0+1
/* @60 */ 	sta         __i5+1
.fopen_label_61:
/* @64 */ 	lda         (__i5)
/* @70 */ 	sta         __i0
/* @72 */ 	and          #128
/* @74 */ 	beq         .fopen_label_73
/* @76 */ 	lda          #255
.fopen_label_73:
/* @77 */ 	sta         __i0+1
/* @79 */ 	lda         __i0
/* @80 */ 	ora         __i0+1
/* @82 */ 	bne         .fopen_label_412
/* @413 */ 	jmp         .fopen_label_215
.fopen_label_412:
/* @85 */ 	lda         (__i5)
/* @91 */ 	sta         __i0
/* @92 */ 	and          #128
/* @94 */ 	beq         .fopen_label_93
/* @95 */ 	lda          #255
.fopen_label_93:
/* @96 */ 	sta         __i0+1
/* @99 */ 	lda         __i0
/* @100 */ 	cmp          #114
/* @101 */ 	bne         .fopen_label_120
/* @102 */ 	lda         __i0+1
/* @104 */ 	bne         .fopen_label_120
/* @108 */ 	lda         __i4
/* @109 */ 	ora          #1
/* @110 */ 	sta         __i0
/* @111 */ 	lda         __i4+1
/* @112 */ 	sta         __i0+1
/* @115 */ 	lda         __i0
/* @116 */ 	sta         __i4
/* @117 */ 	lda         __i0+1
/* @118 */ 	sta         __i4+1
/* @119 */ 	bra         .fopen_label_210
.fopen_label_120:
/* @123 */ 	lda         (__i5)
/* @129 */ 	sta         __i0
/* @130 */ 	and          #128
/* @132 */ 	beq         .fopen_label_131
/* @133 */ 	lda          #255
.fopen_label_131:
/* @134 */ 	sta         __i0+1
/* @137 */ 	lda         __i0
/* @138 */ 	cmp          #119
/* @139 */ 	bne         .fopen_label_158
/* @140 */ 	lda         __i0+1
/* @142 */ 	bne         .fopen_label_158
/* @146 */ 	lda         __i4
/* @147 */ 	ora          #2
/* @148 */ 	sta         __i0
/* @149 */ 	lda         __i4+1
/* @150 */ 	sta         __i0+1
/* @153 */ 	lda         __i0
/* @154 */ 	sta         __i4
/* @155 */ 	lda         __i0+1
/* @156 */ 	sta         __i4+1
/* @157 */ 	bra         .fopen_label_209
.fopen_label_158:
/* @161 */ 	lda         (__i5)
/* @167 */ 	sta         __i0
/* @168 */ 	and          #128
/* @170 */ 	beq         .fopen_label_169
/* @171 */ 	lda          #255
.fopen_label_169:
/* @172 */ 	sta         __i0+1
/* @175 */ 	lda         __i0
/* @176 */ 	cmp          #97
/* @177 */ 	bne         .fopen_label_196
/* @178 */ 	lda         __i0+1
/* @180 */ 	bne         .fopen_label_196
/* @184 */ 	lda         __i4
/* @185 */ 	ora          #4
/* @186 */ 	sta         __i0
/* @187 */ 	lda         __i4+1
/* @188 */ 	sta         __i0+1
/* @191 */ 	lda         __i0
/* @192 */ 	sta         __i4
/* @193 */ 	lda         __i0+1
/* @194 */ 	sta         __i4+1
/* @195 */ 	bra         .fopen_label_208
.fopen_label_196:
/* @199 */ 	stz         __i0
/* @200 */ 	stz         __i0+1
/* @201 */ 	ldx          #8
	jsr          __load_result
/* @202 */ 	lda         #__i0
/* @204 */ 	jsr         __result2
.fopen_label_205:
/* @206 */ 	ldy          #10
	jmp          __leave
.fopen_label_208:
.fopen_label_209:
.fopen_label_210:
/* @211 */ 	lda          #__i5
/* @213 */ 	jsr         __rinc21
/* @214 */ 	jmp         .fopen_label_61
.fopen_label_215:
/* @217 */ 	lda         __i4
/* @218 */ 	cmp          #3
/* @219 */ 	bne         .fopen_label_229
/* @220 */ 	lda         __i4+1
/* @222 */ 	bne         .fopen_label_229
/* @224 */ 	lda          #2
/* @225 */ 	sta         __i4
/* @227 */ 	stz         __i4+1
/* @228 */ 	bra         .fopen_label_271
.fopen_label_229:
/* @231 */ 	lda         __i4
/* @232 */ 	cmp          #1
/* @233 */ 	bne         .fopen_label_241
/* @234 */ 	lda         __i4+1
/* @236 */ 	bne         .fopen_label_241
/* @238 */ 	stz         __i4
/* @239 */ 	stz         __i4+1
/* @240 */ 	bra         .fopen_label_270
.fopen_label_241:
/* @243 */ 	lda         __i4
/* @244 */ 	cmp          #2
/* @245 */ 	bne         .fopen_label_255
/* @246 */ 	lda         __i4+1
/* @248 */ 	bne         .fopen_label_255
/* @250 */ 	lda          #65
/* @251 */ 	sta         __i4
/* @252 */ 	lda          #2
/* @253 */ 	sta         __i4+1
/* @254 */ 	bra         .fopen_label_269
.fopen_label_255:
/* @257 */ 	lda         __i4
/* @258 */ 	cmp          #4
/* @259 */ 	bne         .fopen_label_268
/* @260 */ 	lda         __i4+1
/* @262 */ 	bne         .fopen_label_268
/* @264 */ 	lda          #65
/* @265 */ 	sta         __i4
/* @266 */ 	lda          #4
/* @267 */ 	sta         __i4+1
.fopen_label_268:
.fopen_label_269:
.fopen_label_270:
.fopen_label_271:
/* @272 */ 	ldx          #255
/* @273 */ 	ldy          #1
/* @275 */ 	jsr         __pushxy
/* @276 */ 	jsr         __pushi4
/* @277 */ 	jsr         __pushi7
/* @278 */ 	ldx         #__i6
/* @279 */ 	ldy          #0
/* @280 */ 	jsr         open
/* @282 */ 	jsr         __incsp6
/* @284 */ 	lda         __i6
/* @285 */ 	cmp          #255
/* @286 */ 	bne         .fopen_label_299
/* @287 */ 	lda         __i6+1
/* @288 */ 	cmp          #255
/* @289 */ 	bne         .fopen_label_299
/* @293 */ 	stz         __i0
/* @294 */ 	stz         __i0+1
/* @295 */ 	ldx          #8
	jsr          __load_result
/* @296 */ 	lda         #__i0
/* @297 */ 	jsr         __result2
/* @298 */ 	jmp         .fopen_label_205
.fopen_label_299:
/* @301 */ 	ldx          #92
/* @303 */ 	jsr         __pushxy0
/* @304 */ 	ldx         #__i8
/* @305 */ 	ldy          #0
/* @306 */ 	jsr         malloc
/* @308 */ 	jsr         __incsp2
/* @310 */ 	lda         __i8
/* @312 */ 	bne         .fopen_label_325
/* @313 */ 	lda         __i8+1
/* @315 */ 	bne         .fopen_label_325
/* @319 */ 	stz         __i0
/* @320 */ 	stz         __i0+1
/* @321 */ 	ldx          #8
	jsr          __load_result
/* @322 */ 	lda         #__i0
/* @323 */ 	jsr         __result2
/* @324 */ 	jmp         .fopen_label_205
.fopen_label_325:
/* @328 */ 	clc         
/* @329 */ 	lda         __i8
/* @330 */ 	adc          #28
/* @331 */ 	sta         __i0
/* @332 */ 	lda         __i8+1
/* @333 */ 	adc          #0
/* @334 */ 	sta         __i0+1
/* @337 */ 	lda         __i0
/* @339 */ 	ldy          #2
/* @340 */ 	sta         (__i8), Y
/* @341 */ 	lda         __i0+1
/* @343 */ 	iny         
/* @344 */ 	sta         (__i8), Y
/* @345 */ 	lda         __i6
/* @346 */ 	sta         (__i8)
/* @347 */ 	lda         __i6+1
/* @348 */ 	ldy          #1
/* @349 */ 	sta         (__i8), Y
/* @350 */ 	lda          #64
/* @352 */ 	ldy          #4
/* @353 */ 	sta         (__i8), Y
/* @354 */ 	lda          #0
/* @356 */ 	iny         
/* @357 */ 	sta         (__i8), Y
/* @359 */ 	ldy          #10
.fopen_label_361:
/* @362 */ 	sta         (__i8), Y
/* @363 */ 	iny         
/* @365 */ 	cpy          #12
/* @366 */ 	bne         .fopen_label_361
/* @369 */ 	ldy          #4
/* @370 */ 	lda         (__i8), Y
/* @371 */ 	sta         __i0
/* @372 */ 	iny         
/* @373 */ 	lda         (__i8), Y
/* @374 */ 	sta         __i0+1
/* @377 */ 	lda         __i0
/* @379 */ 	iny         
/* @380 */ 	sta         (__i8), Y
/* @381 */ 	lda         __i0+1
/* @383 */ 	iny         
/* @384 */ 	sta         (__i8), Y
/* @385 */ 	lda          #0
/* @387 */ 	ldy          #14
/* @388 */ 	sta         (__i8), Y
/* @389 */ 	inc          A
/* @390 */ 	ldy          #12
/* @391 */ 	sta         (__i8), Y
/* @392 */ 	dec          A
/* @394 */ 	iny         
/* @395 */ 	sta         (__i8), Y
/* @396 */ 	lda          #255
/* @398 */ 	ldy          #15
/* @399 */ 	sta         (__i8), Y
/* @400 */ 	lda          #0
/* @402 */ 	iny         
/* @403 */ 	sta         (__i8), Y
/* @406 */ 	iny         
/* @407 */ 	sta         (__i8), Y
/* @408 */ 	ldx          #8
	jsr          __load_result
/* @409 */ 	lda         #__i8
/* @410 */ 	jsr         __result2
/* @411 */ 	jmp         .fopen_label_205
.func_end_fopen:
	.size fopen, .func_end_fopen-fopen

	.data
	.section ".rodata", "aMS", @progbits
