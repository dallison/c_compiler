	.file   "libc/wchar.c"
	.file 1 "libc/include/wchar.h"
	.file 2 "libc/include/stddef.h"
	.file 3 "libc/include/limits.h"
	.file 4 "libc/include/stdio.h"
	.file 5 "libc/include/stdarg.h"
	.file 6 "libc/include/errno.h"
	.file 7 "libc/include/syscall.h"
	.file 8 "libc/include/davecc_guest_syscalls.h"
	.file 9 "libc/include/stdlib.h"
	.file 10 "libc/include/string.h"
	.file 11 "libc/include/time.h"
	.file 12 "libc/wchar.c"
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

.PCbegin:
	.section ".text.reset_state", "ax", @progbits
	.local  reset_state
	.type reset_state, @function

reset_state:
	ldx          #5
	jsr          __enter_leaf_nomask
	ldx          #0
	jsr          __arg_value2_i0			// state
	.loc 13 15 1
	lda         __i0
	sta         __i1
	lda         __i0+1
	sta         __i1+1
	ldy          #1
	lda          #0
.reset_state_label_24:
	sta         (__i1), Y
	dey         
	bpl         .reset_state_label_24
	.loc 13 16 1
	ldy          #2
	lda          #0
.reset_state_label_35:
	sta         (__i1), Y
	iny         
	cpy          #4
	bne         .reset_state_label_35
	.loc 13 17 1
	lda          #0
	ldy          #4
	sta         (__i1), Y
	.loc 13 18 1
	lda          #0
	ldy          #5
	sta         (__i1), Y
	.loc 13 19 1
	lda          #0
	ldy          #6
	sta         (__i1), Y
	ldy          #8
	jmp          __leave_leaf_void_nomask
.func_end_reset_state:
	.size reset_state, .func_end_reset_state-reset_state

	.section ".text.mbsinit", "ax", @progbits
	.global mbsinit
	.type mbsinit, @function

mbsinit:
	lda          #7
	jsr          __enter_leaf_res_nomask
	ldx          #0
	jsr          __arg_value2_i0			// state
	.loc 13 23 1
	ldx          #1
	lda         __i0
	bne         .mbsinit_label_17
	lda         __i0+1
	beq         .mbsinit_label_18
.mbsinit_label_17:
	dex         
.mbsinit_label_18:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i1			// __invented__7
	lda         __b0
	sta         (__i1)
	lda         __i0
	bne         .mbsinit_label_38
	lda         __i0+1
	beq         .mbsinit_label_133
.mbsinit_label_38:
	ldy          #4
	lda         (__i0), Y
	sta         __i2
	stz         __i2+1
	ldx          #1
	ora         __i2+1
	beq         .mbsinit_label_63
	dex         
.mbsinit_label_63:
	stx         __b0
	ldx          #5
	jsr          __var_addr_i3			// __invented__8
	lda         __b0
	sta         (__i3)
	lda         __i2
	ora         __i2+1
	bne         .mbsinit_label_116
	ldy          #6
	lda         (__i0), Y
	sta         __i2
	stz         __i2+1
	ldx          #1
	ora         __i2+1
	beq         .mbsinit_label_100
	dex         
.mbsinit_label_100:
	txa         
	sta         (__i3)
.mbsinit_label_116:
	lda         (__i3)
	sta         (__i1)
.mbsinit_label_133:
	lda         (__i1)
	sta         __i2
	stz         __i2+1
	lda         #__i2
	jsr         __result2
	ldy          #12
	jmp          __leave_leaf_nomask
.func_end_mbsinit:
	.size mbsinit, .func_end_mbsinit-mbsinit

	.section ".text.decode_utf8", "ax", @progbits
	.local  decode_utf8
	.type decode_utf8, @function

decode_utf8:
	lda          #31
	jsr          __enter_res
	.byte        0x48,0x00,0x00		// Save mask i:8 b:2 l:0 x:0 f:0 
	ldx          #4
	jsr          __arg_value2_i4			// count
	ldx          #6
	jsr          __arg_value2_i5			// state
	ldx          #2
	jsr          __arg_value2_i6			// string
	ldx          #0
	jsr          __arg_value2_i7			// output
	.loc 13 29 20
	stz         __i0
	stz         __i0+1
	lda          #__i0
	ldx          #27
	jsr         __set_var_value2
	.loc 13 33 17
	lda         __i4
	ora         __i4+1
	bne         .decode_utf8_label_113
	.loc 13 33 17
	lda          #254
	sta         __i0
	inc          A
	sta         __i0+1
	ldx          #32
	lda          #__i0
	jsr         __load_result_value2
.decode_utf8_label_110:
	ldy          #42
	jmp          __leave
.decode_utf8_label_113:
	.loc 13 34 26
	ldy          #4
	lda         (__i5), Y
	sta         __i0
	stz         __i0+1
	beq         .decode_utf8_label_1391
	jmp         .decode_utf8_label_634
.decode_utf8_label_1391:
	.loc 13 35 56
	ldx          #27
	jsr          __var_value2_i0			// consumed
	ldx          #27
	jsr          __var_addr_i1			// consumed
	lda          #__i1
	jsr         __inc21
	clc         
	lda         __i6
	adc         __i0
	sta         __i1
	lda         __i6+1
	adc         __i0+1
	sta         __i1+1
	lda         (__i1)
	sta         __b0
	sta         __b2
	.loc 13 36 19
	lda         __b0
	sta         __i0
	stz         __i0+1
	cmp          #128
	lda         __i0+1
	sbc          #0
	bvc         .decode_utf8_label_169
	eor          #128
.decode_utf8_label_169:
	bpl         .decode_utf8_label_249
	.loc 13 37 27
	lda         __i7
	bne         .decode_utf8_label_179
	lda         __i7+1
	beq         .decode_utf8_label_200
.decode_utf8_label_179:
	.loc 13 37 27
	lda         __b2
	sta         __i0
	stz         __i0+1
	sta         (__i7)
	lda         __i0+1
	ldy          #1
	sta         (__i7), Y
.decode_utf8_label_200:
	.loc 13 38 1
	lda         __b2
	sta         __i0
	stz         __i0+1
	bne         .decode_utf8_label_223
	ldx          #6
	jsr          __var_addr_i0			// __invented__24
	ldy          #1
	lda          #0
.decode_utf8_label_217:
	sta         (__i0), Y
	dey         
	bpl         .decode_utf8_label_217
	bra         .decode_utf8_label_234
.decode_utf8_label_223:
	ldx          #6
	jsr          __var_addr_i0			// __invented__24
	lda          #1
	ldy          #0
	sta         (__i0)
	dec          A
	iny         
	sta         (__i0), Y
.decode_utf8_label_234:
	ldx          #6
	jsr          __var_addr_i0			// __invented__24
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	ldx          #32
	lda          #__i1
	jsr         __load_result_value2
	jmp         .decode_utf8_label_110
.decode_utf8_label_249:
	.loc 13 40 37
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #0
	cmp          #194
	lda         __i0+1
	sbc          #0
	bvc         .decode_utf8_label_261
	eor          #128
.decode_utf8_label_261:
	bmi         .decode_utf8_label_259
	inx         
.decode_utf8_label_259:
	stx         __b0
	ldx          #7
	jsr          __var_addr_i8			// __invented__25
	lda         __b0
	sta         (__i8)
	lda         __i0
	cmp          #194
	lda         __i0+1
	sbc          #0
	bvc         .decode_utf8_label_280
	eor          #128
.decode_utf8_label_280:
	bmi         .decode_utf8_label_316
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #0
	lda          #223
	cmp         __i0
	txa         
	sbc         __i0+1
	bvc         .decode_utf8_label_298
	eor          #128
.decode_utf8_label_298:
	bmi         .decode_utf8_label_296
	inx         
.decode_utf8_label_296:
	txa         
	sta         (__i8)
.decode_utf8_label_316:
	lda         (__i8)
	beq         .decode_utf8_label_366
	.loc 13 41 1
	lda         __b2
	sta         __i0
	stz         __i0+1
	and          #31
	sta         __i1
	stz         __i1+1
	lda         __i5
	sta         __i0
	lda         __i5+1
	sta         __i0+1
	lda         __i1
	sta         (__i0)
	lda         __i1+1
	ldy          #1
	sta         (__i0), Y
	.loc 13 42 1
	lda          #2
	ldy          #5
	sta         (__i0), Y
	jmp         .decode_utf8_label_629
.decode_utf8_label_366:
	.loc 13 43 44
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #0
	cmp          #224
	lda         __i0+1
	sbc          #0
	bvc         .decode_utf8_label_378
	eor          #128
.decode_utf8_label_378:
	bmi         .decode_utf8_label_376
	inx         
.decode_utf8_label_376:
	stx         __b0
	ldx          #8
	jsr          __var_addr_i9			// __invented__26
	lda         __b0
	sta         (__i9)
	lda         __i0
	cmp          #224
	lda         __i0+1
	sbc          #0
	bvc         .decode_utf8_label_397
	eor          #128
.decode_utf8_label_397:
	bmi         .decode_utf8_label_433
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #0
	lda          #239
	cmp         __i0
	txa         
	sbc         __i0+1
	bvc         .decode_utf8_label_415
	eor          #128
.decode_utf8_label_415:
	bmi         .decode_utf8_label_413
	inx         
.decode_utf8_label_413:
	txa         
	sta         (__i9)
.decode_utf8_label_433:
	lda         (__i9)
	beq         .decode_utf8_label_482
	.loc 13 44 1
	lda         __b2
	sta         __i0
	stz         __i0+1
	and          #15
	sta         __i1
	stz         __i1+1
	lda         __i5
	sta         __i0
	lda         __i5+1
	sta         __i0+1
	lda         __i1
	sta         (__i0)
	lda         __i1+1
	ldy          #1
	sta         (__i0), Y
	.loc 13 45 1
	lda          #3
	ldy          #5
	sta         (__i0), Y
	jmp         .decode_utf8_label_628
.decode_utf8_label_482:
	.loc 13 46 44
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #0
	cmp          #240
	lda         __i0+1
	sbc          #0
	bvc         .decode_utf8_label_494
	eor          #128
.decode_utf8_label_494:
	bmi         .decode_utf8_label_492
	inx         
.decode_utf8_label_492:
	stx         __b0
	ldx          #9
	jsr          __var_addr_i10			// __invented__27
	lda         __b0
	sta         (__i10)
	lda         __i0
	cmp          #240
	lda         __i0+1
	sbc          #0
	bvc         .decode_utf8_label_513
	eor          #128
.decode_utf8_label_513:
	bmi         .decode_utf8_label_549
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #0
	lda          #244
	cmp         __i0
	txa         
	sbc         __i0+1
	bvc         .decode_utf8_label_531
	eor          #128
.decode_utf8_label_531:
	bmi         .decode_utf8_label_529
	inx         
.decode_utf8_label_529:
	txa         
	sta         (__i10)
.decode_utf8_label_549:
	lda         (__i10)
	beq         .decode_utf8_label_598
	.loc 13 47 1
	lda         __b2
	sta         __i0
	stz         __i0+1
	and          #7
	sta         __i1
	stz         __i1+1
	lda         __i5
	sta         __i0
	lda         __i5+1
	sta         __i0+1
	lda         __i1
	sta         (__i0)
	lda         __i1+1
	ldy          #1
	sta         (__i0), Y
	.loc 13 48 1
	lda          #4
	ldy          #5
	sta         (__i0), Y
	bra         .decode_utf8_label_627
.decode_utf8_label_598:
	.loc 13 50 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #201
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 13 51 1
	jsr         __pushi5
	jsr         reset_state
	jsr         __incsp2
	.loc 13 52 1
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #32
	lda          #__i0
	jsr         __load_result_value2
	jmp         .decode_utf8_label_110
.decode_utf8_label_627:
.decode_utf8_label_628:
.decode_utf8_label_629:
	.loc 13 54 1
	lda          #1
	ldy          #4
	sta         (__i5), Y
.decode_utf8_label_634:
	.loc 13 57 64
.decode_utf8_label_636:
	lda         __i5
	sta         __i0
	lda         __i5+1
	sta         __i0+1
	ldy          #4
	lda         (__i0), Y
	sta         __i1
	stz         __i1+1
	iny         
	lda         (__i0), Y
	sta         __i0
	stz         __i0+1
	ldx          #0
	lda         __i1
	cmp         __i0
	lda         __i1+1
	sbc         __i0+1
	bvc         .decode_utf8_label_677
	eor          #128
.decode_utf8_label_677:
	bpl         .decode_utf8_label_675
	inx         
.decode_utf8_label_675:
	stx         __b0
	ldx          #10
	jsr          __var_addr_i8			// __invented__28
	lda         __b0
	sta         (__i8)
	lda         __i1
	cmp         __i0
	lda         __i1+1
	sbc         __i0+1
	bvc         .decode_utf8_label_697
	eor          #128
.decode_utf8_label_697:
	bpl         .decode_utf8_label_730
	ldx          #27
	jsr          __var_value2_i0			// consumed
	ldx          #1
	lda         __i0+1
	cmp         __i4+1
	bcc         .decode_utf8_label_711
	bne         .decode_utf8_label_710
	lda         __i0
	cmp         __i4
	bcc         .decode_utf8_label_711
.decode_utf8_label_710:
	dex         
.decode_utf8_label_711:
	txa         
	sta         (__i8)
.decode_utf8_label_730:
	lda         (__i8)
	bne         .decode_utf8_label_1393
	jmp         .decode_utf8_label_904
.decode_utf8_label_1393:
	.loc 13 58 53
	ldx          #27
	jsr          __var_value2_i0			// consumed
	clc         
	lda         __i6
	adc         __i0
	sta         __i1
	lda         __i6+1
	adc         __i0+1
	sta         __i1+1
	lda         (__i1)
	sta         __b0
	sta         __b3
	.loc 13 59 28
	lda         __b0
	sta         __i0
	stz         __i0+1
	and          #192
	sta         __i1
	stz         __i1+1
	cmp          #128
	bne         .decode_utf8_label_783
	lda         __i1+1
	beq         .decode_utf8_label_818
.decode_utf8_label_783:
	.loc 13 60 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #201
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 13 61 1
	jsr         __pushi5
	jsr         reset_state
	jsr         __incsp2
	.loc 13 62 1
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #32
	lda          #__i0
	jsr         __load_result_value2
	jmp         .decode_utf8_label_110
.decode_utf8_label_818:
	.loc 13 64 1
	lda         __i5
	sta         __i0
	lda         __i5+1
	sta         __i0+1
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	lda         __i1
	asl          A
	sta         __i2
	lda         __i1+1
	rol          A
	sta         __i2+1
	ldx          #5
