	.file   "libc/math_extra.c"
	.file 1 "libc/include/math.h"
	.file 2 "libc/include/errno.h"
	.file 3 "libc/include/syscall.h"
	.file 4 "libc/include/davecc_guest_syscalls.h"
	.file 5 "libc/include/limits.h"
	.file 6 "libc/include/stddef.h"
	.file 7 "libc/include/stdint.h"
	.file 8 "libc/math_extra.c"
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
	.section ".text.__davecc_fpclassify", "ax", @progbits
	.global __davecc_fpclassify
	.type __davecc_fpclassify, @function

__davecc_fpclassify:
	lda          #13
	jsr          __enter_leaf_res
	.byte        0x00,0x06,0x00		// Save mask i:0 b:0 l:3 x:0 f:0 
	ldx          #0
	jsr          __arg_value4_f0			// value
	.loc 9 16 1
	ldx          #7
	jsr          __var_addr_i0			// representation
	ldx          #__f0
	ldy          #__i0
	jsr          __store_indirect4
	.loc 9 17 1
	ldx          #__i0
	ldy          #__l2
	jsr          __load_indirect4
	lda         __l2+2
	sta         __l3
	lda         __l2+3
	sta         __l3+1
	lda          #0
	stz         __l3+2
	stz         __l3+3
	ldx          #7
.__davecc_fpclassify_label_54:
	lsr         __l3+3
	ror         __l3+2
	ror         __l3+1
	ror         __l3
	dex         
	bne         .__davecc_fpclassify_label_54
	lda         __l3
	sta         __l4
	stz         __l4+1
	stz         __l4+2
	lda          #0
	stz         __l4+3
	ldx          #3
.__davecc_fpclassify_label_75:
	lda         __l4, X
	sta         __l0, X
	dex         
	bpl         .__davecc_fpclassify_label_75
	.loc 9 18 1
	lda         __l2
	sta         __l3
	lda         __l2+1
	sta         __l3+1
	lda         __l2+2
	and          #127
	sta         __l3+2
	lda          #0
	stz         __l3+3
	ldx          #3
.__davecc_fpclassify_label_97:
	lda         __l3, X
	sta         __l1, X
	dex         
	bpl         .__davecc_fpclassify_label_97
	.loc 9 19 23
	lda         __l4
	cmp          #255
	bne         .__davecc_fpclassify_label_165
	lda         __l4+1
	bne         .__davecc_fpclassify_label_165
	lda         __l4+2
	bne         .__davecc_fpclassify_label_165
	lda         __l4+3
	bne         .__davecc_fpclassify_label_165
	.loc 9 19 23
	lda         __l1
	ora         __l1+1
	ora         __l1+2
	ora         __l1+3
	bne         .__davecc_fpclassify_label_135
	ldx          #9
	jsr          __var_addr_i0			// __invented__2
	lda          #1
	ldy          #0
	sta         (__i0)
	dec          A
	iny         
	sta         (__i0), Y
	bra         .__davecc_fpclassify_label_146
.__davecc_fpclassify_label_135:
	ldx          #9
	jsr          __var_addr_i0			// __invented__2
	ldy          #1
	lda          #0
.__davecc_fpclassify_label_142:
	sta         (__i0), Y
	dey         
	bpl         .__davecc_fpclassify_label_142
.__davecc_fpclassify_label_146:
	ldx          #9
	jsr          __var_addr_i0			// __invented__2
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	lda         #__i1
	jsr         __result2
.__davecc_fpclassify_label_162:
	ldy          #20
	jmp          __leave_leaf
.__davecc_fpclassify_label_165:
	.loc 9 32 20
	lda         __l0
	ora         __l0+1
	ora         __l0+2
	ora         __l0+3
	bne         .__davecc_fpclassify_label_216
	.loc 9 32 20
	lda         __l1
	ora         __l1+1
	ora         __l1+2
	ora         __l1+3
	bne         .__davecc_fpclassify_label_189
	ldx          #11
	jsr          __var_addr_i0			// __invented__3
	lda          #2
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	bra         .__davecc_fpclassify_label_200
.__davecc_fpclassify_label_189:
	ldx          #11
	jsr          __var_addr_i0			// __invented__3
	lda          #3
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
.__davecc_fpclassify_label_200:
	ldx          #11
	jsr          __var_addr_i0			// __invented__3
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	lda         #__i1
	jsr         __result2
	bra         .__davecc_fpclassify_label_162
.__davecc_fpclassify_label_216:
	.loc 9 33 1
	lda          #4
	sta         __i0
	stz         __i0+1
	lda         #__i0
	jsr         __result2
	bra         .__davecc_fpclassify_label_162
.func_end___davecc_fpclassify:
	.size __davecc_fpclassify, .func_end___davecc_fpclassify-__davecc_fpclassify

	.section ".text.__davecc_signbit", "ax", @progbits
	.global __davecc_signbit
	.type __davecc_signbit, @function

__davecc_signbit:
	lda          #9
	jsr          __enter_leaf_res_nomask
	ldx          #0
	jsr          __arg_value4_f0			// value
	.loc 9 42 1
	ldx          #7
	jsr          __var_addr_i0			// representation
	ldx          #__f0
	ldy          #__i0
	jsr          __store_indirect4
	.loc 9 43 1
	ldx          #__i0
	ldy          #__l0
	jsr          __load_indirect4
	lda         __l0+3
	sta         __l1
	lda          #0
	stz         __l1+1
	stz         __l1+2
	stz         __l1+3
	ldx          #7
.__davecc_signbit_label_39:
	lsr         __l1+3
	ror         __l1+2
	ror         __l1+1
	ror         __l1
	dex         
	bne         .__davecc_signbit_label_39
	ldx          #1
	lda         __l1
	ora         __l1+1
	ora         __l1+2
	ora         __l1+3
	bne         .__davecc_signbit_label_50
	dex         
.__davecc_signbit_label_50:
	txa         
	sta         __i0
	stz         __i0+1
	lda         #__i0
	jsr         __result2
	ldy          #16
	jmp          __leave_leaf_nomask
