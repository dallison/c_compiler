	.file   "/Users/dallison/Google Drive/c_compiler/libc/fgetc.c"
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


	.local  SetErrorOrEof
	.type SetErrorOrEof, @function

SetErrorOrEof:
/* @5 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @9 */ 	ldx          #2
	jsr          __arg_value2_i0			// n
/* @13 */ 	ldx          #0
	jsr          __arg_value2_i1			// stream
/* @15 */ 	lda         __i0+1
/* @16 */ 	bpl         .SetErrorOrEof_label_23
/* @18 */ 	lda          #1
/* @20 */ 	ldy          #17
/* @21 */ 	sta         (__i1), Y
/* @22 */ 	bra         .SetErrorOrEof_label_28
.SetErrorOrEof_label_23:
/* @24 */ 	lda          #1
/* @26 */ 	ldy          #16
/* @27 */ 	sta         (__i1), Y
.SetErrorOrEof_label_28:
/* @29 */ 	ldy          #8
	jmp          __leave_leaf_void_nomask
.func_end_SetErrorOrEof:
	.size SetErrorOrEof, .func_end_SetErrorOrEof-SetErrorOrEof

	.global ReadFullBuffer
	.type ReadFullBuffer, @function

ReadFullBuffer:
/* @6 */ 	ldx          #7
	jsr          __enter
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
/* @15 */ 	ldx          #0
	jsr          __arg_value2_i5			// stream
/* @23 */ 	ldy          #2
/* @24 */ 	lda         (__i5), Y
/* @26 */ 	sta         __i0
/* @28 */ 	iny         
/* @29 */ 	lda         (__i5), Y
/* @31 */ 	sta         __i0+1
/* @34 */ 	lda         __i0
/* @35 */ 	sta         __i4
/* @36 */ 	lda         __i0+1
/* @37 */ 	sta         __i4+1
/* @41 */ 	iny         
/* @42 */ 	lda         (__i5), Y
/* @43 */ 	sta         __i0
/* @45 */ 	iny         
/* @46 */ 	lda         (__i5), Y
/* @47 */ 	sta         __i0+1
/* @50 */ 	lda         __i0
/* @51 */ 	sta         __i6
/* @52 */ 	lda         __i0+1
/* @53 */ 	sta         __i6+1
.ReadFullBuffer_label_54:
/* @56 */ 	lda          #0
/* @57 */ 	cmp         __i6
/* @59 */ 	sbc         __i6+1
/* @60 */ 	bvc         .ReadFullBuffer_label_55
/* @62 */ 	eor          #128
.ReadFullBuffer_label_55:
/* @63 */ 	bmi         .ReadFullBuffer_label_168
/* @169 */ 	jmp         .ReadFullBuffer_label_165
.ReadFullBuffer_label_168:
/* @64 */ 	jsr         __pushi6
/* @65 */ 	jsr         __pushi4
/* @68 */ 	lda         (__i5)
/* @69 */ 	sta         __i0
/* @70 */ 	ldy          #1
/* @71 */ 	lda         (__i5), Y
/* @72 */ 	sta         __i0+1
/* @75 */ 	jsr         __pushi0
/* @76 */ 	ldx         #__i7
/* @77 */ 	ldy          #0
/* @78 */ 	jsr         read
/* @80 */ 	jsr         __incsp6
/* @82 */ 	lda          #0
/* @83 */ 	cmp         __i7
/* @85 */ 	sbc         __i7+1
/* @86 */ 	bvc         .ReadFullBuffer_label_81
/* @87 */ 	eor          #128
.ReadFullBuffer_label_81:
/* @88 */ 	bmi         .ReadFullBuffer_label_95
/* @89 */ 	jsr         __pushi7
/* @90 */ 	jsr         __pushi5
/* @91 */ 	jsr         SetErrorOrEof
/* @93 */ 	jsr         __incsp4
/* @94 */ 	bra         .ReadFullBuffer_label_165
.ReadFullBuffer_label_95:
/* @98 */ 	lda         __i5
/* @99 */ 	sta         __i0
/* @100 */ 	lda         __i5+1
/* @101 */ 	sta         __i0+1
/* @107 */ 	ldy          #8
/* @108 */ 	lda         (__i0), Y
/* @109 */ 	sta         __i1
/* @111 */ 	iny         
/* @112 */ 	lda         (__i0), Y
/* @113 */ 	sta         __i1+1
/* @117 */ 	clc         
/* @118 */ 	lda         __i1
/* @119 */ 	adc         __i7
/* @120 */ 	sta         __i2
/* @121 */ 	lda         __i1+1
/* @122 */ 	adc         __i7+1
/* @123 */ 	sta         __i2+1
/* @128 */ 	lda         __i2
/* @129 */ 	dey         
/* @130 */ 	sta         (__i0), Y
/* @131 */ 	lda         __i2+1
/* @132 */ 	iny         
/* @133 */ 	sta         (__i0), Y
/* @136 */ 	clc         
/* @137 */ 	lda         __i4
/* @138 */ 	adc         __i7
/* @139 */ 	sta         __i0
/* @140 */ 	lda         __i4+1
/* @141 */ 	adc         __i7+1
/* @142 */ 	sta         __i0+1
/* @145 */ 	lda         __i0
/* @146 */ 	sta         __i4
/* @147 */ 	lda         __i0+1
/* @148 */ 	sta         __i4+1
/* @151 */ 	sec         
/* @152 */ 	lda         __i6
/* @153 */ 	sbc         __i7
/* @154 */ 	sta         __i0
/* @155 */ 	lda         __i6+1
/* @156 */ 	sbc         __i7+1
/* @157 */ 	sta         __i0+1
/* @160 */ 	lda         __i0
/* @161 */ 	sta         __i6
/* @162 */ 	lda         __i0+1
/* @163 */ 	sta         __i6+1
/* @164 */ 	jmp         .ReadFullBuffer_label_54
.ReadFullBuffer_label_165:
/* @166 */ 	ldy          #10
	jmp          __leave_void
