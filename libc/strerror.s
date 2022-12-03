	.file   "/Users/dallison/Google Drive/c_compiler/libc/strerror.c"
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


	.global strerror
	.type strerror, @function

strerror:
/* @51 */ 	stx         __result
/* @53 */ 	sty         __result+1
/* @54 */ 	ldx          #47
	jsr          __enter_leaf
	.byte        0x21,0x00,0x00		// Save mask i:1 b:1 l:0 x:0 f:0 
/* @58 */ 	ldx          #0
	jsr          __arg_value2_i0			// errnum
/* @72 */ 	lda         __i0+1
/* @74 */ 	and          #128
/* @75 */ 	ora         __i0
/* @76 */ 	sta         __b1
/* @79 */ 	sec         
/* @81 */ 	sbc          #11
/* @82 */ 	bvc         .strerror_label_78
/* @83 */ 	eor          #128
.strerror_label_78:
/* @84 */ 	bmi         .strerror_label_146
/* @87 */ 	lda         __b1
/* @88 */ 	cmp          #11
/* @89 */ 	bne         .strerror_label_682
/* @683 */ 	jmp         .strerror_label_343
.strerror_label_682:
/* @93 */ 	lda         __b1
/* @94 */ 	cmp          #12
/* @95 */ 	bne         .strerror_label_684
/* @685 */ 	jmp         .strerror_label_354
.strerror_label_684:
/* @99 */ 	lda         __b1
/* @100 */ 	cmp          #13
/* @101 */ 	bne         .strerror_label_686
/* @687 */ 	jmp         .strerror_label_365
.strerror_label_686:
/* @105 */ 	lda         __b1
/* @106 */ 	cmp          #14
/* @107 */ 	bne         .strerror_label_688
/* @689 */ 	jmp         .strerror_label_376
.strerror_label_688:
/* @111 */ 	lda         __b1
/* @112 */ 	cmp          #15
/* @113 */ 	bne         .strerror_label_690
/* @691 */ 	jmp         .strerror_label_387
.strerror_label_690:
/* @117 */ 	lda         __b1
/* @118 */ 	cmp          #16
/* @119 */ 	bne         .strerror_label_692
/* @693 */ 	jmp         .strerror_label_398
.strerror_label_692:
/* @123 */ 	lda         __b1
/* @124 */ 	cmp          #17
/* @125 */ 	bne         .strerror_label_694
/* @695 */ 	jmp         .strerror_label_409
.strerror_label_694:
/* @129 */ 	lda         __b1
/* @130 */ 	cmp          #18
/* @131 */ 	bne         .strerror_label_696
/* @697 */ 	jmp         .strerror_label_420
.strerror_label_696:
/* @135 */ 	lda         __b1
/* @136 */ 	cmp          #200
/* @137 */ 	beq         .strerror_label_208
/* @141 */ 	lda         __b1
/* @142 */ 	cmp          #201
/* @143 */ 	beq         .strerror_label_222
/* @145 */ 	jmp         .strerror_label_431
.strerror_label_146:
/* @149 */ 	lda         __b1
/* @150 */ 	cmp          #1
/* @151 */ 	beq         .strerror_label_233
/* @155 */ 	lda         __b1
/* @156 */ 	cmp          #2
/* @157 */ 	beq         .strerror_label_244
/* @161 */ 	lda         __b1
/* @162 */ 	cmp          #3
/* @163 */ 	bne         .strerror_label_712
/* @713 */ 	jmp         .strerror_label_255
.strerror_label_712:
/* @167 */ 	lda         __b1
/* @168 */ 	cmp          #4
/* @169 */ 	bne         .strerror_label_708
/* @709 */ 	jmp         .strerror_label_266
.strerror_label_708:
/* @173 */ 	lda         __b1
/* @174 */ 	cmp          #5
/* @175 */ 	bne         .strerror_label_710
/* @711 */ 	jmp         .strerror_label_277
.strerror_label_710:
/* @179 */ 	lda         __b1
/* @180 */ 	cmp          #6
/* @181 */ 	bne         .strerror_label_698
/* @699 */ 	jmp         .strerror_label_288
.strerror_label_698:
/* @185 */ 	lda         __b1
/* @186 */ 	cmp          #7
/* @187 */ 	bne         .strerror_label_700
/* @701 */ 	jmp         .strerror_label_299
.strerror_label_700:
/* @191 */ 	lda         __b1
/* @192 */ 	cmp          #8
/* @193 */ 	bne         .strerror_label_702
/* @703 */ 	jmp         .strerror_label_310
.strerror_label_702:
/* @197 */ 	lda         __b1
/* @198 */ 	cmp          #9
/* @199 */ 	bne         .strerror_label_704
/* @705 */ 	jmp         .strerror_label_321
.strerror_label_704:
/* @203 */ 	lda         __b1
/* @204 */ 	cmp          #10
/* @205 */ 	bne         .strerror_label_706
/* @707 */ 	jmp         .strerror_label_332
.strerror_label_706:
/* @207 */ 	jmp         .strerror_label_431
.strerror_label_208:
/* @211 */ 	lda         #%lo(.str.1)
/* @212 */ 	sta         __i3
/* @213 */ 	lda         #%hi(.str.1)
/* @214 */ 	sta         __i3+1
/* @216 */ 	lda         #__i3
/* @218 */ 	jsr         __result2
.strerror_label_219:
/* @220 */ 	ldy          #50
	jmp          __leave_leaf
