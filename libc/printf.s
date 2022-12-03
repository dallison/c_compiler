	.file   "/Users/dallison/Google Drive/c_compiler/libc/printf.c"
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
/* @36 */ 	stx         __result
/* @38 */ 	sty         __result+1
/* @39 */ 	ldx          #7
	jsr          __enter
	.byte        0x62,0x00,0x00		// Save mask i:2 b:3 l:0 x:0 f:0 
/* @44 */ 	ldx          #2
	jsr          __arg_value2_i0			// format
/* @53 */ 	ldx          #0
	jsr          __arg_value2_i2			// p
/* @54 */ 	lda          #255
/* @55 */ 	ldy          #0
/* @56 */ 	sta         (__i0)
/* @58 */ 	iny         
/* @59 */ 	sta         (__i0), Y
/* @62 */ 	iny         
/* @63 */ 	sta         (__i0), Y
/* @66 */ 	iny         
/* @67 */ 	sta         (__i0), Y
/* @68 */ 	lda          #0
/* @70 */ 	iny         
/* @71 */ 	sta         (__i0), Y
/* @74 */ 	iny         
/* @75 */ 	sta         (__i0), Y
/* @77 */ 	ldy          #13
.CollectFormat_label_79:
/* @81 */ 	sta         (__i0), Y
/* @82 */ 	iny         
/* @84 */ 	cpy          #15
/* @85 */ 	bne         .CollectFormat_label_79
/* @86 */ 	stz         __i1
/* @87 */ 	stz         __i1+1
/* @88 */ 	stz         __b0
.CollectFormat_label_89:
/* @94 */ 	lda         __b0
/* @96 */ 	beq         .CollectFormat_label_93
/* @98 */ 	lda          #255
.CollectFormat_label_93:
/* @99 */ 	inc          A
/* @100 */ 	sta         __b1
/* @104 */ 	beq         .CollectFormat_label_131
/* @107 */ 	lda         (__i2)
/* @113 */ 	sta         __i3
/* @115 */ 	and          #128
/* @117 */ 	beq         .CollectFormat_label_116
/* @118 */ 	lda          #255
.CollectFormat_label_116:
/* @119 */ 	sta         __i3+1
/* @124 */ 	ldx          #1
/* @125 */ 	lda         __i3
/* @126 */ 	ora         __i3+1
/* @128 */ 	bne         .CollectFormat_label_123
/* @129 */ 	dex         
.CollectFormat_label_123:
/* @130 */ 	stx         __b1
.CollectFormat_label_131:
/* @133 */ 	lda         __b1
/* @135 */ 	beq         .CollectFormat_label_235
/* @138 */ 	lda         (__i2)
/* @139 */ 	sta         __b2
/* @143 */ 	cmp          #32
/* @144 */ 	beq         .CollectFormat_label_217
/* @148 */ 	lda         __b2
/* @149 */ 	cmp          #35
/* @150 */ 	beq         .CollectFormat_label_209
/* @154 */ 	lda         __b2
/* @155 */ 	cmp          #43
/* @156 */ 	beq         .CollectFormat_label_225
/* @160 */ 	lda         __b2
/* @161 */ 	cmp          #45
/* @162 */ 	beq         .CollectFormat_label_174
/* @166 */ 	lda         __b2
/* @167 */ 	cmp          #48
/* @168 */ 	beq         .CollectFormat_label_182
/* @171 */ 	lda          #1
/* @172 */ 	sta         __b0
/* @173 */ 	bra         .CollectFormat_label_233
.CollectFormat_label_174:
/* @175 */ 	lda          #__i2
/* @177 */ 	jsr         __rinc21
/* @178 */ 	lda          #1
/* @179 */ 	ldy          #4
/* @180 */ 	sta         (__i0), Y
/* @181 */ 	bra         .CollectFormat_label_233
.CollectFormat_label_182:
/* @185 */ 	ldy          #4
/* @186 */ 	lda         (__i0), Y
/* @194 */ 	beq         .CollectFormat_label_191
/* @195 */ 	lda          #255
.CollectFormat_label_191:
/* @196 */ 	inc          A
/* @201 */ 	beq         .CollectFormat_label_205
/* @202 */ 	lda          #1
/* @203 */ 	ldy          #5
/* @204 */ 	sta         (__i0), Y
.CollectFormat_label_205:
/* @206 */ 	lda          #__i2
/* @207 */ 	jsr         __rinc21
/* @208 */ 	bra         .CollectFormat_label_233
.CollectFormat_label_209:
/* @210 */ 	lda          #1
/* @212 */ 	ldy          #8
/* @213 */ 	sta         (__i0), Y
/* @214 */ 	lda          #__i2
/* @215 */ 	jsr         __rinc21
/* @216 */ 	bra         .CollectFormat_label_233
.CollectFormat_label_217:
/* @218 */ 	lda          #1
/* @220 */ 	ldy          #7
/* @221 */ 	sta         (__i0), Y
/* @222 */ 	lda          #__i2
/* @223 */ 	jsr         __rinc21
/* @224 */ 	bra         .CollectFormat_label_233
.CollectFormat_label_225:
/* @226 */ 	lda          #1
/* @228 */ 	ldy          #6
/* @229 */ 	sta         (__i0), Y
/* @230 */ 	lda          #__i2
/* @231 */ 	jsr         __rinc21
.CollectFormat_label_233:
/* @234 */ 	jmp         .CollectFormat_label_89
.CollectFormat_label_235:
/* @238 */ 	lda         (__i2)
/* @244 */ 	sta         __i3
/* @245 */ 	and          #128
/* @247 */ 	beq         .CollectFormat_label_246
/* @248 */ 	lda          #255
.CollectFormat_label_246:
/* @249 */ 	sta         __i3+1
/* @252 */ 	lda         __i3
/* @253 */ 	cmp          #42
/* @254 */ 	bne         .CollectFormat_label_274
/* @255 */ 	lda         __i3+1
/* @257 */ 	bne         .CollectFormat_label_274
/* @259 */ 	lda          #__i2
/* @260 */ 	jsr         __rinc21
/* @261 */ 	lda          #254
/* @262 */ 	ldy          #0
/* @263 */ 	sta         (__i0)
/* @264 */ 	inc          A
/* @265 */ 	iny         
/* @266 */ 	sta         (__i0), Y
/* @267 */ 	ldx          #8
	jsr          __load_result
/* @268 */ 	lda         #__i2
/* @270 */ 	jsr         __result2
.CollectFormat_label_271:
/* @272 */ 	ldy          #10
	jmp          __leave
.CollectFormat_label_274:
/* @277 */ 	lda         (__i2)
/* @283 */ 	sta         __i3
/* @284 */ 	and          #128
/* @286 */ 	beq         .CollectFormat_label_285
/* @287 */ 	lda          #255
.CollectFormat_label_285:
/* @288 */ 	sta         __i3+1
/* @291 */ 	ldx         __i3
/* @292 */ 	ldy         __i3+1
/* @294 */ 	jsr         __builtin_isdigit
/* @296 */ 	stz         __i4+1
/* @300 */ 	cmp          #0
/* @301 */ 	beq         .CollectFormat_label_393
.CollectFormat_label_302:
/* @305 */ 	lda         (__i2)
/* @311 */ 	sta         __i3
/* @312 */ 	and          #128
/* @314 */ 	beq         .CollectFormat_label_313
/* @315 */ 	lda          #255
.CollectFormat_label_313:
/* @316 */ 	sta         __i3+1
/* @319 */ 	ldx         __i3
/* @320 */ 	ldy         __i3+1
/* @321 */ 	jsr         __builtin_isdigit
/* @323 */ 	stz         __i4+1
/* @327 */ 	cmp          #0
/* @328 */ 	beq         .CollectFormat_label_387
/* @331 */ 	lda          #__i3
/* @332 */ 	ldx          #__i1
/* @334 */ 	jsr         __smul2_10
/* @337 */ 	lda         __i2
/* @338 */ 	sta         __i4
/* @339 */ 	lda         __i2+1
/* @340 */ 	sta         __i4+1
/* @341 */ 	lda          #__i2
/* @342 */ 	jsr         __rinc21
/* @347 */ 	lda         (__i4)
/* @353 */ 	sta         __i4
/* @354 */ 	and          #128
/* @356 */ 	beq         .CollectFormat_label_355
/* @357 */ 	lda          #255
.CollectFormat_label_355:
/* @358 */ 	sta         __i4+1
/* @363 */ 	clc         
/* @364 */ 	lda         __i3
/* @365 */ 	adc         __i4
/* @366 */ 	sta         __i5
/* @367 */ 	lda         __i3+1
/* @368 */ 	adc         __i4+1
/* @369 */ 	sta         __i5+1
/* @373 */ 	sec         
/* @374 */ 	lda         __i5
/* @375 */ 	sbc          #48
/* @376 */ 	sta         __i3
/* @377 */ 	lda         __i5+1
/* @378 */ 	sbc          #0
/* @379 */ 	sta         __i3+1
/* @382 */ 	lda         __i3
/* @383 */ 	sta         __i1
/* @384 */ 	lda         __i3+1
/* @385 */ 	sta         __i1+1
/* @386 */ 	bra         .CollectFormat_label_302
.CollectFormat_label_387:
/* @388 */ 	lda         __i1
/* @389 */ 	sta         (__i0)
/* @390 */ 	lda         __i1+1
/* @391 */ 	ldy          #1
/* @392 */ 	sta         (__i0), Y
.CollectFormat_label_393:
/* @396 */ 	lda         (__i2)
/* @402 */ 	sta         __i3
/* @403 */ 	and          #128
/* @405 */ 	beq         .CollectFormat_label_404
/* @406 */ 	lda          #255
.CollectFormat_label_404:
/* @407 */ 	sta         __i3+1
/* @410 */ 	lda         __i3
/* @411 */ 	cmp          #46
/* @412 */ 	beq         .CollectFormat_label_752
/* @753 */ 	jmp         .CollectFormat_label_573
.CollectFormat_label_752:
/* @413 */ 	lda         __i3+1
/* @415 */ 	beq         .CollectFormat_label_754
/* @755 */ 	jmp         .CollectFormat_label_573
.CollectFormat_label_754:
/* @417 */ 	lda          #__i2
/* @418 */ 	jsr         __rinc21
/* @421 */ 	lda         (__i2)
/* @427 */ 	sta         __i3
/* @428 */ 	and          #128
/* @430 */ 	beq         .CollectFormat_label_429
/* @431 */ 	lda          #255
.CollectFormat_label_429:
/* @432 */ 	sta         __i3+1
/* @435 */ 	lda         __i3
/* @436 */ 	cmp          #42
/* @437 */ 	bne         .CollectFormat_label_454
/* @438 */ 	lda         __i3+1
/* @440 */ 	bne         .CollectFormat_label_454
/* @442 */ 	lda          #__i2
/* @443 */ 	jsr         __rinc21
/* @444 */ 	lda          #254
/* @445 */ 	ldy          #2
/* @446 */ 	sta         (__i0), Y
/* @447 */ 	inc          A
/* @448 */ 	iny         
/* @449 */ 	sta         (__i0), Y
/* @450 */ 	ldx          #8
	jsr          __load_result
/* @451 */ 	lda         #__i2
/* @452 */ 	jsr         __result2
/* @453 */ 	jmp         .CollectFormat_label_271
.CollectFormat_label_454:
/* @457 */ 	lda         (__i2)
/* @463 */ 	sta         __i3
/* @464 */ 	and          #128
/* @466 */ 	beq         .CollectFormat_label_465
/* @467 */ 	lda          #255
.CollectFormat_label_465:
/* @468 */ 	sta         __i3+1
/* @471 */ 	ldx         __i3
/* @472 */ 	ldy         __i3+1
/* @473 */ 	jsr         __builtin_isdigit
/* @475 */ 	stz         __i4+1
/* @479 */ 	cmp          #0
/* @480 */ 	beq         .CollectFormat_label_572
.CollectFormat_label_481:
/* @484 */ 	lda         (__i2)
/* @490 */ 	sta         __i3
/* @491 */ 	and          #128
/* @493 */ 	beq         .CollectFormat_label_492
/* @494 */ 	lda          #255
.CollectFormat_label_492:
/* @495 */ 	sta         __i3+1
/* @498 */ 	ldx         __i3
/* @499 */ 	ldy         __i3+1
/* @500 */ 	jsr         __builtin_isdigit
/* @502 */ 	stz         __i4+1
/* @506 */ 	cmp          #0
/* @507 */ 	beq         .CollectFormat_label_565
/* @510 */ 	lda          #__i3
/* @511 */ 	ldx          #__i1
/* @512 */ 	jsr         __smul2_10
/* @515 */ 	lda         __i2
/* @516 */ 	sta         __i4
/* @517 */ 	lda         __i2+1
/* @518 */ 	sta         __i4+1
/* @519 */ 	lda          #__i2
/* @520 */ 	jsr         __rinc21
/* @525 */ 	lda         (__i4)
/* @531 */ 	sta         __i4
/* @532 */ 	and          #128
/* @534 */ 	beq         .CollectFormat_label_533
/* @535 */ 	lda          #255
.CollectFormat_label_533:
/* @536 */ 	sta         __i4+1
/* @541 */ 	clc         
/* @542 */ 	lda         __i3
/* @543 */ 	adc         __i4
/* @544 */ 	sta         __i5
/* @545 */ 	lda         __i3+1
/* @546 */ 	adc         __i4+1
/* @547 */ 	sta         __i5+1
/* @551 */ 	sec         
/* @552 */ 	lda         __i5
/* @553 */ 	sbc          #48
/* @554 */ 	sta         __i3
/* @555 */ 	lda         __i5+1
/* @556 */ 	sbc          #0
/* @557 */ 	sta         __i3+1
/* @560 */ 	lda         __i3
/* @561 */ 	sta         __i1
/* @562 */ 	lda         __i3+1
/* @563 */ 	sta         __i1+1
/* @564 */ 	bra         .CollectFormat_label_481
.CollectFormat_label_565:
/* @566 */ 	lda         __i1
/* @567 */ 	ldy          #2
/* @568 */ 	sta         (__i0), Y
/* @569 */ 	lda         __i1+1
/* @570 */ 	iny         
/* @571 */ 	sta         (__i0), Y
.CollectFormat_label_572:
.CollectFormat_label_573:
/* @576 */ 	lda         (__i2)
/* @577 */ 	sta         __b2
/* @581 */ 	cmp          #76
/* @582 */ 	bne         .CollectFormat_label_756
/* @757 */ 	jmp         .CollectFormat_label_707
.CollectFormat_label_756:
/* @586 */ 	lda         __b2
/* @587 */ 	cmp          #104
/* @588 */ 	beq         .CollectFormat_label_662
/* @592 */ 	lda         __b2
/* @593 */ 	cmp          #106
/* @594 */ 	bne         .CollectFormat_label_758
/* @759 */ 	jmp         .CollectFormat_label_727
.CollectFormat_label_758:
/* @598 */ 	lda         __b2
/* @599 */ 	cmp          #108
/* @600 */ 	beq         .CollectFormat_label_616
/* @604 */ 	lda         __b2
/* @605 */ 	cmp          #116
/* @606 */ 	bne         .CollectFormat_label_760
/* @761 */ 	jmp         .CollectFormat_label_737
.CollectFormat_label_760:
/* @610 */ 	lda         __b2
/* @611 */ 	cmp          #122
/* @612 */ 	bne         .CollectFormat_label_762
/* @763 */ 	jmp         .CollectFormat_label_717
.CollectFormat_label_762:
/* @615 */ 	jmp         .CollectFormat_label_747
.CollectFormat_label_616:
/* @619 */ 	ldy          #1
/* @620 */ 	lda         (__i2), Y
/* @626 */ 	sta         __i3
/* @627 */ 	and          #128
/* @629 */ 	beq         .CollectFormat_label_628
/* @630 */ 	lda          #255
.CollectFormat_label_628:
/* @631 */ 	sta         __i3+1
/* @634 */ 	lda         __i3
/* @635 */ 	cmp          #108
/* @636 */ 	bne         .CollectFormat_label_651
/* @637 */ 	lda         __i3+1
/* @639 */ 	bne         .CollectFormat_label_651
/* @641 */ 	lda          #4
/* @642 */ 	ldy          #13
/* @643 */ 	sta         (__i0), Y
/* @644 */ 	lda          #0
/* @646 */ 	iny         
/* @647 */ 	sta         (__i0), Y
/* @648 */ 	lda          #__i2
/* @649 */ 	jsr         __rinc21
/* @650 */ 	bra         .CollectFormat_label_658
.CollectFormat_label_651:
/* @652 */ 	lda          #3
/* @653 */ 	ldy          #13
/* @654 */ 	sta         (__i0), Y
/* @655 */ 	lda          #0
/* @656 */ 	iny         
/* @657 */ 	sta         (__i0), Y
.CollectFormat_label_658:
/* @659 */ 	lda          #__i2
/* @660 */ 	jsr         __rinc21
/* @661 */ 	jmp         .CollectFormat_label_747
.CollectFormat_label_662:
/* @665 */ 	ldy          #1
/* @666 */ 	lda         (__i2), Y
/* @672 */ 	sta         __i3
/* @673 */ 	and          #128
/* @675 */ 	beq         .CollectFormat_label_674
/* @676 */ 	lda          #255
.CollectFormat_label_674:
/* @677 */ 	sta         __i3+1
/* @680 */ 	lda         __i3
/* @681 */ 	cmp          #104
/* @682 */ 	bne         .CollectFormat_label_696
/* @683 */ 	lda         __i3+1
/* @685 */ 	bne         .CollectFormat_label_696
/* @687 */ 	lda          #1
/* @688 */ 	ldy          #13
/* @689 */ 	sta         (__i0), Y
/* @690 */ 	dec          A
/* @691 */ 	iny         
/* @692 */ 	sta         (__i0), Y
/* @693 */ 	lda          #__i2
/* @694 */ 	jsr         __rinc21
/* @695 */ 	bra         .CollectFormat_label_703
.CollectFormat_label_696:
/* @697 */ 	lda          #2
/* @698 */ 	ldy          #13
/* @699 */ 	sta         (__i0), Y
/* @700 */ 	lda          #0
/* @701 */ 	iny         
/* @702 */ 	sta         (__i0), Y
.CollectFormat_label_703:
/* @704 */ 	lda          #__i2
/* @705 */ 	jsr         __rinc21
/* @706 */ 	bra         .CollectFormat_label_747
.CollectFormat_label_707:
/* @708 */ 	lda          #5
/* @709 */ 	ldy          #13
/* @710 */ 	sta         (__i0), Y
/* @711 */ 	lda          #0
/* @712 */ 	iny         
/* @713 */ 	sta         (__i0), Y
/* @714 */ 	lda          #__i2
/* @715 */ 	jsr         __rinc21
/* @716 */ 	bra         .CollectFormat_label_747
.CollectFormat_label_717:
/* @718 */ 	lda          #7
/* @719 */ 	ldy          #13
/* @720 */ 	sta         (__i0), Y
/* @721 */ 	lda          #0
/* @722 */ 	iny         
/* @723 */ 	sta         (__i0), Y
/* @724 */ 	lda          #__i2
/* @725 */ 	jsr         __rinc21
/* @726 */ 	bra         .CollectFormat_label_747
.CollectFormat_label_727:
/* @728 */ 	lda          #6
/* @729 */ 	ldy          #13
/* @730 */ 	sta         (__i0), Y
/* @731 */ 	lda          #0
/* @732 */ 	iny         
/* @733 */ 	sta         (__i0), Y
/* @734 */ 	lda          #__i2
/* @735 */ 	jsr         __rinc21
/* @736 */ 	bra         .CollectFormat_label_747
.CollectFormat_label_737:
/* @738 */ 	lda          #8
/* @739 */ 	ldy          #13
/* @740 */ 	sta         (__i0), Y
/* @741 */ 	lda          #0
/* @742 */ 	iny         
/* @743 */ 	sta         (__i0), Y
/* @744 */ 	lda          #__i2
/* @745 */ 	jsr         __rinc21
.CollectFormat_label_747:
/* @748 */ 	ldx          #8
	jsr          __load_result
