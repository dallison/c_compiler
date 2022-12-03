	.file   "/Users/dallison/Google Drive/c_compiler/libc/strtoul.c"
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


	.global strtoul
	.type strtoul, @function

strtoul:
/* @29 */ 	stx         __result
/* @31 */ 	sty         __result+1
/* @32 */ 	ldx          #7
	jsr          __enter
	.byte        0xa3,0x06,0x00		// Save mask i:3 b:5 l:3 x:0 f:0 
/* @41 */ 	ldx          #2
	jsr          __arg_value2_i0			// end
/* @45 */ 	ldx          #0
	jsr          __arg_value2_i1			// str
/* @52 */ 	ldx          #4
	jsr          __arg_value2_i3			// base
/* @63 */ 	stz         __b1
.strtoul_label_64:
/* @69 */ 	lda         (__i1)
/* @75 */ 	sta         __i5
/* @77 */ 	and          #128
/* @79 */ 	beq         .strtoul_label_78
/* @81 */ 	lda          #255
.strtoul_label_78:
/* @82 */ 	sta         __i5+1
/* @87 */ 	ldx          #1
/* @88 */ 	lda         __i5
/* @89 */ 	ora         __i5+1
/* @91 */ 	bne         .strtoul_label_86
/* @92 */ 	dex         
.strtoul_label_86:
/* @93 */ 	stx         __b2
/* @95 */ 	txa         
/* @96 */ 	cmp          #0
/* @97 */ 	beq         .strtoul_label_126
/* @100 */ 	lda         (__i1)
/* @106 */ 	sta         __i5
/* @107 */ 	and          #128
/* @109 */ 	beq         .strtoul_label_108
/* @110 */ 	lda          #255
.strtoul_label_108:
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
.strtoul_label_126:
/* @128 */ 	lda         __b2
/* @130 */ 	beq         .strtoul_label_135
/* @131 */ 	lda          #__i1
/* @133 */ 	jsr         __rinc21
/* @134 */ 	bra         .strtoul_label_64
.strtoul_label_135:
/* @138 */ 	lda         (__i1)
/* @144 */ 	sta         __i5
/* @145 */ 	and          #128
/* @147 */ 	beq         .strtoul_label_146
/* @148 */ 	lda          #255
.strtoul_label_146:
/* @149 */ 	sta         __i5+1
/* @152 */ 	lda         __i5
/* @153 */ 	cmp          #45
/* @154 */ 	bne         .strtoul_label_164
/* @155 */ 	lda         __i5+1
/* @157 */ 	bne         .strtoul_label_164
/* @159 */ 	lda          #1
/* @160 */ 	sta         __b1
/* @161 */ 	lda          #__i1
/* @162 */ 	jsr         __rinc21
/* @163 */ 	bra         .strtoul_label_191
.strtoul_label_164:
/* @167 */ 	lda         (__i1)
/* @173 */ 	sta         __i5
/* @174 */ 	and          #128
/* @176 */ 	beq         .strtoul_label_175
/* @177 */ 	lda          #255
.strtoul_label_175:
/* @178 */ 	sta         __i5+1
/* @181 */ 	lda         __i5
/* @182 */ 	cmp          #43
/* @183 */ 	bne         .strtoul_label_190
/* @184 */ 	lda         __i5+1
/* @186 */ 	bne         .strtoul_label_190
/* @188 */ 	lda          #__i1
/* @189 */ 	jsr         __rinc21
.strtoul_label_190:
.strtoul_label_191:
/* @192 */ 	lda         __i1
/* @193 */ 	sta         __i2
/* @194 */ 	lda         __i1+1
/* @195 */ 	sta         __i2+1
/* @201 */ 	ldx          #1
/* @202 */ 	lda         __i3
/* @203 */ 	cmp          #16
/* @204 */ 	bne         .strtoul_label_199
/* @205 */ 	lda         __i3+1
/* @207 */ 	beq         .strtoul_label_200
.strtoul_label_199:
/* @208 */ 	dex         
.strtoul_label_200:
/* @209 */ 	stx         __b3
/* @211 */ 	txa         
/* @212 */ 	bne         .strtoul_label_223
/* @216 */ 	ldx          #1
/* @217 */ 	lda         __i3
/* @218 */ 	ora         __i3+1
/* @220 */ 	beq         .strtoul_label_215
/* @221 */ 	dex         
.strtoul_label_215:
/* @222 */ 	stx         __b3
.strtoul_label_223:
/* @225 */ 	lda         __b3
/* @227 */ 	beq         .strtoul_label_322
/* @232 */ 	lda         (__i1)
/* @238 */ 	sta         __i5
/* @239 */ 	and          #128
/* @241 */ 	beq         .strtoul_label_240
/* @242 */ 	lda          #255
.strtoul_label_240:
/* @243 */ 	sta         __i5+1
/* @248 */ 	ldx          #1
/* @249 */ 	lda         __i5
/* @250 */ 	cmp          #48
/* @251 */ 	bne         .strtoul_label_246
/* @252 */ 	lda         __i5+1
/* @254 */ 	beq         .strtoul_label_247
.strtoul_label_246:
/* @255 */ 	dex         
.strtoul_label_247:
/* @256 */ 	stx         __b4
/* @258 */ 	txa         
/* @259 */ 	cmp          #0
/* @260 */ 	beq         .strtoul_label_297
/* @263 */ 	ldy          #1
/* @264 */ 	lda         (__i1), Y
/* @270 */ 	sta         __i5
/* @271 */ 	and          #128
/* @273 */ 	beq         .strtoul_label_272
/* @274 */ 	lda          #255
.strtoul_label_272:
/* @275 */ 	sta         __i5+1
/* @278 */ 	ldx         __i5
/* @279 */ 	ldy         __i5+1
/* @281 */ 	jsr         __builtin_tolower
/* @282 */ 	sta         __i6
/* @283 */ 	stz         __i6+1
/* @288 */ 	ldx          #1
/* @290 */ 	cmp          #120
/* @291 */ 	bne         .strtoul_label_286
/* @292 */ 	lda         __i6+1
/* @294 */ 	beq         .strtoul_label_287
.strtoul_label_286:
/* @295 */ 	dex         
.strtoul_label_287:
/* @296 */ 	stx         __b4
.strtoul_label_297:
/* @299 */ 	lda         __b4
/* @301 */ 	beq         .strtoul_label_321
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
.strtoul_label_321:
.strtoul_label_322:
/* @328 */ 	ldx          #1
/* @329 */ 	lda         __i3
/* @330 */ 	cmp          #8
/* @331 */ 	bne         .strtoul_label_326
/* @332 */ 	lda         __i3+1
/* @334 */ 	beq         .strtoul_label_327
.strtoul_label_326:
/* @335 */ 	dex         
.strtoul_label_327:
/* @336 */ 	stx         __b4
/* @338 */ 	txa         
/* @339 */ 	bne         .strtoul_label_350
/* @343 */ 	ldx          #1
/* @344 */ 	lda         __i3
/* @345 */ 	ora         __i3+1
/* @347 */ 	beq         .strtoul_label_342
/* @348 */ 	dex         
.strtoul_label_342:
/* @349 */ 	stx         __b4
.strtoul_label_350:
/* @352 */ 	lda         __b4
/* @354 */ 	beq         .strtoul_label_383
/* @357 */ 	lda         (__i1)
/* @363 */ 	sta         __i5
/* @364 */ 	and          #128
/* @366 */ 	beq         .strtoul_label_365
/* @367 */ 	lda          #255
.strtoul_label_365:
/* @368 */ 	sta         __i5+1
/* @371 */ 	lda         __i5
/* @372 */ 	cmp          #48
/* @373 */ 	bne         .strtoul_label_382
/* @374 */ 	lda         __i5+1
/* @376 */ 	bne         .strtoul_label_382
/* @378 */ 	lda          #8
/* @379 */ 	sta         __i3
/* @381 */ 	stz         __i3+1
.strtoul_label_382:
.strtoul_label_383:
/* @384 */ 	lda         __i3
/* @385 */ 	ora         __i3+1
/* @386 */ 	bne         .strtoul_label_391
/* @387 */ 	lda          #10
/* @388 */ 	sta         __i3
/* @390 */ 	stz         __i3+1
.strtoul_label_391:
/* @396 */ 	ldx          #0
/* @398 */ 	lda         __i3
/* @399 */ 	cmp          #2
/* @400 */ 	lda         __i3+1
/* @401 */ 	sbc          #0
/* @402 */ 	bvc         .strtoul_label_397
/* @403 */ 	eor          #128
.strtoul_label_397:
/* @404 */ 	bpl         .strtoul_label_395
/* @405 */ 	inx         
.strtoul_label_395:
/* @406 */ 	stx         __b5
/* @408 */ 	txa         
/* @409 */ 	bne         .strtoul_label_423
/* @412 */ 	ldx          #0
/* @414 */ 	lda          #36
/* @415 */ 	cmp         __i3
/* @416 */ 	txa         
/* @417 */ 	sbc         __i3+1
/* @418 */ 	bvc         .strtoul_label_413
/* @419 */ 	eor          #128
.strtoul_label_413:
/* @420 */ 	bpl         .strtoul_label_411
/* @421 */ 	inx         
.strtoul_label_411:
/* @422 */ 	stx         __b5
.strtoul_label_423:
/* @425 */ 	lda         __b5
/* @427 */ 	beq         .strtoul_label_458
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
/* @444 */ 	ldx          #3
.strtoul_label_446:
/* @448 */ 	sta         __l2, X
/* @449 */ 	dex         
/* @450 */ 	bpl         .strtoul_label_446
/* @451 */ 	ldx          #8
	jsr          __load_result
