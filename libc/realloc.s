	.file   "/Users/dallison/Google Drive/c_compiler/libc/realloc.c"
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


	.local  AlignSize
	.type AlignSize, @function

AlignSize:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @11 */ 	ldx          #0
	jsr          __arg_value2_i0			// s
/* @14 */ 	clc         
/* @16 */ 	lda         __i0
/* @17 */ 	adc          #1
/* @18 */ 	sta         __i1
/* @20 */ 	lda         __i0+1
/* @21 */ 	adc          #0
/* @22 */ 	sta         __i1+1
/* @26 */ 	lda         __i1
/* @27 */ 	and          #254
/* @28 */ 	sta         __i2
/* @29 */ 	lda         __i1+1
/* @30 */ 	sta         __i2+1
/* @32 */ 	lda         #__i2
/* @34 */ 	jsr         __result2
/* @36 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_AlignSize:
	.size AlignSize, .func_end_AlignSize-AlignSize

	.local  ShrinkBlock
	.type ShrinkBlock, @function

ShrinkBlock:
/* @10 */ 	ldx          #7
	jsr          __enter
	.byte        0x06,0x00,0x00		// Save mask i:6 b:0 l:0 x:0 f:0 
/* @17 */ 	ldx          #4
	jsr          __arg_value2_i4			// new_length
/* @21 */ 	ldx          #2
	jsr          __arg_value2_i5			// orig_length
/* @28 */ 	ldx          #6
	jsr          __arg_value2_i7			// len_ptr
/* @34 */ 	ldx          #0
	jsr          __arg_value2_i9			// alloc_block
/* @37 */ 	lda         __i4+1
/* @38 */ 	cmp         __i5+1
/* @39 */ 	bcc         .ShrinkBlock_label_35
/* @40 */ 	bne         .ShrinkBlock_label_47
/* @42 */ 	lda         __i4
/* @43 */ 	cmp         __i5
/* @44 */ 	bcs         .ShrinkBlock_label_47
.ShrinkBlock_label_35:
/* @46 */ 	bra         .ShrinkBlock_label_69
.ShrinkBlock_label_47:
/* @49 */ 	ldx         #%lo(.str.3)
	ldy         #%hi(.str.3)
/* @51 */ 	jsr         __pushxy
/* @53 */ 	ldx          #37
/* @55 */ 	jsr         __pushxy0
/* @57 */ 	ldx         #%lo(.str.4)
	ldy         #%hi(.str.4)
/* @58 */ 	jsr         __pushxy
/* @60 */ 	ldx         #%lo(.str.5)
	ldy         #%hi(.str.5)
/* @61 */ 	jsr         __pushxy
/* @63 */ 	ldx         #__i0
/* @64 */ 	ldy          #0
/* @65 */ 	jsr         printf
/* @67 */ 	jsr         __incsp8
/* @68 */ 	jsr         abort
.ShrinkBlock_label_69:
/* @72 */ 	sec         
/* @73 */ 	lda         __i5
/* @74 */ 	sbc         __i4
/* @75 */ 	sta         __i0
/* @76 */ 	lda         __i5+1
/* @77 */ 	sbc         __i4+1
/* @78 */ 	sta         __i0+1
/* @81 */ 	lda         __i0
/* @82 */ 	sta         __i6
/* @83 */ 	lda         __i0+1
/* @84 */ 	sta         __i6+1
/* @87 */ 	cmp          #0
/* @88 */ 	bcc         .ShrinkBlock_label_155
/* @89 */ 	bne         .ShrinkBlock_label_85
/* @90 */ 	lda         __i6
/* @91 */ 	cmp          #4
/* @92 */ 	bcc         .ShrinkBlock_label_155
.ShrinkBlock_label_85:
/* @94 */ 	lda         __i4
/* @95 */ 	sta         (__i7)
/* @96 */ 	lda         __i4+1
/* @97 */ 	ldy          #1
/* @98 */ 	sta         (__i7), Y
/* @101 */ 	clc         
/* @102 */ 	lda         __i9
/* @103 */ 	adc          #2
/* @104 */ 	sta         __i0
/* @105 */ 	lda         __i9+1
/* @106 */ 	adc          #0
/* @107 */ 	sta         __i0+1
/* @111 */ 	clc         
/* @112 */ 	lda         __i0
/* @113 */ 	adc         __i4
/* @114 */ 	sta         __i1
/* @115 */ 	lda         __i0+1
/* @116 */ 	adc         __i4+1
/* @117 */ 	sta         __i1+1
/* @120 */ 	lda         __i1
/* @121 */ 	sta         __i8
/* @122 */ 	lda         __i1+1
/* @123 */ 	sta         __i8+1
/* @126 */ 	sec         
/* @127 */ 	lda         __i6
/* @128 */ 	sbc          #2
/* @129 */ 	sta         __i0
/* @130 */ 	lda         __i6+1
/* @131 */ 	sbc          #0
/* @132 */ 	sta         __i0+1
/* @135 */ 	lda         __i0
/* @136 */ 	sta         (__i8)
/* @137 */ 	lda         __i0+1
/* @139 */ 	sta         (__i8), Y
/* @142 */ 	clc         
/* @143 */ 	lda         __i8
/* @144 */ 	adc          #2
/* @145 */ 	sta         __i0
/* @146 */ 	lda         __i8+1
/* @147 */ 	adc          #0
/* @148 */ 	sta         __i0+1
/* @151 */ 	jsr         __pushi0
/* @152 */ 	jsr         free
/* @154 */ 	jsr         __incsp2
.ShrinkBlock_label_155:
/* @156 */ 	ldy          #10
	jmp          __leave_void
