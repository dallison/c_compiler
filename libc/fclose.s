	.file   "/Users/dallison/Google Drive/c_compiler/libc/fclose.c"
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


	.global fclose
	.type fclose, @function

fclose:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i4			// fp
/* @22 */ 	lda         __i4
/* @24 */ 	bne         .fclose_label_43
/* @26 */ 	lda         __i4+1
/* @28 */ 	bne         .fclose_label_43
/* @32 */ 	lda          #255
/* @33 */ 	sta         __i0
/* @35 */ 	sta         __i0+1
/* @36 */ 	ldx          #8
	jsr          __load_result
/* @37 */ 	lda         #__i0
/* @39 */ 	jsr         __result2
.fclose_label_40:
/* @41 */ 	ldy          #10
	jmp          __leave
.fclose_label_43:
/* @44 */ 	jsr         __pushi4
/* @46 */ 	ldx         #__i0
/* @47 */ 	ldy          #0
/* @48 */ 	jsr         fflush
/* @50 */ 	jsr         __incsp2
/* @53 */ 	lda         (__i4)
/* @54 */ 	sta         __i0
/* @55 */ 	ldy          #1
/* @56 */ 	lda         (__i4), Y
/* @57 */ 	sta         __i0+1
/* @60 */ 	jsr         __pushi0
/* @61 */ 	ldx         #__i5
/* @62 */ 	ldy          #0
/* @63 */ 	jsr         close
/* @64 */ 	jsr         __incsp2
/* @66 */ 	lda         __i5
/* @67 */ 	cmp          #255
/* @68 */ 	bne         .fclose_label_83
/* @69 */ 	lda         __i5+1
/* @70 */ 	cmp          #255
/* @71 */ 	bne         .fclose_label_83
/* @75 */ 	lda          #255
/* @76 */ 	sta         __i0
/* @78 */ 	sta         __i0+1
/* @79 */ 	ldx          #8
	jsr          __load_result
/* @80 */ 	lda         #__i0
/* @81 */ 	jsr         __result2
/* @82 */ 	bra         .fclose_label_40
.fclose_label_83:
/* @87 */ 	ldy          #14
/* @88 */ 	lda         (__i4), Y
/* @93 */ 	beq         .fclose_label_109
/* @97 */ 	ldy          #2
/* @98 */ 	lda         (__i4), Y
/* @99 */ 	sta         __i0
/* @101 */ 	iny         
/* @102 */ 	lda         (__i4), Y
/* @103 */ 	sta         __i0+1
/* @106 */ 	jsr         __pushi0
/* @107 */ 	jsr         free
/* @108 */ 	jsr         __incsp2
.fclose_label_109:
/* @112 */ 	lda         (__i4)
/* @113 */ 	sta         __i0
/* @114 */ 	ldy          #1
/* @115 */ 	lda         (__i4), Y
/* @116 */ 	sta         __i0+1
/* @119 */ 	lda          #2
/* @120 */ 	cmp         __i0
/* @121 */ 	lda          #0
/* @122 */ 	sbc         __i0+1
/* @123 */ 	bvc         .fclose_label_118
/* @125 */ 	eor          #128
.fclose_label_118:
/* @126 */ 	bpl         .fclose_label_130
/* @127 */ 	jsr         __pushi4
/* @128 */ 	jsr         free
/* @129 */ 	jsr         __incsp2
.fclose_label_130:
/* @133 */ 	stz         __i0
/* @134 */ 	stz         __i0+1
/* @135 */ 	ldx          #8
	jsr          __load_result
/* @136 */ 	lda         #__i0
/* @137 */ 	jsr         __result2
/* @138 */ 	jmp         .fclose_label_40
.func_end_fclose:
	.size fclose, .func_end_fclose-fclose

	.data
	.section ".rodata", "aMS", @progbits