/* @749 */ 	lda         #__i2
/* @750 */ 	jsr         __result2
/* @751 */ 	jmp         .CollectFormat_label_271
.func_end_CollectFormat:
	.size CollectFormat, .func_end_CollectFormat-CollectFormat

	.local  ResolvePrecision
	.type ResolvePrecision, @function

ResolvePrecision:
/* @3 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @7 */ 	ldx          #0
	jsr          __arg_value2_i0			// fmt
/* @11 */ 	ldx          #2
	jsr          __arg_value2_i1			// ap
/* @15 */ 	ldy          #2
/* @16 */ 	lda         (__i0), Y
/* @18 */ 	sta         __i2
/* @20 */ 	iny         
/* @21 */ 	lda         (__i0), Y
/* @23 */ 	sta         __i2+1
/* @26 */ 	lda         __i2
/* @27 */ 	cmp          #254
/* @28 */ 	bne         .ResolvePrecision_label_56
/* @29 */ 	lda         __i2+1
/* @30 */ 	cmp          #255
/* @31 */ 	bne         .ResolvePrecision_label_56
/* @33 */ 	lda         __i1
/* @35 */ 	sta         __mem_src
/* @36 */ 	lda         __i1+1
/* @38 */ 	sta         __mem_src+1
/* @41 */ 	lda         #__i2
/* @43 */ 	sta         __mem_dest
/* @45 */ 	stz         __mem_dest+1
/* @47 */ 	jsr         __builtin_va_arg2
/* @50 */ 	lda         __i2
/* @51 */ 	ldy          #2
/* @52 */ 	sta         (__i0), Y
/* @53 */ 	lda         __i2+1
/* @54 */ 	iny         
/* @55 */ 	sta         (__i0), Y
.ResolvePrecision_label_56:
/* @57 */ 	ldy          #8
	jmp          __leave_leaf_void_nomask
.func_end_ResolvePrecision:
	.size ResolvePrecision, .func_end_ResolvePrecision-ResolvePrecision

	.local  ResolveFieldWidth
	.type ResolveFieldWidth, @function

ResolveFieldWidth:
/* @3 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @7 */ 	ldx          #0
	jsr          __arg_value2_i0			// fmt
/* @11 */ 	ldx          #2
	jsr          __arg_value2_i1			// ap
/* @15 */ 	lda         (__i0)
/* @16 */ 	sta         __i2
/* @18 */ 	ldy          #1
/* @19 */ 	lda         (__i0), Y
/* @20 */ 	sta         __i2+1
/* @23 */ 	lda         __i2
/* @24 */ 	cmp          #254
/* @25 */ 	bne         .ResolveFieldWidth_label_53
/* @26 */ 	lda         __i2+1
/* @27 */ 	cmp          #255
/* @28 */ 	bne         .ResolveFieldWidth_label_53
/* @31 */ 	lda         __i1
/* @33 */ 	sta         __mem_src
/* @34 */ 	lda         __i1+1
/* @36 */ 	sta         __mem_src+1
/* @39 */ 	lda         #__i2
/* @41 */ 	sta         __mem_dest
/* @43 */ 	stz         __mem_dest+1
/* @45 */ 	jsr         __builtin_va_arg2
/* @48 */ 	lda         __i2
/* @49 */ 	sta         (__i0)
/* @50 */ 	lda         __i2+1
/* @51 */ 	ldy          #1
/* @52 */ 	sta         (__i0), Y
.ResolveFieldWidth_label_53:
/* @54 */ 	ldy          #8
	jmp          __leave_leaf_void_nomask
.func_end_ResolveFieldWidth:
	.size ResolveFieldWidth, .func_end_ResolveFieldWidth-ResolveFieldWidth

	.local  FixFloatPrecision
	.type FixFloatPrecision, @function

FixFloatPrecision:
/* @5 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @9 */ 	ldx          #0
	jsr          __arg_value2_i0			// fmt
/* @13 */ 	ldx          #2
	jsr          __arg_value2_i1			// max
/* @17 */ 	ldy          #2
/* @18 */ 	lda         (__i0), Y
/* @20 */ 	sta         __i2
/* @22 */ 	iny         
/* @23 */ 	lda         (__i0), Y
/* @25 */ 	sta         __i2+1
/* @28 */ 	lda         __i2
/* @29 */ 	cmp          #255
/* @30 */ 	bne         .FixFloatPrecision_label_41
/* @31 */ 	lda         __i2+1
/* @32 */ 	cmp          #255
/* @33 */ 	bne         .FixFloatPrecision_label_41
/* @35 */ 	lda          #6
/* @36 */ 	ldy          #2
/* @37 */ 	sta         (__i0), Y
/* @38 */ 	lda          #0
/* @39 */ 	iny         
/* @40 */ 	sta         (__i0), Y
.FixFloatPrecision_label_41:
/* @44 */ 	ldy          #2
/* @45 */ 	lda         (__i0), Y
/* @46 */ 	sta         __i2
/* @47 */ 	iny         
/* @48 */ 	lda         (__i0), Y
/* @49 */ 	sta         __i2+1
/* @52 */ 	lda          #0
/* @53 */ 	cmp         __i2
/* @55 */ 	sbc         __i2+1
/* @56 */ 	bvc         .FixFloatPrecision_label_51
/* @58 */ 	eor          #128
.FixFloatPrecision_label_51:
/* @59 */ 	bmi         .FixFloatPrecision_label_66
/* @60 */ 	lda          #6
/* @61 */ 	ldy          #2
/* @62 */ 	sta         (__i0), Y
/* @63 */ 	lda          #0
/* @64 */ 	iny         
/* @65 */ 	sta         (__i0), Y
.FixFloatPrecision_label_66:
/* @69 */ 	ldy          #2
/* @70 */ 	lda         (__i0), Y
/* @71 */ 	sta         __i2
/* @72 */ 	iny         
/* @73 */ 	lda         (__i0), Y
/* @74 */ 	sta         __i2+1
/* @77 */ 	lda         __i1
/* @78 */ 	cmp         __i2
/* @79 */ 	lda         __i1+1
/* @80 */ 	sbc         __i2+1
/* @81 */ 	bvc         .FixFloatPrecision_label_76
/* @82 */ 	eor          #128
.FixFloatPrecision_label_76:
/* @83 */ 	bpl         .FixFloatPrecision_label_90
/* @84 */ 	lda         __i1
/* @85 */ 	ldy          #2
/* @86 */ 	sta         (__i0), Y
/* @87 */ 	lda         __i1+1
/* @88 */ 	iny         
/* @89 */ 	sta         (__i0), Y
.FixFloatPrecision_label_90:
/* @91 */ 	ldy          #8
	jmp          __leave_leaf_void_nomask
.func_end_FixFloatPrecision:
	.size FixFloatPrecision, .func_end_FixFloatPrecision-FixFloatPrecision

	.local  ConvertDecimalLongLong
	.type ConvertDecimalLongLong, @function

ConvertDecimalLongLong:
/* @12 */ 	stx         __result
/* @14 */ 	sty         __result+1
/* @15 */ 	ldx          #23
	jsr          __enter
	.byte        0x22,0x40,0x00		// Save mask i:2 b:1 l:0 x:2 f:0 
/* @23 */ 	ldx          #8
	jsr          __arg_value2_i0			// buf
/* @27 */ 	ldx          #10
	jsr          __arg_value2_i1			// buflen
/* @31 */ 	ldx          #0
	jsr          __arg_value8_x1			// v
/* @38 */ 	sec         
/* @40 */ 	lda         __i1
/* @41 */ 	sbc          #1
/* @42 */ 	sta         __i2
/* @44 */ 	lda         __i1+1
/* @45 */ 	sbc          #0
/* @46 */ 	sta         __i2+1
/* @50 */ 	clc         
/* @51 */ 	lda         __i0
/* @52 */ 	adc         __i2
/* @53 */ 	sta         __i3
/* @54 */ 	lda         __i0+1
/* @55 */ 	adc         __i2+1
/* @56 */ 	sta         __i3+1
/* @59 */ 	lda         __i3
/* @60 */ 	sta         __i4
/* @61 */ 	lda         __i3+1
/* @62 */ 	sta         __i4+1
/* @63 */ 	lda         __x1
/* @64 */ 	ora         __x1+1
/* @66 */ 	ora         __x1+2
/* @68 */ 	ora         __x1+3
/* @70 */ 	ora         __x1+4
/* @72 */ 	ora         __x1+5
/* @74 */ 	ora         __x1+6
/* @76 */ 	ora         __x1+7
/* @77 */ 	bne         .ConvertDecimalLongLong_label_88
/* @78 */ 	lda          #48
/* @79 */ 	ldy          #0
/* @80 */ 	sta         (__i4)
/* @81 */ 	ldx          #24
	jsr          __load_result
/* @82 */ 	lda         #__i4
/* @84 */ 	jsr         __result2
.ConvertDecimalLongLong_label_85:
/* @86 */ 	ldy          #26
	jmp          __leave
.ConvertDecimalLongLong_label_88:
.ConvertDecimalLongLong_label_89:
/* @90 */ 	lda         __x1
/* @91 */ 	ora         __x1+1
/* @92 */ 	ora         __x1+2
/* @93 */ 	ora         __x1+3
/* @94 */ 	ora         __x1+4
/* @95 */ 	ora         __x1+5
/* @96 */ 	ora         __x1+6
/* @97 */ 	ora         __x1+7
/* @99 */ 	beq         .ConvertDecimalLongLong_label_185
/* @101 */ 	ldx          #19
	jsr          __var_addr_i5			// qr
/* @102 */ 	ldx         #%lo(.lit.31)
	ldy         #%hi(.lit.31)
/* @104 */ 	jsr         __push8xy
/* @105 */ 	jsr         __pushx1
/* @108 */ 	jsr         __pushi5
/* @109 */ 	jsr         lldiv
/* @111 */ 	jsr         __pullxy
/* @112 */ 	stx         __i5
/* @113 */ 	sty         __i5+1
/* @115 */ 	jsr         __incsp16
/* @122 */ 	ldy          #15
/* @123 */ 	ldx          #7
.ConvertDecimalLongLong_label_124:
/* @126 */ 	lda         (__i5), Y
/* @127 */ 	sta         __x0, X
/* @128 */ 	dey         
/* @129 */ 	dex         
/* @130 */ 	bpl         .ConvertDecimalLongLong_label_124
/* @134 */ 	clc         
/* @137 */ 	ldy          #8
/* @138 */ 	ldx          #0
.ConvertDecimalLongLong_label_139:
/* @140 */ 	lda         __x0, X
/* @141 */ 	adc         .lit.35, X
/* @142 */ 	sta         __x2, X
/* @143 */ 	inx         
/* @144 */ 	dey         
/* @145 */ 	bne         .ConvertDecimalLongLong_label_139
/* @148 */ 	lda         __x2
/* @149 */ 	sta         __b2
/* @152 */ 	lda         __i4
/* @153 */ 	sta         __i0
/* @154 */ 	lda         __i4+1
/* @155 */ 	sta         __i0+1
/* @156 */ 	lda          #__i4
/* @158 */ 	jsr         __rdec21
/* @161 */ 	lda         __b2
/* @162 */ 	sta         (__i0)
/* @168 */ 	ldy          #7
/* @169 */ 	ldx          #7
.ConvertDecimalLongLong_label_170:
/* @171 */ 	lda         (__i5), Y
/* @172 */ 	sta         __x0, X
/* @173 */ 	dey         
/* @174 */ 	dex         
/* @175 */ 	bpl         .ConvertDecimalLongLong_label_170
/* @178 */ 	ldx          #7
.ConvertDecimalLongLong_label_179:
/* @180 */ 	lda         __x0, X
/* @181 */ 	sta         __x1, X
/* @182 */ 	dex         
/* @183 */ 	bpl         .ConvertDecimalLongLong_label_179
/* @184 */ 	bra         .ConvertDecimalLongLong_label_89
.ConvertDecimalLongLong_label_185:
/* @188 */ 	clc         
/* @189 */ 	lda         __i4
/* @190 */ 	adc          #1
/* @191 */ 	sta         __i0
/* @192 */ 	lda         __i4+1
/* @193 */ 	adc          #0
/* @194 */ 	sta         __i0+1
/* @196 */ 	ldx          #24
	jsr          __load_result
/* @197 */ 	lda         #__i0
/* @198 */ 	jsr         __result2
/* @199 */ 	jmp         .ConvertDecimalLongLong_label_85
.func_end_ConvertDecimalLongLong:
	.size ConvertDecimalLongLong, .func_end_ConvertDecimalLongLong-ConvertDecimalLongLong

	.local  ConvertHexLongLong
	.type ConvertHexLongLong, @function

ConvertHexLongLong:
/* @18 */ 	stx         __result
/* @20 */ 	sty         __result+1
/* @21 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x41,0x20,0x00		// Save mask i:1 b:2 l:0 x:1 f:0 
/* @28 */ 	ldx          #8
	jsr          __arg_value2_i1			// buf
/* @32 */ 	ldx          #10
	jsr          __arg_value2_i2			// buflen
/* @36 */ 	ldx          #0
	jsr          __arg_value8_x0			// v