.func_end_ShrinkBlock:
	.size ShrinkBlock, .func_end_ShrinkBlock-ShrinkBlock

	.local  ExpandIntoFreeBlockAbove
	.type ExpandIntoFreeBlockAbove, @function

ExpandIntoFreeBlockAbove:
/* @10 */ 	ldx          #7
	jsr          __enter
	.byte        0x08,0x00,0x00		// Save mask i:8 b:0 l:0 x:0 f:0 
/* @16 */ 	ldx          #10
	jsr          __arg_value2_i4			// next_ptr
/* @23 */ 	ldx          #0
	jsr          __arg_value2_i6			// free_block
/* @27 */ 	ldx          #8
	jsr          __arg_value2_i7			// len_ptr
/* @31 */ 	ldx          #2
	jsr          __arg_value2_i8			// new_length
/* @37 */ 	ldx          #4
	jsr          __arg_value2_i10			// len_diff
/* @41 */ 	ldx          #6
	jsr          __arg_value2_i11			// free_remaining
/* @44 */ 	lda          #0
/* @45 */ 	cmp         __i11+1
/* @46 */ 	bcc         .ExpandIntoFreeBlockAbove_label_42
/* @47 */ 	bne         .ExpandIntoFreeBlockAbove_label_54
/* @49 */ 	lda          #4
/* @50 */ 	cmp         __i11
/* @51 */ 	bcs         .ExpandIntoFreeBlockAbove_label_54
.ExpandIntoFreeBlockAbove_label_42:
/* @53 */ 	bra         .ExpandIntoFreeBlockAbove_label_76
.ExpandIntoFreeBlockAbove_label_54:
/* @56 */ 	ldx         #%lo(.str.13)
	ldy         #%hi(.str.13)
/* @58 */ 	jsr         __pushxy
/* @60 */ 	ldx          #53
/* @62 */ 	jsr         __pushxy0
/* @64 */ 	ldx         #%lo(.str.14)
	ldy         #%hi(.str.14)
/* @65 */ 	jsr         __pushxy
/* @67 */ 	ldx         #%lo(.str.15)
	ldy         #%hi(.str.15)