.func_end___davecc_signbit:
	.size __davecc_signbit, .func_end___davecc_signbit-__davecc_signbit

	.section ".text.sinh", "ax", @progbits
	.global sinh
	.type sinh, @function

sinh:
	lda          #11
	jsr          __enter_res
	.byte        0x00,0x00,0x02		// Save mask i:0 b:0 l:0 x:0 f:2 
	.loc 9 55 25
	ldx          #0
	jsr          __arg_value4_f1			// x
	jsr         __pushf1
	ldx         #__f2
	ldy          #0
	jsr         exp
	ldx          #12
	jsr          __load_result
	.loc 9 56 26
	ldx          #3
.sinh_label_31:
	lda         __f1, X
	sta         __f0, X
	dex         
	bpl         .sinh_label_31
	ldy          #3
	lda         __f0+3
	eor          #128
	sta         __f0+3
	jsr         __pushf0
	ldx         #__f0
	ldy          #0
	jsr         exp
	ldx          #12
	jsr          __load_result
	.loc 9 57 1
	lda          #__f1
	ldx          #__f2
	ldy          #__f0
	jsr         __fsub
	ldx          #3
.sinh_label_67:
	lda         .lit.20, X
	sta         __f0, X
	dex         
	bpl         .sinh_label_67
	lda          #__f2
	ldx          #__f1
	ldy          #__f0
	jsr         __fdiv
	ldx          #12
	jsr          __load_result
	lda         #__f2
	jsr         __result4
	ldy          #18
	jmp          __leave
.func_end_sinh:
	.size sinh, .func_end_sinh-sinh

	.section ".text.cosh", "ax", @progbits
	.global cosh
	.type cosh, @function

cosh:
	lda          #11
	jsr          __enter_res
	.byte        0x00,0x00,0x02		// Save mask i:0 b:0 l:0 x:0 f:2 
	.loc 9 61 25
	ldx          #0
	jsr          __arg_value4_f1			// x
	jsr         __pushf1
	ldx         #__f2
	ldy          #0
	jsr         exp
	ldx          #12
	jsr          __load_result
	.loc 9 62 26
	ldx          #3
.cosh_label_31:
	lda         __f1, X
	sta         __f0, X
	dex         
	bpl         .cosh_label_31
	ldy          #3
	lda         __f0+3
	eor          #128
	sta         __f0+3
	jsr         __pushf0
	ldx         #__f0
	ldy          #0
	jsr         exp
	ldx          #12
	jsr          __load_result
	.loc 9 63 1
	lda          #__f1
	ldx          #__f2
	ldy          #__f0
	jsr         __fadd
	ldx          #3
.cosh_label_67:
	lda         .lit.23, X
	sta         __f0, X
	dex         
	bpl         .cosh_label_67
	lda          #__f2
	ldx          #__f1
	ldy          #__f0
	jsr         __fdiv
	ldx          #12
	jsr          __load_result
	lda         #__f2
	jsr         __result4
	ldy          #18
	jmp          __leave
.func_end_cosh:
	.size cosh, .func_end_cosh-cosh

	.section ".text.tanh", "ax", @progbits
	.global tanh
	.type tanh, @function

tanh:
	lda          #11
	jsr          __enter_res
	.byte        0x00,0x00,0x03		// Save mask i:0 b:0 l:0 x:0 f:3 
	.loc 9 67 25
	ldx          #0
	jsr          __arg_value4_f1			// x
	jsr         __pushf1
	ldx         #__f2
	ldy          #0
	jsr         exp
	ldx          #12
	jsr          __load_result
	.loc 9 68 26
	ldx          #3
.tanh_label_30:
	lda         __f1, X
	sta         __f0, X
	dex         
	bpl         .tanh_label_30
	ldy          #3
	lda         __f0+3
	eor          #128
	sta         __f0+3
	jsr         __pushf0
	ldx         #__f0
	ldy          #0
	jsr         exp
	ldx          #12
	jsr          __load_result
	.loc 9 69 1
	lda          #__f1
	ldx          #__f2
	ldy          #__f0
	jsr         __fsub
	lda          #__f3
	ldx          #__f2
	ldy          #__f0
	jsr         __fadd
	lda          #__f0
	ldx          #__f1
	ldy          #__f3
	jsr         __fdiv
	ldx          #12
	jsr          __load_result
	lda         #__f0
	jsr         __result4
	ldy          #18
	jmp          __leave
.func_end_tanh:
	.size tanh, .func_end_tanh-tanh

	.section ".text.asinh", "ax", @progbits
	.global asinh
	.type asinh, @function

asinh:
	lda          #11
	jsr          __enter_res
	.byte        0x01,0x00,0x03		// Save mask i:1 b:0 l:0 x:0 f:3 
	ldx          #0
	jsr          __arg_value4_f0			// x
	.loc 9 73 1
	ldx          #3
.asinh_label_17:
	lda         __f0, X
	sta         __f1, X
	dex         
	bpl         .asinh_label_17
	ldx          #7
	jsr          __var_addr_i4			// __invented__5
	ldx          #__f1
	ldy          #__i4
	jsr          __store_indirect4
	lda          #__f2
	ldx          #__f1
	ldy          #__f1
	jsr         __fmul
	ldx          #3
.asinh_label_46:
	lda         .lit.26, X
	sta         __f1, X
	dex         
	bpl         .asinh_label_46
	lda          #__f3
	ldx          #__f2
	ldy          #__f1
	jsr         __fadd
	jsr         __pushf3
	ldx         #__f1
	ldy          #0
	jsr         sqrt
	ldx          #12
	jsr          __load_result
	ldx          #__i4
	ldy          #__f0
	jsr          __load_indirect4
	lda          #__f2
	ldx          #__f0
	ldy          #__f1
	jsr         __fadd
	jsr         __pushf2
	ldx         #__f0
	ldy          #0
	jsr         log
	ldx          #12
	jsr          __load_result
	ldx          #12
	jsr          __load_result
	lda         #__f0
	jsr         __result4
	ldy          #18
	jmp          __leave
