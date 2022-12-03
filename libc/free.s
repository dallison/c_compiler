	.file   "/Users/dallison/Google Drive/c_compiler/libc/free.c"
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


	.local  MergeWithAboveIfPossible
	.type MergeWithAboveIfPossible, @function

MergeWithAboveIfPossible:
/* @3 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
/* @10 */ 	ldx          #0
	jsr          __arg_value2_i1			// alloc_block
/* @16 */ 	ldx          #4
	jsr          __arg_value2_i3			// free_block
/* @20 */ 	ldx          #8
	jsr          __arg_value2_i4			// alloc_length
/* @24 */ 	ldx          #2
	jsr          __arg_value2_i5			// alloc_header
/* @28 */ 	ldx          #6
	jsr          __arg_value2_i6			// next_ptr
/* @30 */ 	lda         __i1
/* @31 */ 	sta         __i0
/* @33 */ 	lda         __i1+1
/* @34 */ 	sta         __i0+1
/* @35 */ 	lda         __i3
/* @36 */ 	sta         __i2
/* @37 */ 	lda         __i3+1
/* @38 */ 	sta         __i2+1
/* @41 */ 	clc         
/* @42 */ 	lda         __i0
/* @43 */ 	adc         __i4
/* @44 */ 	sta         __i7
/* @45 */ 	lda         __i0+1
/* @46 */ 	adc         __i4+1
/* @47 */ 	sta         __i7+1
/* @50 */ 	lda         __i7
/* @51 */ 	cmp         __i2
/* @52 */ 	bne         .MergeWithAboveIfPossible_label_115
/* @53 */ 	lda         __i7+1
/* @54 */ 	cmp         __i2+1
/* @55 */ 	bne         .MergeWithAboveIfPossible_label_115
/* @60 */ 	ldy          #2
/* @61 */ 	lda         (__i3), Y
/* @62 */ 	sta         __i0
/* @64 */ 	iny         
/* @65 */ 	lda         (__i3), Y
/* @66 */ 	sta         __i0+1
/* @69 */ 	lda         __i0
/* @70 */ 	dey         
/* @71 */ 	sta         (__i5), Y
/* @72 */ 	lda         __i0+1
/* @73 */ 	iny         
/* @74 */ 	sta         (__i5), Y
/* @77 */ 	clc         
/* @78 */ 	lda         __i4
/* @79 */ 	adc          #2
/* @80 */ 	sta         __i0
/* @81 */ 	lda         __i4+1
/* @82 */ 	adc          #0
/* @83 */ 	sta         __i0+1
/* @86 */ 	lda         (__i3)
/* @87 */ 	sta         __i1
/* @88 */ 	ldy          #1
/* @89 */ 	lda         (__i3), Y
/* @90 */ 	sta         __i1+1
/* @95 */ 	clc         
/* @96 */ 	lda         __i0
/* @97 */ 	adc         __i1
/* @98 */ 	sta         __i2
/* @99 */ 	lda         __i0+1
/* @100 */ 	adc         __i1+1
/* @101 */ 	sta         __i2+1
/* @104 */ 	lda         __i2
/* @105 */ 	sta         (__i5)
/* @106 */ 	lda         __i2+1
/* @108 */ 	sta         (__i5), Y
/* @109 */ 	lda         __i5
/* @110 */ 	sta         (__i6)
/* @111 */ 	lda         __i5+1
/* @113 */ 	sta         (__i6), Y
/* @114 */ 	bra         .MergeWithAboveIfPossible_label_161
.MergeWithAboveIfPossible_label_115:
/* @118 */ 	lda         __i5
/* @119 */ 	sta         __i0
/* @120 */ 	lda         __i5+1
/* @121 */ 	sta         __i0+1
/* @126 */ 	lda         (__i0)
/* @127 */ 	sta         __i1
/* @128 */ 	ldy          #1
/* @129 */ 	lda         (__i0), Y
/* @130 */ 	sta         __i1+1
/* @134 */ 	clc         
/* @135 */ 	lda         __i1
/* @136 */ 	adc          #2
/* @137 */ 	sta         __i2
/* @138 */ 	lda         __i1+1
/* @139 */ 	adc          #0
/* @140 */ 	sta         __i2+1
/* @145 */ 	lda         __i2
/* @146 */ 	sta         (__i0)
/* @147 */ 	lda         __i2+1
/* @149 */ 	sta         (__i0), Y
/* @150 */ 	lda         __i3
/* @151 */ 	iny         
/* @152 */ 	sta         (__i5), Y
/* @153 */ 	lda         __i3+1
/* @154 */ 	iny         
/* @155 */ 	sta         (__i5), Y
/* @156 */ 	lda         __i5
/* @157 */ 	sta         (__i6)
/* @158 */ 	lda         __i5+1
/* @159 */ 	ldy          #1
/* @160 */ 	sta         (__i6), Y
.MergeWithAboveIfPossible_label_161:
/* @162 */ 	ldy          #8
	jmp          __leave_leaf_void
