	.file   "/Users/dallison/Google Drive/c_compiler/libc/sincos.c"
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


	.local  __sin
	.type __sin, @function

__sin:
/* @24 */ 	stx         __result
/* @26 */ 	sty         __result+1
/* @27 */ 	ldx          #27
	jsr          __enter
	.byte        0x01,0x40,0x03		// Save mask i:1 b:0 l:0 x:2 f:3 
/* @35 */ 	ldx          #4
	jsr          __arg_value2_i4			// quad
/* @51 */ 	ldx          #0
	jsr          __arg_value4_f0			// x
/* @55 */ 	lda         __f0+3
/* @56 */ 	bpl         .__sin_label_97
/* @58 */ 	ldx          #0
	jsr          __arg_value4_f0			// x
/* @63 */ 	ldx          #3
.__sin_label_64:
/* @66 */ 	lda         __f0, X
/* @67 */ 	sta         __f2, X
/* @68 */ 	dex         
/* @69 */ 	bpl         .__sin_label_64
/* @70 */ 	ldy          #3
/* @71 */ 	lda         __f2+3
/* @73 */ 	eor          #128
/* @74 */ 	sta         __f2+3
/* @76 */ 	lda          #__f2
/* @78 */ 	ldx          #0
/* @80 */ 	jsr         __set_arg_value4
/* @83 */ 	clc         
/* @84 */ 	lda         __i4
/* @85 */ 	adc          #2
/* @86 */ 	sta         __i0
/* @88 */ 	lda         __i4+1
/* @89 */ 	adc          #0
/* @90 */ 	sta         __i0+1
/* @93 */ 	lda         __i0
/* @94 */ 	sta         __i4
/* @95 */ 	lda         __i0+1
/* @96 */ 	sta         __i4+1
.__sin_label_97:
/* @99 */ 	ldx          #0
	jsr          __arg_value4_f0			// x
/* @104 */ 	ldx          #3
.__sin_label_105:
/* @106 */ 	lda         .lit.3, X
/* @107 */ 	sta         __f2, X
/* @108 */ 	dex         
/* @109 */ 	bpl         .__sin_label_105
/* @114 */ 	lda          #__f3
/* @115 */ 	ldx          #__f0
/* @116 */ 	ldy          #__f2
/* @118 */ 	jsr         __fmul
/* @120 */ 	lda          #__f3
/* @121 */ 	ldx          #0
/* @122 */ 	jsr         __set_arg_value4
/* @124 */ 	ldx          #0
	jsr          __arg_value4_f0			// x
/* @127 */ 	lda          #0
/* @128 */ 	cmp         __f0
/* @130 */ 	sbc         __f0+1
/* @132 */ 	lda          #0
/* @133 */ 	sbc         __f0+2
/* @134 */ 	lda          #95
/* @135 */ 	sbc         __f0+3
/* @136 */ 	bvc         .__sin_label_126
/* @137 */ 	eor          #128
.__sin_label_126:
/* @138 */ 	bmi         .__sin_label_751
/* @752 */ 	jmp         .__sin_label_280
.__sin_label_751:
/* @140 */ 	ldx          #11
	jsr          __var_addr_i0			// e
/* @143 */ 	jsr         __pushi0
/* @145 */ 	ldx          #0
	jsr          __arg_value4_f0			// x
/* @148 */ 	jsr         __pushf0
/* @149 */ 	ldx          #7
	jsr         __var_addr_xy			// y
/* @150 */ 	jsr         modf
/* @152 */ 	jsr         __incsp6
/* @155 */ 	lda          #__f0
/* @156 */ 	ldx          #__i4
/* @158 */ 	jsr         __i2tof
/* @160 */ 	ldx          #11
	jsr          __var_value4_f2			// e
/* @167 */ 	lda          #__f3
/* @168 */ 	ldx          #__f2
/* @169 */ 	ldy          #__f0
/* @171 */ 	jsr         __fadd
/* @173 */ 	lda          #__f3
/* @175 */ 	ldx          #11
/* @177 */ 	jsr         __set_var_value4
/* @179 */ 	ldx          #15
	jsr          __var_addr_i0			// f