.decode_utf8_label_845:
	asl         __i2
	rol         __i2+1
	dex         
	bne         .decode_utf8_label_845
	lda         __b3
	sta         __i1
	stz         __i1+1
	and          #63
	sta         __i3
	stz         __i3+1
	lda         __i2
	ora         __i3
	sta         __i1
	lda         __i2+1
	ora         __i3+1
	sta         __i1+1
	lda         __i1
	sta         (__i0)
	lda         __i1+1
	ldy          #1
	sta         (__i0), Y
	.loc 13 65 1
	clc         
	lda         __i0
	adc          #4
	sta         __i1
	lda         __i0+1
	adc          #0
	sta         __i1+1
	lda          #__i1
	jsr         __inc1
	.loc 13 66 1
	ldx          #27
	jsr          __var_addr_i0			// consumed
	lda          #__i0
	jsr         __inc21
	jmp         .decode_utf8_label_636
.decode_utf8_label_904:
	.loc 13 68 41
	lda         __i5
	sta         __i0
	lda         __i5+1
	sta         __i0+1
	ldy          #4
	lda         (__i0), Y
	sta         __i1
	stz         __i1+1
	iny         
	lda         (__i0), Y
	sta         __i0
	stz         __i0+1
	lda         __i1
	cmp         __i0
	lda         __i1+1
	sbc         __i0+1
	bvc         .decode_utf8_label_942
	eor          #128
.decode_utf8_label_942:
	bpl         .decode_utf8_label_959
	.loc 13 68 41
	lda          #254
	sta         __i0
	inc          A
	sta         __i0+1
	ldx          #32
	lda          #__i0
	jsr         __load_result_value2
	jmp         .decode_utf8_label_110
.decode_utf8_label_959:
	.loc 13 70 1
	lda         __i5
	sta         __i0
	lda         __i5+1
	sta         __i0+1
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	lda          #__i1
	ldx          #12
	jsr         __set_var_value2
	.loc 13 71 1
	ldy          #5
	lda         (__i0), Y
	sta         __i0
	stz         __i0+1
	cmp          #2
	bne         .decode_utf8_label_1020
	lda         __i0+1
	bne         .decode_utf8_label_1020
	ldx          #16
	jsr          __var_addr_i0			// __invented__29
	ldy          #3
	ldx          #3
.decode_utf8_label_1013:
	lda         .lit.53, X
	sta         (__i0), Y
	dey         
	dex         
	bpl         .decode_utf8_label_1013
	bra         .decode_utf8_label_1085
.decode_utf8_label_1020:
	ldy          #5
	lda         (__i5), Y
	sta         __i0
	stz         __i0+1
	cmp          #3
	bne         .decode_utf8_label_1056
	lda         __i0+1
	bne         .decode_utf8_label_1056
	ldx          #20
	jsr          __var_addr_i0			// __invented__30
	ldy          #3
	ldx          #3
.decode_utf8_label_1049:
	lda         .lit.56, X
	sta         (__i0), Y
	dey         
	dex         
	bpl         .decode_utf8_label_1049
	bra         .decode_utf8_label_1070
.decode_utf8_label_1056:
	ldx          #20
	jsr          __var_addr_i0			// __invented__30
	ldy          #3
	ldx          #3
.decode_utf8_label_1064:
	lda         .lit.57, X
	sta         (__i0), Y
	dey         
	dex         
	bpl         .decode_utf8_label_1064
.decode_utf8_label_1070:
	ldx          #20
	jsr          __var_addr_i0			// __invented__30
	ldx          #__i0
	ldy          #__l0
	jsr          __load_indirect4
	ldx          #16
	jsr          __var_addr_i0			// __invented__29
	ldx          #__l0
	ldy          #__i0
	jsr          __store_indirect4
.decode_utf8_label_1085:
	ldx          #16
	jsr          __var_addr_i0			// __invented__29
	ldx          #__i0
	ldy          #__l0
	jsr          __load_indirect4
	lda          #__l0
	ldx          #22
	jsr         __set_var_value2
	.loc 13 76 30
	ldx          #12
	jsr          __var_value2_i0			// value
	ldx          #22
	jsr          __var_value2_i1			// minimum
	ldx          #1
	lda         __i0+1
	cmp         __i1+1
	bcc         .decode_utf8_label_1109
	bne         .decode_utf8_label_1108
	lda         __i0
	cmp         __i1
	bcc         .decode_utf8_label_1109
.decode_utf8_label_1108:
	dex         
.decode_utf8_label_1109:
	stx         __b0
	ldx          #23
	jsr          __var_addr_i9			// __invented__31
	lda         __b0
	sta         (__i9)
	lda         __i0+1
	cmp         __i1+1
	bcc         .decode_utf8_label_1178
	bne         .decode_utf8_label_1130
	lda         __i0
	cmp         __i1
	bcc         .decode_utf8_label_1178
.decode_utf8_label_1130:
	ldx          #12
	jsr          __var_value2_i0			// value
	lda         __i0
	sta         __l0
	lda         __i0+1
	sta         __l0+1
	stz         __l0+2
	stz         __l0+3
	ldx          #0
	lda          #255
	cmp         __l0
	sbc         __l0+1
	lda          #16
	sbc         __l0+2
	txa         
	sbc         __l0+3
	bvc         .decode_utf8_label_1156
	eor          #128
.decode_utf8_label_1156:
	bpl         .decode_utf8_label_1154
	inx         
.decode_utf8_label_1154:
	txa         
	sta         (__i9)
.decode_utf8_label_1178:
	lda         (__i9)
	sta         __b0
	ldx          #24
	jsr          __var_addr_i10			// __invented__32
	lda         __b0
	sta         (__i10)
	bne         .decode_utf8_label_1275
	ldx          #12
	jsr          __var_value2_i0			// value
	ldx          #1
	lda         __i0+1
	cmp          #216
	bcc         .decode_utf8_label_1203
	bne         .decode_utf8_label_1204
	lda         __i0
	cmp          #0
	bcs         .decode_utf8_label_1204
.decode_utf8_label_1203:
	dex         
.decode_utf8_label_1204:
	stx         __b0
	ldx          #25
	jsr          __var_addr_i1			// __invented__33
	lda         __b0
	sta         (__i1)
	lda         __i0+1
	cmp          #216
	bcc         .decode_utf8_label_1258
	bne         .decode_utf8_label_1224
	lda         __i0
	cmp          #0
	bcc         .decode_utf8_label_1258
.decode_utf8_label_1224:
	ldx          #12
	jsr          __var_value2_i0			// value
	ldx          #1
	lda          #223
	cmp         __i0+1
	bcc         .decode_utf8_label_1238
	bne         .decode_utf8_label_1239
	lda          #255
	cmp         __i0
	bcs         .decode_utf8_label_1239
.decode_utf8_label_1238:
	dex         
.decode_utf8_label_1239:
	txa         
	sta         (__i1)
.decode_utf8_label_1258:
	lda         (__i1)
	sta         (__i10)
.decode_utf8_label_1275:
	lda         (__i10)
	sta         __b0
	ldx          #4
	jsr          __var_addr_i11			// __invented__34
	lda         __b0
	sta         (__i11)
	bne         .decode_utf8_label_1320
	ldx          #12
	jsr          __var_value2_i0			// value
	ldx          #1
	lda          #127
	cmp         __i0+1
	bcc         .decode_utf8_label_1301
	bne         .decode_utf8_label_1300
	lda          #255
	cmp         __i0
	bcc         .decode_utf8_label_1301
.decode_utf8_label_1300:
	dex         
.decode_utf8_label_1301:
	txa         
	sta         (__i11)
.decode_utf8_label_1320:
	lda         (__i11)
	beq         .decode_utf8_label_1360
	.loc 13 77 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #201
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 13 78 1
	jsr         __pushi5
	jsr         reset_state
	jsr         __incsp2
	.loc 13 79 1
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #32
	lda          #__i0
	jsr         __load_result_value2
	jmp         .decode_utf8_label_110
.decode_utf8_label_1360:
	.loc 13 81 1
	jsr         __pushi5
	jsr         reset_state
	jsr         __incsp2
	.loc 13 82 27
	lda         __i7
	bne         .decode_utf8_label_1366
	lda         __i7+1
	beq         .decode_utf8_label_1384
.decode_utf8_label_1366:
	.loc 13 82 27
	ldx          #12
	jsr          __var_value2_i0			// value
	lda         __i0
	sta         (__i7)
	lda         __i0+1
	ldy          #1
	sta         (__i7), Y
.decode_utf8_label_1384:
	.loc 13 83 1
	ldx          #27
	jsr          __var_value2_i0			// consumed
	ldx          #32
	lda          #__i0
	jsr         __load_result_value2
	jmp         .decode_utf8_label_110
.func_end_decode_utf8:
	.size decode_utf8, .func_end_decode_utf8-decode_utf8

	.section ".text.mbrtowc", "ax", @progbits
	.global mbrtowc
	.type mbrtowc, @function

mbrtowc:
	lda          #7
	jsr          __enter_res
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
	ldx          #6
	jsr          __arg_value2_i4			// state
	ldx          #2
	jsr          __arg_value2_i0			// string
	ldx          #4
	jsr          __arg_value2_i1			// count
	ldx          #0
	jsr          __arg_value2_i2			// output
	.loc 13 88 26
	lda         __i4
	bne         .mbrtowc_label_49
	lda         __i4+1
	bne         .mbrtowc_label_49
	.loc 13 88 26
	lda         #%lo(internal_input_state)
	sta         __i1
	lda         #%hi(internal_input_state)
	sta         __i1+1
	lda         __i1
	sta         __i4
	lda         __i1+1
	sta         __i4+1
.mbrtowc_label_49:
	.loc 13 89 27
	lda         __i0
	bne         .mbrtowc_label_73
	lda         __i0+1
	bne         .mbrtowc_label_73
	.loc 13 90 1
	jsr         __pushi4
	jsr         reset_state
	jsr         __incsp2
	.loc 13 91 1
	stz         __i0
	stz         __i0+1
	ldx          #8
	lda          #__i0
	jsr         __load_result_value2
	ldy          #18
	jmp          __leave
.mbrtowc_label_73:
	.loc 13 93 1
	ldy          #10
	jsr          __leave
	ldx         __result
	ldy         __result+1
	jmp         decode_utf8
.func_end_mbrtowc:
	.size mbrtowc, .func_end_mbrtowc-mbrtowc

	.section ".text.mbrlen", "ax", @progbits
	.global mbrlen
	.type mbrlen, @function

mbrlen:
	lda          #7
	jsr          __enter_res_nomask
	ldx          #4
	jsr          __arg_value2_i0			// state
	ldx          #2
	jsr          __arg_value2_i1			// count
	ldx          #0
	jsr          __arg_value2_i2			// string
	.loc 13 98 1
	jsr         __pushi0
	jsr         __pushi1
	jsr         __pushi2
	stz         __i3
	stz         __i3+1
	jsr         __pushi3
	ldx         #__i3
	ldy          #0
	jsr         mbrtowc
	ldx          #8
	lda          #__i3
	jsr         __load_result_value2
	ldy          #16
	jmp          __leave_nomask
.func_end_mbrlen:
	.size mbrlen, .func_end_mbrlen-mbrlen

	.section ".text.wcrtomb", "ax", @progbits
	.global wcrtomb
	.type wcrtomb, @function

wcrtomb:
	lda          #10
	jsr          __enter_res
	.byte        0x06,0x00,0x00		// Save mask i:6 b:0 l:0 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i5			// wide
	ldx          #4
	jsr          __arg_value2_i6			// state
	ldx          #0
	jsr          __arg_value2_i7			// output
	.loc 13 103 40
	lda         __i5
	sta         __i4
	lda         __i5+1
	sta         __i4+1
	.loc 13 104 26
	lda         __i6
	bne         .wcrtomb_label_76
	lda         __i6+1
	bne         .wcrtomb_label_76
	.loc 13 104 26
	lda         #%lo(internal_output_state)
	sta         __i0
	lda         #%hi(internal_output_state)
	sta         __i0+1
	lda         __i0
	sta         __i6
	lda         __i0+1
	sta         __i6+1
.wcrtomb_label_76:
	.loc 13 105 27
	lda         __i7
	bne         .wcrtomb_label_102
	lda         __i7+1
	bne         .wcrtomb_label_102
	.loc 13 106 1
	jsr         __pushi6
	jsr         reset_state
	jsr         __incsp2
	.loc 13 107 1
	lda          #1
	sta         __i0
	dec          A
	stz         __i0+1
	ldx          #11
	lda          #__i0
	jsr         __load_result_value2
.wcrtomb_label_99:
	ldy          #19
	jmp          __leave
.wcrtomb_label_102:
	.loc 13 109 1
	jsr         __pushi6
	jsr         reset_state
	jsr         __incsp2
	.loc 13 111 39
	ldx          #0
	lda         __i5
	cmp          #0
	lda         __i5+1
	sbc          #0
	bvc         .wcrtomb_label_112
	eor          #128
.wcrtomb_label_112:
	bpl         .wcrtomb_label_110
	inx         
.wcrtomb_label_110:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i0			// __invented__53
	lda         __b0
	sta         (__i0)
	lda         __i5
	cmp          #0
	lda         __i5+1
	sbc          #0
	bvc         .wcrtomb_label_131
	eor          #128
.wcrtomb_label_131:
	bmi         .wcrtomb_label_176
	lda         __i4
	sta         __l0
	lda         __i4+1
	sta         __l0+1
	stz         __l0+2
	stz         __l0+3
	ldx          #0
	lda          #255
	cmp         __l0
	sbc         __l0+1
	lda          #16
	sbc         __l0+2
	txa         
	sbc         __l0+3
	bvc         .wcrtomb_label_154
	eor          #128
.wcrtomb_label_154:
	bpl         .wcrtomb_label_152
	inx         
.wcrtomb_label_152:
	txa         
	sta         (__i0)
.wcrtomb_label_176:
	lda         (__i0)
	sta         __b0
	ldx          #5
	jsr          __var_addr_i1			// __invented__54
	lda         __b0
	sta         (__i1)
	bne         .wcrtomb_label_266
	ldx          #1
	lda         __i4+1
	cmp          #216
	bcc         .wcrtomb_label_198
	bne         .wcrtomb_label_199
	lda         __i4
	cmp          #0
	bcs         .wcrtomb_label_199
.wcrtomb_label_198:
	dex         
.wcrtomb_label_199:
	stx         __b0
	ldx          #6
	jsr          __var_addr_i2			// __invented__55
	lda         __b0
	sta         (__i2)
	lda         __i4+1
	cmp          #216
	bcc         .wcrtomb_label_249
	bne         .wcrtomb_label_218
	lda         __i4
	cmp          #0
	bcc         .wcrtomb_label_249
.wcrtomb_label_218:
	ldx          #1
	lda          #223
	cmp         __i4+1
	bcc         .wcrtomb_label_229
	bne         .wcrtomb_label_230
	lda          #255
	cmp         __i4
	bcs         .wcrtomb_label_230
.wcrtomb_label_229:
	dex         
.wcrtomb_label_230:
	txa         
	sta         (__i2)
.wcrtomb_label_249:
	lda         (__i2)
	sta         (__i1)
