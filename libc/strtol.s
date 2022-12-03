	.file   "/Users/dallison/Google Drive/c_compiler/libc/strtol.c"
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


	.global strtol
	.type strtol, @function

strtol:
/* @32 */ 	stx         __result
/* @34 */ 	sty         __result+1
/* @35 */ 	ldx          #7
	jsr          __enter
	.byte        0xa3,0x06,0x00		// Save mask i:3 b:5 l:3 x:0 f:0 
/* @44 */ 	ldx          #2
	jsr          __arg_value2_i0			// end
/* @48 */ 	ldx          #0
	jsr          __arg_value2_i1			// str
/* @55 */ 	ldx          #4
	jsr          __arg_value2_i3			// base
/* @64 */ 	stz         __b1
.strtol_label_65:
/* @70 */ 	lda         (__i1)
/* @76 */ 	sta         __i5
/* @78 */ 	and          #128
/* @80 */ 	beq         .strtol_label_79
/* @82 */ 	lda          #255
.strtol_label_79:
/* @83 */ 	sta         __i5+1
/* @88 */ 	ldx          #1
/* @89 */ 	lda         __i5
/* @90 */ 	ora         __i5+1
/* @92 */ 	bne         .strtol_label_87
/* @93 */ 	dex         
.strtol_label_87:
/* @94 */ 	stx         __b2
/* @96 */ 	txa         
/* @97 */ 	cmp          #0
/* @98 */ 	beq         .strtol_label_127
/* @101 */ 	lda         (__i1)
/* @107 */ 	sta         __i5
/* @108 */ 	and          #128
/* @110 */ 	beq         .strtol_label_109
/* @111 */ 	lda          #255
.strtol_label_109:
/* @112 */ 	sta         __i5+1
/* @115 */ 	ldx         __i5
/* @116 */ 	ldy         __i5+1
/* @118 */ 	jsr         __builtin_isspace
/* @119 */ 	sta         __i6
/* @120 */ 	stz         __i6+1
/* @123 */ 	lda         __i6+1
/* @124 */ 	and          #128
/* @125 */ 	ora         __i6
/* @126 */ 	sta         __b2
.strtol_label_127:
/* @129 */ 	lda         __b2
/* @131 */ 	beq         .strtol_label_136
/* @132 */ 	lda          #__i1
/* @134 */ 	jsr         __rinc21
/* @135 */ 	bra         .strtol_label_65
.strtol_label_136:
/* @139 */ 	lda         (__i1)
/* @145 */ 	sta         __i5
/* @146 */ 	and          #128
/* @148 */ 	beq         .strtol_label_147
/* @149 */ 	lda          #255
.strtol_label_147:
/* @150 */ 	sta         __i5+1
/* @153 */ 	lda         __i5
/* @154 */ 	cmp          #45
/* @155 */ 	bne         .strtol_label_165
/* @156 */ 	lda         __i5+1
/* @158 */ 	bne         .strtol_label_165
/* @160 */ 	lda          #1
/* @161 */ 	sta         __b1
/* @162 */ 	lda          #__i1
/* @163 */ 	jsr         __rinc21
/* @164 */ 	bra         .strtol_label_192
.strtol_label_165:
/* @168 */ 	lda         (__i1)
/* @174 */ 	sta         __i5
/* @175 */ 	and          #128
/* @177 */ 	beq         .strtol_label_176
/* @178 */ 	lda          #255
.strtol_label_176:
/* @179 */ 	sta         __i5+1
/* @182 */ 	lda         __i5
/* @183 */ 	cmp          #43
/* @184 */ 	bne         .strtol_label_191
/* @185 */ 	lda         __i5+1
/* @187 */ 	bne         .strtol_label_191
/* @189 */ 	lda          #__i1
/* @190 */ 	jsr         __rinc21
.strtol_label_191:
.strtol_label_192:
/* @193 */ 	lda         __i1
/* @194 */ 	sta         __i2
/* @195 */ 	lda         __i1+1
/* @196 */ 	sta         __i2+1
/* @202 */ 	ldx          #1
/* @203 */ 	lda         __i3
/* @204 */ 	cmp          #16
/* @205 */ 	bne         .strtol_label_200
/* @206 */ 	lda         __i3+1
/* @208 */ 	beq         .strtol_label_201
.strtol_label_200:
/* @209 */ 	dex         
.strtol_label_201:
/* @210 */ 	stx         __b3
/* @212 */ 	txa         
/* @213 */ 	bne         .strtol_label_224
/* @217 */ 	ldx          #1
/* @218 */ 	lda         __i3
/* @219 */ 	ora         __i3+1
/* @221 */ 	beq         .strtol_label_216
/* @222 */ 	dex         
.strtol_label_216:
/* @223 */ 	stx         __b3
.strtol_label_224:
/* @226 */ 	lda         __b3
/* @228 */ 	beq         .strtol_label_323
/* @233 */ 	lda         (__i1)
/* @239 */ 	sta         __i5
/* @240 */ 	and          #128
/* @242 */ 	beq         .strtol_label_241
/* @243 */ 	lda          #255
.strtol_label_241:
/* @244 */ 	sta         __i5+1
/* @249 */ 	ldx          #1
/* @250 */ 	lda         __i5
/* @251 */ 	cmp          #48
/* @252 */ 	bne         .strtol_label_247
/* @253 */ 	lda         __i5+1
/* @255 */ 	beq         .strtol_label_248
.strtol_label_247:
/* @256 */ 	dex         
.strtol_label_248:
/* @257 */ 	stx         __b4
/* @259 */ 	txa         
/* @260 */ 	cmp          #0
/* @261 */ 	beq         .strtol_label_298
/* @264 */ 	ldy          #1
/* @265 */ 	lda         (__i1), Y
/* @271 */ 	sta         __i5
/* @272 */ 	and          #128
/* @274 */ 	beq         .strtol_label_273
/* @275 */ 	lda          #255
.strtol_label_273:
/* @276 */ 	sta         __i5+1
/* @279 */ 	ldx         __i5
/* @280 */ 	ldy         __i5+1
/* @282 */ 	jsr         __builtin_tolower
/* @283 */ 	sta         __i6
/* @284 */ 	stz         __i6+1
/* @289 */ 	ldx          #1
/* @291 */ 	cmp          #120
/* @292 */ 	bne         .strtol_label_287
/* @293 */ 	lda         __i6+1
/* @295 */ 	beq         .strtol_label_288
.strtol_label_287:
/* @296 */ 	dex         
.strtol_label_288:
/* @297 */ 	stx         __b4
.strtol_label_298:
/* @300 */ 	lda         __b4
/* @302 */ 	beq         .strtol_label_322
/* @305 */ 	clc         
/* @306 */ 	lda         __i1
/* @307 */ 	adc          #2
/* @308 */ 	sta         __i5
/* @309 */ 	lda         __i1+1
/* @310 */ 	adc          #0
/* @311 */ 	sta         __i5+1
/* @314 */ 	lda         __i5
/* @315 */ 	sta         __i1
/* @316 */ 	lda         __i5+1
/* @317 */ 	sta         __i1+1
/* @318 */ 	lda          #16
/* @319 */ 	sta         __i3
/* @321 */ 	stz         __i3+1
.strtol_label_322:
.strtol_label_323:
/* @329 */ 	ldx          #1
/* @330 */ 	lda         __i3
/* @331 */ 	cmp          #8
/* @332 */ 	bne         .strtol_label_327
/* @333 */ 	lda         __i3+1
/* @335 */ 	beq         .strtol_label_328
.strtol_label_327:
/* @336 */ 	dex         
.strtol_label_328:
/* @337 */ 	stx         __b4
/* @339 */ 	txa         
/* @340 */ 	bne         .strtol_label_351
/* @344 */ 	ldx          #1
/* @345 */ 	lda         __i3
/* @346 */ 	ora         __i3+1
/* @348 */ 	beq         .strtol_label_343
/* @349 */ 	dex         
.strtol_label_343:
/* @350 */ 	stx         __b4
.strtol_label_351:
/* @353 */ 	lda         __b4
/* @355 */ 	beq         .strtol_label_384
/* @358 */ 	lda         (__i1)
/* @364 */ 	sta         __i5
/* @365 */ 	and          #128
/* @367 */ 	beq         .strtol_label_366
/* @368 */ 	lda          #255
.strtol_label_366:
/* @369 */ 	sta         __i5+1
/* @372 */ 	lda         __i5
/* @373 */ 	cmp          #48
/* @374 */ 	bne         .strtol_label_383
/* @375 */ 	lda         __i5+1
/* @377 */ 	bne         .strtol_label_383
/* @379 */ 	lda          #8
/* @380 */ 	sta         __i3
/* @382 */ 	stz         __i3+1
.strtol_label_383:
.strtol_label_384:
/* @385 */ 	lda         __i3
/* @386 */ 	ora         __i3+1
/* @387 */ 	bne         .strtol_label_392
/* @388 */ 	lda          #10
/* @389 */ 	sta         __i3
/* @391 */ 	stz         __i3+1
.strtol_label_392:
/* @397 */ 	ldx          #0
/* @399 */ 	lda         __i3
/* @400 */ 	cmp          #2
/* @401 */ 	lda         __i3+1
/* @402 */ 	sbc          #0
/* @403 */ 	bvc         .strtol_label_398
/* @404 */ 	eor          #128
.strtol_label_398:
/* @405 */ 	bpl         .strtol_label_396
/* @406 */ 	inx         
.strtol_label_396:
/* @407 */ 	stx         __b5
/* @409 */ 	txa         
/* @410 */ 	bne         .strtol_label_424
/* @413 */ 	ldx          #0
/* @415 */ 	lda          #36
/* @416 */ 	cmp         __i3
/* @417 */ 	txa         
/* @418 */ 	sbc         __i3+1
/* @419 */ 	bvc         .strtol_label_414
/* @420 */ 	eor          #128
.strtol_label_414:
/* @421 */ 	bpl         .strtol_label_412
/* @422 */ 	inx         
.strtol_label_412:
/* @423 */ 	stx         __b5
.strtol_label_424:
/* @426 */ 	lda         __b5
/* @428 */ 	beq         .strtol_label_459
/* @431 */ 	lda          #214
/* @432 */ 	sta         __i5
/* @433 */ 	lda          #3
/* @434 */ 	sta         __i5+1
/* @436 */ 	lda          #7
/* @437 */ 	ldy          #0
/* @438 */ 	sta         (__i5)
/* @439 */ 	tya         
/* @440 */ 	iny         
/* @441 */ 	sta         (__i5), Y
/* @445 */ 	ldx          #3
.strtol_label_447:
/* @449 */ 	sta         __l2, X
/* @450 */ 	dex         
/* @451 */ 	bpl         .strtol_label_447
/* @452 */ 	ldx          #8
	jsr          __load_result
