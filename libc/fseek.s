	.file   "/Users/dallison/Google Drive/c_compiler/libc/fseek.c"
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


	.global fseek
	.type fseek, @function

fseek:
/* @10 */ 	stx         __result
/* @12 */ 	sty         __result+1
/* @13 */ 	ldx          #7
	jsr          __enter
	.byte        0x03,0x04,0x00		// Save mask i:3 b:0 l:2 x:0 f:0 
/* @19 */ 	ldx          #0
	jsr          __arg_value2_i4			// stream
/* @26 */ 	ldx          #6
	jsr          __arg_value2_i6			// whence
/* @30 */ 	ldx          #2
	jsr          __arg_value4_l2			// offset
/* @31 */ 	jsr         __pushi4
/* @33 */ 	ldx         #__i0
/* @34 */ 	ldy          #0
/* @35 */ 	jsr         fflush
/* @37 */ 	jsr         __incsp2
/* @38 */ 	jsr         __pushi6
/* @39 */ 	jsr         __pushl2
/* @42 */ 	lda         (__i4)
/* @43 */ 	sta         __i0
/* @45 */ 	ldy          #1
/* @46 */ 	lda         (__i4), Y
/* @47 */ 	sta         __i0+1
/* @50 */ 	jsr         __pushi0
/* @52 */ 	ldx         #__l3
/* @53 */ 	ldy          #0
/* @54 */ 	jsr         lseek
/* @56 */ 	jsr         __incsp8
/* @59 */ 	lda         __l3
/* @60 */ 	sta         __i5
/* @61 */ 	lda         __l3+1
/* @62 */ 	sta         __i5+1
/* @64 */ 	lda         __i5
/* @65 */ 	cmp          #255
/* @66 */ 	bne         .fseek_label_78
/* @67 */ 	lda         __i5+1
/* @68 */ 	cmp          #255
/* @69 */ 	bne         .fseek_label_78
/* @71 */ 	ldx          #8
	jsr          __load_result
/* @72 */ 	lda         #__i5
/* @74 */ 	jsr         __result2
.fseek_label_75:
/* @76 */ 	ldy          #10
	jmp          __leave
.fseek_label_78:
/* @79 */ 	lda          #0
/* @81 */ 	ldy          #16
/* @82 */ 	sta         (__i4), Y
/* @85 */ 	iny         
/* @86 */ 	sta         (__i4), Y
/* @89 */ 	stz         __i0
/* @90 */ 	stz         __i0+1
/* @91 */ 	ldx          #8
	jsr          __load_result
/* @92 */ 	lda         #__i0
/* @93 */ 	jsr         __result2
/* @94 */ 	bra         .fseek_label_75
.func_end_fseek:
	.size fseek, .func_end_fseek-fseek

	.global ftell
	.type ftell, @function

ftell:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x02,0x00		// Save mask i:1 b:0 l:1 x:0 f:0 
/* @15 */ 	ldx          #0
	jsr          __arg_value2_i4			// stream
/* @16 */ 	jsr         __pushi4
/* @19 */ 	ldx         #__i0
/* @20 */ 	ldy          #0
/* @21 */ 	jsr         fflush
/* @23 */ 	jsr         __incsp2
/* @24 */ 	ldx          #0
/* @26 */ 	jsr         __pushxy0
/* @27 */ 	ldx         #%lo(.lit.10)
	ldy         #%hi(.lit.10)
/* @29 */ 	jsr         __push4xy
/* @32 */ 	lda         (__i4)
/* @33 */ 	sta         __i0
/* @35 */ 	ldy          #1
/* @36 */ 	lda         (__i4), Y
/* @37 */ 	sta         __i0+1
/* @40 */ 	jsr         __pushi0
/* @42 */ 	ldx         #__l2
/* @43 */ 	ldy          #0
/* @44 */ 	jsr         lseek
/* @46 */ 	jsr         __incsp8
/* @48 */ 	ldx          #8
	jsr          __load_result
/* @49 */ 	lda         #__l2
/* @51 */ 	jsr         __result4
/* @53 */ 	ldy          #10
	jmp          __leave
.func_end_ftell:
	.size ftell, .func_end_ftell-ftell

	.global fgetpos
	.type fgetpos, @function

fgetpos:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x02,0x00		// Save mask i:1 b:0 l:1 x:0 f:0 
/* @17 */ 	ldx          #0
	jsr          __arg_value2_i0			// stream
/* @21 */ 	ldx          #2
	jsr          __arg_value2_i4			// pos