.wcrtomb_label_266:
	lda         (__i1)
	beq         .wcrtomb_label_302
	.loc 13 112 1
	lda          #214
	sta         __i2
	lda          #3
	sta         __i2+1
	lda          #201
	ldy          #0
	sta         (__i2)
	tya         
	iny         
	sta         (__i2), Y
	.loc 13 113 1
	lda          #255
	sta         __i2
	sta         __i2+1
	ldx          #11
	lda          #__i2
	jsr         __load_result_value2
	jmp         .wcrtomb_label_99
.wcrtomb_label_302:
	.loc 13 115 19
	lda         __i4+1
	cmp          #0
	bcc         .wcrtomb_label_304
	bne         .wcrtomb_label_330
	lda         __i4
	cmp          #128
	bcs         .wcrtomb_label_330
.wcrtomb_label_304:
	.loc 13 116 1
	lda         __i4
	sta         (__i7)
	.loc 13 117 1
	lda          #1
	sta         __i2
	dec          A
	stz         __i2+1
	ldx          #11
	lda          #__i2
	jsr         __load_result_value2
	jmp         .wcrtomb_label_99
.wcrtomb_label_330:
	.loc 13 119 20
	lda         __i4+1
	cmp          #8
	bcc         .wcrtomb_label_332
	bne         .wcrtomb_label_427
	lda         __i4
	cmp          #0
	bcs         .wcrtomb_label_427
.wcrtomb_label_332:
	.loc 13 120 1
	lda         __i4
	sta         __i2
	lda         __i4+1
	sta         __i2+1
	lsr          A
	sta         __i3+1
	lda         __i2
	ror          A
	sta         __i3
	ldx          #5
.wcrtomb_label_359:
	lsr         __i3+1
	ror         __i3
	dex         
	bne         .wcrtomb_label_359
	lda         __i3
	ora          #192
	sta         __i8
	lda         __i3+1
	sta         __i8+1
	lda         __i8
	sta         __b0
	lda         __i7
	sta         __i3
	lda         __i7+1
	sta         __i3+1
	lda         __b0
	sta         (__i3)
	.loc 13 121 1
	lda         __i2
	and          #63
	sta         __i8
	stz         __i8+1
	ora          #128
	sta         __i2
	lda         __i8+1
	sta         __i2+1
	lda         __i2
	ldy          #1
	sta         (__i3), Y
	.loc 13 122 1
	lda          #2
	sta         __i2
	lda          #0
	stz         __i2+1
	ldx          #11
	lda          #__i2
	jsr         __load_result_value2
	jmp         .wcrtomb_label_99
.wcrtomb_label_427:
	.loc 13 124 22
	lda         __i4
	sta         __l0
	lda         __i4+1
	sta         __l0+1
	stz         __l0+2
	stz         __l0+3
	lda         __l0
	cmp          #0
	lda         __l0+1
	sbc          #0
	lda         __l0+2
	sbc          #1
	lda         __l0+3
	sbc          #0
	bvc         .wcrtomb_label_438
	eor          #128
.wcrtomb_label_438:
	bmi         .wcrtomb_label_739
	jmp         .wcrtomb_label_578
.wcrtomb_label_739:
	.loc 13 125 1
	lda         __i4
	sta         __i2
	lda         __i4+1
	sta         __i2+1
	sta         __i3
	lda          #0
	stz         __i3+1
	ldx          #4
.wcrtomb_label_466:
	lsr         __i3+1
	ror         __i3
	dex         
	bne         .wcrtomb_label_466
	lda         __i3
	ora          #224
	sta         __i8
	lda         __i3+1
	sta         __i8+1
	lda         __i8
	sta         __b0
	lda         __i7
	sta         __i3
	lda         __i7+1
	sta         __i3+1
	lda         __b0
	sta         (__i3)
	.loc 13 126 1
	lda         __i2+1
	lsr          A
	sta         __i8+1
	lda         __i2
	ror          A
	sta         __i8
	ldx          #5
.wcrtomb_label_507:
	lsr         __i8+1
	ror         __i8
	dex         
	bne         .wcrtomb_label_507
	lda         __i8
	and          #63
	sta         __i9
	stz         __i9+1
	ora          #128
	sta         __i8
	lda         __i9+1
	sta         __i8+1
	lda         __i8
	ldy          #1
	sta         (__i3), Y
	.loc 13 127 1
	lda         __i2
	and          #63
	sta         __i8
	stz         __i8+1
	ora          #128
	sta         __i2
	lda         __i8+1
	sta         __i2+1
	lda         __i2
	ldy          #2
	sta         (__i3), Y
	.loc 13 128 1
	lda          #3
	sta         __i2
	lda          #0
	stz         __i2+1
	ldx          #11
	lda          #__i2
	jsr         __load_result_value2
	jmp         .wcrtomb_label_99
.wcrtomb_label_578:
	.loc 13 130 1
	lda         __i4
	sta         __i2
	lda         __i4+1
	sta         __i2+1
	stz         __i3
	stz         __i3+1
	lda         __i3
	ora          #240
	sta         __i8
	lda         __i3+1
	sta         __i8+1
	lda         __i8
	sta         __b0
	lda         __i7
	sta         __i3
	lda         __i7+1
	sta         __i3+1
	lda         __b0
	sta         (__i3)
	.loc 13 131 1
	lda         __i2+1
	sta         __i8
	lda          #0
	stz         __i8+1
	ldx          #4
.wcrtomb_label_624:
	lsr         __i8+1
	ror         __i8
	dex         
	bne         .wcrtomb_label_624
	lda         __i8
	and          #63
	sta         __i9
	stz         __i9+1
	ora          #128
	sta         __i8
	lda         __i9+1
	sta         __i8+1
	lda         __i8
	ldy          #1
	sta         (__i3), Y
	.loc 13 132 1
	lda         __i2+1
	lsr          A
	sta         __i8+1
	lda         __i2
	ror          A
	sta         __i8
	ldx          #5
.wcrtomb_label_668:
	lsr         __i8+1
	ror         __i8
	dex         
	bne         .wcrtomb_label_668
	lda         __i8
	and          #63
	sta         __i9
	stz         __i9+1
	ora          #128
	sta         __i8
	lda         __i9+1
	sta         __i8+1
	lda         __i8
	ldy          #2
	sta         (__i3), Y
	.loc 13 133 1
	lda         __i2
	and          #63
	sta         __i8
	stz         __i8+1
	ora          #128
	sta         __i2
	lda         __i8+1
	sta         __i2+1
	lda         __i2
	ldy          #3
	sta         (__i3), Y
	.loc 13 134 1
	lda          #4
	sta         __i2
	lda          #0
	stz         __i2+1
	ldx          #11
	lda          #__i2
	jsr         __load_result_value2
	jmp         .wcrtomb_label_99
.func_end_wcrtomb:
	.size wcrtomb, .func_end_wcrtomb-wcrtomb

	.section ".text.mbsrtowcs", "ax", @progbits
	.global mbsrtowcs
	.type mbsrtowcs, @function

mbsrtowcs:
	lda          #23
	jsr          __enter_res
	.byte        0x05,0x00,0x00		// Save mask i:5 b:0 l:0 x:0 f:0 
	ldx          #4
	jsr          __arg_value2_i4			// count
	ldx          #2
	jsr          __arg_value2_i5			// source
	ldx          #6
	jsr          __arg_value2_i6			// state
	.loc 13 139 30
	lda         (__i5)
	sta         __i0
	ldy          #1
	lda         (__i5), Y
	sta         __i0+1
	lda          #__i0
	ldx          #19
	jsr         __set_var_value2
	.loc 13 140 19
	stz         __i0
	stz         __i0+1
	lda          #__i0
	ldx          #5
	jsr         __set_var_value2
	.loc 13 142 26
	lda         __i6
	bne         .mbsrtowcs_label_94
	lda         __i6+1
	bne         .mbsrtowcs_label_94
	.loc 13 143 1
	ldx          #12
	lda          #__i8
	jsr          __var_addr_push_i8
	jsr         reset_state
	jsr         __incsp2
	.loc 13 144 1
	lda         __i8
	sta         __i6
	lda         __i8+1
	sta         __i6+1
.mbsrtowcs_label_94:
	.loc 13 146 26
.mbsrtowcs_label_96:
	ldx          #19
	jsr          __var_value2_i0			// current
	lda         (__i0)
	sta         __i0
	stz         __i0+1
	bne         .mbsrtowcs_label_452
	jmp         .mbsrtowcs_label_377
.mbsrtowcs_label_452:
	.loc 13 148 39
	ldx          #19
	jsr          __var_value2_i8			// current
	jsr         __pushi8
	ldx         #__i0
	ldy          #0
	jsr         strlen
	clc         
	lda         __i0
	adc          #1
	sta         __i1
	lda         __i0+1
	adc          #0
	sta         __i1+1
	lda         __i1
	sta         __i7
	lda         __i1+1
	sta         __i7+1
	.loc 13 149 61
	jsr         __pushi6
	jsr         __pushi1
	jsr         __pushi8
	ldx          #16
	lda          #__i1
	jsr          __var_addr_push_i1
	ldx         #__i1
	ldy          #0
	jsr         mbrtowc
	lda          #__i1
	ldx          #14
	jsr         __set_var_value2
	.loc 13 150 55
	ldx          #1
	lda         __i1
	cmp          #255
	bne         .mbsrtowcs_label_170
	lda         __i1+1
	cmp          #255
	beq         .mbsrtowcs_label_171
.mbsrtowcs_label_170:
	dex         
.mbsrtowcs_label_171:
	stx         __b0
	ldx          #17
	jsr          __var_addr_i0			// __invented__64
	lda         __b0
	sta         (__i0)
	lda         __i1
	cmp          #255
	bne         .mbsrtowcs_label_190
	lda         __i1+1
	cmp          #255
	beq         .mbsrtowcs_label_222
.mbsrtowcs_label_190:
	ldx          #14
	jsr          __var_value2_i1			// consumed
	ldx          #1
	lda         __i1
	cmp          #254
	bne         .mbsrtowcs_label_203
	lda         __i1+1
	cmp          #255
	beq         .mbsrtowcs_label_204
.mbsrtowcs_label_203:
	dex         
.mbsrtowcs_label_204:
	txa         
	sta         (__i0)
.mbsrtowcs_label_222:
	lda         (__i0)
	beq         .mbsrtowcs_label_269
	.loc 13 151 27
	ldx          #0
	jsr          __arg_value2_i1			// output
	lda         __i1
	bne         .mbsrtowcs_label_239
	lda         __i1+1
	beq         .mbsrtowcs_label_257
.mbsrtowcs_label_239:
	.loc 13 151 27
	ldx          #19
	jsr          __var_value2_i1			// current
	lda         __i1
	sta         (__i5)
	lda         __i1+1
	ldy          #1
	sta         (__i5), Y
.mbsrtowcs_label_257:
	.loc 13 152 1
	lda          #255
	sta         __i1
	sta         __i1+1
	ldx          #24
	lda          #__i1
	jsr         __load_result_value2
.mbsrtowcs_label_266:
	ldy          #34
	jmp          __leave
.mbsrtowcs_label_269:
	.loc 13 154 27
	ldx          #0
	jsr          __arg_value2_i1			// output
	lda         __i1
	bne         .mbsrtowcs_label_274
	lda         __i1+1
	beq         .mbsrtowcs_label_347
.mbsrtowcs_label_274:
	.loc 13 155 23
	ldx          #5
	jsr          __var_value2_i1			// written
	lda         __i1
	cmp         __i4
	bne         .mbsrtowcs_label_310
	lda         __i1+1
	cmp         __i4+1
	bne         .mbsrtowcs_label_310
	.loc 13 156 1
	ldx          #19
	jsr          __var_value2_i1			// current
	lda         __i1
	sta         (__i5)
	lda         __i1+1
	ldy          #1
	sta         (__i5), Y
	.loc 13 157 1
	ldx          #5
	jsr          __var_value2_i1			// written
	ldx          #24
	lda          #__i1
	jsr         __load_result_value2
	bra         .mbsrtowcs_label_266
.mbsrtowcs_label_310:
	.loc 13 159 1
	ldx          #16
	jsr          __var_value2_i1			// value
	ldx          #0
	jsr          __arg_value2_i2			// output
	ldx          #5
	jsr          __var_value2_i3			// written
	lda         __i3
	asl          A
	sta         __i8
	lda         __i3+1
	rol          A
	sta         __i8+1
	clc         
	lda         __i2
	adc         __i8
	sta         __i3
	lda         __i2+1
	adc         __i8+1
	sta         __i3+1
	lda         __i1
	sta         (__i3)
	lda         __i1+1
	ldy          #1
	sta         (__i3), Y
.mbsrtowcs_label_347:
	.loc 13 161 1
	ldx          #5
	jsr          __var_addr_i1			// written
	lda          #__i1
	jsr         __inc21
	.loc 13 162 1
	ldx          #14
	jsr          __var_value2_i1			// consumed
	ldx          #19
	jsr          __var_value2_i2			// current
	clc         
	lda         __i2
	adc         __i1
	sta         __i3
	lda         __i2+1
	adc         __i1+1
	sta         __i3+1
	lda          #__i3
	ldx          #19
	jsr         __set_var_value2
	jmp         .mbsrtowcs_label_96
.mbsrtowcs_label_377:
	.loc 13 164 27
	ldx          #0
	jsr          __arg_value2_i0			// output
	lda         __i0
	bne         .mbsrtowcs_label_382
	lda         __i0+1
	beq         .mbsrtowcs_label_445
.mbsrtowcs_label_382:
	.loc 13 165 22
	ldx          #5
	jsr          __var_value2_i0			// written
	lda         __i0+1
	cmp         __i4+1
	bcc         .mbsrtowcs_label_394
	bne         .mbsrtowcs_label_437
	lda         __i0
	cmp         __i4
	bcs         .mbsrtowcs_label_437
.mbsrtowcs_label_394:
	.loc 13 165 22
	ldx          #0
	jsr          __arg_value2_i0			// output
	ldx          #5
	jsr          __var_value2_i1			// written
	lda         __i1
	asl          A
	sta         __i2
	lda         __i1+1
	rol          A
	sta         __i2+1
	clc         
	lda         __i0
	adc         __i2
	sta         __i1
	lda         __i0+1
	adc         __i2+1
	sta         __i1+1
	ldy          #1
	lda          #0
.mbsrtowcs_label_432:
	sta         (__i1), Y
	dey         
	bpl         .mbsrtowcs_label_432
.mbsrtowcs_label_437:
	.loc 13 166 1
	ldy          #1
	lda          #0
.mbsrtowcs_label_441:
	sta         (__i5), Y
	dey         
	bpl         .mbsrtowcs_label_441
.mbsrtowcs_label_445:
	.loc 13 168 1
	ldx          #5
	jsr          __var_value2_i0			// written
	ldx          #24
	lda          #__i0
	jsr         __load_result_value2
	jmp         .mbsrtowcs_label_266
.func_end_mbsrtowcs:
	.size mbsrtowcs, .func_end_mbsrtowcs-mbsrtowcs

	.section ".text.wcsrtombs", "ax", @progbits
	.global wcsrtombs
	.type wcsrtombs, @function

