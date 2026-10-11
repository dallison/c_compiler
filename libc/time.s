	.file   "libc/time.c"
	.file 1 "libc/include/errno.h"
	.file 2 "libc/include/syscall.h"
	.file 3 "libc/include/davecc_guest_syscalls.h"
	.file 4 "libc/include/stdint.h"
	.file 5 "libc/include/limits.h"
	.file 6 "libc/include/stdio.h"
	.file 7 "libc/include/stdarg.h"
	.file 8 "libc/include/string.h"
	.file 9 "libc/include/time.h"
	.file 10 "libc/time.c"
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
	.section ".text.DaysFromCivil", "ax", @progbits
	.local  DaysFromCivil
	.type DaysFromCivil, @function

DaysFromCivil:
	lda          #23
	jsr          __enter_leaf_res
	.byte        0x01,0x60,0x00		// Save mask i:1 b:0 l:0 x:3 f:0 
	ldx          #10
	jsr          __arg_value2_i3			// day
	.loc 11 36 1
	ldx          #8
	jsr          __arg_value2_i4			// month
	ldx          #1
	lda          #0
	cmp         __i4+1
	bcc         .DaysFromCivil_label_52
	bne         .DaysFromCivil_label_53
	lda          #2
	cmp         __i4
	bcs         .DaysFromCivil_label_53
.DaysFromCivil_label_52:
	dex         
.DaysFromCivil_label_53:
	txa         
	sta         __i4
	stz         __i4+1
	sta         __x1
	lda         __i4+1
	sta         __x1+1
	and          #128
	beq         .DaysFromCivil_label_82
	lda          #255
.DaysFromCivil_label_82:
	ldy          #7
.DaysFromCivil_label_88:
	sta         __x1, Y
	dey         
	cpy          #1
	bne         .DaysFromCivil_label_88
	ldx          #0
	jsr          __arg_value8_x2			// year
	sec         
	ldy          #8
	ldx          #0
.DaysFromCivil_label_104:
	lda         __x2, X
	sbc         __x1, X
	sta         __x3, X
	inx         
	dey         
	bne         .DaysFromCivil_label_104
	lda          #__x3
	ldx          #0
	jsr         __set_arg_value8
	.loc 11 37 52
	lda         __x3
	cmp          #0
	lda         __x3+1
	sbc          #0
	lda         __x3+2
	sbc          #0
	lda         __x3+3
	sbc          #0
	lda         __x3+4
	sbc          #0
	lda         __x3+5
	sbc          #0
	lda         __x3+6
	sbc          #0
	lda         __x3+7
	sbc          #0
	bvc         .DaysFromCivil_label_119
	eor          #128
.DaysFromCivil_label_119:
	bmi         .DaysFromCivil_label_154
	ldx          #0
	jsr          __arg_value8_x0			// year
	ldx          #19
	jsr          __var_addr_i1			// __invented__10
	ldx          #__x0
	ldy          #__i1
	jsr          __store_indirect8
	bra         .DaysFromCivil_label_178
.DaysFromCivil_label_154:
	ldx          #0
	jsr          __arg_value8_x0			// year
	sec         
	ldy          #8
	ldx          #0
.DaysFromCivil_label_164:
	lda         __x0, X
	sbc         .lit.5, X
	sta         __x1, X
	inx         
	dey         
	bne         .DaysFromCivil_label_164
	ldx          #19
	jsr          __var_addr_i1			// __invented__10
	ldx          #__x1
	ldy          #__i1
	jsr          __store_indirect8
.DaysFromCivil_label_178:
	ldx          #19
	jsr          __var_addr_i1			// __invented__10
	ldx          #__i1
	ldy          #__x0
	jsr          __load_indirect8
	ldx          #7
.DaysFromCivil_label_192:
	lda         .lit.6, X
	sta         __x1, X
	dex         
	bpl         .DaysFromCivil_label_192
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __sdiv8
	lda          #__x2
	ldx          #11
	jsr         __set_var_value8
	.loc 11 38 52
	ldx          #0
	jsr          __arg_value8_x0			// year
	ldx          #7
.DaysFromCivil_label_222:
	lda         .lit.6, X
	sta         __x1, X
	dex         
	bpl         .DaysFromCivil_label_222
	lda          #__x3
	ldx          #__x2
	ldy          #__x1
	jsr         __smul8
	sec         
	ldy          #8
	ldx          #0
.DaysFromCivil_label_243:
	lda         __x0, X
	sbc         __x3, X
	sta         __x1, X
	inx         
	dey         
	bne         .DaysFromCivil_label_243
	lda         __x1
	sta         __i1
	lda         __x1+7
	and          #128
	ora         __x1+1
	sta         __i1+1
	lda         __i1
	sta         __i0
	lda         __i1+1
	sta         __i0+1
	.loc 11 39 55
	ldx          #8
	jsr          __arg_value2_i1			// month
	ldx          #8
	jsr          __arg_value2_i2			// month
	lda          #0
	cmp         __i2+1
	bcc         .DaysFromCivil_label_271
	bne         .DaysFromCivil_label_291
	lda          #2
	cmp         __i2
	bcs         .DaysFromCivil_label_291
.DaysFromCivil_label_271:
	ldx          #21
	jsr          __var_addr_i2			// __invented__11
	lda          #253
	ldy          #0
	sta         (__i2)
	lda          #255
	iny         
	sta         (__i2), Y
	bra         .DaysFromCivil_label_302
.DaysFromCivil_label_291:
	ldx          #21
	jsr          __var_addr_i2			// __invented__11
	lda          #9
	ldy          #0
	sta         (__i2)
	tya         
	iny         
	sta         (__i2), Y
.DaysFromCivil_label_302:
	ldx          #21
	jsr          __var_addr_i2			// __invented__11
	lda         (__i2)
	sta         __i4
	ldy          #1
	lda         (__i2), Y
	sta         __i4+1
	clc         
	lda         __i1
	adc         __i4
	sta         __i2
	lda         __i1+1
	adc         __i4+1
	sta         __i2+1
	.loc 11 41 62
	lda         __i2
	sta         __x0
	lda         __i2+1
	sta         __x0+1
	and          #128
	beq         .DaysFromCivil_label_334
	lda          #255
.DaysFromCivil_label_334:
	ldy          #7
.DaysFromCivil_label_338:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .DaysFromCivil_label_338
	ldx          #7
.DaysFromCivil_label_347:
	lda         .lit.10, X
	sta         __x1, X
	dex         
	bpl         .DaysFromCivil_label_347
	lda          #__x2
	ldx          #__x1
	ldy          #__x0
	jsr         __smul8
	clc         
	ldy          #8
	ldx          #0
.DaysFromCivil_label_368:
	lda         __x2, X
	adc         .lit.11, X
	sta         __x0, X
	inx         
	dey         
	bne         .DaysFromCivil_label_368
	ldx          #7
.DaysFromCivil_label_381:
	lda         .lit.12, X
	sta         __x1, X
	dex         
	bpl         .DaysFromCivil_label_381
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __sdiv8
	lda         __i3
	sta         __x0
	lda         __i3+1
	sta         __x0+1
	lda          #0
	ldy          #7
.DaysFromCivil_label_401:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .DaysFromCivil_label_401
	clc         
	ldy          #8
	ldx          #0
.DaysFromCivil_label_413:
	lda         __x2, X
	adc         __x0, X
	sta         __x1, X
	inx         
	dey         
	bne         .DaysFromCivil_label_413
	sec         
	ldy          #8
	ldx          #0
.DaysFromCivil_label_427:
	lda         __x1, X
	sbc         .lit.13, X
	sta         __x0, X
	inx         
	dey         
	bne         .DaysFromCivil_label_427
	lda         __x0
	sta         __i1
	lda         __x0+7
	and          #128
	ora         __x0+1
	sta         __i1+1
	.loc 11 43 32
	lda         __i0
	sta         __i2
	lda         __i0+1
	sta         __i2+1
	lda         __i2
	sta         __x0
	lda         __i2+1
	sta         __x0+1
	lda          #0
	ldy          #7
.DaysFromCivil_label_459:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .DaysFromCivil_label_459
	ldx          #7
.DaysFromCivil_label_470:
	lda         .lit.14, X
	sta         __x1, X
	dex         
	bpl         .DaysFromCivil_label_470
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __smul8
	lda         __i2+1
	lsr          A
	sta         __i0+1
	lda         __i2
	ror          A
	sta         __i0
	lsr         __i0+1
	ror         __i0
	lda         __i0
	sta         __x0
	lda         __i0+1
	sta         __x0+1
	lda          #0
	ldy          #7
.DaysFromCivil_label_503:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .DaysFromCivil_label_503
	clc         
	ldy          #8
	ldx          #0
.DaysFromCivil_label_515:
	lda         __x2, X
	adc         __x0, X
	sta         __x1, X
	inx         
	dey         
	bne         .DaysFromCivil_label_515
	lda         __i2
	sta         __x0
	lda         __i2+1
	sta         __x0+1
	lda          #0
	ldy          #7
.DaysFromCivil_label_531:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .DaysFromCivil_label_531
	ldx          #7
.DaysFromCivil_label_542:
	lda         .lit.20, X
	sta         __x2, X
	dex         
	bpl         .DaysFromCivil_label_542
	lda          #__x3
	ldx          #__x0
	ldy          #__x2
	jsr         __umul8
	lda         __x3+4
	sta         __x0
	lda         __x3+5
	sta         __x0+1
	lda         __x3+6
	sta         __x0+2
	lda         __x3+7
	sta         __x0+3
	lda          #0
	stz         __x0+4
	stz         __x0+5
	stz         __x0+6
	sta         __x0+7
	ldx          #5
.DaysFromCivil_label_573:
	lsr         __x0+7
	ror         __x0+6
	ror         __x0+5
	ror         __x0+4
	ror         __x0+3
	ror         __x0+2
	ror         __x0+1
	ror         __x0
	dex         
	bne         .DaysFromCivil_label_573
	lda         __x0
	sta         __i0
	lda         __x0+1
	sta         __i0+1
	lda         __i0
	sta         __x0
	lda         __i0+1
	sta         __x0+1
	lda          #0
	ldy          #7
.DaysFromCivil_label_600:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .DaysFromCivil_label_600
	sec         
	ldy          #8
	ldx          #0
.DaysFromCivil_label_612:
	lda         __x1, X
	sbc         __x0, X
	sta         __x2, X
	inx         
	dey         
	bne         .DaysFromCivil_label_612
	lda         __i1
	sta         __x0
	lda         __i1+1
	sta         __x0+1
	lda          #0
	ldy          #7
.DaysFromCivil_label_628:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .DaysFromCivil_label_628
	clc         
	ldy          #8
	ldx          #0
.DaysFromCivil_label_640:
	lda         __x2, X
	adc         __x0, X
	sta         __x1, X
	inx         
	dey         
	bne         .DaysFromCivil_label_640
	.loc 11 44 1
	ldx          #11
	jsr          __var_value8_x0			// era
	ldx          #7
.DaysFromCivil_label_656:
	lda         .lit.17, X
	sta         __x2, X
	dex         
	bpl         .DaysFromCivil_label_656
	lda          #__x3
	ldx          #__x0
	ldy          #__x2
	jsr         __smul8
	clc         
	ldy          #8
	ldx          #0
.DaysFromCivil_label_676:
	lda         __x3, X
	adc         __x1, X
	sta         __x0, X
	inx         
	dey         
	bne         .DaysFromCivil_label_676
	sec         
	ldy          #8
	ldx          #0
.DaysFromCivil_label_690:
	lda         __x0, X
	sbc         .lit.18, X
	sta         __x1, X
	inx         
	dey         
	bne         .DaysFromCivil_label_690
	lda         #__x1
	jsr         __result8
	ldy          #38
	jmp          __leave_leaf
.func_end_DaysFromCivil:
	.size DaysFromCivil, .func_end_DaysFromCivil-DaysFromCivil

	.section ".text.BrokenDownTime", "ax", @progbits
	.local  BrokenDownTime
	.type BrokenDownTime, @function

BrokenDownTime:
	lda          #143
	jsr          __enter_res
	.byte        0x05,0x60,0x00		// Save mask i:5 b:0 l:0 x:3 f:0 
	ldx          #0
	jsr          __arg_value8_x0			// seconds
	ldx          #8
	jsr          __arg_value2_i6			// result
	.loc 11 48 31
	ldx          #7
.BrokenDownTime_label_84:
	lda         __x0, X
	sta         __x1, X
	dex         
	bpl         .BrokenDownTime_label_84
	ldx          #7
.BrokenDownTime_label_96:
	lda         .lit.23, X
	sta         __x2, X
	dex         
	bpl         .BrokenDownTime_label_96
	lda          #__x3
	ldx          #__x1
	ldy          #__x2
	jsr         __sdiv8
	lda          #__x3
	ldx          #115
	jsr         __set_var_value8
	.loc 11 49 36
	ldx          #7
.BrokenDownTime_label_123:
	lda         .lit.23, X
	sta         __x2, X
	dex         
	bpl         .BrokenDownTime_label_123
	lda          #__x3
	ldx          #__x1
	ldy          #__x2
	jsr         __smod8
	lda          #__x3
	ldx          #19
	jsr         __set_var_value8
	.loc 11 50 20
	lda         __x3
	cmp          #0
	lda         __x3+1
	sbc          #0
	lda         __x3+2
	sbc          #0
	lda         __x3+3
	sbc          #0
	lda         __x3+4
	sbc          #0
	lda         __x3+5
	sbc          #0
	lda         __x3+6
	sbc          #0
	lda         __x3+7
	sbc          #0
	bvc         .BrokenDownTime_label_145
	eor          #128
.BrokenDownTime_label_145:
	bpl         .BrokenDownTime_label_203
	.loc 11 51 1
	ldx          #19
	jsr          __var_value8_x0			// remainder
	clc         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_184:
	lda         __x0, X
	adc         .lit.23, X
	sta         __x1, X
	inx         
	dey         
	bne         .BrokenDownTime_label_184
	lda          #__x1
	ldx          #19
	jsr         __set_var_value8
	.loc 11 52 1
	ldx          #115
	jsr          __var_addr_i0			// days
	lda          #__i0
	jsr         __dec8
.BrokenDownTime_label_203:
	.loc 11 55 38
	ldx          #115
	jsr          __var_value8_x0			// days
	clc         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_214:
	lda         __x0, X
	adc         .lit.27, X
	sta         __x1, X
	inx         
	dey         
	bne         .BrokenDownTime_label_214
	lda          #__x1
	ldx          #27
	jsr         __set_var_value8
	.loc 11 57 71
	lda         __x1
	cmp          #0
	lda         __x1+1
	sbc          #0
	lda         __x1+2
	sbc          #0
	lda         __x1+3
	sbc          #0
	lda         __x1+4
	sbc          #0
	lda         __x1+5
	sbc          #0
	lda         __x1+6
	sbc          #0
	lda         __x1+7
	sbc          #0
	bvc         .BrokenDownTime_label_229
	eor          #128
.BrokenDownTime_label_229:
	bmi         .BrokenDownTime_label_259
	ldx          #27
	jsr          __var_value8_x0			// adjusted_days
	ldx          #43
	jsr          __var_addr_i0			// __invented__25
	ldx          #__x0
	ldy          #__i0
	jsr          __store_indirect8
	bra         .BrokenDownTime_label_283
.BrokenDownTime_label_259:
	ldx          #27
	jsr          __var_value8_x0			// adjusted_days
	sec         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_269:
	lda         __x0, X
	sbc         .lit.28, X
	sta         __x1, X
	inx         
	dey         
	bne         .BrokenDownTime_label_269
	ldx          #43
	jsr          __var_addr_i0			// __invented__25
	ldx          #__x1
	ldy          #__i0
	jsr          __store_indirect8
.BrokenDownTime_label_283:
	ldx          #43
	jsr          __var_addr_i0			// __invented__25
	ldx          #__i0
	ldy          #__x0
	jsr          __load_indirect8
	ldx          #7
.BrokenDownTime_label_297:
	lda         .lit.29, X
	sta         __x1, X
	dex         
	bpl         .BrokenDownTime_label_297
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __sdiv8
	jsr          __spill8
	.byte __x2
	.byte 0x83,0x00
	.loc 11 58 50
	ldx          #27
	jsr          __var_value8_x0			// adjusted_days
	ldx          #7
.BrokenDownTime_label_319:
	lda         .lit.29, X
	sta         __x1, X
	dex         
	bpl         .BrokenDownTime_label_319
	lda          #__x3
	ldx          #__x2
	ldy          #__x1
	jsr         __smul8
	sec         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_340:
	lda         __x0, X
	sbc         __x3, X
	sta         __x1, X
	inx         
	dey         
	bne         .BrokenDownTime_label_340
	jsr          __spill8
	.byte __x1
	.byte 0x7b,0x00
	.loc 11 62 4
	ldx          #7
.BrokenDownTime_label_354:
	lda         .lit.30, X
	sta         __x0, X
	dex         
	bpl         .BrokenDownTime_label_354
	lda          #__x3
	ldx          #__x1
	ldy          #__x0
	jsr         __sdiv8
	sec         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_374:
	lda         __x1, X
	sbc         __x3, X
	sta         __x0, X
	inx         
	dey         
	bne         .BrokenDownTime_label_374
	ldx          #7
.BrokenDownTime_label_387:
	lda         .lit.31, X
	sta         __x3, X
	dex         
	bpl         .BrokenDownTime_label_387
	jsr          __reload8
	.byte __x2
	.byte 0x7b,0x00
	lda          #__x1
	ldx          #__x2
	ldy          #__x3
	jsr         __sdiv8
	clc         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_407:
	lda         __x0, X
	adc         __x1, X
	sta         __x3, X
	inx         
	dey         
	bne         .BrokenDownTime_label_407
	ldx          #7
.BrokenDownTime_label_420:
	lda         .lit.28, X
	sta         __x0, X
	dex         
	bpl         .BrokenDownTime_label_420
	lda          #__x1
	ldx          #__x2
	ldy          #__x0
	jsr         __sdiv8
	sec         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_440:
	lda         __x3, X
	sbc         __x1, X
	sta         __x0, X
	inx         
	dey         
	bne         .BrokenDownTime_label_440
	ldx          #7
.BrokenDownTime_label_453:
	lda         .lit.32, X
	sta         __x1, X
	dex         
	bpl         .BrokenDownTime_label_453
	lda          #__x3
	ldx          #__x0
	ldy          #__x1
	jsr         __sdiv8
	.loc 11 63 39
	ldx          #7