.strerror_label_222:
/* @225 */ 	lda         #%lo(.str.2)
/* @226 */ 	sta         __i3
/* @227 */ 	lda         #%hi(.str.2)
/* @228 */ 	sta         __i3+1
/* @230 */ 	lda         #__i3
/* @231 */ 	jsr         __result2
/* @232 */ 	bra         .strerror_label_219
.strerror_label_233:
/* @236 */ 	lda         #%lo(.str.3)
/* @237 */ 	sta         __i3
/* @238 */ 	lda         #%hi(.str.3)
/* @239 */ 	sta         __i3+1
/* @241 */ 	lda         #__i3
/* @242 */ 	jsr         __result2
/* @243 */ 	bra         .strerror_label_219
.strerror_label_244:
/* @247 */ 	lda         #%lo(.str.4)
/* @248 */ 	sta         __i3
/* @249 */ 	lda         #%hi(.str.4)
/* @250 */ 	sta         __i3+1
/* @252 */ 	lda         #__i3
/* @253 */ 	jsr         __result2
/* @254 */ 	bra         .strerror_label_219
.strerror_label_255:
/* @258 */ 	lda         #%lo(.str.5)
/* @259 */ 	sta         __i3
/* @260 */ 	lda         #%hi(.str.5)
/* @261 */ 	sta         __i3+1
/* @263 */ 	lda         #__i3
/* @264 */ 	jsr         __result2
/* @265 */ 	bra         .strerror_label_219
.strerror_label_266:
/* @269 */ 	lda         #%lo(.str.6)
/* @270 */ 	sta         __i3
/* @271 */ 	lda         #%hi(.str.6)
/* @272 */ 	sta         __i3+1
/* @274 */ 	lda         #__i3
/* @275 */ 	jsr         __result2
/* @276 */ 	bra         .strerror_label_219
.strerror_label_277:
/* @280 */ 	lda         #%lo(.str.7)
/* @281 */ 	sta         __i3
/* @282 */ 	lda         #%hi(.str.7)
/* @283 */ 	sta         __i3+1
/* @285 */ 	lda         #__i3
/* @286 */ 	jsr         __result2
/* @287 */ 	bra         .strerror_label_219
.strerror_label_288:
/* @291 */ 	lda         #%lo(.str.8)
/* @292 */ 	sta         __i3
/* @293 */ 	lda         #%hi(.str.8)
/* @294 */ 	sta         __i3+1
/* @296 */ 	lda         #__i3
/* @297 */ 	jsr         __result2
/* @298 */ 	bra         .strerror_label_219
.strerror_label_299:
/* @302 */ 	lda         #%lo(.str.9)
/* @303 */ 	sta         __i3
/* @304 */ 	lda         #%hi(.str.9)
/* @305 */ 	sta         __i3+1
/* @307 */ 	lda         #__i3
/* @308 */ 	jsr         __result2
/* @309 */ 	bra         .strerror_label_219
.strerror_label_310:
/* @313 */ 	lda         #%lo(.str.10)
/* @314 */ 	sta         __i3
/* @315 */ 	lda         #%hi(.str.10)
/* @316 */ 	sta         __i3+1
/* @318 */ 	lda         #__i3
/* @319 */ 	jsr         __result2
/* @320 */ 	jmp         .strerror_label_219
.strerror_label_321:
/* @324 */ 	lda         #%lo(.str.11)
/* @325 */ 	sta         __i3
/* @326 */ 	lda         #%hi(.str.11)
/* @327 */ 	sta         __i3+1
/* @329 */ 	lda         #__i3
/* @330 */ 	jsr         __result2
/* @331 */ 	jmp         .strerror_label_219
.strerror_label_332:
/* @335 */ 	lda         #%lo(.str.12)
/* @336 */ 	sta         __i3
/* @337 */ 	lda         #%hi(.str.12)
/* @338 */ 	sta         __i3+1
/* @340 */ 	lda         #__i3
/* @341 */ 	jsr         __result2
/* @342 */ 	jmp         .strerror_label_219
.strerror_label_343:
/* @346 */ 	lda         #%lo(.str.13)
/* @347 */ 	sta         __i3
/* @348 */ 	lda         #%hi(.str.13)
/* @349 */ 	sta         __i3+1
/* @351 */ 	lda         #__i3
/* @352 */ 	jsr         __result2
/* @353 */ 	jmp         .strerror_label_219
.strerror_label_354:
/* @357 */ 	lda         #%lo(.str.14)
/* @358 */ 	sta         __i3
/* @359 */ 	lda         #%hi(.str.14)
/* @360 */ 	sta         __i3+1
/* @362 */ 	lda         #__i3
/* @363 */ 	jsr         __result2
/* @364 */ 	jmp         .strerror_label_219
.strerror_label_365:
/* @368 */ 	lda         #%lo(.str.15)
/* @369 */ 	sta         __i3
/* @370 */ 	lda         #%hi(.str.15)
/* @371 */ 	sta         __i3+1
/* @373 */ 	lda         #__i3
/* @374 */ 	jsr         __result2
/* @375 */ 	jmp         .strerror_label_219
.strerror_label_376:
/* @379 */ 	lda         #%lo(.str.16)
/* @380 */ 	sta         __i3
/* @381 */ 	lda         #%hi(.str.16)
/* @382 */ 	sta         __i3+1
/* @384 */ 	lda         #__i3
/* @385 */ 	jsr         __result2
/* @386 */ 	jmp         .strerror_label_219
.strerror_label_387:
/* @390 */ 	lda         #%lo(.str.17)
/* @391 */ 	sta         __i3
/* @392 */ 	lda         #%hi(.str.17)
/* @393 */ 	sta         __i3+1
/* @395 */ 	lda         #__i3
/* @396 */ 	jsr         __result2
/* @397 */ 	jmp         .strerror_label_219
.strerror_label_398:
/* @401 */ 	lda         #%lo(.str.18)
/* @402 */ 	sta         __i3
/* @403 */ 	lda         #%hi(.str.18)
/* @404 */ 	sta         __i3+1
/* @406 */ 	lda         #__i3
/* @407 */ 	jsr         __result2
/* @408 */ 	jmp         .strerror_label_219
.strerror_label_409:
/* @412 */ 	lda         #%lo(.str.19)
/* @413 */ 	sta         __i3
/* @414 */ 	lda         #%hi(.str.19)
/* @415 */ 	sta         __i3+1
/* @417 */ 	lda         #__i3
/* @418 */ 	jsr         __result2
/* @419 */ 	jmp         .strerror_label_219
.strerror_label_420:
/* @423 */ 	lda         #%lo(.str.20)
/* @424 */ 	sta         __i3
/* @425 */ 	lda         #%hi(.str.20)
/* @426 */ 	sta         __i3+1
/* @428 */ 	lda         #__i3
/* @429 */ 	jsr         __result2
/* @430 */ 	jmp         .strerror_label_219
.strerror_label_431:
/* @434 */ 	lda         #%lo(.str.21)
/* @435 */ 	sta         __i3
/* @436 */ 	lda         #%hi(.str.21)
/* @437 */ 	sta         __i3+1
/* @440 */ 	lda         __i3
/* @441 */ 	sta         __i1
/* @442 */ 	lda         __i3+1
/* @443 */ 	sta         __i1+1
/* @445 */ 	ldx          #35
	jsr          __var_addr_i3			// buf
