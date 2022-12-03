	.file   "/Users/dallison/Google Drive/c_compiler/libc/gets.c"
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


	.global gets
	.type gets, @function

gets:
/* @11 */ 	stx         __result
/* @13 */ 	sty         __result+1
/* @14 */ 	ldx          #7
	jsr          __enter
	.byte        0x23,0x00,0x00		// Save mask i:3 b:1 l:0 x:0 f:0 
/* @23 */ 	ldx          #0
	jsr          __arg_value2_i5			// str
/* @26 */ 	lda         __i5
/* @27 */ 	sta         __i4
/* @28 */ 	lda         __i5+1
/* @29 */ 	sta         __i4+1
.gets_label_30:
/* @33 */ 	lda         stdin+0
/* @34 */ 	sta         __i0
/* @35 */ 	lda         stdin+1
/* @36 */ 	sta         __i0+1
/* @38 */ 	jsr         __pushi0
/* @40 */ 	ldx         #__i6
/* @41 */ 	ldy          #0
/* @42 */ 	jsr         fgetc
/* @44 */ 	jsr         __incsp2
/* @47 */ 	lda         __i6
/* @48 */ 	sta         __b2
/* @52 */ 	sta         __i0
/* @54 */ 	and          #128
/* @56 */ 	beq         .gets_label_55
/* @58 */ 	lda          #255
.gets_label_55:
/* @59 */ 	sta         __i0+1
/* @62 */ 	lda         __i0
/* @63 */ 	cmp          #255
/* @64 */ 	bne         .gets_label_101
/* @65 */ 	lda         __i0+1
/* @66 */ 	cmp          #255
/* @67 */ 	bne         .gets_label_101
/* @71 */ 	lda         stdin+0
/* @72 */ 	sta         __i0
/* @73 */ 	lda         stdin+1
/* @74 */ 	sta         __i0+1
/* @76 */ 	lda          #1
/* @78 */ 	ldy          #16
/* @79 */ 	sta         (__i0), Y
/* @81 */ 	lda         __i4
/* @82 */ 	cmp         __i5
/* @83 */ 	bne         .gets_label_99
/* @84 */ 	lda         __i4+1
/* @85 */ 	cmp         __i5+1
/* @86 */ 	bne         .gets_label_99
/* @90 */ 	stz         __i0
/* @91 */ 	stz         __i0+1
/* @92 */ 	ldx          #8
	jsr          __load_result
/* @93 */ 	lda         #__i0
/* @95 */ 	jsr         __result2
.gets_label_96:
/* @97 */ 	ldy          #10
	jmp          __leave
.gets_label_99:
/* @100 */ 	bra         .gets_label_151
.gets_label_101:
/* @104 */ 	lda         __i4
/* @105 */ 	sta         __i0
/* @106 */ 	lda         __i4+1
/* @107 */ 	sta         __i0+1
/* @108 */ 	lda          #__i4
/* @110 */ 	jsr         __rinc21
/* @113 */ 	lda         __b2
/* @114 */ 	sta         (__i0)
/* @118 */ 	sta         __i0
/* @119 */ 	and          #128
/* @121 */ 	beq         .gets_label_120
/* @122 */ 	lda          #255
.gets_label_120:
/* @123 */ 	sta         __i0+1
/* @126 */ 	lda         __i0
/* @127 */ 	cmp          #10
/* @128 */ 	bne         .gets_label_148
/* @129 */ 	lda         __i0+1
/* @131 */ 	bne         .gets_label_148
/* @135 */ 	clc         
/* @136 */ 	lda         __i4
/* @137 */ 	adc          #255
/* @138 */ 	sta         __i0
/* @139 */ 	lda         __i4+1
/* @140 */ 	adc          #255
/* @141 */ 	sta         __i0+1
/* @144 */ 	lda          #0
/* @145 */ 	tay         
/* @146 */ 	sta         (__i0)
/* @147 */ 	bra         .gets_label_151
.gets_label_148:
/* @150 */ 	jmp         .gets_label_30
.gets_label_151:
/* @152 */ 	ldx          #8
	jsr          __load_result
/* @153 */ 	lda         #__i5
/* @154 */ 	jsr         __result2
/* @155 */ 	bra         .gets_label_96
.func_end_gets:
	.size gets, .func_end_gets-gets

	.data
	.section ".rodata", "aMS", @progbits