.func_end_asinh:
	.size asinh, .func_end_asinh-asinh

	.section ".text.acosh", "ax", @progbits
	.global acosh
	.type acosh, @function

acosh:
	lda          #27
	jsr          __enter_res
	.byte        0x02,0x00,0x03		// Save mask i:2 b:0 l:0 x:0 f:3 
	ldx          #0
	jsr          __arg_value4_f1			// x
	.loc 9 77 14
	ldx          #3
.acosh_label_25:
	lda         .lit.37, X
	sta         __f0, X
	dex         
	bpl         .acosh_label_25
	lda          #__b0
	ldx          #__f1
	ldy          #__f0
	jsr         __cmpltf
	lda         __b0
	beq         .acosh_label_89
	.loc 9 78 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #200
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 9 79 1
	ldx          #3
	lda          #0
.acosh_label_60:
	sta         __f0, X
	dex         
	bpl         .acosh_label_60
	ldx          #3
	lda          #0
.acosh_label_68:
	sta         __f2, X
	dex         
	bpl         .acosh_label_68
	lda          #__f3
	ldx          #__f0
	ldy          #__f2
	jsr         __fdiv
	ldx          #28
	jsr          __load_result
	lda         #__f3
	jsr         __result4
.acosh_label_86:
	ldy          #34
	jmp          __leave
.acosh_label_89:
	.loc 9 81 1
	ldx          #3
.acosh_label_94:
	lda         __f1, X
	sta         __f2, X
	dex         
	bpl         .acosh_label_94
	jsr          __spill4
	.byte __f2
	.byte 0x0f,0x00
	ldx          #7
	jsr          __var_addr_i4			// __invented__7
	ldx          #__f2
	ldy          #__i4
	jsr          __store_indirect4
	ldx          #3
.acosh_label_111:
	lda         .lit.37, X
	sta         __f0, X
	dex         
	bpl         .acosh_label_111
	lda          #__f3
	ldx          #__f2
	ldy          #__f0
	jsr         __fsub
	jsr         __pushf3
	ldx         #__f3
	ldy          #0
	jsr         sqrt
	ldx          #28
	jsr          __load_result
	jsr          __spill4
	.byte __f3
	.byte 0x13,0x00
	ldx          #11
	jsr          __var_addr_i5			// __invented__8
	ldx          #__f3
	ldy          #__i5
	jsr          __store_indirect4
	ldx          #3
.acosh_label_145:
	lda         .lit.37, X
	sta         __f0, X
	dex         
	bpl         .acosh_label_145
	jsr          __reload4
	.byte __f3
	.byte 0x0f,0x00
	lda          #__f2
	ldx          #__f3
	ldy          #__f0
	jsr         __fadd
	jsr         __pushf2
	ldx         #__f0
	ldy          #0
	jsr         sqrt
	ldx          #28
	jsr          __load_result
	jsr          __spill4
	.byte __f0
	.byte 0x17,0x00
	ldx          #__i5
	ldy          #__f2
	jsr          __load_indirect4
	lda          #__f3
	ldx          #__f2
	ldy          #__f0
	jsr         __fmul
	ldx          #__i4
	ldy          #__f2
	jsr          __load_indirect4
	lda          #__f0
	ldx          #__f2
	ldy          #__f3
	jsr         __fadd
	jsr         __pushf0
	ldx         #__f0
	ldy          #0
	jsr         log
	ldx          #28
	jsr          __load_result
	ldx          #28
	jsr          __load_result
	lda         #__f0
	jsr         __result4
	jmp         .acosh_label_86
.func_end_acosh:
	.size acosh, .func_end_acosh-acosh

	.section ".text.atanh", "ax", @progbits
	.global atanh
	.type atanh, @function

atanh:
	lda          #24
	jsr          __enter_res
	.byte        0x02,0x00,0x03		// Save mask i:2 b:0 l:0 x:0 f:3 
	ldx          #0
	jsr          __arg_value4_f1			// x
	.loc 9 85 28
	ldx          #3
.atanh_label_29:
	lda         .lit.55, X
	sta         __f0, X
	dex         
	bpl         .atanh_label_29
	ldx          #3
.atanh_label_39:
	lda         __f0, X
	sta         __f2, X
	dex         
	bpl         .atanh_label_39
	ldy          #3
	lda         __f2+3
	eor          #128
	sta         __f2+3
	lda          #__b0
	ldx          #__f2
	ldy          #__f1
	jsr         __cmpgef
	ldx          #4
	jsr          __var_addr_i4			// __invented__10
	lda         __b0
	sta         (__i4)
	lda          #__b0
	ldx          #__f2
	ldy          #__f1
	jsr         __cmpgef
	lda         __b0
	bne         .atanh_label_100
	ldx          #3
.atanh_label_82:
	lda         .lit.55, X
	sta         __f0, X
	dex         
	bpl         .atanh_label_82
	lda          #__b0
	ldx          #__f1
	ldy          #__f0
	jsr         __cmpgef
	lda         __b0
	sta         (__i4)
.atanh_label_100:
	lda         (__i4)
	bne         .atanh_label_428
	jmp         .atanh_label_323
.atanh_label_428:
	.loc 9 86 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #200
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 9 87 1
	ldx          #3
.atanh_label_134:
	lda         .lit.55, X
	sta         __f0, X
	dex         
	bpl         .atanh_label_134
	lda          #__b0
	ldx          #__f1
	ldy          #__f0
	jsr         __cmpeqf
	lda         __b0
	beq         .atanh_label_181
	ldx          #3
.atanh_label_151:
	lda         .lit.55, X
	sta         __f0, X
	dex         
	bpl         .atanh_label_151
	ldx          #3
	lda          #0