.BrokenDownTime_label_473:
	lda         .lit.33, X
	sta         __x0, X
	dex         
	bpl         .BrokenDownTime_label_473
	jsr          __reload8
	.byte __x2
	.byte 0x83,0x00
	lda          #__x1
	ldx          #__x2
	ldy          #__x0
	jsr         __smul8
	clc         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_493:
	lda         __x3, X
	adc         __x1, X
	sta         __x0, X
	inx         
	dey         
	bne         .BrokenDownTime_label_493
	lda          #__x0
	ldx          #67
	jsr         __set_var_value8
	.loc 11 66 58
	ldx          #7
.BrokenDownTime_label_513:
	lda         .lit.32, X
	sta         __x0, X
	dex         
	bpl         .BrokenDownTime_label_513
	lda          #__x1
	ldx          #__x3
	ldy          #__x0
	jsr         __smul8
	lda         __x3+7
	cmp          #128
	ror          A
	sta         __x0+7
	lda         __x3+6
	ror          A
	sta         __x0+6
	lda         __x3+5
	ror          A
	sta         __x0+5
	lda         __x3+4
	ror          A
	sta         __x0+4
	lda         __x3+3
	ror          A
	sta         __x0+3
	lda         __x3+2
	ror          A
	sta         __x0+2
	lda         __x3+1
	ror          A
	sta         __x0+1
	lda         __x3
	ror          A
	sta         __x0
	lda         __x0+7
	cmp          #128
	ror         __x0+7
	ror         __x0+6
	ror         __x0+5
	ror         __x0+4
	ror         __x0+3
	ror         __x0+2
	ror         __x0+1
	ror         __x0
	clc         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_572:
	lda         __x1, X
	adc         __x0, X
	sta         __x2, X
	inx         
	dey         
	bne         .BrokenDownTime_label_572
	ldx          #7
.BrokenDownTime_label_585:
	lda         .lit.35, X
	sta         __x0, X
	dex         
	bpl         .BrokenDownTime_label_585
	lda          #__x1
	ldx          #__x3
	ldy          #__x0
	jsr         __sdiv8
	sec         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_605:
	lda         __x2, X
	sbc         __x1, X
	sta         __x0, X
	inx         
	dey         
	bne         .BrokenDownTime_label_605
	jsr          __reload8
	.byte __x2
	.byte 0x7b,0x00
	sec         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_619:
	lda         __x2, X
	sbc         __x0, X
	sta         __x1, X
	inx         
	dey         
	bne         .BrokenDownTime_label_619
	.loc 11 67 50
	ldx          #7
.BrokenDownTime_label_633:
	lda         .lit.36, X
	sta         __x0, X
	dex         
	bpl         .BrokenDownTime_label_633
	lda          #__x3
	ldx          #__x1
	ldy          #__x0
	jsr         __smul8
	clc         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_653:
	lda         __x3, X
	adc         .lit.37, X
	sta         __x0, X
	inx         
	dey         
	bne         .BrokenDownTime_label_653
	ldx          #7
.BrokenDownTime_label_666:
	lda         .lit.38, X
	sta         __x3, X
	dex         
	bpl         .BrokenDownTime_label_666
	lda          #__x2
	ldx          #__x0
	ldy          #__x3
	jsr         __sdiv8
	jsr          __spill8
	.byte __x2
	.byte 0x8b,0x00
	lda          #__x2
	ldx          #83
	jsr         __set_var_value8
	.loc 11 69 58
	ldx          #7
.BrokenDownTime_label_692:
	lda         .lit.38, X
	sta         __x0, X
	dex         
	bpl         .BrokenDownTime_label_692
	lda          #__x3
	ldx          #__x2
	ldy          #__x0
	jsr         __smul8
	clc         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_712:
	lda         __x3, X
	adc         .lit.37, X
	sta         __x0, X
	inx         
	dey         
	bne         .BrokenDownTime_label_712
	ldx          #7
.BrokenDownTime_label_725:
	lda         .lit.36, X
	sta         __x3, X
	dex         
	bpl         .BrokenDownTime_label_725
	lda          #__x2
	ldx          #__x0
	ldy          #__x3
	jsr         __sdiv8
	sec         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_745:
	lda         __x1, X
	sbc         __x2, X
	sta         __x0, X
	inx         
	dey         
	bne         .BrokenDownTime_label_745
	clc         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_759:
	lda         __x0, X
	adc         .lit.26, X
	sta         __x1, X
	inx         
	dey         
	bne         .BrokenDownTime_label_759
	lda         __x1
	sta         __i0
	lda         __x1+7
	and          #128
	ora         __x1+1
	sta         __i0+1
	lda         __i0
	sta         __i4
	lda         __i0+1
	sta         __i4+1
	.loc 11 71 19
	jsr          __reload8
	.byte __x0
	.byte 0x8b,0x00
	lda         __x0
	cmp          #10
	lda         __x0+1
	sbc          #0
	lda         __x0+2
	sbc          #0
	lda         __x0+3
	sbc          #0
	lda         __x0+4
	sbc          #0
	lda         __x0+5
	sbc          #0
	lda         __x0+6
	sbc          #0
	lda         __x0+7
	sbc          #0
	bvc         .BrokenDownTime_label_783
	eor          #128
.BrokenDownTime_label_783:
	bpl         .BrokenDownTime_label_827
	ldx          #83
	jsr          __var_value8_x0			// month_prime
	clc         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_812:
	lda         __x0, X
	adc         .lit.41, X
	sta         __x1, X
	inx         
	dey         
	bne         .BrokenDownTime_label_812
	ldx          #91
	jsr          __var_addr_i0			// __invented__26
	ldx          #__x1
	ldy          #__i0
	jsr          __store_indirect8
	bra         .BrokenDownTime_label_851
.BrokenDownTime_label_827:
	ldx          #83
	jsr          __var_value8_x0			// month_prime
	sec         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_837:
	lda         __x0, X
	sbc         .lit.42, X
	sta         __x1, X
	inx         
	dey         
	bne         .BrokenDownTime_label_837
	ldx          #91
	jsr          __var_addr_i0			// __invented__26
	ldx          #__x1
	ldy          #__i0
	jsr          __store_indirect8
.BrokenDownTime_label_851:
	ldx          #91
	jsr          __var_addr_i0			// __invented__26
	ldx          #__i0
	ldy          #__x0
	jsr          __load_indirect8
	lda         __x0
	sta         __i0
	lda         __x0+7
	and          #128
	ora         __x0+1
	sta         __i0+1
	lda         __i0
	sta         __i5
	lda         __i0+1
	sta         __i5+1
	.loc 11 72 1
	ldx          #1
	lda          #0
	cmp         __i0+1
	bcc         .BrokenDownTime_label_878
	bne         .BrokenDownTime_label_879
	lda          #2
	cmp         __i0
	bcs         .BrokenDownTime_label_879
.BrokenDownTime_label_878:
	dex         
.BrokenDownTime_label_879:
	txa         
	sta         __i0
	stz         __i0+1
	sta         __x0
	lda         __i0+1
	sta         __x0+1
	and          #128
	beq         .BrokenDownTime_label_905
	lda          #255
.BrokenDownTime_label_905:
	ldy          #7
.BrokenDownTime_label_910:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .BrokenDownTime_label_910
	ldx          #67
	jsr          __var_value8_x1			// year
	clc         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_924:
	lda         __x1, X
	adc         __x0, X
	sta         __x2, X
	inx         
	dey         
	bne         .BrokenDownTime_label_924
	lda          #__x2
	ldx          #67
	jsr         __set_var_value8
	.loc 11 74 30
	sec         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_944:
	lda         __x2, X
	sbc         .lit.45, X
	sta         __x0, X
	inx         
	dey         
	bne         .BrokenDownTime_label_944
	lda          #__x0
	ldx          #99
	jsr         __set_var_value8
	.loc 11 75 39
	lda         __x0
	sta         __i0
	lda         __x0+7
	and          #128
	ora         __x0+1
	sta         __i0+1
	lda         __i0
	sta         __x1
	lda         __i0+1
	sta         __x1+1
	and          #128
	beq         .BrokenDownTime_label_975
	lda          #255
.BrokenDownTime_label_975:
	ldy          #7
.BrokenDownTime_label_979:
	sta         __x1, Y
	dey         
	cpy          #1
	bne         .BrokenDownTime_label_979
	lda         __x1
	cmp         __x0
	bne         .BrokenDownTime_label_986
	lda         __x1+1
	cmp         __x0+1
	bne         .BrokenDownTime_label_986
	lda         __x1+2
	cmp         __x0+2
	bne         .BrokenDownTime_label_986
	lda         __x1+3
	cmp         __x0+3
	bne         .BrokenDownTime_label_986
	lda         __x1+4
	cmp         __x0+4
	bne         .BrokenDownTime_label_986
	lda         __x1+5
	cmp         __x0+5
	bne         .BrokenDownTime_label_986
	lda         __x1+6
	cmp         __x0+6
	bne         .BrokenDownTime_label_986
	lda         __x1+7
	cmp         __x0+7
	beq         .BrokenDownTime_label_1037
.BrokenDownTime_label_986:
	.loc 11 76 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #32
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 11 77 1
	lda          #255
	sta         __i0
	sta         __i0+1
	ldx          #144
	lda          #__i0
	jsr         __load_result_value2
.BrokenDownTime_label_1034:
	ldy          #156
	jmp          __leave
.BrokenDownTime_label_1037:
	.loc 11 80 1
	ldx          #19
	jsr          __var_value8_x0			// remainder
	ldx          #7
.BrokenDownTime_label_1046:
	lda         .lit.50, X
	sta         __x1, X
	dex         
	bpl         .BrokenDownTime_label_1046
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __smod8
	lda         __x2
	sta         __i0
	lda         __x2+7
	and          #128
	ora         __x2+1
	sta         __i0+1
	lda         __i6
	sta         __i1
	lda         __i6+1
	sta         __i1+1
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
	.loc 11 81 1
	ldx          #7
.BrokenDownTime_label_1090:
	lda         .lit.50, X
	sta         __x1, X
	dex         
	bpl         .BrokenDownTime_label_1090
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __sdiv8
	ldx          #7
.BrokenDownTime_label_1108:
	lda         .lit.50, X
	sta         __x1, X
	dex         
	bpl         .BrokenDownTime_label_1108
	lda          #__x3
	ldx          #__x2
	ldy          #__x1
	jsr         __smod8
	lda         __x3
	sta         __i0
	lda         __x3+7
	and          #128
	ora         __x3+1
	sta         __i0+1
	lda         __i0
	ldy          #2
	sta         (__i1), Y
	lda         __i0+1
	iny         
	sta         (__i1), Y
	.loc 11 82 1
	ldx          #7
.BrokenDownTime_label_1147:
	lda         .lit.53, X
	sta         __x1, X
	dex         
	bpl         .BrokenDownTime_label_1147
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __sdiv8
	lda         __x2
	sta         __i0
	lda         __x2+7
	and          #128
	ora         __x2+1
	sta         __i0+1
	lda         __i0
	ldy          #4
	sta         (__i1), Y
	lda         __i0+1
	iny         
	sta         (__i1), Y
	.loc 11 83 1
	lda         __i4
	ldy          #6
	sta         (__i1), Y
	lda         __i4+1
	iny         
	sta         (__i1), Y
	.loc 11 84 1
	sec         
	lda         __i5
	sbc          #1
	sta         __i0
	lda         __i5+1
	sbc          #0
	sta         __i0+1
	lda         __i0
	ldy          #8
	sta         (__i1), Y
	lda         __i0+1
	iny         
	sta         (__i1), Y
	.loc 11 85 1
	ldx          #99
	jsr          __var_value8_x0			// tm_year
	lda         __x0
	sta         __i0
	lda         __x0+7
	and          #128
	ora         __x0+1
	sta         __i0+1
	lda         __i0
	ldy          #10
	sta         (__i1), Y
	lda         __i0+1
	iny         
	sta         (__i1), Y
	.loc 11 86 34
	ldx          #115
	jsr          __var_value8_x0			// days
	clc         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_1243:
	lda         __x0, X
	adc         .lit.34, X
	sta         __x1, X
	inx         
	dey         
	bne         .BrokenDownTime_label_1243
	ldx          #7
.BrokenDownTime_label_1255:
	lda         .lit.58, X
	sta         __x0, X
	dex         
	bpl         .BrokenDownTime_label_1255
	lda          #__x2
	ldx          #__x1
	ldy          #__x0
	jsr         __smod8
	lda          #__x2
	ldx          #107
	jsr         __set_var_value8
	.loc 11 87 19
	lda         __x2
	cmp          #0
	lda         __x2+1
	sbc          #0
	lda         __x2+2
	sbc          #0
	lda         __x2+3
	sbc          #0
	lda         __x2+4
	sbc          #0
	lda         __x2+5
	sbc          #0
	lda         __x2+6
	sbc          #0
	lda         __x2+7
	sbc          #0
	bvc         .BrokenDownTime_label_1276
	eor          #128
.BrokenDownTime_label_1276:
	bpl         .BrokenDownTime_label_1318
	.loc 11 88 1
	ldx          #107
	jsr          __var_value8_x0			// week_day
	clc         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_1306:
	lda         __x0, X
	adc         .lit.58, X
	sta         __x1, X
	inx         
	dey         
	bne         .BrokenDownTime_label_1306
	lda          #__x1
	ldx          #107
	jsr         __set_var_value8
.BrokenDownTime_label_1318:
	.loc 11 90 1
	ldx          #107
	jsr          __var_value8_x0			// week_day
	lda         __x0
	sta         __i0
	lda         __x0+7
	and          #128
	ora         __x0+1
	sta         __i0+1
	lda         __i6
	sta         __i7
	lda         __i6+1
	sta         __i7+1
	lda         __i0
	ldy          #12
	sta         (__i7), Y
	lda         __i0+1
	iny         
	sta         (__i7), Y
	.loc 11 91 1
	ldx          #115
	jsr          __var_value8_x0			// days
	ldx          #11
	jsr          __var_addr_i8			// __invented__27
	ldx          #__x0
	ldy          #__i8
	jsr          __store_indirect8
	ldx          #67
	jsr          __var_value8_x0			// year
	ldx          #1
	jsr         __pushxy0
	ldx          #1
	jsr         __pushxy0
	jsr         __pushx0
	ldx         #__x1
	ldy          #0
	jsr         DaysFromCivil
	ldx          #__i8
	ldy          #__x0
	jsr          __load_indirect8
	sec         
	ldy          #8
	ldx          #0
.BrokenDownTime_label_1387:
	lda         __x0, X
	sbc         __x1, X
	sta         __x2, X
	inx         
	dey         
	bne         .BrokenDownTime_label_1387
	lda         __x2
	sta         __i0
	lda         __x2+7
	and          #128
	ora         __x2+1
	sta         __i0+1
	lda         __i0
	ldy          #14
	sta         (__i7), Y
	lda         __i0+1
	iny         
	sta         (__i7), Y
	.loc 11 92 1
	stz         __i0
	stz         __i0+1
	ldx          #144
	lda          #__i0
	jsr         __load_result_value2
	jmp         .BrokenDownTime_label_1034
.func_end_BrokenDownTime:
	.size BrokenDownTime, .func_end_BrokenDownTime-BrokenDownTime

	.section ".text.ReadTimeZoneInfo", "ax", @progbits
	.local  ReadTimeZoneInfo
	.type ReadTimeZoneInfo, @function

ReadTimeZoneInfo:
	lda          #7
	jsr          __enter_res
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
	ldx          #14
	jsr          __arg_value2_i0			// abbreviation_capacity
	ldx          #12
	jsr          __arg_value2_i1			// abbreviation
	ldx          #10
	jsr          __arg_value2_i2			// info
	ldx          #0
	jsr          __arg_value2_i3			// zone_name
	.loc 11 100 1
	jsr         __pushi0
	jsr         __pushi1
	jsr         __pushi2
	ldx          #2
	lda          #__i4
	jsr          __arg_addr_push
	jsr         __pushi3
	ldx          #56
	jsr         __pushxy0
	ldx         #__l0
	ldy          #0
	jsr         syscall
	jsr         __incsp12
	lda         __l0
	sta         __i0
	lda         __l0+3
	and          #128
	ora         __l0+1
	sta         __i0+1
	ldx          #8
	lda          #__i0
	jsr         __load_result_value2
	ldy          #26
	jmp          __leave
.func_end_ReadTimeZoneInfo:
	.size ReadTimeZoneInfo, .func_end_ReadTimeZoneInfo-ReadTimeZoneInfo

	.section ".text.timespec_get", "ax", @progbits
	.global timespec_get
	.type timespec_get, @function

timespec_get:
	lda          #24
	jsr          __enter_res
	.byte        0x02,0x60,0x00		// Save mask i:2 b:0 l:0 x:3 f:0 
	ldx          #2
	jsr          __arg_value2_i0			// base
	ldx          #0
	jsr          __arg_value2_i4			// result
	.loc 11 112 40
	ldx          #0
	lda         __i0
	cmp          #1
	bne         .timespec_get_label_35
	lda         __i0+1
	beq         .timespec_get_label_34
.timespec_get_label_35:
	inx         
.timespec_get_label_34:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i5			// __invented__33
	lda         __b0
	sta         (__i5)
	lda         __i0
	cmp          #1
	bne         .timespec_get_label_84
	lda         __i0+1
	bne         .timespec_get_label_84
	ldx          #1
	lda         __i4
	bne         .timespec_get_label_65
	lda         __i4+1
	beq         .timespec_get_label_66
.timespec_get_label_65:
	dex         
.timespec_get_label_66:
	txa         
	sta         (__i5)
.timespec_get_label_84:
	lda         (__i5)
	beq         .timespec_get_label_106
	.loc 11 113 1
	stz         __i0
	stz         __i0+1
	ldx          #25
	lda          #__i0
	jsr         __load_result_value2
.timespec_get_label_103:
	ldy          #31
	jmp          __leave
.timespec_get_label_106:
	.loc 11 116 25
	ldx          #7
	lda          #0
.timespec_get_label_113:
	sta         __x0, X
	dex         
	bpl         .timespec_get_label_113
	lda          #__x0
	ldx          #12
	jsr         __set_var_value8
	.loc 11 139 38
	ldx          #12
	lda          #__i0
	jsr          __var_addr_push_i0
	ldx          #30
	jsr         __pushxy0
	ldx         #__l0
	ldy          #0
	jsr         syscall
	jsr         __incsp4
	lda         __l0
	ora         __l0+1
	ora         __l0+2
	ora         __l0+3
	beq         .timespec_get_label_156
	.loc 11 140 1
	stz         __i0
	stz         __i0+1
	ldx          #25
	lda          #__i0
	jsr         __load_result_value2
	bra         .timespec_get_label_103
.timespec_get_label_156:
	.loc 11 145 41
	ldx          #12
	jsr          __var_value8_x0			// microseconds
	ldx          #7