/* @182 */ 	jsr         __pushi0
/* @184 */ 	ldx          #11
	jsr          __var_value4_f0			// e
/* @188 */ 	ldx          #3
.__sin_label_189:
/* @190 */ 	lda         .lit.7, X
/* @191 */ 	sta         __f2, X
/* @192 */ 	dex         
/* @193 */ 	bpl         .__sin_label_189
/* @199 */ 	lda          #__f3
/* @200 */ 	ldx          #__f2
/* @201 */ 	ldy          #__f0
/* @202 */ 	jsr         __fmul
/* @205 */ 	jsr         __pushf3
/* @207 */ 	ldx         #__f0
/* @208 */ 	ldy          #0
/* @209 */ 	jsr         modf
/* @210 */ 	jsr         __incsp6
/* @212 */ 	ldx          #11
	jsr          __var_value4_f0			// e
/* @217 */ 	lda          #__i0
/* @218 */ 	ldx          #__f0
/* @220 */ 	jsr         __ftoi2
/* @224 */ 	lda         __i0
/* @225 */ 	sta         __i1
/* @226 */ 	lda         __i0+1
/* @227 */ 	sta         __i1+1
/* @229 */ 	ldx          #15
	jsr          __var_value4_f0			// f
/* @233 */ 	ldx          #3
.__sin_label_234:
/* @235 */ 	lda         .lit.9, X
/* @236 */ 	sta         __f2, X
/* @237 */ 	dex         
/* @238 */ 	bpl         .__sin_label_234
/* @244 */ 	lda          #__f3
/* @245 */ 	ldx          #__f2
/* @246 */ 	ldy          #__f0
/* @247 */ 	jsr         __fmul
/* @252 */ 	lda          #__i0
/* @253 */ 	ldx          #__f3
/* @254 */ 	jsr         __ftoi2
/* @258 */ 	lda         __i0
/* @259 */ 	sta         __i2
/* @260 */ 	lda         __i0+1
/* @261 */ 	sta         __i2+1
/* @266 */ 	sec         
/* @267 */ 	lda         __i1
/* @268 */ 	sbc         __i2
/* @269 */ 	sta         __i0
/* @270 */ 	lda         __i1+1
/* @271 */ 	sbc         __i2+1
/* @272 */ 	sta         __i0+1
/* @275 */ 	lda         __i0
/* @276 */ 	sta         __i4
/* @277 */ 	lda         __i0+1
/* @278 */ 	sta         __i4+1
/* @279 */ 	bra         .__sin_label_383
.__sin_label_280:
/* @282 */ 	ldx          #0
	jsr          __arg_value4_f0			// x
/* @287 */ 	lda          #__x0
/* @288 */ 	ldx          #__f0
/* @290 */ 	jsr         __ftoi8
/* @294 */ 	ldx          #7
.__sin_label_295:
/* @296 */ 	lda         __x0, X
/* @297 */ 	sta         __x1, X
/* @298 */ 	dex         
/* @299 */ 	bpl         .__sin_label_295
/* @301 */ 	ldx          #0
	jsr          __arg_value4_f0			// x