/* @453 */ 	lda         #__l2
/* @455 */ 	jsr         __result4
.strtol_label_456:
/* @457 */ 	ldy          #10
	jmp          __leave
.strtol_label_459:
/* @460 */ 	ldx          #3
/* @461 */ 	lda          #0
.strtol_label_462:
/* @463 */ 	sta         __l1, X
/* @464 */ 	dex         
/* @465 */ 	bpl         .strtol_label_462
.strtol_label_466:
/* @469 */ 	lda         (__i1)
/* @475 */ 	sta         __i5
/* @476 */ 	and          #128
/* @478 */ 	beq         .strtol_label_477
/* @479 */ 	lda          #255
.strtol_label_477:
/* @480 */ 	sta         __i5+1
/* @482 */ 	lda         __i5
/* @483 */ 	ora         __i5+1
/* @485 */ 	bne         .strtol_label_811
/* @812 */ 	jmp         .strtol_label_683
.strtol_label_811:
/* @488 */ 	lda         (__i1)
/* @493 */ 	sta         __b0
/* @494 */ 	lda         __i3
/* @495 */ 	sta         __i4
/* @496 */ 	lda         __i3+1
/* @497 */ 	sta         __i4+1
/* @500 */ 	lda         __b0
/* @501 */ 	sta         __i5
/* @502 */ 	and          #128
/* @504 */ 	beq         .strtol_label_503
/* @505 */ 	lda          #255
.strtol_label_503:
/* @506 */ 	sta         __i5+1
/* @509 */ 	ldx         __i5
/* @510 */ 	ldy         __i5+1
/* @512 */ 	jsr         __builtin_isalpha
/* @514 */ 	stz         __i6+1
/* @518 */ 	cmp          #0
/* @519 */ 	beq         .strtol_label_564
/* @522 */ 	lda         __b0
/* @523 */ 	sta         __i5
/* @524 */ 	and          #128
/* @526 */ 	beq         .strtol_label_525
/* @527 */ 	lda          #255
.strtol_label_525:
/* @528 */ 	sta         __i5+1
/* @531 */ 	ldx         __i5
/* @532 */ 	ldy         __i5+1
/* @534 */ 	jsr         __builtin_toupper
/* @535 */ 	sta         __i6
/* @536 */ 	stz         __i6+1
/* @540 */ 	sec         
/* @542 */ 	sbc          #65
/* @543 */ 	sta         __i5
/* @544 */ 	lda         __i6+1
/* @545 */ 	sbc          #0
/* @546 */ 	sta         __i5+1
/* @550 */ 	clc         
/* @551 */ 	lda         __i5
/* @552 */ 	adc          #10
/* @553 */ 	sta         __i6
/* @554 */ 	lda         __i5+1
/* @555 */ 	adc          #0
/* @556 */ 	sta         __i6+1
/* @559 */ 	lda         __i6
/* @560 */ 	sta         __i4
/* @561 */ 	lda         __i6+1
/* @562 */ 	sta         __i4+1
/* @563 */ 	bra         .strtol_label_613
.strtol_label_564:
/* @567 */ 	lda         __b0
/* @568 */ 	sta         __i5
/* @569 */ 	and          #128
/* @571 */ 	beq         .strtol_label_570
/* @572 */ 	lda          #255
.strtol_label_570:
/* @573 */ 	sta         __i5+1
/* @576 */ 	ldx         __i5
/* @577 */ 	ldy         __i5+1
/* @579 */ 	jsr         __builtin_isdigit
/* @581 */ 	stz         __i6+1
/* @585 */ 	cmp          #0
/* @586 */ 	beq         .strtol_label_612
/* @589 */ 	lda         __b0
/* @590 */ 	sta         __i5
/* @591 */ 	and          #128
/* @593 */ 	beq         .strtol_label_592
/* @594 */ 	lda          #255
.strtol_label_592:
/* @595 */ 	sta         __i5+1
/* @599 */ 	sec         
/* @600 */ 	lda         __i5
/* @601 */ 	sbc          #48
/* @602 */ 	sta         __i6
/* @603 */ 	lda         __i5+1
/* @604 */ 	sbc          #0
/* @605 */ 	sta         __i6+1
/* @608 */ 	lda         __i6
/* @609 */ 	sta         __i4
/* @610 */ 	lda         __i6+1
/* @611 */ 	sta         __i4+1
.strtol_label_612:
.strtol_label_613:
/* @615 */ 	lda         __i4
/* @616 */ 	cmp         __i3
/* @617 */ 	lda         __i4+1
/* @618 */ 	sbc         __i3+1
/* @619 */ 	bvc         .strtol_label_614
/* @620 */ 	eor          #128
.strtol_label_614:
/* @621 */ 	bpl         .strtol_label_683
/* @625 */ 	lda         __i3
/* @626 */ 	sta         __l2
/* @627 */ 	lda         __i3+1
/* @628 */ 	sta         __l2+1
/* @629 */ 	and          #128
/* @631 */ 	beq         .strtol_label_630
/* @632 */ 	lda          #255
.strtol_label_630:
/* @634 */ 	sta         __l2+2
/* @635 */ 	sta         __l2+3
/* @640 */ 	lda          #__l3
/* @641 */ 	ldx          #__l1
/* @642 */ 	ldy          #__l2
/* @644 */ 	jsr         __smul4
/* @647 */ 	lda         __i4
/* @648 */ 	sta         __l2
/* @649 */ 	lda         __i4+1
/* @650 */ 	sta         __l2+1
/* @651 */ 	and          #128
/* @653 */ 	beq         .strtol_label_652
/* @654 */ 	lda          #255
.strtol_label_652:
/* @655 */ 	sta         __l2+2
/* @656 */ 	sta         __l2+3
/* @661 */ 	clc         
/* @663 */ 	ldy          #4
/* @664 */ 	ldx          #0
.strtol_label_665:
/* @666 */ 	lda         __l3, X
/* @667 */ 	adc         __l2, X
/* @668 */ 	sta         __l4, X
/* @669 */ 	inx         
/* @670 */ 	dey         
/* @671 */ 	bne         .strtol_label_665
/* @674 */ 	ldx          #3
.strtol_label_675:
/* @676 */ 	lda         __l4, X
/* @677 */ 	sta         __l1, X
/* @678 */ 	dex         
/* @679 */ 	bpl         .strtol_label_675
/* @680 */ 	lda          #__i1
/* @681 */ 	jsr         __rinc21
/* @682 */ 	jmp         .strtol_label_466
.strtol_label_683:
/* @685 */ 	lda         __i1
/* @686 */ 	cmp         __i2
/* @687 */ 	bne         .strtol_label_717
/* @688 */ 	lda         __i1+1
/* @689 */ 	cmp         __i2+1
/* @690 */ 	bne         .strtol_label_717
/* @694 */ 	lda          #214
/* @695 */ 	sta         __i5
/* @696 */ 	lda          #3
/* @697 */ 	sta         __i5+1
/* @699 */ 	lda          #7
/* @700 */ 	ldy          #0
/* @701 */ 	sta         (__i5)
/* @702 */ 	tya         
/* @703 */ 	iny         
/* @704 */ 	sta         (__i5), Y
/* @707 */ 	ldx          #3
.strtol_label_709:
/* @710 */ 	sta         __l2, X
/* @711 */ 	dex         
/* @712 */ 	bpl         .strtol_label_709
/* @713 */ 	ldx          #8
	jsr          __load_result
