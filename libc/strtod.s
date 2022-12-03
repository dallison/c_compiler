	.file   "/Users/dallison/Google Drive/c_compiler/libc/strtod.c"
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


	.global strtod
	.type strtod, @function

strtod:
/* @28 */ 	stx         __result
/* @30 */ 	sty         __result+1
/* @31 */ 	ldx          #57
	jsr          __enter
	.byte        0xa9,0x20,0x00		// Save mask i:9 b:5 l:0 x:1 f:0 
/* @56 */ 	ldx          #0
	jsr          __arg_value2_i0			// str
/* @72 */ 	ldx          #2
	jsr          __arg_value2_i10			// endptr
/* @78 */ 	lda          #32
/* @80 */ 	sta         __mem_size
/* @82 */ 	ldx          #53
	jsr          __var_addr_i1			// fx
/* @84 */ 	lda         __i1
/* @86 */ 	sta         __mem_dest
/* @88 */ 	lda         __i1+1
/* @90 */ 	sta         __mem_dest+1
/* @92 */ 	jsr         __zeromem1
/* @94 */ 	lda          #16
/* @95 */ 	sta         __mem_size
/* @97 */ 	ldx          #19
	jsr          __var_addr_i1			// n
/* @99 */ 	lda         __i1
/* @100 */ 	sta         __mem_dest
/* @101 */ 	lda         __i1+1
/* @102 */ 	sta         __mem_dest+1
/* @103 */ 	jsr         __zeromem1
/* @104 */ 	stz         __b2
/* @105 */ 	lda         __i0
/* @106 */ 	sta         __i4
/* @107 */ 	lda         __i0+1
/* @108 */ 	sta         __i4+1
/* @111 */ 	lda         (__i4)
/* @117 */ 	sta         __i1
/* @119 */ 	and          #128
/* @121 */ 	beq         .strtod_label_120
/* @123 */ 	lda          #255
.strtod_label_120:
/* @124 */ 	sta         __i1+1
/* @127 */ 	lda         __i1
/* @128 */ 	cmp          #45
/* @129 */ 	bne         .strtod_label_139
/* @130 */ 	lda         __i1+1
/* @132 */ 	bne         .strtod_label_139
/* @134 */ 	lda          #128
/* @135 */ 	sta         __b2
/* @136 */ 	lda          #__i4
/* @138 */ 	jsr         __rinc21
.strtod_label_139:
/* @142 */ 	lda         (__i4)
/* @148 */ 	sta         __i0
/* @149 */ 	and          #128
/* @151 */ 	beq         .strtod_label_150
/* @152 */ 	lda          #255
.strtod_label_150:
/* @153 */ 	sta         __i0+1
/* @156 */ 	lda         __i0
/* @157 */ 	cmp          #43
/* @158 */ 	bne         .strtod_label_165
/* @159 */ 	lda         __i0+1
/* @161 */ 	bne         .strtod_label_165
/* @163 */ 	lda          #__i4
/* @164 */ 	jsr         __rinc21
.strtod_label_165:
/* @166 */ 	stz         __i5
/* @167 */ 	stz         __i5+1
.strtod_label_168:
/* @173 */ 	lda         (__i4)
/* @179 */ 	sta         __i0
/* @180 */ 	and          #128
/* @182 */ 	beq         .strtod_label_181
/* @183 */ 	lda          #255
.strtod_label_181:
/* @184 */ 	sta         __i0+1
/* @187 */ 	ldx         __i0
/* @188 */ 	ldy         __i0+1
/* @190 */ 	jsr         __builtin_isdigit
/* @191 */ 	sta         __i1
/* @192 */ 	stz         __i1+1
/* @195 */ 	lda         __i1+1
/* @196 */ 	and          #128
/* @197 */ 	ora         __i1
/* @198 */ 	sta         __b4
/* @202 */ 	beq         .strtod_label_230
/* @205 */ 	lda         (__i4)
/* @211 */ 	sta         __i0
/* @212 */ 	and          #128
/* @214 */ 	beq         .strtod_label_213
/* @215 */ 	lda          #255
.strtod_label_213:
/* @216 */ 	sta         __i0+1
/* @221 */ 	ldx          #0
/* @222 */ 	lda         __i0
/* @223 */ 	cmp          #46
/* @224 */ 	bne         .strtod_label_220
/* @225 */ 	lda         __i0+1
/* @227 */ 	beq         .strtod_label_219
.strtod_label_220:
/* @228 */ 	inx         
.strtod_label_219:
/* @229 */ 	stx         __b4
.strtod_label_230:
/* @232 */ 	lda         __b4
/* @234 */ 	beq         .strtod_label_262
/* @237 */ 	lda         (__i4)
/* @243 */ 	sta         __i0
/* @244 */ 	and          #128
/* @246 */ 	beq         .strtod_label_245
/* @247 */ 	lda          #255
.strtod_label_245:
/* @248 */ 	sta         __i0+1
/* @253 */ 	ldx          #0
/* @254 */ 	lda         __i0
/* @255 */ 	cmp          #101
/* @256 */ 	bne         .strtod_label_252
/* @257 */ 	lda         __i0+1
/* @259 */ 	beq         .strtod_label_251
.strtod_label_252:
/* @260 */ 	inx         
.strtod_label_251:
/* @261 */ 	stx         __b4
.strtod_label_262:
/* @264 */ 	lda         __b4
/* @266 */ 	beq         .strtod_label_294
/* @269 */ 	lda         (__i4)
/* @275 */ 	sta         __i0
/* @276 */ 	and          #128
/* @278 */ 	beq         .strtod_label_277
/* @279 */ 	lda          #255
.strtod_label_277:
/* @280 */ 	sta         __i0+1
/* @285 */ 	ldx          #0
/* @286 */ 	lda         __i0
/* @287 */ 	cmp          #69
/* @288 */ 	bne         .strtod_label_284
/* @289 */ 	lda         __i0+1
/* @291 */ 	beq         .strtod_label_283
.strtod_label_284:
/* @292 */ 	inx         
.strtod_label_283:
/* @293 */ 	stx         __b4
.strtod_label_294:
/* @296 */ 	lda         __b4
/* @298 */ 	beq         .strtod_label_396
/* @301 */ 	lda         (__i4)
/* @307 */ 	sta         __i0
/* @308 */ 	and          #128
/* @310 */ 	beq         .strtod_label_309
/* @311 */ 	lda          #255
.strtod_label_309:
/* @312 */ 	sta         __i0+1
/* @316 */ 	sec         
/* @317 */ 	lda         __i0
/* @318 */ 	sbc          #48
/* @319 */ 	sta         __i1
/* @320 */ 	lda         __i0+1
/* @321 */ 	sbc          #0
/* @322 */ 	sta         __i1+1
/* @326 */ 	lda         __i1
/* @327 */ 	sta         __x0
/* @328 */ 	lda         __i1+1
/* @329 */ 	sta         __x0+1
/* @330 */ 	lda          #0
/* @332 */ 	ldy          #7
.strtod_label_333:
/* @335 */ 	sta         __x0, Y
/* @336 */ 	dey         
/* @337 */ 	cpy          #1
/* @338 */ 	bne         .strtod_label_333
/* @341 */ 	ldx          #19
	jsr          __var_addr_i11			// n
