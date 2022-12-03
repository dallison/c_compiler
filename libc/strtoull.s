	.file   "/Users/dallison/Google Drive/c_compiler/libc/strtoull.c"
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


	.global strtoull
	.type strtoull, @function

strtoull:
/* @29 */ 	stx         __result
/* @31 */ 	sty         __result+1
/* @32 */ 	ldx          #15
	jsr          __enter
	.byte        0xa3,0x60,0x00		// Save mask i:3 b:5 l:0 x:3 f:0 
/* @41 */ 	ldx          #2
	jsr          __arg_value2_i0			// end
/* @45 */ 	ldx          #0
	jsr          __arg_value2_i1			// str
/* @52 */ 	ldx          #4
	jsr          __arg_value2_i3			// base
/* @63 */ 	stz         __b1
.strtoull_label_64:
/* @69 */ 	lda         (__i1)
/* @75 */ 	sta         __i5
/* @77 */ 	and          #128
/* @79 */ 	beq         .strtoull_label_78
/* @81 */ 	lda          #255
.strtoull_label_78:
/* @82 */ 	sta         __i5+1
/* @87 */ 	ldx          #1
/* @88 */ 	lda         __i5
/* @89 */ 	ora         __i5+1
/* @91 */ 	bne         .strtoull_label_86
/* @92 */ 	dex         
.strtoull_label_86:
/* @93 */ 	stx         __b2
/* @95 */ 	txa         
/* @96 */ 	cmp          #0
/* @97 */ 	beq         .strtoull_label_126
/* @100 */ 	lda         (__i1)
/* @106 */ 	sta         __i5
/* @107 */ 	and          #128
/* @109 */ 	beq         .strtoull_label_108
/* @110 */ 	lda          #255
.strtoull_label_108:
/* @111 */ 	sta         __i5+1
/* @114 */ 	ldx         __i5
/* @115 */ 	ldy         __i5+1
/* @117 */ 	jsr         __builtin_isspace
/* @118 */ 	sta         __i6
/* @119 */ 	stz         __i6+1
/* @122 */ 	lda         __i6+1
/* @123 */ 	and          #128
/* @124 */ 	ora         __i6
/* @125 */ 	sta         __b2
.strtoull_label_126:
/* @128 */ 	lda         __b2
/* @130 */ 	beq         .strtoull_label_135
/* @131 */ 	lda          #__i1
/* @133 */ 	jsr         __rinc21
/* @134 */ 	bra         .strtoull_label_64
.strtoull_label_135:
/* @138 */ 	lda         (__i1)
/* @144 */ 	sta         __i5
/* @145 */ 	and          #128
/* @147 */ 	beq         .strtoull_label_146
/* @148 */ 	lda          #255
.strtoull_label_146:
/* @149 */ 	sta         __i5+1
/* @152 */ 	lda         __i5
/* @153 */ 	cmp          #45
/* @154 */ 	bne         .strtoull_label_164
/* @155 */ 	lda         __i5+1
/* @157 */ 	bne         .strtoull_label_164
/* @159 */ 	lda          #1
/* @160 */ 	sta         __b1
/* @161 */ 	lda          #__i1
/* @162 */ 	jsr         __rinc21
/* @163 */ 	bra         .strtoull_label_191
.strtoull_label_164:
/* @167 */ 	lda         (__i1)
/* @173 */ 	sta         __i5
/* @174 */ 	and          #128
/* @176 */ 	beq         .strtoull_label_175
/* @177 */ 	lda          #255
.strtoull_label_175:
/* @178 */ 	sta         __i5+1
/* @181 */ 	lda         __i5
/* @182 */ 	cmp          #43
/* @183 */ 	bne         .strtoull_label_190
/* @184 */ 	lda         __i5+1
/* @186 */ 	bne         .strtoull_label_190
/* @188 */ 	lda          #__i1
/* @189 */ 	jsr         __rinc21
.strtoull_label_190:
.strtoull_label_191:
/* @192 */ 	lda         __i1
/* @193 */ 	sta         __i2
/* @194 */ 	lda         __i1+1
/* @195 */ 	sta         __i2+1
/* @201 */ 	ldx          #1
/* @202 */ 	lda         __i3
/* @203 */ 	cmp          #16
/* @204 */ 	bne         .strtoull_label_199
/* @205 */ 	lda         __i3+1
/* @207 */ 	beq         .strtoull_label_200
.strtoull_label_199:
/* @208 */ 	dex         
.strtoull_label_200:
/* @209 */ 	stx         __b3
/* @211 */ 	txa         
/* @212 */ 	bne         .strtoull_label_223
/* @216 */ 	ldx          #1
/* @217 */ 	lda         __i3
/* @218 */ 	ora         __i3+1
/* @220 */ 	beq         .strtoull_label_215
/* @221 */ 	dex         
.strtoull_label_215:
/* @222 */ 	stx         __b3
.strtoull_label_223:
/* @225 */ 	lda         __b3
/* @227 */ 	beq         .strtoull_label_322
/* @232 */ 	lda         (__i1)
/* @238 */ 	sta         __i5
/* @239 */ 	and          #128
/* @241 */ 	beq         .strtoull_label_240
/* @242 */ 	lda          #255
.strtoull_label_240:
/* @243 */ 	sta         __i5+1
/* @248 */ 	ldx          #1
/* @249 */ 	lda         __i5
/* @250 */ 	cmp          #48
/* @251 */ 	bne         .strtoull_label_246
/* @252 */ 	lda         __i5+1
/* @254 */ 	beq         .strtoull_label_247
.strtoull_label_246:
/* @255 */ 	dex         
.strtoull_label_247:
/* @256 */ 	stx         __b4
/* @258 */ 	txa         
/* @259 */ 	cmp          #0
/* @260 */ 	beq         .strtoull_label_297
/* @263 */ 	ldy          #1
/* @264 */ 	lda         (__i1), Y
/* @270 */ 	sta         __i5
/* @271 */ 	and          #128
/* @273 */ 	beq         .strtoull_label_272
/* @274 */ 	lda          #255
.strtoull_label_272:
/* @275 */ 	sta         __i5+1
/* @278 */ 	ldx         __i5
/* @279 */ 	ldy         __i5+1
/* @281 */ 	jsr         __builtin_tolower
/* @282 */ 	sta         __i6
/* @283 */ 	stz         __i6+1
/* @288 */ 	ldx          #1
/* @290 */ 	cmp          #120
/* @291 */ 	bne         .strtoull_label_286
/* @292 */ 	lda         __i6+1
/* @294 */ 	beq         .strtoull_label_287
.strtoull_label_286:
/* @295 */ 	dex         
.strtoull_label_287:
/* @296 */ 	stx         __b4
.strtoull_label_297:
/* @299 */ 	lda         __b4
/* @301 */ 	beq         .strtoull_label_321
/* @304 */ 	clc         
/* @305 */ 	lda         __i1
/* @306 */ 	adc          #2
/* @307 */ 	sta         __i5
/* @308 */ 	lda         __i1+1
/* @309 */ 	adc          #0
/* @310 */ 	sta         __i5+1
/* @313 */ 	lda         __i5
/* @314 */ 	sta         __i1
/* @315 */ 	lda         __i5+1
/* @316 */ 	sta         __i1+1
/* @317 */ 	lda          #16
/* @318 */ 	sta         __i3
/* @320 */ 	stz         __i3+1
.strtoull_label_321:
.strtoull_label_322:
/* @328 */ 	ldx          #1
/* @329 */ 	lda         __i3
/* @330 */ 	cmp          #8
/* @331 */ 	bne         .strtoull_label_326
/* @332 */ 	lda         __i3+1
/* @334 */ 	beq         .strtoull_label_327
.strtoull_label_326:
/* @335 */ 	dex         
.strtoull_label_327:
/* @336 */ 	stx         __b4
/* @338 */ 	txa         
/* @339 */ 	bne         .strtoull_label_350
/* @343 */ 	ldx          #1
/* @344 */ 	lda         __i3
/* @345 */ 	ora         __i3+1
/* @347 */ 	beq         .strtoull_label_342
/* @348 */ 	dex         
.strtoull_label_342:
/* @349 */ 	stx         __b4
.strtoull_label_350:
/* @352 */ 	lda         __b4
/* @354 */ 	beq         .strtoull_label_383
/* @357 */ 	lda         (__i1)
/* @363 */ 	sta         __i5
/* @364 */ 	and          #128
/* @366 */ 	beq         .strtoull_label_365
/* @367 */ 	lda          #255
.strtoull_label_365:
/* @368 */ 	sta         __i5+1
/* @371 */ 	lda         __i5
/* @372 */ 	cmp          #48
/* @373 */ 	bne         .strtoull_label_382
/* @374 */ 	lda         __i5+1
/* @376 */ 	bne         .strtoull_label_382
/* @378 */ 	lda          #8
/* @379 */ 	sta         __i3
/* @381 */ 	stz         __i3+1
.strtoull_label_382:
.strtoull_label_383:
/* @384 */ 	lda         __i3
/* @385 */ 	ora         __i3+1
/* @386 */ 	bne         .strtoull_label_391
/* @387 */ 	lda          #10
/* @388 */ 	sta         __i3
/* @390 */ 	stz         __i3+1
.strtoull_label_391:
/* @396 */ 	ldx          #0
/* @398 */ 	lda         __i3
/* @399 */ 	cmp          #2
/* @400 */ 	lda         __i3+1
/* @401 */ 	sbc          #0
/* @402 */ 	bvc         .strtoull_label_397
/* @403 */ 	eor          #128
.strtoull_label_397:
/* @404 */ 	bpl         .strtoull_label_395
/* @405 */ 	inx         
.strtoull_label_395:
/* @406 */ 	stx         __b5
/* @408 */ 	txa         
/* @409 */ 	bne         .strtoull_label_423
/* @412 */ 	ldx          #0
/* @414 */ 	lda          #36
/* @415 */ 	cmp         __i3
/* @416 */ 	txa         
/* @417 */ 	sbc         __i3+1
/* @418 */ 	bvc         .strtoull_label_413
/* @419 */ 	eor          #128
.strtoull_label_413:
/* @420 */ 	bpl         .strtoull_label_411
/* @421 */ 	inx         
.strtoull_label_411:
/* @422 */ 	stx         __b5
.strtoull_label_423:
/* @425 */ 	lda         __b5
/* @427 */ 	beq         .strtoull_label_458
/* @430 */ 	lda          #214
/* @431 */ 	sta         __i5
/* @432 */ 	lda          #3
/* @433 */ 	sta         __i5+1
/* @435 */ 	lda          #7
/* @436 */ 	ldy          #0
/* @437 */ 	sta         (__i5)
/* @438 */ 	tya         
/* @439 */ 	iny         
/* @440 */ 	sta         (__i5), Y
/* @444 */ 	ldx          #7
.strtoull_label_446:
/* @448 */ 	sta         __x1, X
/* @449 */ 	dex         
/* @450 */ 	bpl         .strtoull_label_446
/* @451 */ 	ldx          #16
	jsr          __load_result