.timespec_get_label_166:
	lda         .lit.82, X
	sta         __x2, X
	dex         
	bpl         .timespec_get_label_166
	lda          #__x3
	ldx          #__x0
	ldy          #__x2
	jsr         __sdiv8
	ldx          #7
.timespec_get_label_183:
	lda         __x3, X
	sta         __x1, X
	dex         
	bpl         .timespec_get_label_183
	.loc 11 146 43
	ldx          #7
.timespec_get_label_194:
	lda         .lit.82, X
	sta         __x2, X
	dex         
	bpl         .timespec_get_label_194
	lda          #__x3
	ldx          #__x0
	ldy          #__x2
	jsr         __smod8
	lda          #__x3
	ldx          #20
	jsr         __set_var_value8
	.loc 11 147 20
	lda         __x3
	cmp          #0
	lda         __x3+1
	sbc          #0
	lda         __x3+2
	sbc          #0
	lda         __x3+3
	sbc          #0
	lda         __x3+4
	sbc          #0
	lda         __x3+5
	sbc          #0
	lda         __x3+6
	sbc          #0
	lda         __x3+7
	sbc          #0
	bvc         .timespec_get_label_216
	eor          #128
.timespec_get_label_216:
	bpl         .timespec_get_label_267
	.loc 11 148 1
	ldx          #20
	jsr          __var_value8_x0			// remainder
	clc         
	ldy          #8
	ldx          #0
.timespec_get_label_251:
	lda         __x0, X
	adc         .lit.82, X
	sta         __x2, X
	inx         
	dey         
	bne         .timespec_get_label_251
	lda          #__x2
	ldx          #20
	jsr         __set_var_value8
	.loc 11 149 1
	lda          #__x1
	jsr         __rdecd
.timespec_get_label_267:
	.loc 11 151 1
	ldx          #7
.timespec_get_label_272:
	lda         __x1, X
	sta         __x0, X
	dex         
	bpl         .timespec_get_label_272
	lda         __x0
	sta         __l0
	lda         __x0+1
	sta         __l0+1
	lda         __x0+2
	sta         __l0+2
	lda         __x0+7
	and          #128
	ora         __x0+3
	sta         __l0+3
	lda         __i4
	sta         __i0
	lda         __i4+1
	sta         __i0+1
	ldx          #__l0
	ldy          #__i0
	jsr          __store_indirect4
	.loc 11 152 41
	ldx          #__i0
	ldy          #__l0
	jsr          __load_indirect4
	lda         __l0
	sta         __x2
	lda         __l0+1
	sta         __x2+1
	lda         __l0+2
	sta         __x2+2
	lda         __l0+3
	sta         __x2+3
	and          #128
	beq         .timespec_get_label_319
	lda          #255
.timespec_get_label_319:
	ldy          #7
.timespec_get_label_324:
	sta         __x2, Y
	dey         
	cpy          #3
	bne         .timespec_get_label_324
	lda         __x2
	cmp         __x0
	bne         .timespec_get_label_331
	lda         __x2+1
	cmp         __x0+1
	bne         .timespec_get_label_331
	lda         __x2+2
	cmp         __x0+2
	bne         .timespec_get_label_331
	lda         __x2+3
	cmp         __x0+3
	bne         .timespec_get_label_331
	lda         __x2+4
	cmp         __x0+4
	bne         .timespec_get_label_331
	lda         __x2+5
	cmp         __x0+5
	bne         .timespec_get_label_331
	lda         __x2+6
	cmp         __x0+6
	bne         .timespec_get_label_331
	lda         __x2+7
	cmp         __x0+7
	beq         .timespec_get_label_364
.timespec_get_label_331:
	.loc 11 153 1
	stz         __i0
	stz         __i0+1
	ldx          #25
	lda          #__i0
	jsr         __load_result_value2
	jmp         .timespec_get_label_103
.timespec_get_label_364:
	.loc 11 155 1
	ldx          #20
	jsr          __var_value8_x0			// remainder
	ldx          #7
.timespec_get_label_374:
	lda         .lit.88, X
	sta         __x2, X
	dex         
	bpl         .timespec_get_label_374
	lda          #__x3
	ldx          #__x0
	ldy          #__x2
	jsr         __smul8
	lda         __x3
	sta         __l0
	lda         __x3+1
	sta         __l0+1
	lda         __x3+2
	sta         __l0+2
	lda         __x3+7
	and          #128
	ora         __x3+3
	sta         __l0+3
	ldy          #7
	ldx          #3
.timespec_get_label_405:
	lda         __l0, X
	sta         (__i4), Y
	dey         
	dex         
	bpl         .timespec_get_label_405
	.loc 11 156 1
	lda          #1
	sta         __i0
	dec          A
	stz         __i0+1
	ldx          #25
	lda          #__i0
	jsr         __load_result_value2
	jmp         .timespec_get_label_103
.func_end_timespec_get:
	.size timespec_get, .func_end_timespec_get-timespec_get

	.section ".text.timespec_getres", "ax", @progbits
	.global timespec_getres
	.type timespec_getres, @function

timespec_getres:
	lda          #5
	jsr          __enter_leaf_res_nomask
	ldx          #2
	jsr          __arg_value2_i0			// base
	ldx          #0
	jsr          __arg_value2_i1			// result
	.loc 11 160 16
	lda         __i0
	cmp          #1
	bne         .timespec_getres_label_17
	lda         __i0+1
	beq         .timespec_getres_label_38
.timespec_getres_label_17:
	.loc 11 161 1
	stz         __i0
	stz         __i0+1
	lda         #__i0
	jsr         __result2
.timespec_getres_label_35:
	ldy          #12
	jmp          __leave_leaf_nomask
.timespec_getres_label_38:
	.loc 11 163 27
	lda         __i1
	bne         .timespec_getres_label_40
	lda         __i1+1
	beq         .timespec_getres_label_78
.timespec_getres_label_40:
	.loc 11 165 1
	lda         __i1
	sta         __i0
	lda         __i1+1
	sta         __i0+1
	ldy          #3
	lda          #0
.timespec_getres_label_60:
	sta         (__i0), Y
	dey         
	bpl         .timespec_getres_label_60
	.loc 11 166 1
	ldy          #7
	ldx          #3
.timespec_getres_label_72:
	lda         .lit.95, X
	sta         (__i0), Y
	dey         
	dex         
	bpl         .timespec_getres_label_72
.timespec_getres_label_78:
	.loc 11 172 1
	lda          #1
	sta         __i0
	dec          A
	stz         __i0+1
	lda         #__i0
	jsr         __result2
	bra         .timespec_getres_label_35
.func_end_timespec_getres:
	.size timespec_getres, .func_end_timespec_getres-timespec_getres

	.section ".text.gmtime_r", "ax", @progbits
	.global gmtime_r
	.type gmtime_r, @function

gmtime_r:
	lda          #7
	jsr          __enter_res
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i4			// result
	ldx          #0
	jsr          __arg_value2_i0			// time_point
	.loc 11 176 56
	ldx          #__i0
	ldy          #__l0
	jsr          __load_indirect4
	lda         __l0
	sta         __x0
	lda         __l0+1
	sta         __x0+1
	lda         __l0+2
	sta         __x0+2
	lda         __l0+3
	sta         __x0+3
	and          #128
	beq         .gmtime_r_label_41
	lda          #255
.gmtime_r_label_41:
	ldy          #7
.gmtime_r_label_47:
	sta         __x0, Y
	dey         
	cpy          #3
	bne         .gmtime_r_label_47
	jsr         __pushi4
	jsr         __pushx0
	ldx         #__i1
	ldy          #0
	jsr         BrokenDownTime
	lda         __i1
	ora         __i1+1
	beq         .gmtime_r_label_75
	.loc 11 177 1
	stz         __i0
	stz         __i0+1
	ldx          #8
	lda          #__i0
	jsr         __load_result_value2
.gmtime_r_label_72:
	ldy          #14
	jmp          __leave
.gmtime_r_label_75:
	.loc 11 179 1
	lda         __i4
	sta         __i0
	lda         __i4+1
	sta         __i0+1
	ldy          #16
	lda          #0
.gmtime_r_label_88:
	sta         (__i0), Y
	iny         
	cpy          #18
	bne         .gmtime_r_label_88
	.loc 11 180 1
	ldy          #18
	lda          #0
.gmtime_r_label_99:
	sta         (__i0), Y
	iny         
	cpy          #22
	bne         .gmtime_r_label_99
	.loc 11 181 1
	lda         #%lo(.str.97)
	sta         __i1
	lda         #%hi(.str.97)
	sta         __i1+1
	lda         __i1
	ldy          #22
	sta         (__i0), Y
	lda         __i1+1
	iny         
	sta         (__i0), Y
	.loc 11 182 1
	ldx          #8
	lda          #__i0
	jsr         __load_result_value2
	bra         .gmtime_r_label_72
.func_end_gmtime_r:
	.size gmtime_r, .func_end_gmtime_r-gmtime_r

	.section ".text.localtime_r", "ax", @progbits
	.global localtime_r
	.type localtime_r, @function

localtime_r:
	stx          __result
	sty          __result+1
	ldx          #54
	ldy          #1
	jsr          __enter+2
	.byte        0x06,0x60,0x00		// Save mask i:6 b:0 l:0 x:3 f:0 
	ldx          #2
	jsr          __arg_value2_i4			// result
	ldx          #0
	jsr          __arg_value2_i5			// time_point
	.loc 11 188 52
	ldx          #0
	ldy          #1
	jsr         __pushxy
	ldx          #50
	ldy          #1
	lda          #__i6
	jsr          __var_addrb_push_i6
	ldx          #52
	jsr         __pushxy0
	ldx         #__l0
	ldy          #0
	jsr         syscall
	jsr         __incsp6
	lda         __l0
	cmp          #0
	lda         __l0+1
	sbc          #0
	lda         __l0+2
	sbc          #0
	lda         __l0+3
	sbc          #0
	bvc         .localtime_r_label_73
	eor          #128
.localtime_r_label_73:
	bpl         .localtime_r_label_98
	.loc 11 189 1
	ldy          #57
	ldx          #1
	jsr          __leave+2
	ldx         __result
	ldy         __result+1
	jmp         gmtime_r
.localtime_r_label_95:
	ldy          #61
	ldx          #1
	jmp          __leave+2
.localtime_r_label_98:
	.loc 11 194 31
	ldx          #__i5
	ldy          #__l0
	jsr          __load_indirect4
	lda         __l0
	sta         __x0
	lda         __l0+1
	sta         __x0+1
	lda         __l0+2
	sta         __x0+2
	lda         __l0+3
	sta         __x0+3
	and          #128
	beq         .localtime_r_label_115
	lda          #255
.localtime_r_label_115:
	ldy          #7
.localtime_r_label_121:
	sta         __x0, Y
	dey         
	cpy          #3
	bne         .localtime_r_label_121
	ldx          #16
	jsr         __pushxy0
	ldx         #%lo(local_time_zone)
	ldy         #%hi(local_time_zone)
	jsr         __pushxy
	ldx          #32
	lda          #__i7
	jsr          __var_addr_push_i7
	jsr         __pushx0
	jsr         __pushi6
	ldx         #__i0
	ldy          #0
	jsr         ReadTimeZoneInfo
	lda         __i0
	cmp          #0
	lda         __i0+1
	sbc          #0
	bvc         .localtime_r_label_151
	eor          #128
.localtime_r_label_151:
	bpl         .localtime_r_label_165
	.loc 11 195 1
	ldy          #57
	ldx          #1
	jsr          __leave+2
	ldx         __result
	ldy         __result+1
	jmp         gmtime_r
.localtime_r_label_165:
	.loc 11 198 39
	ldx          #__i5
	ldy          #__l0
	jsr          __load_indirect4
	lda         __l0
	sta         __x0
	lda         __l0+1
	sta         __x0+1
	lda         __l0+2
	sta         __x0+2
	lda         __l0+3
	sta         __x0+3
	and          #128
	beq         .localtime_r_label_182
	lda          #255
.localtime_r_label_182:
	ldy          #7
.localtime_r_label_186:
	sta         __x0, Y
	dey         
	cpy          #3
	bne         .localtime_r_label_186
	ldx          #7
.localtime_r_label_194:
	lda         __x0, X
	sta         __x1, X
	dex         
	bpl         .localtime_r_label_194
	.loc 11 199 47
	lda          #0
	eor          #255
	sta         __x0
	lda          #0
	eor          #255
	sta         __x0+1
	lda          #0
	eor          #255
	sta         __x0+2
	lda          #0
	eor          #255
	sta         __x0+3
	lda          #0
	eor          #255
	sta         __x0+4
	lda          #0
	eor          #255
	sta         __x0+5
	lda          #0
	eor          #255
	sta         __x0+6
	lda          #0
	eor          #255
	sta         __x0+7
	lsr          A
	sta         __x2+7
	lda         __x0+6
	ror          A
	sta         __x2+6
	lda         __x0+5
	ror          A
	sta         __x2+5
	lda         __x0+4
	ror          A
	sta         __x2+4
	lda         __x0+3
	ror          A
	sta         __x2+3
	lda         __x0+2
	ror          A
	sta         __x2+2
	lda         __x0+1
	ror          A
	sta         __x2+1
	lda         __x0
	ror          A
	sta         __x2
	lda          #__x2
	ldx          #40
	jsr         __set_var_value8
	.loc 11 200 31
	sec         
	ldy          #8
	ldx          #0
.localtime_r_label_271:
	lda          #0
	sbc         __x2, X
	sta         __x0, X
	inx         
	dey         
	bne         .localtime_r_label_271
	sec         
	ldy          #8
	ldx          #0
.localtime_r_label_285:
	lda         __x0, X
	sbc         .lit.125, X
	sta         __x2, X
	inx         
	dey         
	bne         .localtime_r_label_285
	lda          #__x2
	ldx          #48
	jsr         __set_var_value8
	.loc 11 204 43
	ldy          #19
	ldx          #3
.localtime_r_label_307:
	lda         (__i7), Y
	sta         __l0, X
	dey         
	dex         
	bpl         .localtime_r_label_307
	ldx          #0
	txa         
	cmp         __l0
	sbc         __l0+1
	txa         
	sbc         __l0+2
	txa         
	sbc         __l0+3
	bvc         .localtime_r_label_318
	eor          #128
.localtime_r_label_318:
	bpl         .localtime_r_label_316
	inx         
.localtime_r_label_316:
	stx         __b0
	ldx          #49
	jsr          __var_addr_i8			// __invented__44
	lda         __b0
	sta         (__i8)
	lda          #0
	cmp         __l0
	sbc         __l0+1
	lda          #0
	sbc         __l0+2
	lda          #0
	sbc         __l0+3
	bvc         .localtime_r_label_341
	eor          #128
.localtime_r_label_341:
	bpl         .localtime_r_label_438
	ldx          #40
	jsr          __var_value8_x0			// maximum
	ldy          #19
	ldx          #3
.localtime_r_label_362:
	lda         (__i7), Y
	sta         __l0, X
	dey         
	dex         
	bpl         .localtime_r_label_362
	lda         __l0
	sta         __x2
	lda         __l0+1
	sta         __x2+1
	lda         __l0+2
	sta         __x2+2
	lda         __l0+3
	sta         __x2+3
	and          #128
	beq         .localtime_r_label_380
	lda          #255
.localtime_r_label_380:
	ldy          #7
.localtime_r_label_384:
	sta         __x2, Y
	dey         
	cpy          #3
	bne         .localtime_r_label_384
	sec         
	ldy          #8
	ldx          #0
.localtime_r_label_396:
	lda         __x0, X
	sbc         __x2, X
	sta         __x3, X
	inx         
	dey         
	bne         .localtime_r_label_396
	ldx          #0
	lda         __x3
	cmp         __x1
	lda         __x3+1
	sbc         __x1+1
	lda         __x3+2
	sbc         __x1+2
	lda         __x3+3
	sbc         __x1+3
	lda         __x3+4
	sbc         __x1+4
	lda         __x3+5
	sbc         __x1+5
	lda         __x3+6
	sbc         __x1+6
	lda         __x3+7
	sbc         __x1+7
	bvc         .localtime_r_label_408
	eor          #128
.localtime_r_label_408:
	bpl         .localtime_r_label_406
	inx         
.localtime_r_label_406:
	txa         
	sta         (__i8)
.localtime_r_label_438:
	lda         (__i8)
	sta         __b0
	ldx          #50
	jsr          __var_addr_i9			// __invented__45
	lda         __b0
	sta         (__i9)
	beq         .localtime_r_label_839
	jmp         .localtime_r_label_613
.localtime_r_label_839:
	ldy          #19
	ldx          #3
.localtime_r_label_465:
	lda         (__i7), Y
	sta         __l0, X
	dey         
	dex         
	bpl         .localtime_r_label_465
	ldx          #0
	lda         __l0
	cmp          #0
	lda         __l0+1
	sbc          #0
	lda         __l0+2
	sbc          #0
	lda         __l0+3
	sbc          #0
	bvc         .localtime_r_label_476
	eor          #128
.localtime_r_label_476:
	bpl         .localtime_r_label_474
	inx         
.localtime_r_label_474:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i0			// __invented__46
	lda         __b0
	sta         (__i0)
	lda         __l0
	cmp          #0
	lda         __l0+1
	sbc          #0
	lda         __l0+2
	sbc          #0
	lda         __l0+3
	sbc          #0
	bvc         .localtime_r_label_499
	eor          #128
.localtime_r_label_499:
	bpl         .localtime_r_label_596
	ldx          #48
	jsr          __var_value8_x0			// minimum
	ldy          #19
	ldx          #3
.localtime_r_label_520:
	lda         (__i7), Y
	sta         __l0, X
	dey         
	dex         
	bpl         .localtime_r_label_520
	lda         __l0
	sta         __x2
	lda         __l0+1
	sta         __x2+1
	lda         __l0+2
	sta         __x2+2
	lda         __l0+3
	sta         __x2+3
	and          #128
	beq         .localtime_r_label_538
	lda          #255
.localtime_r_label_538:
	ldy          #7
.localtime_r_label_542:
	sta         __x2, Y
	dey         
	cpy          #3
	bne         .localtime_r_label_542
	sec         
	ldy          #8
	ldx          #0
.localtime_r_label_554:
	lda         __x0, X
	sbc         __x2, X
	sta         __x3, X
	inx         
	dey         
	bne         .localtime_r_label_554
	ldx          #0
	lda         __x1
	cmp         __x3
	lda         __x1+1
	sbc         __x3+1
	lda         __x1+2
	sbc         __x3+2
	lda         __x1+3
	sbc         __x3+3
	lda         __x1+4
	sbc         __x3+4
	lda         __x1+5
	sbc         __x3+5
	lda         __x1+6
	sbc         __x3+6
	lda         __x1+7
	sbc         __x3+7
	bvc         .localtime_r_label_566
	eor          #128