/* @45 */ 	sec         
/* @47 */ 	lda         __i2
/* @48 */ 	sbc          #1
/* @49 */ 	sta         __i3
/* @51 */ 	lda         __i2+1
/* @52 */ 	sbc          #0
/* @53 */ 	sta         __i3+1
/* @57 */ 	clc         
/* @58 */ 	lda         __i1
/* @59 */ 	adc         __i3
/* @60 */ 	sta         __i4
/* @61 */ 	lda         __i1+1
/* @62 */ 	adc         __i3+1
/* @63 */ 	sta         __i4+1
/* @66 */ 	lda         __i4
/* @67 */ 	sta         __i0
/* @68 */ 	lda         __i4+1
/* @69 */ 	sta         __i0+1
/* @70 */ 	lda         __x0
/* @71 */ 	ora         __x0+1
/* @73 */ 	ora         __x0+2
/* @75 */ 	ora         __x0+3
/* @77 */ 	ora         __x0+4
/* @79 */ 	ora         __x0+5
/* @81 */ 	ora         __x0+6
/* @83 */ 	ora         __x0+7
/* @84 */ 	bne         .ConvertHexLongLong_label_94
/* @85 */ 	lda          #48
/* @86 */ 	ldy          #0
/* @87 */ 	sta         (__i0)
/* @88 */ 	lda         #__i0
/* @90 */ 	jsr         __result2
.ConvertHexLongLong_label_91:
/* @92 */ 	ldy          #8
	jmp          __leave_leaf
.ConvertHexLongLong_label_94:
.ConvertHexLongLong_label_95:
/* @96 */ 	lda         __x0
/* @97 */ 	ora         __x0+1
/* @98 */ 	ora         __x0+2
/* @99 */ 	ora         __x0+3
/* @100 */ 	ora         __x0+4
/* @101 */ 	ora         __x0+5
/* @102 */ 	ora         __x0+6
/* @103 */ 	ora         __x0+7
/* @105 */ 	bne         .ConvertHexLongLong_label_293
/* @294 */ 	jmp         .ConvertHexLongLong_label_279
.ConvertHexLongLong_label_293:
/* @108 */ 	lda         __x0
/* @109 */ 	and          #15
/* @112 */ 	stz         __x1+1
/* @114 */ 	stz         __x1+2
/* @116 */ 	stz         __x1+3
/* @118 */ 	stz         __x1+4
/* @120 */ 	stz         __x1+5
/* @122 */ 	stz         __x1+6
/* @124 */ 	stz         __x1+7
/* @128 */ 	sta         __b1
/* @131 */ 	sta         __i1
/* @133 */ 	stz         __i1+1
/* @136 */ 	lda          #0
/* @137 */ 	cmp         __i1+1
/* @138 */ 	bcc         .ConvertHexLongLong_label_135
/* @139 */ 	bne         .ConvertHexLongLong_label_197
/* @140 */ 	lda          #9
/* @141 */ 	cmp         __i1
/* @142 */ 	bcs         .ConvertHexLongLong_label_197
.ConvertHexLongLong_label_135:
/* @145 */ 	lda         __b1
/* @146 */ 	sta         __i1
/* @148 */ 	stz         __i1+1
/* @152 */ 	sec         
/* @154 */ 	sbc          #10
/* @155 */ 	sta         __i2
/* @156 */ 	lda         __i1+1
/* @157 */ 	sbc          #0
/* @158 */ 	sta         __i2+1
/* @160 */ 	ldx          #12
	jsr          __arg_value1_b3			// upper
/* @162 */ 	lda         __b3
/* @164 */ 	beq         .ConvertHexLongLong_label_169
/* @166 */ 	lda          #65
/* @167 */ 	sta         __b0
/* @168 */ 	bra         .ConvertHexLongLong_label_173
.ConvertHexLongLong_label_169:
/* @171 */ 	lda          #97
/* @172 */ 	sta         __b0
.ConvertHexLongLong_label_173:
/* @177 */ 	lda         __b0
/* @178 */ 	sta         __i1
/* @180 */ 	stz         __i1+1
/* @185 */ 	clc         
/* @186 */ 	lda         __i2
/* @187 */ 	adc         __i1
/* @188 */ 	sta         __i3
/* @189 */ 	lda         __i2+1
/* @190 */ 	adc         __i1+1
/* @191 */ 	sta         __i3+1
/* @194 */ 	lda         __i3
/* @195 */ 	sta         __b2
/* @196 */ 	bra         .ConvertHexLongLong_label_217
.ConvertHexLongLong_label_197:
/* @199 */ 	lda         __b1
/* @200 */ 	sta         __i1
/* @202 */ 	stz         __i1+1
/* @206 */ 	clc         
/* @208 */ 	adc          #48
/* @209 */ 	sta         __i2
/* @210 */ 	lda         __i1+1
/* @211 */ 	adc          #0
/* @212 */ 	sta         __i2+1
/* @215 */ 	lda         __i2
/* @216 */ 	sta         __b2
.ConvertHexLongLong_label_217:
/* @220 */ 	lda         __i0
/* @221 */ 	sta         __i1
/* @222 */ 	lda         __i0+1
/* @223 */ 	sta         __i1+1
/* @224 */ 	lda          #__i0
/* @226 */ 	jsr         __rdec21
/* @229 */ 	lda         __b2
/* @230 */ 	sta         (__i1)
/* @233 */ 	lda         __x0+7
/* @234 */ 	lsr          A
/* @235 */ 	sta         __x1+7
/* @236 */ 	lda         __x0+6
/* @237 */ 	ror          A
/* @238 */ 	sta         __x1+6
/* @239 */ 	lda         __x0+5
/* @240 */ 	ror          A
/* @241 */ 	sta         __x1+5
/* @242 */ 	lda         __x0+4
/* @243 */ 	ror          A
/* @244 */ 	sta         __x1+4
/* @245 */ 	lda         __x0+3
/* @246 */ 	ror          A
/* @247 */ 	sta         __x1+3
/* @248 */ 	lda         __x0+2
/* @249 */ 	ror          A
/* @250 */ 	sta         __x1+2
/* @251 */ 	lda         __x0+1
/* @252 */ 	ror          A
/* @253 */ 	sta         __x1+1
/* @254 */ 	lda         __x0
/* @255 */ 	ror          A
/* @256 */ 	sta         __x1
/* @257 */ 	ldx          #3
.ConvertHexLongLong_label_258:
/* @259 */ 	lsr         __x1+7
/* @260 */ 	ror         __x1+6
/* @261 */ 	ror         __x1+5
/* @262 */ 	ror         __x1+4
/* @263 */ 	ror         __x1+3
/* @264 */ 	ror         __x1+2
/* @265 */ 	ror         __x1+1
/* @266 */ 	ror         __x1
/* @267 */ 	dex         
/* @268 */ 	bne         .ConvertHexLongLong_label_258
/* @271 */ 	ldx          #7
.ConvertHexLongLong_label_272:
/* @274 */ 	lda         __x1, X
/* @275 */ 	sta         __x0, X
/* @276 */ 	dex         
/* @277 */ 	bpl         .ConvertHexLongLong_label_272
/* @278 */ 	jmp         .ConvertHexLongLong_label_95
.ConvertHexLongLong_label_279:
/* @282 */ 	clc         
/* @283 */ 	lda         __i0
/* @284 */ 	adc          #1
/* @285 */ 	sta         __i1
/* @286 */ 	lda         __i0+1
/* @287 */ 	adc          #0
/* @288 */ 	sta         __i1+1
/* @290 */ 	lda         #__i1
/* @291 */ 	jsr         __result2
/* @292 */ 	jmp         .ConvertHexLongLong_label_91
.func_end_ConvertHexLongLong:
	.size ConvertHexLongLong, .func_end_ConvertHexLongLong-ConvertHexLongLong

	.local  ConvertHexPointer
	.type ConvertHexPointer, @function

ConvertHexPointer:
/* @20 */ 	stx         __result
/* @22 */ 	sty         __result+1
/* @23 */ 	ldx          #8
	jsr          __enter
	.byte        0x65,0x00,0x00		// Save mask i:5 b:3 l:0 x:0 f:0 
/* @28 */ 	ldx          #6
	jsr          __arg_value1_b3			// upper
/* @35 */ 	ldx          #2
	jsr          __arg_value2_i5			// buf
/* @39 */ 	ldx          #4
	jsr          __arg_value2_i6			// buflen
/* @48 */ 	ldx          #0
	jsr          __arg_value2_i8			// ptr
/* @51 */ 	lda         __i8
/* @53 */ 	bne         .ConvertHexPointer_label_102
/* @55 */ 	lda         __i8+1
/* @57 */ 	bne         .ConvertHexPointer_label_102
/* @61 */ 	sec         
/* @62 */ 	lda         __i6
/* @63 */ 	sbc          #6
/* @64 */ 	sta         __i0
/* @65 */ 	lda         __i6+1
/* @66 */ 	sbc          #0
/* @67 */ 	sta         __i0+1
/* @71 */ 	clc         
/* @72 */ 	lda         __i5
/* @73 */ 	adc         __i0
/* @74 */ 	sta         __i1
/* @75 */ 	lda         __i5+1
/* @76 */ 	adc         __i0+1
/* @77 */ 	sta         __i1+1
/* @80 */ 	lda         __i1
/* @81 */ 	sta         __i4
/* @82 */ 	lda         __i1+1
/* @83 */ 	sta         __i4+1
/* @85 */ 	ldx         #%lo(.str.44)
	ldy         #%hi(.str.44)
/* @87 */ 	jsr         __pushxy
/* @88 */ 	jsr         __pushi4
/* @90 */ 	ldx         #__i0
/* @91 */ 	ldy          #0
/* @92 */ 	jsr         strcpy
/* @94 */ 	jsr         __incsp4
/* @95 */ 	ldx          #9
	jsr          __load_result
/* @96 */ 	lda         #__i4
/* @98 */ 	jsr         __result2
.ConvertHexPointer_label_99:
/* @100 */ 	ldy          #11
	jmp          __leave
.ConvertHexPointer_label_102:
/* @105 */ 	sec         
/* @106 */ 	lda         __i6
/* @107 */ 	sbc          #1
/* @108 */ 	sta         __i0
/* @109 */ 	lda         __i6+1
/* @110 */ 	sbc          #0
/* @111 */ 	sta         __i0+1
/* @115 */ 	clc         
/* @116 */ 	lda         __i5
/* @117 */ 	adc         __i0
/* @118 */ 	sta         __i1
/* @119 */ 	lda         __i5+1
/* @120 */ 	adc         __i0+1
/* @121 */ 	sta         __i1+1
/* @124 */ 	lda         __i1
/* @125 */ 	sta         __i4
/* @126 */ 	lda         __i1+1
/* @127 */ 	sta         __i4+1
/* @128 */ 	lda         __i8
/* @129 */ 	sta         __i7
/* @130 */ 	lda         __i8+1
/* @131 */ 	sta         __i7+1
.ConvertHexPointer_label_132:
/* @133 */ 	lda         __i7
/* @134 */ 	ora         __i7+1
/* @136 */ 	bne         .ConvertHexPointer_label_296
/* @297 */ 	jmp         .ConvertHexPointer_label_275
.ConvertHexPointer_label_296:
/* @139 */ 	lda         __i7
/* @140 */ 	and          #15
/* @143 */ 	stz         __i0+1
/* @147 */ 	sta         __b4
/* @150 */ 	sta         __i0
/* @152 */ 	stz         __i0+1
/* @155 */ 	lda          #0
/* @156 */ 	cmp         __i0+1
/* @157 */ 	bcc         .ConvertHexPointer_label_154
/* @158 */ 	bne         .ConvertHexPointer_label_215
/* @159 */ 	lda          #9
/* @160 */ 	cmp         __i0
/* @161 */ 	bcs         .ConvertHexPointer_label_215
.ConvertHexPointer_label_154:
/* @164 */ 	lda         __b4
/* @165 */ 	sta         __i0
/* @167 */ 	stz         __i0+1
/* @171 */ 	sec         
/* @173 */ 	sbc          #10
/* @174 */ 	sta         __i1
/* @175 */ 	lda         __i0+1
/* @176 */ 	sbc          #0
/* @177 */ 	sta         __i1+1
/* @178 */ 	lda         __b3
/* @180 */ 	beq         .ConvertHexPointer_label_185
/* @182 */ 	lda          #65
/* @183 */ 	sta         __b2
/* @184 */ 	bra         .ConvertHexPointer_label_189
.ConvertHexPointer_label_185:
/* @187 */ 	lda          #97
/* @188 */ 	sta         __b2
.ConvertHexPointer_label_189:
/* @193 */ 	lda         __b2
/* @194 */ 	sta         __i0
/* @196 */ 	stz         __i0+1
/* @201 */ 	clc         
/* @202 */ 	lda         __i1
/* @203 */ 	adc         __i0
/* @204 */ 	sta         __i2
/* @205 */ 	lda         __i1+1
/* @206 */ 	adc         __i0+1
/* @207 */ 	sta         __i2+1
/* @209 */ 	lda          #__i2
/* @211 */ 	ldx          #4
/* @213 */ 	jsr         __set_var_value1
/* @214 */ 	bra         .ConvertHexPointer_label_235
.ConvertHexPointer_label_215:
/* @217 */ 	lda         __b4
/* @218 */ 	sta         __i0
/* @220 */ 	stz         __i0+1
/* @224 */ 	clc         
/* @226 */ 	adc          #48
/* @227 */ 	sta         __i1
/* @228 */ 	lda         __i0+1
/* @229 */ 	adc          #0
/* @230 */ 	sta         __i1+1
/* @232 */ 	lda          #__i1
/* @233 */ 	ldx          #4
/* @234 */ 	jsr         __set_var_value1
.ConvertHexPointer_label_235:
/* @238 */ 	lda         __i4
/* @239 */ 	sta         __i0
/* @240 */ 	lda         __i4+1
/* @241 */ 	sta         __i0+1
/* @242 */ 	lda          #__i4
/* @244 */ 	jsr         __rdec21
/* @246 */ 	ldx          #4
	jsr          __var_value1_b0			// ch
/* @251 */ 	lda         __b0
/* @252 */ 	sta         (__i0)
/* @255 */ 	lda         __i7+1
/* @256 */ 	lsr          A
/* @257 */ 	sta         __i0+1
/* @258 */ 	lda         __i7
/* @259 */ 	ror          A
/* @260 */ 	sta         __i0
/* @262 */ 	ldx          #3
.ConvertHexPointer_label_263:
/* @264 */ 	lsr         __i0+1
/* @265 */ 	ror         __i0
/* @266 */ 	dex         
/* @267 */ 	bne         .ConvertHexPointer_label_263
/* @270 */ 	lda         __i0
/* @271 */ 	sta         __i7
/* @272 */ 	lda         __i0+1
/* @273 */ 	sta         __i7+1
/* @274 */ 	jmp         .ConvertHexPointer_label_132
.ConvertHexPointer_label_275:
/* @278 */ 	lda         __i4
/* @279 */ 	sta         __i0
/* @280 */ 	lda         __i4+1
/* @281 */ 	sta         __i0+1
/* @282 */ 	lda          #__i4
/* @283 */ 	jsr         __rdec21
/* @286 */ 	lda          #120
/* @287 */ 	ldy          #0
/* @288 */ 	sta         (__i0)
/* @289 */ 	lda          #48
/* @291 */ 	sta         (__i4)
/* @292 */ 	ldx          #9
	jsr          __load_result
/* @293 */ 	lda         #__i4
/* @294 */ 	jsr         __result2
/* @295 */ 	jmp         .ConvertHexPointer_label_99
.func_end_ConvertHexPointer:
	.size ConvertHexPointer, .func_end_ConvertHexPointer-ConvertHexPointer

	.local  Pad
	.type Pad, @function

Pad:
/* @8 */ 	stx         __result
/* @10 */ 	sty         __result+1
/* @11 */ 	ldx          #7
	jsr          __enter
	.byte        0x06,0x00,0x00		// Save mask i:6 b:0 l:0 x:0 f:0 
/* @22 */ 	ldx          #6
	jsr          __arg_value1_b0			// zero
/* @26 */ 	ldx          #4
	jsr          __arg_value2_i6			// n
/* @32 */ 	ldx          #0
	jsr          __arg_value2_i8			// writer
/* @36 */ 	ldx          #2
	jsr          __arg_value2_i9			// data