/* @452 */ 	lda         #__x1
/* @454 */ 	jsr         __result8
.strtoull_label_455:
/* @456 */ 	ldy          #18
	jmp          __leave
.strtoull_label_458:
/* @459 */ 	ldx          #7
/* @460 */ 	lda          #0
.strtoull_label_461:
/* @462 */ 	sta         __x0, X
/* @463 */ 	dex         
/* @464 */ 	bpl         .strtoull_label_461
.strtoull_label_465:
/* @468 */ 	lda         (__i1)
/* @474 */ 	sta         __i5
/* @475 */ 	and          #128
/* @477 */ 	beq         .strtoull_label_476
/* @478 */ 	lda          #255
.strtoull_label_476:
/* @479 */ 	sta         __i5+1
/* @481 */ 	lda         __i5
/* @482 */ 	ora         __i5+1
/* @484 */ 	bne         .strtoull_label_834
/* @835 */ 	jmp         .strtoull_label_757
.strtoull_label_834:
/* @487 */ 	lda         (__i1)
/* @492 */ 	sta         __b0
/* @493 */ 	lda         __i3
/* @494 */ 	sta         __i4
/* @495 */ 	lda         __i3+1
/* @496 */ 	sta         __i4+1
/* @499 */ 	lda         __b0
/* @500 */ 	sta         __i5
/* @501 */ 	and          #128
/* @503 */ 	beq         .strtoull_label_502
/* @504 */ 	lda          #255
.strtoull_label_502:
/* @505 */ 	sta         __i5+1
/* @508 */ 	ldx         __i5
/* @509 */ 	ldy         __i5+1
/* @511 */ 	jsr         __builtin_isalpha
/* @513 */ 	stz         __i6+1
/* @517 */ 	cmp          #0
/* @518 */ 	beq         .strtoull_label_563
/* @521 */ 	lda         __b0
/* @522 */ 	sta         __i5
/* @523 */ 	and          #128
/* @525 */ 	beq         .strtoull_label_524
/* @526 */ 	lda          #255
.strtoull_label_524:
/* @527 */ 	sta         __i5+1
/* @530 */ 	ldx         __i5
/* @531 */ 	ldy         __i5+1
/* @533 */ 	jsr         __builtin_toupper
/* @534 */ 	sta         __i6
/* @535 */ 	stz         __i6+1
/* @539 */ 	sec         
/* @541 */ 	sbc          #65
/* @542 */ 	sta         __i5
/* @543 */ 	lda         __i6+1
/* @544 */ 	sbc          #0
/* @545 */ 	sta         __i5+1
/* @549 */ 	clc         
/* @550 */ 	lda         __i5
/* @551 */ 	adc          #10
/* @552 */ 	sta         __i6
/* @553 */ 	lda         __i5+1
/* @554 */ 	adc          #0
/* @555 */ 	sta         __i6+1
/* @558 */ 	lda         __i6
/* @559 */ 	sta         __i4
/* @560 */ 	lda         __i6+1
/* @561 */ 	sta         __i4+1
/* @562 */ 	bra         .strtoull_label_612
.strtoull_label_563:
/* @566 */ 	lda         __b0
/* @567 */ 	sta         __i5
/* @568 */ 	and          #128
/* @570 */ 	beq         .strtoull_label_569
/* @571 */ 	lda          #255
.strtoull_label_569:
/* @572 */ 	sta         __i5+1
/* @575 */ 	ldx         __i5
/* @576 */ 	ldy         __i5+1
/* @578 */ 	jsr         __builtin_isdigit
/* @580 */ 	stz         __i6+1
/* @584 */ 	cmp          #0
/* @585 */ 	beq         .strtoull_label_611
/* @588 */ 	lda         __b0
/* @589 */ 	sta         __i5
/* @590 */ 	and          #128
/* @592 */ 	beq         .strtoull_label_591
/* @593 */ 	lda          #255
.strtoull_label_591:
/* @594 */ 	sta         __i5+1
/* @598 */ 	sec         
/* @599 */ 	lda         __i5
/* @600 */ 	sbc          #48
/* @601 */ 	sta         __i6
/* @602 */ 	lda         __i5+1
/* @603 */ 	sbc          #0
/* @604 */ 	sta         __i6+1
/* @607 */ 	lda         __i6
/* @608 */ 	sta         __i4
/* @609 */ 	lda         __i6+1
/* @610 */ 	sta         __i4+1
.strtoull_label_611:
.strtoull_label_612:
/* @614 */ 	lda         __i4
/* @615 */ 	cmp         __i3
/* @616 */ 	lda         __i4+1
/* @617 */ 	sbc         __i3+1
/* @618 */ 	bvc         .strtoull_label_613
/* @619 */ 	eor          #128
.strtoull_label_613:
/* @620 */ 	bmi         .strtoull_label_836
/* @837 */ 	jmp         .strtoull_label_757
.strtoull_label_836:
/* @623 */ 	lda         __i3
/* @624 */ 	sta         __x1
/* @625 */ 	lda         __i3+1
/* @626 */ 	sta         __x1+1
/* @627 */ 	lda          #0
/* @628 */ 	ldy          #7
.strtoull_label_629:
/* @630 */ 	sta         __x1, Y
/* @631 */ 	dey         
/* @632 */ 	cpy          #1
/* @633 */ 	bne         .strtoull_label_629
/* @638 */ 	lda          #__x2
/* @639 */ 	ldx          #__x0
/* @640 */ 	ldy          #__x1
/* @642 */ 	jsr         __umul8
/* @644 */ 	lda         __i4
/* @645 */ 	sta         __x1
/* @646 */ 	lda         __i4+1
/* @647 */ 	sta         __x1+1
/* @648 */ 	lda          #0
/* @649 */ 	ldy          #7
.strtoull_label_650:
/* @651 */ 	sta         __x1, Y
/* @652 */ 	dey         
/* @653 */ 	cpy          #1
/* @654 */ 	bne         .strtoull_label_650
/* @659 */ 	clc         
/* @661 */ 	ldy          #8
/* @662 */ 	ldx          #0
.strtoull_label_663:
/* @664 */ 	lda         __x2, X
/* @665 */ 	adc         __x1, X
/* @666 */ 	sta         __x3, X
/* @667 */ 	inx         
/* @668 */ 	dey         
/* @669 */ 	bne         .strtoull_label_663
/* @671 */ 	lda          #__x3
/* @673 */ 	ldx          #11
/* @675 */ 	jsr         __set_var_value8
/* @677 */ 	ldx          #11
	jsr          __var_value8_x1			// nresult