/* @452 */ 	lda         #__l2
/* @454 */ 	jsr         __result4
.strtoul_label_455:
/* @456 */ 	ldy          #10
	jmp          __leave
.strtoul_label_458:
/* @459 */ 	ldx          #3
/* @460 */ 	lda          #0
.strtoul_label_461:
/* @462 */ 	sta         __l0, X
/* @463 */ 	dex         
/* @464 */ 	bpl         .strtoul_label_461
.strtoul_label_465:
/* @468 */ 	lda         (__i1)
/* @474 */ 	sta         __i5
/* @475 */ 	and          #128
/* @477 */ 	beq         .strtoul_label_476
/* @478 */ 	lda          #255
.strtoul_label_476:
/* @479 */ 	sta         __i5+1
/* @481 */ 	lda         __i5
/* @482 */ 	ora         __i5+1
/* @484 */ 	bne         .strtoul_label_809
/* @810 */ 	jmp         .strtoul_label_732
.strtoul_label_809:
/* @487 */ 	lda         (__i1)
/* @492 */ 	sta         __b0
/* @493 */ 	lda         __i3
/* @494 */ 	sta         __i4
/* @495 */ 	lda         __i3+1
/* @496 */ 	sta         __i4+1
/* @499 */ 	lda         __b0
/* @500 */ 	sta         __i5
/* @501 */ 	and          #128
/* @503 */ 	beq         .strtoul_label_502
/* @504 */ 	lda          #255
.strtoul_label_502:
/* @505 */ 	sta         __i5+1
/* @508 */ 	ldx         __i5
/* @509 */ 	ldy         __i5+1
/* @511 */ 	jsr         __builtin_isalpha
/* @513 */ 	stz         __i6+1
/* @517 */ 	cmp          #0
/* @518 */ 	beq         .strtoul_label_563
/* @521 */ 	lda         __b0
/* @522 */ 	sta         __i5
/* @523 */ 	and          #128
/* @525 */ 	beq         .strtoul_label_524
/* @526 */ 	lda          #255
.strtoul_label_524:
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
/* @562 */ 	bra         .strtoul_label_612
.strtoul_label_563:
/* @566 */ 	lda         __b0
/* @567 */ 	sta         __i5
/* @568 */ 	and          #128
/* @570 */ 	beq         .strtoul_label_569
/* @571 */ 	lda          #255
.strtoul_label_569:
/* @572 */ 	sta         __i5+1
/* @575 */ 	ldx         __i5
/* @576 */ 	ldy         __i5+1
/* @578 */ 	jsr         __builtin_isdigit
/* @580 */ 	stz         __i6+1
/* @584 */ 	cmp          #0
/* @585 */ 	beq         .strtoul_label_611
/* @588 */ 	lda         __b0
/* @589 */ 	sta         __i5
/* @590 */ 	and          #128
/* @592 */ 	beq         .strtoul_label_591
/* @593 */ 	lda          #255
.strtoul_label_591:
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
.strtoul_label_611:
.strtoul_label_612:
/* @614 */ 	lda         __i4
/* @615 */ 	cmp         __i3
/* @616 */ 	lda         __i4+1
/* @617 */ 	sbc         __i3+1
/* @618 */ 	bvc         .strtoul_label_613
/* @619 */ 	eor          #128
.strtoul_label_613:
/* @620 */ 	bmi         .strtoul_label_811
/* @812 */ 	jmp         .strtoul_label_732
.strtoul_label_811:
/* @624 */ 	lda         __i3
/* @625 */ 	sta         __l2
/* @626 */ 	lda         __i3+1
/* @627 */ 	sta         __l2+1
/* @628 */ 	and          #128
/* @630 */ 	beq         .strtoul_label_629
/* @631 */ 	lda          #255
.strtoul_label_629:
/* @633 */ 	sta         __l2+2
/* @634 */ 	sta         __l2+3
/* @639 */ 	lda          #__l3
/* @640 */ 	ldx          #__l0
/* @641 */ 	ldy          #__l2
/* @643 */ 	jsr         __umul4
/* @646 */ 	lda         __i4
/* @647 */ 	sta         __l2
/* @648 */ 	lda         __i4+1
/* @649 */ 	sta         __l2+1
/* @650 */ 	and          #128
/* @652 */ 	beq         .strtoul_label_651
/* @653 */ 	lda          #255
.strtoul_label_651:
/* @654 */ 	sta         __l2+2
/* @655 */ 	sta         __l2+3
/* @660 */ 	clc         
/* @662 */ 	ldy          #4
/* @663 */ 	ldx          #0
.strtoul_label_664:
/* @665 */ 	lda         __l3, X
/* @666 */ 	adc         __l2, X
/* @667 */ 	sta         __l4, X
/* @668 */ 	inx         
/* @669 */ 	dey         
/* @670 */ 	bne         .strtoul_label_664
/* @673 */ 	ldx          #3
.strtoul_label_674:
/* @675 */ 	lda         __l4, X
/* @676 */ 	sta         __l1, X
/* @677 */ 	dex         
/* @678 */ 	bpl         .strtoul_label_674
/* @680 */ 	lda         __l1+3
/* @681 */ 	cmp         __l0+3
/* @682 */ 	bcc         .strtoul_label_679
/* @683 */ 	bne         .strtoul_label_722
/* @684 */ 	lda         __l1+2
/* @685 */ 	cmp         __l0+2
/* @686 */ 	bcc         .strtoul_label_679
/* @687 */ 	bne         .strtoul_label_722
/* @688 */ 	lda         __l1+1
/* @689 */ 	cmp         __l0+1
/* @690 */ 	bcc         .strtoul_label_679
/* @691 */ 	bne         .strtoul_label_722
/* @692 */ 	lda         __l1
/* @693 */ 	cmp         __l0
/* @694 */ 	bcs         .strtoul_label_722
.strtoul_label_679:
/* @698 */ 	lda          #214
/* @699 */ 	sta         __i5
/* @700 */ 	lda          #3
/* @701 */ 	sta         __i5+1
/* @703 */ 	lda          #15
/* @704 */ 	ldy          #0
/* @705 */ 	sta         (__i5)
/* @706 */ 	tya         
/* @707 */ 	iny         
/* @708 */ 	sta         (__i5), Y
/* @712 */ 	ldx          #3
.strtoul_label_713:
/* @714 */ 	lda         .lit.18, X
/* @715 */ 	sta         __l2, X
/* @716 */ 	dex         
/* @717 */ 	bpl         .strtoul_label_713
/* @718 */ 	ldx          #8
	jsr          __load_result
