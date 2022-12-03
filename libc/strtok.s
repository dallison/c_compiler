	.file   "/Users/dallison/Google Drive/c_compiler/libc/strtok.c"
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


	.global strtok
	.type strtok, @function

strtok:
/* @5 */ 	stx         __result
/* @7 */ 	sty         __result+1
/* @8 */ 	ldx          #7
	jsr          __enter
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i4			// s1
/* @25 */ 	ldx          #2
	jsr          __arg_value2_i7			// s2
/* @27 */ 	lda         __i4
/* @29 */ 	bne         .strtok_label_39
/* @31 */ 	lda         __i4+1
/* @33 */ 	bne         .strtok_label_39
/* @35 */ 	lda         .local.saved.87+0
/* @36 */ 	sta         __i4
/* @37 */ 	lda         .local.saved.87+1
/* @38 */ 	sta         __i4+1
.strtok_label_39:
/* @40 */ 	jsr         __pushi4
/* @41 */ 	ldx         #__i5
/* @42 */ 	ldy          #0
/* @43 */ 	jsr         strlen
/* @45 */ 	jsr         __incsp2
/* @46 */ 	jsr         __pushi7
/* @47 */ 	jsr         __pushi4
/* @48 */ 	ldx         #__i6
/* @49 */ 	ldy          #0
/* @50 */ 	jsr         strcspn
/* @52 */ 	jsr         __incsp4
/* @54 */ 	lda         __i6
/* @55 */ 	cmp         __i5
/* @56 */ 	bne         .strtok_label_72
/* @57 */ 	lda         __i6+1
/* @58 */ 	cmp         __i5+1
/* @59 */ 	bne         .strtok_label_72
/* @63 */ 	stz         __i0
/* @64 */ 	stz         __i0+1
/* @65 */ 	ldx          #8
	jsr          __load_result
/* @66 */ 	lda         #__i0
/* @68 */ 	jsr         __result2
.strtok_label_69:
/* @70 */ 	ldy          #10
	jmp          __leave
.strtok_label_72:
/* @75 */ 	clc         
/* @76 */ 	lda         __i4
/* @77 */ 	adc         __i6
/* @78 */ 	sta         __i0
/* @79 */ 	lda         __i4+1
/* @80 */ 	adc         __i6+1
/* @81 */ 	sta         __i0+1
/* @84 */ 	lda         __i0
/* @85 */ 	sta         __i4
/* @86 */ 	lda         __i0+1
/* @87 */ 	sta         __i4+1
/* @90 */ 	sec         
/* @91 */ 	lda         __i5
/* @92 */ 	sbc         __i6
/* @93 */ 	sta         __i0
/* @94 */ 	lda         __i5+1
/* @95 */ 	sbc         __i6+1
/* @96 */ 	sta         __i0+1
/* @99 */ 	lda         __i0
/* @100 */ 	sta         __i5
/* @101 */ 	lda         __i0+1
/* @102 */ 	sta         __i5+1
/* @103 */ 	jsr         __pushi7
/* @104 */ 	jsr         __pushi4
/* @105 */ 	ldx         #__i6
/* @106 */ 	ldy          #0
/* @107 */ 	jsr         strspn
/* @108 */ 	jsr         __incsp4
/* @110 */ 	lda         __i6
/* @111 */ 	cmp         __i5
/* @112 */ 	bne         .strtok_label_121
/* @113 */ 	lda         __i6+1
/* @114 */ 	cmp         __i5+1
/* @115 */ 	bne         .strtok_label_121
/* @117 */ 	ldx          #8
	jsr          __load_result
/* @118 */ 	lda         #__i4
/* @119 */ 	jsr         __result2
/* @120 */ 	bra         .strtok_label_69
.strtok_label_121:
/* @124 */ 	clc         
/* @125 */ 	lda         __i4
/* @126 */ 	adc         __i6
/* @127 */ 	sta         __i0
/* @128 */ 	lda         __i4+1
/* @129 */ 	adc         __i6+1
/* @130 */ 	sta         __i0+1
/* @133 */ 	lda          #0
/* @134 */ 	tay         
/* @135 */ 	sta         (__i0)
/* @138 */ 	clc         
/* @139 */ 	lda         __i4
/* @140 */ 	adc         __i6
/* @141 */ 	sta         __i0
/* @142 */ 	lda         __i4+1
/* @143 */ 	adc         __i6+1
/* @144 */ 	sta         __i0+1
/* @148 */ 	clc         
/* @149 */ 	lda         __i0
/* @150 */ 	adc          #1
/* @151 */ 	sta         __i1
/* @152 */ 	lda         __i0+1
/* @153 */ 	adc          #0
/* @154 */ 	sta         __i1+1
/* @156 */ 	lda         __i1
/* @157 */ 	sta         .local.saved.87+0
/* @158 */ 	lda         __i1+1
/* @159 */ 	sta         .local.saved.87+1
/* @160 */ 	ldx          #8
	jsr          __load_result
/* @161 */ 	lda         #__i4
/* @162 */ 	jsr         __result2
/* @163 */ 	jmp         .strtok_label_69
.func_end_strtok:
	.size strtok, .func_end_strtok-strtok

	.data
	.type   .local.saved.87,@object
	.local  .local.saved.87
	.comm   .local.saved.87,2,1

	.section ".rodata", "aMS", @progbits