/* @304 */ 	lda          #__f2
/* @305 */ 	ldx          #__x1
/* @307 */ 	jsr         __i8tof
/* @314 */ 	lda          #__f3
/* @315 */ 	ldx          #__f0
/* @316 */ 	ldy          #__f2
/* @318 */ 	jsr         __fsub
/* @320 */ 	lda          #__f3
/* @321 */ 	ldx          #7
/* @322 */ 	jsr         __set_var_value4
/* @325 */ 	lda         __i4
/* @326 */ 	sta         __x0
/* @327 */ 	lda         __i4+1
/* @328 */ 	sta         __x0+1
/* @329 */ 	and          #128
/* @331 */ 	beq         .__sin_label_330
/* @333 */ 	lda          #255
.__sin_label_330:
/* @334 */ 	ldy          #7
.__sin_label_335:
/* @336 */ 	sta         __x0, Y
/* @337 */ 	dey         
/* @338 */ 	cpy          #1
/* @339 */ 	bne         .__sin_label_335
/* @343 */ 	clc         
/* @345 */ 	ldy          #8
/* @346 */ 	ldx          #0
.__sin_label_347:
/* @348 */ 	lda         __x0, X
/* @349 */ 	adc         __x1, X
/* @350 */ 	sta         __x2, X
/* @351 */ 	inx         
/* @352 */ 	dey         
/* @353 */ 	bne         .__sin_label_347
/* @357 */ 	lda         __x2
/* @358 */ 	and          #3
/* @359 */ 	sta         __x0
/* @361 */ 	stz         __x0+1
/* @363 */ 	stz         __x0+2
/* @365 */ 	stz         __x0+3
/* @368 */ 	stz         __x0+4
/* @371 */ 	stz         __x0+5
/* @374 */ 	stz         __x0+6
/* @376 */ 	stz         __x0+7
/* @380 */ 	sta         __i4
/* @381 */ 	lda         __x0+1
/* @382 */ 	sta         __i4+1
.__sin_label_383:
/* @386 */ 	lda         __i4
/* @387 */ 	and          #1
/* @390 */ 	stz         __i0+1
/* @395 */ 	beq         .__sin_label_420
/* @397 */ 	ldx          #7
	jsr          __var_value4_f0			// y
/* @401 */ 	ldx          #3
.__sin_label_402:
/* @403 */ 	lda         .lit.13, X
/* @404 */ 	sta         __f2, X
/* @405 */ 	dex         
/* @406 */ 	bpl         .__sin_label_402
/* @412 */ 	lda          #__f3
/* @413 */ 	ldx          #__f2
/* @414 */ 	ldy          #__f0
/* @415 */ 	jsr         __fsub
/* @417 */ 	lda          #__f3
/* @418 */ 	ldx          #7
/* @419 */ 	jsr         __set_var_value4
.__sin_label_420:
/* @422 */ 	lda          #1
/* @423 */ 	cmp         __i4
/* @424 */ 	dec          A
/* @425 */ 	sbc         __i4+1
/* @426 */ 	bvc         .__sin_label_421
/* @427 */ 	eor          #128
.__sin_label_421:
/* @428 */ 	bpl         .__sin_label_449
/* @430 */ 	ldx          #7
	jsr          __var_value4_f0			// y
/* @435 */ 	ldx          #3
.__sin_label_436:
/* @437 */ 	lda         __f0, X
/* @438 */ 	sta         __f2, X
/* @439 */ 	dex         
/* @440 */ 	bpl         .__sin_label_436
/* @441 */ 	ldy          #3
/* @442 */ 	lda         __f2+3
/* @443 */ 	eor          #128
/* @444 */ 	sta         __f2+3
/* @446 */ 	lda          #__f2
/* @447 */ 	ldx          #7
/* @448 */ 	jsr         __set_var_value4
.__sin_label_449:
/* @451 */ 	ldx          #7
	jsr          __var_value4_f0			// y
/* @453 */ 	ldx          #7
	jsr          __var_value4_f2			// y
/* @460 */ 	lda          #__f3
/* @461 */ 	ldx          #__f0
/* @462 */ 	ldy          #__f2
/* @463 */ 	jsr         __fmul
/* @465 */ 	lda          #__f3
/* @467 */ 	ldx          #19
/* @468 */ 	jsr         __set_var_value4
/* @470 */ 	ldx          #19
	jsr          __var_value4_f0			// y2
/* @474 */ 	ldx          #3
.__sin_label_475:
/* @476 */ 	lda         .lit.14, X
/* @477 */ 	sta         __f2, X
/* @478 */ 	dex         
/* @479 */ 	bpl         .__sin_label_475
/* @485 */ 	lda          #__f3
/* @486 */ 	ldx          #__f2
/* @487 */ 	ldy          #__f0
/* @488 */ 	jsr         __fmul
/* @493 */ 	ldx          #3
.__sin_label_494:
/* @495 */ 	lda         .lit.15, X
/* @496 */ 	sta         __f0, X
/* @497 */ 	dex         
/* @498 */ 	bpl         .__sin_label_494
/* @503 */ 	lda          #__f2
/* @504 */ 	ldx          #__f3
/* @505 */ 	ldy          #__f0
/* @506 */ 	jsr         __fadd
/* @508 */ 	ldx          #19
	jsr          __var_value4_f0			// y2