/* @68 */ 	jsr         __pushxy
/* @70 */ 	ldx         #__i0
/* @71 */ 	ldy          #0
/* @72 */ 	jsr         printf
/* @74 */ 	jsr         __incsp8
/* @75 */ 	jsr         abort
.ExpandIntoFreeBlockAbove_label_76:
/* @80 */ 	ldy          #2
/* @81 */ 	lda         (__i6), Y
/* @82 */ 	sta         __i0
/* @84 */ 	iny         
/* @85 */ 	lda         (__i6), Y
/* @86 */ 	sta         __i0+1
/* @89 */ 	lda         __i0
/* @90 */ 	sta         __i5
/* @91 */ 	lda         __i0+1
/* @92 */ 	sta         __i5+1
/* @93 */ 	lda         __i8
/* @94 */ 	sta         (__i7)
/* @95 */ 	lda         __i8+1
/* @96 */ 	ldy          #1
/* @97 */ 	sta         (__i7), Y
/* @100 */ 	clc         
/* @101 */ 	lda         __i6
/* @102 */ 	adc         __i10
/* @103 */ 	sta         __i0
/* @104 */ 	lda         __i6+1
/* @105 */ 	adc         __i10+1
/* @106 */ 	sta         __i0+1
/* @109 */ 	lda         __i0
/* @110 */ 	sta         __i9
/* @111 */ 	lda         __i0+1
/* @112 */ 	sta         __i9+1
/* @113 */ 	lda         __i11
/* @114 */ 	sta         (__i9)
/* @115 */ 	lda         __i11+1
/* @117 */ 	sta         (__i9), Y
/* @118 */ 	lda         __i5
/* @119 */ 	iny         
/* @120 */ 	sta         (__i9), Y
/* @121 */ 	lda         __i5+1
/* @122 */ 	iny         
/* @123 */ 	sta         (__i9), Y
/* @124 */ 	lda         __i9
/* @125 */ 	sta         (__i4)
/* @126 */ 	lda         __i9+1
/* @127 */ 	ldy          #1
/* @128 */ 	sta         (__i4), Y
/* @129 */ 	ldy          #10
	jmp          __leave_void
.func_end_ExpandIntoFreeBlockAbove:
	.size ExpandIntoFreeBlockAbove, .func_end_ExpandIntoFreeBlockAbove-ExpandIntoFreeBlockAbove

	.local  MergeWithFreeBlockBelow
	.type MergeWithFreeBlockBelow, @function

MergeWithFreeBlockBelow:
/* @5 */ 	stx         __result
/* @7 */ 	sty         __result+1
/* @8 */ 	ldx          #11
	jsr          __enter
	.byte        0x08,0x00,0x00		// Save mask i:8 b:0 l:0 x:0 f:0 
/* @14 */ 	ldx          #0
	jsr          __arg_value2_i4			// alloc_block
/* @18 */ 	ldx          #4
	jsr          __arg_value2_i5			// free_block
/* @22 */ 	ldx          #2
	jsr          __arg_value2_i6			// prev
/* @33 */ 	ldx          #6
	jsr          __arg_value2_i10			// new_length
/* @37 */ 	ldx          #8
	jsr          __arg_value2_i11			// orig_length
/* @42 */ 	lda          #__i5
/* @44 */ 	ldx          #7
/* @46 */ 	jsr         __set_var_value2
/* @49 */ 	lda         __i6
/* @51 */ 	bne         .MergeWithFreeBlockBelow_label_69
/* @53 */ 	lda         __i6+1
/* @55 */ 	bne         .MergeWithFreeBlockBelow_label_69
/* @59 */ 	lda         #%lo(__free_list)
/* @60 */ 	sta         __i0
/* @61 */ 	lda         #%hi(__free_list)
/* @62 */ 	sta         __i0+1
/* @64 */ 	lda         __i0
/* @65 */ 	sta         __i7
/* @66 */ 	lda         __i0+1
/* @67 */ 	sta         __i7+1
/* @68 */ 	bra         .MergeWithFreeBlockBelow_label_85
.MergeWithFreeBlockBelow_label_69:
/* @72 */ 	clc         
/* @73 */ 	lda         __i6
/* @74 */ 	adc          #2
/* @75 */ 	sta         __i0
/* @76 */ 	lda         __i6+1
/* @77 */ 	adc          #0
/* @78 */ 	sta         __i0+1
/* @81 */ 	lda         __i0
/* @82 */ 	sta         __i7
/* @83 */ 	lda         __i0+1
/* @84 */ 	sta         __i7+1
.MergeWithFreeBlockBelow_label_85:
/* @89 */ 	ldy          #2
/* @90 */ 	lda         (__i5), Y
/* @91 */ 	sta         __i0
/* @93 */ 	iny         
/* @94 */ 	lda         (__i5), Y
/* @95 */ 	sta         __i0+1
/* @98 */ 	lda         __i0
/* @99 */ 	sta         __i8
/* @100 */ 	lda         __i0+1
/* @101 */ 	sta         __i8+1
/* @103 */ 	ldx          #7
	jsr          __var_value2_i0			// free_addr
