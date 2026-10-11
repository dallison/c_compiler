	.file   "libc/sqrt.c"
	.file 1 "libc/include/math.h"
	.file 2 "libc/sqrt.c"
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
	.section ".text.sqrt", "ax", @progbits
	.global sqrt
	.type sqrt, @function

sqrt:
	lda          #17
	jsr          __enter_leaf_res
	.byte        0x00,0x00,0x03		// Save mask i:0 b:0 l:0 x:0 f:3 
	ldx          #0
	jsr          __arg_value4_f0			// x
	.loc 3 23 13
	ldx          #3
.sqrt_label_28:
	lda         __f0, X
	sta         __f1, X
	dex         
	bpl         .sqrt_label_28
	lda          #__b0
	ldx          #__f1
	ldy          #__f1
	jsr         __cmpnef
	lda         __b0
	beq         .sqrt_label_54
	.loc 3 23 13
	lda         #__f0
	jsr         __result4
.sqrt_label_51:
	ldy          #24
	jmp          __leave_leaf
.sqrt_label_54:
	.loc 3 24 14
	ldx          #3
	lda          #0
.sqrt_label_61:
	sta         __f1, X
	dex         
	bpl         .sqrt_label_61
	lda          #__b0
	ldx          #__f0
	ldy          #__f1
	jsr         __cmpltf
	lda         __b0
	beq         .sqrt_label_116
	.loc 3 24 32
	ldx          #3
.sqrt_label_77:
	lda         __f0, X
	sta         __f1, X
	dex         
	bpl         .sqrt_label_77
	lda          #__f2
	ldx          #__f1
	ldy          #__f1
	jsr         __fsub
	lda          #__f2
	ldx          #7
	jsr         __set_var_value4
	.loc 3 24 34
	lda          #__f1
	ldx          #__f2
	ldy          #__f2
	jsr         __fdiv
	lda         #__f1
	jsr         __result4
	bra         .sqrt_label_51
.sqrt_label_116:
	.loc 3 25 15
	ldx          #3
	lda          #0
.sqrt_label_123:
	sta         __f1, X
	dex         
	bpl         .sqrt_label_123
	lda          #__b0
	ldx          #__f0
	ldy          #__f1
	jsr         __cmpeqf
	lda         __b0
	beq         .sqrt_label_139
	.loc 3 25 15
	lda         #__f0
	jsr         __result4
	bra         .sqrt_label_51
.sqrt_label_139:
	.loc 3 27 19
	ldx          #3
.sqrt_label_145:
	lda         .lit.11, X
	sta         __f1, X
	dex         
	bpl         .sqrt_label_145
	lda          #__f1
	ldx          #11
	jsr         __set_var_value4
	.loc 3 28 18
.sqrt_label_156:
	ldx          #3
.sqrt_label_162:
	lda         .lit.8, X
	sta         __f1, X
	dex         
	bpl         .sqrt_label_162
	lda          #__b0
	ldx          #__f0
	ldy          #__f1
	jsr         __cmpgef
	lda         __b0
	beq         .sqrt_label_221
	.loc 3 28 20
	ldx          #3
.sqrt_label_180:
	lda         .lit.7, X
	sta         __f1, X
	dex         
	bpl         .sqrt_label_180
	lda          #__f2
	ldx          #__f0
	ldy          #__f1
	jsr         __fmul
	ldx          #3
.sqrt_label_196:
	lda         __f2, X
	sta         __f0, X
	dex         
	bpl         .sqrt_label_196
	.loc 3 28 31
	ldx          #11
	jsr          __var_value4_f1			// scale
	lda          #__f2
	ldx          #__f1
	ldy          #__f1
	jsr         __fadd
	lda          #__f2
	ldx          #11
	jsr         __set_var_value4
	bra         .sqrt_label_156
.sqrt_label_221:
	.loc 3 29 18
.sqrt_label_223:
	ldx          #3
.sqrt_label_229:
	lda         .lit.7, X
	sta         __f1, X
	dex         
	bpl         .sqrt_label_229
	lda          #__b0
	ldx          #__f0
	ldy          #__f1
	jsr         __cmpltf
	lda         __b0
	beq         .sqrt_label_293
	.loc 3 29 20
	ldx          #3
.sqrt_label_246:
	lda         .lit.8, X
	sta         __f1, X
	dex      