/* @680 */ 	lda         __x1+7
/* @681 */ 	cmp         __x0+7
/* @682 */ 	bcc         .strtoull_label_679
/* @683 */ 	bne         .strtoull_label_743
/* @685 */ 	lda         __x1+6
/* @686 */ 	cmp         __x0+6
/* @687 */ 	bcc         .strtoull_label_679
/* @688 */ 	bne         .strtoull_label_743
/* @690 */ 	lda         __x1+5
/* @691 */ 	cmp         __x0+5
/* @692 */ 	bcc         .strtoull_label_679
/* @693 */ 	bne         .strtoull_label_743
/* @695 */ 	lda         __x1+4
/* @696 */ 	cmp         __x0+4
/* @697 */ 	bcc         .strtoull_label_679
/* @698 */ 	bne         .strtoull_label_743
/* @700 */ 	lda         __x1+3
/* @701 */ 	cmp         __x0+3
/* @702 */ 	bcc         .strtoull_label_679
/* @703 */ 	bne         .strtoull_label_743
/* @705 */ 	lda         __x1+2
/* @706 */ 	cmp         __x0+2
/* @707 */ 	bcc         .strtoull_label_679
/* @708 */ 	bne         .strtoull_label_743
/* @709 */ 	lda         __x1+1
/* @710 */ 	cmp         __x0+1
/* @711 */ 	bcc         .strtoull_label_679
/* @712 */ 	bne         .strtoull_label_743
/* @713 */ 	lda         __x1
/* @714 */ 	cmp         __x0
/* @715 */ 	bcs         .strtoull_label_743
.strtoull_label_679:
/* @719 */ 	lda          #214
/* @720 */ 	sta         __i5
/* @721 */ 	lda          #3
/* @722 */ 	sta         __i5+1
/* @724 */ 	lda          #15
/* @725 */ 	ldy          #0
/* @726 */ 	sta         (__i5)
/* @727 */ 	tya         
/* @728 */ 	iny         
/* @729 */ 	sta         (__i5), Y
/* @733 */ 	ldx          #7
.strtoull_label_734:
/* @735 */ 	lda         .lit.18, X
/* @736 */ 	sta         __x1, X
/* @737 */ 	dex         
/* @738 */ 	bpl         .strtoull_label_734
/* @739 */ 	ldx          #16
	jsr          __load_result