/* @107 */ 	clc         
/* @108 */ 	lda         __i0
/* @109 */ 	adc         __i10
/* @110 */ 	sta         __i1
/* @111 */ 	lda         __i0+1
/* @112 */ 	adc         __i10+1
/* @113 */ 	sta         __i1+1
/* @117 */ 	clc         
/* @118 */ 	lda         __i1
/* @119 */ 	adc          #2
/* @120 */ 	sta         __i0
/* @121 */ 	lda         __i1+1
/* @122 */ 	adc          #0
/* @123 */ 	sta         __i0+1
/* @126 */ 	lda         __i0
/* @127 */ 	sta         __i9
/* @128 */ 	lda         __i0+1
/* @129 */ 	sta         __i9+1
/* @132 */ 	lda         (__i5)
/* @133 */ 	sta         __i0
/* @134 */ 	ldy          #1
/* @135 */ 	lda         (__i5), Y
/* @136 */ 	sta         __i0+1
/* @140 */ 	clc         
/* @141 */ 	lda         __i0
/* @142 */ 	adc         __i11
/* @143 */ 	sta         __i1
/* @144 */ 	lda         __i0+1
/* @145 */ 	adc         __i11+1
/* @146 */ 	sta         __i1+1
/* @150 */ 	sec         
/* @151 */ 	lda         __i1
/* @152 */ 	sbc         __i10
/* @153 */ 	sta         __i0
/* @154 */ 	lda         __i1+1
/* @155 */ 	sbc         __i10+1
/* @156 */ 	sta         __i0+1
/* @159 */ 	lda         __i0
/* @160 */ 	sta         (__i9)
/* @161 */ 	lda         __i0+1
/* @163 */ 	sta         (__i9), Y
/* @164 */ 	lda         __i8
/* @165 */ 	iny         
/* @166 */ 	sta         (__i9), Y
/* @167 */ 	lda         __i8+1
/* @168 */ 	iny         
/* @169 */ 	sta         (__i9), Y
/* @170 */ 	lda         __i9
/* @171 */ 	sta         (__i7)
/* @172 */ 	lda         __i9+1
/* @173 */ 	ldy          #1
/* @174 */ 	sta         (__i7), Y
/* @175 */ 	lda          #__i5
/* @177 */ 	ldx          #5
/* @178 */ 	jsr         __set_var_value2
/* @180 */ 	ldx          #5
	jsr          __var_value2_i0			// len_ptr
/* @183 */ 	lda         __i10
/* @184 */ 	sta         (__i0)
/* @185 */ 	lda         __i10+1
/* @186 */ 	ldy          #1
/* @187 */ 	sta         (__i0), Y
/* @188 */ 	jsr         __pushi11
/* @189 */ 	jsr         __pushi4
/* @191 */ 	ldx          #5
	jsr          __var_value2_i0			// len_ptr
/* @195 */ 	clc         
/* @196 */ 	lda         __i0
/* @197 */ 	adc          #2
/* @198 */ 	sta         __i1
/* @199 */ 	lda         __i0+1
/* @200 */ 	adc          #0
/* @201 */ 	sta         __i1+1
/* @204 */ 	jsr         __pushi1
/* @206 */ 	ldx         #__i0
/* @207 */ 	ldy          #0
/* @208 */ 	jsr         memmove
/* @210 */ 	jsr         __incsp6
/* @212 */ 	ldx          #5
	jsr          __var_value2_i0			// len_ptr
/* @216 */ 	clc         
/* @217 */ 	lda         __i0
/* @218 */ 	adc          #2
/* @219 */ 	sta         __i1
/* @220 */ 	lda         __i0+1
/* @221 */ 	adc          #0
/* @222 */ 	sta         __i1+1
/* @224 */ 	ldx          #12
	jsr          __load_result
/* @225 */ 	lda         #__i1
/* @227 */ 	jsr         __result2
/* @229 */ 	ldy          #14
	jmp          __leave
.func_end_MergeWithFreeBlockBelow:
	.size MergeWithFreeBlockBelow, .func_end_MergeWithFreeBlockBelow-MergeWithFreeBlockBelow

	.global realloc
	.type realloc, @function

realloc:
/* @9 */ 	stx         __result
/* @11 */ 	sty         __result+1
/* @12 */ 	ldx          #19
	jsr          __enter
	.byte        0x29,0x00,0x00		// Save mask i:9 b:1 l:0 x:0 f:0 
/* @27 */ 	ldx          #2
	jsr          __arg_value2_i5			// n
/* @55 */ 	ldx          #0
	jsr          __arg_value2_i0			// p