/* @38 */ 	lda          #9
/* @39 */ 	sta         __i0
/* @42 */ 	stz         __i0+1
/* @43 */ 	lda         __b0
/* @45 */ 	beq         .Pad_label_52
/* @47 */ 	lda         #%lo(zeroes)
/* @48 */ 	sta         __i4
/* @49 */ 	lda         #%hi(zeroes)
/* @50 */ 	sta         __i4+1
/* @51 */ 	bra         .Pad_label_58
.Pad_label_52:
/* @54 */ 	lda         #%lo(spaces)
/* @55 */ 	sta         __i4
/* @56 */ 	lda         #%hi(spaces)
/* @57 */ 	sta         __i4+1
.Pad_label_58:
/* @61 */ 	lda         __i4
/* @62 */ 	sta         __i5
/* @63 */ 	lda         __i4+1
/* @64 */ 	sta         __i5+1
.Pad_label_65:
/* @67 */ 	lda          #0
/* @68 */ 	cmp         __i6
/* @70 */ 	sbc         __i6+1
/* @71 */ 	bvc         .Pad_label_66
/* @73 */ 	eor          #128
.Pad_label_66:
/* @74 */ 	bpl         .Pad_label_121
/* @75 */ 	lda         __i6
/* @76 */ 	sta         __i7
/* @77 */ 	lda         __i6+1
/* @78 */ 	sta         __i7+1
/* @80 */ 	lda          #9
/* @81 */ 	cmp         __i7
/* @82 */ 	lda          #0
/* @83 */ 	sbc         __i7+1
/* @84 */ 	bvc         .Pad_label_79
/* @85 */ 	eor          #128
.Pad_label_79:
/* @86 */ 	bpl         .Pad_label_91
/* @87 */ 	lda          #9
/* @88 */ 	sta         __i7
/* @90 */ 	stz         __i7+1
.Pad_label_91:
/* @92 */ 	jsr         __pushi9
/* @93 */ 	jsr         __pushi7
/* @94 */ 	jsr         __pushi5
/* @96 */ 	ldx         #__i0
/* @97 */ 	ldy          #0
/* @100 */ 	jsr         .Pad_label_98
/* @101 */ 	bra         .Pad_label_99
.Pad_label_98:
/* @102 */ 	jmp         (__i8)
.Pad_label_99:
/* @104 */ 	jsr         __incsp6
/* @107 */ 	sec         
/* @108 */ 	lda         __i6
/* @109 */ 	sbc         __i7
/* @110 */ 	sta         __i0
/* @111 */ 	lda         __i6+1
/* @112 */ 	sbc         __i7+1
/* @113 */ 	sta         __i0+1
/* @116 */ 	lda         __i0
/* @117 */ 	sta         __i6
/* @118 */ 	lda         __i0+1
/* @119 */ 	sta         __i6+1
/* @120 */ 	bra         .Pad_label_65
.Pad_label_121:
/* @122 */ 	ldx          #8
	jsr          __load_result
/* @123 */ 	lda         #__i6
/* @125 */ 	jsr         __result2
/* @127 */ 	ldy          #10
	jmp          __leave
.func_end_Pad:
	.size Pad, .func_end_Pad-Pad

	.local  Prepend
	.type Prepend, @function

Prepend:
/* @12 */ 	stx         __result
/* @14 */ 	sty         __result+1
/* @15 */ 	ldx          #7
	jsr          __enter
	.byte        0x23,0x00,0x00		// Save mask i:3 b:1 l:0 x:0 f:0 
/* @19 */ 	ldx          #6
	jsr          __arg_value1_b0			// negative
/* @23 */ 	ldx          #8
	jsr          __arg_value1_b2			// suppress_write
/* @27 */ 	ldx          #0
	jsr          __arg_value2_i4			// writer
/* @31 */ 	ldx          #2
	jsr          __arg_value2_i5			// data
/* @35 */ 	ldx          #4
	jsr          __arg_value2_i6			// fmt
/* @36 */ 	lda         __b0
/* @38 */ 	beq         .Prepend_label_83
/* @42 */ 	lda         __b2
/* @44 */ 	beq         .Prepend_label_41
/* @46 */ 	lda          #255
.Prepend_label_41:
/* @47 */ 	inc          A
/* @52 */ 	beq         .Prepend_label_71
/* @53 */ 	jsr         __pushi5
/* @54 */ 	ldx          #1
/* @56 */ 	jsr         __pushxy0
/* @58 */ 	ldx         #%lo(.str.60)
	ldy         #%hi(.str.60)
/* @60 */ 	jsr         __pushxy
/* @62 */ 	ldx         #__i0
/* @63 */ 	ldy          #0
/* @66 */ 	jsr         .Prepend_label_64
/* @67 */ 	bra         .Prepend_label_65
.Prepend_label_64:
/* @68 */ 	jmp         (__i4)
.Prepend_label_65:
/* @70 */ 	jsr         __incsp6
.Prepend_label_71:
/* @74 */ 	lda          #1
/* @75 */ 	sta         __b0
/* @76 */ 	ldx          #8
	jsr          __load_result
/* @77 */ 	lda         #__b0
/* @79 */ 	jsr         __result1
.Prepend_label_80:
/* @81 */ 	ldy          #10
	jmp          __leave
.Prepend_label_83:
/* @87 */ 	ldy          #6
/* @88 */ 	lda         (__i6), Y
/* @93 */ 	beq         .Prepend_label_131
/* @97 */ 	lda         __b2
/* @99 */ 	beq         .Prepend_label_96
/* @100 */ 	lda          #255
.Prepend_label_96:
/* @101 */ 	inc          A
/* @106 */ 	beq         .Prepend_label_122
/* @107 */ 	jsr         __pushi5
/* @108 */ 	ldx          #1
/* @109 */ 	jsr         __pushxy0
/* @111 */ 	ldx         #%lo(.str.61)
	ldy         #%hi(.str.61)
/* @112 */ 	jsr         __pushxy
/* @114 */ 	ldx         #__i0
/* @115 */ 	ldy          #0
/* @118 */ 	jsr         .Prepend_label_116
/* @119 */ 	bra         .Prepend_label_117
.Prepend_label_116:
/* @120 */ 	jmp         (__i4)
.Prepend_label_117:
/* @121 */ 	jsr         __incsp6
.Prepend_label_122:
/* @125 */ 	lda          #1
/* @126 */ 	sta         __b0
/* @127 */ 	ldx          #8
	jsr          __load_result
/* @128 */ 	lda         #__b0
/* @129 */ 	jsr         __result1
/* @130 */ 	bra         .Prepend_label_80
.Prepend_label_131:
/* @135 */ 	ldy          #7
/* @136 */ 	lda         (__i6), Y
/* @141 */ 	beq         .Prepend_label_179
/* @145 */ 	lda         __b2
/* @147 */ 	beq         .Prepend_label_144
/* @148 */ 	lda          #255
.Prepend_label_144:
/* @149 */ 	inc          A
/* @154 */ 	beq         .Prepend_label_170
/* @155 */ 	jsr         __pushi5
/* @156 */ 	ldx          #1
/* @157 */ 	jsr         __pushxy0
/* @159 */ 	ldx         #%lo(.str.62)
	ldy         #%hi(.str.62)
/* @160 */ 	jsr         __pushxy
/* @162 */ 	ldx         #__i0
/* @163 */ 	ldy          #0
/* @166 */ 	jsr         .Prepend_label_164
/* @167 */ 	bra         .Prepend_label_165
.Prepend_label_164:
/* @168 */ 	jmp         (__i4)
.Prepend_label_165:
/* @169 */ 	jsr         __incsp6
.Prepend_label_170:
/* @173 */ 	lda          #1
/* @174 */ 	sta         __b0
/* @175 */ 	ldx          #8
	jsr          __load_result
/* @176 */ 	lda         #__b0
/* @177 */ 	jsr         __result1
/* @178 */ 	jmp         .Prepend_label_80
.Prepend_label_179:
/* @182 */ 	stz         __b0
/* @183 */ 	ldx          #8
	jsr          __load_result
/* @184 */ 	lda         #__b0
/* @185 */ 	jsr         __result1
/* @186 */ 	jmp         .Prepend_label_80
.func_end_Prepend:
	.size Prepend, .func_end_Prepend-Prepend

	.local  WriteFormatted
	.type WriteFormatted, @function

WriteFormatted:
/* @11 */ 	stx         __result
/* @13 */ 	sty         __result+1
/* @14 */ 	ldx          #7
	jsr          __enter
	.byte        0x49,0x00,0x00		// Save mask i:9 b:2 l:0 x:0 f:0 
/* @20 */ 	ldx          #6
	jsr          __arg_value2_i4			// s
/* @24 */ 	ldx          #4
	jsr          __arg_value2_i5			// fmt
/* @28 */ 	ldx          #8
	jsr          __arg_value2_i6			// len
/* @35 */ 	ldx          #10
	jsr          __arg_value1_b2			// negative
/* @39 */ 	ldx          #2
	jsr          __arg_value2_i8			// data
/* @43 */ 	ldx          #0
	jsr          __arg_value2_i9			// writer
/* @46 */ 	stz         __i10
/* @48 */ 	stz         __i10+1
/* @51 */ 	lda         (__i5)
/* @52 */ 	sta         __i0
/* @53 */ 	ldy          #1
/* @54 */ 	lda         (__i5), Y
/* @55 */ 	sta         __i0+1
/* @58 */ 	lda         __i0
/* @59 */ 	cmp          #255
/* @60 */ 	bne         .WriteFormatted_label_57
/* @61 */ 	lda         __i0+1
/* @62 */ 	cmp          #255
/* @63 */ 	bne         .WriteFormatted_label_374
/* @375 */ 	jmp         .WriteFormatted_label_335
.WriteFormatted_label_374:
.WriteFormatted_label_57:
/* @67 */ 	lda         (__i5)
/* @68 */ 	sta         __i0
/* @69 */ 	ldy          #1
/* @70 */ 	lda         (__i5), Y
/* @71 */ 	sta         __i0+1
/* @75 */ 	cmp         __i6+1
/* @76 */ 	bcs         .WriteFormatted_label_376
/* @377 */ 	jmp         .WriteFormatted_label_334
.WriteFormatted_label_376:
/* @77 */ 	bne         .WriteFormatted_label_73
/* @78 */ 	lda         __i0
/* @79 */ 	cmp         __i6
/* @80 */ 	bcs         .WriteFormatted_label_378
/* @379 */ 	jmp         .WriteFormatted_label_334
.WriteFormatted_label_378:
.WriteFormatted_label_73:
/* @84 */ 	lda         (__i5)
/* @85 */ 	sta         __i0
/* @86 */ 	ldy          #1
/* @87 */ 	lda         (__i5), Y
/* @88 */ 	sta         __i0+1
/* @92 */ 	sec         
/* @93 */ 	lda         __i0
/* @94 */ 	sbc         __i6
/* @95 */ 	sta         __i1
/* @96 */ 	lda         __i0+1
/* @97 */ 	sbc         __i6+1
/* @98 */ 	sta         __i1+1
/* @101 */ 	lda         __i1
/* @102 */ 	sta         __i7
/* @103 */ 	lda         __i1+1
/* @104 */ 	sta         __i7+1
/* @108 */ 	ldy          #4
/* @109 */ 	lda         (__i5), Y
/* @114 */ 	bne         .WriteFormatted_label_380
/* @381 */ 	jmp         .WriteFormatted_label_194
.WriteFormatted_label_380:
/* @115 */ 	lda          #0
/* @117 */ 	jsr         __pusha
/* @118 */ 	lda         __b2
/* @119 */ 	jsr         __pusha
/* @120 */ 	jsr         __pushi5
/* @121 */ 	jsr         __pushi8
/* @122 */ 	jsr         __pushi9
/* @124 */ 	ldx         #__b3
/* @125 */ 	ldy          #0
/* @126 */ 	jsr         Prepend
/* @128 */ 	jsr         __incsp10
/* @130 */ 	lda         __b3
/* @132 */ 	beq         .WriteFormatted_label_136
/* @133 */ 	lda          #__i7
/* @135 */ 	jsr         __rdec21
.WriteFormatted_label_136:
/* @137 */ 	jsr         __pushi8
/* @138 */ 	jsr         __pushi6
/* @139 */ 	jsr         __pushi4
/* @141 */ 	ldx         #__i11
/* @142 */ 	ldy          #0
/* @145 */ 	jsr         .WriteFormatted_label_143
/* @146 */ 	bra         .WriteFormatted_label_144
.WriteFormatted_label_143:
/* @147 */ 	jmp         (__i9)
.WriteFormatted_label_144:
/* @149 */ 	jsr         __incsp6
/* @153 */ 	clc         
/* @154 */ 	lda          #0
/* @155 */ 	adc         __i11
/* @156 */ 	sta         __i0
/* @157 */ 	lda          #0
/* @158 */ 	adc         __i11+1
/* @159 */ 	sta         __i0+1
/* @162 */ 	lda         __i0
/* @163 */ 	sta         __i10
/* @164 */ 	lda         __i0+1
/* @165 */ 	sta         __i10+1
/* @166 */ 	lda          #0
/* @167 */ 	jsr         __pusha
/* @168 */ 	jsr         __pushi7
/* @169 */ 	jsr         __pushi8
/* @170 */ 	jsr         __pushi9
/* @172 */ 	ldx         #__i12
/* @173 */ 	ldy          #0
/* @174 */ 	jsr         Pad
/* @176 */ 	jsr         __incsp8
/* @180 */ 	clc         
/* @181 */ 	lda         __i10
/* @182 */ 	adc         __i12
/* @183 */ 	sta         __i0
/* @184 */ 	lda         __i10+1
/* @185 */ 	adc         __i12+1
/* @186 */ 	sta         __i0+1
/* @189 */ 	lda         __i0
/* @190 */ 	sta         __i10
/* @191 */ 	lda         __i0+1
/* @192 */ 	sta         __i10+1
/* @193 */ 	jmp         .WriteFormatted_label_326
.WriteFormatted_label_194:
/* @198 */ 	ldy          #5
/* @199 */ 	lda         (__i5), Y
/* @207 */ 	beq         .WriteFormatted_label_204
/* @209 */ 	lda          #255
.WriteFormatted_label_204:
/* @210 */ 	inc          A
/* @215 */ 	jsr         __pusha
/* @216 */ 	lda         __b2
/* @217 */ 	jsr         __pusha
/* @218 */ 	jsr         __pushi5
/* @219 */ 	jsr         __pushi8
/* @220 */ 	jsr         __pushi9
/* @222 */ 	ldx         #__b3
/* @223 */ 	ldy          #0
/* @224 */ 	jsr         Prepend
/* @225 */ 	jsr         __incsp10
/* @227 */ 	lda         __b3
/* @229 */ 	beq         .WriteFormatted_label_232
/* @230 */ 	lda          #__i7
/* @231 */ 	jsr         __rdec21
.WriteFormatted_label_232:
/* @235 */ 	ldy          #5
/* @236 */ 	lda         (__i5), Y
/* @241 */ 	jsr         __pusha
/* @242 */ 	jsr         __pushi7
/* @243 */ 	jsr         __pushi8
/* @244 */ 	jsr         __pushi9
/* @246 */ 	ldx         #__i11
/* @247 */ 	ldy          #0
/* @248 */ 	jsr         Pad
/* @249 */ 	jsr         __incsp8
/* @253 */ 	clc         
/* @254 */ 	lda          #0
/* @255 */ 	adc         __i11
/* @256 */ 	sta         __i0
/* @257 */ 	lda          #0
/* @258 */ 	adc         __i11+1
/* @259 */ 	sta         __i0+1
/* @262 */ 	lda         __i0
/* @263 */ 	sta         __i10
/* @264 */ 	lda         __i0+1
/* @265 */ 	sta         __i10+1
/* @268 */ 	ldy          #5
/* @269 */ 	lda         (__i5), Y
/* @277 */ 	beq         .WriteFormatted_label_274
/* @278 */ 	lda          #255
.WriteFormatted_label_274:
/* @279 */ 	inc          A
/* @284 */ 	beq         .WriteFormatted_label_297
/* @285 */ 	lda          #0
/* @286 */ 	jsr         __pusha
/* @287 */ 	lda         __b2
/* @288 */ 	jsr         __pusha
/* @289 */ 	jsr         __pushi5
/* @290 */ 	jsr         __pushi8
/* @291 */ 	jsr         __pushi9
/* @293 */ 	ldx         #__b0
/* @294 */ 	ldy          #0
/* @295 */ 	jsr         Prepend
/* @296 */ 	jsr         __incsp10
.WriteFormatted_label_297:
/* @298 */ 	jsr         __pushi8
/* @299 */ 	jsr         __pushi6
/* @300 */ 	jsr         __pushi4
/* @302 */ 	ldx         #__i11
/* @303 */ 	ldy          #0
/* @306 */ 	jsr         .WriteFormatted_label_304
/* @307 */ 	bra         .WriteFormatted_label_305
.WriteFormatted_label_304:
/* @308 */ 	jmp         (__i9)
.WriteFormatted_label_305:
/* @309 */ 	jsr         __incsp6
/* @313 */ 	clc         
/* @314 */ 	lda         __i10
/* @315 */ 	adc         __i11
/* @316 */ 	sta         __i0
/* @317 */ 	lda         __i10+1
/* @318 */ 	adc         __i11+1
/* @319 */ 	sta         __i0+1
/* @322 */ 	lda         __i0
/* @323 */ 	sta         __i10
/* @324 */ 	lda         __i0+1
/* @325 */ 	sta         __i10+1
.WriteFormatted_label_326:
/* @327 */ 	ldx          #8
	jsr          __load_result
/* @328 */ 	lda         #__i10
/* @330 */ 	jsr         __result2
.WriteFormatted_label_331:
/* @332 */ 	ldy          #10
	jmp          __leave
