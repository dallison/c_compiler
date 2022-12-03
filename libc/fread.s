	.file   "/Users/dallison/Google Drive/c_compiler/libc/fread.c"
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


	.global fread
	.type fread, @function

fread:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #7
	jsr          __enter
	.byte        0x27,0x00,0x00		// Save mask i:7 b:1 l:0 x:0 f:0 
/* @15 */ 	ldx          #6
	jsr          __arg_value2_i4			// stream
/* @19 */ 	ldx          #2
	jsr          __arg_value2_i5			// size
/* @23 */ 	ldx          #4
	jsr          __arg_value2_i0			// n
/* @32 */ 	ldx          #0
	jsr          __arg_value2_i1			// ptr
/* @39 */ 	lda          #__i2
/* @40 */ 	ldx          #__i5
/* @41 */ 	ldy          #__i0
/* @43 */ 	jsr         __umul2
/* @47 */ 	lda         __i2
/* @48 */ 	sta         __i9
/* @50 */ 	lda         __i2+1
/* @51 */ 	sta         __i9+1
/* @52 */ 	stz         __i6
/* @53 */ 	stz         __i6+1
/* @54 */ 	lda         __i1
/* @55 */ 	sta         __i7
/* @56 */ 	lda         __i1+1
/* @57 */ 	sta         __i7+1
.fread_label_58:
/* @64 */ 	ldx          #1
/* @65 */ 	lda          #0
/* @66 */ 	cmp         __i9+1
/* @67 */ 	bcc         .fread_label_63
/* @68 */ 	bne         .fread_label_62
/* @69 */ 	lda          #0
/* @70 */ 	cmp         __i9
/* @71 */ 	bcc         .fread_label_63
.fread_label_62:
/* @72 */ 	dex         
.fread_label_63:
/* @73 */ 	stx         __b2
/* @75 */ 	txa         
/* @76 */ 	cmp          #0
/* @77 */ 	beq         .fread_label_104
/* @78 */ 	jsr         __pushi4
/* @80 */ 	ldx         #__i10
/* @81 */ 	ldy          #0
/* @82 */ 	jsr         fgetc
/* @84 */ 	jsr         __incsp2
/* @87 */ 	lda         __i10
/* @88 */ 	sta         __i8
/* @89 */ 	lda         __i10+1
/* @90 */ 	sta         __i8+1
/* @95 */ 	ldx          #0
/* @96 */ 	lda         __i10
/* @97 */ 	cmp          #255
/* @98 */ 	bne         .fread_label_94
/* @99 */ 	lda         __i10+1
/* @100 */ 	cmp          #255
/* @101 */ 	beq         .fread_label_93
.fread_label_94:
/* @102 */ 	inx         
.fread_label_93:
/* @103 */ 	stx         __b2
.fread_label_104:
/* @106 */ 	lda         __b2
/* @108 */ 	beq         .fread_label_136
/* @111 */ 	lda         __i7
/* @112 */ 	sta         __i0
/* @113 */ 	lda         __i7+1
/* @114 */ 	sta         __i0+1
/* @115 */ 	lda          #__i7
/* @117 */ 	jsr         __rinc21
/* @120 */ 	lda         __i8
/* @121 */ 	sta         __i1
/* @122 */ 	lda         __i8+1
/* @123 */ 	sta         __i1+1
/* @128 */ 	lda         __i1
/* @129 */ 	sta         (__i0)
/* @130 */ 	lda          #__i6
/* @131 */ 	jsr         __rinc21
/* @132 */ 	lda          #__i9
/* @134 */ 	jsr         __rdec21
/* @135 */ 	bra         .fread_label_58
.fread_label_136:
/* @139 */ 	lda          #__i0
/* @140 */ 	ldx          #__i6
/* @141 */ 	ldy          #__i5
/* @143 */ 	jsr         __udiv2
/* @145 */ 	ldx          #8
	jsr          __load_result
/* @146 */ 	lda         #__i0
/* @148 */ 	jsr         __result2
/* @150 */ 	ldy          #10
	jmp          __leave
.func_end_fread:
	.size fread, .func_end_fread-fread

	.data
	.section ".rodata", "aMS", @progbits