.func_end_ReadFullBuffer:
	.size ReadFullBuffer, .func_end_ReadFullBuffer-ReadFullBuffer

	.global getc
	.type getc, @function

getc:
/* @16 */ 	stx         __result
/* @18 */ 	sty         __result+1
/* @19 */ 	ldx          #8
	jsr          __enter
	.byte        0x45,0x00,0x00		// Save mask i:5 b:2 l:0 x:0 f:0 
/* @26 */ 	ldx          #0
	jsr          __arg_value2_i4			// stream
/* @41 */ 	ldy          #2
/* @42 */ 	lda         (__i4), Y
/* @44 */ 	sta         __i0
/* @46 */ 	iny         
/* @47 */ 	lda         (__i4), Y
/* @48 */ 	sta         __i0+1
/* @51 */ 	lda         __i0
/* @53 */ 	bne         .getc_label_132
/* @54 */ 	lda         __i0+1
/* @56 */ 	bne         .getc_label_132
/* @58 */ 	ldx          #1
/* @60 */ 	jsr         __pushxy0
/* @62 */ 	ldx          #4
	jsr          __var_addr_i8			// rbuf
/* @65 */ 	jsr         __pushi8
/* @68 */ 	lda         (__i4)
/* @69 */ 	sta         __i0
/* @70 */ 	ldy          #1
/* @71 */ 	lda         (__i4), Y
/* @72 */ 	sta         __i0+1
/* @75 */ 	jsr         __pushi0
/* @76 */ 	ldx         #__i5
/* @77 */ 	ldy          #0
/* @78 */ 	jsr         read
/* @80 */ 	jsr         __incsp6
/* @82 */ 	lda          #0
/* @83 */ 	cmp         __i5
/* @85 */ 	sbc         __i5+1
/* @86 */ 	bvc         .getc_label_81
/* @88 */ 	eor          #128
.getc_label_81:
/* @89 */ 	bmi         .getc_label_108
/* @90 */ 	jsr         __pushi5
/* @91 */ 	jsr         __pushi4
/* @92 */ 	jsr         SetErrorOrEof
/* @94 */ 	jsr         __incsp4
/* @97 */ 	lda          #255
/* @98 */ 	sta         __i0
/* @100 */ 	sta         __i0+1
/* @101 */ 	ldx          #9
	jsr          __load_result
/* @102 */ 	lda         #__i0
/* @104 */ 	jsr         __result2
.getc_label_105:
/* @106 */ 	ldy          #11
	jmp          __leave