/* @447 */ 	lda         __i3
/* @448 */ 	sta         __i2
/* @449 */ 	lda         __i3+1
/* @450 */ 	sta         __i2+1
.strerror_label_451:
/* @454 */ 	lda         (__i1)
/* @460 */ 	sta         __i3
/* @461 */ 	and          #128
/* @463 */ 	beq         .strerror_label_462
/* @465 */ 	lda          #255
.strerror_label_462:
/* @466 */ 	sta         __i3+1
/* @468 */ 	lda         __i3
/* @469 */ 	ora         __i3+1
/* @471 */ 	beq         .strerror_label_502
/* @474 */ 	lda         __i2
/* @475 */ 	sta         __i3
/* @476 */ 	lda         __i2+1
/* @477 */ 	sta         __i3+1
/* @478 */ 	lda          #__i2
/* @480 */ 	jsr         __rinc21
/* @483 */ 	lda         __i1
/* @484 */ 	sta         __i4
/* @485 */ 	lda         __i1+1
/* @486 */ 	sta         __i4+1
/* @487 */ 	lda          #__i1
/* @488 */ 	jsr         __rinc21
/* @493 */ 	lda         (__i4)
/* @500 */ 	sta         (__i3)
/* @501 */ 	bra         .strerror_label_451
.strerror_label_502:
/* @504 */ 	ldx          #45
	jsr          __var_addr_i3			// buf2