.WriteFormatted_label_334:
.WriteFormatted_label_335:
/* @336 */ 	lda         __b2
/* @338 */ 	beq         .WriteFormatted_label_356
/* @339 */ 	jsr         __pushi8
/* @340 */ 	ldx          #1
/* @342 */ 	jsr         __pushxy0
/* @344 */ 	ldx         #%lo(.str.69)
	ldy         #%hi(.str.69)
/* @346 */ 	jsr         __pushxy
/* @348 */ 	ldx         #__i0
/* @349 */ 	ldy          #0
/* @352 */ 	jsr         .WriteFormatted_label_350
/* @353 */ 	bra         .WriteFormatted_label_351
.WriteFormatted_label_350:
/* @354 */ 	jmp         (__i9)
.WriteFormatted_label_351:
/* @355 */ 	jsr         __incsp6
.WriteFormatted_label_356:
/* @357 */ 	jsr         __pushi8
/* @358 */ 	jsr         __pushi6
/* @359 */ 	jsr         __pushi4
/* @361 */ 	ldx         #__i11
/* @362 */ 	ldy          #0
/* @365 */ 	jsr         .WriteFormatted_label_363
/* @366 */ 	bra         .WriteFormatted_label_364
.WriteFormatted_label_363:
/* @367 */ 	jmp         (__i9)
.WriteFormatted_label_364:
/* @368 */ 	jsr         __incsp6
/* @370 */ 	ldx          #8
	jsr          __load_result
/* @371 */ 	lda         #__i11
/* @372 */ 	jsr         __result2
/* @373 */ 	bra         .WriteFormatted_label_331
.func_end_WriteFormatted:
	.size WriteFormatted, .func_end_WriteFormatted-WriteFormatted

	.local  Printf
	.type Printf, @function

Printf:
/* @43 */ 	stx         __result
/* @45 */ 	sty         __result+1
/* @46 */ 	ldx          #31
	ldy          #1
	jsr          __enter+2
	.byte        0xab,0x20,0x01		// Save mask i:11 b:5 l:0 x:1 f:1 
/* @67 */ 	ldx          #4
	jsr          __arg_value2_i0			// format
/* @93 */ 	ldx          #2
	jsr          __arg_value2_i9			// data
/* @97 */ 	ldx          #0
	jsr          __arg_value2_i10			// writer
/* @103 */ 	ldx          #4
	ldy          #1
	jsr          __var_addrb_i1			// buf
/* @107 */ 	clc         
/* @108 */ 	lda         __i1
/* @109 */ 	adc          #0
/* @110 */ 	sta         __i2
/* @111 */ 	lda         __i1+1
/* @112 */ 	adc          #1
/* @113 */ 	sta         __i2+1
/* @115 */ 	lda          #__i2
/* @117 */ 	ldx          #27
/* @118 */ 	ldy          #1
/* @120 */ 	jsr         __set_var_value2b
/* @121 */ 	lda         __i0
/* @122 */ 	sta         __i4
/* @123 */ 	lda         __i0+1
/* @124 */ 	sta         __i4+1
/* @125 */ 	stz         __i5
/* @126 */ 	stz         __i5+1
.Printf_label_127:
/* @130 */ 	lda         (__i4)
/* @136 */ 	sta         __i0
/* @138 */ 	and          #128
/* @140 */ 	beq         .Printf_label_139
/* @142 */ 	lda          #255
.Printf_label_139:
/* @143 */ 	sta         __i0+1
/* @145 */ 	lda         __i0
/* @146 */ 	ora         __i0+1
/* @148 */ 	bne         .Printf_label_1585
/* @1586 */ 	jmp         .Printf_label_1577
.Printf_label_1585:
/* @151 */ 	lda         (__i4)
/* @157 */ 	sta         __i0
/* @158 */ 	and          #128
/* @160 */ 	beq         .Printf_label_159
/* @161 */ 	lda          #255
.Printf_label_159:
/* @162 */ 	sta         __i0+1
/* @165 */ 	lda         __i0
/* @166 */ 	cmp          #37
/* @167 */ 	beq         .Printf_label_1587
/* @1588 */ 	jmp         .Printf_label_1535
.Printf_label_1587:
/* @168 */ 	lda         __i0+1
/* @170 */ 	beq         .Printf_label_1589
/* @1590 */ 	jmp         .Printf_label_1535
.Printf_label_1589:
/* @172 */ 	lda          #__i4
/* @174 */ 	jsr         __rinc21
/* @176 */ 	lda          #19
/* @178 */ 	sta         __mem_size
/* @180 */ 	ldx          #23
	ldy          #1
	jsr          __var_addrb_i11			// fmt
/* @182 */ 	lda         __i11
/* @184 */ 	sta         __mem_dest
/* @185 */ 	lda         __i11+1
/* @187 */ 	sta         __mem_dest+1
/* @189 */ 	jsr         __zeromem1
/* @194 */ 	jsr         __pushi11
/* @195 */ 	jsr         __pushi4
/* @196 */ 	ldx         #__i4
/* @197 */ 	ldy          #0
/* @198 */ 	jsr         CollectFormat
/* @200 */ 	jsr         __incsp4
/* @201 */ 	stz         __b2
/* @202 */ 	stz         __b3
/* @204 */ 	ldx          #6
	jsr          __arg_addr_i12			// ap
/* @207 */ 	jsr         __pushi12
/* @212 */ 	jsr         __pushi11
/* @213 */ 	jsr         ResolveFieldWidth
/* @214 */ 	jsr         __incsp4
/* @219 */ 	jsr         __pushi12
/* @224 */ 	jsr         __pushi11
/* @225 */ 	jsr         ResolvePrecision
/* @226 */ 	jsr         __incsp4
/* @229 */ 	lda         (__i4)
/* @230 */ 	sta         __b4
/* @234 */ 	cmp          #88
/* @235 */ 	beq         .Printf_label_303
/* @239 */ 	lda         __b4
/* @240 */ 	cmp          #99
/* @241 */ 	bne         .Printf_label_1591
/* @1592 */ 	jmp         .Printf_label_772
.Printf_label_1591:
/* @245 */ 	lda         __b4
/* @246 */ 	cmp          #100
/* @247 */ 	beq         .Printf_label_299
/* @251 */ 	lda         __b4
/* @252 */ 	cmp          #101
/* @253 */ 	bne         .Printf_label_1593
/* @1594 */ 	jmp         .Printf_label_727
.Printf_label_1593:
/* @257 */ 	lda         __b4
/* @258 */ 	cmp          #102
/* @259 */ 	bne         .Printf_label_1595
/* @1596 */ 	jmp         .Printf_label_725
.Printf_label_1595:
/* @263 */ 	lda         __b4
/* @264 */ 	cmp          #103
/* @265 */ 	bne         .Printf_label_1597
/* @1598 */ 	jmp         .Printf_label_726
.Printf_label_1597:
/* @269 */ 	lda         __b4
/* @270 */ 	cmp          #105
/* @271 */ 	beq         .Printf_label_301
/* @275 */ 	lda         __b4
/* @276 */ 	cmp          #112
/* @277 */ 	bne         .Printf_label_1599
/* @1600 */ 	jmp         .Printf_label_703
.Printf_label_1599:
/* @281 */ 	lda         __b4
/* @282 */ 	cmp          #115
/* @283 */ 	bne         .Printf_label_1601
/* @1602 */ 	jmp         .Printf_label_751
.Printf_label_1601:
/* @287 */ 	lda         __b4
/* @288 */ 	cmp          #117
/* @289 */ 	beq         .Printf_label_300
/* @293 */ 	lda         __b4
/* @294 */ 	cmp          #120
/* @295 */ 	beq         .Printf_label_302
/* @298 */ 	jmp         .Printf_label_794
.Printf_label_299:
.Printf_label_300:
.Printf_label_301:
.Printf_label_302:
.Printf_label_303:
/* @308 */ 	lda         (__i4)
/* @314 */ 	sta         __i0
/* @315 */ 	and          #128
/* @317 */ 	beq         .Printf_label_316
/* @318 */ 	lda          #255
.Printf_label_316:
/* @319 */ 	sta         __i0+1
/* @324 */ 	ldx          #1
/* @325 */ 	lda         __i0
/* @326 */ 	cmp          #117
/* @327 */ 	bne         .Printf_label_322
/* @328 */ 	lda         __i0+1
/* @330 */ 	beq         .Printf_label_323
.Printf_label_322:
/* @331 */ 	dex         
.Printf_label_323:
/* @332 */ 	stx         __b0
/* @334 */ 	txa         
/* @335 */ 	bne         .Printf_label_363
/* @338 */ 	lda         (__i4)
/* @344 */ 	sta         __i0
/* @345 */ 	and          #128
/* @347 */ 	beq         .Printf_label_346
/* @348 */ 	lda          #255
.Printf_label_346:
/* @349 */ 	sta         __i0+1
/* @354 */ 	ldx          #1
/* @355 */ 	lda         __i0
/* @356 */ 	cmp          #120
/* @357 */ 	bne         .Printf_label_352
/* @358 */ 	lda         __i0+1
/* @360 */ 	beq         .Printf_label_353
.Printf_label_352:
/* @361 */ 	dex         
.Printf_label_353:
/* @362 */ 	stx         __b0
.Printf_label_363:
/* @365 */ 	lda         __b0
/* @366 */ 	bne         .Printf_label_394
/* @369 */ 	lda         (__i4)
/* @375 */ 	sta         __i0
/* @376 */ 	and          #128
/* @378 */ 	beq         .Printf_label_377
/* @379 */ 	lda          #255
.Printf_label_377:
/* @380 */ 	sta         __i0+1
/* @385 */ 	ldx          #1
/* @386 */ 	lda         __i0
/* @387 */ 	cmp          #88
/* @388 */ 	bne         .Printf_label_383
/* @389 */ 	lda         __i0+1
/* @391 */ 	beq         .Printf_label_384
.Printf_label_383:
/* @392 */ 	dex         
.Printf_label_384:
/* @393 */ 	stx         __b0
.Printf_label_394:
/* @397 */ 	lda         __b0
/* @398 */ 	sta         __b3
/* @405 */ 	ldy          #13
/* @406 */ 	lda         (__i11), Y
/* @407 */ 	sta         __i0
/* @409 */ 	iny         
/* @410 */ 	lda         (__i11), Y
/* @411 */ 	sta         __i0+1
/* @415 */ 	lda         __i0
/* @416 */ 	sta         __b1
/* @420 */ 	cmp          #1
/* @421 */ 	bne         .Printf_label_1603
/* @1604 */ 	jmp         .Printf_label_557
.Printf_label_1603:
/* @425 */ 	lda         __b1
/* @426 */ 	cmp          #2
/* @427 */ 	bne         .Printf_label_1605
/* @1606 */ 	jmp         .Printf_label_604
.Printf_label_1605:
/* @431 */ 	lda         __b1
/* @432 */ 	cmp          #3
/* @433 */ 	beq         .Printf_label_486
/* @437 */ 	lda         __b1
/* @438 */ 	cmp          #4
/* @439 */ 	beq         .Printf_label_532
/* @445 */ 	lda         __i12
/* @447 */ 	sta         __mem_src
/* @448 */ 	lda         __i12+1
/* @450 */ 	sta         __mem_src+1
/* @453 */ 	lda         #__i0
/* @454 */ 	sta         __mem_dest
/* @455 */ 	stz         __mem_dest+1
/* @457 */ 	jsr         __builtin_va_arg2
/* @461 */ 	lda         __i0
/* @462 */ 	sta         __x0
/* @463 */ 	lda         __i0+1
/* @464 */ 	sta         __x0+1
/* @465 */ 	and          #128
/* @467 */ 	beq         .Printf_label_466
/* @468 */ 	lda          #255
.Printf_label_466:
/* @470 */ 	ldy          #7
.Printf_label_471:
/* @473 */ 	sta         __x0, Y
/* @474 */ 	dey         
/* @475 */ 	cpy          #1
/* @476 */ 	bne         .Printf_label_471
/* @479 */ 	ldx          #7
.Printf_label_480:
/* @481 */ 	lda         __x0, X
/* @482 */ 	sta         __x1, X
/* @483 */ 	dex         
/* @484 */ 	bpl         .Printf_label_480
/* @485 */ 	jmp         .Printf_label_651
.Printf_label_486:
/* @491 */ 	lda         __i12
/* @492 */ 	sta         __mem_src
/* @493 */ 	lda         __i12+1
/* @494 */ 	sta         __mem_src+1
/* @497 */ 	lda         #__l0
/* @498 */ 	sta         __mem_dest
/* @499 */ 	stz         __mem_dest+1
/* @501 */ 	jsr         __builtin_va_arg4
/* @505 */ 	lda         __l0
/* @506 */ 	sta         __x0
/* @507 */ 	lda         __l0+1
/* @508 */ 	sta         __x0+1
/* @509 */ 	lda         __l0+2
/* @510 */ 	sta         __x0+2
/* @511 */ 	lda         __l0+3
/* @512 */ 	sta         __x0+3
/* @513 */ 	and          #128
/* @515 */ 	beq         .Printf_label_514
/* @516 */ 	lda          #255
.Printf_label_514:
/* @517 */ 	ldy          #7
.Printf_label_518:
/* @519 */ 	sta         __x0, Y
/* @520 */ 	dey         
/* @521 */ 	cpy          #3
/* @522 */ 	bne         .Printf_label_518
/* @525 */ 	ldx          #7
.Printf_label_526:
/* @527 */ 	lda         __x0, X
/* @528 */ 	sta         __x1, X
/* @529 */ 	dex         
/* @530 */ 	bpl         .Printf_label_526
/* @531 */ 	jmp         .Printf_label_651
.Printf_label_532:
/* @537 */ 	lda         __i12
/* @538 */ 	sta         __mem_src
/* @539 */ 	lda         __i12+1
/* @540 */ 	sta         __mem_src+1
/* @543 */ 	lda         #__x0
/* @544 */ 	sta         __mem_dest
/* @545 */ 	stz         __mem_dest+1
/* @547 */ 	jsr         __builtin_va_arg8
/* @550 */ 	ldx          #7
.Printf_label_551:
/* @552 */ 	lda         __x0, X
/* @553 */ 	sta         __x1, X
/* @554 */ 	dex         
/* @555 */ 	bpl         .Printf_label_551
/* @556 */ 	bra         .Printf_label_651
.Printf_label_557:
/* @561 */ 	lda         __i12
/* @562 */ 	sta         __mem_src
/* @563 */ 	lda         __i12+1
/* @564 */ 	sta         __mem_src+1
/* @567 */ 	lda         #__i0
/* @568 */ 	sta         __mem_dest
/* @569 */ 	stz         __mem_dest+1
/* @570 */ 	jsr         __builtin_va_arg2
/* @574 */ 	lda         __i0
/* @575 */ 	sta         __i1
/* @577 */ 	stz         __i1+1
/* @582 */ 	sta         __x0
/* @583 */ 	lda         __i1+1
/* @584 */ 	sta         __x0+1
/* @585 */ 	and          #128
/* @587 */ 	beq         .Printf_label_586
/* @588 */ 	lda          #255
.Printf_label_586:
/* @589 */ 	ldy          #7
.Printf_label_590:
/* @591 */ 	sta         __x0, Y
/* @592 */ 	dey         
/* @593 */ 	cpy          #1
/* @594 */ 	bne         .Printf_label_590
/* @597 */ 	ldx          #7
.Printf_label_598:
/* @599 */ 	lda         __x0, X
/* @600 */ 	sta         __x1, X
/* @601 */ 	dex         
/* @602 */ 	bpl         .Printf_label_598
/* @603 */ 	bra         .Printf_label_651
.Printf_label_604:
/* @608 */ 	lda         __i12
/* @609 */ 	sta         __mem_src
/* @610 */ 	lda         __i12+1
/* @611 */ 	sta         __mem_src+1
/* @614 */ 	lda         #__i0
/* @615 */ 	sta         __mem_dest
/* @616 */ 	stz         __mem_dest+1
/* @617 */ 	jsr         __builtin_va_arg2
/* @621 */ 	lda         __i0
/* @622 */ 	sta         __i1
/* @623 */ 	lda         __i0+1
/* @624 */ 	sta         __i1+1
/* @628 */ 	lda         __i1
/* @629 */ 	sta         __x0
/* @630 */ 	lda         __i1+1
/* @631 */ 	sta         __x0+1
/* @632 */ 	and          #128
/* @634 */ 	beq         .Printf_label_633
/* @635 */ 	lda          #255
.Printf_label_633:
/* @636 */ 	ldy          #7
.Printf_label_637:
/* @638 */ 	sta         __x0, Y
/* @639 */ 	dey         
/* @640 */ 	cpy          #1
/* @641 */ 	bne         .Printf_label_637
/* @644 */ 	ldx          #7
.Printf_label_645:
/* @646 */ 	lda         __x0, X
/* @647 */ 	sta         __x1, X
/* @648 */ 	dex         
/* @649 */ 	bpl         .Printf_label_645
.Printf_label_651:
/* @656 */ 	lda         __b3
/* @658 */ 	beq         .Printf_label_655
/* @659 */ 	lda          #255
.Printf_label_655:
/* @660 */ 	inc          A
/* @661 */ 	sta         __b5
/* @665 */ 	beq         .Printf_label_673
/* @668 */ 	ldx          #0
/* @669 */ 	lda         __x1+7
/* @670 */ 	bpl         .Printf_label_667
/* @671 */ 	inx         
.Printf_label_667:
/* @672 */ 	stx         __b5
.Printf_label_673:
/* @675 */ 	lda         __b5
/* @677 */ 	beq         .Printf_label_701
/* @678 */ 	lda          #1
/* @679 */ 	sta         __b2
/* @682 */ 	sec         
/* @684 */ 	ldy          #8
/* @685 */ 	ldx          #0
.Printf_label_686:
/* @687 */ 	lda          #0
/* @688 */ 	sbc         __x1, X
/* @689 */ 	sta         __x0, X
/* @690 */ 	inx         
/* @691 */ 	dey         
/* @692 */ 	bne         .Printf_label_686
/* @695 */ 	ldx          #7
.Printf_label_696:
/* @697 */ 	lda         __x0, X
/* @698 */ 	sta         __x1, X
/* @699 */ 	dex         
/* @700 */ 	bpl         .Printf_label_696
.Printf_label_701:
/* @702 */ 	bra         .Printf_label_794
.Printf_label_703:
/* @708 */ 	lda         __i12
/* @709 */ 	sta         __mem_src
/* @710 */ 	lda         __i12+1
/* @711 */ 	sta         __mem_src+1
/* @714 */ 	lda         #__i0
/* @715 */ 	sta         __mem_dest
/* @716 */ 	stz         __mem_dest+1
/* @717 */ 	jsr         __builtin_va_arg2
/* @720 */ 	lda         __i0
/* @721 */ 	sta         __i6
/* @722 */ 	lda         __i0+1
/* @723 */ 	sta         __i6+1
/* @724 */ 	bra         .Printf_label_794
.Printf_label_725:
.Printf_label_726:
.Printf_label_727:
/* @732 */ 	lda         __i12
/* @733 */ 	sta         __mem_src
/* @734 */ 	lda         __i12+1
/* @735 */ 	sta         __mem_src+1
/* @738 */ 	lda         #__f0
/* @739 */ 	sta         __mem_dest
/* @740 */ 	stz         __mem_dest+1
/* @741 */ 	jsr         __builtin_va_arg4
/* @744 */ 	ldx          #3
.Printf_label_745:
/* @746 */ 	lda         __f0, X
/* @747 */ 	sta         __f1, X
/* @748 */ 	dex         
/* @749 */ 	bpl         .Printf_label_745
/* @750 */ 	bra         .Printf_label_794
.Printf_label_751:
/* @755 */ 	lda         __i12
/* @756 */ 	sta         __mem_src
/* @757 */ 	lda         __i12+1
/* @758 */ 	sta         __mem_src+1
/* @761 */ 	lda         #__i0
/* @762 */ 	sta         __mem_dest
/* @763 */ 	stz         __mem_dest+1
/* @764 */ 	jsr         __builtin_va_arg2
/* @767 */ 	lda         __i0
/* @768 */ 	sta         __i7
/* @769 */ 	lda         __i0+1
/* @770 */ 	sta         __i7+1
/* @771 */ 	bra         .Printf_label_794
.Printf_label_772:
/* @776 */ 	lda         __i12
/* @777 */ 	sta         __mem_src
/* @778 */ 	lda         __i12+1
/* @779 */ 	sta         __mem_src+1
/* @782 */ 	lda         #__i0
/* @783 */ 	sta         __mem_dest
/* @784 */ 	stz         __mem_dest+1
/* @785 */ 	jsr         __builtin_va_arg2
/* @787 */ 	lda          #__i0
/* @789 */ 	ldx          #24
/* @790 */ 	ldy          #1
/* @792 */ 	jsr         __set_var_value1b
.Printf_label_794:
/* @797 */ 	lda         (__i4)
/* @798 */ 	sta         __b5
/* @802 */ 	cmp          #88
/* @803 */ 	bne         .Printf_label_1607
/* @1608 */ 	jmp         .Printf_label_997
.Printf_label_1607:
/* @807 */ 	lda         __b5
/* @808 */ 	cmp          #99
/* @809 */ 	bne         .Printf_label_1609
/* @1610 */ 	jmp         .Printf_label_1411
.Printf_label_1609:
/* @813 */ 	lda         __b5
/* @814 */ 	cmp          #100
/* @815 */ 	bne         .Printf_label_1623
/* @1624 */ 	jmp         .Printf_label_908
.Printf_label_1623:
/* @819 */ 	lda         __b5
/* @820 */ 	cmp          #101
/* @821 */ 	bne         .Printf_label_1611
/* @1612 */ 	jmp         .Printf_label_1250
.Printf_label_1611:
/* @825 */ 	lda         __b5
/* @826 */ 	cmp          #102
/* @827 */ 	bne         .Printf_label_1613
/* @1614 */ 	jmp         .Printf_label_1167
.Printf_label_1613:
/* @831 */ 	lda         __b5
/* @832 */ 	cmp          #103
/* @833 */ 	bne         .Printf_label_1615
/* @1616 */ 	jmp         .Printf_label_1331
.Printf_label_1615:
/* @837 */ 	lda         __b5
/* @838 */ 	cmp          #105
/* @839 */ 	beq         .Printf_label_909
/* @843 */ 	lda         __b5
/* @844 */ 	cmp          #112
/* @845 */ 	bne         .Printf_label_1617
/* @1618 */ 	jmp         .Printf_label_1101
.Printf_label_1617:
/* @849 */ 	lda         __b5
/* @850 */ 	cmp          #115
/* @851 */ 	bne         .Printf_label_1619
/* @1620 */ 	jmp         .Printf_label_1469
.Printf_label_1619:
/* @855 */ 	lda         __b5
/* @856 */ 	cmp          #117
/* @857 */ 	beq         .Printf_label_910
/* @861 */ 	lda         __b5
/* @862 */ 	cmp          #120
/* @863 */ 	bne         .Printf_label_1621
/* @1622 */ 	jmp         .Printf_label_996
.Printf_label_1621:
/* @866 */ 	jsr         __pushi9
/* @867 */ 	ldx          #1
/* @869 */ 	jsr         __pushxy0
/* @872 */ 	lda         __i4
/* @873 */ 	sta         __i0
/* @874 */ 	lda         __i4+1
/* @875 */ 	sta         __i0+1
/* @876 */ 	lda          #__i4
/* @877 */ 	jsr         __rinc21
/* @880 */ 	jsr         __pushi0
/* @882 */ 	ldx         #__i13
/* @883 */ 	ldy          #0
/* @886 */ 	jsr         .Printf_label_884
/* @887 */ 	bra         .Printf_label_885
.Printf_label_884:
/* @888 */ 	jmp         (__i10)
.Printf_label_885:
/* @890 */ 	jsr         __incsp6
/* @894 */ 	clc         
/* @895 */ 	lda         __i5
/* @896 */ 	adc         __i13
/* @897 */ 	sta         __i0
/* @898 */ 	lda         __i5+1
/* @899 */ 	adc         __i13+1
/* @900 */ 	sta         __i0+1
/* @903 */ 	lda         __i0
/* @904 */ 	sta         __i5
/* @905 */ 	lda         __i0+1
/* @906 */ 	sta         __i5+1
/* @907 */ 	jmp         .Printf_label_1533
.Printf_label_908:
.Printf_label_909:
.Printf_label_910:
/* @911 */ 	lda          #__i4
/* @912 */ 	jsr         __rinc21
/* @913 */ 	ldx          #0
/* @914 */ 	ldy          #1
/* @916 */ 	jsr         __pushxy
/* @918 */ 	ldx          #4
	ldy          #1
	jsr          __var_addrb_i0			// buf