/* @719 */ 	lda         #__l2
/* @720 */ 	jsr         __result4
/* @721 */ 	jmp         .strtoul_label_455
.strtoul_label_722:
/* @723 */ 	ldx          #3
.strtoul_label_724:
/* @725 */ 	lda         __l1, X
/* @726 */ 	sta         __l0, X
/* @727 */ 	dex         
/* @728 */ 	bpl         .strtoul_label_724
/* @729 */ 	lda          #__i1
/* @730 */ 	jsr         __rinc21
/* @731 */ 	jmp         .strtoul_label_465
.strtoul_label_732:
/* @734 */ 	lda         __i1
/* @735 */ 	cmp         __i2
/* @736 */ 	bne         .strtoul_label_766
/* @737 */ 	lda         __i1+1
/* @738 */ 	cmp         __i2+1
/* @739 */ 	bne         .strtoul_label_766
/* @743 */ 	lda          #214
/* @744 */ 	sta         __i5
/* @745 */ 	lda          #3
/* @746 */ 	sta         __i5+1
/* @748 */ 	lda          #7
/* @749 */ 	ldy          #0
/* @750 */ 	sta         (__i5)
/* @751 */ 	tya         
/* @752 */ 	iny         
/* @753 */ 	sta         (__i5), Y
/* @756 */ 	ldx          #3
.strtoul_label_758:
/* @759 */ 	sta         __l2, X
/* @760 */ 	dex         
/* @761 */ 	bpl         .strtoul_label_758
/* @762 */ 	ldx          #8
	jsr          __load_result