/* @714 */ 	lda         #__l2
/* @715 */ 	jsr         __result4
/* @716 */ 	jmp         .strtol_label_456
.strtol_label_717:
/* @719 */ 	lda         __i0
/* @721 */ 	bne         .strtol_label_718
/* @722 */ 	lda         __i0+1
/* @724 */ 	beq         .strtol_label_731
.strtol_label_718:
/* @726 */ 	lda         __i1
/* @727 */ 	sta         (__i0)
/* @728 */ 	lda         __i1+1
/* @729 */ 	ldy          #1
/* @730 */ 	sta         (__i0), Y
.strtol_label_731:
/* @733 */ 	lda         __l1+3
/* @734 */ 	bpl         .strtol_label_782
/* @735 */ 	lda         __b1
/* @737 */ 	beq         .strtol_label_747
/* @740 */ 	ldx          #3
.strtol_label_741:
/* @742 */ 	lda         .lit.17, X
/* @743 */ 	sta         __l0, X
/* @744 */ 	dex         
/* @745 */ 	bpl         .strtol_label_741
/* @746 */ 	bra         .strtol_label_756
.strtol_label_747:
/* @750 */ 	ldx          #3
.strtol_label_751:
/* @752 */ 	lda         .lit.18, X
/* @753 */ 	sta         __l0, X
/* @754 */ 	dex         
/* @755 */ 	bpl         .strtol_label_751
.strtol_label_756:
/* @759 */ 	ldx          #3
.strtol_label_760:
/* @761 */ 	lda         __l0, X
/* @762 */ 	sta         __l1, X
/* @763 */ 	dex         
/* @764 */ 	bpl         .strtol_label_760
/* @767 */ 	lda          #214
/* @768 */ 	sta         __i5
/* @769 */ 	lda          #3
/* @770 */ 	sta         __i5+1
/* @772 */ 	lda          #15
/* @773 */ 	ldy          #0
/* @774 */ 	sta         (__i5)
/* @775 */ 	tya         
/* @776 */ 	iny         
/* @777 */ 	sta         (__i5), Y
/* @778 */ 	ldx          #8
	jsr          __load_result