.localtime_r_label_566:
	bpl         .localtime_r_label_564
	inx         
.localtime_r_label_564:
	txa         
	sta         (__i0)
.localtime_r_label_596:
	lda         (__i0)
	sta         (__i9)
.localtime_r_label_613:
	lda         (__i9)
	beq         .localtime_r_label_647
	.loc 11 205 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #32
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 11 206 1
	stz         __i0
	stz         __i0+1
	ldx          #55
	ldy          #1
	lda          #__i0
	jsr         __load_result_value2b
	jmp         .localtime_r_label_95
.localtime_r_label_647:
	.loc 11 208 1
	ldy          #19
	ldx          #3
.localtime_r_label_656:
	lda         (__i7), Y
	sta         __l0, X
	dey         
	dex         
	bpl         .localtime_r_label_656
	lda         __l0
	sta         __x0
	lda         __l0+1
	sta         __x0+1
	lda         __l0+2
	sta         __x0+2
	lda         __l0+3
	sta         __x0+3
	and          #128
	beq         .localtime_r_label_674
	lda          #255
.localtime_r_label_674:
	ldy          #7
.localtime_r_label_678:
	sta         __x0, Y
	dey         
	cpy          #3
	bne         .localtime_r_label_678
	clc         
	ldy          #8
	ldx          #0
.localtime_r_label_689:
	lda         __x1, X
	adc         __x0, X
	sta         __x2, X
	inx         
	dey         
	bne         .localtime_r_label_689
	ldx          #7
.localtime_r_label_699:
	lda         __x2, X
	sta         __x1, X
	dex         
	bpl         .localtime_r_label_699
	.loc 11 209 43
	jsr         __pushi4
	jsr         __pushx2
	ldx         #__i0
	ldy          #0
	jsr         BrokenDownTime
	lda         __i0
	ora         __i0+1
	beq         .localtime_r_label_725
	.loc 11 210 1
	stz         __i0
	stz         __i0+1
	ldx          #55
	ldy          #1
	lda          #__i0
	jsr         __load_result_value2b
	jmp         .localtime_r_label_95
.localtime_r_label_725:
	.loc 11 212 1
	ldy          #23
	ldx          #3
.localtime_r_label_735:
	lda         (__i7), Y
	sta         __l0, X
	dey         
	dex         
	bpl         .localtime_r_label_735
	lda         __l0
	and          #1
	sta         __l1
	stz         __l1+1
	stz         __l1+2
	stz         __l1+3
	ldx          #1
	ora         __l1+1
	ora         __l1+2
	ora         __l1+3
	bne         .localtime_r_label_757
	dex         
.localtime_r_label_757:
	txa         
	sta         __i0
	stz         __i0+1
	lda         __i4
	sta         __i1
	lda         __i4+1
	sta         __i1+1
	lda         __i0
	ldy          #16
	sta         (__i1), Y
	lda         __i0+1
	iny         
	sta         (__i1), Y
	.loc 11 213 1
	ldy          #19
	ldx          #3
.localtime_r_label_799:
	lda         (__i7), Y
	sta         __l0, X
	dey         
	dex         
	bpl         .localtime_r_label_799
	ldy          #21
	ldx          #3
.localtime_r_label_812:
	lda         __l0, X
	sta         (__i1), Y
	dey         
	dex         
	bpl         .localtime_r_label_812
	.loc 11 214 1
	lda         #%lo(local_time_zone)
	sta         __i0
	lda         #%hi(local_time_zone)
	sta         __i0+1
	lda         __i0
	ldy          #22
	sta         (__i1), Y
	lda         __i0+1
	iny         
	sta         (__i1), Y
	.loc 11 215 1
	ldx          #55
	ldy          #1
	lda          #__i1
	jsr         __load_result_value2b
	jmp         .localtime_r_label_95
.func_end_localtime_r:
	.size localtime_r, .func_end_localtime_r-localtime_r

	.section ".text.gmtime", "ax", @progbits
	.global gmtime
	.type gmtime, @function

gmtime:
	lda          #7
	jsr          __enter_res_nomask
	ldx          #0
	jsr          __arg_value2_i0			// time_point
	.loc 11 225 1
	ldx         #%lo(broken_down_time)
	ldy         #%hi(broken_down_time)
	jsr         __pushxy
	jsr         __pushi0
	ldx         #__i1
	ldy          #0
	jsr         gmtime_r
	ldx          #8
	lda          #__i1
	jsr         __load_result_value2
	ldy          #12
	jmp          __leave_nomask
.func_end_gmtime:
	.size gmtime, .func_end_gmtime-gmtime

	.section ".text.localtime", "ax", @progbits
	.global localtime
	.type localtime, @function

localtime:
	lda          #7
	jsr          __enter_res_nomask
	ldx          #0
	jsr          __arg_value2_i0			// time_point
	.loc 11 229 1
	ldx         #%lo(broken_down_time)
	ldy         #%hi(broken_down_time)
	jsr         __pushxy
	jsr         __pushi0
	ldx         #__i1
	ldy          #0
	jsr         localtime_r
	ldx          #8
	lda          #__i1
	jsr         __load_result_value2
	ldy          #12
	jmp          __leave_nomask
.func_end_localtime:
	.size localtime, .func_end_localtime-localtime

	.section ".text.asctime_r", "ax", @progbits
	.global asctime_r
	.type asctime_r, @function

asctime_r:
	lda          #15
	jsr          __enter_res
	.byte        0x0c,0x00,0x00		// Save mask i:12 b:0 l:0 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i4			// buffer
	ldx          #0
	jsr          __arg_value2_i6			// value
	.loc 11 234 49
	.loc 11 237 42
	.loc 11 239 66
	ldx          #1
	lda         __i6
	bne         .asctime_r_label_49
	lda         __i6+1
	beq         .asctime_r_label_50
.asctime_r_label_49:
	dex         
.asctime_r_label_50:
	stx         __b0
	ldx          #5
	jsr          __var_addr_i7			// __invented__53
	lda         __b0
	sta         (__i7)
	lda         __i6
	bne         .asctime_r_label_70
	lda         __i6+1
	beq         .asctime_r_label_99
.asctime_r_label_70:
	ldx          #1
	lda         __i4
	bne         .asctime_r_label_80
	lda         __i4+1
	beq         .asctime_r_label_81
.asctime_r_label_80:
	dex         
.asctime_r_label_81:
	txa         
	sta         (__i7)
.asctime_r_label_99:
	lda         (__i7)
	sta         __b0
	ldx          #6
	jsr          __var_addr_i8			// __invented__54
	lda         __b0
	sta         (__i8)
	bne         .asctime_r_label_153
	ldy          #12
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	ldx          #0
	lda         __i0
	cmp          #0
	lda         __i0+1
	sbc          #0
	bvc         .asctime_r_label_134
	eor          #128
.asctime_r_label_134:
	bpl         .asctime_r_label_132
	inx         
.asctime_r_label_132:
	txa         
	sta         (__i8)
.asctime_r_label_153:
	lda         (__i8)
	sta         __b0
	ldx          #7
	jsr          __var_addr_i9			// __invented__55
	lda         __b0
	sta         (__i9)
	bne         .asctime_r_label_204
	ldy          #12
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	ldx          #0
	lda         __i0
	cmp          #7
	lda         __i0+1
	sbc          #0
	bvc         .asctime_r_label_186
	eor          #128
.asctime_r_label_186:
	bmi         .asctime_r_label_184
	inx         
.asctime_r_label_184:
	txa         
	sta         (__i9)
.asctime_r_label_204:
	lda         (__i9)
	sta         __b0
	ldx          #8
	jsr          __var_addr_i10			// __invented__56
	lda         __b0
	sta         (__i10)
	bne         .asctime_r_label_257
	ldy          #8
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	ldx          #0
	lda         __i0
	cmp          #0
	lda         __i0+1
	sbc          #0
	bvc         .asctime_r_label_239
	eor          #128
.asctime_r_label_239:
	bpl         .asctime_r_label_237
	inx         
.asctime_r_label_237:
	txa         
	sta         (__i10)
.asctime_r_label_257:
	lda         (__i10)
	sta         __b0
	ldx          #9
	jsr          __var_addr_i11			// __invented__57
	lda         __b0
	sta         (__i11)
	bne         .asctime_r_label_308
	ldy          #8
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	ldx          #0
	lda         __i0
	cmp          #12
	lda         __i0+1
	sbc          #0
	bvc         .asctime_r_label_290
	eor          #128
.asctime_r_label_290:
	bmi         .asctime_r_label_288
	inx         
.asctime_r_label_288:
	txa         
	sta         (__i11)
.asctime_r_label_308:
	lda         (__i11)
	beq         .asctime_r_label_344
	.loc 11 240 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #7
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 11 241 1
	stz         __i0
	stz         __i0+1
	ldx          #16
	lda          #__i0
	jsr         __load_result_value2
.asctime_r_label_341:
	ldy          #22
	jmp          __leave
.asctime_r_label_344:
	.loc 11 247 23
	lda         __i6
	sta         __i0
	lda         __i6+1
	sta         __i0+1
	ldy          #10
	lda         (__i0), Y
	sta         __i1
	iny         
	lda         (__i0), Y
	sta         __i1+1
	clc         
	lda         __i1
	adc          #108
	sta         __i2
	lda         __i1+1
	adc          #7
	sta         __i2+1
	jsr          __spill2
	.byte __i2
	.byte 0x0b,0x00
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	iny         
	lda         (__i0), Y
	sta         __i3
	iny         
	lda         (__i0), Y
	sta         __i3+1
	iny         
	lda         (__i0), Y
	sta         __i12
	iny         
	lda         (__i0), Y
	sta         __i12+1
	iny         
	lda         (__i0), Y
	sta         __i13
	iny         
	lda         (__i0), Y
	sta         __i13+1
	iny         
	lda         (__i0), Y
	sta         __i14
	iny         
	lda         (__i0), Y
	sta         __i14+1
	lda         __i14
	asl          A
	sta         __i15
	lda         __i14+1
	rol          A
	sta         __i15+1
	lda         #%lo(.local.months.2547)
	sta         __i14
	lda         #%hi(.local.months.2547)
	sta         __i14+1
	clc         
	lda         __i14
	adc         __i15
	sta         __i2
	lda         __i14+1
	adc         __i15+1
	sta         __i2+1
	lda         (__i2)
	sta         __i14
	ldy          #1
	lda         (__i2), Y
	sta         __i14+1
	ldy          #12
	lda         (__i0), Y
	sta         __i2
	iny         
	lda         (__i0), Y
	sta         __i2+1
	lda         __i2
	asl          A
	sta         __i0
	lda         __i2+1
	rol          A
	sta         __i0+1
	lda         #%lo(.local.week_days.2546)
	sta         __i2
	lda         #%hi(.local.week_days.2546)
	sta         __i2+1
	clc         
	lda         __i2
	adc         __i0
	sta         __i15
	lda         __i2+1
	adc         __i0+1
	sta         __i15+1
	lda         (__i15)
	sta         __i0
	ldy          #1
	lda         (__i15), Y
	sta         __i0+1
	jsr          __reload2
	.byte __i2
	.byte 0x0b,0x00
	jsr         __pushi2
	jsr         __pushi1
	jsr         __pushi3
	jsr         __pushi12
	jsr         __pushi13
	jsr         __pushi14
	jsr         __pushi0
	ldx         #%lo(.str.146)
	ldy         #%hi(.str.146)
	jsr         __pushxy
	ldx          #26
	jsr         __pushxy0
	jsr         __pushi4
	ldx         #__i12
	ldy          #0
	jsr         __snprintf_int
	lda          #20
	sta         __t0
	jsr         __incsp
	lda         __i12
	sta         __i5
	lda         __i12+1
	sta         __i5+1
	.loc 11 248 33
	ldx          #0
	lda         __i12
	cmp          #0
	lda         __i12+1
	sbc          #0
	bvc         .asctime_r_label_561
	eor          #128
.asctime_r_label_561:
	bpl         .asctime_r_label_559
	inx         
.asctime_r_label_559:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i0			// __invented__58
	lda         __b0
	sta         (__i0)
	lda         __i12
	cmp          #0
	lda         __i12+1
	sbc          #0
	bvc         .asctime_r_label_580
	eor          #128
.asctime_r_label_580:
	bmi         .asctime_r_label_610
	ldx          #0
	lda         __i5
	cmp          #26
	lda         __i5+1
	sbc          #0
	bvc         .asctime_r_label_592
	eor          #128
.asctime_r_label_592:
	bmi         .asctime_r_label_590
	inx         
.asctime_r_label_590:
	txa         
	sta         (__i0)
.asctime_r_label_610:
	lda         (__i0)
	beq         .asctime_r_label_644
	.loc 11 249 1
	lda          #214
	sta         __i1
	lda          #3
	sta         __i1+1
	lda          #32
	ldy          #0
	sta         (__i1)
	tya         
	iny         
	sta         (__i1), Y
	.loc 11 250 1
	stz         __i1
	stz         __i1+1
	ldx          #16
	lda          #__i1
	jsr         __load_result_value2
	jmp         .asctime_r_label_341
.asctime_r_label_644:
	.loc 11 252 1
	ldx          #16
	lda          #__i4
	jsr         __load_result_value2
	jmp         .asctime_r_label_341
.func_end_asctime_r:
	.size asctime_r, .func_end_asctime_r-asctime_r

	.section ".text.asctime", "ax", @progbits
	.global asctime
	.type asctime, @function

asctime:
	lda          #7
	jsr          __enter_res_nomask
	ldx          #0
	jsr          __arg_value2_i0			// value
	.loc 11 256 1
	ldx         #%lo(formatted_time)
	ldy         #%hi(formatted_time)
	jsr         __pushxy
	jsr         __pushi0
	ldx         #__i1
	ldy          #0
	jsr         asctime_r
	ldx          #8
	lda          #__i1
	jsr         __load_result_value2
	ldy          #12
	jmp          __leave_nomask
.func_end_asctime:
	.size asctime, .func_end_asctime-asctime

	.section ".text.ctime_r", "ax", @progbits
	.global ctime_r
	.type ctime_r, @function

ctime_r:
	lda          #32
	jsr          __enter_res
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i4			// time_point
	ldx          #2
	jsr          __arg_value2_i5			// buffer
	.loc 11 261 80
	ldx          #1
	lda         __i4
	bne         .ctime_r_label_22
	lda         __i4+1
	beq         .ctime_r_label_23
.ctime_r_label_22:
	dex         
.ctime_r_label_23:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i6			// __invented__62
	lda         __b0
	sta         (__i6)
	lda         __i4
	bne         .ctime_r_label_43
	lda         __i4+1
	beq         .ctime_r_label_83
.ctime_r_label_43:
	ldx          #28
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         localtime_r
	ldx          #1
	lda         __i0
	bne         .ctime_r_label_64
	lda         __i0+1
	beq         .ctime_r_label_65
.ctime_r_label_64:
	dex         
.ctime_r_label_65:
	txa         
	sta         (__i6)
.ctime_r_label_83:
	lda         (__i6)
	beq         .ctime_r_label_105
	.loc 11 262 1
	stz         __i0
	stz         __i0+1
	ldx          #33
	lda          #__i0
	jsr         __load_result_value2
.ctime_r_label_102:
	ldy          #39
	jmp          __leave
.ctime_r_label_105:
	.loc 11 264 1
	jsr         __pushi5
	ldx          #28
	lda          #__i0
	jsr          __var_addr_push_i0
	ldx         #__i0
	ldy          #0
	jsr         asctime_r
	ldx          #33
	lda          #__i0
	jsr         __load_result_value2
	bra         .ctime_r_label_102
.func_end_ctime_r:
	.size ctime_r, .func_end_ctime_r-ctime_r

	.section ".text.ctime", "ax", @progbits
	.global ctime
	.type ctime, @function

ctime:
	lda          #7
	jsr          __enter_res_nomask
	ldx          #0
	jsr          __arg_value2_i0			// time_point
	.loc 11 268 1
	ldx         #%lo(formatted_time)
	ldy         #%hi(formatted_time)
	jsr         __pushxy
	jsr         __pushi0
	ldx         #__i1
	ldy          #0
	jsr         ctime_r
	ldx          #8
	lda          #__i1
	jsr         __load_result_value2
	ldy          #12
	jmp          __leave_nomask
.func_end_ctime:
	.size ctime, .func_end_ctime-ctime

	.section ".text.difftime", "ax", @progbits
	.global difftime
	.type difftime, @function

difftime:
	lda          #5
	jsr          __enter_leaf_res
	.byte        0x00,0x00,0x02		// Save mask i:0 b:0 l:0 x:0 f:2 
	ldx          #0
	jsr          __arg_value4_l0			// later
	ldx          #4
	jsr          __arg_value4_l1			// earlier
	.loc 11 272 1
	lda          #__f0
	ldx          #__l0
	jsr         __i4tof
	lda          #__f1
	ldx          #__l1
	jsr         __i4tof
	lda          #__f2
	ldx          #__f0
	ldy          #__f1
	jsr         __fsub
	lda         #__f2
	jsr         __result4
	ldy          #16
	jmp          __leave_leaf
.func_end_difftime:
	.size difftime, .func_end_difftime-difftime

	.section ".text.mktime", "ax", @progbits
	.global mktime
	.type mktime, @function

mktime:
	lda          #91
	jsr          __enter_res
	.byte        0x03,0x60,0x00		// Save mask i:3 b:0 l:0 x:3 f:0 
	ldx          #0
	jsr          __arg_value2_i4			// value
	.loc 11 281 26
	lda         __i4
	bne         .mktime_label_89
	lda         __i4+1
	bne         .mktime_label_89
	.loc 11 282 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #7
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 11 283 1
	ldx          #3
.mktime_label_76:
	lda         .lit.246, X
	sta         __l0, X
	dex         
	bpl         .mktime_label_76
	ldx          #92
	jsr          __load_result
	lda         #__l0
	jsr         __result4
.mktime_label_86:
	ldy          #96
	jmp          __leave
.mktime_label_89:
	.loc 11 285 1
	lda         __i4
	sta         __i0
	lda         __i4+1
	sta         __i0+1
	ldy          #10
	lda         (__i0), Y
	sta         __i1
	iny         
	lda         (__i0), Y
	sta         __i1+1
	lda         __i1
	sta         __x0
	lda         __i1+1
	sta         __x0+1
	and          #128
	beq         .mktime_label_118
	lda          #255
.mktime_label_118:
	ldy          #7
.mktime_label_124:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .mktime_label_124
	clc         
	ldy          #8
	ldx          #0
