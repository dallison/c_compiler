	.file   "/Users/dallison/Google Drive/c_compiler/libc/strtoll.c"
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


	.global strtoll
	.type strtoll, @function

strtoll:
/* @32 */ 	stx         __result
/* @34 */ 	sty         __result+1
/* @35 */ 	ldx          #15
	jsr          __enter
	.byte        0xa3,0x60,0x00		// Save mask i:3 b:5 l:0 x:3 f:0 
/* @44 */ 	ldx          #2
	jsr          __arg_value2_i0			// end
/* @48 */ 	ldx          #0
	jsr          __arg_value2_i1			// str
/* @55 */ 	ldx          #4
	jsr          __arg_value2_i3			// base
/* @64 */ 	stz         __b1
.strtoll_label_65:
/* @70 */ 	lda         (__i1)
/* @76 */ 	sta         __i5
/* @78 */ 	and          #128
/* @80 */ 	beq         .strtoll_label_79
/* @82 */ 	lda          #255
.strtoll_label_79:
/* @83 */ 	sta         __i5+1
/* @88 */ 	ldx          #1
/* @89 */ 	lda         __i5
/* @90 */ 	ora         __i5+1
/* @92 */ 	bne         .strtoll_label_87
/* @93 */ 	dex         
.strtoll_label_87:
/* @94 */ 	stx         __b2
/* @96 */ 	txa         
/* @97 */ 	cmp          #0
/* @98 */ 	beq         .strtoll_label_127
/* @101 */ 	lda         (__i1)
/* @107 */ 	sta         __i5
/* @108 */ 	and          #128
/* @110 */ 	beq         .strtoll_label_109
/* @111 */ 	lda          #255
.strtoll_label_109:
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
.strtoll_label_127:
/* @129 */ 	lda         __b2
/* @131 */ 	beq         .strtoll_label_136
/* @132 */ 	lda          #__i1
/* @134 */ 	jsr         __rinc21
/* @135 */ 	bra         .strtoll_label_65
.strtoll_label_136:
/* @139 */ 	lda         (__i1)
/* @145 */ 	sta         __i5
/* @146 */ 	and          #128
/* @148 */ 	beq         .strtoll_label_147
/* @149 */ 	lda          #255
.strtoll_label_147:
/* @150 */ 	sta         __i5+1
/* @153 */ 	lda         __i5
/* @154 */ 	cmp          #45
/* @155 */ 	bne         .strtoll_label_165
/* @156 */ 	lda         __i5+1
/* @158 */ 	bne         .strtoll_label_165
/* @160 */ 	lda          #1
/* @161 */ 	sta         __b1
/* @162 */ 	lda          #__i1
/* @163 */ 	jsr         __rinc21
/* @164 */ 	bra         .strtoll_label_192
.strtoll_label_165:
/* @168 */ 	lda         (__i1)
/* @174 */ 	sta         __i5
/* @175 */ 	and          #128
/* @177 */ 	beq         .strtoll_label_176
/* @178 */ 	lda          #255
.strtoll_label_176:
/* @179 */ 	sta         __i5+1
/* @182 */ 	lda         __i5
/* @183 */ 	cmp          #43
/* @184 */ 	bne         .strtoll_label_191
/* @185 */ 	lda         __i5+1
/* @187 */ 	bne         .strtoll_label_191
/* @189 */ 	lda          #__i1
/* @190 */ 	jsr         __rinc21
.strtoll_label_191:
.strtoll_label_192:
/* @193 */ 	lda         __i1
/* @194 */ 	sta         __i2
/* @195 */ 	lda         __i1+1
/* @196 */ 	sta         __i2+1
/* @202 */ 	ldx          #1
/* @203 */ 	lda         __i3
/* @204 */ 	cmp          #16
/* @205 */ 	bne         .strtoll_label_200
/* @206 */ 	lda         __i3+1
/* @208 */ 	beq         .strtoll_label_201
.strtoll_label_200:
/* @209 */ 	dex         
.strtoll_label_201:
/* @210 */ 	stx         __b3
/* @212 */ 	txa         
/* @213 */ 	bne         .strtoll_label_224
/* @217 */ 	ldx          #1
/* @218 */ 	lda         __i3
/* @219 */ 	ora         __i3+1
/* @221 */ 	beq         .strtoll_label_216
/* @222 */ 	dex         
.strtoll_label_216:
/* @223 */ 	stx         __b3
.strtoll_label_224:
/* @226 */ 	lda         __b3
/* @228 */ 	beq         .strtoll_label_323
/* @233 */ 	lda         (__i1)
/* @239 */ 	sta         __i5
/* @240 */ 	and          #128
/* @242 */ 	beq         .strtoll_label_241
/* @243 */ 	lda          #255
.strtoll_label_241:
/* @244 */ 	sta         __i5+1
/* @249 */ 	ldx          #1
/* @250 */ 	lda         __i5
/* @251 */ 	cmp          #48
/* @252 */ 	bne         .strtoll_label_247
/* @253 */ 	lda         __i5+1
/* @255 */ 	beq         .strtoll_label_248
.strtoll_label_247:
/* @256 */ 	dex         
.strtoll_label_248:
/* @257 */ 	stx         __b4
/* @259 */ 	txa         
/* @260 */ 	cmp          #0
/* @261 */ 	beq         .strtoll_label_298
/* @264 */ 	ldy          #1
/* @265 */ 	lda         (__i1), Y
/* @271 */ 	sta         __i5
/* @272 */ 	and          #128
/* @274 */ 	beq         .strtoll_label_273
/* @275 */ 	lda          #255
.strtoll_label_273:
/* @276 */ 	sta         __i5+1
/* @279 */ 	ldx         __i5
/* @280 */ 	ldy         __i5+1
/* @282 */ 	jsr         __builtin_tolower
/* @283 */ 	sta         __i6
/* @284 */ 	stz         __i6+1
/* @289 */ 	ldx          #1
/* @291 */ 	cmp          #120
/* @292 */ 	bne         .strtoll_label_287
/* @293 */ 	lda         __i6+1
/* @295 */ 	beq         .strtoll_label_288
.strtoll_label_287:
/* @296 */ 	dex         
.strtoll_label_288:
/* @297 */ 	stx         __b4
.strtoll_label_298:
/* @300 */ 	lda         __b4
/* @302 */ 	beq         .strtoll_label_322
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
.strtoll_label_322:
.strtoll_label_323:
/* @329 */ 	ldx          #1
/* @330 */ 	lda         __i3
/* @331 */ 	cmp          #8
/* @332 */ 	bne         .strtoll_label_327
/* @333 */ 	lda         __i3+1
/* @335 */ 	beq         .strtoll_label_328
.strtoll_label_327:
/* @336 */ 	dex         
.strtoll_label_328:
/* @337 */ 	stx         __b4
/* @339 */ 	txa         
/* @340 */ 	bne         .strtoll_label_351
/* @344 */ 	ldx          #1
/* @345 */ 	lda         __i3
/* @346 */ 	ora         __i3+1
/* @348 */ 	beq         .strtoll_label_343
/* @349 */ 	dex         
.strtoll_label_343:
/* @350 */ 	stx         __b4
.strtoll_label_351:
/* @353 */ 	lda         __b4
/* @355 */ 	beq         .strtoll_label_384
/* @358 */ 	lda         (__i1)
/* @364 */ 	sta         __i5
/* @365 */ 	and          #128
/* @367 */ 	beq         .strtoll_label_366
/* @368 */ 	lda          #255
.strtoll_label_366:
/* @369 */ 	sta         __i5+1
/* @372 */ 	lda         __i5
/* @373 */ 	cmp          #48
/* @374 */ 	bne         .strtoll_label_383
/* @375 */ 	lda         __i5+1
/* @377 */ 	bne         .strtoll_label_383
/* @379 */ 	lda          #8
/* @380 */ 	sta         __i3
/* @382 */ 	stz         __i3+1
.strtoll_label_383:
.strtoll_label_384:
/* @385 */ 	lda         __i3
/* @386 */ 	ora         __i3+1
/* @387 */ 	bne         .strtoll_label_392
/* @388 */ 	lda          #10
/* @389 */ 	sta         __i3
/* @391 */ 	stz         __i3+1
.strtoll_label_392:
/* @397 */ 	ldx          #0
/* @399 */ 	lda         __i3
/* @400 */ 	cmp          #2
/* @401 */ 	lda         __i3+1
/* @402 */ 	sbc          #0
/* @403 */ 	bvc         .strtoll_label_398
/* @404 */ 	eor          #128
.strtoll_label_398:
/* @405 */ 	bpl         .strtoll_label_396
/* @406 */ 	inx         
.strtoll_label_396:
/* @407 */ 	stx         __b5
/* @409 */ 	txa         
/* @410 */ 	bne         .strtoll_label_424
/* @413 */ 	ldx          #0
/* @415 */ 	lda          #36
/* @416 */ 	cmp         __i3
/* @417 */ 	txa         
/* @418 */ 	sbc         __i3+1
/* @419 */ 	bvc         .strtoll_label_414
/* @420 */ 	eor          #128
.strtoll_label_414:
/* @421 */ 	bpl         .strtoll_label_412
/* @422 */ 	inx         
.strtoll_label_412:
/* @423 */ 	stx         __b5
.strtoll_label_424:
/* @426 */ 	lda         __b5
/* @428 */ 	beq         .strtoll_label_459
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
/* @445 */ 	ldx          #7
.strtoll_label_447:
/* @449 */ 	sta         __x2, X
/* @450 */ 	dex         
/* @451 */ 	bpl         .strtoll_label_447
/* @452 */ 	ldx          #16
	jsr          __load_result