/* @921 */ 	jsr         __pushi0
/* @922 */ 	jsr         __pushx1
/* @923 */ 	ldx         #__i8
/* @924 */ 	ldy          #0
/* @925 */ 	jsr         ConvertDecimalLongLong
/* @927 */ 	jsr         __incsp12
/* @932 */ 	lda         __b3
/* @934 */ 	beq         .Printf_label_931
/* @935 */ 	lda          #255
.Printf_label_931:
/* @936 */ 	inc          A
/* @937 */ 	sta         __b6
/* @941 */ 	beq         .Printf_label_945
/* @943 */ 	lda         __b2
/* @944 */ 	sta         __b6
.Printf_label_945:
/* @948 */ 	lda         __b6
/* @950 */ 	jsr         __pusha
/* @952 */ 	ldx          #27
	ldy          #1
	jsr          __var_value2b_i0			// end
/* @956 */ 	sec         
/* @957 */ 	lda         __i0
/* @958 */ 	sbc         __i8
/* @959 */ 	sta         __i1
/* @960 */ 	lda         __i0+1
/* @961 */ 	sbc         __i8+1
/* @962 */ 	sta         __i1+1
/* @965 */ 	jsr         __pushi1
/* @966 */ 	jsr         __pushi8
/* @971 */ 	jsr         __pushi11
/* @972 */ 	jsr         __pushi9
/* @973 */ 	jsr         __pushi10
/* @975 */ 	ldx         #__i13
/* @976 */ 	ldy          #0
/* @977 */ 	jsr         WriteFormatted
/* @978 */ 	jsr         __incsp12
/* @982 */ 	clc         
/* @983 */ 	lda         __i5
/* @984 */ 	adc         __i13
/* @985 */ 	sta         __i0
/* @986 */ 	lda         __i5+1
/* @987 */ 	adc         __i13+1
/* @988 */ 	sta         __i0+1
/* @991 */ 	lda         __i0
/* @992 */ 	sta         __i5
/* @993 */ 	lda         __i0+1
/* @994 */ 	sta         __i5+1
/* @995 */ 	jmp         .Printf_label_1533
.Printf_label_996:
.Printf_label_997:
/* @1000 */ 	lda         (__i4)
/* @1006 */ 	sta         __i0
/* @1007 */ 	and          #128
/* @1009 */ 	beq         .Printf_label_1008
/* @1010 */ 	lda          #255
.Printf_label_1008:
/* @1011 */ 	sta         __i0+1
/* @1017 */ 	ldx          #1
/* @1018 */ 	lda         __i0
/* @1019 */ 	cmp          #88
/* @1020 */ 	bne         .Printf_label_1015
/* @1021 */ 	lda         __i0+1
/* @1023 */ 	beq         .Printf_label_1016
.Printf_label_1015:
/* @1024 */ 	dex         
.Printf_label_1016:
/* @1025 */ 	stx         __b0
/* @1027 */ 	lda          #__b0
/* @1029 */ 	ldx          #25
/* @1030 */ 	ldy          #1
/* @1031 */ 	jsr         __set_var_value1b
/* @1032 */ 	lda          #__i4
/* @1033 */ 	jsr         __rinc21
/* @1035 */ 	ldx          #25
	ldy          #1
	jsr          __var_value1b_b0			// upper
/* @1038 */ 	lda         __b0
/* @1039 */ 	jsr         __pusha
/* @1040 */ 	ldx          #0
/* @1041 */ 	ldy          #1
/* @1042 */ 	jsr         __pushxy
/* @1044 */ 	ldx          #4
	ldy          #1
	jsr          __var_addrb_i0			// buf
/* @1047 */ 	jsr         __pushi0
/* @1048 */ 	jsr         __pushx1
/* @1049 */ 	ldx         #__i8
/* @1050 */ 	ldy          #0
/* @1051 */ 	jsr         ConvertHexLongLong
/* @1053 */ 	jsr         __incsp14
/* @1054 */ 	lda          #0
/* @1055 */ 	jsr         __pusha
/* @1057 */ 	ldx          #27
	ldy          #1
	jsr          __var_value2b_i0			// end
/* @1061 */ 	sec         
/* @1062 */ 	lda         __i0
/* @1063 */ 	sbc         __i8
/* @1064 */ 	sta         __i1
/* @1065 */ 	lda         __i0+1
/* @1066 */ 	sbc         __i8+1
/* @1067 */ 	sta         __i1+1
/* @1070 */ 	jsr         __pushi1
/* @1071 */ 	jsr         __pushi8
/* @1076 */ 	jsr         __pushi11
/* @1077 */ 	jsr         __pushi9
/* @1078 */ 	jsr         __pushi10
/* @1080 */ 	ldx         #__i13
/* @1081 */ 	ldy          #0
/* @1082 */ 	jsr         WriteFormatted
/* @1083 */ 	jsr         __incsp12
/* @1087 */ 	clc         
/* @1088 */ 	lda         __i5
/* @1089 */ 	adc         __i13
/* @1090 */ 	sta         __i0
/* @1091 */ 	lda         __i5+1
/* @1092 */ 	adc         __i13+1
/* @1093 */ 	sta         __i0+1
/* @1096 */ 	lda         __i0
/* @1097 */ 	sta         __i5
/* @1098 */ 	lda         __i0+1
/* @1099 */ 	sta         __i5+1
/* @1100 */ 	jmp         .Printf_label_1533
.Printf_label_1101:
/* @1102 */ 	lda          #__i4
/* @1103 */ 	jsr         __rinc21
/* @1104 */ 	lda          #0
/* @1105 */ 	jsr         __pusha
/* @1106 */ 	ldx          #0
/* @1107 */ 	ldy          #1
/* @1108 */ 	jsr         __pushxy
/* @1110 */ 	ldx          #4
	ldy          #1
	jsr          __var_addrb_i0			// buf
/* @1113 */ 	jsr         __pushi0
/* @1114 */ 	jsr         __pushi6
/* @1115 */ 	ldx         #__i8
/* @1116 */ 	ldy          #0
/* @1117 */ 	jsr         ConvertHexPointer
/* @1119 */ 	jsr         __incsp8
/* @1120 */ 	lda          #0
/* @1121 */ 	jsr         __pusha
/* @1123 */ 	ldx          #27
	ldy          #1
	jsr          __var_value2b_i0			// end
/* @1127 */ 	sec         
/* @1128 */ 	lda         __i0
/* @1129 */ 	sbc         __i8
/* @1130 */ 	sta         __i1
/* @1131 */ 	lda         __i0+1
/* @1132 */ 	sbc         __i8+1
/* @1133 */ 	sta         __i1+1
/* @1136 */ 	jsr         __pushi1
/* @1137 */ 	jsr         __pushi8
/* @1142 */ 	jsr         __pushi11
/* @1143 */ 	jsr         __pushi9
/* @1144 */ 	jsr         __pushi10
/* @1146 */ 	ldx         #__i13
/* @1147 */ 	ldy          #0
/* @1148 */ 	jsr         WriteFormatted
/* @1149 */ 	jsr         __incsp12
/* @1153 */ 	clc         
/* @1154 */ 	lda         __i5
/* @1155 */ 	adc         __i13
/* @1156 */ 	sta         __i0
/* @1157 */ 	lda         __i5+1
/* @1158 */ 	adc         __i13+1
/* @1159 */ 	sta         __i0+1
/* @1162 */ 	lda         __i0
/* @1163 */ 	sta         __i5
/* @1164 */ 	lda         __i0+1
/* @1165 */ 	sta         __i5+1
/* @1166 */ 	jmp         .Printf_label_1533
.Printf_label_1167:
/* @1169 */ 	ldx          #254
/* @1170 */ 	jsr         __pushxy0
/* @1175 */ 	jsr         __pushi11
/* @1176 */ 	jsr         FixFloatPrecision
/* @1177 */ 	jsr         __incsp4
/* @1178 */ 	lda          #__i4
/* @1179 */ 	jsr         __rinc21
/* @1180 */ 	ldx          #0
/* @1181 */ 	ldy          #1
/* @1182 */ 	jsr         __pushxy
/* @1184 */ 	ldx          #4
	ldy          #1
	jsr          __var_addrb_i0			// buf