wcsrtombs:
	lda          #16
	jsr          __enter_res
	.byte        0x05,0x00,0x00		// Save mask i:5 b:0 l:0 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i4			// source
	ldx          #6
	jsr          __arg_value2_i7			// state
	.loc 13 173 33
	lda         (__i4)
	sta         __i0
	ldy          #1
	lda         (__i4), Y
	sta         __i0+1
	lda          #__i0
	ldx          #12
	jsr         __set_var_value2
	.loc 13 174 19
	stz         __i5
	stz         __i5+1
	.loc 13 176 27
.wcsrtombs_label_54:
	ldx          #12
	jsr          __var_value2_i0			// current
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	lda         __i1
	ora         __i1+1
	bne         .wcsrtombs_label_422
	jmp         .wcsrtombs_label_365
.wcsrtombs_label_422:
	.loc 13 177 48
	ldx          #12
	jsr          __var_value2_i0			// current
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	jsr         __pushi7
	jsr         __pushi1
	ldx          #9
	lda          #__i0
	jsr          __var_addr_push_i0
	ldx         #__i8
	ldy          #0
	jsr         wcrtomb
	lda         __i8
	sta         __i6
	lda         __i8+1
	sta         __i6+1
	.loc 13 179 27
	lda         __i8
	cmp          #255
	bne         .wcsrtombs_label_146
	lda         __i8+1
	cmp          #255
	bne         .wcsrtombs_label_146
	.loc 13 180 27
	ldx          #0
	jsr          __arg_value2_i0			// output
	lda         __i0
	bne         .wcsrtombs_label_116
	lda         __i0+1
	beq         .wcsrtombs_label_134
.wcsrtombs_label_116:
	.loc 13 180 27
	ldx          #12
	jsr          __var_value2_i0			// current
	lda         __i0
	sta         (__i4)
	lda         __i0+1
	ldy          #1
	sta         (__i4), Y
.wcsrtombs_label_134:
	.loc 13 181 1
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #17
	lda          #__i0
	jsr         __load_result_value2
.wcsrtombs_label_143:
	ldy          #27
	jmp          __leave
.wcsrtombs_label_146:
	.loc 13 183 55
	ldx          #0
	jsr          __arg_value2_i0			// output
	ldx          #0
	lda         __i0
	bne         .wcsrtombs_label_154
	lda         __i0+1
	beq         .wcsrtombs_label_153
.wcsrtombs_label_154:
	inx         
.wcsrtombs_label_153:
	stx         __b0
	ldx          #10
	jsr          __var_addr_i1			// __invented__72
	lda         __b0
	sta         (__i1)
	lda         __i0
	bne         .wcsrtombs_label_173
	lda         __i0+1
	beq         .wcsrtombs_label_216
.wcsrtombs_label_173:
	clc         
	lda         __i5
	adc         __i6
	sta         __i0
	lda         __i5+1
	adc         __i6+1
	sta         __i0+1
	ldx          #4
	jsr          __arg_value2_i2			// count
	ldx          #1
	lda         __i2+1
	cmp         __i0+1
	bcc         .wcsrtombs_label_197
	bne         .wcsrtombs_label_196
	lda         __i2
	cmp         __i0
	bcc         .wcsrtombs_label_197
.wcsrtombs_label_196:
	dex         
.wcsrtombs_label_197:
	txa         
	sta         (__i1)
.wcsrtombs_label_216:
	lda         (__i1)
	beq         .wcsrtombs_label_242
	.loc 13 184 1
	ldx          #12
	jsr          __var_value2_i0			// current
	lda         __i0
	sta         (__i4)
	lda         __i0+1
	ldy          #1
	sta         (__i4), Y
	.loc 13 185 1
	ldx          #17
	lda          #__i5
	jsr         __load_result_value2
	bra         .wcsrtombs_label_143
.wcsrtombs_label_242:
	.loc 13 187 27
	ldx          #0
	jsr          __arg_value2_i0			// output
	lda         __i0
	bne         .wcsrtombs_label_247
	lda         __i0+1
	beq         .wcsrtombs_label_338
.wcsrtombs_label_247:
	.loc 13 188 30
	stz         __i0
	stz         __i0+1
	lda          #__i0
	ldx          #5
	jsr         __set_var_value2
.wcsrtombs_label_265:
	ldx          #5
	jsr          __var_value2_i0			// i
	lda         __i0+1
	cmp         __i6+1
	bcc         .wcsrtombs_label_269
	bne         .wcsrtombs_label_337
	lda         __i0
	cmp         __i6
	bcs         .wcsrtombs_label_337
.wcsrtombs_label_269:
	.loc 13 188 30
	ldx          #5
	jsr          __var_value2_i0			// i
	ldx          #9
	jsr          __var_addr_i2			// bytes
	clc         
	lda         __i2
	adc         __i0
	sta         __i3
	lda         __i2+1
	adc         __i0+1
	sta         __i3+1
	lda         (__i3)
	sta         __b0
	ldx          #0
	jsr          __arg_value2_i2			// output
	clc         
	lda         __i5
	adc         __i0
	sta         __i3
	lda         __i5+1
	adc         __i0+1
	sta         __i3+1
	clc         
	lda         __i2
	adc         __i3
	sta         __i0
	lda         __i2+1
	adc         __i3+1
	sta         __i0+1
	lda         __b0
	sta         (__i0)
	ldx          #5
	jsr          __var_addr_i0			// i
	lda          #__i0
	jsr         __inc21
	bra         .wcsrtombs_label_265
.wcsrtombs_label_337:
.wcsrtombs_label_338:
	.loc 13 190 1
	clc         
	lda         __i5
	adc         __i6
	sta         __i0
	lda         __i5+1
	adc         __i6+1
	sta         __i0+1
	lda         __i0
	sta         __i5
	lda         __i0+1
	sta         __i5+1
	.loc 13 191 1
	ldx          #12
	jsr          __var_addr_i0			// current
	lda          #__i0
	ldx          #2
	jsr         __inc2
	jmp         .wcsrtombs_label_54
.wcsrtombs_label_365:
	.loc 13 193 27
	ldx          #0
	jsr          __arg_value2_i0			// output
	lda         __i0
	bne         .wcsrtombs_label_370
	lda         __i0+1
	beq         .wcsrtombs_label_418
.wcsrtombs_label_370:
	.loc 13 194 22
	ldx          #4
	jsr          __arg_value2_i0			// count
	lda         __i5+1
	cmp         __i0+1
	bcc         .wcsrtombs_label_382
	bne         .wcsrtombs_label_409
	lda         __i5
	cmp         __i0
	bcs         .wcsrtombs_label_409
.wcsrtombs_label_382:
	.loc 13 194 22
	ldx          #0
	jsr          __arg_value2_i0			// output
	clc         
	lda         __i0
	adc         __i5
	sta         __i1
	lda         __i0+1
	adc         __i5+1
	sta         __i1+1
	lda          #0
	tay         
	sta         (__i1)
.wcsrtombs_label_409:
	.loc 13 195 1
	ldy          #1
	lda          #0
.wcsrtombs_label_413:
	sta         (__i4), Y
	dey         
	bpl         .wcsrtombs_label_413
.wcsrtombs_label_418:
	.loc 13 197 1
	ldx          #17
	lda          #__i5
	jsr         __load_result_value2
	jmp         .wcsrtombs_label_143
.func_end_wcsrtombs:
	.size wcsrtombs, .func_end_wcsrtombs-wcsrtombs

	.section ".text.mblen", "ax", @progbits
	.global mblen
	.type mblen, @function

mblen:
	lda          #10
	jsr          __enter_res
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i4			// string
	ldx          #2
	jsr          __arg_value2_i5			// count
	.loc 13 202 27
	lda         __i4
	bne         .mblen_label_55
	lda         __i4+1
	bne         .mblen_label_55
	.loc 13 203 1
	ldx         #%lo(legacy_input_state)
	ldy         #%hi(legacy_input_state)
	jsr         __pushxy
	jsr         reset_state
	jsr         __incsp2
	.loc 13 204 1
	stz         __i0
	stz         __i0+1
	ldx          #11
	lda          #__i0
	jsr         __load_result_value2
.mblen_label_52:
	ldy          #17
	jmp          __leave
.mblen_label_55:
	.loc 13 206 1
	ldx         #%lo(legacy_input_state)
	ldy         #%hi(legacy_input_state)
	jsr         __pushxy
	jsr         __pushi5
	jsr         __pushi4
	ldx         #__i7
	ldy          #0
	jsr         mbrlen
	lda         __i7
	sta         __i6
	lda         __i7+1
	sta         __i6+1
	.loc 13 207 1
	ldx          #1
	lda         __i7
	cmp          #255
	bne         .mblen_label_76
	lda         __i7+1
	cmp          #255
	beq         .mblen_label_77
.mblen_label_76:
	dex         
.mblen_label_77:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i0			// __invented__77
	lda         __b0
	sta         (__i0)
	lda         __i7
	cmp          #255
	bne         .mblen_label_96
	lda         __i7+1
	cmp          #255
	beq         .mblen_label_125
.mblen_label_96:
	ldx          #1
	lda         __i6
	cmp          #254
	bne         .mblen_label_106
	lda         __i6+1
	cmp          #255
	beq         .mblen_label_107
.mblen_label_106:
	dex         
.mblen_label_107:
	txa         
	sta         (__i0)
.mblen_label_125:
	lda         (__i0)
	beq         .mblen_label_149
	ldx          #6
	jsr          __var_addr_i1			// __invented__78
	lda          #255
	ldy          #0
	sta         (__i1)
	iny         
	sta         (__i1), Y
	bra         .mblen_label_159
.mblen_label_149:
	ldx          #6
	jsr          __var_addr_i1			// __invented__78
	lda         __i6
	sta         (__i1)
	lda         __i6+1
	ldy          #1
	sta         (__i1), Y
.mblen_label_159:
	ldx          #6
	jsr          __var_addr_i1			// __invented__78
	lda         (__i1)
	sta         __i2
	ldy          #1
	lda         (__i1), Y
	sta         __i2+1
	ldx          #11
	lda          #__i2
	jsr         __load_result_value2
	jmp         .mblen_label_52
.func_end_mblen:
	.size mblen, .func_end_mblen-mblen

	.section ".text.mbtowc", "ax", @progbits
	.global mbtowc
	.type mbtowc, @function

mbtowc:
	lda          #10
	jsr          __enter_res
	.byte        0x05,0x00,0x00		// Save mask i:5 b:0 l:0 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i4			// string
	ldx          #4
	jsr          __arg_value2_i5			// count
	ldx          #0
	jsr          __arg_value2_i6			// output
	.loc 13 213 27
	lda         __i4
	bne         .mbtowc_label_59
	lda         __i4+1
	bne         .mbtowc_label_59
	.loc 13 214 1
	ldx         #%lo(legacy_input_state)
	ldy         #%hi(legacy_input_state)
	jsr         __pushxy
	jsr         reset_state
	jsr         __incsp2
	.loc 13 215 1
	stz         __i0
	stz         __i0+1
	ldx          #11
	lda          #__i0
	jsr         __load_result_value2
.mbtowc_label_56:
	ldy          #19
	jmp          __leave
.mbtowc_label_59:
	.loc 13 217 1
	ldx         #%lo(legacy_input_state)
	ldy         #%hi(legacy_input_state)
	jsr         __pushxy
	jsr         __pushi5
	jsr         __pushi4
	jsr         __pushi6
	ldx         #__i8
	ldy          #0
	jsr         mbrtowc
	lda         __i8
	sta         __i7
	lda         __i8+1
	sta         __i7+1
	.loc 13 218 1
	ldx          #1
	lda         __i8
	cmp          #255
	bne         .mbtowc_label_81
	lda         __i8+1
	cmp          #255
	beq         .mbtowc_label_82
.mbtowc_label_81:
	dex         
.mbtowc_label_82:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i0			// __invented__83
	lda         __b0
	sta         (__i0)
	lda         __i8
	cmp          #255
	bne         .mbtowc_label_101
	lda         __i8+1
	cmp          #255
	beq         .mbtowc_label_130
.mbtowc_label_101:
	ldx          #1
	lda         __i7
	cmp          #254
	bne         .mbtowc_label_111
	lda         __i7+1
	cmp          #255
	beq         .mbtowc_label_112
.mbtowc_label_111:
	dex         
.mbtowc_label_112:
	txa         
	sta         (__i0)
.mbtowc_label_130:
	lda         (__i0)
	beq         .mbtowc_label_154
	ldx          #6
	jsr          __var_addr_i1			// __invented__84
	lda          #255
	ldy          #0
	sta         (__i1)
	iny         
	sta         (__i1), Y
	bra         .mbtowc_label_164
.mbtowc_label_154:
	ldx          #6
	jsr          __var_addr_i1			// __invented__84
	lda         __i7
	sta         (__i1)
	lda         __i7+1
	ldy          #1
	sta         (__i1), Y
.mbtowc_label_164:
	ldx          #6
	jsr          __var_addr_i1			// __invented__84
	lda         (__i1)
	sta         __i2
	ldy          #1
	lda         (__i1), Y
	sta         __i2+1
	ldx          #11
	lda          #__i2
	jsr         __load_result_value2
	jmp         .mbtowc_label_56
.func_end_mbtowc:
	.size mbtowc, .func_end_mbtowc-mbtowc

	.section ".text.wctomb", "ax", @progbits
	.global wctomb
	.type wctomb, @function

wctomb:
	lda          #9
	jsr          __enter_res
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i4			// output
	ldx          #2
	jsr          __arg_value2_i5			// value
	.loc 13 223 27
	lda         __i4
	bne         .wctomb_label_52
	lda         __i4+1
	bne         .wctomb_label_52
	.loc 13 224 1
	ldx         #%lo(legacy_output_state)
	ldy         #%hi(legacy_output_state)
	jsr         __pushxy
	jsr         reset_state
	jsr         __incsp2
	.loc 13 225 1
	stz         __i0
	stz         __i0+1
	ldx          #10
	lda          #__i0
	jsr         __load_result_value2
.wctomb_label_49:
	ldy          #16
	jmp          __leave
.wctomb_label_52:
	.loc 13 227 1
	ldx         #%lo(legacy_output_state)
	ldy         #%hi(legacy_output_state)
	jsr         __pushxy
	jsr         __pushi5
	jsr         __pushi4
	ldx         #__i7
	ldy          #0
	jsr         wcrtomb
	lda         __i7
	sta         __i6
	lda         __i7+1
	sta         __i6+1
	.loc 13 228 1
	lda         __i7
	cmp          #255
	bne         .wctomb_label_90
	lda         __i7+1
	cmp          #255
	bne         .wctomb_label_90
	ldx          #5
	jsr          __var_addr_i0			// __invented__88
	lda          #255
	ldy          #0
	sta         (__i0)
	iny         
	sta         (__i0), Y
	bra         .wctomb_label_100