/* @59 */ 	lda         __i0
/* @61 */ 	bne         .realloc_label_82
/* @63 */ 	lda         __i0+1
/* @65 */ 	bne         .realloc_label_82
/* @67 */ 	jsr         __pushi5
/* @69 */ 	ldx         #__i12
/* @70 */ 	ldy          #0
/* @71 */ 	jsr         malloc
/* @73 */ 	jsr         __incsp2
/* @75 */ 	ldx          #20
	jsr          __load_result
/* @76 */ 	lda         #__i12
/* @78 */ 	jsr         __result2
.realloc_label_79:
/* @80 */ 	ldy          #22
	jmp          __leave
.realloc_label_82:
/* @84 */ 	ldx          #0
	jsr          __arg_value2_i0			// p
/* @88 */ 	sec         
/* @89 */ 	lda         __i0
/* @90 */ 	sbc          #2
/* @91 */ 	sta         __i1
/* @92 */ 	lda         __i0+1
/* @93 */ 	sbc          #0
/* @94 */ 	sta         __i1+1
/* @97 */ 	lda         __i1
/* @98 */ 	sta         __i6
/* @99 */ 	lda         __i1+1
/* @100 */ 	sta         __i6+1
/* @103 */ 	lda         (__i6)
/* @104 */ 	sta         __i0
/* @105 */ 	ldy          #1
/* @106 */ 	lda         (__i6), Y
/* @107 */ 	sta         __i0+1
/* @110 */ 	lda         __i0
/* @111 */ 	sta         __i7
/* @112 */ 	lda         __i0+1
/* @113 */ 	sta         __i7+1
/* @115 */ 	ldx          #0
	jsr          __arg_value2_i0			// p
/* @119 */ 	sec         
/* @120 */ 	lda         __i0
/* @121 */ 	sbc          #2
/* @122 */ 	sta         __i1
/* @123 */ 	lda         __i0+1
/* @124 */ 	sbc          #0
/* @125 */ 	sta         __i1+1
/* @128 */ 	lda         __i1
/* @129 */ 	sta         __i8
/* @130 */ 	lda         __i1+1
/* @131 */ 	sta         __i8+1
/* @133 */ 	ldx          #0
	jsr          __arg_value2_i0			// p
/* @136 */ 	lda         __i0
/* @137 */ 	sta         __i9
/* @138 */ 	lda         __i0+1
/* @139 */ 	sta         __i9+1
/* @140 */ 	jsr         __pushi5
/* @141 */ 	ldx         #__i5
/* @142 */ 	ldy          #0
/* @143 */ 	jsr         AlignSize
/* @144 */ 	jsr         __incsp2
/* @146 */ 	lda         __i5
/* @147 */ 	cmp         __i7
/* @148 */ 	bne         .realloc_label_160
/* @149 */ 	lda         __i5+1
/* @150 */ 	cmp         __i7+1
/* @151 */ 	bne         .realloc_label_160
/* @154 */ 	ldx          #0
	jsr          __arg_value2_i0			// p
/* @156 */ 	ldx          #20
	jsr          __load_result
/* @157 */ 	lda         #__i0
/* @158 */ 	jsr         __result2
/* @159 */ 	jmp         .realloc_label_79
.realloc_label_160:
/* @162 */ 	lda         __i5+1
/* @163 */ 	cmp         __i7+1
/* @164 */ 	bcc         .realloc_label_161
/* @165 */ 	bne         .realloc_label_184
/* @166 */ 	lda         __i5
/* @167 */ 	cmp         __i7
/* @168 */ 	bcs         .realloc_label_184
.realloc_label_161:
/* @170 */ 	jsr         __pushi6
/* @171 */ 	jsr         __pushi5
/* @172 */ 	jsr         __pushi7
/* @173 */ 	jsr         __pushi8
/* @174 */ 	jsr         ShrinkBlock
/* @176 */ 	jsr         __incsp8
/* @178 */ 	ldx          #0
	jsr          __arg_value2_i0			// p
/* @180 */ 	ldx          #20
	jsr          __load_result
