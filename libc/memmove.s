	.file   "/Users/dallison/Google Drive/c_compiler/libc/memmove.c"
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


	.global memmove
	.type memmove, @function

memmove:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
/* @17 */ 	ldx          #0
	jsr          __arg_value2_i1			// dest
/* @23 */ 	ldx          #2
	jsr          __arg_value2_i3			// src
/* @27 */ 	ldx          #4
	jsr          __arg_value2_i4			// n
/* @29 */ 	lda         __i1
/* @30 */ 	sta         __i0
/* @32 */ 	lda         __i1+1
/* @33 */ 	sta         __i0+1
/* @34 */ 	lda         __i3
/* @35 */ 	sta         __i2
/* @36 */ 	lda         __i3+1
/* @37 */ 	sta         __i2+1
/* @42 */ 	ldx          #0
/* @44 */ 	lda         __i0
/* @45 */ 	cmp         __i2
/* @46 */ 	lda         __i0+1
/* @47 */ 	sbc         __i2+1
/* @48 */ 	bvc         .memmove_label_43
/* @50 */ 	eor          #128
.memmove_label_43:
/* @51 */ 	bmi         .memmove_label_41
/* @52 */ 	inx         
.memmove_label_41:
/* @53 */ 	stx         __b0
/* @55 */ 	txa         
/* @56 */ 	cmp          #0
/* @57 */ 	beq         .memmove_label_81
/* @60 */ 	clc         
/* @61 */ 	lda         __i2
/* @62 */ 	adc         __i4
/* @63 */ 	sta         __i5
/* @64 */ 	lda         __i2+1
/* @65 */ 	adc         __i4+1
/* @66 */ 	sta         __i5+1
/* @69 */ 	ldx          #0
/* @72 */ 	lda         __i0
/* @73 */ 	cmp         __i5
/* @74 */ 	lda         __i0+1
/* @75 */ 	sbc         __i5+1
/* @76 */ 	bvc         .memmove_label_71
/* @77 */ 	eor          #128
.memmove_label_71:
/* @78 */ 	bpl         .memmove_label_68
/* @79 */ 	inx         
.memmove_label_68:
/* @80 */ 	stx         __b0
.memmove_label_81:
/* @83 */ 	lda         __b0
/* @85 */ 	beq         .memmove_label_177
/* @88 */ 	sec         
/* @89 */ 	lda         __i4
/* @90 */ 	sbc          #1
/* @91 */ 	sta         __i5
/* @92 */ 	lda         __i4+1
/* @93 */ 	sbc          #0
/* @94 */ 	sta         __i5+1
/* @98 */ 	clc         
/* @99 */ 	lda         __i0
/* @100 */ 	adc         __i5
/* @101 */ 	sta         __i6
/* @102 */ 	lda         __i0+1
/* @103 */ 	adc         __i5+1
/* @104 */ 	sta         __i6+1
/* @107 */ 	lda         __i6
/* @108 */ 	sta         __i0
/* @109 */ 	lda         __i6+1
/* @110 */ 	sta         __i0+1
/* @113 */ 	sec         
/* @114 */ 	lda         __i4
/* @115 */ 	sbc          #1
/* @116 */ 	sta         __i5
/* @117 */ 	lda         __i4+1
/* @118 */ 	sbc          #0
/* @119 */ 	sta         __i5+1
/* @123 */ 	clc         
/* @124 */ 	lda         __i2
/* @125 */ 	adc         __i5
/* @126 */ 	sta         __i6
/* @127 */ 	lda         __i2+1
/* @128 */ 	adc         __i5+1
/* @129 */ 	sta         __i6+1
/* @132 */ 	lda         __i6
/* @133 */ 	sta         __i2
/* @134 */ 	lda         __i6+1
/* @135 */ 	sta         __i2+1
.memmove_label_136:
/* @138 */ 	lda         __i2
/* @139 */ 	cmp         __i3
/* @140 */ 	lda         __i2+1
/* @141 */ 	sbc         __i3+1
/* @142 */ 	bvc         .memmove_label_137
/* @143 */ 	eor          #128
.memmove_label_137:
/* @144 */ 	bmi         .memmove_label_175
/* @147 */ 	lda         __i0
/* @148 */ 	sta         __i5
/* @149 */ 	lda         __i0+1
/* @150 */ 	sta         __i5+1
/* @151 */ 	lda          #__i0
/* @153 */ 	jsr         __rdec21
/* @156 */ 	lda         __i2
/* @157 */ 	sta         __i6
/* @158 */ 	lda         __i2+1
/* @159 */ 	sta         __i6+1
/* @160 */ 	lda          #__i2
/* @161 */ 	jsr         __rdec21
/* @166 */ 	lda         (__i6)
/* @173 */ 	sta         (__i5)
/* @174 */ 	bra         .memmove_label_136
.memmove_label_175:
/* @176 */ 	bra         .memmove_label_199
.memmove_label_177:
/* @179 */ 	lda         __i4
/* @181 */ 	sta         __mem_size
/* @182 */ 	lda         __i4+1
/* @184 */ 	sta         __mem_size+1
/* @185 */ 	lda         __i3
/* @187 */ 	sta         __mem_src
/* @188 */ 	lda         __i3+1
/* @190 */ 	sta         __mem_src+1
/* @191 */ 	lda         __i1
/* @193 */ 	sta         __mem_dest
/* @194 */ 	lda         __i1+1
/* @196 */ 	sta         __mem_dest+1
/* @198 */ 	jsr         __builtin_memcpy
.memmove_label_199:
/* @200 */ 	ldx          #8
	jsr          __load_result
/* @201 */ 	lda         #__i1
/* @203 */ 	jsr         __result2
/* @205 */ 	ldy          #10
	jmp          __leave
.func_end_memmove:
	.size memmove, .func_end_memmove-memmove

	.data
	.section ".rodata", "aMS", @progbits
