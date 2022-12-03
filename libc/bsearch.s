	.file   "/Users/dallison/Google Drive/c_compiler/libc/bsearch.c"
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


	.global bsearch
	.type bsearch, @function

bsearch:
/* @5 */ 	stx         __result
/* @7 */ 	sty         __result+1
/* @8 */ 	ldx          #9
	jsr          __enter
	.byte        0x06,0x00,0x00		// Save mask i:6 b:0 l:0 x:0 f:0 
/* @12 */ 	ldx          #0
	jsr          __arg_value2_i4			// key
/* @19 */ 	ldx          #4
	jsr          __arg_value2_i1			// nmemb
/* @23 */ 	ldx          #6
	jsr          __arg_value2_i5			// size
/* @33 */ 	ldx          #8
	jsr          __arg_value2_i9			// compar
/* @41 */ 	stz         __i2
/* @43 */ 	stz         __i2+1
/* @44 */ 	lda          #__i2
/* @46 */ 	ldx          #5
/* @48 */ 	jsr         __set_var_value2
/* @51 */ 	lda          #__i2
/* @52 */ 	ldx          #__i1
/* @53 */ 	ldy          #__i5
/* @55 */ 	jsr         __umul2
/* @58 */ 	lda         __i2
/* @59 */ 	sta         __i0
/* @60 */ 	lda         __i2+1
/* @61 */ 	sta         __i0+1
/* @62 */ 	lda         __i0
/* @63 */ 	sta         __i6
/* @64 */ 	lda         __i0+1
/* @65 */ 	sta         __i6+1
.bsearch_label_66:
/* @68 */ 	ldx          #5
	jsr          __var_value2_i0			// low
/* @71 */ 	lda         __i0+1
/* @72 */ 	cmp         __i6+1
/* @73 */ 	bcc         .bsearch_label_70
/* @74 */ 	beq         .bsearch_label_224
/* @225 */ 	jmp         .bsearch_label_215
.bsearch_label_224:
/* @75 */ 	lda         __i0
/* @76 */ 	cmp         __i6
/* @77 */ 	bcc         .bsearch_label_226
/* @227 */ 	jmp         .bsearch_label_215
.bsearch_label_226:
.bsearch_label_70:
/* @80 */ 	ldx          #5
	jsr          __var_value2_i0			// low
/* @82 */ 	ldx          #5
	jsr          __var_value2_i1			// low
/* @86 */ 	sec         
/* @87 */ 	lda         __i6
/* @88 */ 	sbc         __i1
/* @89 */ 	sta         __i2
/* @90 */ 	lda         __i6+1
/* @91 */ 	sbc         __i1+1
/* @92 */ 	sta         __i2+1
/* @97 */ 	lsr          A
/* @98 */ 	sta         __i1+1
/* @99 */ 	lda         __i2
/* @100 */ 	ror          A
/* @101 */ 	sta         __i1
/* @106 */ 	clc         
/* @107 */ 	lda         __i0
/* @108 */ 	adc         __i1
/* @109 */ 	sta         __i2
/* @110 */ 	lda         __i0+1
/* @111 */ 	adc         __i1+1
/* @112 */ 	sta         __i2+1
/* @115 */ 	lda         __i2
/* @116 */ 	sta         __i7
/* @117 */ 	lda         __i2+1
/* @118 */ 	sta         __i7+1
/* @121 */ 	lda          #__i0
/* @122 */ 	ldx          #__i7
/* @123 */ 	ldy          #__i5
/* @125 */ 	jsr         __umod2
/* @129 */ 	sec         
/* @130 */ 	lda         __i7
/* @131 */ 	sbc         __i0
/* @132 */ 	sta         __i1
/* @133 */ 	lda         __i7+1
/* @134 */ 	sbc         __i0+1
/* @135 */ 	sta         __i1+1
/* @138 */ 	lda         __i1
/* @139 */ 	sta         __i7
/* @140 */ 	lda         __i1+1
/* @141 */ 	sta         __i7+1
/* @143 */ 	ldx          #2
	jsr          __arg_value2_i0			// base
/* @147 */ 	clc         
/* @148 */ 	lda         __i0
/* @149 */ 	adc         __i7
/* @150 */ 	sta         __i1
/* @151 */ 	lda         __i0+1
/* @152 */ 	adc         __i7+1
/* @153 */ 	sta         __i1+1
/* @156 */ 	jsr         __pushi1
/* @157 */ 	jsr         __pushi4
/* @158 */ 	ldx         #__i8
/* @159 */ 	ldy          #0
/* @162 */ 	jsr         .bsearch_label_160
/* @163 */ 	bra         .bsearch_label_161
.bsearch_label_160:
/* @164 */ 	jmp         (__i9)
.bsearch_label_161:
/* @166 */ 	jsr         __incsp4
/* @167 */ 	lda         __i8
/* @168 */ 	ora         __i8+1
/* @169 */ 	bne         .bsearch_label_190
/* @171 */ 	ldx          #2
	jsr          __arg_value2_i0			// base
/* @175 */ 	clc         
/* @176 */ 	lda         __i0
/* @177 */ 	adc         __i7
/* @178 */ 	sta         __i1
/* @179 */ 	lda         __i0+1
/* @180 */ 	adc         __i7+1
/* @181 */ 	sta         __i1+1
/* @183 */ 	ldx          #10
	jsr          __load_result
/* @184 */ 	lda         #__i1
/* @186 */ 	jsr         __result2
.bsearch_label_187:
/* @188 */ 	ldy          #12
	jmp          __leave
.bsearch_label_190:
/* @192 */ 	lda         __i8+1
/* @193 */ 	bpl         .bsearch_label_199
/* @194 */ 	lda         __i7
/* @195 */ 	sta         __i6
/* @196 */ 	lda         __i7+1
/* @197 */ 	sta         __i6+1
/* @198 */ 	bra         .bsearch_label_213
.bsearch_label_199:
/* @202 */ 	clc         
/* @203 */ 	lda         __i7
/* @204 */ 	adc         __i5
/* @205 */ 	sta         __i0
/* @206 */ 	lda         __i7+1
/* @207 */ 	adc         __i5+1
/* @208 */ 	sta         __i0+1
/* @210 */ 	lda          #__i0
/* @211 */ 	ldx          #5
/* @212 */ 	jsr         __set_var_value2
.bsearch_label_213:
/* @214 */ 	jmp         .bsearch_label_66
.bsearch_label_215:
/* @218 */ 	stz         __i0
/* @219 */ 	stz         __i0+1
/* @220 */ 	ldx          #10
	jsr          __load_result
/* @221 */ 	lda         #__i0
/* @222 */ 	jsr         __result2
/* @223 */ 	bra         .bsearch_label_187
.func_end_bsearch:
	.size bsearch, .func_end_bsearch-bsearch

	.data
	.section ".rodata", "aMS", @progbits