/* @181 */ 	lda         #__i0
/* @182 */ 	jsr         __result2
/* @183 */ 	jmp         .realloc_label_79
.realloc_label_184:
/* @185 */ 	lda         __free_list+0
/* @186 */ 	sta         __i10
/* @187 */ 	lda         __free_list+1
/* @188 */ 	sta         __i10+1
/* @189 */ 	stz         __i11
/* @190 */ 	stz         __i11+1
/* @193 */ 	stz         __i0
/* @194 */ 	stz         __i0+1
/* @195 */ 	lda          #__i0
/* @197 */ 	ldx          #5
/* @199 */ 	jsr         __set_var_value2
.realloc_label_200:
/* @202 */ 	lda         __i10
/* @204 */ 	bne         .realloc_label_201
/* @205 */ 	lda         __i10+1
/* @207 */ 	bne         .realloc_label_568
/* @569 */ 	jmp         .realloc_label_510
.realloc_label_568:
.realloc_label_201:
/* @210 */ 	lda         __i11
/* @212 */ 	bne         .realloc_label_228
/* @213 */ 	lda         __i11+1
/* @215 */ 	bne         .realloc_label_228
/* @219 */ 	lda         #%lo(__free_list)
/* @220 */ 	sta         __i0
/* @221 */ 	lda         #%hi(__free_list)
/* @222 */ 	sta         __i0+1
/* @223 */ 	lda          #__i0
/* @225 */ 	ldx          #7
/* @226 */ 	jsr         __set_var_value2
/* @227 */ 	bra         .realloc_label_242
.realloc_label_228:
/* @231 */ 	clc         
/* @232 */ 	lda         __i11
/* @233 */ 	adc          #2
/* @234 */ 	sta         __i0
/* @235 */ 	lda         __i11+1
/* @236 */ 	adc          #0
/* @237 */ 	sta         __i0+1
/* @239 */ 	lda          #__i0
/* @240 */ 	ldx          #7
/* @241 */ 	jsr         __set_var_value2
.realloc_label_242:
/* @244 */ 	lda         __i8
/* @245 */ 	cmp         __i10
/* @246 */ 	lda         __i8+1
/* @247 */ 	sbc         __i10+1
/* @248 */ 	bvc         .realloc_label_243
/* @250 */ 	eor          #128
.realloc_label_243:
/* @251 */ 	bmi         .realloc_label_570
/* @571 */ 	jmp         .realloc_label_485
.realloc_label_570:
/* @252 */ 	lda          #__i10
/* @254 */ 	ldx          #9
/* @255 */ 	jsr         __set_var_value2
/* @258 */ 	sec         
/* @259 */ 	lda         __i5
/* @260 */ 	sbc         __i7
/* @261 */ 	sta         __i0
/* @262 */ 	lda         __i5+1
/* @263 */ 	sbc         __i7+1
/* @264 */ 	sta         __i0+1
/* @266 */ 	lda          #__i0
/* @268 */ 	ldx          #11
/* @269 */ 	jsr         __set_var_value2
/* @272 */ 	clc         
/* @273 */ 	lda         __i9
/* @274 */ 	adc         __i7
/* @275 */ 	sta         __i0
/* @276 */ 	lda         __i9+1
/* @277 */ 	adc         __i7+1
/* @278 */ 	sta         __i0+1
/* @280 */ 	ldx          #9
	jsr          __var_value2_i1			// free_addr
/* @284 */ 	lda         __i0
/* @285 */ 	cmp         __i1
/* @286 */ 	beq         .realloc_label_572
/* @573 */ 	jmp         .realloc_label_377
.realloc_label_572:
/* @287 */ 	lda         __i0+1
/* @288 */ 	cmp         __i1+1
/* @289 */ 	beq         .realloc_label_574
/* @575 */ 	jmp         .realloc_label_377
.realloc_label_574:
/* @293 */ 	lda         (__i10)
/* @294 */ 	sta         __i0
/* @295 */ 	ldy          #1
/* @296 */ 	lda         (__i10), Y
/* @297 */ 	sta         __i0+1
/* @299 */ 	ldx          #11
	jsr          __var_value2_i1			// diff
/* @303 */ 	lda         __i1+1
/* @304 */ 	cmp         __i0+1
/* @305 */ 	bcc         .realloc_label_302
/* @306 */ 	beq         .realloc_label_576
/* @577 */ 	jmp         .realloc_label_376
.realloc_label_576:
/* @307 */ 	lda         __i1
/* @308 */ 	cmp         __i0
/* @309 */ 	bcs         .realloc_label_376
.realloc_label_302:
/* @313 */ 	lda         (__i10)
/* @314 */ 	sta         __i0
/* @315 */ 	ldy          #1
/* @316 */ 	lda         (__i10), Y
/* @317 */ 	sta         __i0+1
/* @319 */ 	ldx          #11
	jsr          __var_value2_i1			// diff