/* @1187 */ 	jsr         __pushi0
/* @1193 */ 	ldy          #2
/* @1194 */ 	lda         (__i11), Y
/* @1195 */ 	sta         __i0
/* @1196 */ 	iny         
/* @1197 */ 	lda         (__i11), Y
/* @1198 */ 	sta         __i0+1
/* @1201 */ 	jsr         __pushi0
/* @1202 */ 	jsr         __pushf1
/* @1203 */ 	ldx         #__i8
/* @1204 */ 	ldy          #0
/* @1205 */ 	jsr         __PrintFloatFormat
/* @1207 */ 	jsr         __incsp10
/* @1208 */ 	lda          #0
/* @1209 */ 	jsr         __pusha
/* @1210 */ 	jsr         __pushi8
/* @1212 */ 	ldx         #__i13
/* @1213 */ 	ldy          #0
/* @1214 */ 	jsr         strlen
/* @1216 */ 	jsr         __incsp2
/* @1219 */ 	jsr         __pushi13
/* @1220 */ 	jsr         __pushi8
/* @1225 */ 	jsr         __pushi11
/* @1226 */ 	jsr         __pushi9
/* @1227 */ 	jsr         __pushi10
/* @1229 */ 	ldx         #__i14
/* @1230 */ 	ldy          #0
/* @1231 */ 	jsr         WriteFormatted
/* @1232 */ 	jsr         __incsp12
/* @1236 */ 	clc         
/* @1237 */ 	lda         __i5
/* @1238 */ 	adc         __i14
/* @1239 */ 	sta         __i0
/* @1240 */ 	lda         __i5+1
/* @1241 */ 	adc         __i14+1
/* @1242 */ 	sta         __i0+1
/* @1245 */ 	lda         __i0
/* @1246 */ 	sta         __i5
/* @1247 */ 	lda         __i0+1
/* @1248 */ 	sta         __i5+1
/* @1249 */ 	jmp         .Printf_label_1533
.Printf_label_1250:
/* @1252 */ 	ldx          #251
/* @1253 */ 	jsr         __pushxy0
/* @1258 */ 	jsr         __pushi11
/* @1259 */ 	jsr         FixFloatPrecision
/* @1260 */ 	jsr         __incsp4
/* @1261 */ 	lda          #__i4
/* @1262 */ 	jsr         __rinc21
/* @1263 */ 	ldx          #0
/* @1264 */ 	ldy          #1
/* @1265 */ 	jsr         __pushxy
/* @1267 */ 	ldx          #4
	ldy          #1
	jsr          __var_addrb_i0			// buf
/* @1270 */ 	jsr         __pushi0
/* @1276 */ 	ldy          #2
/* @1277 */ 	lda         (__i11), Y
/* @1278 */ 	sta         __i0
/* @1279 */ 	iny         
/* @1280 */ 	lda         (__i11), Y
/* @1281 */ 	sta         __i0+1
/* @1284 */ 	jsr         __pushi0
/* @1285 */ 	jsr         __pushf1
/* @1286 */ 	ldx         #__i8
/* @1287 */ 	ldy          #0
/* @1288 */ 	jsr         __PrintScientificFormat
/* @1289 */ 	jsr         __incsp10
/* @1290 */ 	lda          #0
/* @1291 */ 	jsr         __pusha
/* @1292 */ 	jsr         __pushi8
/* @1294 */ 	ldx         #__i13
/* @1295 */ 	ldy          #0
/* @1296 */ 	jsr         strlen
/* @1297 */ 	jsr         __incsp2
/* @1300 */ 	jsr         __pushi13
/* @1301 */ 	jsr         __pushi8
/* @1306 */ 	jsr         __pushi11
/* @1307 */ 	jsr         __pushi9
/* @1308 */ 	jsr         __pushi10
/* @1310 */ 	ldx         #__i14
/* @1311 */ 	ldy          #0
/* @1312 */ 	jsr         WriteFormatted
/* @1313 */ 	jsr         __incsp12
/* @1317 */ 	clc         
/* @1318 */ 	lda         __i5
/* @1319 */ 	adc         __i14
/* @1320 */ 	sta         __i0
/* @1321 */ 	lda         __i5+1
/* @1322 */ 	adc         __i14+1
/* @1323 */ 	sta         __i0+1
/* @1326 */ 	lda         __i0
/* @1327 */ 	sta         __i5
/* @1328 */ 	lda         __i0+1
/* @1329 */ 	sta         __i5+1
/* @1330 */ 	jmp         .Printf_label_1533
.Printf_label_1331:
/* @1332 */ 	ldx          #251
/* @1333 */ 	jsr         __pushxy0
/* @1338 */ 	jsr         __pushi11
/* @1339 */ 	jsr         FixFloatPrecision
/* @1340 */ 	jsr         __incsp4
/* @1341 */ 	lda          #__i4
/* @1342 */ 	jsr         __rinc21
/* @1343 */ 	ldx          #0
/* @1344 */ 	ldy          #1
/* @1345 */ 	jsr         __pushxy
/* @1347 */ 	ldx          #4
	ldy          #1
	jsr          __var_addrb_i0			// buf
/* @1350 */ 	jsr         __pushi0
/* @1356 */ 	ldy          #2
/* @1357 */ 	lda         (__i11), Y
/* @1358 */ 	sta         __i0
/* @1359 */ 	iny         
/* @1360 */ 	lda         (__i11), Y
/* @1361 */ 	sta         __i0+1
/* @1364 */ 	jsr         __pushi0
/* @1365 */ 	jsr         __pushf1
/* @1366 */ 	ldx         #__i8
/* @1367 */ 	ldy          #0
/* @1368 */ 	jsr         __PrintGeneralFormat
/* @1369 */ 	jsr         __incsp10
/* @1370 */ 	lda          #0
/* @1371 */ 	jsr         __pusha
/* @1372 */ 	jsr         __pushi8
/* @1374 */ 	ldx         #__i13
/* @1375 */ 	ldy          #0
/* @1376 */ 	jsr         strlen
/* @1377 */ 	jsr         __incsp2
/* @1380 */ 	jsr         __pushi13
/* @1381 */ 	jsr         __pushi8
/* @1386 */ 	jsr         __pushi11
/* @1387 */ 	jsr         __pushi9
/* @1388 */ 	jsr         __pushi10
/* @1390 */ 	ldx         #__i14
/* @1391 */ 	ldy          #0
/* @1392 */ 	jsr         WriteFormatted
/* @1393 */ 	jsr         __incsp12
/* @1397 */ 	clc         
/* @1398 */ 	lda         __i5
/* @1399 */ 	adc         __i14
/* @1400 */ 	sta         __i0
/* @1401 */ 	lda         __i5+1
/* @1402 */ 	adc         __i14+1
/* @1403 */ 	sta         __i0+1
/* @1406 */ 	lda         __i0
/* @1407 */ 	sta         __i5
/* @1408 */ 	lda         __i0+1
/* @1409 */ 	sta         __i5+1
/* @1410 */ 	jmp         .Printf_label_1533
.Printf_label_1411:
/* @1412 */ 	lda          #__i4
/* @1413 */ 	jsr         __rinc21
/* @1414 */ 	lda          #1
/* @1415 */ 	sta         __mem_size
/* @1417 */ 	ldx          #4
	jsr          __var_addr_i0			// b
/* @1419 */ 	lda         __i0
/* @1420 */ 	sta         __mem_dest
/* @1421 */ 	lda         __i0+1
/* @1422 */ 	sta         __mem_dest+1
/* @1423 */ 	jsr         __zeromem1
/* @1425 */ 	ldx          #24
	ldy          #1
	jsr          __var_value1b_b0			// value_c
/* @1427 */ 	lda          #__b0
/* @1428 */ 	ldx          #4
/* @1430 */ 	jsr         __set_var_value1
/* @1431 */ 	lda          #0
/* @1432 */ 	jsr         __pusha
/* @1433 */ 	ldx          #1
/* @1434 */ 	jsr         __pushxy0
/* @1439 */ 	jsr         __pushi0
/* @1444 */ 	jsr         __pushi11
/* @1445 */ 	jsr         __pushi9
/* @1446 */ 	jsr         __pushi10
/* @1448 */ 	ldx         #__i13
/* @1449 */ 	ldy          #0
/* @1450 */ 	jsr         WriteFormatted
/* @1451 */ 	jsr         __incsp12
/* @1455 */ 	clc         
/* @1456 */ 	lda         __i5
/* @1457 */ 	adc         __i13
/* @1458 */ 	sta         __i0
/* @1459 */ 	lda         __i5+1
/* @1460 */ 	adc         __i13+1
/* @1461 */ 	sta         __i0+1
/* @1464 */ 	lda         __i0
/* @1465 */ 	sta         __i5
/* @1466 */ 	lda         __i0+1
/* @1467 */ 	sta         __i5+1
/* @1468 */ 	bra         .Printf_label_1533
.Printf_label_1469:
/* @1470 */ 	lda          #__i4
/* @1471 */ 	jsr         __rinc21
/* @1475 */ 	lda          #0
/* @1477 */ 	ldy          #5
/* @1478 */ 	sta         (__i11), Y
/* @1484 */ 	iny         
/* @1485 */ 	sta         (__i11), Y
/* @1490 */ 	iny         
/* @1491 */ 	sta         (__i11), Y
/* @1493 */ 	jsr         __pusha
/* @1494 */ 	jsr         __pushi7
/* @1496 */ 	ldx         #__i13
/* @1497 */ 	ldy          #0
/* @1498 */ 	jsr         strlen
/* @1499 */ 	jsr         __incsp2
/* @1502 */ 	jsr         __pushi13
/* @1503 */ 	jsr         __pushi7
/* @1508 */ 	jsr         __pushi11
/* @1509 */ 	jsr         __pushi9
/* @1510 */ 	jsr         __pushi10
/* @1512 */ 	ldx         #__i14
/* @1513 */ 	ldy          #0
/* @1514 */ 	jsr         WriteFormatted
/* @1515 */ 	jsr         __incsp12
/* @1519 */ 	clc         
/* @1520 */ 	lda         __i5
/* @1521 */ 	adc         __i14
/* @1522 */ 	sta         __i0
/* @1523 */ 	lda         __i5+1
/* @1524 */ 	adc         __i14+1
/* @1525 */ 	sta         __i0+1
/* @1528 */ 	lda         __i0
/* @1529 */ 	sta         __i5
/* @1530 */ 	lda         __i0+1
/* @1531 */ 	sta         __i5+1
.Printf_label_1533:
/* @1534 */ 	bra         .Printf_label_1575
.Printf_label_1535:
/* @1536 */ 	jsr         __pushi9
/* @1537 */ 	ldx          #1
/* @1538 */ 	jsr         __pushxy0
/* @1541 */ 	lda         __i4
/* @1542 */ 	sta         __i0
/* @1543 */ 	lda         __i4+1
/* @1544 */ 	sta         __i0+1
/* @1545 */ 	lda          #__i4
/* @1546 */ 	jsr         __rinc21
/* @1549 */ 	jsr         __pushi0
/* @1551 */ 	ldx         #__i11
/* @1552 */ 	ldy          #0
/* @1555 */ 	jsr         .Printf_label_1553
/* @1556 */ 	bra         .Printf_label_1554
.Printf_label_1553:
/* @1557 */ 	jmp         (__i10)
.Printf_label_1554:
/* @1558 */ 	jsr         __incsp6
/* @1562 */ 	clc         
/* @1563 */ 	lda         __i5
/* @1564 */ 	adc         __i11
/* @1565 */ 	sta         __i0
/* @1566 */ 	lda         __i5+1
/* @1567 */ 	adc         __i11+1
/* @1568 */ 	sta         __i0+1
/* @1571 */ 	lda         __i0
/* @1572 */ 	sta         __i5
/* @1573 */ 	lda         __i0+1
/* @1574 */ 	sta         __i5+1
.Printf_label_1575:
/* @1576 */ 	jmp         .Printf_label_127
.Printf_label_1577:
/* @1578 */ 	ldx          #32
	ldy          #1
	jsr          __load_result+2
/* @1579 */ 	lda         #__i5
/* @1581 */ 	jsr         __result2
/* @1583 */ 	ldy          #34
	ldx          #1
	jmp          __leave+2
.func_end_Printf:
	.size Printf, .func_end_Printf-Printf

	.local  FILEWriter
	.type FILEWriter, @function

FILEWriter:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @17 */ 	ldx          #4
	jsr          __arg_value2_i1			// data
/* @21 */ 	ldx          #2
	jsr          __arg_value2_i2			// len
/* @25 */ 	ldx          #0
	jsr          __arg_value2_i3			// s
/* @27 */ 	lda         __i1
/* @28 */ 	sta         __i0
/* @30 */ 	lda         __i1+1
/* @31 */ 	sta         __i0+1
/* @32 */ 	jsr         __pushi0
/* @33 */ 	jsr         __pushi2
/* @34 */ 	ldx          #1
/* @36 */ 	jsr         __pushxy0
/* @37 */ 	jsr         __pushi3
/* @39 */ 	ldx         #__i4
/* @40 */ 	ldy          #0
/* @41 */ 	jsr         fwrite
/* @43 */ 	jsr         __incsp8
/* @45 */ 	ldx          #8
	jsr          __load_result
/* @46 */ 	lda         #__i4
/* @48 */ 	jsr         __result2
/* @50 */ 	ldy          #10
	jmp          __leave
.func_end_FILEWriter:
	.size FILEWriter, .func_end_FILEWriter-FILEWriter

	.local  StringWriter
	.type StringWriter, @function

StringWriter:
/* @5 */ 	stx         __result
/* @7 */ 	sty         __result+1
/* @8 */ 	ldx          #7
	jsr          __enter
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @16 */ 	ldx          #4
	jsr          __arg_value2_i1			// data
/* @20 */ 	ldx          #2
	jsr          __arg_value2_i2			// len
/* @24 */ 	ldx          #0
	jsr          __arg_value2_i3			// s
/* @26 */ 	lda         __i1
/* @27 */ 	sta         __i0
/* @29 */ 	lda         __i1+1
/* @30 */ 	sta         __i0+1
/* @36 */ 	ldy          #2
/* @37 */ 	lda         (__i0), Y
/* @38 */ 	sta         __i4
/* @40 */ 	iny         
/* @41 */ 	lda         (__i0), Y
/* @42 */ 	sta         __i4+1
/* @45 */ 	ldx          #0
/* @48 */ 	txa         
/* @49 */ 	cmp         __i4
/* @51 */ 	sbc         __i4+1
/* @52 */ 	bvc         .StringWriter_label_47
/* @54 */ 	eor          #128
.StringWriter_label_47:
/* @55 */ 	bpl         .StringWriter_label_44
/* @56 */ 	inx         
.StringWriter_label_44:
/* @57 */ 	stx         __b0
/* @59 */ 	txa         
/* @60 */ 	cmp          #0
/* @61 */ 	beq         .StringWriter_label_84
/* @64 */ 	ldy          #2
/* @65 */ 	lda         (__i0), Y
/* @66 */ 	sta         __i1
/* @67 */ 	iny         
/* @68 */ 	lda         (__i0), Y
/* @69 */ 	sta         __i1+1
/* @74 */ 	ldx          #1
/* @76 */ 	cmp         __i2+1
/* @77 */ 	bcc         .StringWriter_label_73
/* @78 */ 	bne         .StringWriter_label_72
/* @79 */ 	lda         __i1
/* @80 */ 	cmp         __i2
/* @81 */ 	bcc         .StringWriter_label_73
.StringWriter_label_72:
/* @82 */ 	dex         
.StringWriter_label_73:
/* @83 */ 	stx         __b0
.StringWriter_label_84:
/* @86 */ 	lda         __b0
/* @88 */ 	beq         .StringWriter_label_103
/* @91 */ 	ldy          #2
/* @92 */ 	lda         (__i0), Y
/* @93 */ 	sta         __i1
/* @94 */ 	iny         
/* @95 */ 	lda         (__i0), Y
/* @96 */ 	sta         __i1+1
/* @99 */ 	lda         __i1
/* @100 */ 	sta         __i2
/* @101 */ 	lda         __i1+1
/* @102 */ 	sta         __i2+1
.StringWriter_label_103:
/* @106 */ 	lda         (__i0)
/* @107 */ 	sta         __i1
/* @108 */ 	ldy          #1
/* @109 */ 	lda         (__i0), Y
/* @110 */ 	sta         __i1+1
/* @112 */ 	lda         __i2
/* @114 */ 	sta         __mem_size
/* @115 */ 	lda         __i2+1
/* @117 */ 	sta         __mem_size+1
/* @118 */ 	lda         __i3
/* @120 */ 	sta         __mem_src
/* @121 */ 	lda         __i3+1
/* @123 */ 	sta         __mem_src+1
/* @125 */ 	lda         __i1
/* @127 */ 	sta         __mem_dest
/* @128 */ 	lda         __i1+1
/* @130 */ 	sta         __mem_dest+1
/* @132 */ 	jsr         __builtin_memcpy
/* @135 */ 	lda         __i0
/* @136 */ 	sta         __i1
/* @137 */ 	lda         __i0+1
/* @138 */ 	sta         __i1+1
/* @143 */ 	lda         (__i1)
/* @144 */ 	sta         __i4
/* @145 */ 	ldy          #1
/* @146 */ 	lda         (__i1), Y
/* @147 */ 	sta         __i4+1
/* @151 */ 	clc         
/* @152 */ 	lda         __i4
/* @153 */ 	adc         __i2
/* @154 */ 	sta         __i5
/* @155 */ 	lda         __i4+1
/* @156 */ 	adc         __i2+1
/* @157 */ 	sta         __i5+1
/* @162 */ 	lda         __i5
/* @163 */ 	sta         (__i1)
/* @164 */ 	lda         __i5+1
/* @166 */ 	sta         (__i1), Y
/* @169 */ 	lda         __i0
/* @170 */ 	sta         __i1
/* @171 */ 	lda         __i0+1
/* @172 */ 	sta         __i1+1
/* @177 */ 	iny         
/* @178 */ 	lda         (__i1), Y
/* @179 */ 	sta         __i4
/* @180 */ 	iny         
/* @181 */ 	lda         (__i1), Y
/* @182 */ 	sta         __i4+1
/* @186 */ 	sec         
/* @187 */ 	lda         __i4
/* @188 */ 	sbc         __i2
/* @189 */ 	sta         __i5
/* @190 */ 	lda         __i4+1
/* @191 */ 	sbc         __i2+1
/* @192 */ 	sta         __i5+1
/* @197 */ 	lda         __i5
/* @198 */ 	dey         
/* @199 */ 	sta         (__i1), Y
/* @200 */ 	lda         __i5+1
/* @201 */ 	iny         
/* @202 */ 	sta         (__i1), Y
/* @203 */ 	ldx          #8
	jsr          __load_result