.wctomb_label_90:
	ldx          #5
	jsr          __var_addr_i0			// __invented__88
	lda         __i6
	sta         (__i0)
	lda         __i6+1
	ldy          #1
	sta         (__i0), Y
.wctomb_label_100:
	ldx          #5
	jsr          __var_addr_i0			// __invented__88
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	ldx          #10
	lda          #__i1
	jsr         __load_result_value2
	bra         .wctomb_label_49
.func_end_wctomb:
	.size wctomb, .func_end_wctomb-wctomb

	.section ".text.mbstowcs", "ax", @progbits
	.global mbstowcs
	.type mbstowcs, @function

mbstowcs:
	lda          #16
	jsr          __enter_res
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i0			// string
	ldx          #4
	jsr          __arg_value2_i4			// count
	ldx          #0
	jsr          __arg_value2_i5			// output
	.loc 13 233 28
	lda          #__i0
	ldx          #5
	jsr         __set_var_value2
	.loc 13 235 1
	ldx          #12
	lda          #__i6
	jsr          __var_addr_push_i6
	jsr         reset_state
	jsr         __incsp2
	.loc 13 236 1
	jsr         __pushi6
	jsr         __pushi4
	ldx          #5
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi5
	ldx         #__i0
	ldy          #0
	jsr         mbsrtowcs
	ldx          #17
	lda          #__i0
	jsr         __load_result_value2
	ldy          #25
	jmp          __leave
.func_end_mbstowcs:
	.size mbstowcs, .func_end_mbstowcs-mbstowcs

	.section ".text.wcstombs", "ax", @progbits
	.global wcstombs
	.type wcstombs, @function

wcstombs:
	lda          #16
	jsr          __enter_res
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i0			// string
	ldx          #4
	jsr          __arg_value2_i4			// count
	ldx          #0
	jsr          __arg_value2_i5			// output
	.loc 13 241 31
	lda          #__i0
	ldx          #5
	jsr         __set_var_value2
	.loc 13 243 1
	ldx          #12
	lda          #__i6
	jsr          __var_addr_push_i6
	jsr         reset_state
	jsr         __incsp2
	.loc 13 244 1
	jsr         __pushi6
	jsr         __pushi4
	ldx          #5
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi5
	ldx         #__i0
	ldy          #0
	jsr         wcsrtombs
	ldx          #17
	lda          #__i0
	jsr         __load_result_value2
	ldy          #25
	jmp          __leave
.func_end_wcstombs:
	.size wcstombs, .func_end_wcstombs-wcstombs

	.section ".text.wcslen", "ax", @progbits
	.global wcslen
	.type wcslen, @function

wcslen:
	lda          #5
	jsr          __enter_leaf_res_nomask
	ldx          #0
	jsr          __arg_value2_i1			// string
	.loc 13 248 28
	lda         __i1
	sta         __i0
	lda         __i1+1
	sta         __i0+1
	.loc 13 249 23
.wcslen_label_22:
	lda         (__i0)
	sta         __i2
	ldy          #1
	lda         (__i0), Y
	sta         __i2+1
	lda         __i2
	ora         __i2+1
	beq         .wcslen_label_42
	.loc 13 249 23
	lda          #__i0
	ldx          #2
	jsr         __rinc2
	bra         .wcslen_label_22
.wcslen_label_42:
	.loc 13 250 1
	sec         
	lda         __i0
	sbc         __i1
	sta         __i2
	lda         __i0+1
	sbc         __i1+1
	sta         __i2+1
	lsr          A
	sta         __i3+1
	lda         __i2
	ror          A
	sta         __i3
	lda         #__i3
	jsr         __result2
	ldy          #10
	jmp          __leave_leaf_nomask
.func_end_wcslen:
	.size wcslen, .func_end_wcslen-wcslen

	.section ".text.wcscpy", "ax", @progbits
	.global wcscpy
	.type wcscpy, @function

wcscpy:
	lda          #5
	jsr          __enter_leaf_res
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i1			// destination
	ldx          #2
	jsr          __arg_value2_i2			// source
	.loc 13 255 30
	lda         __i1
	sta         __i0
	lda         __i1+1
	sta         __i0+1
	.loc 13 256 47
.wcscpy_label_24:
	lda         __i2
	sta         __i3
	lda         __i2+1
	sta         __i3+1
	lda          #__i2
	ldx          #2
	jsr         __rinc2
	lda         (__i3)
	sta         __i4
	ldy          #1
	lda         (__i3), Y
	sta         __i4+1
	lda         __i1
	sta         __i3
	lda         __i1+1
	sta         __i3+1
	lda          #__i1
	ldx          #2
	jsr         __rinc2
	lda         __i4
	sta         (__i3)
	lda         __i4+1
	ldy          #1
	sta         (__i3), Y
	lda         __i4
	ora         __i4+1
	beq         .wcscpy_label_69
	bra         .wcscpy_label_24
.wcscpy_label_69:
	.loc 13 257 1
	lda         #__i0
	jsr         __result2
	ldy          #12
	jmp          __leave_leaf
.func_end_wcscpy:
	.size wcscpy, .func_end_wcscpy-wcscpy

	.section ".text.wcsncpy", "ax", @progbits
	.global wcsncpy
	.type wcsncpy, @function

wcsncpy:
	lda          #6
	jsr          __enter_leaf_res
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i1			// destination
	ldx          #2
	jsr          __arg_value2_i2			// source
	.loc 13 262 30
	lda         __i1
	sta         __i0
	lda         __i1+1
	sta         __i0+1
	.loc 13 263 40
.wcsncpy_label_28:
	ldx          #4
	jsr          __arg_value2_i3			// count
	ldx          #1
	lda         __i3
	ora         __i3+1
	bne         .wcsncpy_label_35
	dex         
.wcsncpy_label_35:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i4			// __invented__90
	lda         __b0
	sta         (__i4)
	lda         __i3
	ora         __i3+1
	beq         .wcsncpy_label_83
	lda         (__i2)
	sta         __i3
	ldy          #1
	lda         (__i2), Y
	sta         __i3+1
	ldx          #1
	lda         __i3
	ora         __i3+1
	bne         .wcsncpy_label_67
	dex         
.wcsncpy_label_67:
	txa         
	sta         (__i4)
.wcsncpy_label_83:
	lda         (__i4)
	beq         .wcsncpy_label_143
	.loc 13 264 1
	lda         __i2
	sta         __i3
	lda         __i2+1
	sta         __i3+1
	lda          #__i2
	ldx          #2
	jsr         __rinc2
	lda         (__i3)
	sta         __i5
	ldy          #1
	lda         (__i3), Y
	sta         __i5+1
	lda         __i1
	sta         __i3
	lda         __i1+1
	sta         __i3+1
	lda          #__i1
	ldx          #2
	jsr         __rinc2
	lda         __i5
	sta         (__i3)
	lda         __i5+1
	ldy          #1
	sta         (__i3), Y
	.loc 13 265 1
	ldx          #4
	jsr          __arg_addr_i3			// count
	lda          #__i3
	jsr         __dec21
	jmp         .wcsncpy_label_28
.wcsncpy_label_143:
	.loc 13 267 22
.wcsncpy_label_145:
	ldx          #4
	jsr          __arg_value2_i3			// count
	ldx          #4
	jsr          __arg_addr_i5			// count
	lda          #__i5
	jsr         __dec21
	lda         __i3
	ora         __i3+1
	beq         .wcsncpy_label_178
	.loc 13 267 22
	lda         __i1
	sta         __i3
	lda         __i1+1
	sta         __i3+1
	lda          #__i1
	ldx          #2
	jsr         __rinc2
	ldy          #1
	lda          #0
.wcsncpy_label_172:
	sta         (__i3), Y
	dey         
	bpl         .wcsncpy_label_172
	bra         .wcsncpy_label_145
.wcsncpy_label_178:
	.loc 13 268 1
	lda         #__i0
	jsr         __result2
	ldy          #15
	jmp          __leave_leaf
.func_end_wcsncpy:
	.size wcsncpy, .func_end_wcsncpy-wcsncpy

	.section ".text.wcscat", "ax", @progbits
	.global wcscat
	.type wcscat, @function

wcscat:
	lda          #11
	jsr          __enter_res
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i0			// source
	ldx          #0
	jsr          __arg_value2_i1			// destination
	.loc 13 273 1
	ldx          #5
	jsr          __var_addr_i4			// __invented__91
	lda         __i0
	sta         (__i4)
	lda         __i0+1
	ldy          #1
	sta         (__i4), Y
	lda         __i1
	sta         __i5
	lda         __i1+1
	sta         __i5+1
	ldx          #7
	jsr          __var_addr_i6			// __invented__92
	lda         __i5
	sta         (__i6)
	lda         __i5+1
	ldy          #1
	sta         (__i6), Y
	jsr         __pushi5
	ldx         #__i2
	ldy          #0
	jsr         wcslen
	lda         __i2
	asl          A
	sta         __i0
	lda         __i2+1
	rol          A
	sta         __i0+1
	lda         (__i6)
	sta         __i1
	ldy          #1
	lda         (__i6), Y
	sta         __i1+1
	clc         
	lda         __i1
	adc         __i0
	sta         __i2
	lda         __i1+1
	adc         __i0+1
	sta         __i2+1
	lda         (__i4)
	sta         __i0
	lda         (__i4), Y
	sta         __i0+1
	jsr         __pushi0
	jsr         __pushi2
	ldx         #__i0
	ldy          #0
	jsr         wcscpy
	.loc 13 274 1
	ldx          #12
	lda          #__i5
	jsr         __load_result_value2
	ldy          #18
	jmp          __leave
.func_end_wcscat:
	.size wcscat, .func_end_wcscat-wcscat

	.section ".text.wcsncat", "ax", @progbits
	.global wcsncat
	.type wcsncat, @function

wcsncat:
	lda          #10
	jsr          __enter_res
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i5			// destination
	ldx          #2
	jsr          __arg_value2_i6			// source
	.loc 13 279 52
	lda         __i5
	sta         __i0
	lda         __i5+1
	sta         __i0+1
	ldx          #5
	jsr          __var_addr_i7			// __invented__93
	lda         __i0
	sta         (__i7)
	lda         __i0+1
	ldy          #1
	sta         (__i7), Y
	jsr         __pushi0
	ldx         #__i0
	ldy          #0
	jsr         wcslen
	lda         __i0
	asl          A
	sta         __i1
	lda         __i0+1
	rol          A
	sta         __i1+1
	lda         (__i7)
	sta         __i0
	ldy          #1
	lda         (__i7), Y
	sta         __i0+1
	clc         
	lda         __i0
	adc         __i1
	sta         __i2
	lda         __i0+1
	adc         __i1+1
	sta         __i2+1
	lda         __i2
	sta         __i4
	lda         __i2+1
	sta         __i4+1
	.loc 13 280 42
.wcsncat_label_90:
	ldx          #4
	jsr          __arg_value2_i0			// count
	ldx          #4
	jsr          __arg_addr_i1			// count
	lda          #__i1
	jsr         __dec21
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .wcsncat_label_103
	dex         
.wcsncat_label_103:
	stx         __b0
	ldx          #6
	jsr          __var_addr_i1			// __invented__94
	lda         __b0
	sta         (__i1)
	lda         __i0
	ora         __i0+1
	beq         .wcsncat_label_151
	lda         (__i6)
	sta         __i0
	ldy          #1
	lda         (__i6), Y
	sta         __i0+1
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .wcsncat_label_135
	dex         
.wcsncat_label_135:
	txa         
	sta         (__i1)
.wcsncat_label_151:
	lda         (__i1)
	beq         .wcsncat_label_204
	.loc 13 280 42
	lda         __i6
	sta         __i0
	lda         __i6+1
	sta         __i0+1
	lda          #__i6
	ldx          #2
	jsr         __rinc2
	lda         (__i0)
	sta         __i2
	ldy          #1
	lda         (__i0), Y
	sta         __i2+1
	lda         __i4
	sta         __i0
	lda         __i4+1
	sta         __i0+1
	lda          #__i4
	ldx          #2
	jsr         __rinc2
	lda         __i2
	sta         (__i0)
	lda         __i2+1
	ldy          #1
	sta         (__i0), Y
	jmp         .wcsncat_label_90
.wcsncat_label_204:
	.loc 13 281 1
	ldy          #1
	lda          #0
.wcsncat_label_208:
	sta         (__i4), Y
	dey         
	bpl         .wcsncat_label_208
	.loc 13 282 1
	ldx          #11
	lda          #__i5
	jsr         __load_result_value2
	ldy          #19
	jmp          __leave
.func_end_wcsncat:
	.size wcsncat, .func_end_wcsncat-wcsncat

	.section ".text.wcscmp", "ax", @progbits
	.global wcscmp
	.type wcscmp, @function

wcscmp:
	lda          #8
	jsr          __enter_leaf_res
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i0			// left
	ldx          #2
	jsr          __arg_value2_i1			// right
	.loc 13 286 43
.wcscmp_label_20:
	lda         (__i0)
	sta         __i2
	ldy          #1
	lda         (__i0), Y
	sta         __i2+1
	lda         (__i1)
	sta         __i3
	lda         (__i1), Y
	sta         __i3+1
	ldx          #1
	lda         __i2
	cmp         __i3
	bne         .wcscmp_label_41
	lda         __i2+1
	cmp         __i3+1
	beq         .wcscmp_label_42
.wcscmp_label_41:
	dex         
.wcscmp_label_42:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i4			// __invented__95
	lda         __b0
	sta         (__i4)
	lda         __i2
	cmp         __i3
	bne         .wcscmp_label_97
	lda         __i2+1
	cmp         __i3+1
	bne         .wcscmp_label_97
	lda         (__i0)
	sta         __i2
	ldy          #1
	lda         (__i0), Y
	sta         __i2+1
	ldx          #1
	lda         __i2
	ora         __i2+1
	bne         .wcscmp_label_81
	dex         
.wcscmp_label_81:
	txa         
	sta         (__i4)
.wcscmp_label_97:
	lda         (__i4)
	beq         .wcscmp_label_121
	.loc 13 287 1
	lda          #__i0
	ldx          #2
	jsr         __rinc2
	.loc 13 288 1
	lda          #__i1
	ldx          #2
	jsr         __rinc2
	bra         .wcscmp_label_20
.wcscmp_label_121:
	.loc 13 290 1
	lda         (__i0)
	sta         __i2
	ldy          #1
	lda         (__i0), Y
	sta         __i2+1
	lda         (__i1)
	sta         __i3
	lda         (__i1), Y
	sta         __i3+1
	lda         __i2
	cmp         __i3
	lda         __i2+1
	sbc         __i3+1
	bvc         .wcscmp_label_139
	eor          #128
.wcscmp_label_139:
	bpl         .wcscmp_label_159
	ldx          #6
	jsr          __var_addr_i2			// __invented__96
	lda          #255
	ldy          #0
	sta         (__i2)
	iny         
	sta         (__i2), Y
	bra         .wcscmp_label_207