/* @324 */ 	sec         
/* @325 */ 	lda         __i0
/* @326 */ 	sbc         __i1
/* @327 */ 	sta         __i2
/* @328 */ 	lda         __i0+1
/* @329 */ 	sbc         __i1+1
/* @330 */ 	sta         __i2+1
/* @332 */ 	lda          #__i2
/* @334 */ 	ldx          #13
/* @335 */ 	jsr         __set_var_value2
/* @337 */ 	ldx          #13
	jsr          __var_value2_i0			// freelen
/* @340 */ 	lda          #4
/* @341 */ 	cmp         __i0
/* @342 */ 	lda          #0
/* @343 */ 	sbc         __i0+1
/* @344 */ 	bvc         .realloc_label_339
/* @345 */ 	eor          #128
.realloc_label_339:
/* @346 */ 	bpl         .realloc_label_375
/* @348 */ 	ldx          #7
	jsr          __var_value2_i0			// next_ptr
/* @351 */ 	jsr         __pushi0
/* @352 */ 	jsr         __pushi6
/* @354 */ 	ldx          #13
	jsr          __var_value2_i0			// freelen
/* @357 */ 	jsr         __pushi0
/* @359 */ 	ldx          #11
	jsr          __var_value2_i0			// diff
/* @362 */ 	jsr         __pushi0
/* @363 */ 	jsr         __pushi5
/* @364 */ 	jsr         __pushi10
/* @365 */ 	jsr         ExpandIntoFreeBlockAbove
/* @367 */ 	jsr         __incsp12
/* @369 */ 	ldx          #0
	jsr          __arg_value2_i0			// p
/* @371 */ 	ldx          #20
	jsr          __load_result
/* @372 */ 	lda         #__i0
/* @373 */ 	jsr         __result2
/* @374 */ 	jmp         .realloc_label_79
.realloc_label_375:
.realloc_label_376:
.realloc_label_377:
/* @379 */ 	lda         __i11
/* @381 */ 	bne         .realloc_label_378
/* @382 */ 	lda         __i11+1
/* @384 */ 	bne         .realloc_label_578
/* @579 */ 	jmp         .realloc_label_484
.realloc_label_578:
.realloc_label_378:
/* @386 */ 	lda          #__i11
/* @388 */ 	ldx          #15
/* @389 */ 	jsr         __set_var_value2
/* @393 */ 	ldx          #15
	jsr          __var_value2_i0			// prev_addr
/* @396 */ 	lda         (__i11)
/* @397 */ 	sta         __i1
/* @398 */ 	ldy          #1
/* @399 */ 	lda         (__i11), Y
/* @400 */ 	sta         __i1+1
/* @405 */ 	clc         
/* @406 */ 	lda         __i0
/* @407 */ 	adc         __i1
/* @408 */ 	sta         __i2
/* @409 */ 	lda         __i0+1
/* @410 */ 	adc         __i1+1
/* @411 */ 	sta         __i2+1
/* @416 */ 	ldx          #1
/* @417 */ 	lda         __i2
/* @418 */ 	cmp         __i8
/* @419 */ 	bne         .realloc_label_414
/* @420 */ 	lda         __i2+1
/* @421 */ 	cmp         __i8+1
/* @422 */ 	beq         .realloc_label_415
.realloc_label_414:
/* @423 */ 	dex         
.realloc_label_415:
/* @424 */ 	stx         __b2
/* @426 */ 	txa         
/* @427 */ 	cmp          #0
/* @428 */ 	beq         .realloc_label_453
/* @431 */ 	lda         (__i11)
/* @432 */ 	sta         __i0
/* @433 */ 	ldy          #1
/* @434 */ 	lda         (__i11), Y
/* @435 */ 	sta         __i0+1
/* @437 */ 	ldx          #11
	jsr          __var_value2_i1			// diff
/* @443 */ 	ldx          #1
/* @444 */ 	lda         __i0+1
/* @445 */ 	cmp         __i1+1
/* @446 */ 	bcc         .realloc_label_441
/* @447 */ 	bne         .realloc_label_442
/* @448 */ 	lda         __i0
/* @449 */ 	cmp         __i1
/* @450 */ 	bcs         .realloc_label_442
.realloc_label_441:
/* @451 */ 	dex         
.realloc_label_442:
/* @452 */ 	stx         __b2
.realloc_label_453:
/* @455 */ 	lda         __b2
/* @457 */ 	beq         .realloc_label_482
/* @458 */ 	jsr         __pushi7
/* @459 */ 	jsr         __pushi5
/* @460 */ 	jsr         __pushi11
/* @462 */ 	ldx          #5
	jsr          __var_value2_i0			// prev_prev