/* @515 */ 	lda          #__f3
/* @516 */ 	ldx          #__f2
/* @517 */ 	ldy          #__f0
/* @518 */ 	jsr         __fmul
/* @523 */ 	ldx          #3
.__sin_label_524:
/* @525 */ 	lda         .lit.16, X
/* @526 */ 	sta         __f0, X
/* @527 */ 	dex         
/* @528 */ 	bpl         .__sin_label_524
/* @533 */ 	lda          #__f2
/* @534 */ 	ldx          #__f3
/* @535 */ 	ldy          #__f0
/* @536 */ 	jsr         __fadd
/* @538 */ 	ldx          #19
	jsr          __var_value4_f0			// y2
/* @545 */ 	lda          #__f3
/* @546 */ 	ldx          #__f2
/* @547 */ 	ldy          #__f0
/* @548 */ 	jsr         __fmul
/* @553 */ 	ldx          #3
.__sin_label_554:
/* @555 */ 	lda         .lit.17, X
/* @556 */ 	sta         __f0, X
/* @557 */ 	dex         
/* @558 */ 	bpl         .__sin_label_554
/* @563 */ 	lda          #__f2
/* @564 */ 	ldx          #__f3
/* @565 */ 	ldy          #__f0
/* @566 */ 	jsr         __fadd
/* @568 */ 	ldx          #19
	jsr          __var_value4_f0			// y2
/* @575 */ 	lda          #__f3
/* @576 */ 	ldx          #__f2
/* @577 */ 	ldy          #__f0
/* @578 */ 	jsr         __fmul
/* @583 */ 	ldx          #3
.__sin_label_584:
/* @585 */ 	lda         .lit.18, X
/* @586 */ 	sta         __f0, X
/* @587 */ 	dex         
/* @588 */ 	bpl         .__sin_label_584
/* @593 */ 	lda          #__f2
/* @594 */ 	ldx          #__f3
/* @595 */ 	ldy          #__f0
/* @596 */ 	jsr         __fadd
/* @598 */ 	ldx          #7
	jsr          __var_value4_f0			// y
/* @605 */ 	lda          #__f3
/* @606 */ 	ldx          #__f2
/* @607 */ 	ldy          #__f0
/* @608 */ 	jsr         __fmul
/* @610 */ 	lda          #__f3
/* @612 */ 	ldx          #23
/* @613 */ 	jsr         __set_var_value4
/* @615 */ 	ldx          #19
	jsr          __var_value4_f0			// y2
/* @620 */ 	ldx          #3
.__sin_label_621:
/* @622 */ 	lda         .lit.19, X
/* @623 */ 	sta         __f2, X
/* @624 */ 	dex         
/* @625 */ 	bpl         .__sin_label_621
/* @630 */ 	lda          #__f3
/* @631 */ 	ldx          #__f0
/* @632 */ 	ldy          #__f2
/* @633 */ 	jsr         __fadd
/* @635 */ 	ldx          #19
	jsr          __var_value4_f0			// y2
/* @642 */ 	lda          #__f2
/* @643 */ 	ldx          #__f3
/* @644 */ 	ldy          #__f0
/* @645 */ 	jsr         __fmul
/* @650 */ 	ldx          #3
.__sin_label_651:
/* @652 */ 	lda         .lit.20, X
/* @653 */ 	sta         __f0, X
/* @654 */ 	dex         
/* @655 */ 	bpl         .__sin_label_651
/* @660 */ 	lda          #__f3
/* @661 */ 	ldx          #__f2
/* @662 */ 	ldy          #__f0
/* @663 */ 	jsr         __fadd
/* @665 */ 	ldx          #19
	jsr          __var_value4_f0			// y2