.mktime_label_137:
	lda         __x0, X
	adc         .lit.219, X
	sta         __x2, X
	inx         
	dey         
	bne         .mktime_label_137
	jsr          __spill8
	.byte __x2
	.byte 0x3f,0x00
	.loc 11 286 1
	ldy          #8
	lda         (__i0), Y
	sta         __i1
	iny         
	lda         (__i0), Y
	sta         __i1+1
	lda         __i1
	sta         __x0
	lda         __i1+1
	sta         __x0+1
	and          #128
	beq         .mktime_label_164
	lda          #255
.mktime_label_164:
	ldy          #7
.mktime_label_168:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .mktime_label_168
	jsr          __spill8
	.byte __x0
	.byte 0x37,0x00
	.loc 11 287 1
	ldx          #7
.mktime_label_180:
	lda         .lit.221, X
	sta         __x3, X
	dex         
	bpl         .mktime_label_180
	jsr          __reload8
	.byte __x2
	.byte 0x37,0x00
	lda          #__x0
	ldx          #__x2
	ldy          #__x3
	jsr         __sdiv8
	jsr          __reload8
	.byte __x2
	.byte 0x3f,0x00
	clc         
	ldy          #8
	ldx          #0
.mktime_label_201:
	lda         __x2, X
	adc         __x0, X
	sta         __x3, X
	inx         
	dey         
	bne         .mktime_label_201
	ldx          #7
.mktime_label_211:
	lda         __x3, X
	sta         __x1, X
	dex         
	bpl         .mktime_label_211
	.loc 11 288 1
	ldx          #7
.mktime_label_222:
	lda         .lit.221, X
	sta         __x0, X
	dex         
	bpl         .mktime_label_222
	jsr          __reload8
	.byte __x2
	.byte 0x37,0x00
	lda          #__x3
	ldx          #__x2
	ldy          #__x0
	jsr         __smod8
	lda          #__x3
	ldx          #11
	jsr         __set_var_value8
	.loc 11 289 16
	lda         __x3
	cmp          #0
	lda         __x3+1
	sbc          #0
	lda         __x3+2
	sbc          #0
	lda         __x3+3
	sbc          #0
	lda         __x3+4
	sbc          #0
	lda         __x3+5
	sbc          #0
	lda         __x3+6
	sbc          #0
	lda         __x3+7
	sbc          #0
	bvc         .mktime_label_244
	eor          #128
.mktime_label_244:
	bpl         .mktime_label_294
	.loc 11 290 1
	ldx          #11
	jsr          __var_value8_x0			// month
	clc         
	ldy          #8
	ldx          #0
.mktime_label_278:
	lda         __x0, X
	adc         .lit.221, X
	sta         __x2, X
	inx         
	dey         
	bne         .mktime_label_278
	lda          #__x2
	ldx          #11
	jsr         __set_var_value8
	.loc 11 291 1
	lda          #__x1
	jsr         __rdecd
.mktime_label_294:
	.loc 11 293 1
	ldx          #11
	jsr          __var_value8_x0			// month
	lda         __x0
	sta         __i0
	lda         __x0+7
	and          #128
	ora         __x0+1
	sta         __i0+1
	clc         
	lda         __i0
	adc          #1
	sta         __i1
	lda         __i0+1
	adc          #0
	sta         __i1+1
	ldx          #1
	jsr         __pushxy0
	jsr         __pushi1
	jsr         __pushx1
	ldx         #__x0
	ldy          #0
	jsr         DaysFromCivil
	ldx          #7
.mktime_label_334:
	lda         .lit.229, X
	sta         __x2, X
	dex         
	bpl         .mktime_label_334
	lda          #__x3
	ldx          #__x0
	ldy          #__x2
	jsr         __smul8
	jsr          __spill8
	.byte __x3
	.byte 0x47,0x00
	lda         __i4
	sta         __i0
	lda         __i4+1
	sta         __i0+1
	ldy          #6
	lda         (__i0), Y
	sta         __i1
	iny         
	lda         (__i0), Y
	sta         __i1+1
	sec         
	lda         __i1
	sbc          #1
	sta         __i2
	lda         __i1+1
	sbc          #0
	sta         __i2+1
	lda         __i2
	sta         __x0
	lda         __i2+1
	sta         __x0+1
	and          #128
	beq         .mktime_label_382
	lda          #255
.mktime_label_382:
	ldy          #7
.mktime_label_386:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .mktime_label_386
	ldx          #7
.mktime_label_397:
	lda         .lit.229, X
	sta         __x2, X
	dex         
	bpl         .mktime_label_397
	lda          #__x3
	ldx          #__x0
	ldy          #__x2
	jsr         __smul8
	jsr          __reload8
	.byte __x2
	.byte 0x47,0x00
	clc         
	ldy          #8
	ldx          #0
.mktime_label_417:
	lda         __x2, X
	adc         __x3, X
	sta         __x0, X
	inx         
	dey         
	bne         .mktime_label_417
	jsr          __spill8
	.byte __x0
	.byte 0x4f,0x00
	ldy          #4
	lda         (__i0), Y
	sta         __i1
	iny         
	lda         (__i0), Y
	sta         __i1+1
	lda         __i1
	sta         __x3
	lda         __i1+1
	sta         __x3+1
	and          #128
	beq         .mktime_label_442
	lda          #255
.mktime_label_442:
	ldy          #7
.mktime_label_446:
	sta         __x3, Y
	dey         
	cpy          #1
	bne         .mktime_label_446
	ldx          #7
.mktime_label_457:
	lda         .lit.232, X
	sta         __x2, X
	dex         
	bpl         .mktime_label_457
	lda          #__x0
	ldx          #__x3
	ldy          #__x2
	jsr         __smul8
	jsr          __reload8
	.byte __x3
	.byte 0x4f,0x00
	clc         
	ldy          #8
	ldx          #0
.mktime_label_477:
	lda         __x3, X
	adc         __x0, X
	sta         __x2, X
	inx         
	dey         
	bne         .mktime_label_477
	jsr          __spill8
	.byte __x2
	.byte 0x57,0x00
	ldy          #2
	lda         (__i0), Y
	sta         __i1
	iny         
	lda         (__i0), Y
	sta         __i1+1
	lda         __i1
	sta         __x0
	lda         __i1+1
	sta         __x0+1
	and          #128
	beq         .mktime_label_502
	lda          #255
.mktime_label_502:
	ldy          #7
.mktime_label_506:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .mktime_label_506
	ldx          #7
.mktime_label_517:
	lda         .lit.234, X
	sta         __x3, X
	dex         
	bpl         .mktime_label_517
	lda          #__x2
	ldx          #__x0
	ldy          #__x3
	jsr         __smul8
	jsr          __reload8
	.byte __x3
	.byte 0x57,0x00
	clc         
	ldy          #8
	ldx          #0
.mktime_label_537:
	lda         __x3, X
	adc         __x2, X
	sta         __x0, X
	inx         
	dey         
	bne         .mktime_label_537
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	lda         __i1
	sta         __x2
	lda         __i1+1
	sta         __x2+1
	and          #128
	beq         .mktime_label_561
	lda          #255
.mktime_label_561:
	ldy          #7
.mktime_label_565:
	sta         __x2, Y
	dey         
	cpy          #1
	bne         .mktime_label_565
	clc         
	ldy          #8
	ldx          #0
.mktime_label_577:
	lda         __x0, X
	adc         __x2, X
	sta         __x3, X
	inx         
	dey         
	bne         .mktime_label_577
	lda          #__x3
	ldx          #19
	jsr         __set_var_value8
	.loc 11 297 1
	lda         __x3
	sta         __l0
	lda         __x3+1
	sta         __l0+1
	lda         __x3+2
	sta         __l0+2
	lda         __x3+7
	and          #128
	ora         __x3+3
	sta         __l0+3
	lda          #__l0
	ldx          #23
	jsr         __set_var_value4
	.loc 11 298 33
	lda         __l0
	sta         __x0
	lda         __l0+1
	sta         __x0+1
	lda         __l0+2
	sta         __x0+2
	lda         __l0+3
	sta         __x0+3
	and          #128
	beq         .mktime_label_624
	lda          #255
.mktime_label_624:
	ldy          #7
.mktime_label_628:
	sta         __x0, Y
	dey         
	cpy          #3
	bne         .mktime_label_628
	lda         __x0
	cmp         __x3
	bne         .mktime_label_635
	lda         __x0+1
	cmp         __x3+1
	bne         .mktime_label_635
	lda         __x0+2
	cmp         __x3+2
	bne         .mktime_label_635
	lda         __x0+3
	cmp         __x3+3
	bne         .mktime_label_635
	lda         __x0+4
	cmp         __x3+4
	bne         .mktime_label_635
	lda         __x0+5
	cmp         __x3+5
	bne         .mktime_label_635
	lda         __x0+6
	cmp         __x3+6
	bne         .mktime_label_635
	lda         __x0+7
	cmp         __x3+7
	beq         .mktime_label_689
.mktime_label_635:
	.loc 11 299 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #32
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 11 300 1
	ldx          #3
.mktime_label_680:
	lda         .lit.246, X
	sta         __l0, X
	dex         
	bpl         .mktime_label_680
	ldx          #92
	jsr          __load_result
	lda         #__l0
	jsr         __result4
	jmp         .mktime_label_86
.mktime_label_689:
	.loc 11 303 54
	ldx          #47
	lda          #__i5
	jsr          __var_addr_push_i5
	ldx          #23
	lda          #__i6
	jsr          __var_addr_push_i6
	ldx         #__i0
	ldy          #0
	jsr         localtime_r
	lda         __i0
	bne         .mktime_label_706
	lda         __i0+1
	bne         .mktime_label_937
	jmp         .mktime_label_869
.mktime_label_937:
.mktime_label_706:
	.loc 11 304 1
	ldy          #21
	ldx          #3
.mktime_label_723:
	lda         (__i5), Y
	sta         __l0, X
	dey         
	dex         
	bpl         .mktime_label_723
	lda         __l0
	sta         __x0
	lda         __l0+1
	sta         __x0+1
	lda         __l0+2
	sta         __x0+2
	lda         __l0+3
	sta         __x0+3
	and          #128
	beq         .mktime_label_741
	lda          #255
.mktime_label_741:
	ldy          #7
.mktime_label_745:
	sta         __x0, Y
	dey         
	cpy          #3
	bne         .mktime_label_745
	ldx          #19
	jsr          __var_value8_x2			// seconds
	sec         
	ldy          #8
	ldx          #0
.mktime_label_759:
	lda         __x2, X
	sbc         __x0, X
	sta         __x3, X
	inx         
	dey         
	bne         .mktime_label_759
	lda          #__x3
	ldx          #19
	jsr         __set_var_value8
	.loc 11 305 1
	lda         __x3
	sta         __l0
	lda         __x3+1
	sta         __l0+1
	lda         __x3+2
	sta         __l0+2
	lda         __x3+7
	and          #128
	ora         __x3+3
	sta         __l0+3
	lda          #__l0
	ldx          #23
	jsr         __set_var_value4
	.loc 11 306 33
	lda         __l0
	sta         __x0
	lda         __l0+1
	sta         __x0+1
	lda         __l0+2
	sta         __x0+2
	lda         __l0+3
	sta         __x0+3
	and          #128
	beq         .mktime_label_803
	lda          #255
.mktime_label_803:
	ldy          #7
.mktime_label_807:
	sta         __x0, Y
	dey         
	cpy          #3
	bne         .mktime_label_807
	lda         __x0
	cmp         __x3
	bne         .mktime_label_814
	lda         __x0+1
	cmp         __x3+1
	bne         .mktime_label_814
	lda         __x0+2
	cmp         __x3+2
	bne         .mktime_label_814
	lda         __x0+3
	cmp         __x3+3
	bne         .mktime_label_814
	lda         __x0+4
	cmp         __x3+4
	bne         .mktime_label_814
	lda         __x0+5
	cmp         __x3+5
	bne         .mktime_label_814
	lda         __x0+6
	cmp         __x3+6
	bne         .mktime_label_814
	lda         __x0+7
	cmp         __x3+7
	beq         .mktime_label_868
.mktime_label_814:
	.loc 11 307 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #32
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 11 308 1
	ldx          #3
.mktime_label_859:
	lda         .lit.246, X
	sta         __l0, X
	dex         
	bpl         .mktime_label_859
	ldx          #92
	jsr          __load_result
	lda         #__l0
	jsr         __result4
	jmp         .mktime_label_86
.mktime_label_868:
.mktime_label_869:
	.loc 11 312 54
	jsr         __pushi5
	jsr         __pushi6
	ldx         #__i0
	ldy          #0
	jsr         localtime_r
	lda         __i0
	bne         .mktime_label_908
	lda         __i0+1
	bne         .mktime_label_908
	.loc 11 312 54
	ldx          #3
.mktime_label_899:
	lda         .lit.246, X
	sta         __l0, X
	dex         
	bpl         .mktime_label_899
	ldx          #92
	jsr          __load_result
	lda         #__l0
	jsr         __result4
	jmp         .mktime_label_86
.mktime_label_908:
	.loc 11 313 1
	jsr         __copymem1
	.byte        __i4, __i5, 24
	.loc 11 314 1
	ldx          #23
	jsr          __var_value4_l0			// result
	ldx          #92
	jsr          __load_result
	lda         #__l0
	jsr         __result4
	jmp         .mktime_label_86
.func_end_mktime:
	.size mktime, .func_end_mktime-mktime

	.section ".text.AppendText", "ax", @progbits
	.local  AppendText
	.type AppendText, @function

AppendText:
	lda          #5
	jsr          __enter_leaf_res
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
	ldx          #6
	jsr          __arg_value2_i0			// text
	ldx          #4
	jsr          __arg_value2_i1			// length
	ldx          #2
	jsr          __arg_value2_i2			// capacity
	ldx          #0
	jsr          __arg_value2_i3			// output
	.loc 11 319 23
.AppendText_label_24:
	lda         (__i0)
	sta         __i4
	stz         __i4+1
	beq         .AppendText_label_134
	.loc 11 320 30
	lda         (__i1)
	sta         __i4
	ldy          #1
	lda         (__i1), Y
	sta         __i4+1
	clc         
	lda         __i4
	adc          #1
	sta         __i5
	lda         __i4+1
	adc          #0
	sta         __i5+1
	cmp         __i2+1
	bcc         .AppendText_label_81
	bne         .AppendText_label_61
	lda         __i5
	cmp         __i2
	bcc         .AppendText_label_81
.AppendText_label_61:
	.loc 11 320 30
	stz         __i4
	stz         __i4+1
	lda         #__i4
	jsr         __result2
.AppendText_label_78:
	ldy          #16
	jmp          __leave_leaf
.AppendText_label_81:
	.loc 11 321 1
	lda         __i0
	sta         __i4
	lda         __i0+1
	sta         __i4+1
	lda          #__i0
	jsr         __rinc21
	lda         (__i4)
	sta         __b0
	lda         __i1
	sta         __i4
	lda         __i1+1
	sta         __i4+1
	lda         (__i4)
	sta         __i5
	ldy          #1
	lda         (__i4), Y
	sta         __i5+1
	lda          #__i4
	jsr         __inc21
	clc         
	lda         __i3
	adc         __i5
	sta         __i4
	lda         __i3+1
	adc         __i5+1
	sta         __i4+1
	lda         __b0
	sta         (__i4)
	bra         .AppendText_label_24
.AppendText_label_134:
	.loc 11 323 1
	lda          #1
	sta         __i4
	dec          A
	stz         __i4+1
	lda         #__i4
	jsr         __result2
	bra         .AppendText_label_78
.func_end_AppendText:
	.size AppendText, .func_end_AppendText-AppendText

	.section ".text.AppendNumber", "ax", @progbits
	.local  AppendNumber
	.type AppendNumber, @function

AppendNumber:
	lda          #41
	jsr          __enter_leaf_res
	.byte        0x05,0x40,0x00		// Save mask i:5 b:0 l:0 x:2 f:0 
	ldx          #0
	jsr          __arg_value2_i0			// output
	ldx          #6
	jsr          __arg_value2_i2			// value
	ldx          #10
	jsr          __arg_value1_b0			// padding
	.loc 11 329 13
	stz         __i4
	stz         __i4+1
	lda          #__i4
	ldx          #39
	jsr         __set_var_value2
	.loc 11 331 25
	ldx          #0
	lda         __i2
	cmp          #0
	lda         __i2+1
	sbc          #0
	bvc         .AppendNumber_label_61
	eor          #128
.AppendNumber_label_61:
	bpl         .AppendNumber_label_59
	inx         
.AppendNumber_label_59:
	txa         
	sta         __i4
	stz         __i4+1
	sta         __i1
	lda         __i4+1
	sta         __i1+1
	.loc 11 332 1
	lda         __i4
	ora         __i4+1
	beq         .AppendNumber_label_132
	clc         
	lda         __i2
	adc          #1
	sta         __i4
	lda         __i2+1
	adc          #0
	sta         __i4+1
	sec         
	lda          #0
	sbc         __i4
	sta         __i5
	lda          #0
	sbc         __i4+1
	sta         __i5+1
	clc         
	lda         __i5
	adc          #1
	sta         __i4
	lda         __i5+1
	adc          #0
	sta         __i4+1
	ldx          #5
	jsr          __var_addr_i5			// __invented__87
	lda         __i4
	sta         (__i5)
	lda         __i4+1
	ldy          #1
	sta         (__i5), Y
	bra         .AppendNumber_label_142
.AppendNumber_label_132:
	ldx          #5
	jsr          __var_addr_i4			// __invented__87
	lda         __i2
	sta         (__i4)
	lda         __i2+1
	ldy          #1
	sta         (__i4), Y
.AppendNumber_label_142:
	ldx          #5
	jsr          __var_addr_i4			// __invented__87
	lda         (__i4)
	sta         __i5
	ldy          #1
	lda         (__i4), Y
	sta         __i5+1
	lda         __i5
	sta         __i3
	lda         __i5+1
	sta         __i3+1
	.loc 11 337 3
.AppendNumber_label_161:
	.loc 11 335 1
	lda         __i3
	sta         __i4
	lda         __i3+1
	sta         __i4+1
	lda          #10
	sta         __i5
	lda          #0
	stz         __i5+1
	lda          #__i6
	ldx          #__i4
	ldy          #__i5
	jsr         __umod2
	clc         
	lda          #48
	adc         __i6
	sta         __i5
	lda          #0
	adc         __i6+1
	sta         __i5+1
	lda         __i5
	sta         __b1
	ldx          #39
	jsr          __var_value2_i5			// used
	ldx          #39
	jsr          __var_addr_i6			// used
	lda          #__i6
	jsr         __inc21
	ldx          #37
	jsr          __var_addr_i7			// buffer
	clc         
	lda         __i7
	adc         __i5
	sta         __i8
	lda         __i7+1
	adc         __i5+1
	sta         __i8+1
	lda         __b1
	sta         (__i8)
	.loc 11 336 1
	lda         __i4
	sta         __x0
	lda         __i4+1
	sta         __x0+1
	lda          #0
	ldy          #7