.atanh_label_160:
	sta         __f2, X
	dex         
	bpl         .atanh_label_160
	lda          #__f3
	ldx          #__f0
	ldy          #__f2
	jsr         __fdiv
	ldx          #8
	jsr          __var_addr_i0			// __invented__11
	ldx          #__f3
	ldy          #__i0
	jsr          __store_indirect4
	jmp         .atanh_label_307
.atanh_label_181:
	ldx          #3
.atanh_label_186:
	lda         .lit.55, X
	sta         __f0, X
	dex         
	bpl         .atanh_label_186
	ldx          #3
.atanh_label_195:
	lda         __f0, X
	sta         __f2, X
	dex         
	bpl         .atanh_label_195
	ldy          #3
	lda         __f2+3
	eor          #128
	sta         __f2+3
	lda          #__b0
	ldx          #__f1
	ldy          #__f2
	jsr         __cmpeqf
	lda         __b0
	beq         .atanh_label_260
	ldx          #3
.atanh_label_217:
	lda         .lit.55, X
	sta         __f0, X
	dex         
	bpl         .atanh_label_217
	ldx          #3
	lda          #0
.atanh_label_226:
	sta         __f2, X
	dex         
	bpl         .atanh_label_226
	lda          #__f3
	ldx          #__f0
	ldy          #__f2
	jsr         __fdiv
	ldx          #3
.atanh_label_243:
	lda         __f3, X
	sta         __f0, X
	dex         
	bpl         .atanh_label_243
	ldy          #3
	lda         __f0+3
	eor          #128
	sta         __f0+3
	ldx          #12
	jsr          __var_addr_i0			// __invented__12
	ldx          #__f0
	ldy          #__i0
	jsr          __store_indirect4
	bra         .atanh_label_292
.atanh_label_260:
	ldx          #3
	lda          #0
.atanh_label_265:
	sta         __f0, X
	dex         
	bpl         .atanh_label_265
	ldx          #3
	lda          #0
.atanh_label_273:
	sta         __f2, X
	dex         
	bpl         .atanh_label_273
	lda          #__f3
	ldx          #__f0
	ldy          #__f2
	jsr         __fdiv
	ldx          #12
	jsr          __var_addr_i0			// __invented__12
	ldx          #__f3
	ldy          #__i0
	jsr          __store_indirect4
.atanh_label_292:
	ldx          #12
	jsr          __var_addr_i0			// __invented__12
	ldx          #__i0
	ldy          #__f0
	jsr          __load_indirect4
	ldx          #8
	jsr          __var_addr_i0			// __invented__11
	ldx          #__f0
	ldy          #__i0
	jsr          __store_indirect4
.atanh_label_307:
	ldx          #8
	jsr          __var_addr_i0			// __invented__11
	ldx          #__i0
	ldy          #__f0
	jsr          __load_indirect4
	ldx          #25
	jsr          __load_result
	lda         #__f0
	jsr         __result4
.atanh_label_320:
	ldy          #31
	jmp          __leave
.atanh_label_323:
	.loc 9 89 1
	ldx          #16
	jsr          __var_addr_i5			// __invented__13
	ldy          #3
	ldx          #3
.atanh_label_332:
	lda         .lit.53, X
	sta         (__i5), Y
	dey         
	dex         
	bpl         .atanh_label_332
	ldx          #3
.atanh_label_341:
	lda         __f1, X
	sta         __f0, X
	dex         
	bpl         .atanh_label_341
	jsr          __spill4
	.byte __f0
	.byte 0x14,0x00
	ldx          #3
.atanh_label_350:
	lda         .lit.55, X
	sta         __f2, X
	dex         
	bpl         .atanh_label_350
	lda          #__f3
	ldx          #__f2
	ldy          #__f0
	jsr         __fadd
	ldx          #3
.atanh_label_369:
	lda         .lit.55, X
	sta         __f2, X
	dex         
	bpl         .atanh_label_369
	jsr          __reload4
	.byte __f2
	.byte 0x14,0x00
	lda          #__f0
	ldx          #__f2
	ldy          #__f2
	jsr         __fsub
	lda          #__f2
	ldx          #__f3
	ldy          #__f0
	jsr         __fdiv
	jsr         __pushf2
	ldx         #__f0
	ldy          #0
	jsr         log
	ldx          #25
	jsr          __load_result
	ldx          #__i5
	ldy          #__f2
	jsr          __load_indirect4
	lda          #__f3
	ldx          #__f2
	ldy          #__f0
	jsr         __fmul
	ldx          #25
	jsr          __load_result
	lda         #__f3
	jsr         __result4
	jmp         .atanh_label_320
.func_end_atanh:
	.size atanh, .func_end_atanh-atanh

	.section ".text.exp2", "ax", @progbits
	.global exp2
	.type exp2, @function

exp2:
	lda          #7
	jsr          __enter_res
	.byte        0x00,0x00,0x01		// Save mask i:0 b:0 l:0 x:0 f:1 
	ldx          #0
	jsr          __arg_value4_f0			// x
	.loc 9 92 25
	jsr         __pushf0
	ldx         #%lo(.lit.58)
	ldy         #%hi(.lit.58)
	jsr         __push4xy
	ldx         #__f1
	ldy          #0
	jsr         pow
	ldx          #8
	jsr          __load_result
	ldx          #8
	jsr          __load_result
	lda         #__f1
	jsr         __result4
	ldy          #14
	jmp          __leave
.func_end_exp2:
	.size exp2, .func_end_exp2-exp2

	.section ".text.expm1", "ax", @progbits
	.global expm1
	.type expm1, @function

expm1:
	lda          #7
	jsr          __enter_res
	.byte        0x00,0x00,0x02		// Save mask i:0 b:0 l:0 x:0 f:2 
	ldx          #0
	jsr          __arg_value4_f0			// x
	.loc 9 93 26
	jsr         __pushf0
	ldx         #__f1
	ldy          #0
	jsr         exp
	ldx          #8
	jsr          __load_result
	ldx          #3
