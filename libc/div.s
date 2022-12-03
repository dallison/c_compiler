	.file   "/Users/dallison/Google Drive/c_compiler/libc/div.c"
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


	.global div
	.type div, @function

div:
/* @1 */ 	ldx          #11
	jsr          __enter_nomask
/* @6 */ 	lda          #__i0			// numer
	ldx          #0
	jsr          __arg_value2
/* @10 */ 	lda          #__i1			// denom
	ldx          #2
	jsr          __arg_value2
/* @13 */ 	lda          #__i2			// struct return address
	ldx          #0
	jsr          __arg_value2
/* @14 */ 	jsr         __pushi1
/* @15 */ 	jsr         __pushi0
/* @17 */ 	lda          #4
/* @19 */ 	sta         __mem_size
/* @21 */ 	lda         __i2
/* @23 */ 	sta         __mem_src
/* @25 */ 	lda         __i2+1
/* @27 */ 	sta         __mem_src+1
/* @29 */ 	jsr         __pushmem1
/* @30 */ 	jsr         __cdivmod2
/* @32 */ 	jsr         __pullxy
/* @33 */ 	stx         __i2
/* @34 */ 	sty         __i2+1
/* @36 */ 	jsr         __incsp6
.div_label_37:
/* @38 */ 	ldy          #14
	jmp          __leave_void_nomask
.func_end_div:
	.size div, .func_end_div-div

	.global ldiv
	.type ldiv, @function

ldiv:
/* @3 */ 	ldx          #13
	jsr          __enter_leaf
	.byte        0x00,0x02,0x00		// Save mask i:0 b:0 l:1 x:0 f:0 
/* @9 */ 	lda          #__l0			// numer
	ldx          #0
	jsr          __arg_value4
/* @13 */ 	lda          #__l1			// denom
	ldx          #4
	jsr          __arg_value4
/* @14 */ 	lda          #__i0			// struct return address
	ldx          #0
	jsr          __arg_value2
/* @17 */ 	lda          #__l2
/* @18 */ 	ldx          #__l0
/* @19 */ 	ldy          #__l1
/* @21 */ 	jsr         __sdiv4
/* @25 */ 	ldy          #3
.ldiv_label_26:
/* @28 */ 	lda         __l2, Y
/* @29 */ 	sta         (__i0), Y
/* @30 */ 	dey         
/* @31 */ 	bpl         .ldiv_label_26
/* @34 */ 	lda          #__l2
/* @35 */ 	ldx          #__l0
/* @36 */ 	ldy          #__l1
/* @38 */ 	jsr         __smod4
/* @41 */ 	lda          #__i1			// r
	ldx          #11
	jsr          __var_addr
/* @45 */ 	lda         __l2
/* @47 */ 	ldy          #4
/* @48 */ 	sta         (__i1), Y
/* @50 */ 	lda         __l2+1
/* @52 */ 	iny         
/* @53 */ 	sta         (__i1), Y
/* @55 */ 	lda         __l2+2
/* @57 */ 	iny         
/* @58 */ 	sta         (__i1), Y
/* @59 */ 	lda         __l2+3
/* @61 */ 	iny         
/* @62 */ 	sta         (__i1), Y
.ldiv_label_63:
/* @64 */ 	ldy          #16
	jmp          __leave_leaf_void
.func_end_ldiv:
	.size ldiv, .func_end_ldiv-ldiv

	.global lldiv
	.type lldiv, @function

lldiv:
/* @3 */ 	ldx          #21
	jsr          __enter_leaf
	.byte        0x00,0x40,0x00		// Save mask i:0 b:0 l:0 x:2 f:0 
/* @9 */ 	lda          #__x0			// numer
	ldx          #0
	jsr          __arg_value8
/* @12 */ 	lda          #__i0			// struct return address
	ldx          #0
	jsr          __arg_value2
/* @14 */ 	lda          #__x1			// denom
	ldx          #8
	jsr          __arg_value8
/* @19 */ 	lda          #__x2
/* @20 */ 	ldx          #__x0
/* @21 */ 	ldy          #__x1
/* @23 */ 	jsr         __sdiv8
/* @27 */ 	ldy          #7
.lldiv_label_28:
/* @30 */ 	lda         __x2, Y
/* @31 */ 	sta         (__i0), Y
/* @32 */ 	dey         
/* @33 */ 	bpl         .lldiv_label_28
/* @35 */ 	lda          #__x1			// denom
	ldx          #8
	jsr          __arg_value8
/* @40 */ 	lda          #__x2
/* @41 */ 	ldx          #__x0
/* @42 */ 	ldy          #__x1
/* @44 */ 	jsr         __smod8
/* @47 */ 	lda          #__i1			// r
	ldx          #19
	jsr          __var_addr
/* @51 */ 	lda         __x2
/* @53 */ 	ldy          #8
/* @54 */ 	sta         (__i1), Y
/* @56 */ 	lda         __x2+1
/* @58 */ 	iny         
/* @59 */ 	sta         (__i1), Y
/* @61 */ 	lda         __x2+2
/* @63 */ 	iny         
/* @64 */ 	sta         (__i1), Y
/* @66 */ 	lda         __x2+3
/* @68 */ 	iny         
/* @69 */ 	sta         (__i1), Y
/* @71 */ 	lda         __x2+4
/* @73 */ 	iny         
/* @74 */ 	sta         (__i1), Y
/* @76 */ 	lda         __x2+5
/* @78 */ 	iny         
/* @79 */ 	sta         (__i1), Y
/* @81 */ 	lda         __x2+6
/* @83 */ 	iny         
/* @84 */ 	sta         (__i1), Y
/* @85 */ 	lda         __x2+7
/* @87 */ 	iny         
/* @88 */ 	sta         (__i1), Y
.lldiv_label_89:
/* @90 */ 	ldy          #24
	jmp          __leave_leaf_void
.func_end_lldiv:
	.size lldiv, .func_end_lldiv-lldiv

	.data
	.section ".rodata", "aMS", @progbits