.AppendNumber_label_238:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .AppendNumber_label_238
	ldx          #7
.AppendNumber_label_250:
	lda         .lit.264, X
	sta         __x1, X
	dex         
	bpl         .AppendNumber_label_250
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __umul8
	lda         __x2+4
	sta         __x0
	lda         __x2+5
	sta         __x0+1
	lda         __x2+6
	sta         __x0+2
	lda         __x2+7
	sta         __x0+3
	lda          #0
	stz         __x0+4
	stz         __x0+5
	stz         __x0+6
	sta         __x0+7
	ldx          #3
.AppendNumber_label_286:
	lsr         __x0+7
	ror         __x0+6
	ror         __x0+5
	ror         __x0+4
	ror         __x0+3
	ror         __x0+2
	ror         __x0+1
	ror         __x0
	dex         
	bne         .AppendNumber_label_286
	lda         __x0
	sta         __i4
	lda         __x0+1
	sta         __i4+1
	lda         __i4
	sta         __i3
	lda         __i4+1
	sta         __i3+1
	lda         __i3
	ora         __i3+1
	beq         .AppendNumber_label_523
	jmp         .AppendNumber_label_161
.AppendNumber_label_523:
	.loc 11 338 15
	lda         __i1
	ora         __i1+1
	beq         .AppendNumber_label_346
	.loc 11 338 15
	ldx          #39
	jsr          __var_value2_i1			// used
	lda          #__i6
	jsr         __inc21
	clc         
	lda         __i7
	adc         __i1
	sta         __i3
	lda         __i7+1
	adc         __i1+1
	sta         __i3+1
	lda          #45
	ldy          #0
	sta         (__i3)
.AppendNumber_label_346:
	.loc 11 339 22
.AppendNumber_label_348:
	ldx          #39
	jsr          __var_value2_i1			// used
	ldx          #8
	jsr          __arg_value2_i3			// width
	lda         __i1
	cmp         __i3
	lda         __i1+1
	sbc         __i3+1
	bvc         .AppendNumber_label_355
	eor          #128
.AppendNumber_label_355:
	bpl         .AppendNumber_label_389
	.loc 11 339 22
	ldx          #39
	jsr          __var_value2_i1			// used
	ldx          #39
	jsr          __var_addr_i3			// used
	lda          #__i3
	jsr         __inc21
	ldx          #37
	jsr          __var_addr_i3			// buffer
	clc         
	lda         __i3
	adc         __i1
	sta         __i4
	lda         __i3+1
	adc         __i1+1
	sta         __i4+1
	lda         __b0
	sta         (__i4)
	bra         .AppendNumber_label_348
.AppendNumber_label_389:
	.loc 11 340 19
.AppendNumber_label_391:
	ldx          #39
	jsr          __var_value2_i1			// used
	lda         __i1
	ora         __i1+1
	bne         .AppendNumber_label_525
	jmp         .AppendNumber_label_512
.AppendNumber_label_525:
	.loc 11 341 30
	ldx          #4
	jsr          __arg_value2_i1			// length
	lda         (__i1)
	sta         __i3
	ldy          #1
	lda         (__i1), Y
	sta         __i3+1
	clc         
	lda         __i3
	adc          #1
	sta         __i1
	lda         __i3+1
	adc          #0
	sta         __i1+1
	ldx          #2
	jsr          __arg_value2_i3			// capacity
	lda         __i1+1
	cmp         __i3+1
	bcc         .AppendNumber_label_445
	bne         .AppendNumber_label_425
	lda         __i1
	cmp         __i3
	bcc         .AppendNumber_label_445
.AppendNumber_label_425:
	.loc 11 341 30
	stz         __i1
	stz         __i1+1
	lda         #__i1
	jsr         __result2
.AppendNumber_label_442:
	ldy          #56
	jmp          __leave_leaf
.AppendNumber_label_445:
	.loc 11 342 1
	ldx          #39
	jsr          __var_addr_i1			// used
	lda          #__i1
	jsr         __dec21
	lda         (__i1)
	sta         __i3
	ldy          #1
	lda         (__i1), Y
	sta         __i3+1
	ldx          #37
	jsr          __var_addr_i1			// buffer
	clc         
	lda         __i1
	adc         __i3
	sta         __i4
	lda         __i1+1
	adc         __i3+1
	sta         __i4+1
	lda         (__i4)
	sta         __b1
	ldx          #4
	jsr          __arg_value2_i1			// length
	lda         (__i1)
	sta         __i3
	ldy          #1
	lda         (__i1), Y
	sta         __i3+1
	lda          #__i1
	jsr         __inc21
	clc         
	lda         __i0
	adc         __i3
	sta         __i1
	lda         __i0+1
	adc         __i3+1
	sta         __i1+1
	lda         __b1
	sta         (__i1)
	jmp         .AppendNumber_label_391
.AppendNumber_label_512:
	.loc 11 344 1
	lda          #1
	sta         __i1
	dec          A
	stz         __i1+1
	lda         #__i1
	jsr         __result2
	bra         .AppendNumber_label_442
.func_end_AppendNumber:
	.size AppendNumber, .func_end_AppendNumber-AppendNumber

	.section ".text.strftime", "ax", @progbits
	.global strftime
	.type strftime, @function

strftime:
	lda          #102
	jsr          __enter_res
	.byte        0x4c,0x40,0x00		// Save mask i:12 b:2 l:0 x:2 f:0 
	ldx          #0
	jsr          __arg_value2_i4			// output
	ldx          #4
	jsr          __arg_value2_i5			// format
	ldx          #6
	jsr          __arg_value2_i6			// value
	ldx          #2
	jsr          __arg_value2_i7			// capacity
	.loc 11 351 34
	.loc 11 354 66
	.loc 11 355 18
	stz         __i0
	stz         __i0+1
	lda          #__i0
	ldx          #88
	jsr         __set_var_value2
	.loc 11 356 91
	ldx          #1
	lda         __i4
	bne         .strftime_label_210
	lda         __i4+1
	beq         .strftime_label_211
.strftime_label_210:
	dex         
.strftime_label_211:
	stx         __b0
	ldx          #5
	jsr          __var_addr_i8			// __invented__93
	lda         __b0
	sta         (__i8)
	lda         __i4
	bne         .strftime_label_229
	lda         __i4+1
	beq         .strftime_label_258
.strftime_label_229:
	ldx          #1
	lda         __i5
	bne         .strftime_label_239
	lda         __i5+1
	beq         .strftime_label_240
.strftime_label_239:
	dex         
.strftime_label_240:
	txa         
	sta         (__i8)
.strftime_label_258:
	lda         (__i8)
	sta         __b0
	ldx          #6
	jsr          __var_addr_i9			// __invented__94
	lda         __b0
	sta         (__i9)
	bne         .strftime_label_299
	ldx          #1
	lda         __i6
	bne         .strftime_label_280
	lda         __i6+1
	beq         .strftime_label_281
.strftime_label_280:
	dex         
.strftime_label_281:
	txa         
	sta         (__i9)
.strftime_label_299:
	lda         (__i9)
	sta         __b0
	ldx          #7
	jsr          __var_addr_i10			// __invented__95
	lda         __b0
	sta         (__i10)
	bne         .strftime_label_338
	ldx          #1
	lda         __i7
	ora         __i7+1
	beq         .strftime_label_322
	dex         
.strftime_label_322:
	txa         
	sta         (__i10)
.strftime_label_338:
	lda         (__i10)
	beq         .strftime_label_360
	.loc 11 357 1
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
.strftime_label_357:
	ldy          #113
	jmp          __leave
.strftime_label_360:
	.loc 11 359 25
.strftime_label_362:
	lda         (__i5)
	sta         __i0
	stz         __i0+1
	bne         .strftime_label_4417
	jmp         .strftime_label_4381
.strftime_label_4417:
	.loc 11 362 21
	lda         (__i5)
	sta         __i0
	stz         __i0+1
	cmp          #37
	bne         .strftime_label_392
	lda         __i0+1
	beq         .strftime_label_473
.strftime_label_392:
	.loc 11 363 29
	ldx          #88
	jsr          __var_value2_i0			// length
	clc         
	lda         __i0
	adc          #1
	sta         __i1
	lda         __i0+1
	adc          #0
	sta         __i1+1
	cmp         __i7+1
	bcc         .strftime_label_430
	bne         .strftime_label_414
	lda         __i1
	cmp         __i7
	bcc         .strftime_label_430
.strftime_label_414:
	.loc 11 363 29
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	bra         .strftime_label_357
.strftime_label_430:
	.loc 11 364 1
	lda         __i5
	sta         __i0
	lda         __i5+1
	sta         __i0+1
	lda          #__i5
	jsr         __rinc21
	lda         (__i0)
	sta         __b0
	ldx          #88
	jsr          __var_value2_i0			// length
	ldx          #88
	jsr          __var_addr_i1			// length
	lda          #__i1
	jsr         __inc21
	clc         
	lda         __i4
	adc         __i0
	sta         __i1
	lda         __i4+1
	adc         __i0+1
	sta         __i1+1
	lda         __b0
	sta         (__i1)
	.loc 11 365 1
	jmp         .strftime_label_362
.strftime_label_473:
	.loc 11 367 1
	lda          #__i5
	jsr         __rinc21
	.loc 11 368 1
	lda         __i5
	sta         __i0
	lda         __i5+1
	sta         __i0+1
	lda          #__i5
	jsr         __rinc21
	lda         (__i0)
	sta         __b0
	sta         __b2
	.loc 11 369 24
	lda         __b0
	sta         __i0
	stz         __i0+1
	cmp          #37
	bne         .strftime_label_568
	lda         __i0+1
	bne         .strftime_label_568
	.loc 11 370 29
	ldx          #88
	jsr          __var_value2_i0			// length
	clc         
	lda         __i0
	adc          #1
	sta         __i1
	lda         __i0+1
	adc          #0
	sta         __i1+1
	cmp         __i7+1
	bcc         .strftime_label_543
	bne         .strftime_label_527
	lda         __i1
	cmp         __i7
	bcc         .strftime_label_543
.strftime_label_527:
	.loc 11 370 29
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_543:
	.loc 11 371 1
	ldx          #88
	jsr          __var_value2_i0			// length
	ldx          #88
	jsr          __var_addr_i1			// length
	lda          #__i1
	jsr         __inc21
	clc         
	lda         __i4
	adc         __i0
	sta         __i1
	lda         __i4+1
	adc         __i0+1
	sta         __i1+1
	lda          #37
	ldy          #0
	sta         (__i1)
	jmp         .strftime_label_4379
.strftime_label_568:
	.loc 11 372 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #89
	bne         .strftime_label_661
	lda         __i0+1
	bne         .strftime_label_661
	.loc 11 374 33
	ldy          #10
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	clc         
	lda         __i0
	adc          #108
	sta         __i1
	lda         __i0+1
	adc          #7
	sta         __i1+1
	lda          #48
	jsr         __pusha
	ldx          #4
	jsr         __pushxy0
	jsr         __pushi1
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_629
	dex         
.strftime_label_629:
	txa         
	cmp          #0
	beq         .strftime_label_640
	lda          #255
.strftime_label_640:
	inc          A
	beq         .strftime_label_659
	.loc 11 374 33
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_659:
	jmp         .strftime_label_4378
.strftime_label_661:
	.loc 11 375 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #121
	bne         .strftime_label_766
	lda         __i0+1
	bne         .strftime_label_766
	.loc 11 377 41
	ldy          #10
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	clc         
	lda         __i0
	adc          #108
	sta         __i1
	lda         __i0+1
	adc          #7
	sta         __i1+1
	lda          #100
	sta         __i0
	lda          #0
	stz         __i0+1
	lda          #__i2
	ldx          #__i1
	ldy          #__i0
	jsr         __smod2
	lda          #48
	jsr         __pusha
	ldx          #2
	jsr         __pushxy0
	jsr         __pushi2
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_735
	dex         
.strftime_label_735:
	txa         
	cmp          #0
	beq         .strftime_label_746
	lda          #255
.strftime_label_746:
	inc          A
	beq         .strftime_label_764
	.loc 11 377 41
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_764:
	jmp         .strftime_label_4377
.strftime_label_766:
	.loc 11 378 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #67
	beq         .strftime_label_4419
	jmp         .strftime_label_979
.strftime_label_4419:
	lda         __i0+1
	beq         .strftime_label_4421
	jmp         .strftime_label_979
.strftime_label_4421:
	.loc 11 380 41
	ldy          #10
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	clc         
	lda         __i0
	adc          #108
	sta         __i1
	lda         __i0+1
	adc          #7
	sta         __i1+1
	lda         __i1
	sta         __x0
	lda         __i1+1
	sta         __x0+1
	and          #128
	beq         .strftime_label_810
	lda          #255
.strftime_label_810:
	ldy          #7
.strftime_label_815:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .strftime_label_815
	ldx          #7
.strftime_label_827:
	lda         .lit.483, X
	sta         __x1, X
	dex         
	bpl         .strftime_label_827
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __smul8
	lda         __x2+4
	sta         __x1
	lda         __x2+5
	sta         __x1+1
	lda         __x2+6
	sta         __x1+2
	lda         __x2+7
	sta         __x1+3
	eor          #128
	cmp          #128
	lda          #0
	sbc          #0
	sta         __x1+4
	sta         __x1+5
	sta         __x1+6
	sta         __x1+7
	ldx          #5
.strftime_label_864:
	lda         __x1+7
	cmp          #128
	ror         __x1+7
	ror         __x1+6
	ror         __x1+5
	ror         __x1+4
	ror         __x1+3
	ror         __x1+2
	ror         __x1+1
	ror         __x1
	dex         
	bne         .strftime_label_864
	lda         __x0+7
	sta         __x2
	lda          #0
	stz         __x2+1
	stz         __x2+2
	stz         __x2+3
	sta         __x2+4
	sta         __x2+5
	sta         __x2+6
	sta         __x2+7
	ldx          #7
.strftime_label_891:
	lsr         __x2+7
	ror         __x2+6
	ror         __x2+5
	ror         __x2+4
	ror         __x2+3
	ror         __x2+2
	ror         __x2+1
	ror         __x2
	dex         
	bne         .strftime_label_891
	clc         
	ldy          #8
	ldx          #0
.strftime_label_910:
	lda         __x1, X
	adc         __x2, X
	sta         __x0, X
	inx         
	dey         
	bne         .strftime_label_910
	lda         __x0
	sta         __i0
	lda         __x0+7
	and          #128
	ora         __x0+1
	sta         __i0+1
	lda          #48
	jsr         __pusha
	ldx          #2
	jsr         __pushxy0
	jsr         __pushi0
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_948
	dex         
.strftime_label_948:
	txa         
	cmp          #0
	beq         .strftime_label_959
	lda          #255
.strftime_label_959:
	inc          A
	beq         .strftime_label_977
	.loc 11 380 41
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_977:
	jmp         .strftime_label_4376
.strftime_label_979:
	.loc 11 381 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #109
	bne         .strftime_label_1067
	lda         __i0+1
	bne         .strftime_label_1067
	.loc 11 383 29
	ldy          #8
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	clc         
	lda         __i0
	adc          #1
	sta         __i1
	lda         __i0+1
	adc          #0
	sta         __i1+1
	lda          #48
	jsr         __pusha
	ldx          #2
	jsr         __pushxy0
	jsr         __pushi1
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_1036
	dex         
.strftime_label_1036:
	txa         
	cmp          #0
	beq         .strftime_label_1047
	lda          #255
.strftime_label_1047:
	inc          A
	beq         .strftime_label_1065
	.loc 11 383 29
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_1065:
	jmp         .strftime_label_4375
.strftime_label_1067:
	.loc 11 384 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #100
	bne         .strftime_label_1145
	lda         __i0+1
	bne         .strftime_label_1145
	.loc 11 386 26
	ldy          #6
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	lda          #48
	jsr         __pusha
	ldx          #2
	jsr         __pushxy0
	jsr         __pushi0
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_1114
	dex         
.strftime_label_1114:
	txa         
	cmp          #0
	beq         .strftime_label_1125
	lda          #255
.strftime_label_1125:
	inc          A
	beq         .strftime_label_1143
	.loc 11 386 26
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_1143:
	jmp         .strftime_label_4374
.strftime_label_1145:
	.loc 11 387 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #101
	bne         .strftime_label_1223
	lda         __i0+1
	bne         .strftime_label_1223
	.loc 11 389 26
	ldy          #6
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	lda          #32
	jsr         __pusha
	ldx          #2
	jsr         __pushxy0
	jsr         __pushi0
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_1192
	dex         
.strftime_label_1192:
	txa         
	cmp          #0
	beq         .strftime_label_1203
	lda          #255
.strftime_label_1203:
	inc          A
	beq         .strftime_label_1221
	.loc 11 389 26
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_1221:
	jmp         .strftime_label_4373
.strftime_label_1223:
	.loc 11 390 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #72
	bne         .strftime_label_1301
	lda         __i0+1
	bne         .strftime_label_1301
	.loc 11 392 26
	ldy          #4
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	lda          #48
	jsr         __pusha
	ldx          #2
	jsr         __pushxy0
	jsr         __pushi0
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_1270
	dex         
.strftime_label_1270:
	txa         
	cmp          #0
	beq         .strftime_label_1281
	lda          #255
.strftime_label_1281:
	inc          A
	beq         .strftime_label_1299
	.loc 11 392 26
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_1299:
	jmp         .strftime_label_4372
.strftime_label_1301:
	.loc 11 393 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #73
	beq         .strftime_label_4423
	jmp         .strftime_label_1421
.strftime_label_4423:
	lda         __i0+1
	beq         .strftime_label_4425
	jmp         .strftime_label_1421
.strftime_label_4425:
	.loc 11 394 31
	ldy          #4
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	lda          #12
	sta         __i1
	lda          #0
	stz         __i1+1
	lda          #__i2
	ldx          #__i0
	ldy          #__i1
	jsr         __smod2
	lda          #__i2
	ldx          #9
	jsr         __set_var_value2
	.loc 11 395 16
	ldx          #9
	jsr          __var_value2_i0			// hour
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_1364
	.loc 11 395 16
	lda          #12
	sta         __i0
	lda          #0
	stz         __i0+1
	lda          #__i0
	ldx          #9
	jsr         __set_var_value2
.strftime_label_1364:
	.loc 11 396 61
	ldx          #9
	jsr          __var_value2_i0			// hour
	lda          #48
	jsr         __pusha
	ldx          #2
	jsr         __pushxy0
	jsr         __pushi0
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_1390
	dex         
.strftime_label_1390:
	txa         
	cmp          #0
	beq         .strftime_label_1401
	lda          #255