/* @779 */ 	lda         #__l1
/* @780 */ 	jsr         __result4
/* @781 */ 	jmp         .strtol_label_456
.strtol_label_782:
/* @783 */ 	lda         __b1
/* @785 */ 	beq         .strtol_label_806
/* @788 */ 	sec         
/* @789 */ 	ldy          #4
/* @790 */ 	ldx          #0
.strtol_label_791:
/* @792 */ 	lda          #0
/* @793 */ 	sbc         __l1, X
/* @794 */ 	sta         __l2, X
/* @795 */ 	inx         
/* @796 */ 	dey         
/* @797 */ 	bne         .strtol_label_791
/* @800 */ 	ldx          #3
.strtol_label_801:
/* @802 */ 	lda         __l2, X
/* @803 */ 	sta         __l1, X
/* @804 */ 	dex         
/* @805 */ 	bpl         .strtol_label_801
.strtol_label_806:
/* @807 */ 	ldx          #8
	jsr          __load_result
/* @808 */ 	lda         #__l1
/* @809 */ 	jsr         __result4
/* @810 */ 	jmp         .strtol_label_456
.func_end_strtol:
	.size strtol, .func_end_strtol-strtol

	.global atoi
	.type atoi, @function

atoi:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x02,0x00		// Save mask i:0 b:0 l:1 x:0 f:0 
/* @15 */ 	ldx          #0
	jsr          __arg_value2_i0			// s