.wcscmp_label_159:
	lda         (__i0)
	sta         __i2
	ldy          #1
	lda         (__i0), Y
	sta         __i2+1
	lda         (__i1)
	sta         __i3
	lda         (__i1), Y
	sta         __i3+1
	ldx          #0
	lda         __i2
	cmp         __i3
	bne         .wcscmp_label_179
	lda         __i2+1
	cmp         __i3+1
	beq         .wcscmp_label_178
.wcscmp_label_179:
	inx         
.wcscmp_label_178:
	txa         
	sta         __i2
	lda          #0
	stz         __i2+1
	ldx          #6
	jsr          __var_addr_i3			// __invented__96
	lda         __i2
	sta         (__i3)
	lda         __i2+1
	ldy          #1
	sta         (__i3), Y
.wcscmp_label_207:
	ldx          #6
	jsr          __var_addr_i2			// __invented__96
	lda         (__i2)
	sta         __i3
	ldy          #1
	lda         (__i2), Y
	sta         __i3+1
	lda         #__i3
	jsr         __result2
	ldy          #15
	jmp          __leave_leaf
.func_end_wcscmp:
	.size wcscmp, .func_end_wcscmp-wcscmp

	.section ".text.wcsncmp", "ax", @progbits
	.global wcsncmp
	.type wcsncmp, @function

wcsncmp:
	lda          #9
	jsr          __enter_leaf_res
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i0			// left
	ldx          #2
	jsr          __arg_value2_i1			// right
	.loc 13 294 57
.wcsncmp_label_23:
	ldx          #4
	jsr          __arg_value2_i2			// count
	ldx          #1
	lda         __i2
	ora         __i2+1
	bne         .wcsncmp_label_30
	dex         
.wcsncmp_label_30:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i3			// __invented__97
	lda         __b0
	sta         (__i3)
	lda         __i2
	ora         __i2+1
	beq         .wcsncmp_label_90
	lda         (__i0)
	sta         __i2
	ldy          #1
	lda         (__i0), Y
	sta         __i2+1
	lda         (__i1)
	sta         __i4
	lda         (__i1), Y
	sta         __i4+1
	ldx          #1
	lda         __i2
	cmp         __i4
	bne         .wcsncmp_label_71
	lda         __i2+1
	cmp         __i4+1
	beq         .wcsncmp_label_72
.wcsncmp_label_71:
	dex         
.wcsncmp_label_72:
	txa         
	sta         (__i3)
.wcsncmp_label_90:
	lda         (__i3)
	sta         __b0
	ldx          #5
	jsr          __var_addr_i2			// __invented__98
	lda         __b0
	sta         (__i2)
	beq         .wcsncmp_label_138
	lda         (__i0)
	sta         __i4
	ldy          #1
	lda         (__i0), Y
	sta         __i4+1
	ldx          #1
	lda         __i4
	ora         __i4+1
	bne         .wcsncmp_label_122
	dex         
.wcsncmp_label_122:
	txa         
	sta         (__i2)
.wcsncmp_label_138:
	lda         (__i2)
	beq         .wcsncmp_label_169
	.loc 13 295 1
	lda          #__i0
	ldx          #2
	jsr         __rinc2
	.loc 13 296 1
	lda          #__i1
	ldx          #2
	jsr         __rinc2
	.loc 13 297 1
	ldx          #4
	jsr          __arg_addr_i4			// count
	lda          #__i4
	jsr         __dec21
	jmp         .wcsncmp_label_23
.wcsncmp_label_169:
	.loc 13 299 17
	ldx          #4
	jsr          __arg_value2_i4			// count
	lda         __i4
	ora         __i4+1
	bne         .wcsncmp_label_188
	.loc 13 299 17
	stz         __i4
	stz         __i4+1
	lda         #__i4
	jsr         __result2
.wcsncmp_label_185:
	ldy          #18
	jmp          __leave_leaf
.wcsncmp_label_188:
	.loc 13 300 1
	lda         (__i0)
	sta         __i4
	ldy          #1
	lda         (__i0), Y
	sta         __i4+1
	lda         (__i1)
	sta         __i5
	lda         (__i1), Y
	sta         __i5+1
	lda         __i4
	cmp         __i5
	lda         __i4+1
	sbc         __i5+1
	bvc         .wcsncmp_label_206
	eor          #128
.wcsncmp_label_206:
	bpl         .wcsncmp_label_226
	ldx          #7
	jsr          __var_addr_i4			// __invented__99
	lda          #255
	ldy          #0
	sta         (__i4)
	iny         
	sta         (__i4), Y
	bra         .wcsncmp_label_274
.wcsncmp_label_226:
	lda         (__i0)
	sta         __i4
	ldy          #1
	lda         (__i0), Y
	sta         __i4+1
	lda         (__i1)
	sta         __i5
	lda         (__i1), Y
	sta         __i5+1
	ldx          #0
	lda         __i4
	cmp         __i5
	bne         .wcsncmp_label_246
	lda         __i4+1
	cmp         __i5+1
	beq         .wcsncmp_label_245
.wcsncmp_label_246:
	inx         
.wcsncmp_label_245:
	txa         
	sta         __i4
	lda          #0
	stz         __i4+1
	ldx          #7
	jsr          __var_addr_i5			// __invented__99
	lda         __i4
	sta         (__i5)
	lda         __i4+1
	ldy          #1
	sta         (__i5), Y
.wcsncmp_label_274:
	ldx          #7
	jsr          __var_addr_i4			// __invented__99
	lda         (__i4)
	sta         __i5
	ldy          #1
	lda         (__i4), Y
	sta         __i5+1
	lda         #__i5
	jsr         __result2
	jmp         .wcsncmp_label_185
.func_end_wcsncmp:
	.size wcsncmp, .func_end_wcsncmp-wcsncmp

	.section ".text.wmemcmp", "ax", @progbits
	.global wmemcmp
	.type wmemcmp, @function

wmemcmp:
	lda          #9
	jsr          __enter_leaf_res
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
	ldx          #4
	jsr          __arg_value2_i0			// count
	ldx          #0
	jsr          __arg_value2_i1			// left
	ldx          #2
	jsr          __arg_value2_i2			// right
	.loc 13 305 29
	stz         __i3
	stz         __i3+1
	lda          #__i3
	ldx          #5
	jsr         __set_var_value2
.wmemcmp_label_36:
	ldx          #5
	jsr          __var_value2_i3			// i
	lda         __i3+1
	cmp         __i0+1
	bcc         .wmemcmp_label_40
	beq         .wmemcmp_label_247
	jmp         .wmemcmp_label_238
.wmemcmp_label_247:
	lda         __i3
	cmp         __i0
	bcc         .wmemcmp_label_249
	jmp         .wmemcmp_label_238
.wmemcmp_label_249:
.wmemcmp_label_40:
	.loc 13 306 26
	ldx          #5
	jsr          __var_value2_i3			// i
	lda         __i3
	asl          A
	sta         __i4
	lda         __i3+1
	rol          A
	sta         __i4+1
	clc         
	lda         __i1
	adc         __i4
	sta         __i5
	lda         __i1+1
	adc         __i4+1
	sta         __i5+1
	lda         (__i5)
	sta         __i4
	ldy          #1
	lda         (__i5), Y
	sta         __i4+1
	lda         __i3
	asl          A
	sta         __i5
	lda         __i3+1
	rol          A
	sta         __i5+1
	clc         
	lda         __i2
	adc         __i5
	sta         __i3
	lda         __i2+1
	adc         __i5+1
	sta         __i3+1
	lda         (__i3)
	sta         __i5
	lda         (__i3), Y
	sta         __i5+1
	lda         __i4
	cmp         __i5
	bne         .wmemcmp_label_110
	lda         __i4+1
	cmp         __i5+1
	bne         .wmemcmp_label_251
	jmp         .wmemcmp_label_229
.wmemcmp_label_251:
.wmemcmp_label_110:
	.loc 13 306 26
	ldx          #5
	jsr          __var_value2_i3			// i
	lda         __i3
	asl          A
	sta         __i4
	lda         __i3+1
	rol          A
	sta         __i4+1
	clc         
	lda         __i1
	adc         __i4
	sta         __i5
	lda         __i1+1
	adc         __i4+1
	sta         __i5+1
	lda         (__i5)
	sta         __i4
	ldy          #1
	lda         (__i5), Y
	sta         __i4+1
	lda         __i3
	asl          A
	sta         __i5
	lda         __i3+1
	rol          A
	sta         __i5+1
	clc         
	lda         __i2
	adc         __i5
	sta         __i3
	lda         __i2+1
	adc         __i5+1
	sta         __i3+1
	lda         (__i3)
	sta         __i5
	lda         (__i3), Y
	sta         __i5+1
	lda         __i4
	cmp         __i5
	lda         __i4+1
	sbc         __i5+1
	bvc         .wmemcmp_label_179
	eor          #128
.wmemcmp_label_179:
	bpl         .wmemcmp_label_199
	ldx          #7
	jsr          __var_addr_i3			// __invented__100
	lda          #255
	ldy          #0
	sta         (__i3)
	iny         
	sta         (__i3), Y
	bra         .wmemcmp_label_210
.wmemcmp_label_199:
	ldx          #7
	jsr          __var_addr_i3			// __invented__100
	lda          #1
	ldy          #0
	sta         (__i3)
	dec          A
	iny         
	sta         (__i3), Y
.wmemcmp_label_210:
	ldx          #7
	jsr          __var_addr_i3			// __invented__100
	lda         (__i3)
	sta         __i4
	ldy          #1
	lda         (__i3), Y
	sta         __i4+1
	lda         #__i4
	jsr         __result2
.wmemcmp_label_226:
	ldy          #18
	jmp          __leave_leaf
.wmemcmp_label_229:
	ldx          #5
	jsr          __var_addr_i3			// i
	lda          #__i3
	jsr         __inc21
	jmp         .wmemcmp_label_36
.wmemcmp_label_238:
	.loc 13 308 1
	stz         __i3
	stz         __i3+1
	lda         #__i3
	jsr         __result2
	bra         .wmemcmp_label_226
.func_end_wmemcmp:
	.size wmemcmp, .func_end_wmemcmp-wmemcmp

	.section ".text.wmemcpy", "ax", @progbits
	.global wmemcpy
	.type wmemcpy, @function

wmemcpy:
	lda          #7
	jsr          __enter_leaf_res
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
	ldx          #4
	jsr          __arg_value2_i0			// count
	ldx          #2
	jsr          __arg_value2_i1			// source
	ldx          #0
	jsr          __arg_value2_i2			// destination
	.loc 13 314 29
	stz         __i3
	stz         __i3+1
	lda          #__i3
	ldx          #5
	jsr         __set_var_value2
.wmemcpy_label_33:
	ldx          #5
	jsr          __var_value2_i3			// i
	lda         __i3+1
	cmp         __i0+1
	bcc         .wmemcpy_label_37
	bne         .wmemcpy_label_113
	lda         __i3
	cmp         __i0
	bcs         .wmemcpy_label_113
.wmemcpy_label_37:
	.loc 13 314 29
	ldx          #5
	jsr          __var_value2_i3			// i
	lda         __i3
	asl          A
	sta         __i4
	lda         __i3+1
	rol          A
	sta         __i4+1
	clc         
	lda         __i1
	adc         __i4
	sta         __i5
	lda         __i1+1
	adc         __i4+1
	sta         __i5+1
	lda         (__i5)
	sta         __i4
	ldy          #1
	lda         (__i5), Y
	sta         __i4+1
	lda         __i3
	asl          A
	sta         __i5
	lda         __i3+1
	rol          A
	sta         __i5+1
	clc         
	lda         __i2
	adc         __i5
	sta         __i3
	lda         __i2+1
	adc         __i5+1
	sta         __i3+1
	lda         __i4
	sta         (__i3)
	lda         __i4+1
	sta         (__i3), Y
	ldx          #5
	jsr          __var_addr_i3			// i
	lda          #__i3
	jsr         __inc21
	bra         .wmemcpy_label_33
.wmemcpy_label_113:
	.loc 13 315 1
	lda         #__i2
	jsr         __result2
	ldy          #16
	jmp          __leave_leaf
.func_end_wmemcpy:
	.size wmemcpy, .func_end_wmemcpy-wmemcpy

	.section ".text.wmemmove", "ax", @progbits
	.global wmemmove
	.type wmemmove, @function

wmemmove:
	lda          #7
	jsr          __enter_res
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i0			// destination
	ldx          #2
	jsr          __arg_value2_i1			// source
	.loc 13 319 27
	lda         __i0+1
	cmp         __i1+1
	bcc         .wmemmove_label_18
	bne         .wmemmove_label_41
	lda         __i0
	cmp         __i1
	bcs         .wmemmove_label_41
.wmemmove_label_18:
	.loc 13 320 1
	ldx          #4
	jsr          __arg_value2_i2			// count
	ldy          #10
	jsr          __leave
	ldx         __result
	ldy         __result+1
	jmp         wmemcpy
.wmemmove_label_38:
	ldy          #16
	jmp          __leave
.wmemmove_label_41:
	.loc 13 322 20
.wmemmove_label_43:
	ldx          #4
	jsr          __arg_value2_i2			// count
	lda         __i2
	ora         __i2+1
	beq         .wmemmove_label_118
	.loc 13 323 1
	ldx          #4
	jsr          __arg_addr_i2			// count
	lda          #__i2
	jsr         __dec21
	.loc 13 324 1
	ldx          #4
	jsr          __arg_value2_i2			// count
	lda         __i2
	asl          A
	sta         __i3
	lda         __i2+1
	rol          A
	sta         __i3+1
	clc         
	lda         __i1
	adc         __i3
	sta         __i4
	lda         __i1+1
	adc         __i3+1
	sta         __i4+1
	lda         (__i4)
	sta         __i3
	ldy          #1
	lda         (__i4), Y
	sta         __i3+1
	lda         __i2
	asl          A
	sta         __i4
	lda         __i2+1
	rol          A
	sta         __i4+1
	clc         
	lda         __i0
	adc         __i4
	sta         __i2
	lda         __i0+1
	adc         __i4+1
	sta         __i2+1
	lda         __i3
	sta         (__i2)
	lda         __i3+1
	sta         (__i2), Y
	bra         .wmemmove_label_43
.wmemmove_label_118:
	.loc 13 326 1
	ldx          #8
	lda          #__i0
	jsr         __load_result_value2
	bra         .wmemmove_label_38
.func_end_wmemmove:
	.size wmemmove, .func_end_wmemmove-wmemmove

	.section ".text.wmemset", "ax", @progbits
	.global wmemset
	.type wmemset, @function

wmemset:
	lda          #7
	jsr          __enter_leaf_res
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
	ldx          #4
	jsr          __arg_value2_i0			// count
	ldx          #2
	jsr          __arg_value2_i1			// value
	ldx          #0
	jsr          __arg_value2_i2			// destination
	.loc 13 331 29
	stz         __i3
	stz         __i3+1
	lda          #__i3
	ldx          #5
	jsr         __set_var_value2