.getc_label_108:
/* @114 */ 	lda         (__i8)
/* @120 */ 	sta         __i0
/* @121 */ 	and          #128
/* @123 */ 	beq         .getc_label_122
/* @125 */ 	lda          #255
.getc_label_122:
/* @126 */ 	sta         __i0+1
/* @128 */ 	ldx          #9
	jsr          __load_result
/* @129 */ 	lda         #__i0
/* @130 */ 	jsr         __result2
/* @131 */ 	bra         .getc_label_105
.getc_label_132:
/* @136 */ 	ldy          #15
/* @137 */ 	lda         (__i4), Y
/* @143 */ 	sta         __i0
/* @144 */ 	and          #128
/* @146 */ 	beq         .getc_label_145
/* @147 */ 	lda          #255
.getc_label_145:
/* @148 */ 	sta         __i0+1
/* @151 */ 	lda          #0
/* @152 */ 	cmp         __i0
/* @154 */ 	sbc         __i0+1
/* @155 */ 	bvc         .getc_label_150
/* @156 */ 	eor          #128
.getc_label_150:
/* @157 */ 	bpl         .getc_label_227
/* @160 */ 	ldy          #2
/* @161 */ 	lda         (__i4), Y
/* @162 */ 	sta         __i0
/* @163 */ 	iny         
/* @164 */ 	lda         (__i4), Y
/* @165 */ 	sta         __i0+1
/* @168 */ 	clc         
/* @169 */ 	lda         __i4
/* @170 */ 	adc          #15
/* @171 */ 	sta         __i1
/* @172 */ 	lda         __i4+1
/* @173 */ 	adc          #0
/* @174 */ 	sta         __i1+1
/* @176 */ 	lda          #__i1
/* @178 */ 	jsr         __dec1
/* @183 */ 	lda         __i1
/* @189 */ 	sta         __i1
/* @190 */ 	and          #128
/* @192 */ 	beq         .getc_label_191
/* @193 */ 	lda          #255
.getc_label_191:
/* @194 */ 	sta         __i1+1
/* @199 */ 	clc         
/* @200 */ 	lda         __i0
/* @201 */ 	adc         __i1
/* @202 */ 	sta         __i2
/* @203 */ 	lda         __i0+1
/* @204 */ 	adc         __i1+1
/* @205 */ 	sta         __i2+1
/* @210 */ 	lda         (__i2)
/* @216 */ 	sta         __i0
/* @217 */ 	and          #128
/* @219 */ 	beq         .getc_label_218
/* @220 */ 	lda          #255
.getc_label_218:
/* @221 */ 	sta         __i0+1
/* @223 */ 	ldx          #9
	jsr          __load_result
/* @224 */ 	lda         #__i0
/* @225 */ 	jsr         __result2
/* @226 */ 	jmp         .getc_label_105
.getc_label_227:
/* @231 */ 	ldy          #6
/* @232 */ 	lda         (__i4), Y
/* @233 */ 	sta         __i0
/* @235 */ 	iny         
/* @236 */ 	lda         (__i4), Y
/* @237 */ 	sta         __i0+1
/* @241 */ 	iny         
/* @242 */ 	lda         (__i4), Y
/* @243 */ 	sta         __i1
/* @245 */ 	iny         
/* @246 */ 	lda         (__i4), Y
/* @247 */ 	sta         __i1+1
/* @251 */ 	lda         __i0
/* @252 */ 	cmp         __i1
/* @253 */ 	lda         __i0+1
/* @254 */ 	sbc         __i1+1
/* @255 */ 	bvc         .getc_label_250
/* @256 */ 	eor          #128
.getc_label_250:
/* @257 */ 	bpl         .getc_label_328
/* @260 */ 	ldy          #2
/* @261 */ 	lda         (__i4), Y
/* @262 */ 	sta         __i0
/* @263 */ 	iny         
/* @264 */ 	lda         (__i4), Y
/* @265 */ 	sta         __i0+1
/* @268 */ 	lda         __i4
/* @269 */ 	sta         __i1
/* @270 */ 	lda         __i4+1
/* @271 */ 	sta         __i1+1
/* @275 */ 	clc         
/* @276 */ 	lda         __i1
/* @277 */ 	adc          #6
/* @278 */ 	sta         __i2
/* @279 */ 	lda         __i1+1
/* @280 */ 	adc          #0
/* @281 */ 	sta         __i2+1
/* @286 */ 	ldy          #6
/* @287 */ 	lda         (__i1), Y
/* @288 */ 	sta         __i3
/* @289 */ 	iny         
/* @290 */ 	lda         (__i1), Y
/* @291 */ 	sta         __i3+1
/* @293 */ 	lda          #__i2
/* @295 */ 	jsr         __inc21
/* @300 */ 	clc         
/* @301 */ 	lda         __i0
/* @302 */ 	adc         __i3
/* @303 */ 	sta         __i1
/* @304 */ 	lda         __i0+1
/* @305 */ 	adc         __i3+1
/* @306 */ 	sta         __i1+1
/* @311 */ 	lda         (__i1)
/* @317 */ 	sta         __i0
/* @318 */ 	and          #128
/* @320 */ 	beq         .getc_label_319
/* @321 */ 	lda          #255
.getc_label_319:
/* @322 */ 	sta         __i0+1
/* @324 */ 	ldx          #9
	jsr          __load_result
