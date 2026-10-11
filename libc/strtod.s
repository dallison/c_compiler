	.file   "libc/strtod.c"
	.file 1 "libc/include/_fpfuncs.h"
	.file 2 "libc/include/stdint.h"
	.file 3 "libc/include/limits.h"
	.file 4 "libc/include/stdbool.h"
	.file 5 "libc/include/ctype.h"
	.file 6 "libc/include/stddef.h"
	.file 7 "libc/include/string.h"
	.file 8 "libc/strtod.c"
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
	.section ".text.strtod", "ax", @progbits
	.global strtod
	.type strtod, @function

strtod:
	lda          #68
	jsr          __enter_res
	.byte        0x45,0x20,0x00		// Save mask i:5 b:2 l:0 x:1 f:0 
	ldx          #0
	jsr          __arg_value2_i5			// str
	.loc 9 47 21
	ldx          #64
	jsr          __var_addr_i2			// fx
	jsr         __zeromem1
	.byte        __i2, 32
	.loc 9 48 20
	ldx          #19
	jsr          __var_addr_i2			// n
	jsr         __zeromem1
	.byte        __i2, 16
	.loc 9 50 21
	stz         __b2
	.loc 9 52 20
	lda         __i5
	sta         __i4
	lda         __i5+1
	sta         __i4+1
	.loc 9 53 21
.strtod_label_99:
	lda         (__i4)
	sta         __i1
	lda          #0
	stz         __i1+1
	ldx         __i1
	ldy         __i1+1
	jsr         __builtin_isspace
	sta         __i2
	stz         __i2+1
	cmp          #0
	beq         .strtod_label_129
	.loc 9 54 1
	lda          #__i4
	jsr         __rinc21
	bra         .strtod_label_99
.strtod_label_129:
	.loc 9 56 16
	lda         (__i4)
	sta         __i1
	stz         __i1+1
	cmp          #45
	bne         .strtod_label_157
	lda         __i1+1
	bne         .strtod_label_157
	.loc 9 57 1
	lda          #128
	sta         __b2
	.loc 9 58 1
	lda          #__i4
	jsr         __rinc21
.strtod_label_157:
	.loc 9 60 16
	lda         (__i4)
	sta         __i1
	stz         __i1+1
	cmp          #43
	bne         .strtod_label_182
	lda         __i1+1
	bne         .strtod_label_182
	.loc 9 61 1
	lda          #__i4
	jsr         __rinc21
.strtod_label_182:
	.loc 9 64 20
	stz         __b3
	.loc 9 68 24
	stz         __i0
	stz         __i0+1
	.loc 9 69 21
.strtod_label_189:
	lda         (__i4)
	sta         __i0
	lda          #0
	stz         __i0+1
	ldx         __i0
	ldy         __i0+1
	jsr         __builtin_isdigit
	sta         __i1
	stz         __i1+1
	cmp          #0
	beq         .strtod_label_312
	.loc 9 70 1
	lda          #1
	sta         __b3
	.loc 9 71 1
	lda         (__i4)
	sta         __i0
	stz         __i0+1
	sec         
	sbc          #48
	sta         __i1
	lda         __i0+1
	sbc          #0
	sta         __i1+1
	lda         __i1
	sta         __x0
	lda         __i1+1
	sta         __x0+1
	and          #128
	beq         .strtod_label_247
	lda          #255
.strtod_label_247:
	ldy          #7
.strtod_label_253:
	sta         __x0, Y
	dey         
	cpy          #1
	bne         .strtod_label_253
	ldx          #19
	jsr          __var_addr_i6			// n
	ldx          #__x0
	ldy          #__i6
	jsr          __store_indirect8
	.loc 9 72 1
	ldx          #64
	jsr          __var_addr_i7			// fx
	clc         
	lda         __i7
	adc          #16
	sta         __i0
	lda         __i7+1
	adc          #0
	sta         __i0+1
	jsr         __pushi0
	jsr         __MultiplyBy10Half
	jsr         __incsp2
	.loc 9 73 1
	clc         
	lda         __i7
	adc          #16
	sta         __i0
	lda         __i7+1
	adc          #0
	sta         __i0+1
	jsr         __pushi6
	jsr         __pushi0
	jsr         __AddHalf
	jsr       