/* @465 */ 	jsr         __pushi0
/* @467 */ 	ldx          #0
	jsr          __arg_value2_i0			// p
/* @470 */ 	jsr         __pushi0
/* @472 */ 	ldx         #__i12
/* @473 */ 	ldy          #0
/* @474 */ 	jsr         MergeWithFreeBlockBelow
/* @476 */ 	jsr         __incsp10
/* @478 */ 	ldx          #20
	jsr          __load_result
/* @479 */ 	lda         #__i12
/* @480 */ 	jsr         __result2
/* @481 */ 	jmp         .realloc_label_79
.realloc_label_482:
/* @483 */ 	bra         .realloc_label_510
.realloc_label_484:
.realloc_label_485:
/* @486 */ 	lda          #__i11
/* @487 */ 	ldx          #5
/* @488 */ 	jsr         __set_var_value2
/* @489 */ 	lda         __i10
/* @490 */ 	sta         __i11
/* @491 */ 	lda         __i10+1
/* @492 */ 	sta         __i11+1
/* @496 */ 	ldy          #2
/* @497 */ 	lda         (__i10), Y
/* @498 */ 	sta         __i0
/* @500 */ 	iny         
/* @501 */ 	lda         (__i10), Y
/* @502 */ 	sta         __i0+1
/* @505 */ 	lda         __i0
/* @506 */ 	sta         __i10
/* @507 */ 	lda         __i0+1
/* @508 */ 	sta         __i10+1
/* @509 */ 	jmp         .realloc_label_200
.realloc_label_510:
/* @511 */ 	jsr         __pushi5
/* @512 */ 	ldx         #__i4
/* @513 */ 	ldy          #0
/* @514 */ 	jsr         malloc
/* @515 */ 	jsr         __incsp2
/* @517 */ 	lda         __i4
/* @519 */ 	bne         .realloc_label_532
/* @520 */ 	lda         __i4+1
/* @522 */ 	bne         .realloc_label_532
/* @526 */ 	stz         __i0
/* @527 */ 	stz         __i0+1
/* @528 */ 	ldx          #20
	jsr          __load_result
/* @529 */ 	lda         #__i0
/* @530 */ 	jsr         __result2
/* @531 */ 	jmp         .realloc_label_79
.realloc_label_532:
/* @534 */ 	ldx          #0
	jsr          __arg_value2_i0			// p
/* @536 */ 	lda         __i7
/* @538 */ 	sta         __mem_size
/* @539 */ 	lda         __i7+1
/* @541 */ 	sta         __mem_size+1
/* @543 */ 	lda         __i0
/* @545 */ 	sta         __mem_src
/* @546 */ 	lda         __i0+1
/* @548 */ 	sta         __mem_src+1
/* @549 */ 	lda         __i4
/* @551 */ 	sta         __mem_dest
/* @552 */ 	lda         __i4+1
/* @554 */ 	sta         __mem_dest+1
/* @556 */ 	jsr         __builtin_memcpy
/* @558 */ 	ldx          #0
	jsr          __arg_value2_i0			// p
/* @561 */ 	jsr         __pushi0
/* @562 */ 	jsr         free
/* @563 */ 	jsr         __incsp2
/* @564 */ 	ldx          #20
	jsr          __load_result
/* @565 */ 	lda         #__i4
/* @566 */ 	jsr         __result2
/* @567 */ 	jmp         .realloc_label_79
.func_end_realloc:
	.size realloc, .func_end_realloc-realloc

	.data
	.section ".rodata", "aMS", @progbits
.str.3:
	.asciz "new_length < orig_length"
	.type .str.3, @object
	.size .str.3, 25

.str.4:
	.asciz "(null)"
	.type .str.4, @object
	.size .str.4, 1

.str.5:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.5, @object
	.size .str.5, 30

.str.13:
	.asciz "free_remaining > sizeof(FreeBlockHeader)"
	.type .str.13, @object
	.size .str.13, 41

.str.14:
	.asciz "(null)"
	.type .str.14, @object
	.size .str.14, 1

.str.15:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.15, @object
	.size .str.15, 30