.expm1_label_23:
	lda         .lit.61, X
	sta         __f0, X
	dex         
	bpl         .expm1_label_23
	lda          #__f2
	ldx          #__f1
	ldy          #__f0
	jsr         __fsub
	ldx          #8
	jsr          __load_result
	lda         #__f2
	jsr         __result4
	ldy          #14
	jmp          __leave
.func_end_expm1:
	.size expm1, .func_end_expm1-expm1

	.section ".text.log10", "ax", @progbits
	.global log10
	.type log10, @function

log10:
	lda          #7
	jsr          __enter_res
	.byte        0x00,0x00,0x02		// Save mask i:0 b:0 l:0 x:0 f:2 
	ldx          #0
	jsr          __arg_value4_f0			// x
	.loc 9 94 26
	jsr         __pushf0
	ldx         #__f1
	ldy          #0
	jsr         log
	ldx          #8
	jsr          __load_result
	ldx          #3
.log10_label_23:
	lda         .lit.63, X
	sta         __f0, X
	dex         
	bpl         .log10_label_23
	lda          #__f2
	ldx          #__f1
	ldy          #__f0
	jsr         __fdiv
	ldx          #8
	jsr          __load_result
	lda         #__f2
	jsr         __result4
	ldy          #14
	jmp          __leave
.func_end_log10:
	.size log10, .func_end_log10-log10

	.section ".text.log1p", "ax", @progbits
	.global log1p
	.type log1p, @function

log1p:
	lda          #7
	jsr          __enter_res
	.byte        0x00,0x00,0x02		// Save mask i:0 b:0 l:0 x:0 f:2 
	ldx          #0
	jsr          __arg_value4_f0			// x
	.loc 9 95 26
	ldx          #3
.log1p_label_15:
	lda         .lit.64, X
	sta         __f1, X
	dex         
	bpl         .log1p_label_15
	lda          #__f2
	ldx          #__f1
	ldy          #__f0
	jsr         __fadd
	jsr         __pushf2
	ldx         #__f1
	ldy          #0
	jsr         log
	ldx          #8
	jsr          __load_result
	ldx          #8
	jsr          __load_result
	lda         #__f1
	jsr         __result4
	ldy          #14
	jmp          __leave
.func_end_log1p:
	.size log1p, .func_end_log1p-log1p

	.section ".text.log2", "ax", @progbits
	.global log2
	.type log2, @function

log2:
	lda          #7
	jsr          __enter_res
	.byte        0x00,0x00,0x02		// Save mask i:0 b:0 l:0 x:0 f:2 
	ldx          #0
	jsr          __arg_value4_f0			// x
	.loc 9 96 25
	jsr         __pushf0
	ldx         #__f1
	ldy          #0
	jsr         log
	ldx          #8
	jsr          __load_result
	ldx          #3
.log2_label_23:
	lda         .lit.67, X
	sta         __f0, X
	dex         
	bpl         .log2_label_23
	lda          #__f2
	ldx          #__f1
	ldy          #__f0
	jsr         __fdiv
	ldx          #8
	jsr          __load_result
	lda         #__f2
	jsr         __result4
	ldy          #14
	jmp          __leave
.func_end_log2:
	.size log2, .func_end_log2-log2

	.section ".text.ilogb", "ax", @progbits
	.global ilogb
	.type ilogb, @function

ilogb:
	lda          #15
	jsr          __enter_res
	.byte        0x03,0x00,0x02		// Save mask i:3 b:0 l:0 x:0 f:2 
	ldx          #0
	jsr          __arg_value4_f1			// x
	.loc 9 100 101
	ldx          #3
	lda          #0
.ilogb_label_37:
	sta         __f0, X
	dex         
	bpl         .ilogb_label_37
	lda          #__b0
	ldx          #__f1
	ldy          #__f0
	jsr         __cmpeqf
	ldx          #4
	jsr          __var_addr_i4			// __invented__17
	lda         __b0
	sta         (__i4)
	ldx          #3
	lda          #0
.ilogb_label_61:
	sta         __f0, X
	dex         
	bpl         .ilogb_label_61
	lda          #__b0
	ldx          #__f1
	ldy          #__f0
	jsr         __cmpeqf
	lda         __b0
	bne         .ilogb_label_98
	jsr         __pushf1
	ldx         #__i0
	ldy          #0
	jsr         __davecc_fpclassify
	ldx          #1
	lda         __i0
	ora         __i0+1
	beq         .ilogb_label_81
	dex         
.ilogb_label_81:
	txa         
	sta         (__i4)
.ilogb_label_98:
	lda         (__i4)
	sta         __b0
	ldx          #5
	jsr          __var_addr_i5			// __invented__18
	lda         __b0
	sta         (__i5)
	bne         .ilogb_label_145
	jsr         __pushf1
	ldx         #__i0
	ldy          #0
	jsr         __davecc_fpclassify
	ldx          #1
	lda         __i0
	cmp          #1
	bne         .ilogb_label_126
	lda         __i0+1
	beq         .ilogb_label_127
.ilogb_label_126:
	dex         
.ilogb_label_127:
	txa         
	sta         (__i5)
.ilogb_label_145:
	lda         (__i5)
	beq         .ilogb_label_228
	.loc 9 101 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #200
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 9 102 1
	ldx          #3
	lda          #0
.ilogb_label_178:
	sta         __f0, X
	dex         
	bpl         .ilogb_label_178
	lda          #__b0
	ldx          #__f1
	ldy          #__f0
	jsr         __cmpeqf
	lda         __b0
	beq         .ilogb_label_200
	ldx          #7
	jsr          __var_addr_i0			// __invented__19
	lda          #0
	tay         
	sta         (__i0)
	lda          #128
	iny         
	sta         (__i0), Y
	bra         .ilogb_label_211
.ilogb_label_200:
	ldx          #7
	jsr          __var_addr_i0			// __invented__19
	lda          #255
	ldy          #0
	sta         (__i0)
	lda          #127
	iny         
	sta         (__i0), Y