/* @672 */ 	lda          #__f2
/* @673 */ 	ldx          #__f3
/* @674 */ 	ldy          #__f0
/* @675 */ 	jsr         __fmul
/* @680 */ 	ldx          #3
.__sin_label_681:
/* @682 */ 	lda         .lit.21, X
/* @683 */ 	sta         __f0, X
/* @684 */ 	dex         
/* @685 */ 	bpl         .__sin_label_681
/* @690 */ 	lda          #__f3
/* @691 */ 	ldx          #__f2
/* @692 */ 	ldy          #__f0
/* @693 */ 	jsr         __fadd
/* @695 */ 	ldx          #19
	jsr          __var_value4_f0			// y2
/* @702 */ 	lda          #__f2
/* @703 */ 	ldx          #__f3
/* @704 */ 	ldy          #__f0
/* @705 */ 	jsr         __fmul
/* @710 */ 	ldx          #3
.__sin_label_711:
/* @712 */ 	lda         .lit.22, X
/* @713 */ 	sta         __f0, X
/* @714 */ 	dex         
/* @715 */ 	bpl         .__sin_label_711
/* @720 */ 	lda          #__f3
/* @721 */ 	ldx          #__f2
/* @722 */ 	ldy          #__f0
/* @723 */ 	jsr         __fadd
/* @726 */ 	ldx          #3
.__sin_label_727:
/* @728 */ 	lda         __f3, X
/* @729 */ 	sta         __f1, X
/* @730 */ 	dex         
/* @731 */ 	bpl         .__sin_label_727
/* @733 */ 	ldx          #23
	jsr          __var_value4_f0			// t1
/* @738 */ 	lda          #__f2
/* @739 */ 	ldx          #__f0
/* @740 */ 	ldy          #__f1
/* @742 */ 	jsr         __fdiv
/* @744 */ 	ldx          #28
	jsr          __load_result
/* @745 */ 	lda         #__f2
/* @747 */ 	jsr         __result4
/* @749 */ 	ldy          #30
	jmp          __leave
.func_end___sin:
	.size __sin, .func_end___sin-__sin

	.global sin
	.type sin, @function

sin:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x00,0x01		// Save mask i:0 b:0 l:0 x:0 f:1 
/* @12 */ 	ldx          #0
	jsr          __arg_value4_f0			// x
/* @14 */ 	ldx          #0
/* @16 */ 	jsr         __pushxy0
/* @17 */ 	jsr         __pushf0
/* @19 */ 	ldx         #__f1
/* @20 */ 	ldy          #0
/* @21 */ 	jsr         __sin
/* @23 */ 	jsr         __incsp6
/* @25 */ 	ldx          #8
	jsr          __load_result
/* @26 */ 	lda         #__f1
/* @28 */ 	jsr         __result4
/* @30 */ 	ldy          #10
	jmp          __leave
.func_end_sin:
	.size sin, .func_end_sin-sin

	.global cos
	.type cos, @function

cos:
/* @5 */ 	stx         __result
/* @7 */ 	sty         __result+1
/* @8 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x00,0x01		// Save mask i:0 b:0 l:0 x:0 f:1 
/* @13 */ 	ldx          #0
	jsr          __arg_value4_f1			// x
/* @16 */ 	lda         __f1+3
/* @17 */ 	bpl         .cos_label_40
/* @20 */ 	ldx          #3
.cos_label_21:
/* @23 */ 	lda         __f1, X
/* @24 */ 	sta         __f0, X
/* @25 */ 	dex         
/* @26 */ 	bpl         .cos_label_21
/* @27 */ 	ldy          #3
/* @28 */ 	lda         __f0+3
/* @30 */ 	eor          #128
/* @31 */ 	sta         __f0+3
/* @34 */ 	ldx          #3
.cos_label_35:
/* @36 */ 	lda         __f0, X
/* @37 */ 	sta         __f1, X
/* @38 */ 	dex         
/* @39 */ 	bpl         .cos_label_35
.cos_label_40:
/* @42 */ 	ldx          #1
/* @44 */ 	jsr         __pushxy0
/* @45 */ 	jsr         __pushf1
/* @48 */ 	ldx         #__f0
/* @49 */ 	ldy          #0
/* @50 */ 	jsr         __sin
/* @52 */ 	jsr         __incsp6
/* @54 */ 	ldx          #8
	jsr          __load_result