/* @325 */ 	lda         #__i0
/* @326 */ 	jsr         __result2
/* @327 */ 	jmp         .getc_label_105
.getc_label_328:
/* @334 */ 	ldy          #17
/* @335 */ 	lda         (__i4), Y
/* @341 */ 	sta         __i0
/* @342 */ 	and          #128
/* @344 */ 	beq         .getc_label_343
/* @345 */ 	lda          #255
.getc_label_343:
/* @346 */ 	sta         __i0+1
/* @351 */ 	ldx          #1
/* @352 */ 	lda         __i0
/* @353 */ 	ora         __i0+1
/* @355 */ 	bne         .getc_label_350
/* @356 */ 	dex         
.getc_label_350:
/* @357 */ 	stx         __b3
/* @359 */ 	txa         
/* @360 */ 	bne         .getc_label_388
/* @364 */ 	ldy          #16
/* @365 */ 	lda         (__i4), Y
/* @371 */ 	sta         __i0
/* @372 */ 	and          #128
/* @374 */ 	beq         .getc_label_373
/* @375 */ 	lda          #255
.getc_label_373:
/* @376 */ 	sta         __i0+1
/* @381 */ 	ldx          #1
/* @382 */ 	lda         __i0
/* @383 */ 	ora         __i0+1
/* @385 */ 	bne         .getc_label_380
/* @386 */ 	dex         
.getc_label_380:
/* @387 */ 	stx         __b3
.getc_label_388:
/* @390 */ 	lda         __b3
/* @392 */ 	beq         .getc_label_403
/* @395 */ 	lda          #255
/* @396 */ 	sta         __i0
/* @398 */ 	sta         __i0+1
/* @399 */ 	ldx          #9
	jsr          __load_result