/* @344 */ 	ldy          #7
/* @345 */ 	ldx          #7
.strtod_label_346:
/* @347 */ 	lda         __x0, X
/* @348 */ 	sta         (__i11), Y
/* @349 */ 	dey         
/* @350 */ 	dex         
/* @351 */ 	bpl         .strtod_label_346
/* @353 */ 	ldx          #53
	jsr          __var_addr_i12			// fx
/* @357 */ 	clc         
/* @358 */ 	lda         __i12
/* @359 */ 	adc          #16
/* @360 */ 	sta         __i0
/* @361 */ 	lda         __i12+1
/* @362 */ 	adc          #0
/* @363 */ 	sta         __i0+1
/* @366 */ 	jsr         __pushi0
/* @367 */ 	jsr         __MultiplyBy10Half
/* @369 */ 	jsr         __incsp2
/* @374 */ 	jsr         __pushi11
/* @380 */ 	clc         
/* @381 */ 	lda         __i12
/* @382 */ 	adc          #16
/* @383 */ 	sta         __i0
/* @384 */ 	lda         __i12+1
/* @385 */ 	adc          #0
/* @386 */ 	sta         __i0+1
/* @389 */ 	jsr         __pushi0
/* @390 */ 	jsr         __AddHalf
/* @392 */ 	jsr         __incsp4
/* @393 */ 	lda          #__i4
/* @394 */ 	jsr         __rinc21
/* @395 */ 	jmp         .strtod_label_168
.strtod_label_396:
/* @397 */ 	jsr         Break
/* @400 */ 	lda         (__i4)
/* @406 */ 	sta         __i0
/* @407 */ 	and          #128
/* @409 */ 	beq         .strtod_label_408
/* @410 */ 	lda          #255
.strtod_label_408:
/* @411 */ 	sta         __i0+1
/* @414 */ 	lda         __i0
/* @415 */ 	cmp          #46
/* @416 */ 	beq         .strtod_label_1187
/* @1188 */ 	jmp         .strtod_label_643
.strtod_label_1187:
/* @417 */ 	lda         __i0+1
/* @419 */ 	beq         .strtod_label_1189
/* @1190 */ 	jmp         .strtod_label_643
.strtod_label_1189:
/* @421 */ 	lda          #__i4
/* @422 */ 	jsr         __rinc21
.strtod_label_423:
/* @428 */ 	lda         (__i4)
/* @434 */ 	sta         __i0
/* @435 */ 	and          #128
/* @437 */ 	beq         .strtod_label_436
/* @438 */ 	lda          #255
.strtod_label_436:
/* @439 */ 	sta         __i0+1
/* @442 */ 	ldx         __i0
/* @443 */ 	ldy         __i0+1
/* @444 */ 	jsr         __builtin_isdigit
/* @445 */ 	sta         __i1
/* @446 */ 	stz         __i1+1
/* @449 */ 	lda         __i1+1
/* @450 */ 	and          #128
/* @451 */ 	ora         __i1
/* @452 */ 	sta         __b5
/* @456 */ 	beq         .strtod_label_484
/* @459 */ 	lda         (__i4)
/* @465 */ 	sta         __i0
/* @466 */ 	and          #128
/* @468 */ 	beq         .strtod_label_467
/* @469 */ 	lda          #255
.strtod_label_467:
/* @470 */ 	sta         __i0+1
/* @475 */ 	ldx          #0
/* @476 */ 	lda         __i0
/* @477 */ 	cmp          #101
/* @478 */ 	bne         .strtod_label_474
/* @479 */ 	lda         __i0+1
/* @481 */ 	beq         .strtod_label_473
.strtod_label_474:
/* @482 */ 	inx         
.strtod_label_473:
/* @483 */ 	stx         __b5
.strtod_label_484:
/* @486 */ 	lda         __b5
/* @488 */ 	beq         .strtod_label_516
/* @491 */ 	lda         (__i4)
/* @497 */ 	sta         __i0
/* @498 */ 	and          #128
/* @500 */ 	beq         .strtod_label_499
/* @501 */ 	lda          #255
.strtod_label_499:
/* @502 */ 	sta         __i0+1
/* @507 */ 	ldx          #0
/* @508 */ 	lda         __i0
/* @509 */ 	cmp          #69
/* @510 */ 	bne         .strtod_label_506
/* @511 */ 	lda         __i0+1
/* @513 */ 	beq         .strtod_label_505
.strtod_label_506:
/* @514 */ 	inx         
.strtod_label_505:
/* @515 */ 	stx         __b5
.strtod_label_516:
/* @518 */ 	lda         __b5
/* @520 */ 	bne         .strtod_label_1191
/* @1192 */ 	jmp         .strtod_label_616
.strtod_label_1191:
/* @523 */ 	lda         (__i4)
/* @529 */ 	sta         __i0
/* @530 */ 	and          #128
/* @532 */ 	beq         .strtod_label_531
/* @533 */ 	lda          #255
.strtod_label_531:
/* @534 */ 	sta         __i0+1
/* @538 */ 	sec         
/* @539 */ 	lda         __i0
/* @540 */ 	sbc          #48
/* @541 */ 	sta         __i1
/* @542 */ 	lda         __i0+1
/* @543 */ 	sbc          #0
/* @544 */ 	sta         __i1+1
/* @548 */ 	lda         __i1
/* @549 */ 	sta         __x0
/* @550 */ 	lda         __i1+1
/* @551 */ 	sta         __x0+1
/* @552 */ 	lda          #0
/* @553 */ 	ldy          #7
.strtod_label_554:
/* @555 */ 	sta         __x0, Y
/* @556 */ 	dey         
/* @557 */ 	cpy          #1
/* @558 */ 	bne         .strtod_label_554
/* @561 */ 	ldx          #19
	jsr          __var_addr_i11			// n