.func_end_MergeWithAboveIfPossible:
	.size MergeWithAboveIfPossible, .func_end_MergeWithAboveIfPossible-MergeWithAboveIfPossible

	.local  MergeWithBelowIfPossible
	.type MergeWithBelowIfPossible, @function

MergeWithBelowIfPossible:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @16 */ 	ldx          #2
	jsr          __arg_value2_i1			// prev
/* @20 */ 	ldx          #0
	jsr          __arg_value2_i2			// free_block
/* @21 */ 	lda         __i1
/* @22 */ 	sta         __i0
/* @23 */ 	lda         __i1+1
/* @24 */ 	sta         __i0+1
/* @27 */ 	lda         (__i1)
/* @28 */ 	sta         __i3
/* @29 */ 	ldy          #1
/* @30 */ 	lda         (__i1), Y
/* @31 */ 	sta         __i3+1
/* @35 */ 	clc         
/* @36 */ 	lda         __i0
/* @37 */ 	adc         __i3
/* @38 */ 	sta         __i4
/* @39 */ 	lda         __i0+1
/* @40 */ 	adc         __i3+1
/* @41 */ 	sta         __i4+1
/* @44 */ 	lda         __i4
/* @45 */ 	cmp         __i2
/* @46 */ 	bne         .MergeWithBelowIfPossible_label_121
/* @47 */ 	lda         __i4+1
/* @48 */ 	cmp         __i2+1
/* @49 */ 	bne         .MergeWithBelowIfPossible_label_121
/* @54 */ 	ldy          #2
/* @55 */ 	lda         (__i2), Y
/* @56 */ 	sta         __i0
/* @58 */ 	iny         
/* @59 */ 	lda         (__i2), Y
/* @60 */ 	sta         __i0+1
/* @63 */ 	lda         __i0
/* @64 */ 	dey         
/* @65 */ 	sta         (__i1), Y
/* @66 */ 	lda         __i0+1
/* @67 */ 	iny         
/* @68 */ 	sta         (__i1), Y
/* @71 */ 	lda         (__i2)
/* @72 */ 	sta         __i0
/* @73 */ 	ldy          #1
/* @74 */ 	lda         (__i2), Y
/* @75 */ 	sta         __i0+1
/* @78 */ 	lda         __i1
/* @79 */ 	sta         __i3
/* @80 */ 	lda         __i1+1
/* @81 */ 	sta         __i3+1
/* @86 */ 	lda         (__i3)
/* @87 */ 	sta         __i4
/* @89 */ 	lda         (__i3), Y
/* @90 */ 	sta         __i4+1
/* @95 */ 	clc         
/* @96 */ 	lda         __i4
/* @97 */ 	adc         __i0
/* @98 */ 	sta         __i5
/* @99 */ 	lda         __i4+1
/* @100 */ 	adc         __i0+1
/* @101 */ 	sta         __i5+1
/* @106 */ 	lda         __i5
/* @107 */ 	sta         (__i3)
/* @108 */ 	lda         __i5+1
/* @110 */ 	sta         (__i3), Y
/* @113 */ 	tya         
/* @114 */ 	sta         __b0
/* @115 */ 	lda         #__b0
/* @117 */ 	jsr         __result1
.MergeWithBelowIfPossible_label_118:
/* @119 */ 	ldy          #8
	jmp          __leave_leaf
.MergeWithBelowIfPossible_label_121:
/* @124 */ 	stz         __b0
/* @125 */ 	lda         #__b0
/* @126 */ 	jsr         __result1
/* @127 */ 	bra         .MergeWithBelowIfPossible_label_118
.func_end_MergeWithBelowIfPossible:
	.size MergeWithBelowIfPossible, .func_end_MergeWithBelowIfPossible-MergeWithBelowIfPossible

	.local  InsertNewFreeBlockAtEnd
	.type InsertNewFreeBlockAtEnd, @function

InsertNewFreeBlockAtEnd:
/* @3 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @8 */ 	ldx          #0
	jsr          __arg_value2_i0			// free_block
/* @12 */ 	ldx          #4
	jsr          __arg_value2_i1			// length
/* @16 */ 	ldx          #2
	jsr          __arg_value2_i2			// prev