/* @763 */ 	lda         #__l2
/* @764 */ 	jsr         __result4
/* @765 */ 	jmp         .strtoul_label_455
.strtoul_label_766:
/* @768 */ 	lda         __i0
/* @770 */ 	bne         .strtoul_label_767
/* @771 */ 	lda         __i0+1
/* @773 */ 	beq         .strtoul_label_780
.strtoul_label_767:
/* @775 */ 	lda         __i1
/* @776 */ 	sta         (__i0)
/* @777 */ 	lda         __i1+1
/* @778 */ 	ldy          #1
/* @779 */ 	sta         (__i0), Y
.strtoul_label_780:
/* @781 */ 	lda         __b1
/* @783 */ 	beq         .strtoul_label_804
/* @786 */ 	sec         
/* @787 */ 	ldy          #4
/* @788 */ 	ldx          #0
.strtoul_label_789:
/* @790 */ 	lda          #0
/* @791 */ 	sbc         __l0, X
/* @792 */ 	sta         __l2, X
/* @793 */ 	inx         
/* @794 */ 	dey         
/* @795 */ 	bne         .strtoul_label_789
/* @798 */ 	ldx          #3
.strtoul_label_799:
/* @800 */ 	lda         __l2, X
/* @801 */ 	sta         __l0, X
/* @802 */ 	dex         
/* @803 */ 	bpl         .strtoul_label_799
.strtoul_label_804:
/* @805 */ 	ldx          #8
	jsr          __load_result
/* @806 */ 	lda         #__l0
/* @807 */ 	jsr         __result4
/* @808 */ 	jmp         .strtoul_label_455
.func_end_strtoul:
	.size strtoul, .func_end_strtoul-strtoul

	.data
	.section ".rodata", "aMS", @progbits
.lit.18:
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.type .lit.18, @object
	.size .lit.18, 4