/* @564 */ 	ldy          #7
/* @565 */ 	ldx          #7
.strtod_label_566:
/* @567 */ 	lda         __x0, X
/* @568 */ 	sta         (__i11), Y
/* @569 */ 	dey         
/* @570 */ 	dex         
/* @571 */ 	bpl         .strtod_label_566
/* @573 */ 	ldx          #53
	jsr          __var_addr_i12			// fx
/* @577 */ 	clc         
/* @578 */ 	lda         __i12
/* @579 */ 	adc          #16
/* @580 */ 	sta         __i0
/* @581 */ 	lda         __i12+1
/* @582 */ 	adc          #0
/* @583 */ 	sta         __i0+1
/* @586 */ 	jsr         __pushi0
/* @587 */ 	jsr         __MultiplyBy10Half
/* @588 */ 	jsr         __incsp2
/* @593 */ 	jsr         __pushi11
/* @599 */ 	clc         
/* @600 */ 	lda         __i12
/* @601 */ 	adc          #16
/* @602 */ 	sta         __i0
/* @603 */ 	lda         __i12+1
/* @604 */ 	adc          #0
/* @605 */ 	sta         __i0+1
/* @608 */ 	jsr         __pushi0
/* @609 */ 	jsr         __AddHalf
/* @610 */ 	jsr         __incsp4
/* @611 */ 	lda          #__i4
/* @612 */ 	jsr         __rinc21
/* @613 */ 	lda          #__i5
/* @614 */ 	jsr         __rinc21
/* @615 */ 	jmp         .strtod_label_423
.strtod_label_616:
/* @617 */ 	stz         __i6
/* @618 */ 	stz         __i6+1
.strtod_label_619:
/* @621 */ 	lda         __i6
/* @622 */ 	cmp         __i5
/* @623 */ 	lda         __i6+1
/* @624 */ 	sbc         __i5+1
/* @625 */ 	bvc         .strtod_label_620
/* @626 */ 	eor          #128
.strtod_label_620:
/* @627 */ 	bpl         .strtod_label_642
/* @629 */ 	ldx          #53
	jsr          __var_addr_i0			// fx