/* @453 */ 	lda         #__x2
/* @455 */ 	jsr         __result8
.strtoll_label_456:
/* @457 */ 	ldy          #18
	jmp          __leave
.strtoll_label_459:
/* @460 */ 	ldx          #7
/* @461 */ 	lda          #0
.strtoll_label_462:
/* @463 */ 	sta         __x1, X
/* @464 */ 	dex         
/* @465 */ 	bpl         .strtoll_label_462
.strtoll_label_466:
/* @469 */ 	lda         (__i1)
/* @475 */ 	sta         __i5
/* @476 */ 	and          #128
/* @478 */ 	beq         .strtoll_label_477
/* @479 */ 	lda          #255
.strtoll_label_477:
/* @480 */ 	sta         __i5+1
/* @482 */ 	lda         __i5
/* @483 */ 	ora         __i5+1
/* @485 */ 	bne         .strtoll_label_821
/* @822 */ 	jmp         .strtoll_label_690
.strtoll_label_821:
/* @488 */ 	lda         (__i1)
/* @493 */ 	sta         __b0
/* @494 */ 	lda         __i3
/* @495 */ 	sta         __i4
/* @496 */ 	lda         __i3+1
/* @497 */ 	sta         __i4+1
/* @500 */ 	lda         __b0
/* @501 */ 	sta         __i5
/* @502 */ 	and          #128
/* @504 */ 	beq         .strtoll_label_503
/* @505 */ 	lda          #255
.strtoll_label_503:
/* @506 */ 	sta         __i5+1
/* @509 */ 	ldx         __i5
/* @510 */ 	ldy         __i5+1
/* @512 */ 	jsr         __builtin_isalpha
/* @514 */ 	stz         __i6+1
/* @518 */ 	cmp          #0
/* @519 */ 	beq         .strtoll_label_564
/* @522 */ 	lda         __b0
/* @523 */ 	sta         __i5
/* @524 */ 	and          #128
/* @526 */ 	beq         .strtoll_label_525
/* @527 */ 	lda          #255
.strtoll_label_525:
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
/* @563 */ 	bra         .strtoll_label_613
.strtoll_label_564:
/* @567 */ 	lda         __b0
/* @568 */ 	sta         __i5
/* @569 */ 	and          #128
/* @571 */ 	beq         .strtoll_label_570
/* @572 */ 	lda          #255
.strtoll_label_570:
/* @573 */ 	sta         __i5+1
/* @576 */ 	ldx         __i5
/* @577 */ 	ldy         __i5+1
/* @579 */ 	jsr         __builtin_isdigit
/* @581 */ 	stz         __i6+1
/* @585 */ 	cmp          #0
/* @586 */ 	beq         .strtoll_label_612
/* @589 */ 	lda         __b0
/* @590 */ 	sta         __i5
/* @591 */ 	and          #128
/* @593 */ 	beq         .strtoll_label_592
/* @594 */ 	lda          #255
.strtoll_label_592:
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
.strtoll_label_612:
.strtoll_label_613:
/* @615 */ 	lda         __i4
/* @616 */ 	cmp         __i3
/* @617 */ 	lda         __i4+1
/* @618 */ 	sbc         __i3+1
/* @619 */ 	bvc         .strtoll_label_614
/* @620 */ 	eor          #128
.strtoll_label_614:
/* @621 */ 	bpl         .strtoll_label_690
/* @625 */ 	lda         __i3
/* @626 */ 	sta         __x2
/* @627 */ 	lda         __i3+1
/* @628 */ 	sta         __x2+1
/* @629 */ 	and          #128
/* @631 */ 	beq         .strtoll_label_630
/* @632 */ 	lda          #255
.strtoll_label_630:
/* @633 */ 	ldy          #7
.strtoll_label_634:
/* @635 */ 	sta         __x2, Y
/* @636 */ 	dey         
/* @637 */ 	cpy          #1
/* @638 */ 	bne         .strtoll_label_634
/* @643 */ 	lda          #__x3
/* @644 */ 	ldx          #__x1
/* @645 */ 	ldy          #__x2
/* @647 */ 	jsr         __smul8
/* @650 */ 	lda         __i4
/* @651 */ 	sta         __x2
/* @652 */ 	lda         __i4+1
/* @653 */ 	sta         __x2+1
/* @654 */ 	and          #128
/* @656 */ 	beq         .strtoll_label_655
/* @657 */ 	lda          #255
.strtoll_label_655:
/* @658 */ 	ldy          #7
.strtoll_label_659:
/* @660 */ 	sta         __x2, Y
/* @661 */ 	dey         
/* @662 */ 	cpy          #1
/* @663 */ 	bne         .strtoll_label_659
/* @668 */ 	clc         
/* @670 */ 	ldy          #8
/* @671 */ 	ldx          #0
.strtoll_label_672:
/* @673 */ 	lda         __x3, X
/* @674 */ 	adc         __x2, X
/* @675 */ 	sta         __x0, X
/* @676 */ 	inx         
/* @677 */ 	dey         
/* @678 */ 	bne         .strtoll_label_672
/* @681 */ 	ldx          #7
.strtoll_label_682:
/* @683 */ 	lda         __x0, X
/* @684 */ 	sta         __x1, X
/* @685 */ 	dex         
/* @686 */ 	bpl         .strtoll_label_682
/* @687 */ 	lda          #__i1
/* @688 */ 	jsr         __rinc21
/* @689 */ 	jmp         .strtoll_label_466
.strtoll_label_690:
/* @692 */ 	lda         __i1
/* @693 */ 	cmp         __i2
/* @694 */ 	bne         .strtoll_label_724
/* @695 */ 	lda         __i1+1
/* @696 */ 	cmp         __i2+1
/* @697 */ 	bne         .strtoll_label_724
/* @701 */ 	lda          #214
/* @702 */ 	sta         __i5
/* @703 */ 	lda          #3
/* @704 */ 	sta         __i5+1
/* @706 */ 	lda          #7
/* @707 */ 	ldy          #0
/* @708 */ 	sta         (__i5)
/* @709 */ 	tya         
/* @710 */ 	iny         
/* @711 */ 	sta         (__i5), Y
/* @714 */ 	ldx          #7
.strtoll_label_716:
/* @717 */ 	sta         __x0, X
/* @718 */ 	dex         
/* @719 */ 	bpl         .strtoll_label_716
/* @720 */ 	ldx          #16
	jsr          __load_result