.strftime_label_1401:
	inc          A
	beq         .strftime_label_1419
	.loc 11 396 61
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_1419:
	jmp         .strftime_label_4371
.strftime_label_1421:
	.loc 11 397 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #77
	bne         .strftime_label_1499
	lda         __i0+1
	bne         .strftime_label_1499
	.loc 11 399 25
	ldy          #2
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	lda          #48
	jsr         __pusha
	ldx          #2
	jsr         __pushxy0
	jsr         __pushi0
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_1468
	dex         
.strftime_label_1468:
	txa         
	cmp          #0
	beq         .strftime_label_1479
	lda          #255
.strftime_label_1479:
	inc          A
	beq         .strftime_label_1497
	.loc 11 399 25
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_1497:
	jmp         .strftime_label_4370
.strftime_label_1499:
	.loc 11 400 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #83
	bne         .strftime_label_1576
	lda         __i0+1
	bne         .strftime_label_1576
	.loc 11 402 25
	lda         (__i6)
	sta         __i0
	ldy          #1
	lda         (__i6), Y
	sta         __i0+1
	lda          #48
	jsr         __pusha
	ldx          #2
	jsr         __pushxy0
	jsr         __pushi0
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_1545
	dex         
.strftime_label_1545:
	txa         
	cmp          #0
	beq         .strftime_label_1556
	lda          #255
.strftime_label_1556:
	inc          A
	beq         .strftime_label_1574
	.loc 11 402 25
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_1574:
	jmp         .strftime_label_4369
.strftime_label_1576:
	.loc 11 403 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #106
	bne         .strftime_label_1666
	lda         __i0+1
	bne         .strftime_label_1666
	.loc 11 405 30
	ldy          #14
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	clc         
	lda         __i0
	adc          #1
	sta         __i1
	lda         __i0+1
	adc          #0
	sta         __i1+1
	lda          #48
	jsr         __pusha
	ldx          #3
	jsr         __pushxy0
	jsr         __pushi1
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_1635
	dex         
.strftime_label_1635:
	txa         
	cmp          #0
	beq         .strftime_label_1646
	lda          #255
.strftime_label_1646:
	inc          A
	beq         .strftime_label_1664
	.loc 11 405 30
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_1664:
	jmp         .strftime_label_4368
.strftime_label_1666:
	.loc 11 406 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #119
	bne         .strftime_label_1746
	lda         __i0+1
	bne         .strftime_label_1746
	.loc 11 408 26
	ldy          #12
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	lda          #48
	jsr         __pusha
	ldx          #1
	jsr         __pushxy0
	jsr         __pushi0
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_1715
	dex         
.strftime_label_1715:
	txa         
	cmp          #0
	beq         .strftime_label_1726
	lda          #255
.strftime_label_1726:
	inc          A
	beq         .strftime_label_1744
	.loc 11 408 26
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_1744:
	jmp         .strftime_label_4367
.strftime_label_1746:
	.loc 11 409 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #117
	beq         .strftime_label_4427
	jmp         .strftime_label_1877
.strftime_label_4427:
	lda         __i0+1
	beq         .strftime_label_4429
	jmp         .strftime_label_1877
.strftime_label_4429:
	.loc 11 410 51
	ldy          #12
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_1786
	ldx          #13
	jsr          __var_addr_i0			// __invented__96
	lda          #7
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	bra         .strftime_label_1806
.strftime_label_1786:
	ldy          #12
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	ldx          #13
	jsr          __var_addr_i1			// __invented__96
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
.strftime_label_1806:
	ldx          #13
	jsr          __var_addr_i0			// __invented__96
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	lda          #__i1
	ldx          #11
	jsr         __set_var_value2
	.loc 11 411 60
	lda          #48
	jsr         __pusha
	ldx          #1
	jsr         __pushxy0
	jsr         __pushi1
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_1846
	dex         
.strftime_label_1846:
	txa         
	cmp          #0
	beq         .strftime_label_1857
	lda          #255
.strftime_label_1857:
	inc          A
	beq         .strftime_label_1875
	.loc 11 411 60
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_1875:
	jmp         .strftime_label_4366
.strftime_label_1877:
	.loc 11 412 52
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #1
	cmp          #97
	bne         .strftime_label_1887
	lda         __i0+1
	beq         .strftime_label_1888
.strftime_label_1887:
	dex         
.strftime_label_1888:
	stx         __b0
	ldx          #14
	jsr          __var_addr_i11			// __invented__97
	jsr          __spill2
	.byte __i11
	.byte 0x5c,0x00
	lda         __b0
	sta         (__i11)
	lda         __i0
	cmp          #97
	bne         .strftime_label_1907
	lda         __i0+1
	beq         .strftime_label_1942
.strftime_label_1907:
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #1
	cmp          #65
	bne         .strftime_label_1923
	lda         __i0+1
	beq         .strftime_label_1924
.strftime_label_1923:
	dex         
.strftime_label_1924:
	txa         
	sta         (__i11)
.strftime_label_1942:
	lda         (__i11)
	bne         .strftime_label_4431
	jmp         .strftime_label_2302
.strftime_label_4431:
	.loc 11 413 1
	ldy          #12
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	ldx          #0
	lda         __i0
	cmp          #0
	lda         __i0+1
	sbc          #0
	bvc         .strftime_label_1969
	eor          #128
.strftime_label_1969:
	bmi         .strftime_label_1967
	inx         
.strftime_label_1967:
	stx         __b0
	ldx          #15
	jsr          __var_addr_i12			// __invented__98
	lda         __b0
	sta         (__i12)
	lda         __i0
	cmp          #0
	lda         __i0+1
	sbc          #0
	bvc         .strftime_label_1988
	eor          #128
.strftime_label_1988:
	bmi         .strftime_label_2027
	ldy          #12
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	ldx          #0
	lda         __i0
	cmp          #7
	lda         __i0+1
	sbc          #0
	bvc         .strftime_label_2009
	eor          #128
.strftime_label_2009:
	bpl         .strftime_label_2007
	inx         
.strftime_label_2007:
	txa         
	sta         (__i12)
.strftime_label_2027:
	lda         (__i12)
	beq         .strftime_label_2095
	ldy          #12
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	lda         __i0
	asl          A
	sta         __i1
	lda         __i0+1
	rol          A
	sta         __i1+1
	lda         #%lo(.local.week_days.4470)
	sta         __i0
	lda         #%hi(.local.week_days.4470)
	sta         __i0+1
	clc         
	lda         __i0
	adc         __i1
	sta         __i2
	lda         __i0+1
	adc         __i1+1
	sta         __i2+1
	lda         (__i2)
	sta         __i0
	ldy          #1
	lda         (__i2), Y
	sta         __i0+1
	ldx          #17
	jsr          __var_addr_i1			// __invented__99
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
	bra         .strftime_label_2113
.strftime_label_2095:
	lda         #%lo(.str.267)
	sta         __i0
	lda         #%hi(.str.267)
	sta         __i0+1
	ldx          #17
	jsr          __var_addr_i1			// __invented__99
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
.strftime_label_2113:
	ldx          #17
	jsr          __var_addr_i0			// __invented__99
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	lda          #__i1
	ldx          #19
	jsr         __set_var_value2
	.loc 11 415 24
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #97
	bne         .strftime_label_2248
	lda         __i0+1
	bne         .strftime_label_2248
	.loc 11 416 55
	ldx          #23
	jsr          __var_addr_i0			// short_name
	jsr         __zeromem1
	.byte        __i0, 4
	ldx          #19
	jsr          __var_value2_i1			// text
	lda         (__i1)
	sta         __b0
	lda          #__b0
	ldx          #23
	jsr         __set_var_value1
	ldy          #1
	lda         (__i1), Y
	sta         (__i0), Y
	iny         
	lda         (__i1), Y
	sta         (__i0), Y
	.loc 11 417 57
	jsr         __pushi0
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendText
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_2217
	dex         
.strftime_label_2217:
	txa         
	cmp          #0
	beq         .strftime_label_2228
	lda          #255
.strftime_label_2228:
	inc          A
	beq         .strftime_label_2246
	.loc 11 417 57
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_2246:
	bra         .strftime_label_2300
.strftime_label_2248:
	.loc 11 418 58
	ldx          #19
	jsr          __var_value2_i0			// text
	jsr         __pushi0
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendText
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_2270
	dex         
.strftime_label_2270:
	txa         
	cmp          #0
	beq         .strftime_label_2281
	lda          #255
.strftime_label_2281:
	inc          A
	beq         .strftime_label_2299
	.loc 11 419 1
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_2299:
.strftime_label_2300:
	jmp         .strftime_label_4365
.strftime_label_2302:
	.loc 11 421 73
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #1
	cmp          #98
	bne         .strftime_label_2312
	lda         __i0+1
	beq         .strftime_label_2313
.strftime_label_2312:
	dex         
.strftime_label_2313:
	stx         __b0
	ldx          #24
	jsr          __var_addr_i12			// __invented__100
	jsr          __spill2
	.byte __i12
	.byte 0x5e,0x00
	lda         __b0
	sta         (__i12)
	lda         __i0
	cmp          #98
	bne         .strftime_label_2332
	lda         __i0+1
	beq         .strftime_label_2367
.strftime_label_2332:
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #1
	cmp          #66
	bne         .strftime_label_2348
	lda         __i0+1
	beq         .strftime_label_2349
.strftime_label_2348:
	dex         
.strftime_label_2349:
	txa         
	sta         (__i12)
.strftime_label_2367:
	lda         (__i12)
	sta         __b0
	ldx          #25
	jsr          __var_addr_i13			// __invented__101
	lda         __b0
	sta         (__i13)
	bne         .strftime_label_2414
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #1
	cmp          #104
	bne         .strftime_label_2395
	lda         __i0+1
	beq         .strftime_label_2396
.strftime_label_2395:
	dex         
.strftime_label_2396:
	txa         
	sta         (__i13)
.strftime_label_2414:
	lda         (__i13)
	bne         .strftime_label_4433
	jmp         .strftime_label_2771
.strftime_label_4433:
	.loc 11 422 1
	ldy          #8
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	ldx          #0
	lda         __i0
	cmp          #0
	lda         __i0+1
	sbc          #0
	bvc         .strftime_label_2441
	eor          #128
.strftime_label_2441:
	bmi         .strftime_label_2439
	inx         
.strftime_label_2439:
	stx         __b0
	ldx          #26
	jsr          __var_addr_i14			// __invented__102
	lda         __b0
	sta         (__i14)
	lda         __i0
	cmp          #0
	lda         __i0+1
	sbc          #0
	bvc         .strftime_label_2460
	eor          #128
.strftime_label_2460:
	bmi         .strftime_label_2499
	ldy          #8
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	ldx          #0
	lda         __i0
	cmp          #12
	lda         __i0+1
	sbc          #0
	bvc         .strftime_label_2481
	eor          #128
.strftime_label_2481:
	bpl         .strftime_label_2479
	inx         
.strftime_label_2479:
	txa         
	sta         (__i14)
.strftime_label_2499:
	lda         (__i14)
	beq         .strftime_label_2567
	ldy          #8
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	lda         __i0
	asl          A
	sta         __i1
	lda         __i0+1
	rol          A
	sta         __i1+1
	lda         #%lo(.local.months.4471)
	sta         __i0
	lda         #%hi(.local.months.4471)
	sta         __i0+1
	clc         
	lda         __i0
	adc         __i1
	sta         __i2
	lda         __i0+1
	adc         __i1+1
	sta         __i2+1
	lda         (__i2)
	sta         __i0
	ldy          #1
	lda         (__i2), Y
	sta         __i0+1
	ldx          #28
	jsr          __var_addr_i1			// __invented__103
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
	bra         .strftime_label_2585
.strftime_label_2567:
	lda         #%lo(.str.268)
	sta         __i0
	lda         #%hi(.str.268)
	sta         __i0+1
	ldx          #28
	jsr          __var_addr_i1			// __invented__103
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
.strftime_label_2585:
	ldx          #28
	jsr          __var_addr_i0			// __invented__103
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	lda          #__i1
	ldx          #19
	jsr         __set_var_value2
	.loc 11 424 24
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #66
	bne         .strftime_label_2609
	lda         __i0+1
	beq         .strftime_label_2717
.strftime_label_2609:
	.loc 11 425 55
	ldx          #32
	jsr          __var_addr_i0			// short_name
	jsr         __zeromem1
	.byte        __i0, 4
	ldx          #19
	jsr          __var_value2_i1			// text
	lda         (__i1)
	sta         __b0
	lda          #__b0
	ldx          #32
	jsr         __set_var_value1
	ldy          #1
	lda         (__i1), Y
	sta         (__i0), Y
	iny         
	lda         (__i1), Y
	sta         (__i0), Y
	.loc 11 426 57
	jsr         __pushi0
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendText
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_2686
	dex         
.strftime_label_2686:
	txa         
	cmp          #0
	beq         .strftime_label_2697
	lda          #255
.strftime_label_2697:
	inc          A
	beq         .strftime_label_2715
	.loc 11 426 57
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_2715:
	bra         .strftime_label_2769
.strftime_label_2717:
	.loc 11 427 58
	ldx          #19
	jsr          __var_value2_i0			// text
	jsr         __pushi0
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendText
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_2739
	dex         
.strftime_label_2739:
	txa         
	cmp          #0
	beq         .strftime_label_2750
	lda          #255
.strftime_label_2750:
	inc          A
	beq         .strftime_label_2768
	.loc 11 428 1
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_2768:
.strftime_label_2769:
	jmp         .strftime_label_4364
.strftime_label_2771:
	.loc 11 430 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #112
	beq         .strftime_label_4435
	jmp         .strftime_label_2902
.strftime_label_4435:
	lda         __i0+1
	beq         .strftime_label_4437
	jmp         .strftime_label_2902
.strftime_label_4437:
	.loc 11 432 37
	ldy          #4
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	lda         __i0
	cmp          #12
	lda         __i0+1
	sbc          #0
	bvc         .strftime_label_2797
	eor          #128
.strftime_label_2797:
	bpl         .strftime_label_2823
	lda         #%lo(.str.269)
	sta         __i0
	lda         #%hi(.str.269)
	sta         __i0+1
	ldx          #34
	jsr          __var_addr_i1			// __invented__104
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
	bra         .strftime_label_2841
.strftime_label_2823:
	lda         #%lo(.str.270)
	sta         __i0
	lda         #%hi(.str.270)
	sta         __i0+1
	ldx          #34
	jsr          __var_addr_i1			// __invented__104
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
.strftime_label_2841:
	ldx          #34
	jsr          __var_addr_i0			// __invented__104
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	jsr         __pushi1
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendText
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_2871
	dex         
.strftime_label_2871:
	txa         
	cmp          #0
	beq         .strftime_label_2882
	lda          #255
.strftime_label_2882:
	inc          A
	beq         .strftime_label_2900
	.loc 11 432 37
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_2900:
	jmp         .strftime_label_4363
.strftime_label_2902:
	.loc 11 433 52
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #1
	cmp          #110
	bne         .strftime_label_2912
	lda         __i0+1
	beq         .strftime_label_2913
.strftime_label_2912:
	dex         
.strftime_label_2913:
	stx         __b0
	ldx          #35
	jsr          __var_addr_i14			// __invented__105
	jsr          __spill2
	.byte __i14
	.byte 0x60,0x00
	lda         __b0
	sta         (__i14)
	lda         __i0
	cmp          #110
	bne         .strftime_label_2932
	lda         __i0+1
	beq         .strftime_label_2967
.strftime_label_2932:
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #1
	cmp          #116
	bne         .strftime_label_2948
	lda         __i0+1
	beq         .strftime_label_2949
.strftime_label_2948:
	dex         
.strftime_label_2949:
	txa         
	sta         (__i14)
.strftime_label_2967:
	lda         (__i14)
	bne         .strftime_label_4439
	jmp         .strftime_label_3075
.strftime_label_4439:
	.loc 11 434 29
	ldx          #88
	jsr          __var_value2_i0			// length
	clc         
	lda         __i0
	adc          #1
	sta         __i1
	lda         __i0+1
	adc          #0
	sta         __i1+1
	cmp         __i7+1
	bcc         .strftime_label_3010
	bne         .strftime_label_2994
	lda         __i1
	cmp         __i7
	bcc         .strftime_label_3010
.strftime_label_2994:
	.loc 11 434 29
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_3010:
	.loc 11 435 1
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #110
	bne         .strftime_label_3034
	lda         __i0+1
	bne         .strftime_label_3034
	ldx          #36
	jsr          __var_addr_i0			// __invented__106
	lda          #10
	ldy          #0
	sta         (__i0)
	bra         .strftime_label_3042
.strftime_label_3034:
	ldx          #36
	jsr          __var_addr_i0			// __invented__106
	lda          #9
	ldy          #0
	sta         (__i0)
.strftime_label_3042:
	ldx          #36
	jsr          __var_addr_i0			// __invented__106
	lda         (__i0)
	sta         __b0
	ldx          #88
	jsr          __var_value2_i0			// length
	ldx          #88
	jsr          __var_addr_i1			// length
	lda          #__i1
	jsr         __inc21
	clc         
	lda         __i4
	adc         __i0
	sta         __i1
	lda         __i4+1
	adc         __i0+1
	sta         __i1+1
	lda         __b0
	sta         (__i1)
	jmp         .strftime_label_4362
.strftime_label_3075:
	.loc 11 436 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #122
	beq         .strftime_label_4441
	jmp         .strftime_label_3669
.strftime_label_4441:
	lda         __i0+1
	beq         .strftime_label_4443
	jmp         .strftime_label_3669
.strftime_label_4443:
	.loc 11 437 35
	ldy          #21
	ldx          #3
.strftime_label_3097:
	lda         (__i6), Y
	sta         __l0, X
	dey         
	dex         
	bpl         .strftime_label_3097
	lda         __l0
	sta         __i0
	lda         __l0+3
	and          #128
	ora         __l0+1
	sta         __i0+1
	lda          #__i0
	ldx          #38
	jsr         __set_var_value2
	.loc 11 438 16
	lda          #43
	sta         __b3
	.loc 11 439 17
	lda         __i0
	cmp          #0
	lda         __i0+1
	sbc          #0
	bvc         .strftime_label_3123
	eor          #128
.strftime_label_3123:
	bpl         .strftime_label_3152
	.loc 11 440 1
	lda          #45
	sta         __b3
	.loc 11 441 1
	ldx          #38
	jsr          __var_value2_i0			// offset
	sec         
	lda          #0
	sbc         __i0
	sta         __i1
	lda          #0
	sbc         __i0+1
	sta         __i1+1
	lda          #__i1
	ldx          #38
	jsr         __set_var_value2
