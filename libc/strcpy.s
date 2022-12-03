	.file   "/Users/dallison/Google Drive/c_compiler/libc/strcpy.c"
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


	.global strcpy
	.type strcpy, @function

strcpy:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i1			// dest
/* @20 */ 	ldx          #2
	jsr          __arg_value2_i2			// src
/* @21 */ 	lda         __i1
/* @22 */ 	sta         __i0
/* @24 */ 	lda         __i1+1
/* @25 */ 	sta         __i0+1
.strcpy_label_26:
/* @29 */ 	lda         (__i2)
/* @35 */ 	sta         __i3
/* @37 */ 	and          #128
/* @39 */ 	beq         .strcpy_label_38
/* @41 */ 	lda          #255
.strcpy_label_38:
/* @42 */ 	sta         __i3+1
/* @44 */ 	lda         __i3
/* @45 */ 	ora         __i3+1
/* @47 */ 	beq         .strcpy_label_78
/* @50 */ 	lda         __i0
/* @51 */ 	sta         __i3
/* @52 */ 	lda         __i0+1
/* @53 */ 	sta         __i3+1
/* @54 */ 	lda          #__i0
/* @56 */ 	jsr         __rinc21
/* @59 */ 	lda         __i2
/* @60 */ 	sta         __i4
/* @61 */ 	lda         __i2+1
/* @62 */ 	sta         __i4+1
/* @63 */ 	lda          #__i2
/* @64 */ 	jsr         __rinc21
/* @69 */ 	lda         (__i4)
/* @76 */ 	sta         (__i3)
/* @77 */ 	bra         .strcpy_label_26
.strcpy_label_78:
/* @79 */ 	lda          #0
/* @80 */ 	tay         
/* @81 */ 	sta         (__i0)
/* @82 */ 	lda         #__i1
/* @84 */ 	jsr         __result2
/* @86 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_strcpy:
	.size strcpy, .func_end_strcpy-strcpy

	.data
	.section ".rodata", "aMS", @progbits
