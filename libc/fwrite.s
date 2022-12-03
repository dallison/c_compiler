	.file   "/Users/dallison/Google Drive/c_compiler/libc/fwrite.c"
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


	.global fwrite
	.type fwrite, @function

fwrite:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x07,0x00,0x00		// Save mask i:7 b:0 l:0 x:0 f:0 
/* @14 */ 	ldx          #6
	jsr          __arg_value2_i4			// stream
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i0			// ptr
/* @25 */ 	ldx          #2
	jsr          __arg_value2_i6			// size
/* @29 */ 	ldx          #4
	jsr          __arg_value2_i7			// n
/* @37 */ 	lda         __i0
/* @38 */ 	sta         __i10
/* @40 */ 	lda         __i0+1
/* @41 */ 	sta         __i10+1
/* @44 */ 	lda          #__i1
/* @45 */ 	ldx          #__i6
/* @46 */ 	ldy          #__i7
/* @48 */ 	jsr         __umul2
/* @51 */ 	lda         __i1
/* @52 */ 	sta         __i5
/* @53 */ 	lda         __i1+1
/* @54 */ 	sta         __i5+1
/* @55 */ 	stz         __i8
/* @56 */ 	stz         __i8+1
.fwrite_label_57:
/* @58 */ 	lda         __i5
/* @59 */ 	ora         __i5+1
/* @61 */ 	beq         .fwrite_label_110
/* @62 */ 	jsr         __pushi4
/* @65 */ 	lda         __i10
/* @66 */ 	sta         __i0
/* @67 */ 	lda         __i10+1
/* @68 */ 	sta         __i0+1
/* @69 */ 	lda          #__i10
/* @71 */ 	jsr         __rinc21
/* @76 */ 	lda         (__i0)
/* @82 */ 	jsr         __pusha
/* @83 */ 	ldx         #__i9
/* @84 */ 	ldy          #0
/* @85 */ 	jsr         fputc
/* @87 */ 	jsr         __incsp4
/* @89 */ 	lda         __i9
/* @90 */ 	cmp          #255
/* @91 */ 	bne         .fwrite_label_103
/* @92 */ 	lda         __i9+1
/* @93 */ 	cmp          #255
/* @94 */ 	bne         .fwrite_label_103
/* @96 */ 	ldx          #8
	jsr          __load_result
/* @97 */ 	lda         #__i7
/* @99 */ 	jsr         __result2
.fwrite_label_100:
/* @101 */ 	ldy          #10
	jmp          __leave
.fwrite_label_103:
/* @104 */ 	lda          #__i8
/* @105 */ 	jsr         __rinc21
/* @106 */ 	lda          #__i5
/* @108 */ 	jsr         __rdec21
/* @109 */ 	bra         .fwrite_label_57
.fwrite_label_110:
/* @113 */ 	lda          #__i0
/* @114 */ 	ldx          #__i8
/* @115 */ 	ldy          #__i6
/* @117 */ 	jsr         __udiv2
/* @119 */ 	ldx          #8
	jsr          __load_result
/* @120 */ 	lda         #__i0
/* @121 */ 	jsr         __result2
/* @122 */ 	bra         .fwrite_label_100
.func_end_fwrite:
	.size fwrite, .func_end_fwrite-fwrite

	.data
	.section ".rodata", "aMS", @progbits