/* @55 */ 	lda         #__f0
/* @57 */ 	jsr         __result4
/* @59 */ 	ldy          #10
	jmp          __leave
.func_end_cos:
	.size cos, .func_end_cos-cos

	.data
	.p2align  0
twoopi:
	.type   twoopi,@object
	.local  twoopi
	.size   twoopi,4
	.word   1059256707

	.p2align  0
p0:
	.type   p0,@object
	.global p0
	.size   p0,4
	.word   1263481433

	.p2align  0
p1:
	.type   p1,@object
	.global p1
	.size   p1,4
	.word   -896083848

	.p2align  0
p2:
	.type   p2,@object
	.global p2
	.size   p2,4
	.word   1222042850

	.p2align  0
p3:
	.type   p3,@object
	.global p3
	.size   p3,4
	.word   -967287529

	.p2align  0
p4:
	.type   p4,@object
	.global p4
	.size   p4,4
	.word   1125251078

	.p2align  0
q0:
	.type   q0,@object
	.global q0
	.size   q0,4
	.word   1258547151

	.p2align  0
q1:
	.type   q1,@object
	.global q1
	.size   q1,4
	.word   1221021287

	.p2align  0
q2:
	.type   q2,@object
	.global q2
	.size   q2,4
	.word   1175706722

	.p2align  0
q3:
	.type   q3,@object
	.global q3
	.size   q3,4
	.word   1124378443

	.section ".rodata", "aMS", @progbits
.lit.3:
	.byte 0x83
	.byte 0xf9
	.byte 0x22
	.byte 0x3f
	.type .lit.3, @object
	.size .lit.3, 4

.lit.7:
	.byte 0x00
	.byte 0x00
	.byte 0x80
	.byte 0x3e
	.type .lit.7, @object
	.size .lit.7, 4

.lit.9:
	.byte 0x00
	.byte 0x00
	.byte 0x80
	.byte 0x40
	.type .lit.9, @object
	.size .lit.9, 4

.lit.13:
	.byte 0x00
	.byte 0x00
	.byte 0x80
	.byte 0x3f
	.type .lit.13, @object
	.size .lit.13, 4

.lit.14:
	.byte 0x06
	.byte 0xf8
	.byte 0x11
	.byte 0x43
	.type .lit.14, @object
	.size .lit.14, 4

.lit.15:
	.byte 0x17
	.byte 0x5d
	.byte 0x58
	.byte 0xc6
	.type .lit.15, @object
	.size .lit.15, 4

.lit.16:
	.byte 0xe2
	.byte 0xe4
	.byte 0xd6
	.byte 0x48
	.type .lit.16, @object
	.size .lit.16, 4

.lit.17:
	.byte 0x78
	.byte 0xd8
	.byte 0x96
	.byte 0xca
	.type .lit.17, @object
	.size .lit.17, 4

.lit.18:
	.byte 0x59
	.byte 0x32
	.byte 0x4f
	.byte 0x4b
	.type .lit.18, @object
	.size .lit.18, 4

.lit.19:
	.byte 0x4b
	.byte 0xa7
	.byte 0x04
	.byte 0x43
	.type .lit.19, @object
	.size .lit.19, 4

.lit.20:
	.byte 0x62
	.byte 0xdc
	.byte 0x13
	.byte 0x46
	.type .lit.20, @object
	.size .lit.20, 4

.lit.21:
	.byte 0x67
	.byte 0x4e
	.byte 0xc7
	.byte 0x48
	.type .lit.21, @object
	.size .lit.21, 4

.lit.22:
	.byte 0xcf
	.byte 0xe7
	.byte 0x03
	.byte 0x4b
	.type .lit.22, @object
	.size .lit.22, 4