/* @721 */ 	lda         #__x0
/* @722 */ 	jsr         __result8
/* @723 */ 	jmp         .strtoll_label_456
.strtoll_label_724:
/* @726 */ 	lda         __i0
/* @728 */ 	bne         .strtoll_label_725
/* @729 */ 	lda         __i0+1
/* @731 */ 	beq         .strtoll_label_738
.strtoll_label_725:
/* @733 */ 	lda         __i1
/* @734 */ 	sta         (__i0)
/* @735 */ 	lda         __i1+1
/* @736 */ 	ldy          #1
/* @737 */ 	sta         (__i0), Y
.strtoll_label_738:
/* @740 */ 	lda         __x1+7
/* @741 */ 	bpl         .strtoll_label_789
/* @742 */ 	lda         __b1
/* @744 */ 	beq         .strtoll_label_754
/* @747 */ 	ldx          #7
.strtoll_label_748:
/* @749 */ 	lda         .lit.17, X
/* @750 */ 	sta         __x0, X
/* @751 */ 	dex         
/* @752 */ 	bpl         .strtoll_label_748
/* @753 */ 	bra         .strtoll_label_763
.strtoll_label_754:
/* @757 */ 	ldx          #7
.strtoll_label_758:
/* @759 */ 	lda         .lit.18, X
/* @760 */ 	sta         __x0, X
/* @761 */ 	dex         
/* @762 */ 	bpl         .strtoll_label_758
/* @819 */ 	jsr          __spill8
	.byte __x0
	.byte 0x0b,0x00
