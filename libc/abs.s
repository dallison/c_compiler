	.file   "/Users/dallison/Google Drive/c_compiler/libc/abs.c"
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


	.global abs
	.type abs, @function

abs:
/* @3 */ 	stx         __result
/* @5 */ 	sty         __result+1
/* @6 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @10 */ 	ldx          #0
	jsr          __arg_value2_i0			// j
/* @13 */ 	lda         __i0+1
/* @14 */ 	bpl         .abs_label_32
/* @17 */ 	sec         
/* @19 */ 	lda          #0
/* @20 */ 	sbc         __i0
/* @21 */ 	sta         __i1
/* @22 */ 	lda          #0
/* @23 */ 	sbc         __i0+1
/* @24 */ 	sta         __i1+1
/* @26 */ 	lda         #__i1
/* @28 */ 	jsr         __result2
.abs_label_29:
/* @30 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.abs_label_32:
/* @33 */ 	lda         #__i0
/* @34 */ 	jsr         __result2
/* @35 */ 	bra         .abs_label_29
.func_end_abs:
	.size abs, .func_end_abs-abs

	.global labs
	.type labs, @function

labs:
/* @3 */ 	stx         __result
/* @5 */ 	sty         __result+1
/* @6 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @10 */ 	ldx          #0
	jsr          __arg_value4_l0			// j
/* @13 */ 	lda         __l0+3
/* @14 */ 	bpl         .labs_label_37
/* @17 */ 	sec         
/* @19 */ 	ldy          #4
/* @21 */ 	ldx          #0
.labs_label_22:
/* @24 */ 	lda          #0
/* @25 */ 	sbc         __l0, X
/* @26 */ 	sta         __l1, X
/* @27 */ 	inx         
/* @28 */ 	dey         
/* @29 */ 	bne         .labs_label_22
/* @31 */ 	lda         #__l1
/* @33 */ 	jsr         __result4
.labs_label_34:
/* @35 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.labs_label_37:
/* @38 */ 	lda         #__l0
/* @39 */ 	jsr         __result4
/* @40 */ 	bra         .labs_label_34
.func_end_labs:
	.size labs, .func_end_labs-labs

	.global llabs
	.type llabs, @function

llabs:
/* @3 */ 	stx         __result
/* @5 */ 	sty         __result+1
/* @6 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x00,0x20,0x00		// Save mask i:0 b:0 l:0 x:1 f:0 
/* @10 */ 	ldx          #0
	jsr          __arg_value8_x0			// j
/* @13 */ 	lda         __x0+7
/* @14 */ 	bpl         .llabs_label_37
/* @17 */ 	sec         
/* @19 */ 	ldy          #8
/* @21 */ 	ldx          #0
.llabs_label_22:
/* @24 */ 	lda          #0
/* @25 */ 	sbc         __x0, X
/* @26 */ 	sta         __x1, X
/* @27 */ 	inx         
/* @28 */ 	dey         
/* @29 */ 	bne         .llabs_label_22
/* @31 */ 	lda         #__x1
/* @33 */ 	jsr         __result8
.llabs_label_34:
/* @35 */ 	ldy          #8
	jmp          __leave_leaf
.llabs_label_37:
/* @38 */ 	lda         #__x0
/* @39 */ 	jsr         __result8
/* @40 */ 	bra         .llabs_label_34
.func_end_llabs:
	.size llabs, .func_end_llabs-llabs

	.data
	.section ".rodata", "aMS", @progbits