.ilogb_label_211:
	ldx          #7
	jsr          __var_addr_i0			// __invented__19
	lda         (__i0)
	sta         __i1
	ldy          #1
	lda         (__i0), Y
	sta         __i1+1
	ldx          #16
	lda          #__i1
	jsr         __load_result_value2
.ilogb_label_225:
	ldy          #22
	jmp          __leave
.ilogb_label_228:
	.loc 9 104 1
	ldx          #9
	jsr          __var_addr_i0			// exponent
	ldx          #11
	jsr          __var_addr_i6			// __invented__20
	lda         __i0
	sta         (__i6)
	lda         __i0+1
	ldy          #1
	sta         (__i6), Y
	jsr         __pushf1
	ldx         #__f0
	ldy          #0
	jsr         fabs
	ldx          #16
	jsr          __load_result
	lda         (__i6)
	sta         __i0
	ldy          #1
	lda         (__i6), Y
	sta         __i0+1
	jsr         __pushi0
	jsr         __pushf0
	ldx         #__f2
	ldy          #0
	jsr         frexp
	ldx          #16
	jsr          __load_result
	.loc 9 105 1
	ldx          #9
	jsr          __var_value2_i0			// exponent
	sec         
	lda         __i0
	sbc          #1
	sta         __i1
	lda         __i0+1
	sbc          #0
	sta         __i1+1
	ldx          #16
	lda          #__i1
	jsr         __load_result_value2
	bra         .ilogb_label_225
.func_end_ilogb:
	.size ilogb, .func_end_ilogb-ilogb

	.section ".text.logb", "ax", @progbits
	.global logb
	.type logb, @function

logb:
	lda          #7
	jsr          __enter_res
	.byte        0x00,0x00,0x03		// Save mask i:0 b:0 l:0 x:0 f:3 
	ldx          #0
	jsr          __arg_value4_f1			// x
	.loc 9 109 15
	ldx          #3
	lda          #0
.logb_label_23:
	sta         __f0, X
	dex         
	bpl         .logb_label_23
	lda          #__b0
	ldx          #__f1
	ldy          #__f0
	jsr         __cmpeqf
	lda         __b0
	beq         .logb_label_101
	.loc 9 110 1
	lda          #214
	sta         __i0
	lda          #3
	sta         __i0+1
	lda          #15
	ldy          #0
	sta         (__i0)
	tya         
	iny         
	sta         (__i0), Y
	.loc 9 111 1
	ldx          #3
.logb_label_56:
	lda         .lit.90, X
	sta         __f0, X
	dex         
	bpl         .logb_label_56
	ldx          #3
	lda          #0
.logb_label_65:
	sta         __f2, X
	dex         
	bpl         .logb_label_65
	lda          #__f3
	ldx          #__f0
	ldy          #__f2
	jsr         __fdiv
	ldx          #3
.logb_label_83:
	lda         __f3, X
	sta         __f0, X
	dex         
	bpl         .logb_label_83
	ldy          #3
	lda         __f0+3
	eor          #128
	sta         __f0+3
	ldx          #8
	jsr          __load_result
	lda         #__f0
	jsr         __result4
.logb_label_98:
	ldy          #14
	jmp          __leave
.logb_label_101:
	.loc 9 113 46
	jsr         __pushf1
	ldx         #__i0
	ldy          #0
	jsr         __davecc_fpclassify
	lda         __i0
	cmp          #1
	bne         .logb_label_148
	lda         __i0+1
	bne         .logb_label_148
	.loc 9 113 46
	ldx          #3
.logb_label_122:
	lda         .lit.90, X
	sta         __f0, X
	dex         
	bpl         .logb_label_122
	ldx          #3
	lda          #0
.logb_label_131:
	sta         __f2, X
	dex         
	bpl         .logb_label_131
	lda          #__f3
	ldx          #__f0
	ldy          #__f2
	jsr         __fdiv
	ldx          #8
	jsr          __load_result
	lda         #__f3
	jsr         __result4
	bra         .logb_label_98
.logb_label_148:
	.loc 9 114 46
	jsr         __pushf1
	ldx         #__i0
	ldy          #0
	jsr         __davecc_fpclassify
	lda         __i0
	ora         __i0+1
	bne         .logb_label_164
	.loc 9 114 46
	ldx          #8
	jsr          __load_result
	lda         #__f1
	jsr         __result4
	bra         .logb_label_98
.logb_label_164:
	.loc 9 115 1
	jsr         __pushf1
	ldx         #__i0
	ldy          #0
	jsr         ilogb
	lda          #__f0
	ldx          #__i0
	jsr         __i2tof
	ldx          #8
	jsr          __load_result
	lda         #__f0
	jsr         __result4
	bra         .logb_label_98
.func_end_logb:
	.size logb, .func_end_logb-logb

	.section ".text.cbrt", "ax", @progbits
	.global cbrt
	.type cbrt, @function

cbrt:
	lda          #11
	jsr          __enter_res
	.byte        0x00,0x00,0x03		// Save mask i:0 b:0 l:0 x:0 f:3 
	ldx          #0
	jsr          __arg_value4_f1			// x
	.loc 9 119 1
	ldx          #3
	lda          #0
.cbrt_label_23:
	sta         __f0, X
	dex         
	bpl         .cbrt_label_23
	lda          #__b0
	ldx          #__f1
	ldy          #__f0
	jsr         __cmpltf
	lda         __b0
	beq         .cbrt_label_109
	ldx          #3
.cbrt_label_40:
	lda         .lit.100, X
	sta         __f0, X
	dex         
	bpl         .cbrt_label_40
	ldx          #3
.cbrt_label_49:
	lda         .lit.101, X
	sta         __f2, X
	dex         
	bpl         .cbrt_label_49
	lda          #__f3
	ldx          #__f0
	ldy          #__f2
	jsr         __fdiv
	ldx          #3
