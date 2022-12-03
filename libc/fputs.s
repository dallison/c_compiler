	.file   "/Users/dallison/Google Drive/c_compiler/libc/fputs.c"
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


	.global fputs
	.type fputs, @function

fputs:
/* @8 */ 	stx         __result
/* @10 */ 	sty         __result+1
/* @11 */ 	ldx          #7
	jsr          __enter
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
/* @19 */ 	ldx          #0
	jsr          __arg_value2_i0			// str
/* @23 */ 	ldx          #2
	jsr          __arg_value2_i5			// stream
/* @24 */ 	lda         __i0
/* @25 */ 	sta         __i4
/* @27 */ 	lda         __i0+1
/* @28 */ 	sta         __i4+1
.fputs_label_29:
/* @32 */ 	lda         (__i4)
/* @38 */ 	sta         __i0
/* @40 */ 	and          #128
/* @42 */ 	beq         .fputs_label_41
/* @44 */ 	lda          #255
.fputs_label_41:
/* @45 */ 	sta         __i0+1
/* @47 */ 	lda         __i0
/* @48 */ 	ora         __i0+1
/* @50 */ 	beq         .fputs_label_102
/* @51 */ 	jsr         __pushi5
/* @54 */ 	lda         __i4
/* @55 */ 	sta         __i0
/* @56 */ 	lda         __i4+1
/* @57 */ 	sta         __i0+1
/* @58 */ 	lda          #__i4
/* @60 */ 	jsr         __rinc21
/* @65 */ 	lda         (__i0)
/* @71 */ 	jsr         __pusha
/* @73 */ 	ldx         #__i6
/* @74 */ 	ldy          #0
/* @75 */ 	jsr         fputc
/* @77 */ 	jsr         __incsp4
/* @80 */ 	lda         __i6
/* @81 */ 	cmp          #255
/* @82 */ 	bne         .fputs_label_100
/* @83 */ 	lda         __i6+1
/* @84 */ 	cmp          #255
/* @85 */ 	bne         .fputs_label_100
/* @89 */ 	lda          #255
/* @90 */ 	sta         __i0
/* @92 */ 	sta         __i0+1
/* @93 */ 	ldx          #8
	jsr          __load_result
/* @94 */ 	lda         #__i0
/* @96 */ 	jsr         __result2
.fputs_label_97:
/* @98 */ 	ldy          #10
	jmp          __leave
.fputs_label_100:
/* @101 */ 	bra         .fputs_label_29
.fputs_label_102:
/* @105 */ 	stz         __i0
/* @106 */ 	stz         __i0+1
/* @107 */ 	ldx          #8
	jsr          __load_result
/* @108 */ 	lda         #__i0
/* @109 */ 	jsr         __result2
/* @110 */ 	bra         .fputs_label_97
.func_end_fputs:
	.size fputs, .func_end_fputs-fputs

	.global puts
	.type puts, @function

puts:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @13 */ 	ldx          #0
	jsr          __arg_value2_i0			// stream
/* @17 */ 	lda         stdout+0
/* @18 */ 	sta         __i1
/* @20 */ 	lda         stdout+1
/* @21 */ 	sta         __i1+1
/* @23 */ 	jsr         __pushi1
/* @24 */ 	jsr         __pushi0
/* @26 */ 	ldx         #__i4
/* @27 */ 	ldy          #0
/* @28 */ 	jsr         fputs
/* @30 */ 	jsr         __incsp4
/* @32 */ 	ldx          #8
	jsr          __load_result
/* @33 */ 	lda         #__i4
/* @35 */ 	jsr         __result2
/* @37 */ 	ldy          #10
	jmp          __leave
.func_end_puts:
	.size puts, .func_end_puts-puts

	.data
	.section ".rodata", "aMS", @progbits