/* @632 */ 	jsr         __pushi0
/* @634 */ 	ldx         #__b0
/* @635 */ 	ldy          #0
/* @636 */ 	jsr         __DivideBy10
/* @637 */ 	jsr         __incsp2
/* @639 */ 	lda          #__i6
/* @640 */ 	jsr         __rinc21
/* @641 */ 	bra         .strtod_label_619
.strtod_label_642:
.strtod_label_643:
/* @648 */ 	lda         (__i4)
/* @654 */ 	sta         __i0
/* @655 */ 	and          #128
/* @657 */ 	beq         .strtod_label_656
/* @658 */ 	lda          #255
.strtod_label_656:
/* @659 */ 	sta         __i0+1
/* @664 */ 	ldx          #1
/* @665 */ 	lda         __i0
/* @666 */ 	cmp          #101
/* @667 */ 	bne         .strtod_label_662
/* @668 */ 	lda         __i0+1
/* @670 */ 	beq         .strtod_label_663
.strtod_label_662:
/* @671 */ 	dex         
.strtod_label_663:
/* @672 */ 	stx         __b5
/* @674 */ 	txa         
/* @675 */ 	bne         .strtod_label_703
/* @678 */ 	lda         (__i4)
/* @684 */ 	sta         __i0
/* @685 */ 	and          #128
/* @687 */ 	beq         .strtod_label_686
/* @688 */ 	lda          #255
.strtod_label_686:
/* @689 */ 	sta         __i0+1
/* @694 */ 	ldx          #1
/* @695 */ 	lda         __i0
/* @696 */ 	cmp          #69
/* @697 */ 	bne         .strtod_label_692
/* @698 */ 	lda         __i0+1
/* @700 */ 	beq         .strtod_label_693
.strtod_label_692:
/* @701 */ 	dex         
.strtod_label_693:
/* @702 */ 	stx         __b5
.strtod_label_703:
/* @705 */ 	lda         __b5
/* @707 */ 	bne         .strtod_label_1193
/* @1194 */ 	jmp         .strtod_label_952
.strtod_label_1193:
/* @708 */ 	lda          #__i4
/* @709 */ 	jsr         __rinc21
/* @712 */ 	lda         (__i4)
/* @718 */ 	sta         __i0
/* @719 */ 	and          #128
/* @721 */ 	beq         .strtod_label_720
/* @722 */ 	lda          #255
.strtod_label_720:
/* @723 */ 	sta         __i0+1
/* @729 */ 	ldx          #1
/* @730 */ 	lda         __i0
/* @731 */ 	cmp          #45
/* @732 */ 	bne         .strtod_label_727
/* @733 */ 	lda         __i0+1
/* @735 */ 	beq         .strtod_label_728
.strtod_label_727:
/* @736 */ 	dex         
.strtod_label_728:
/* @740 */ 	txa         
/* @741 */ 	sta         __b3
/* @746 */ 	lda         (__i4)
/* @752 */ 	sta         __i0
/* @753 */ 	and          #128
/* @755 */ 	beq         .strtod_label_754
/* @756 */ 	lda          #255
.strtod_label_754:
/* @757 */ 	sta         __i0+1
/* @762 */ 	ldx          #1
/* @763 */ 	lda         __i0
/* @764 */ 	cmp          #43
/* @765 */ 	bne         .strtod_label_760
/* @766 */ 	lda         __i0+1
/* @768 */ 	beq         .strtod_label_761
.strtod_label_760:
/* @769 */ 	dex         
.strtod_label_761:
/* @770 */ 	stx         __b6
/* @772 */ 	txa         
/* @773 */ 	bne         .strtod_label_801
/* @776 */ 	lda         (__i4)
/* @782 */ 	sta         __i0
/* @783 */ 	and          #128
/* @785 */ 	beq         .strtod_label_784
/* @786 */ 	lda          #255
.strtod_label_784:
/* @787 */ 	sta         __i0+1
/* @792 */ 	ldx          #1
/* @793 */ 	lda         __i0
/* @794 */ 	cmp          #45
/* @795 */ 	bne         .strtod_label_790
/* @796 */ 	lda         __i0+1
/* @798 */ 	beq         .strtod_label_791
.strtod_label_790:
/* @799 */ 	dex         
.strtod_label_791:
/* @800 */ 	stx         __b6
.strtod_label_801:
/* @803 */ 	lda         __b6
/* @805 */ 	beq         .strtod_label_808
/* @806 */ 	lda          #__i4
/* @807 */ 	jsr         __rinc21
.strtod_label_808:
/* @809 */ 	stz         __i7
/* @810 */ 	stz         __i7+1
.strtod_label_811:
/* @814 */ 	lda         (__i4)
/* @820 */ 	sta         __i0
/* @821 */ 	and          #128
/* @823 */ 	beq         .strtod_label_822
/* @824 */ 	lda          #255
.strtod_label_822:
/* @825 */ 	sta         __i0+1
/* @828 */ 	ldx         __i0
/* @829 */ 	ldy         __i0+1
/* @830 */ 	jsr         __builtin_isdigit
/* @832 */ 	stz         __i1+1
/* @836 */ 	cmp          #0
/* @837 */ 	beq         .strtod_label_896
/* @840 */ 	lda          #__i0
/* @841 */ 	ldx          #__i7
/* @843 */ 	jsr         __smul2_10
/* @846 */ 	lda         __i4
/* @847 */ 	sta         __i1
/* @848 */ 	lda         __i4+1
/* @849 */ 	sta         __i1+1
/* @850 */ 	lda          #__i4
/* @851 */ 	jsr         __rinc21
/* @856 */ 	lda         (__i1)
/* @862 */ 	sta         __i1
/* @863 */ 	and          #128
/* @865 */ 	beq         .strtod_label_864
/* @866 */ 	lda          #255
.strtod_label_864:
/* @867 */ 	sta         __i1+1
/* @872 */ 	clc         
/* @873 */ 	lda         __i0
/* @874 */ 	adc         __i1
/* @875 */ 	sta         __i2
/* @876 */ 	lda         __i0+1
/* @877 */ 	adc         __i1+1
/* @878 */ 	sta         __i2+1
/* @882 */ 	sec         
/* @883 */ 	lda         __i2
/* @884 */ 	sbc          #48
/* @885 */ 	sta         __i0
/* @886 */ 	lda         __i2+1
/* @887 */ 	sbc          #0
/* @888 */ 	sta         __i0+1
/* @891 */ 	lda         __i0
/* @892 */ 	sta         __i7
/* @893 */ 	lda         __i0+1
/* @894 */ 	sta         __i7+1
/* @895 */ 	bra         .strtod_label_811
.strtod_label_896:
/* @897 */ 	lda         __b3
/* @899 */ 	beq         .strtod_label_927
/* @900 */ 	stz         __i8
/* @901 */ 	stz         __i8+1
.strtod_label_902:
/* @904 */ 	lda         __i8
/* @905 */ 	cmp         __i7
/* @906 */ 	lda         __i8+1
/* @907 */ 	sbc         __i7+1
/* @908 */ 	bvc         .strtod_label_903
/* @909 */ 	eor          #128
.strtod_label_903:
/* @910 */ 	bpl         .strtod_label_925
/* @912 */ 	ldx          #53
	jsr          __var_addr_i0			// fx