/* @400 */ 	lda         #__i0
/* @401 */ 	jsr         __result2
/* @402 */ 	jmp         .getc_label_105
.getc_label_403:
/* @404 */ 	ldy          #6
/* @405 */ 	lda          #0
.getc_label_406:
/* @408 */ 	sta         (__i4), Y
/* @409 */ 	iny         
/* @410 */ 	cpy          #8
/* @411 */ 	bne         .getc_label_406
/* @412 */ 	ldy          #8
/* @413 */ 	lda          #0
.getc_label_414:
/* @415 */ 	sta         (__i4), Y
/* @416 */ 	iny         
/* @417 */ 	cpy          #10
/* @418 */ 	bne         .getc_label_414
/* @422 */ 	ldy          #12
/* @423 */ 	lda         (__i4), Y
/* @424 */ 	sta         __i0
/* @426 */ 	iny         
/* @427 */ 	lda         (__i4), Y
/* @428 */ 	sta         __i0+1
/* @431 */ 	lda         __i0
/* @432 */ 	cmp          #1
/* @433 */ 	bne         .getc_label_443
/* @434 */ 	lda         __i0+1
/* @436 */ 	bne         .getc_label_443
/* @438 */ 	jsr         __pushi4
/* @439 */ 	jsr         ReadFullBuffer
/* @441 */ 	jsr         __incsp2
/* @442 */ 	jmp         .getc_label_626
.getc_label_443:
/* @446 */ 	ldy          #2
/* @447 */ 	lda         (__i4), Y
/* @448 */ 	sta         __i0
/* @449 */ 	iny         
/* @450 */ 	lda         (__i4), Y
/* @451 */ 	sta         __i0+1
/* @454 */ 	ldy          #8
/* @455 */ 	lda         (__i4), Y
/* @456 */ 	sta         __i1
/* @457 */ 	iny         
/* @458 */ 	lda         (__i4), Y
/* @459 */ 	sta         __i1+1
/* @464 */ 	clc         
/* @465 */ 	lda         __i0
/* @466 */ 	adc         __i1
/* @467 */ 	sta         __i2
/* @468 */ 	lda         __i0+1
/* @469 */ 	adc         __i1+1
/* @470 */ 	sta         __i2+1
/* @473 */ 	lda         __i2
/* @474 */ 	sta         __i6
/* @475 */ 	lda         __i2+1
/* @476 */ 	sta         __i6+1
.getc_label_477:
/* @478 */ 	ldx          #1
/* @479 */ 	jsr         __pushxy0
/* @480 */ 	jsr         __pushi6
/* @483 */ 	lda         (__i4)
/* @484 */ 	sta         __i0
/* @485 */ 	ldy          #1
/* @486 */ 	lda         (__i4), Y
/* @487 */ 	sta         __i0+1
/* @490 */ 	jsr         __pushi0
/* @491 */ 	ldx         #__i7
/* @492 */ 	ldy          #0
/* @493 */ 	jsr         read
/* @494 */ 	jsr         __incsp6
/* @496 */ 	lda          #0
/* @497 */ 	cmp         __i7
/* @499 */ 	sbc         __i7+1
/* @500 */ 	bvc         .getc_label_495
/* @501 */ 	eor          #128
.getc_label_495:
/* @502 */ 	bmi         .getc_label_508
/* @503 */ 	jsr         __pushi7
/* @504 */ 	jsr         __pushi4
/* @505 */ 	jsr         SetErrorOrEof
/* @506 */ 	jsr         __incsp4
/* @507 */ 	jmp         .getc_label_625
.getc_label_508:
/* @511 */ 	lda         __i6
/* @512 */ 	sta         __i0
/* @513 */ 	lda         __i6+1
/* @514 */ 	sta         __i0+1
/* @515 */ 	lda          #__i6
/* @517 */ 	jsr         __rinc21
/* @522 */ 	lda         (__i0)
/* @527 */ 	sta         __b2
/* @530 */ 	clc         
/* @531 */ 	lda         __i4
/* @532 */ 	adc          #8
/* @533 */ 	sta         __i0
/* @534 */ 	lda         __i4+1
/* @535 */ 	adc          #0
/* @536 */ 	sta         __i0+1
/* @538 */ 	lda          #__i0
/* @539 */ 	jsr         __inc21
/* @544 */ 	ldy          #12
/* @545 */ 	lda         (__i4), Y
/* @546 */ 	sta         __i0
/* @547 */ 	iny         
/* @548 */ 	lda         (__i4), Y
/* @549 */ 	sta         __i0+1
/* @554 */ 	ldx          #1
/* @555 */ 	lda         __i0
/* @556 */ 	cmp          #2
/* @557 */ 	bne         .getc_label_552
/* @558 */ 	lda         __i0+1
/* @560 */ 	beq         .getc_label_553
.getc_label_552:
/* @561 */ 	dex         
.getc_label_553:
/* @562 */ 	stx         __b0
/* @564 */ 	txa         
/* @565 */ 	cmp          #0
/* @566 */ 	beq         .getc_label_589
/* @569 */ 	lda         __b2
/* @570 */ 	sta         __i0
/* @571 */ 	and          #128
/* @573 */ 	beq         .getc_label_572
/* @574 */ 	lda          #255
.getc_label_572:
/* @575 */ 	sta         __i0+1
/* @580 */ 	ldx          #1
/* @581 */ 	lda         __i0
/* @582 */ 	cmp          #10
/* @583 */ 	bne         .getc_label_578
/* @584 */ 	lda         __i0+1
/* @586 */ 	beq         .getc_label_579
.getc_label_578:
/* @587 */ 	dex         
.getc_label_579:
/* @588 */ 	stx         __b0
.getc_label_589:
/* @591 */ 	lda         __b0
/* @592 */ 	bne         .getc_label_625
/* @596 */ 	ldy          #8
/* @597 */ 	lda         (__i4), Y
/* @598 */ 	sta         __i0
/* @599 */ 	iny         
/* @600 */ 	lda         (__i4), Y
/* @601 */ 	sta         __i0+1
/* @605 */ 	ldy          #4
/* @606 */ 	lda         (__i4), Y
/* @607 */ 	sta         __i1
/* @609 */ 	iny         
/* @610 */ 	lda         (__i4), Y
/* @611 */ 	sta         __i1+1
/* @615 */ 	lda         __i0
/* @616 */ 	cmp         __i1
/* @617 */ 	bne         .getc_label_614
/* @618 */ 	lda         __i0+1
/* @619 */ 	cmp         __i1+1
/* @620 */ 	beq         .getc_label_625
.getc_label_614:
/* @624 */ 	jmp         .getc_label_477
.getc_label_625:
.getc_label_626:
/* @629 */ 	ldy          #2
/* @630 */ 	lda         (__i4), Y
/* @631 */ 	sta         __i0
/* @632 */ 	iny         
/* @633 */ 	lda         (__i4), Y
/* @634 */ 	sta         __i0+1
/* @637 */ 	lda         __i4
/* @638 */ 	sta         __i1
/* @639 */ 	lda         __i4+1
/* @640 */ 	sta         __i1+1
/* @644 */ 	clc         
/* @645 */ 	lda         __i1
/* @646 */ 	adc          #6
/* @647 */ 	sta         __i2
/* @648 */ 	lda         __i1+1
/* @649 */ 	adc          #0
/* @650 */ 	sta         __i2+1
/* @655 */ 	ldy          #6
/* @656 */ 	lda         (__i1), Y
/* @657 */ 	sta         __i3
/* @658 */ 	iny         
/* @659 */ 	lda         (__i1), Y
/* @660 */ 	sta         __i3+1
/* @662 */ 	lda          #__i2
/* @663 */ 	jsr         __inc21
/* @668 */ 	clc         
/* @669 */ 	lda         __i0
/* @670 */ 	adc         __i3
/* @671 */ 	sta         __i1
/* @672 */ 	lda         __i0+1
/* @673 */ 	adc         __i3+1
/* @674 */ 	sta         __i1+1
/* @679 */ 	lda         (__i1)
/* @685 */ 	sta         __i0
/* @686 */ 	and          #128
/* @688 */ 	beq         .getc_label_687
/* @689 */ 	lda          #255
.getc_label_687:
/* @690 */ 	sta         __i0+1
/* @692 */ 	ldx          #9
	jsr          __load_result