/* @740 */ 	lda         #__x1
/* @741 */ 	jsr         __result8
/* @742 */ 	jmp         .strtoull_label_455
.strtoull_label_743:
/* @745 */ 	ldx          #11
	jsr          __var_value8_x1			// nresult
/* @748 */ 	ldx          #7
.strtoull_label_749:
/* @750 */ 	lda         __x1, X
/* @751 */ 	sta         __x0, X
/* @752 */ 	dex         
/* @753 */ 	bpl         .strtoull_label_749
/* @754 */ 	lda          #__i1
/* @755 */ 	jsr         __rinc21
/* @756 */ 	jmp         .strtoull_label_465
.strtoull_label_757:
/* @759 */ 	lda         __i1
/* @760 */ 	cmp         __i2
/* @761 */ 	bne         .strtoull_label_791
/* @762 */ 	lda         __i1+1
/* @763 */ 	cmp         __i2+1
/* @764 */ 	bne         .strtoull_label_791
/* @768 */ 	lda          #214
/* @769 */ 	sta         __i5
/* @770 */ 	lda          #3
/* @771 */ 	sta         __i5+1
/* @773 */ 	lda          #7
/* @774 */ 	ldy          #0
/* @775 */ 	sta         (__i5)
/* @776 */ 	tya         
/* @777 */ 	iny         
/* @778 */ 	sta         (__i5), Y
/* @781 */ 	ldx          #7
.strtoull_label_783:
/* @784 */ 	sta         __x1, X
/* @785 */ 	dex         
/* @786 */ 	bpl         .strtoull_label_783
/* @787 */ 	ldx          #16
	jsr          __load_result