/* @915 */ 	jsr         __pushi0
/* @917 */ 	ldx         #__b0
/* @918 */ 	ldy          #0
/* @919 */ 	jsr         __DivideBy10
/* @920 */ 	jsr         __incsp2
/* @922 */ 	lda          #__i8
/* @923 */ 	jsr         __rinc21
/* @924 */ 	bra         .strtod_label_902
.strtod_label_925:
/* @926 */ 	bra         .strtod_label_951
.strtod_label_927:
/* @928 */ 	stz         __i9
/* @929 */ 	stz         __i9+1
.strtod_label_930:
/* @932 */ 	lda         __i9
/* @933 */ 	cmp         __i7
/* @934 */ 	lda         __i9+1
/* @935 */ 	sbc         __i7+1
/* @936 */ 	bvc         .strtod_label_931
/* @937 */ 	eor          #128
.strtod_label_931:
/* @938 */ 	bpl         .strtod_label_950
/* @940 */ 	ldx          #53
	jsr          __var_addr_i0			// fx
/* @943 */ 	jsr         __pushi0
/* @944 */ 	jsr         __MultiplyBy10
/* @945 */ 	jsr         __incsp2
/* @947 */ 	lda          #__i9
/* @948 */ 	jsr         __rinc21
/* @949 */ 	bra         .strtod_label_930
.strtod_label_950:
.strtod_label_951:
.strtod_label_952:
/* @954 */ 	lda         __i10
/* @956 */ 	bne         .strtod_label_953
/* @957 */ 	lda         __i10+1
/* @959 */ 	beq         .strtod_label_966
.strtod_label_953:
/* @961 */ 	lda         __i4
/* @962 */ 	sta         (__i10)
/* @963 */ 	lda         __i4+1
/* @964 */ 	ldy          #1
/* @965 */ 	sta         (__i10), Y
.strtod_label_966:
/* @969 */ 	lda          #127
/* @970 */ 	sta         __i0
/* @972 */ 	stz         __i0+1
/* @973 */ 	lda          #__i0
/* @975 */ 	ldx          #21
/* @977 */ 	jsr         __set_var_value2
/* @979 */ 	ldx          #53
	jsr          __var_addr_i11			// fx