/* @204 */ 	lda         #__i2
/* @206 */ 	jsr         __result2
/* @208 */ 	ldy          #10
	jmp          __leave
.func_end_StringWriter:
	.size StringWriter, .func_end_StringWriter-StringWriter

	.global fprintf
	.type fprintf, @function

fprintf:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #9
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @21 */ 	ldx          #0
	jsr          __arg_value2_i0			// fp
/* @23 */ 	ldx          #5
	jsr          __var_addr_i1			// ap
/* @25 */ 	ldx          #2
	jsr          __arg_addr_i2			// format
/* @29 */ 	clc         
/* @31 */ 	ldy          #0
/* @32 */ 	lda         __i2
/* @33 */ 	adc          #2
/* @34 */ 	sta         (__i1)
/* @36 */ 	iny         
/* @37 */ 	lda         __i2+1
/* @38 */ 	adc          #0
/* @39 */ 	sta         (__i1), Y
/* @41 */ 	ldx          #5
	jsr          __var_value2_i1			// ap
/* @44 */ 	jsr         __pushi1
/* @46 */ 	ldx          #2
	jsr          __arg_value2_i1			// format
/* @49 */ 	jsr         __pushi1
/* @50 */ 	jsr         __pushi0
/* @53 */ 	lda         #%lo(FILEWriter)
/* @54 */ 	sta         __i1
/* @55 */ 	lda         #%hi(FILEWriter)
/* @56 */ 	sta         __i1+1
/* @58 */ 	jsr         __pushi1
/* @59 */ 	ldx         #__i4
/* @60 */ 	ldy          #0
/* @61 */ 	jsr         Printf
/* @63 */ 	jsr         __incsp8
/* @64 */ 	ldx          #10
	jsr          __load_result
/* @65 */ 	lda         #__i4
/* @67 */ 	jsr         __result2
/* @69 */ 	ldy          #12
	jmp          __leave
.func_end_fprintf:
	.size fprintf, .func_end_fprintf-fprintf

	.global vfprintf
	.type vfprintf, @function

vfprintf:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @15 */ 	ldx          #4
	jsr          __arg_value2_i0			// arg
/* @19 */ 	ldx          #2
	jsr          __arg_value2_i1			// format
/* @23 */ 	ldx          #0
	jsr          __arg_value2_i2			// stream
/* @24 */ 	jsr         __pushi0
/* @25 */ 	jsr         __pushi1
/* @26 */ 	jsr         __pushi2
/* @30 */ 	lda         #%lo(FILEWriter)
/* @31 */ 	sta         __i3
/* @33 */ 	lda         #%hi(FILEWriter)
/* @34 */ 	sta         __i3+1
/* @36 */ 	jsr         __pushi3
/* @38 */ 	ldx         #__i4
/* @39 */ 	ldy          #0
/* @40 */ 	jsr         Printf
/* @42 */ 	jsr         __incsp8
/* @44 */ 	ldx          #8
	jsr          __load_result
/* @45 */ 	lda         #__i4
/* @47 */ 	jsr         __result2
/* @49 */ 	ldy          #10
	jmp          __leave
.func_end_vfprintf:
	.size vfprintf, .func_end_vfprintf-vfprintf

	.global printf
	.type printf, @function

printf:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #9
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @20 */ 	ldx          #5
	jsr          __var_addr_i0			// ap
/* @22 */ 	ldx          #0
	jsr          __arg_addr_i1			// format
/* @26 */ 	clc         
/* @28 */ 	ldy          #0
/* @29 */ 	lda         __i1
/* @30 */ 	adc          #2
/* @31 */ 	sta         (__i0)
/* @33 */ 	iny         
/* @34 */ 	lda         __i1+1
/* @35 */ 	adc          #0
/* @36 */ 	sta         (__i0), Y
/* @38 */ 	ldx          #5
	jsr          __var_value2_i0			// ap
/* @41 */ 	jsr         __pushi0
/* @43 */ 	ldx          #0
	jsr          __arg_value2_i0			// format
/* @46 */ 	jsr         __pushi0
/* @49 */ 	lda         stdout+0
/* @50 */ 	sta         __i0
/* @51 */ 	lda         stdout+1
/* @52 */ 	sta         __i0+1
/* @54 */ 	jsr         __pushi0
/* @57 */ 	lda         #%lo(FILEWriter)
/* @58 */ 	sta         __i0
/* @59 */ 	lda         #%hi(FILEWriter)
/* @60 */ 	sta         __i0+1
/* @62 */ 	jsr         __pushi0
/* @63 */ 	ldx         #__i4
/* @64 */ 	ldy          #0
/* @65 */ 	jsr         Printf
/* @67 */ 	jsr         __incsp8
/* @68 */ 	ldx          #10
	jsr          __load_result
/* @69 */ 	lda         #__i4
/* @71 */ 	jsr         __result2
/* @73 */ 	ldy          #12
	jmp          __leave
.func_end_printf:
	.size printf, .func_end_printf-printf

	.global vprintf
	.type vprintf, @function

vprintf:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @16 */ 	ldx          #2
	jsr          __arg_value2_i0			// arg
/* @20 */ 	ldx          #0
	jsr          __arg_value2_i1			// format
/* @21 */ 	jsr         __pushi0
/* @22 */ 	jsr         __pushi1
/* @26 */ 	lda         stdout+0
/* @27 */ 	sta         __i2
/* @29 */ 	lda         stdout+1
/* @30 */ 	sta         __i2+1
/* @32 */ 	jsr         __pushi2
/* @35 */ 	lda         #%lo(FILEWriter)
/* @36 */ 	sta         __i2
/* @37 */ 	lda         #%hi(FILEWriter)
/* @38 */ 	sta         __i2+1
/* @40 */ 	jsr         __pushi2
/* @42 */ 	ldx         #__i4
/* @43 */ 	ldy          #0
/* @44 */ 	jsr         Printf
/* @46 */ 	jsr         __incsp8
/* @48 */ 	ldx          #8
	jsr          __load_result
/* @49 */ 	lda         #__i4
/* @51 */ 	jsr         __result2
/* @53 */ 	ldy          #10
	jmp          __leave
.func_end_vprintf:
	.size vprintf, .func_end_vprintf-vprintf

	.global sprintf
	.type sprintf, @function

sprintf:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #13
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @22 */ 	ldx          #0
	jsr          __arg_value2_i0			// s
/* @26 */ 	ldx          #5
	jsr          __var_addr_i1			// ap
/* @28 */ 	ldx          #2
	jsr          __arg_addr_i2			// format
/* @32 */ 	clc         
/* @34 */ 	ldy          #0
/* @35 */ 	lda         __i2
/* @36 */ 	adc          #2
/* @37 */ 	sta         (__i1)
/* @39 */ 	iny         
/* @40 */ 	lda         __i2+1
/* @41 */ 	adc          #0
/* @42 */ 	sta         (__i1), Y
/* @44 */ 	lda          #4
/* @46 */ 	sta         __mem_size
/* @48 */ 	ldx          #9
	jsr          __var_addr_i1			// data
/* @50 */ 	lda         __i1
/* @52 */ 	sta         __mem_dest
/* @53 */ 	lda         __i1+1
/* @55 */ 	sta         __mem_dest+1
/* @57 */ 	jsr         __zeromem1
/* @58 */ 	lda          #__i0
/* @60 */ 	ldx          #9
/* @62 */ 	jsr         __set_var_value2
/* @66 */ 	lda          #255
/* @67 */ 	ldy          #2
/* @68 */ 	sta         (__i1), Y
/* @71 */ 	iny         
/* @72 */ 	sta         (__i1), Y
/* @74 */ 	ldx          #5
	jsr          __var_value2_i2			// ap
/* @77 */ 	jsr         __pushi2
/* @79 */ 	ldx          #2
	jsr          __arg_value2_i2			// format
/* @82 */ 	jsr         __pushi2
/* @87 */ 	jsr         __pushi1
/* @90 */ 	lda         #%lo(StringWriter)
/* @91 */ 	sta         __i1
/* @92 */ 	lda         #%hi(StringWriter)
/* @93 */ 	sta         __i1+1
/* @95 */ 	jsr         __pushi1
/* @96 */ 	ldx         #__i4
/* @97 */ 	ldy          #0
/* @98 */ 	jsr         Printf
/* @100 */ 	jsr         __incsp8
/* @101 */ 	ldx          #14
	jsr          __load_result
/* @102 */ 	lda         #__i4
/* @104 */ 	jsr         __result2
/* @106 */ 	ldy          #16
	jmp          __leave
.func_end_sprintf:
	.size sprintf, .func_end_sprintf-sprintf

	.global vsprintf
	.type vsprintf, @function

vsprintf:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #11
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i0			// s
/* @22 */ 	ldx          #4
	jsr          __arg_value2_i1			// arg
/* @26 */ 	ldx          #2
	jsr          __arg_value2_i2			// format
/* @28 */ 	lda          #4
/* @30 */ 	sta         __mem_size
/* @32 */ 	ldx          #7
	jsr          __var_addr_i3			// data
/* @35 */ 	lda         __i3
/* @37 */ 	sta         __mem_dest
/* @39 */ 	lda         __i3+1
/* @41 */ 	sta         __mem_dest+1
/* @43 */ 	jsr         __zeromem1
/* @44 */ 	lda          #__i0
/* @46 */ 	ldx          #7
/* @48 */ 	jsr         __set_var_value2
/* @52 */ 	lda          #255
/* @54 */ 	ldy          #2
/* @55 */ 	sta         (__i3), Y
/* @58 */ 	iny         
/* @59 */ 	sta         (__i3), Y
/* @60 */ 	jsr         __pushi1
/* @61 */ 	jsr         __pushi2
/* @66 */ 	jsr         __pushi3
/* @69 */ 	lda         #%lo(StringWriter)
/* @70 */ 	sta         __i3
/* @71 */ 	lda         #%hi(StringWriter)
/* @72 */ 	sta         __i3+1
/* @74 */ 	jsr         __pushi3
/* @76 */ 	ldx         #__i4
/* @77 */ 	ldy          #0
/* @78 */ 	jsr         Printf
/* @80 */ 	jsr         __incsp8
/* @82 */ 	ldx          #12
	jsr          __load_result
/* @83 */ 	lda         #__i4
/* @85 */ 	jsr         __result2
/* @87 */ 	ldy          #14
	jmp          __leave
.func_end_vsprintf:
	.size vsprintf, .func_end_vsprintf-vsprintf

	.global snprintf
	.type snprintf, @function

snprintf:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #13
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @21 */ 	ldx          #0
	jsr          __arg_value2_i0			// s
/* @25 */ 	ldx          #2
	jsr          __arg_value2_i1			// len
/* @29 */ 	ldx          #5
	jsr          __var_addr_i2			// ap
/* @31 */ 	ldx          #4
	jsr          __arg_addr_i3			// format
/* @35 */ 	clc         
/* @37 */ 	ldy          #0
/* @38 */ 	lda         __i3
/* @39 */ 	adc          #2
/* @40 */ 	sta         (__i2)
/* @42 */ 	iny         
/* @43 */ 	lda         __i3+1
/* @44 */ 	adc          #0
/* @45 */ 	sta         (__i2), Y
/* @47 */ 	lda          #4
/* @49 */ 	sta         __mem_size
/* @51 */ 	ldx          #9
	jsr          __var_addr_i2			// data
/* @53 */ 	lda         __i2
/* @55 */ 	sta         __mem_dest
/* @56 */ 	lda         __i2+1
/* @58 */ 	sta         __mem_dest+1
/* @60 */ 	jsr         __zeromem1
/* @61 */ 	lda          #__i0
/* @63 */ 	ldx          #9
/* @65 */ 	jsr         __set_var_value2
/* @69 */ 	lda         __i1
/* @70 */ 	ldy          #2
/* @71 */ 	sta         (__i2), Y
/* @72 */ 	lda         __i1+1
/* @74 */ 	iny         
/* @75 */ 	sta         (__i2), Y
/* @77 */ 	ldx          #5
	jsr          __var_value2_i3			// ap
/* @80 */ 	jsr         __pushi3
/* @82 */ 	ldx          #4
	jsr          __arg_value2_i3			// format
/* @85 */ 	jsr         __pushi3
/* @90 */ 	jsr         __pushi2
/* @93 */ 	lda         #%lo(StringWriter)
/* @94 */ 	sta         __i2
/* @95 */ 	lda         #%hi(StringWriter)
/* @96 */ 	sta         __i2+1
/* @98 */ 	jsr         __pushi2
/* @99 */ 	ldx         #__i4
/* @100 */ 	ldy          #0
/* @101 */ 	jsr         Printf
/* @103 */ 	jsr         __incsp8
/* @104 */ 	ldx          #14
	jsr          __load_result
/* @105 */ 	lda         #__i4
/* @107 */ 	jsr         __result2
/* @109 */ 	ldy          #16
	jmp          __leave
.func_end_snprintf:
	.size snprintf, .func_end_snprintf-snprintf

	.global vsnprintf
	.type vsnprintf, @function

vsnprintf:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #11
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @17 */ 	ldx          #0
	jsr          __arg_value2_i0			// s
/* @21 */ 	ldx          #2
	jsr          __arg_value2_i1			// n
/* @25 */ 	ldx          #6
	jsr          __arg_value2_i2			// arg
/* @29 */ 	ldx          #4
	jsr          __arg_value2_i3			// format
/* @31 */ 	lda          #4
/* @33 */ 	sta         __mem_size
/* @35 */ 	ldx          #7
	jsr          __var_addr_i4			// data
/* @38 */ 	lda         __i4
/* @40 */ 	sta         __mem_dest
/* @42 */ 	lda         __i4+1
/* @44 */ 	sta         __mem_dest+1
/* @46 */ 	jsr         __zeromem1
/* @47 */ 	lda          #__i0
/* @49 */ 	ldx          #7
/* @51 */ 	jsr         __set_var_value2
/* @55 */ 	lda         __i1
/* @57 */ 	ldy          #2
/* @58 */ 	sta         (__i4), Y
/* @59 */ 	lda         __i1+1
/* @61 */ 	iny         
/* @62 */ 	sta         (__i4), Y
/* @63 */ 	jsr         __pushi2
/* @64 */ 	jsr         __pushi3
/* @69 */ 	jsr         __pushi4
/* @72 */ 	lda         #%lo(StringWriter)
/* @73 */ 	sta         __i4
/* @74 */ 	lda         #%hi(StringWriter)
/* @75 */ 	sta         __i4+1
/* @77 */ 	jsr         __pushi4
/* @79 */ 	ldx         #__i4
/* @80 */ 	ldy          #0
/* @81 */ 	jsr         Printf
/* @83 */ 	jsr         __incsp8
/* @85 */ 	ldx          #12
	jsr          __load_result
/* @86 */ 	lda         #__i4
/* @88 */ 	jsr         __result2
/* @90 */ 	ldy          #14
	jmp          __leave
.func_end_vsnprintf:
	.size vsnprintf, .func_end_vsnprintf-vsnprintf

	.data
	.p2align  0
spaces:
	.type   spaces,@object
	.local  spaces
	.size   spaces,9
	.byte 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00
	.space  9

	.p2align  0
zeroes:
	.type   zeroes,@object
	.local  zeroes
	.size   zeroes,9
	.byte 0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x00
	.space  9

	.section ".rodata", "aMS", @progbits
.lit.31:
	.byte 0x0a
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.type .lit.31, @object
	.size .lit.31, 8

.lit.35:
	.byte 0x30
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.type .lit.35, @object
	.size .lit.35, 8

.str.44:
	.asciz "(null)"
	.type .str.44, @object
	.size .str.44, 7

.str.60:
	.asciz "-"
	.type .str.60, @object
	.size .str.60, 2

.str.61:
	.asciz "+"
	.type .str.61, @object
	.size .str.61, 2

.str.62:
	.asciz " "
	.type .str.62, @object
	.size .str.62, 2

.str.69:
	.asciz "-"
	.type .str.69, @object
	.size .str.69, 2