/* @508 */ 	clc         
/* @509 */ 	lda         __i3
/* @510 */ 	adc          #9
/* @511 */ 	sta         __i4
/* @512 */ 	lda         __i3+1
/* @513 */ 	adc          #0
/* @514 */ 	sta         __i4+1
/* @517 */ 	lda         __i4
/* @518 */ 	sta         __i1
/* @519 */ 	lda         __i4+1
/* @520 */ 	sta         __i1+1
/* @523 */ 	lda         __i1
/* @524 */ 	sta         __i3
/* @525 */ 	lda         __i1+1
/* @526 */ 	sta         __i3+1
/* @527 */ 	lda          #__i1
/* @529 */ 	jsr         __rdec21
/* @532 */ 	lda          #0
/* @533 */ 	tay         
/* @534 */ 	sta         (__i3)
/* @535 */ 	lda         __i0
/* @536 */ 	ora         __i0+1
/* @537 */ 	bne         .strerror_label_542
/* @538 */ 	lda          #48
/* @539 */ 	ldy          #0
/* @540 */ 	sta         (__i1)
/* @541 */ 	bra         .strerror_label_613
.strerror_label_542:
.strerror_label_543:
/* @544 */ 	lda         __i0
/* @545 */ 	ora         __i0+1
/* @547 */ 	beq         .strerror_label_610
/* @550 */ 	lda          #10
/* @551 */ 	sta         __i3
/* @553 */ 	stz         __i3+1
/* @557 */ 	lda          #__i4
/* @558 */ 	ldx          #__i0
/* @559 */ 	ldy          #__i3
/* @561 */ 	jsr         __smod2
/* @565 */ 	clc         
/* @566 */ 	lda         __i4
/* @567 */ 	adc          #48
/* @568 */ 	sta         __i3
/* @569 */ 	lda         __i4+1
/* @570 */ 	adc          #0
/* @571 */ 	sta         __i3+1
/* @574 */ 	lda         __i3
/* @575 */ 	sta         __b0
/* @578 */ 	lda         __i1
/* @579 */ 	sta         __i3
/* @580 */ 	lda         __i1+1
/* @581 */ 	sta         __i3+1
/* @582 */ 	lda          #__i1
/* @583 */ 	jsr         __rdec21
/* @586 */ 	lda         __b0
/* @587 */ 	sta         (__i3)
/* @591 */ 	lda          #10
/* @592 */ 	sta         __i3
/* @594 */ 	stz         __i3+1
/* @598 */ 	lda          #__i4
/* @599 */ 	ldx          #__i0
/* @600 */ 	ldy          #__i3
/* @602 */ 	jsr         __sdiv2
/* @605 */ 	lda         __i4
/* @606 */ 	sta         __i0
/* @607 */ 	lda         __i4+1
/* @608 */ 	sta         __i0+1
/* @609 */ 	bra         .strerror_label_543
.strerror_label_610:
/* @611 */ 	lda          #__i1
/* @612 */ 	jsr         __rinc21
.strerror_label_613:
.strerror_label_614:
/* @617 */ 	lda         (__i1)
/* @623 */ 	sta         __i3
/* @624 */ 	and          #128
/* @626 */ 	beq         .strerror_label_625
/* @627 */ 	lda          #255
.strerror_label_625:
/* @628 */ 	sta         __i3+1
/* @630 */ 	lda         __i3
/* @631 */ 	ora         __i3+1
/* @633 */ 	beq         .strerror_label_663
/* @636 */ 	lda         __i2
/* @637 */ 	sta         __i3
/* @638 */ 	lda         __i2+1
/* @639 */ 	sta         __i3+1
/* @640 */ 	lda          #__i2
/* @641 */ 	jsr         __rinc21
/* @644 */ 	lda         __i1
/* @645 */ 	sta         __i4
/* @646 */ 	lda         __i1+1
/* @647 */ 	sta         __i4+1
/* @648 */ 	lda          #__i1
/* @649 */ 	jsr         __rinc21
/* @654 */ 	lda         (__i4)
/* @661 */ 	sta         (__i3)
/* @662 */ 	bra         .strerror_label_614
.strerror_label_663:
/* @666 */ 	lda         __i2
/* @667 */ 	sta         __i3
/* @668 */ 	lda         __i2+1
/* @669 */ 	sta         __i3+1
/* @670 */ 	lda          #__i2
/* @671 */ 	jsr         __rinc21
/* @674 */ 	lda          #0
/* @675 */ 	tay         
/* @676 */ 	sta         (__i3)
/* @678 */ 	ldx          #35
	jsr          __var_addr_i3			// buf