/* @22 */ 	jsr         __pushi0
/* @24 */ 	ldx         #__l2
/* @25 */ 	ldy          #0
/* @26 */ 	jsr         ftell
/* @28 */ 	jsr         __incsp2
/* @30 */ 	lda         __l2
/* @31 */ 	cmp          #255
/* @32 */ 	bne         .fgetpos_label_59
/* @34 */ 	lda         __l2+1
/* @35 */ 	cmp          #255
/* @36 */ 	bne         .fgetpos_label_59
/* @38 */ 	lda         __l2+2
/* @39 */ 	cmp          #255
/* @40 */ 	bne         .fgetpos_label_59
/* @42 */ 	lda         __l2+3
/* @43 */ 	cmp          #255
/* @44 */ 	bne         .fgetpos_label_59
/* @48 */ 	lda          #255
/* @49 */ 	sta         __i0
/* @51 */ 	sta         __i0+1
/* @52 */ 	ldx          #8
	jsr          __load_result
/* @53 */ 	lda         #__i0
/* @55 */ 	jsr         __result2
.fgetpos_label_56:
/* @57 */ 	ldy          #10
	jmp          __leave
.fgetpos_label_59:
/* @62 */ 	ldx          #3
.fgetpos_label_63:
/* @65 */ 	lda         __l2, X
/* @66 */ 	sta         __l0, X
/* @67 */ 	dex         
/* @68 */ 	bpl         .fgetpos_label_63
/* @71 */ 	lda         __l0
/* @72 */ 	sta         (__i4)
/* @73 */ 	lda         __l0+1
/* @74 */ 	ldy          #1
/* @75 */ 	sta         (__i4), Y
/* @78 */ 	stz         __i0
/* @79 */ 	stz         __i0+1
/* @80 */ 	ldx          #8
	jsr          __load_result
/* @81 */ 	lda         #__i0
/* @82 */ 	jsr         __result2
/* @83 */ 	bra         .fgetpos_label_56
.func_end_fgetpos:
	.size fgetpos, .func_end_fgetpos-fgetpos

	.global fsetpos
	.type fsetpos, @function

fsetpos:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @18 */ 	ldx          #2
	jsr          __arg_value2_i0			// pos
/* @22 */ 	ldx          #0
	jsr          __arg_value2_i1			// stream
/* @24 */ 	ldx          #2
/* @26 */ 	jsr         __pushxy0
/* @30 */ 	lda         (__i0)
/* @31 */ 	sta         __i2
/* @33 */ 	ldy          #1
/* @34 */ 	lda         (__i0), Y
/* @35 */ 	sta         __i2+1
/* @39 */ 	lda         __i2
/* @40 */ 	sta         __l0
/* @41 */ 	lda         __i2+1
/* @42 */ 	sta         __l0+1
/* @44 */ 	and          #128
/* @46 */ 	beq         .fsetpos_label_45
/* @48 */ 	lda          #255
.fsetpos_label_45:
/* @49 */ 	sta         __l0+2
/* @51 */ 	sta         __l0+3
/* @54 */ 	jsr         __pushl0
/* @55 */ 	jsr         __pushi1
/* @56 */ 	ldx         #__i4
/* @57 */ 	ldy          #0
/* @58 */ 	jsr         fseek
/* @60 */ 	jsr         __incsp8
/* @62 */ 	lda         __i4
/* @63 */ 	cmp          #255
/* @64 */ 	bne         .fsetpos_label_82
/* @65 */ 	lda         __i4+1
/* @66 */ 	cmp          #255
/* @67 */ 	bne         .fsetpos_label_82
/* @71 */ 	lda          #255
/* @72 */ 	sta         __i0
/* @74 */ 	sta         __i0+1
/* @75 */ 	ldx          #8
	jsr          __load_result
/* @76 */ 	lda         #__i0
/* @78 */ 	jsr         __result2
.fsetpos_label_79:
/* @80 */ 	ldy          #10
	jmp          __leave
.fsetpos_label_82:
/* @85 */ 	stz         __i0
/* @86 */ 	stz         __i0+1
/* @87 */ 	ldx          #8
	jsr          __load_result
/* @88 */ 	lda         #__i0
/* @89 */ 	jsr         __result2
/* @90 */ 	bra         .fsetpos_label_79
.func_end_fsetpos:
	.size fsetpos, .func_end_fsetpos-fsetpos

	.global rewind
	.type rewind, @function

rewind:
/* @5 */ 	ldx          #7
	jsr          __enter_nomask
/* @10 */ 	ldx          #0
	jsr          __arg_value2_i0			// stream
/* @12 */ 	ldx          #2
/* @14 */ 	jsr         __pushxy0
/* @15 */ 	ldx         #%lo(.lit.22)
	ldy         #%hi(.lit.22)
/* @17 */ 	jsr         __push4xy
/* @18 */ 	jsr         __pushi0
/* @21 */ 	ldx         #__i1
/* @22 */ 	ldy          #0
/* @23 */ 	jsr         fseek
/* @25 */ 	jsr         __incsp8
/* @26 */ 	ldy          #10
	jmp          __leave_void_nomask
.func_end_rewind:
	.size rewind, .func_end_rewind-rewind

	.data
	.section ".rodata", "aMS", @progbits
.lit.10:
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.type .lit.10, @object
	.size .lit.10, 4

.lit.22:
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.type .lit.22, @object
	.size .lit.22, 4