/* @18 */ 	lda         __i1
/* @19 */ 	sta         (__i0)
/* @21 */ 	lda         __i1+1
/* @22 */ 	ldy          #1
/* @23 */ 	sta         (__i0), Y
/* @25 */ 	iny         
/* @26 */ 	lda          #0
.InsertNewFreeBlockAtEnd_label_27:
/* @29 */ 	sta         (__i0), Y
/* @30 */ 	iny         
/* @32 */ 	cpy          #4
/* @33 */ 	bne         .InsertNewFreeBlockAtEnd_label_27
/* @35 */ 	lda         __i2
/* @37 */ 	bne         .InsertNewFreeBlockAtEnd_label_47
/* @38 */ 	lda         __i2+1
/* @40 */ 	bne         .InsertNewFreeBlockAtEnd_label_47
/* @42 */ 	lda         __i0
/* @43 */ 	sta         __free_list+0
/* @44 */ 	lda         __i0+1
/* @45 */ 	sta         __free_list+1
/* @46 */ 	bra         .InsertNewFreeBlockAtEnd_label_55
.InsertNewFreeBlockAtEnd_label_47:
/* @48 */ 	lda         __i0
/* @49 */ 	ldy          #2
/* @50 */ 	sta         (__i2), Y
/* @51 */ 	lda         __i0+1
/* @53 */ 	iny         
/* @54 */ 	sta         (__i2), Y
.InsertNewFreeBlockAtEnd_label_55:
/* @56 */ 	ldy          #8
	jmp          __leave_leaf_void_nomask
.func_end_InsertNewFreeBlockAtEnd:
	.size InsertNewFreeBlockAtEnd, .func_end_InsertNewFreeBlockAtEnd-InsertNewFreeBlockAtEnd

	.global free
	.type free, @function

free:
/* @7 */ 	ldx          #7
	jsr          __enter
	.byte        0x26,0x00,0x00		// Save mask i:6 b:1 l:0 x:0 f:0 
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i5			// p
/* @29 */ 	sec         
/* @31 */ 	lda         __i5
/* @32 */ 	sbc          #2
/* @33 */ 	sta         __i0
/* @35 */ 	lda         __i5+1
/* @36 */ 	sbc          #0
/* @37 */ 	sta         __i0+1
/* @42 */ 	lda         (__i0)
/* @43 */ 	sta         __i1
/* @44 */ 	ldy          #1
/* @45 */ 	lda         (__i0), Y
/* @46 */ 	sta         __i1+1
/* @49 */ 	lda         __i1
/* @50 */ 	sta         __i4
/* @51 */ 	lda         __i1+1
/* @52 */ 	sta         __i4+1
/* @55 */ 	sec         
/* @56 */ 	lda         __i5
/* @57 */ 	sbc          #2
/* @58 */ 	sta         __i0
/* @59 */ 	lda         __i5+1
/* @60 */ 	sbc          #0
/* @61 */ 	sta         __i0+1
/* @64 */ 	lda         __i0
/* @65 */ 	sta         __i6
/* @66 */ 	lda         __i0+1
/* @67 */ 	sta         __i6+1
/* @68 */ 	lda         __free_list+0
/* @69 */ 	sta         __i7
/* @70 */ 	lda         __free_list+1
/* @71 */ 	sta         __i7+1
/* @73 */ 	lda         __i7
/* @75 */ 	bne         .free_label_113
/* @76 */ 	lda         __i7+1
/* @78 */ 	bne         .free_label_113
/* @82 */ 	clc         
/* @83 */ 	lda         __i4
/* @84 */ 	adc          #2
/* @85 */ 	sta         __i0
/* @86 */ 	lda         __i4+1
/* @87 */ 	adc          #0
/* @88 */ 	sta         __i0+1
/* @91 */ 	lda         __i0
/* @92 */ 	sta         (__i6)
/* @93 */ 	lda         __i0+1
/* @94 */ 	ldy          #1
/* @95 */ 	sta         (__i6), Y
/* @97 */ 	iny         
/* @98 */ 	lda          #0
.free_label_99:
/* @101 */ 	sta         (__i6), Y
/* @102 */ 	iny         
/* @104 */ 	cpy          #4
/* @105 */ 	bne         .free_label_99
/* @106 */ 	lda         __i6
/* @107 */ 	sta         __free_list+0
/* @108 */ 	lda         __i6+1
/* @109 */ 	sta         __free_list+1
.free_label_110:
/* @111 */ 	ldy          #10
	jmp          __leave_void