/* @982 */ 	jsr         __pushi11
/* @984 */ 	ldx         #__b6
/* @985 */ 	ldy          #0
/* @986 */ 	jsr         __IsZero
/* @987 */ 	jsr         __incsp2
/* @989 */ 	lda         __b6
/* @991 */ 	beq         .strtod_label_1008
/* @995 */ 	ldx          #3
/* @996 */ 	lda          #0
.strtod_label_997:
/* @998 */ 	sta         __f0, X
/* @999 */ 	dex         
/* @1000 */ 	bpl         .strtod_label_997
/* @1001 */ 	ldx          #58
	jsr          __load_result
/* @1002 */ 	lda         #__f0
/* @1004 */ 	jsr         __result4
.strtod_label_1005:
/* @1006 */ 	ldy          #60
	jmp          __leave
.strtod_label_1008:
/* @1014 */ 	clc         
/* @1015 */ 	lda         __i11
/* @1016 */ 	adc          #16
/* @1017 */ 	sta         __i0
/* @1018 */ 	lda         __i11+1
/* @1019 */ 	adc          #0
/* @1020 */ 	sta         __i0+1
/* @1023 */ 	jsr         __pushi0
/* @1025 */ 	ldx         #__b6
/* @1026 */ 	ldy          #0
/* @1027 */ 	jsr         __IsZeroHalf
/* @1028 */ 	jsr         __incsp2
/* @1030 */ 	lda         __b6
/* @1032 */ 	beq         .strtod_label_1084
.strtod_label_1033:
/* @1035 */ 	ldx          #53
	jsr          __var_addr_i12			// fx
