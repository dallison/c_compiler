	.file   "/Users/dallison/Google Drive/c_compiler/libc/fgets.c"
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


	.global fgets
	.type fgets, @function

fgets:
/* @12 */ 	stx         __result
/* @14 */ 	sty         __result+1
/* @15 */ 	ldx          #7
	jsr          __enter
	.byte        0x25,0x00,0x00		// Save mask i:5 b:1 l:0 x:0 f:0 
/* @20 */ 	ldx          #2
	jsr          __arg_value2_i4			// n
/* @27 */ 	ldx          #0
	jsr          __arg_value2_i6			// s
/* @33 */ 	ldx          #4
	jsr          __arg_value2_i7			// stream
/* @34 */ 	lda          #__i4
/* @36 */ 	jsr         __rdec21
/* @37 */ 	lda         __i6
/* @38 */ 	sta         __i5
/* @39 */ 	lda         __i6+1
/* @40 */ 	sta         __i5+1
.fgets_label_41:
/* @43 */ 	lda          #0
/* @44 */ 	cmp         __i4
/* @46 */ 	sbc         __i4+1
/* @47 */ 	bvc         .fgets_label_42
/* @49 */ 	eor          #128
.fgets_label_42:
/* @50 */ 	bmi         .fgets_label_151
/* @152 */ 	jmp         .fgets_label_146
.fgets_label_151:
/* @51 */ 	jsr         __pushi7
/* @53 */ 	ldx         #__i8
/* @54 */ 	ldy          #0
/* @55 */ 	jsr         fgetc
/* @57 */ 	jsr         __incsp2
/* @60 */ 	lda         __i8
/* @61 */ 	sta         __b2
/* @65 */ 	sta         __i0
/* @66 */ 	and          #128
/* @68 */ 	beq         .fgets_label_67
/* @70 */ 	lda          #255
.fgets_label_67:
/* @71 */ 	sta         __i0+1
/* @74 */ 	lda         __i0
/* @75 */ 	cmp          #255
/* @76 */ 	bne         .fgets_label_106
/* @77 */ 	lda         __i0+1
/* @78 */ 	cmp          #255
/* @79 */ 	bne         .fgets_label_106
/* @81 */ 	lda          #1
/* @83 */ 	ldy          #16
/* @84 */ 	sta         (__i7), Y
/* @86 */ 	lda         __i5
/* @87 */ 	cmp         __i6
/* @88 */ 	bne         .fgets_label_104
/* @89 */ 	lda         __i5+1
/* @90 */ 	cmp         __i6+1
/* @91 */ 	bne         .fgets_label_104
/* @95 */ 	stz         __i0
/* @96 */ 	stz         __i0+1
/* @97 */ 	ldx          #8
	jsr          __load_result
/* @98 */ 	lda         #__i0
/* @100 */ 	jsr         __result2
.fgets_label_101:
/* @102 */ 	ldy          #10
	jmp          __leave
.fgets_label_104:
/* @105 */ 	bra         .fgets_label_146
.fgets_label_106:
/* @109 */ 	lda         __i5
/* @110 */ 	sta         __i0
/* @111 */ 	lda         __i5+1
/* @112 */ 	sta         __i0+1
/* @113 */ 	lda          #__i5
/* @115 */ 	jsr         __rinc21
/* @118 */ 	lda         __b2
/* @119 */ 	sta         (__i0)
/* @123 */ 	sta         __i0
/* @124 */ 	and          #128
/* @126 */ 	beq         .fgets_label_125
/* @127 */ 	lda          #255
.fgets_label_125:
/* @128 */ 	sta         __i0+1
/* @131 */ 	lda         __i0
/* @132 */ 	cmp          #10
/* @133 */ 	bne         .fgets_label_142
/* @134 */ 	lda         __i0+1
/* @136 */ 	bne         .fgets_label_142
/* @138 */ 	lda          #0
/* @139 */ 	tay         
/* @140 */ 	sta         (__i5)
/* @141 */ 	bra         .fgets_label_146
.fgets_label_142:
/* @143 */ 	lda          #__i4
/* @144 */ 	jsr         __rdec21
/* @145 */ 	jmp         .fgets_label_41
.fgets_label_146:
/* @147 */ 	ldx          #8
	jsr          __load_result
/* @148 */ 	lda         #__i6
/* @149 */ 	jsr         __result2
/* @150 */ 	bra         .fgets_label_101
.func_end_fgets:
	.size fgets, .func_end_fgets-fgets

	.data
	.section ".rodata", "aMS", @progbits