.cbrt_label_66:
	lda         __f1, X
	sta         __f0, X
	dex         
	bpl         .cbrt_label_66
	ldy          #3
	lda         __f0+3
	eor          #128
	sta         __f0+3
	jsr         __pushf3
	jsr         __pushf0
	ldx         #__f2
	ldy          #0
	jsr         pow
	ldx          #12
	jsr          __load_result
	ldx          #3
.cbrt_label_92:
	lda         __f2, X
	sta         __f0, X
	dex         
	bpl         .cbrt_label_92
	ldy          #3
	lda         __f0+3
	eor          #128
	sta         __f0+3
	ldx          #7
	jsr          __var_addr_i0			// __invented__25
	ldx          #__f0
	ldy          #__i0
	jsr          __store_indirect4
	bra         .cbrt_label_152
.cbrt_label_109:
	ldx          #3
.cbrt_label_114:
	lda         .lit.100, X
	sta         __f0, X
	dex         
	bpl         .cbrt_label_114
	ldx          #3
.cbrt_label_123:
	lda         .lit.101, X
	sta         __f2, X
	dex         
	bpl         .cbrt_label_123
	lda          #__f3
	ldx          #__f0
	ldy          #__f2
	jsr         __fdiv
	jsr         __pushf3
	jsr         __pushf1
	ldx         #__f2
	ldy          #0
	jsr         pow
	ldx          #12
	jsr          __load_result
	ldx          #7
	jsr          __var_addr_i0			// __invented__25
	ldx          #__f2
	ldy          #__i0
	jsr          __store_indirect4
.cbrt_label_152:
	ldx          #7
	jsr          __var_addr_i0			// __invented__25
	ldx          #__i0
	ldy          #__f0
	jsr          __load_indirect4
	ldx          #12
	jsr          __load_result
	lda         #__f0
	jsr         __result4
	ldy          #18
	jmp          __leave
.func_end_cbrt:
	.size cbrt, .func_end_cbrt-cbrt

	.section ".text.hypot", "ax", @progbits
	.global hypot
	.type hypot, @function

hypot:
	lda          #23
	jsr          __enter_res
	.byte        0x01,0x00,0x03		// Save mask i:1 b:0 l:0 x:0 f:3 
	.loc 9 123 24
	ldx          #0
	jsr          __arg_value4_f0			// x
	jsr         __pushf0
	ldx         #__f2
	ldy          #0
	jsr         fabs
	ldx          #24
	jsr          __load_result
	ldx          #3
.hypot_label_41:
	lda         __f2, X
	sta         __f1, X
	dex         
	bpl         .hypot_label_41
	.loc 9 124 25
	ldx          #4
	jsr          __arg_value4_f0			// y
	jsr         __pushf0
	ldx         #__f0
	ldy          #0
	jsr         fabs
	ldx          #24
	jsr          __load_result
	lda          #__f0
	ldx          #7
	jsr         __set_var_value4
	.loc 9 126 23
	lda          #__b0
	ldx          #__f2
	ldy          #__f0
	jsr         __cmpltf
	lda         __b0
	beq         .hypot_label_110
	.loc 9 127 26
	ldx          #3
.hypot_label_82:
	lda         __f1, X
	sta         __f0, X
	dex         
	bpl         .hypot_label_82
	lda          #__f0
	ldx          #11
	jsr         __set_var_value4
	.loc 9 128 1
	ldx          #7
	jsr          __var_value4_f2			// smaller
	ldx          #3
.hypot_label_99:
	lda         __f2, X
	sta         __f1, X
	dex         
	bpl         .hypot_label_99
	.loc 9 129 1
	lda          #__f0
	ldx          #7
	jsr         __set_var_value4
.hypot_label_110:
	.loc 9 131 51
	jsr         __pushf1
	ldx         #__i0
	ldy          #0
	jsr         __davecc_fpclassify
	lda         __i0
	cmp          #1
	bne         .hypot_label_162
	lda         __i0+1
	bne         .hypot_label_162
	.loc 9 131 51
	ldx          #3
.hypot_label_132:
	lda         .lit.113, X
	sta         __f0, X
	dex         
	bpl         .hypot_label_132
	ldx          #3
	lda          #0
.hypot_label_141:
	sta         __f2, X
	dex         
	bpl         .hypot_label_141
	lda          #__f3
	ldx          #__f0
	ldy          #__f2
	jsr         __fdiv
	ldx          #24
	jsr          __load_result
	lda         #__f3
	jsr         __result4
.hypot_label_159:
	ldy          #34
	jmp          __leave
.hypot_label_162:
	.loc 9 132 20
	ldx          #3
	lda          #0
.hypot_label_169:
	sta         __f0, X
	dex         
	bpl         .hypot_label_169
	lda          #__b0
	ldx          #__f1
	ldy          #__f0
	jsr         __cmpeqf
	lda         __b0
	beq         .hypot_label_194
	.loc 9 132 20
	ldx          #3
	lda          #0
.hypot_label_186:
	sta         __f0, X
	dex         
	bpl         .hypot_label_186
	ldx          #24
	jsr          __load_result
	lda         #__f0
	jsr         __result4
	bra         .hypot_label_159
.hypot_label_194:
	.loc 9 133 1
	ldx          #7
	jsr          __var_value4_f0			// smaller
	ldx          #3
.hypot_label_201:
	lda         __f1, X
	sta         __f2, X
	dex         
	bpl         .hypot_label_201
	lda          #__f3
	ldx          #__f0
	ldy          #__f2
	jsr         __fdiv
	lda          #__f3
	ldx          #15
	jsr         __set_var_value4
	.loc 9 134 1
	ldx          #19
	jsr          __var_addr_i4			// __invented__27
	ldx          #__f2
	ldy          #__i4
	jsr          __store_indirect4
	lda          #__f0
	ldx          #__f3
	ldy          #__f3
	jsr         __fmul
	ldx          #3