/* @679 */ 	lda         #__i3
/* @680 */ 	jsr         __result2
/* @681 */ 	jmp         .strerror_label_219
.func_end_strerror:
	.size strerror, .func_end_strerror-strerror

	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "Domain error"
	.type .str.1, @object
	.size .str.1, 13

.str.2:
	.asciz "Illegal sequence"
	.type .str.2, @object
	.size .str.2, 17

.str.3:
	.asciz "No such file or directory"
	.type .str.3, @object
	.size .str.3, 26

.str.4:
	.asciz "Out of memory"
	.type .str.4, @object
	.size .str.4, 14

.str.5:
	.asciz "Permission denied"
	.type .str.5, @object
	.size .str.5, 18

.str.6:
	.asciz "No such device"
	.type .str.6, @object
	.size .str.6, 15

.str.7:
	.asciz "Too many open files"
	.type .str.7, @object
	.size .str.7, 20

.str.8:
	.asciz "Device or resource busy"
	.type .str.8, @object
	.size .str.8, 24

.str.9:
	.asciz "Invalid argument"
	.type .str.9, @object
	.size .str.9, 17

.str.10:
	.asciz "No space left on device"
	.type .str.10, @object
	.size .str.10, 24

.str.11:
	.asciz "File exists"
	.type .str.11, @object
	.size .str.11, 12

.str.12:
	.asciz "Try again"
	.type .str.12, @object
	.size .str.12, 10

.str.13:
	.asciz "I/O error"
	.type .str.13, @object
	.size .str.13, 10

.str.14:
	.asciz "Interrupted system call"
	.type .str.14, @object
	.size .str.14, 24

.str.15:
	.asciz "Function not implemented"
	.type .str.15, @object
	.size .str.15, 25

.str.16:
	.asciz "Illegal seek"
	.type .str.16, @object
	.size .str.16, 13

.str.17:
	.asciz "Range error"
	.type .str.17, @object
	.size .str.17, 12

.str.18:
	.asciz "Bad file number"
	.type .str.18, @object
	.size .str.18, 16

.str.19:
	.asciz "Exec format error"
	.type .str.19, @object
	.size .str.19, 18

.str.20:
	.asciz "Unknown OS specific error"
	.type .str.20, @object
	.size .str.20, 26

.str.21:
	.asciz "Unknown error: "
	.type .str.21, @object
	.size .str.21, 16