.free_label_113:
/* @114 */ 	stz         __i8
/* @115 */ 	stz         __i8+1
.free_label_116:
/* @118 */ 	lda         __i7
/* @120 */ 	bne         .free_label_117
/* @121 */ 	lda         __i7+1
/* @123 */ 	bne         .free_label_267
/* @268 */ 	jmp         .free_label_218
.free_label_267:
.free_label_117:
/* @126 */ 	lda         __i8
/* @128 */ 	bne         .free_label_145
/* @129 */ 	lda         __i8+1
/* @131 */ 	bne         .free_label_145
/* @135 */ 	lda         #%lo(__free_list)
/* @136 */ 	sta         __i0
/* @137 */ 	lda         #%hi(__free_list)
/* @138 */ 	sta         __i0+1
/* @140 */ 	lda         __i0
/* @141 */ 	sta         __i9
/* @142 */ 	lda         __i0+1
/* @143 */ 	sta         __i9+1
/* @144 */ 	bra         .free_label_161
.free_label_145:
/* @148 */ 	clc         
/* @149 */ 	lda         __i8
/* @150 */ 	adc          #2
/* @151 */ 	sta         __i0
/* @152 */ 	lda         __i8+1
/* @153 */ 	adc          #0
/* @154 */ 	sta         __i0+1
/* @157 */ 	lda         __i0
/* @158 */ 	sta         __i9
/* @159 */ 	lda         __i0+1
/* @160 */ 	sta         __i9+1
.free_label_161:
/* @163 */ 	lda         __i6
/* @164 */ 	cmp         __i7
/* @165 */ 	lda         __i6+1
/* @166 */ 	sbc         __i7+1
/* @167 */ 	bvc         .free_label_162
/* @169 */ 	eor          #128
.free_label_162:
/* @170 */ 	bpl         .free_label_197
/* @171 */ 	jsr         __pushi4
/* @172 */ 	jsr         __pushi9
/* @173 */ 	jsr         __pushi7
/* @174 */ 	jsr         __pushi6
/* @175 */ 	jsr         __pushi5
/* @176 */ 	jsr         MergeWithAboveIfPossible
/* @178 */ 	jsr         __incsp10
/* @180 */ 	lda         __i8
/* @182 */ 	bne         .free_label_179
/* @183 */ 	lda         __i8+1
/* @185 */ 	beq         .free_label_195
.free_label_179:
/* @187 */ 	jsr         __pushi8
/* @188 */ 	jsr         __pushi6
/* @190 */ 	ldx         #__b0
/* @191 */ 	ldy          #0
/* @192 */ 	jsr         MergeWithBelowIfPossible
/* @194 */ 	jsr         __incsp4
.free_label_195:
/* @196 */ 	jmp         .free_label_110
.free_label_197:
/* @198 */ 	lda         __i7
/* @199 */ 	sta         __i8
/* @200 */ 	lda         __i7+1
/* @201 */ 	sta         __i8+1
/* @204 */ 	ldy          #2
/* @205 */ 	lda         (__i7), Y
/* @206 */ 	sta         __i0
/* @208 */ 	iny         
/* @209 */ 	lda         (__i7), Y
/* @210 */ 	sta         __i0+1
/* @213 */ 	lda         __i0
/* @214 */ 	sta         __i7
/* @215 */ 	lda         __i0+1
/* @216 */ 	sta         __i7+1
/* @217 */ 	jmp         .free_label_116
.free_label_218:
/* @220 */ 	lda         __i8
/* @222 */ 	bne         .free_label_219
/* @223 */ 	lda         __i8+1
/* @225 */ 	beq         .free_label_240
.free_label_219:
/* @227 */ 	jsr         __pushi8
/* @228 */ 	jsr         __pushi6
/* @230 */ 	ldx         #__b2
/* @231 */ 	ldy          #0
/* @232 */ 	jsr         MergeWithBelowIfPossible
/* @233 */ 	jsr         __incsp4
/* @235 */ 	lda         __b2
/* @237 */ 	beq         .free_label_239
/* @238 */ 	jmp         .free_label_110
.free_label_239:
.free_label_240:
/* @243 */ 	lda         (__i6)
/* @244 */ 	sta         __i0
/* @245 */ 	ldy          #1
/* @246 */ 	lda         (__i6), Y
/* @247 */ 	sta         __i0+1
/* @251 */ 	clc         
/* @252 */ 	lda         __i0
/* @253 */ 	adc          #2
/* @254 */ 	sta         __i1
/* @255 */ 	lda         __i0+1
/* @256 */ 	adc          #0
/* @257 */ 	sta         __i1+1
/* @260 */ 	jsr         __pushi1
/* @261 */ 	jsr         __pushi8
/* @262 */ 	jsr         __pushi6
/* @263 */ 	jsr         InsertNewFreeBlockAtEnd
/* @265 */ 	jsr         __incsp6
/* @266 */ 	jmp         .free_label_110
.func_end_free:
	.size free, .func_end_free-free

	.data
	.section ".rodata", "aMS", @progbits