.strtoll_label_763:
/* @820 */ 	jsr          __reload8
	.byte __x0
	.byte 0x0b,0x00
/* @766 */ 	ldx          #7
.strtoll_label_767:
/* @768 */ 	lda         __x0, X
/* @769 */ 	sta         __x1, X
/* @770 */ 	dex         
/* @771 */ 	bpl         .strtoll_label_767
/* @774 */ 	lda          #214
/* @775 */ 	sta         __i5
/* @776 */ 	lda          #3
/* @777 */ 	sta         __i5+1
/* @779 */ 	lda          #15
/* @780 */ 	ldy          #0
/* @781 */ 	sta         (__i5)
/* @782 */ 	tya         
/* @783 */ 	iny         
/* @784 */ 	sta         (__i5), Y
/* @785 */ 	ldx          #16
	jsr          __load_result
/* @786 */ 	lda         #__x1
/* @787 */ 	jsr         __result8
/* @788 */ 	jmp         .strtoll_label_456
.strtoll_label_789:
/* @790 */ 	lda         __b1
/* @792 */ 	beq         .strtoll_label_813
/* @795 */ 	sec         
/* @796 */ 	ldy          #8
/* @797 */ 	ldx          #0
.strtoll_label_798:
/* @799 */ 	lda          #0
/* @800 */ 	sbc         __x1, X
/* @801 */ 	sta         __x0, X
/* @802 */ 	inx         
/* @803 */ 	dey         
/* @804 */ 	bne         .strtoll_label_798
/* @807 */ 	ldx          #7
.strtoll_label_808:
/* @809 */ 	lda         __x0, X
/* @810 */ 	sta         __x1, X
/* @811 */ 	dex         
/* @812 */ 	bpl         .strtoll_label_808
.strtoll_label_813:
/* @814 */ 	ldx          #16
	jsr          __load_result