/* @1039 */ 	clc         
/* @1040 */ 	lda         __i12
/* @1041 */ 	adc          #16
/* @1042 */ 	sta         __i0
/* @1043 */ 	lda         __i12+1
/* @1044 */ 	adc          #0
/* @1045 */ 	sta         __i0+1
/* @1048 */ 	jsr         __pushi0
/* @1050 */ 	ldx         #__b6
/* @1051 */ 	ldy          #0
/* @1052 */ 	jsr         __IsOneHalf
/* @1053 */ 	jsr         __incsp2
/* @1058 */ 	lda         __b6
/* @1060 */ 	beq         .strtod_label_1057
/* @1061 */ 	lda          #255
.strtod_label_1057:
/* @1062 */ 	inc          A
/* @1067 */ 	beq         .strtod_label_1082
/* @1072 */ 	jsr         __pushi12
/* @1073 */ 	jsr         __LShift
/* @1074 */ 	jsr         __incsp2
/* @1076 */ 	ldx          #21
	jsr          __var_addr_i0			// exp
/* @1078 */ 	lda          #__i0
/* @1080 */ 	jsr         __dec21
/* @1081 */ 	bra         .strtod_label_1033
.strtod_label_1082:
/* @1083 */ 	bra         .strtod_label_1135
.strtod_label_1084:
.strtod_label_1085:
/* @1087 */ 	ldx          #53
	jsr          __var_addr_i12			// fx
/* @1091 */ 	clc         
/* @1092 */ 	lda         __i12
/* @1093 */ 	adc          #16
/* @1094 */ 	sta         __i0
/* @1095 */ 	lda         __i12+1
/* @1096 */ 	adc          #0
/* @1097 */ 	sta         __i0+1
/* @1100 */ 	jsr         __pushi0
/* @1102 */ 	ldx         #__b6
/* @1103 */ 	ldy          #0
/* @1104 */ 	jsr         __IsOneHalf
/* @1105 */ 	jsr         __incsp2
/* @1110 */ 	lda         __b6
/* @1112 */ 	beq         .strtod_label_1109
/* @1113 */ 	lda          #255
.strtod_label_1109:
/* @1114 */ 	inc          A
/* @1119 */ 	beq         .strtod_label_1134
/* @1124 */ 	jsr         __pushi12
/* @1125 */ 	jsr         __RShift
/* @1126 */ 	jsr         __incsp2
/* @1128 */ 	ldx          #21
	jsr          __var_addr_i0			// exp
/* @1130 */ 	lda          #__i0
/* @1132 */ 	jsr         __inc21
/* @1133 */ 	bra         .strtod_label_1085
.strtod_label_1134:
.strtod_label_1135:
/* @1142 */ 	ldy          #15
/* @1143 */ 	ldx          #7
.strtod_label_1144:
/* @1145 */ 	lda         (__i11), Y
/* @1146 */ 	sta         __x0, X
/* @1147 */ 	dey         
/* @1148 */ 	dex         
/* @1149 */ 	bpl         .strtod_label_1144
/* @1152 */ 	ldx          #7
.strtod_label_1153:
/* @1154 */ 	lda         __x0, X
/* @1155 */ 	sta         __x1, X
/* @1156 */ 	dex         
/* @1157 */ 	bpl         .strtod_label_1153
/* @1158 */ 	jsr         __pushx1
/* @1160 */ 	ldx          #21
	jsr          __var_value2_i0			// exp
/* @1163 */ 	jsr         __pushi0
/* @1166 */ 	lda         __b2
/* @1167 */ 	sta         __i0
/* @1168 */ 	and          #128
/* @1170 */ 	beq         .strtod_label_1169
/* @1171 */ 	lda          #255
.strtod_label_1169:
/* @1172 */ 	sta         __i0+1
/* @1175 */ 	jsr         __pushi0
/* @1177 */ 	ldx         #__f0
/* @1178 */ 	ldy          #0
/* @1179 */ 	jsr         __packIEEE754
/* @1181 */ 	jsr         __incsp12
/* @1183 */ 	ldx          #58
	jsr          __load_result
/* @1184 */ 	lda         #__f0
/* @1185 */ 	jsr         __result4
/* @1186 */ 	jmp         .strtod_label_1005
.func_end_strtod:
	.size strtod, .func_end_strtod-strtod

	.data
	.section ".rodata", "aMS", @progbits