.wmemset_label_33:
	ldx          #5
	jsr          __var_value2_i3			// i
	lda         __i3+1
	cmp         __i0+1
	bcc         .wmemset_label_37
	bne         .wmemset_label_83
	lda         __i3
	cmp         __i0
	bcs         .wmemset_label_83
.wmemset_label_37:
	.loc 13 331 29
	ldx          #5
	jsr          __var_value2_i3			// i
	lda         __i3
	asl          A
	sta         __i4
	lda         __i3+1
	rol          A
	sta         __i4+1
	clc         
	lda         __i2
	adc         __i4
	sta         __i3
	lda         __i2+1
	adc         __i4+1
	sta         __i3+1
	lda         __i1
	sta         (__i3)
	lda         __i1+1
	ldy          #1
	sta         (__i3), Y
	ldx          #5
	jsr          __var_addr_i3			// i
	lda          #__i3
	jsr         __inc21
	bra         .wmemset_label_33
.wmemset_label_83:
	.loc 13 332 1
	lda         #__i2
	jsr         __result2
	ldy          #16
	jmp          __leave_leaf
.func_end_wmemset:
	.size wmemset, .func_end_wmemset-wmemset

	.section ".text.wmemchr", "ax", @progbits
	.global wmemchr
	.type wmemchr, @function

wmemchr:
	lda          #7
	jsr          __enter_leaf_res
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
	ldx          #4
	jsr          __arg_value2_i0			// count
	ldx          #0
	jsr          __arg_value2_i1			// memory
	ldx          #2
	jsr          __arg_value2_i2			// value
	.loc 13 337 29
	stz         __i3
	stz         __i3+1
	lda          #__i3
	ldx          #5
	jsr         __set_var_value2
.wmemchr_label_33:
	ldx          #5
	jsr          __var_value2_i3			// i
	lda         __i3+1
	cmp         __i0+1
	bcc         .wmemchr_label_37
	bne         .wmemchr_label_124
	lda         __i3
	cmp         __i0
	bcs         .wmemchr_label_124
.wmemchr_label_37:
	.loc 13 338 25
	ldx          #5
	jsr          __var_value2_i3			// i
	lda         __i3
	asl          A
	sta         __i4
	lda         __i3+1
	rol          A
	sta         __i4+1
	clc         
	lda         __i1
	adc         __i4
	sta         __i3
	lda         __i1+1
	adc         __i4+1
	sta         __i3+1
	lda         (__i3)
	sta         __i4
	ldy          #1
	lda         (__i3), Y
	sta         __i4+1
	lda         __i4
	cmp         __i2
	bne         .wmemchr_label_115
	lda         __i4+1
	cmp         __i2+1
	bne         .wmemchr_label_115
	.loc 13 338 25
	ldx          #5
	jsr          __var_value2_i3			// i
	lda         __i3
	asl          A
	sta         __i4
	lda         __i3+1
	rol          A
	sta         __i4+1
	clc         
	lda         __i1
	adc         __i4
	sta         __i3
	lda         __i1+1
	adc         __i4+1
	sta         __i3+1
	lda         #__i3
	jsr         __result2
.wmemchr_label_112:
	ldy          #16
	jmp          __leave_leaf
.wmemchr_label_115:
	ldx          #5
	jsr          __var_addr_i3			// i
	lda          #__i3
	jsr         __inc21
	bra         .wmemchr_label_33
.wmemchr_label_124:
	.loc 13 340 1
	stz         __i3
	stz         __i3+1
	lda         #__i3
	jsr         __result2
	bra         .wmemchr_label_112
.func_end_wmemchr:
	.size wmemchr, .func_end_wmemchr-wmemchr

	.section ".text.wcschr", "ax", @progbits
	.global wcschr
	.type wcschr, @function

wcschr:
	lda          #5
	jsr          __enter_leaf_res_nomask
	ldx          #0
	jsr          __arg_value2_i0			// string
	ldx          #2
	jsr          __arg_value2_i1			// value
	.loc 13 346 3
.wcschr_label_14:
	.loc 13 345 23
	lda         (__i0)
	sta         __i2
	ldy          #1
	lda         (__i0), Y
	sta         __i2+1
	lda         __i2
	cmp         __i1
	bne         .wcschr_label_41
	lda         __i2+1
	cmp         __i1+1
	bne         .wcschr_label_41
	.loc 13 345 23
	lda         #__i0
	jsr         __result2
.wcschr_label_38:
	ldy          #12
	jmp          __leave_leaf_nomask
.wcschr_label_41:
	lda         __i0
	sta         __i2
	lda         __i0+1
	sta         __i2+1
	lda          #__i0
	ldx          #2
	jsr         __rinc2
	lda         (__i2)
	sta         __i3
	ldy          #1
	lda         (__i2), Y
	sta         __i3+1
	lda         __i3
	ora         __i3+1
	bne         .wcschr_label_14
	.loc 13 347 1
	stz         __i2
	stz         __i2+1
	lda         #__i2
	jsr         __result2
	bra         .wcschr_label_38
.func_end_wcschr:
	.size wcschr, .func_end_wcschr-wcschr

	.section ".text.wcsrchr", "ax", @progbits
	.global wcsrchr
	.type wcsrchr, @function

wcsrchr:
	lda          #5
	jsr          __enter_leaf_res
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i1			// string
	ldx          #2
	jsr          __arg_value2_i2			// value
	.loc 13 351 35
	stz         __i0
	stz         __i0+1
	.loc 13 354 3
.wcsrchr_label_22:
	.loc 13 353 23
	lda         (__i1)
	sta         __i3
	ldy          #1
	lda         (__i1), Y
	sta         __i3+1
	lda         __i3
	cmp         __i2
	bne         .wcsrchr_label_45
	lda         __i3+1
	cmp         __i2+1
	bne         .wcsrchr_label_45
	.loc 13 353 23
	lda         __i1
	sta         __i0
	lda         __i1+1
	sta         __i0+1
.wcsrchr_label_45:
	lda         __i1
	sta         __i3
	lda         __i1+1
	sta         __i3+1
	lda          #__i1
	ldx          #2
	jsr         __rinc2
	lda         (__i3)
	sta         __i4
	ldy          #1
	lda         (__i3), Y
	sta         __i4+1
	lda         __i4
	ora         __i4+1
	bne         .wcsrchr_label_22
	.loc 13 355 1
	lda         #__i0
	jsr         __result2
	ldy          #12
	jmp          __leave_leaf
.func_end_wcsrchr:
	.size wcsrchr, .func_end_wcsrchr-wcsrchr

	.section ".text.wcsspn", "ax", @progbits
	.global wcsspn
	.type wcsspn, @function

wcsspn:
	lda          #10
	jsr          __enter_res
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i4			// string
	ldx          #2
	jsr          __arg_value2_i5			// accept
	.loc 13 359 17
	stz         __i0
	stz         __i0+1
	lda          #__i0
	ldx          #5
	jsr         __set_var_value2
	.loc 13 361 46
.wcsspn_label_33:
	ldx          #5
	jsr          __var_value2_i0			// count
	lda         __i0
	asl          A
	sta         __i1
	lda         __i0+1
	rol          A
	sta         __i1+1
	clc         
	lda         __i4
	adc         __i1
	sta         __i0
	lda         __i4+1
	adc         __i1+1
	sta         __i0+1
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	ldx          #1
	lda         __i1
	ora         __i1+1
	bne         .wcsspn_label_68
	dex         
.wcsspn_label_68:
	stx         __b0
	ldx          #6
	jsr          __var_addr_i6			// __invented__108
	lda         __b0
	sta         (__i6)
	lda         __i1
	ora         __i1+1
	beq         .wcsspn_label_149
	ldx          #5
	jsr          __var_value2_i0			// count
	lda         __i0
	asl          A
	sta         __i1
	lda         __i0+1
	rol          A
	sta         __i1+1
	clc         
	lda         __i4
	adc         __i1
	sta         __i0
	lda         __i4+1
	adc         __i1+1
	sta         __i0+1
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	jsr         __pushi1
	jsr         __pushi5
	ldx         #__i0
	ldy          #0
	jsr         wcschr
	ldx          #0
	lda         __i0
	bne         .wcsspn_label_131
	lda         __i0+1
	beq         .wcsspn_label_130
.wcsspn_label_131:
	inx         
.wcsspn_label_130:
	txa         
	sta         (__i6)
.wcsspn_label_149:
	lda         (__i6)
	beq         .wcsspn_label_170
	.loc 13 361 46
	ldx          #5
	jsr          __var_addr_i0			// count
	lda          #__i0
	jsr         __inc21
	jmp         .wcsspn_label_33
.wcsspn_label_170:
	.loc 13 362 1
	ldx          #5
	jsr          __var_value2_i0			// count
	ldx          #11
	lda          #__i0
	jsr         __load_result_value2
	ldy          #17
	jmp          __leave
.func_end_wcsspn:
	.size wcsspn, .func_end_wcsspn-wcsspn

	.section ".text.wcscspn", "ax", @progbits
	.global wcscspn
	.type wcscspn, @function

wcscspn:
	lda          #10
	jsr          __enter_res
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i4			// string
	ldx          #2
	jsr          __arg_value2_i5			// reject
	.loc 13 366 17
	stz         __i0
	stz         __i0+1
	lda          #__i0
	ldx          #5
	jsr         __set_var_value2
	.loc 13 368 46
.wcscspn_label_33:
	ldx          #5
	jsr          __var_value2_i0			// count
	lda         __i0
	asl          A
	sta         __i1
	lda         __i0+1
	rol          A
	sta         __i1+1
	clc         
	lda         __i4
	adc         __i1
	sta         __i0
	lda         __i4+1
	adc         __i1+1
	sta         __i0+1
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	ldx          #1
	lda         __i1
	ora         __i1+1
	bne         .wcscspn_label_68
	dex         
.wcscspn_label_68:
	stx         __b0
	ldx          #6
	jsr          __var_addr_i6			// __invented__110
	lda         __b0
	sta         (__i6)
	lda         __i1
	ora         __i1+1
	beq         .wcscspn_label_149
	ldx          #5
	jsr          __var_value2_i0			// count
	lda         __i0
	asl          A
	sta         __i1
	lda         __i0+1
	rol          A
	sta         __i1+1
	clc         
	lda         __i4
	adc         __i1
	sta         __i0
	lda         __i4+1
	adc         __i1+1
	sta         __i0+1
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	jsr         __pushi1
	jsr         __pushi5
	ldx         #__i0
	ldy          #0
	jsr         wcschr
	ldx          #1
	lda         __i0
	bne         .wcscspn_label_130
	lda         __i0+1
	beq         .wcscspn_label_131
.wcscspn_label_130:
	dex         
.wcscspn_label_131:
	txa         
	sta         (__i6)
.wcscspn_label_149:
	lda         (__i6)
	beq         .wcscspn_label_170
	.loc 13 368 46
	ldx          #5
	jsr          __var_addr_i0			// count
	lda          #__i0
	jsr         __inc21
	jmp         .wcscspn_label_33
.wcscspn_label_170:
	.loc 13 369 1
	ldx          #5
	jsr          __var_value2_i0			// count
	ldx          #11
	lda          #__i0
	jsr         __load_result_value2
	ldy          #17
	jmp          __leave
.func_end_wcscspn:
	.size wcscspn, .func_end_wcscspn-wcscspn

	.section ".text.wcspbrk", "ax", @progbits
	.global wcspbrk
	.type wcspbrk, @function

wcspbrk:
	lda          #7
	jsr          __enter_res
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i4			// string
	ldx          #2
	jsr          __arg_value2_i5			// accept
	.loc 13 373 26
.wcspbrk_label_16:
	lda         (__i4)
	sta         __i0
	ldy          #1
	lda         (__i4), Y
	sta         __i0+1
	lda         __i0
	ora         __i0+1
	beq         .wcspbrk_label_69
	.loc 13 374 44
	lda         (__i4)
	sta         __i0
	ldy          #1
	lda         (__i4), Y
	sta         __i0+1
	jsr         __pushi0
	jsr         __pushi5
	ldx         #__i0
	ldy          #0
	jsr         wcschr
	lda         __i0
	bne         .wcspbrk_label_48
	lda         __i0+1
	beq         .wcspbrk_label_61
.wcspbrk_label_48:
	.loc 13 374 44
	ldx          #8
	lda          #__i4
	jsr         __load_result_value2
.wcspbrk_label_58:
	ldy          #14
	jmp          __leave
.wcspbrk_label_61:
	.loc 13 375 1
	lda          #__i4
	ldx          #2
	jsr         __rinc2
	bra         .wcspbrk_label_16
.wcspbrk_label_69:
	.loc 13 377 1
	stz         __i0
	stz         __i0+1
	ldx          #8
	lda          #__i0
	jsr         __load_result_value2
	bra         .wcspbrk_label_58
.func_end_wcspbrk:
	.size wcspbrk, .func_end_wcspbrk-wcspbrk

	.section ".text.wcsstr", "ax", @progbits
	.global wcsstr
	.type wcsstr, @function

wcsstr:
	lda          #7
	jsr          __enter_res
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i5			// substring
	ldx          #0
	jsr          __arg_value2_i6			// string
	.loc 13 381 34
	jsr         __pushi5
	ldx         #__i7
	ldy          #0
	jsr         wcslen
	lda         __i7
	sta         __i4
	lda         __i7+1
	sta         __i4+1
	.loc 13 382 18
	lda         __i7
	ora         __i7+1
	bne         .wcsstr_label_44
	.loc 13 382 18
	ldx          #8
	lda          #__i6
	jsr         __load_result_value2
.wcsstr_label_41:
	ldy          #14
	jmp          __leave
.wcsstr_label_44:
	.loc 13 383 26
.wcsstr_label_46:
	lda         (__i6)
	sta         __i0
	ldy          #1
	lda         (__i6), Y
	sta         __i0+1
	lda         __i0
	ora         __i0+1
	beq         .wcsstr_label_82
	.loc 13 384 46
	jsr         __pushi4
	jsr         __pushi5
	jsr         __pushi6
	ldx         #__i0
	ldy          #0
	jsr         wcsncmp
	lda         __i0
	ora         __i0+1
	bne         .wcsstr_label_74
	.loc 13 384 46
	ldx          #8
	lda          #__i6
	jsr         __load_result_value2
	bra         .wcsstr_label_41
.wcsstr_label_74:
	.loc 13 385 1
	lda          #__i6
	ldx          #2
	jsr         __rinc2
	bra         .wcsstr_label_46
.wcsstr_label_82:
	.loc 13 387 1
	stz         __i0
	stz         __i0+1
	ldx          #8
	lda          #__i0
	jsr         __load_result_value2
	bra         .wcsstr_label_41
.func_end_wcsstr:
	.size wcsstr, .func_end_wcsstr-wcsstr

	.section ".text.wcstok", "ax", @progbits
	.global wcstok
	.type wcstok, @function