/* @815 */ 	lda         #__x1
/* @816 */ 	jsr         __result8
/* @817 */ 	jmp         .strtoll_label_456
.func_end_strtoll:
	.size strtoll, .func_end_strtoll-strtoll

	.global atoll
	.type atoll, @function

atoll:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x20,0x00		// Save mask i:0 b:0 l:0 x:1 f:0 
/* @14 */ 	ldx          #0
	jsr          __arg_value2_i0			// s
/* @16 */ 	ldx          #10
/* @18 */ 	jsr         __pushxy0
/* @22 */ 	stz         __i1
/* @24 */ 	stz         __i1+1
/* @26 */ 	jsr         __pushi1
/* @27 */ 	jsr         __pushi0
/* @29 */ 	ldx         #__x1
/* @30 */ 	ldy          #0
/* @31 */ 	jsr         strtoll
/* @33 */ 	jsr         __incsp6
/* @35 */ 	ldx          #8
	jsr          __load_result
/* @36 */ 	lda         #__x1
/* @38 */ 	jsr         __result8
/* @40 */ 	ldy          #10
	jmp          __leave
.func_end_atoll:
	.size atoll, .func_end_atoll-atoll

	.data
	.section ".rodata", "aMS", @progbits
.lit.17:
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x80
	.type .lit.17, @object
	.size .lit.17, 8

.lit.18:
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.byte 0xff
	.byte 0x7f
	.type .lit.18, @object
	.size .lit.18, 8