.hypot_label_245:
	lda         .lit.113, X
	sta         __f2, X
	dex         
	bpl         .hypot_label_245
	lda          #__f3
	ldx          #__f2
	ldy          #__f0
	jsr         __fadd
	jsr         __pushf3
	ldx         #__f0
	ldy          #0
	jsr         sqrt
	ldx          #24
	jsr          __load_result
	ldx          #__i4
	ldy          #__f2
	jsr          __load_indirect4
	lda          #__f3
	ldx          #__f2
	ldy          #__f0
	jsr         __fmul
	ldx          #24
	jsr          __load_result
	lda         #__f3
	jsr         __result4
	jmp         .hypot_label_159
.func_end_hypot:
	.size hypot, .func_end_hypot-hypot

	.section ".text.erf", "ax", @progbits
	.global erf
	.type erf, @function

erf:
	lda          #51
	jsr          __enter_res
	.byte        0x03,0x00,0x03		// Save mask i:3 b:0 l:0 x:0 f:3 
	ldx          #0
	jsr          __arg_value4_f1			// x
	.loc 9 138 35
	ldx          #3
	lda          #0
.erf_label_42:
	sta         __f0, X
	dex         
	bpl         .erf_label_42
	lda          #__b0
	ldx          #__f1
	ldy          #__f0
	jsr         __cmpltf
	lda         __b0
	beq         .erf_label_86
	ldx          #3
.erf_label_59:
	lda         .lit.128, X
	sta         __f0, X
	dex         
	bpl         .erf_label_59
	ldx          #3
.erf_label_68:
	lda         __f0, X
	sta         __f2, X
	dex         
	bpl         .erf_label_68
	ldy          #3
	lda         __f2+3
	eor          #128
	sta         __f2+3
	ldx          #11
	jsr          __var_addr_i0			// __invented__28
	ldx          #__f2
	ldy          #__i0
	jsr          __store_indirect4
	bra         .erf_label_100
.erf_label_86:
	ldx          #11
	jsr          __var_addr_i0			// __invented__28
	ldy          #3
	ldx          #3
.erf_label_94:
	lda         .lit.128, X
	sta         (__i0), Y
	dey         
	dex         
	bpl         .erf_label_94
.erf_label_100:
	ldx          #11
	jsr          __var_addr_i0			// __invented__28
	ldx          #__i0
	ldy          #__f2
	jsr          __load_indirect4
	jsr          __spill4
	.byte __f2
	.byte 0x2b,0x00
	.loc 9 139 26
	jsr         __pushf1
	ldx         #__f3
	ldy          #0
	jsr         fabs
	ldx          #52
	jsr          __load_result
	jsr          __spill4
	.byte __f3
	.byte 0x27,0x00
	.loc 9 140 46
	ldx          #3
.erf_label_120:
	lda         .lit.122, X
	sta         __f0, X
	dex         
	bpl         .erf_label_120
	jsr          __reload4
	.byte __f0
	.byte 0x27,0x00
	lda          #__f3
	ldx          #__f0
	ldy          #__f0
	jsr         __fmul
	ldx          #3
.erf_label_139:
	lda         .lit.128, X
	sta         __f0, X
	dex         
	bpl         .erf_label_139
	lda          #__f2
	ldx          #__f0
	ldy          #__f3
	jsr         __fadd
	ldx          #3
.erf_label_158:
	lda         .lit.128, X
	sta         __f0, X
	dex         
	bpl         .erf_label_158
	lda          #__f3
	ldx          #__f0
	ldy          #__f2
	jsr         __fdiv
	jsr          __spill4
	.byte __f3
	.byte 0x2f,0x00
	.loc 9 143 36
	ldx          #3
.erf_label_178:
	lda         .lit.123, X
	sta         __f0, X
	dex         
	bpl         .erf_label_178
	lda          #__f2
	ldx          #__f0
	ldy          #__f3
	jsr         __fmul
	ldx          #3
.erf_label_197:
	lda         .lit.124, X
	sta         __f0, X
	dex         
	bpl         .erf_label_197
	lda          #__f3
	ldx          #__f2
	ldy          #__f0
	jsr         __fsub
	jsr          __reload4
	.byte __f2
	.byte 0x2f,0x00
	lda          #__f0
	ldx          #__f3
	ldy          #__f2
	jsr         __fmul
	ldx          #3
.erf_label_226:
	lda         .lit.125, X
	sta         __f3, X
	dex         
	bpl         .erf_label_226
	lda          #__f2
	ldx          #__f0
	ldy          #__f3
	jsr         __fadd
	jsr          __reload4
	.byte __f2
	.byte 0x2f,0x00
	lda          #__f0
	ldx          #__f2
	ldy          #__f2
	jsr         __fmul
	ldx          #3
.erf_label_254:
	lda         .lit.126, X
	sta         __f3, X
	dex         
	bpl         .erf_label_254
	lda          #__f2
	ldx          #__f0
	ldy          #__f3
	jsr         __fsub
	jsr          __reload4
	.byte __f2
	.byte 0x2f,0x00
	lda          #__f0
	ldx          #__f2
	ldy          #__f2
	jsr         __fmul
	ldx          #3
.erf_label_282:
	lda         .lit.127, X
	sta         __f3, X
	dex         
	bpl         .erf_label_282
	lda          #__f2
	ldx          #__f0
	ldy          #__f3
	jsr         __fadd
	jsr          __reload4
	.byte __f2
	.byte 0x2f,0x00
	lda          #__f0
	ldx          #__f2
	ldy          #__f2
	jsr         __fmul
	.loc 9 144 1
	ldx          #27
	jsr          __var_addr_i4			// __invented__29
	jsr          __reload4
	.byte __f3
	.byte 0x2b,0x00
	ldx          #__f3
	ldy          #__i4
	jsr          __store_indirect4
	ldx          #31
	jsr          __var_addr_i5			// __invented__30
	ldy          #3
	ldx          #3
.erf_label_320:
	lda         .lit.128, X
	sta         (__i5), Y
	dey         
	dex         
	bpl         .erf_label_320
	ldx          #7
	jsr          __var_addr_i6			// __invented__31
	ldx          #__f0
	ldy          #__i6
	jsr          __store_indirect