/* @17 */ 	ldx          #10
/* @19 */ 	jsr         __pushxy0
/* @23 */ 	stz         __i1
/* @25 */ 	stz         __i1+1
/* @27 */ 	jsr         __pushi1
/* @28 */ 	jsr         __pushi0
/* @30 */ 	ldx         #__l2
/* @31 */ 	ldy          #0
/* @32 */ 	jsr         strtol
/* @34 */ 	jsr         __incsp6
/* @38 */ 	lda         __l2
/* @39 */ 	sta         __i0
/* @41 */ 	lda         __l2+3
/* @43 */ 	and          #128
/* @44 */ 	ora         __l2+1
/* @45 */ 	sta         __i0+1
/* @47 */ 	ldx          #8
	jsr          __load_result
/* @48 */ 	lda         #__i0
/* @50 */ 	jsr         __result2
/* @52 */ 	ldy          #10
	jmp          __leave
.func_end_atoi:
	.size atoi, .func_end_atoi-atoi

	.global atol
	.type atol, @function

atol:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x02,0x00		// Save mask i:0 b:0 l:1 x:0 f:0 
/* @14 */ 	ldx          #0
	jsr          __arg_value2_i0			// s
/* @16 */ 	ldx          #10
/* @18 */ 	jsr         __pushxy0
/* @22 */ 	stz         __i1
/* @24 */ 	stz         __i1+1
/* @26 */ 	jsr         __pushi1
/* @27 */ 	jsr         __pushi0
/* @29 */ 	ldx         #__l2
/* @30 */ 	ldy          #0
/* @31 */ 	jsr         strtol
/* @33 */ 	jsr         __incsp6
/* @35 */ 	ldx          #8
	jsr          __load_result
/* @36 */ 	lda         #__l2
/* @38 */ 	jsr         __result4
/* @40 */ 	ldy          #10
	jmp          __leave
.func_end_atol:
	.size atol, .func_end_atol-atol

	.data
	.section ".rodata", "aMS", @progbits
.lit.17:
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x80
	.type .lit.17, @object
	.size .lit.17, 4

.lit.18:
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.byte 0x7f
	.type .lit.18, @object
	.size .lit.18, 4