.strftime_label_3152:
	.loc 11 443 29
	ldx          #88
	jsr          __var_value2_i0			// length
	clc         
	lda         __i0
	adc          #1
	sta         __i1
	lda         __i0+1
	adc          #0
	sta         __i1+1
	cmp         __i7+1
	bcc         .strftime_label_3183
	bne         .strftime_label_3167
	lda         __i1
	cmp         __i7
	bcc         .strftime_label_3183
.strftime_label_3167:
	.loc 11 443 29
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_3183:
	.loc 11 444 1
	lda         __i4
	sta         __i0
	lda         __i4+1
	sta         __i0+1
	ldx          #88
	jsr          __var_value2_i1			// length
	ldx          #88
	jsr          __var_addr_i15			// length
	lda          #__i15
	jsr         __inc21
	jsr          __spill2
	.byte __i15
	.byte 0x5a,0x00
	clc         
	lda         __i0
	adc         __i1
	sta         __i2
	lda         __i0+1
	adc         __i1+1
	sta         __i2+1
	lda         __b3
	sta         (__i2)
	.loc 11 448 30
	ldx          #38
	jsr          __var_value2_i1			// offset
	lda         __i1
	sta         __x0
	lda         __i1+1
	sta         __x0+1
	and          #128
	beq         .strftime_label_3224
	lda          #255
.strftime_label_3224:
	ldy          #7
.strftime_label_3228:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .strftime_label_3228
	ldx          #7
.strftime_label_3239:
	lda         .lit.488, X
	sta         __x1, X
	dex         
	bpl         .strftime_label_3239
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __smul8
	lda         __x2+4
	sta         __x1
	lda         __x2+5
	sta         __x1+1
	lda         __x2+6
	sta         __x1+2
	lda         __x2+7
	sta         __x1+3
	eor          #128
	cmp          #128
	lda          #0
	sbc          #0
	sta         __x1+4
	sta         __x1+5
	sta         __x1+6
	sta         __x1+7
	clc         
	ldy          #8
	ldx          #0
.strftime_label_3278:
	lda         __x1, X
	adc         __x0, X
	sta         __x2, X
	inx         
	dey         
	bne         .strftime_label_3278
	lda         __x2+1
	sta         __x1
	lda         __x2+2
	sta         __x1+1
	lda         __x2+3
	sta         __x1+2
	lda         __x2+4
	sta         __x1+3
	lda         __x2+5
	sta         __x1+4
	lda         __x2+6
	sta         __x1+5
	lda         __x2+7
	sta         __x1+6
	eor          #128
	cmp          #128
	lda          #0
	sbc          #0
	sta         __x1+7
	ldx          #3
.strftime_label_3308:
	lda         __x1+7
	cmp          #128
	ror         __x1+7
	ror         __x1+6
	ror         __x1+5
	ror         __x1+4
	ror         __x1+3
	ror         __x1+2
	ror         __x1+1
	ror         __x1
	dex         
	bne         .strftime_label_3308
	lda         __x0+7
	sta         __x2
	lda          #0
	stz         __x2+1
	stz         __x2+2
	stz         __x2+3
	sta         __x2+4
	sta         __x2+5
	sta         __x2+6
	sta         __x2+7
	ldx          #7
.strftime_label_3335:
	lsr         __x2+7
	ror         __x2+6
	ror         __x2+5
	ror         __x2+4
	ror         __x2+3
	ror         __x2+2
	ror         __x2+1
	ror         __x2
	dex         
	bne         .strftime_label_3335
	clc         
	ldy          #8
	ldx          #0
.strftime_label_3353:
	lda         __x1, X
	adc         __x2, X
	sta         __x0, X
	inx         
	dey         
	bne         .strftime_label_3353
	lda         __x0
	sta         __i1
	lda         __x0+7
	and          #128
	ora         __x0+1
	sta         __i1+1
	lda          #48
	jsr         __pusha
	ldx          #2
	jsr         __pushxy0
	jsr         __pushi1
	jsr         __pushi15
	jsr         __pushi7
	jsr         __pushi0
	ldx         #__i0
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_3393
	dex         
.strftime_label_3393:
	txa         
	cmp          #0
	beq         .strftime_label_3404
	lda          #255
.strftime_label_3404:
	inc          A
	sta         __b1
	ldx          #39
	jsr          __var_addr_i15			// __invented__107
	lda         __b1
	sta         (__i15)
	beq         .strftime_label_4445
	jmp         .strftime_label_3647
.strftime_label_4445:
	ldx          #38
	jsr          __var_value2_i0			// offset
	lda         __i0
	sta         __x0
	lda         __i0+1
	sta         __x0+1
	and          #128
	beq         .strftime_label_3432
	lda          #255
.strftime_label_3432:
	ldy          #7
.strftime_label_3436:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .strftime_label_3436
	ldx          #7
.strftime_label_3447:
	lda         .lit.490, X
	sta         __x1, X
	dex         
	bpl         .strftime_label_3447
	lda          #__x2
	ldx          #__x0
	ldy          #__x1
	jsr         __smul8
	lda         __x2+4
	sta         __x1
	lda         __x2+5
	sta         __x1+1
	lda         __x2+6
	sta         __x1+2
	lda         __x2+7
	sta         __x1+3
	eor          #128
	cmp          #128
	lda          #0
	sbc          #0
	sta         __x1+4
	sta         __x1+5
	sta         __x1+6
	sta         __x1+7
	clc         
	ldy          #8
	ldx          #0
.strftime_label_3486:
	lda         __x1, X
	adc         __x0, X
	sta         __x2, X
	inx         
	dey         
	bne         .strftime_label_3486
	lda         __x2+7
	cmp          #128
	ror          A
	sta         __x1+7
	lda         __x2+6
	ror          A
	sta         __x1+6
	lda         __x2+5
	ror          A
	sta         __x1+5
	lda         __x2+4
	ror          A
	sta         __x1+4
	lda         __x2+3
	ror          A
	sta         __x1+3
	lda         __x2+2
	ror          A
	sta         __x1+2
	lda         __x2+1
	ror          A
	sta         __x1+1
	lda         __x2
	ror          A
	sta         __x1
	ldx          #4
.strftime_label_3523:
	lda         __x1+7
	cmp          #128
	ror         __x1+7
	ror         __x1+6
	ror         __x1+5
	ror         __x1+4
	ror         __x1+3
	ror         __x1+2
	ror         __x1+1
	ror         __x1
	dex         
	bne         .strftime_label_3523
	lda         __x0+7
	sta         __x2
	lda          #0
	stz         __x2+1
	stz         __x2+2
	stz         __x2+3
	sta         __x2+4
	sta         __x2+5
	sta         __x2+6
	sta         __x2+7
	ldx          #7
.strftime_label_3550:
	lsr         __x2+7
	ror         __x2+6
	ror         __x2+5
	ror         __x2+4
	ror         __x2+3
	ror         __x2+2
	ror         __x2+1
	ror         __x2
	dex         
	bne         .strftime_label_3550
	clc         
	ldy          #8
	ldx          #0
.strftime_label_3568:
	lda         __x1, X
	adc         __x2, X
	sta         __x0, X
	inx         
	dey         
	bne         .strftime_label_3568
	lda         __x0
	sta         __i0
	lda         __x0+7
	and          #128
	ora         __x0+1
	sta         __i0+1
	lda          #60
	sta         __i1
	lda          #0
	stz         __i1+1
	lda          #__i2
	ldx          #__i0
	ldy          #__i1
	jsr         __smod2
	lda          #48
	jsr         __pusha
	ldx          #2
	jsr         __pushxy0
	jsr         __pushi2
	jsr          __reload2
	.byte __i0
	.byte 0x5a,0x00
	jsr         __pushi0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i1
	ldy          #0
	jsr         AppendNumber
	ldx          #1
	lda         __i1
	ora         __i1+1
	bne         .strftime_label_3621
	dex         
.strftime_label_3621:
	txa         
	cmp          #0
	beq         .strftime_label_3632
	lda          #255
.strftime_label_3632:
	inc          A
	sta         (__i15)
.strftime_label_3647:
	lda         (__i15)
	beq         .strftime_label_3667
	.loc 11 448 30
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_3667:
	jmp         .strftime_label_4361
.strftime_label_3669:
	.loc 11 449 31
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #90
	beq         .strftime_label_4447
	jmp         .strftime_label_3803
.strftime_label_4447:
	lda         __i0+1
	beq         .strftime_label_4449
	jmp         .strftime_label_3803
.strftime_label_4449:
	.loc 11 451 54
	ldy          #22
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	lda         __i0
	bne         .strftime_label_3722
	lda         __i0+1
	bne         .strftime_label_3722
	lda         #%lo(.str.271)
	sta         __i0
	lda         #%hi(.str.271)
	sta         __i0+1
	ldx          #41
	jsr          __var_addr_i1			// __invented__108
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
	bra         .strftime_label_3742
.strftime_label_3722:
	ldy          #22
	lda         (__i6), Y
	sta         __i0
	iny         
	lda         (__i6), Y
	sta         __i0+1
	ldx          #41
	jsr          __var_addr_i1			// __invented__108
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
.strftime_label_3742:
	ldx          #41
	jsr          __var_addr_i0			// __invented__108
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	jsr         __pushi1
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendText
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_3772
	dex         
.strftime_label_3772:
	txa         
	cmp          #0
	beq         .strftime_label_3783
	lda          #255
.strftime_label_3783:
	inc          A
	beq         .strftime_label_3801
	.loc 11 451 54
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_3801:
	jmp         .strftime_label_4360
.strftime_label_3803:
	.loc 11 453 41
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #1
	cmp          #70
	bne         .strftime_label_3813
	lda         __i0+1
	beq         .strftime_label_3814
.strftime_label_3813:
	dex         
.strftime_label_3814:
	stx         __b0
	ldx          #42
	jsr          __var_addr_i15			// __invented__109
	jsr          __spill2
	.byte __i15
	.byte 0x62,0x00
	lda         __b0
	sta         (__i15)
	lda         __i0
	cmp          #70
	bne         .strftime_label_3833
	lda         __i0+1
	beq         .strftime_label_3868
.strftime_label_3833:
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #1
	cmp          #68
	bne         .strftime_label_3849
	lda         __i0+1
	beq         .strftime_label_3850
.strftime_label_3849:
	dex         
.strftime_label_3850:
	txa         
	sta         (__i15)
.strftime_label_3868:
	lda         (__i15)
	sta         __b0
	ldx          #43
	jsr          __var_addr_i11			// __invented__110
	lda         __b0
	sta         (__i11)
	bne         .strftime_label_3915
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #1
	cmp          #82
	bne         .strftime_label_3896
	lda         __i0+1
	beq         .strftime_label_3897
.strftime_label_3896:
	dex         
.strftime_label_3897:
	txa         
	sta         (__i11)
.strftime_label_3915:
	lda         (__i11)
	sta         __b0
	ldx          #44
	jsr          __var_addr_i12			// __invented__111
	lda         __b0
	sta         (__i12)
	bne         .strftime_label_3962
	lda         __b2
	sta         __i0
	stz         __i0+1
	ldx          #1
	cmp          #84
	bne         .strftime_label_3943
	lda         __i0+1
	beq         .strftime_label_3944
.strftime_label_3943:
	dex         
.strftime_label_3944:
	txa         
	sta         (__i12)
.strftime_label_3962:
	lda         (__i12)
	bne         .strftime_label_4451
	jmp         .strftime_label_4274
.strftime_label_4451:
	.loc 11 457 41
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #70
	bne         .strftime_label_4008
	lda         __i0+1
	bne         .strftime_label_4008
	lda         #%lo(.str.272)
	sta         __i0
	lda         #%hi(.str.272)
	sta         __i0+1
	ldx          #48
	jsr          __var_addr_i1			// __invented__112
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
	jmp         .strftime_label_4138
.strftime_label_4008:
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #68
	bne         .strftime_label_4041
	lda         __i0+1
	bne         .strftime_label_4041
	lda         #%lo(.str.273)
	sta         __i0
	lda         #%hi(.str.273)
	sta         __i0+1
	ldx          #50
	jsr          __var_addr_i1			// __invented__113
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
	bra         .strftime_label_4115
.strftime_label_4041:
	lda         __b2
	sta         __i0
	stz         __i0+1
	cmp          #82
	bne         .strftime_label_4074
	lda         __i0+1
	bne         .strftime_label_4074
	lda         #%lo(.str.274)
	sta         __i0
	lda         #%hi(.str.274)
	sta         __i0+1
	ldx          #52
	jsr          __var_addr_i1			// __invented__114
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
	bra         .strftime_label_4092
.strftime_label_4074:
	lda         #%lo(.str.275)
	sta         __i0
	lda         #%hi(.str.275)
	sta         __i0+1
	ldx          #52
	jsr          __var_addr_i1			// __invented__114
	lda         __i0
	sta         (__i1)
	lda         __i0+1
	ldy          #1
	sta         (__i1), Y
.strftime_label_4092:
	ldx          #52
	jsr          __var_addr_i0			// __invented__114
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	ldx          #50
	jsr          __var_addr_i0			// __invented__113
	lda         __i1
	sta         (__i0)
	lda         __i1+1
	ldy          #1
	sta         (__i0), Y
.strftime_label_4115:
	ldx          #50
	jsr          __var_addr_i0			// __invented__113
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	ldx          #48
	jsr          __var_addr_i0			// __invented__112
	lda         __i1
	sta         (__i0)
	lda         __i1+1
	ldy          #1
	sta         (__i0), Y
.strftime_label_4138:
	ldx          #48
	jsr          __var_addr_i0			// __invented__112
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	lda          #__i1
	ldx          #46
	jsr         __set_var_value2
	.loc 11 460 20
	ldx          #46
	jsr          __var_value2_i0			// replacement
	jsr         __pushi6
	jsr         __pushi0
	ldx          #32
	jsr         __pushxy0
	ldx          #86
	lda          #__i14
	jsr          __var_addr_push_i14
	ldx         #__i0
	ldy          #0
	jsr         strftime
	lda          #__i0
	ldx          #54
	jsr         __set_var_value2
	.loc 11 462 49
	ldx          #1
	lda         __i0
	ora         __i0+1
	beq         .strftime_label_4185
	dex         
.strftime_label_4185:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i15			// __invented__115
	lda         __b0
	sta         (__i15)
	lda         __i0
	ora         __i0+1
	beq         .strftime_label_4252
	jsr         __pushi14
	ldx          #88
	lda          #__i0
	jsr          __var_addr_push_i0
	jsr         __pushi7
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         AppendText
	ldx          #1
	lda         __i0
	ora         __i0+1
	bne         .strftime_label_4226
	dex         
.strftime_label_4226:
	txa         
	cmp          #0
	beq         .strftime_label_4237
	lda          #255
.strftime_label_4237:
	inc          A
	sta         (__i15)
.strftime_label_4252:
	lda         (__i15)
	beq         .strftime_label_4272
	.loc 11 462 49
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_4272:
	bra         .strftime_label_4359
.strftime_label_4274:
	.loc 11 464 29
	ldx          #88
	jsr          __var_value2_i0			// length
	clc         
	lda         __i0
	adc          #2
	sta         __i1
	lda         __i0+1
	adc          #0
	sta         __i1+1
	cmp         __i7+1
	bcc         .strftime_label_4305
	bne         .strftime_label_4289
	lda         __i1
	cmp         __i7
	bcc         .strftime_label_4305
.strftime_label_4289:
	.loc 11 464 29
	stz         __i0
	stz         __i0+1
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.strftime_label_4305:
	.loc 11 465 1
	lda         __i4
	sta         __i0
	lda         __i4+1
	sta         __i0+1
	ldx          #88
	jsr          __var_value2_i1			// length
	ldx          #88
	jsr          __var_addr_i2			// length
	lda          #__i2
	jsr         __inc21
	clc         
	lda         __i0
	adc         __i1
	sta         __i3
	lda         __i0+1
	adc         __i1+1
	sta         __i3+1
	lda          #37
	ldy          #0
	sta         (__i3)
	.loc 11 466 1
	ldx          #88
	jsr          __var_value2_i1			// length
	lda          #__i2
	jsr         __inc21
	clc         
	lda         __i0
	adc         __i1
	sta         __i2
	lda         __i0+1
	adc         __i1+1
	sta         __i2+1
	lda         __b2
	sta         (__i2)
.strftime_label_4359:
.strftime_label_4360:
.strftime_label_4361:
.strftime_label_4362:
.strftime_label_4363:
.strftime_label_4364:
.strftime_label_4365:
.strftime_label_4366:
.strftime_label_4367:
.strftime_label_4368:
.strftime_label_4369:
.strftime_label_4370:
.strftime_label_4371:
.strftime_label_4372:
.strftime_label_4373:
.strftime_label_4374:
.strftime_label_4375:
.strftime_label_4376:
.strftime_label_4377:
.strftime_label_4378:
.strftime_label_4379:
	jmp         .strftime_label_362
.strftime_label_4381:
	.loc 11 469 1
	ldx          #88
	jsr          __var_value2_i0			// length
	clc         
	lda         __i4
	adc         __i0
	sta         __i1
	lda         __i4+1
	adc         __i0+1
	sta         __i1+1
	lda          #0
	tay         
	sta         (__i1)
	.loc 11 470 1
	ldx          #88
	jsr          __var_value2_i0			// length
	ldx          #103
	lda          #__i0
	jsr         __load_result_value2
	jmp         .strftime_label_357
.func_end_strftime:
	.size strftime, .func_end_strftime-strftime

	.section ".text.ParseNumber", "ax", @progbits
	.local  ParseNumber
	.type ParseNumber, @function

ParseNumber:
	lda          #7
	jsr          __enter_leaf_res
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
	ldx          #2
	jsr          __arg_value2_i2			// width
	ldx          #0
	jsr          __arg_value2_i3			// input
	.loc 11 474 14
	stz         __i0
	stz         __i0+1
	.loc 11 475 15
	.loc 11 476 72
.ParseNumber_label_35:
	ldx          #0
	txa         
	cmp         __i2
	sbc         __i2+1
	bvc         .ParseNumber_label_40
	eor          #128
.ParseNumber_label_40:
	bpl         .ParseNumber_label_38
	inx         
.ParseNumber_label_38:
	stx         __b0
	ldx          #4
	jsr          __var_addr_i1			// __invented__117
	lda         __b0
	sta         (__i1)
	lda          #0
	cmp         __i2
	sbc         __i2+1
	bvc         .ParseNumber_label_59
	eor          #128
.ParseNumber_label_59:
	bpl         .ParseNumber_label_101
	lda         (__i3)
	sta         __i4
	stz         __i4+1
	ldx          #0
	cmp          #48
	lda         __i4+1
	sbc          #0
	bvc         .ParseNumber_label_83
	eor          #128
.ParseNumber_label_83:
	bmi         .ParseNumber_label_81
	inx         
.ParseNumber_label_81:
	t