wcstok:
	lda          #7
	jsr          __enter_res
	.byte        0x05,0x00,0x00		// Save mask i:5 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i4			// string
	ldx          #4
	jsr          __arg_value2_i5			// state
	ldx          #2
	jsr          __arg_value2_i6			// delimiters
	.loc 13 393 27
	lda         __i4
	bne         .wcstok_label_49
	lda         __i4+1
	bne         .wcstok_label_49
	.loc 13 393 27
	lda         (__i5)
	sta         __i0
	ldy          #1
	lda         (__i5), Y
	sta         __i0+1
	lda         __i0
	sta         __i4
	lda         __i0+1
	sta         __i4+1
.wcstok_label_49:
	.loc 13 394 1
	lda         __i4
	sta         __i8
	lda         __i4+1
	sta         __i8+1
	jsr         __pushi6
	jsr         __pushi8
	ldx         #__i0
	ldy          #0
	jsr         wcsspn
	lda         __i0
	asl          A
	sta         __i1
	lda         __i0+1
	rol          A
	sta         __i1+1
	clc         
	lda         __i8
	adc         __i1
	sta         __i0
	lda         __i8+1
	adc         __i1+1
	sta         __i0+1
	lda         __i0
	sta         __i4
	lda         __i0+1
	sta         __i4+1
	.loc 13 395 23
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	lda         __i1
	ora         __i1+1
	bne         .wcstok_label_120
	.loc 13 396 1
	lda         __i4
	sta         (__i5)
	lda         __i4+1
	ldy          #1
	sta         (__i5), Y
	.loc 13 397 1
	stz         __i0
	stz         __i0+1
	ldx          #8
	lda          #__i0
	jsr         __load_result_value2
.wcstok_label_117:
	ldy          #16
	jmp          __leave
.wcstok_label_120:
	.loc 13 399 1
	lda         __i4
	sta         __i8
	lda         __i4+1
	sta         __i8+1
	lda         __i8
	sta         __i7
	lda         __i8+1
	sta         __i7+1
	.loc 13 400 1
	jsr         __pushi6
	jsr         __pushi8
	ldx         #__i0
	ldy          #0
	jsr         wcscspn
	lda         __i0
	asl          A
	sta         __i1
	lda         __i0+1
	rol          A
	sta         __i1+1
	clc         
	lda         __i8
	adc         __i1
	sta         __i0
	lda         __i8+1
	adc         __i1+1
	sta         __i0+1
	lda         __i0
	sta         __i4
	lda         __i0+1
	sta         __i4+1
	.loc 13 401 23
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	lda         __i1
	ora         __i1+1
	beq         .wcstok_label_205
	.loc 13 401 23
	lda         __i4
	sta         __i0
	lda         __i4+1
	sta         __i0+1
	lda          #__i4
	ldx          #2
	jsr         __rinc2
	ldy          #1
	lda          #0
.wcstok_label_200:
	sta         (__i0), Y
	dey         
	bpl         .wcstok_label_200
.wcstok_label_205:
	.loc 13 402 1
	lda         __i4
	sta         (__i5)
	lda         __i4+1
	ldy          #1
	sta         (__i5), Y
	.loc 13 403 1
	ldx          #8
	lda          #__i7
	jsr         __load_result_value2
	bra         .wcstok_label_117
.func_end_wcstok:
	.size wcstok, .func_end_wcstok-wcstok

	.section ".text.wcscoll", "ax", @progbits
	.global wcscoll
	.type wcscoll, @function

wcscoll:
	lda          #7
	jsr          __enter_res_nomask
	ldx          #2
	jsr          __arg_value2_i0			// right
	ldx          #0
	jsr          __arg_value2_i1			// left
	.loc 13 407 1
	ldy          #10
	jsr          __leave_nomask
	ldx         __result
	ldy         __result+1
	jmp         wcscmp
.func_end_wcscoll:
	.size wcscoll, .func_end_wcscoll-wcscoll

	.section ".text.wcsxfrm", "ax", @progbits
	.global wcsxfrm
	.type wcsxfrm, @function

wcsxfrm:
	lda          #12
	jsr          __enter_res
	.byte        0x07,0x00,0x00		// Save mask i:7 b:0 l:0 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i5			// source
	ldx          #0
	jsr          __arg_value2_i6			// destination
	ldx          #4
	jsr          __arg_value2_i7			// count
	.loc 13 412 31
	jsr         __pushi5
	ldx         #__i8
	ldy          #0
	jsr         wcslen
	lda         __i8
	sta         __i4
	lda         __i8+1
	sta         __i4+1
	.loc 13 413 46
	ldx          #0
	lda         __i6
	bne         .wcsxfrm_label_46
	lda         __i6+1
	beq         .wcsxfrm_label_45
.wcsxfrm_label_46:
	inx         
.wcsxfrm_label_45:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i8			// __invented__120
	lda         __b0
	sta         (__i8)
	lda         __i6
	bne         .wcsxfrm_label_64
	lda         __i6+1
	beq         .wcsxfrm_label_91
.wcsxfrm_label_64:
	ldx          #1
	lda         __i7
	ora         __i7+1
	bne         .wcsxfrm_label_75
	dex         
.wcsxfrm_label_75:
	txa         
	sta         (__i8)
.wcsxfrm_label_91:
	lda         (__i8)
	bne         .wcsxfrm_label_228
	jmp         .wcsxfrm_label_222
.wcsxfrm_label_228:
	.loc 13 414 56
	sec         
	lda         __i7
	sbc          #1
	sta         __i0
	lda         __i7+1
	sbc          #0
	sta         __i0+1
	lda         __i4+1
	cmp         __i0+1
	bcc         .wcsxfrm_label_115
	bne         .wcsxfrm_label_134
	lda         __i4
	cmp         __i0
	bcs         .wcsxfrm_label_134
.wcsxfrm_label_115:
	ldx          #8
	jsr          __var_addr_i0			// __invented__121
	lda         __i4
	sta         (__i0)
	lda         __i4+1
	ldy          #1
	sta         (__i0), Y
	bra         .wcsxfrm_label_155
.wcsxfrm_label_134:
	sec         
	lda         __i7
	sbc          #1
	sta         __i0
	lda         __i7+1
	sbc          #0
	sta         __i0+1
	ldx          #8
	jsr          __var_addr_i1			// __invented__121
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
.wcsxfrm_label_155:
	ldx          #8
	jsr          __var_addr_i0			// __invented__121
	lda         (__i0)
	sta         __i9
	ldy          #1
	lda         (__i0), Y
	sta         __i9+1
	lda          #__i9
	ldx          #6
	jsr         __set_var_value2
	.loc 13 415 1
	lda         __i6
	sta         __i10
	lda         __i6+1
	sta         __i10+1
	jsr         __pushi9
	jsr         __pushi5
	jsr         __pushi10
	ldx         #__i0
	ldy          #0
	jsr         wmemcpy
	.loc 13 416 1
	lda         __i9
	asl          A
	sta         __i0
	lda         __i9+1
	rol          A
	sta         __i0+1
	clc         
	lda         __i10
	adc         __i0
	sta         __i1
	lda         __i10+1
	adc         __i0+1
	sta         __i1+1
	ldy          #1
	lda          #0
.wcsxfrm_label_217:
	sta         (__i1), Y
	dey         
	bpl         .wcsxfrm_label_217
.wcsxfrm_label_222:
	.loc 13 418 1
	ldx          #13
	lda          #__i4
	jsr         __load_result_value2
	ldy          #21
	jmp          __leave
.func_end_wcsxfrm:
	.size wcsxfrm, .func_end_wcsxfrm-wcsxfrm

	.section ".text.fputwc", "ax", @progbits
	.global fputwc
	.type fputwc, @function

fputwc:
	lda          #14
	jsr          __enter_res
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i5			// value
	ldx          #2
	jsr          __arg_value2_i6			// stream
	.loc 13 423 50
	stz         __i0
	stz         __i0+1
	jsr         __pushi0
	jsr         __pushi5
	ldx          #7
	lda          #__i0
	jsr          __var_addr_push_i0
	ldx         #__i7
	ldy          #0
	jsr         wcrtomb
	lda         __i7
	sta         __i4
	lda         __i7+1
	sta         __i4+1
	.loc 13 425 51
	ldx          #1
	jsr         __pushxy0
	jsr         __pushi6
	ldx         #__i0
	ldy          #0
	jsr         fwide
	ldx          #0
	lda         __i0
	cmp          #0
	lda         __i0+1
	sbc          #0
	bvc         .fputwc_label_67
	eor          #128
.fputwc_label_67:
	bpl         .fputwc_label_65
	inx         
.fputwc_label_65:
	stx         __b0
	ldx          #8
	jsr          __var_addr_i7			// __invented__128
	lda         __b0
	sta         (__i7)
	lda         __i0
	cmp          #0
	lda         __i0+1
	sbc          #0
	bvc         .fputwc_label_87
	eor          #128
.fputwc_label_87:
	bmi         .fputwc_label_116
	ldx          #1
	lda         __i4
	cmp          #255
	bne         .fputwc_label_97
	lda         __i4+1
	cmp          #255
	beq         .fputwc_label_98
.fputwc_label_97:
	dex         
.fputwc_label_98:
	txa         
	sta         (__i7)
.fputwc_label_116:
	lda         (__i7)
	beq         .fputwc_label_140
	.loc 13 425 51
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #15
	lda          #__i0
	jsr         __load_result_value2
.fputwc_label_137:
	ldy          #21
	jmp          __leave
.fputwc_label_140:
	.loc 13 426 30
	stz         __i0
	stz         __i0+1
	lda          #__i0
	ldx          #10
	jsr         __set_var_value2
.fputwc_label_152:
	ldx          #10
	jsr          __var_value2_i0			// i
	lda         __i0+1
	cmp         __i4+1
	bcc         .fputwc_label_156
	bne         .fputwc_label_229
	lda         __i0
	cmp         __i4
	bcs         .fputwc_label_229
.fputwc_label_156:
	.loc 13 427 53
	ldx          #10
	jsr          __var_value2_i0			// i
	ldx          #7
	jsr          __var_addr_i1			// bytes
	clc         
	lda         __i1
	adc         __i0
	sta         __i2
	lda         __i1+1
	adc         __i0+1
	sta         __i2+1
	lda         (__i2)
	sta         __i0
	lda          #0
	stz         __i0+1
	jsr         __pushi6
	jsr         __pushi0
	ldx         #__i0
	ldy          #0
	jsr         fputc
	lda         __i0
	cmp          #255
	bne         .fputwc_label_220
	lda         __i0+1
	cmp          #255
	bne         .fputwc_label_220
	.loc 13 427 53
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #15
	lda          #__i0
	jsr         __load_result_value2
	bra         .fputwc_label_137
.fputwc_label_220:
	ldx          #10
	jsr          __var_addr_i0			// i
	lda          #__i0
	jsr         __inc21
	bra         .fputwc_label_152
.fputwc_label_229:
	.loc 13 429 1
	ldx          #15
	lda          #__i5
	jsr         __load_result_value2
	jmp         .fputwc_label_137
.func_end_fputwc:
	.size fputwc, .func_end_fputwc-fputwc

	.section ".text.putwc", "ax", @progbits
	.global putwc
	.type putwc, @function

putwc:
	lda          #7
	jsr          __enter_res_nomask
	ldx          #2
	jsr          __arg_value2_i0			// stream
	ldx          #0
	jsr          __arg_value2_i1			// value
	.loc 13 432 45
	ldy          #10
	jsr          __leave_nomask
	ldx         __result
	ldy         __result+1
	jmp         fputwc
.func_end_putwc:
	.size putwc, .func_end_putwc-putwc

	.section ".text.putwchar", "ax", @progbits
	.global putwchar
	.type putwchar, @function

putwchar:
	lda          #7
	jsr          __enter_res_nomask
	ldx          #0
	jsr          __arg_value2_i0			// value
	.loc 13 433 34
	lda         stdout+0
	sta         __i1
	lda         stdout+1
	sta         __i1+1
	jsr         __pushi1
	jsr         __pushi0
	ldx         #__i1
	ldy          #0
	jsr         fputwc
	ldx          #8
	lda          #__i1
	jsr         __load_result_value2
	ldy          #12
	jmp          __leave_nomask
.func_end_putwchar:
	.size putwchar, .func_end_putwchar-putwchar

	.section ".text.fputws", "ax", @progbits
	.global fputws
	.type fputws, @function

fputws:
	lda          #7
	jsr          __enter_res
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i4			// string
	ldx          #2
	jsr          __arg_value2_i5			// stream
	.loc 13 436 26
.fputws_label_17:
	lda         (__i4)
	sta         __i0
	ldy          #1
	lda         (__i4), Y
	sta         __i0+1
	lda         __i0
	ora         __i0+1
	beq         .fputws_label_83
	.loc 13 437 48
	lda         __i4
	sta         __i0
	lda         __i4+1
	sta         __i0+1
	lda          #__i4
	ldx          #2
	jsr         __rinc2
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	jsr         __pushi5
	jsr         __pushi1
	ldx         #__i0
	ldy          #0
	jsr         fputwc
	lda         __i0
	cmp          #255
	bne         .fputws_label_81
	lda         __i0+1
	cmp          #255
	bne         .fputws_label_81
	.loc 13 437 48
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #8
	lda          #__i0
	jsr         __load_result_value2
.fputws_label_78:
	ldy          #14
	jmp          __leave
.fputws_label_81:
	bra         .fputws_label_17
.fputws_label_83:
	.loc 13 439 1
	stz         __i0
	stz         __i0+1
	ldx          #8
	lda          #__i0
	jsr         __load_result_value2
	bra         .fputws_label_78
.func_end_fputws:
	.size fputws, .func_end_fputws-fputws

	.section ".text.fgetwc", "ax", @progbits
	.global fgetwc
	.type fgetwc, @function

fgetwc:
	lda          #19
	jsr          __enter_res
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i4			// stream
	.loc 13 447 27
	ldx          #1
	jsr         __pushxy0
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         fwide
	lda         __i0
	cmp          #0
	lda         __i0+1
	sbc          #0
	bvc         .fgetwc_label_43
	eor          #128
.fgetwc_label_43:
	bpl         .fgetwc_label_63
	.loc 13 447 27
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #20
	lda          #__i0
	jsr         __load_result_value2
.fgetwc_label_60:
	ldy          #24
	jmp          __leave
.fgetwc_label_63:
	.loc 13 448 1
	ldx          #10
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         reset_state
	jsr         __incsp2
	.loc 13 454 3
.fgetwc_label_74:
	.loc 13 450 26
	jsr         __pushi4
	ldx         #__i7
	ldy          #0
	jsr         fgetc
	lda         __i7
	sta         __i5
	lda         __i7+1
	sta         __i5+1
	.loc 13 451 20
	lda         __i7
	cmp          #255
	bne         .fgetwc_label_106
	lda         __i7+1
	cmp          #255
	bne         .fgetwc_label_106
	.loc 13 451 20
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #20
	lda          #__i0
	jsr         __load_result_value2
	bra         .fgetwc_label_60
.fgetwc_label_106:
	.loc 13 452 1
	lda         __i5+1
	and          #128
	ora         __i5