/* @693 */ 	lda         #__i0
/* @694 */ 	jsr         __result2
/* @695 */ 	jmp         .getc_label_105
.func_end_getc:
	.size getc, .func_end_getc-getc

	.global fgetc
	.type fgetc, @function

fgetc:
/* @3 */ 	stx         __result
/* @5 */ 	sty         __result+1
/* @6 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @11 */ 	ldx          #0
	jsr          __arg_value2_i0			// stream
/* @12 */ 	jsr         __pushi0
/* @15 */ 	ldx         #__i4
/* @16 */ 	ldy          #0
/* @17 */ 	jsr         getc
/* @19 */ 	jsr         __incsp2
/* @21 */ 	ldx          #8
	jsr          __load_result
/* @22 */ 	lda         #__i4
/* @24 */ 	jsr         __result2
/* @26 */ 	ldy          #10
	jmp          __leave
.func_end_fgetc:
	.size fgetc, .func_end_fgetc-fgetc

	.global getchar
	.type getchar, @function

getchar:
/* @3 */ 	stx         __result
/* @5 */ 	sty         __result+1
/* @6 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @12 */ 	lda         stdin+0
/* @13 */ 	sta         __i0
/* @15 */ 	lda         stdin+1
/* @16 */ 	sta         __i0+1
/* @18 */ 	jsr         __pushi0
/* @20 */ 	ldx         #__i4
/* @21 */ 	ldy          #0
/* @22 */ 	jsr         getc
/* @24 */ 	jsr         __incsp2
/* @26 */ 	ldx          #8
	jsr          __load_result
/* @27 */ 	lda         #__i4
/* @29 */ 	jsr         __result2
/* @31 */ 	ldy          #10
	jmp          __leave
.func_end_getchar:
	.size getchar, .func_end_getchar-getchar

	.data
	.section ".rodata", "aMS", @progbits
