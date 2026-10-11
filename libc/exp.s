	.file   "libc/exp.c"
	.file 1 "libc/include/math.h"
	.file 2 "libc/exp.c"
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
	.section ".text.exp", "ax", @progbits
	.global exp
	.type exp, @function

exp:
	lda          #26
	jsr          __enter_leaf_res
	.byte        0x00,0x00,0x03		// Save mask i:0 b:0 l:0 x:0 f:3 
	.loc 3 22 13
	ldx          #0
	jsr          __arg_value4_f1			// x
	lda          #__b0
	ldx          #__f1
	ldy          #__f1
	jsr         __cmpnef
	lda         __b0
	beq         .exp_label_59
	.loc 3 23 1
	ldx          #0
	jsr          __arg_value4_f1			// x
	lda         #__f1
	jsr         __result4
.exp_label_56:
	ldy          #33
	jmp          __leave_leaf
.exp_label_59:
	.loc 3 25 15
	ldx          #0
	jsr          __arg_value4_f1			// x
	ldx          #3
	lda          #0
.exp_label_70:
	sta         __f2, X
	dex         
	bpl         .exp_label_70
	lda          #__b0
	ldx          #__f1
	ldy          #__f2
	jsr         __cmpeqf
	lda         __b0
	beq         .exp_label_97
	.loc 3 26 1
	ldx          #3
.exp_label_89:
	lda         .lit.13, X
	sta         __f1, X
	dex         
	bpl         .exp_label_89
	lda         #__f1
	jsr         __result4
	bra         .exp_label_56
.exp_label_97:
	.loc 3 36 15
	ldx          #0
	jsr          __arg_value4_f1			// x
	ldx          #3
.exp_label_106:
	lda         .lit.4, X
	sta         __f2, X
	dex         
	bpl         .exp_label_106
	lda          #__b0
	ldx          #__f2
	ldy          #__f1
	jsr         __cmpltf
	lda         __b0
	beq         .exp_label_139
	.loc 3 37 1
	ldx          #0
	jsr          __arg_value4_f1			// x
	lda          #__f2
	ldx          #__f1
	ldy          #__f1
	jsr         __fmul
	lda         #__f2
	jsr         __result4
	bra         .exp_label_56
.exp_label_139:
	.loc 3 39 17
	ldx          #0
	jsr          __arg_value4_f1			// x
	ldx          #3
.exp_label_147:
	lda         .lit.5, X
	sta         __f2, X
	dex         
	bpl         .exp_label_147
	ldx          #3
.exp_label_156:
	lda         __f2, X
	sta         __f3, X
	dex         
	bpl         .exp_label_156
	ldy          #3
	lda         __f3+3
	eor          #128
	sta         __f3+3
	lda          #__b0
	ldx          #__f1
	ldy          #__f3
	jsr         __cmpltf
	lda         __b0
	beq         .exp_label_189
	.loc 3 40 1
	ldx          #3
	lda          #0
.exp_label_182:
	sta         __f1, X
	dex         
	bpl         .exp_label_182
	lda         #__f1
	jsr         __result4
	jmp         .exp_label_56
.exp_label_189:
	.loc 3 44 1
	ldx          #0
	jsr          __arg_value4_f1			// x
	ldx          #3
.exp_label_198:
	lda         .lit.7, X
	sta         __f2, X
	dex         
	bpl         .exp_label_198
	lda          #__f3
	ldx          #__f1
	ldy          #__f2
	jsr         __fmul
	ldx          #3
.exp_label_214:
	lda         __f3, X
	sta         __f0, X
	dex         
	bpl         .exp_label_214
	.loc 3 45 1
	lda          #__i0
	ldx          #__f3
	jsr         __ftoi2
	lda         __i0
	sta         __i1
	lda         __i0+1
	sta         __i1+1
	.loc 3 46 42
	ldx          #3
	lda          #0
.exp_label_242:
	sta         __f1, X
	dex         
	bpl         .exp_label_242
	lda          #__b0
	ldx          #__f3
	ldy          #__f1
	jsr         __cmpltf
	ldx          #4
	jsr          __var_addr_i0			// __invented__6
	lda         __b0
	sta 