/* @788 */ 	lda         #__x1
/* @789 */ 	jsr         __result8
/* @790 */ 	jmp         .strtoull_label_455
.strtoull_label_791:
/* @793 */ 	lda         __i0
/* @795 */ 	bne         .strtoull_label_792
/* @796 */ 	lda         __i0+1
/* @798 */ 	beq         .strtoull_label_805
.strtoull_label_792:
/* @800 */ 	lda         __i1
/* @801 */ 	sta         (__i0)
/* @802 */ 	lda         __i1+1
/* @803 */ 	ldy          #1
/* @804 */ 	sta         (__i0), Y
.strtoull_label_805:
/* @806 */ 	lda         __b1
/* @808 */ 	beq         .strtoull_label_829
/* @811 */ 	sec         
/* @812 */ 	ldy          #8
/* @813 */ 	ldx          #0
.strtoull_label_814:
/* @815 */ 	lda          #0
/* @816 */ 	sbc         __x0, X
/* @817 */ 	sta         __x1, X
/* @818 */ 	inx         
/* @819 */ 	dey         
/* @820 */ 	bne         .strtoull_label_814
/* @823 */ 	ldx          #7
.strtoull_label_824:
/* @825 */ 	lda         __x1, X
/* @826 */ 	sta         __x0, X
/* @827 */ 	dex         
/* @828 */ 	bpl         .strtoull_label_824
.strtoull_label_829:
/* @830 */ 	ldx          #16
	jsr          __load_result
/* @831 */ 	lda         #__x0
/* @832 */ 	jsr         __result8
/* @833 */ 	jmp         .strtoull_label_455
.func_end_strtoull:
	.size strtoull, .func_end_strtoull-strtoull

	.data
	.section ".rodata", "aMS", @progbits
.lit.18:
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.type .lit.18, @object
	.size .lit.18, 8

