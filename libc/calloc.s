	.file   "/Users/dallison/Google Drive/c_compiler/libc/calloc.c"
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


	.global calloc
	.type calloc, @function

calloc:
/* @5 */ 	stx         __result
/* @7 */ 	sty         __result+1
/* @8 */ 	ldx          #7
	jsr          __enter
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
/* @17 */ 	ldx          #0
	jsr          __arg_value2_i5			// n
/* @21 */ 	ldx          #2
	jsr          __arg_value2_i6			// m
/* @24 */ 	lda          #__i0
/* @25 */ 	ldx          #__i5
/* @26 */ 	ldy          #__i6
/* @28 */ 	jsr         __umul2
/* @31 */ 	jsr         __pushi0
/* @33 */ 	ldx         #__i4
/* @34 */ 	ldy          #0
/* @35 */ 	jsr         malloc
/* @37 */ 	jsr         __incsp2
/* @39 */ 	lda         __i4
/* @41 */ 	bne         .calloc_label_58
/* @43 */ 	lda         __i4+1
/* @45 */ 	bne         .calloc_label_58
/* @49 */ 	stz         __i0
/* @50 */ 	stz         __i0+1
/* @51 */ 	ldx          #8
	jsr          __load_result
/* @52 */ 	lda         #__i0
/* @54 */ 	jsr         __result2
.calloc_label_55:
/* @56 */ 	ldy          #10
	jmp          __leave
.calloc_label_58:
/* @61 */ 	lda          #__i0
/* @62 */ 	ldx          #__i5
/* @63 */ 	ldy          #__i6
/* @64 */ 	jsr         __umul2
/* @68 */ 	lda         __i0
/* @70 */ 	sta         __mem_size
/* @71 */ 	lda         __i0+1
/* @73 */ 	sta         __mem_size+1
/* @76 */ 	stz         __mem_src
/* @77 */ 	lda         __i4
/* @79 */ 	sta         __mem_dest
/* @80 */ 	lda         __i4+1
/* @82 */ 	sta         __mem_dest+1
/* @84 */ 	jsr         __builtin_memset
/* @85 */ 	ldx          #8
	jsr          __load_result
/* @86 */ 	lda         #__i4
/* @87 */ 	jsr         __result2
/* @88 */ 	bra         .calloc_label_55
.func_end_calloc:
	.size calloc, .func_end_calloc-calloc

	.data
	.section ".rodata", "aMS", @progbits
