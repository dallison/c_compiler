	.file   "/Users/dallison/Google Drive/c_compiler/libc/malloc.c"
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


	.global InitFreeList
	.type InitFreeList, @function

InitFreeList:
/* @4 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @8 */ 	lda         #%lo(_end)
/* @9 */ 	sta         __free_list+0
/* @11 */ 	lda         #%hi(_end)
/* @12 */ 	sta         __free_list+1
/* @15 */ 	lda         #%lo(_end)
/* @16 */ 	sta         __i0
/* @17 */ 	lda         #%hi(_end)
/* @18 */ 	sta         __i0+1
/* @22 */ 	sec         
/* @23 */ 	lda          #0
/* @24 */ 	sbc         __i0
/* @25 */ 	sta         __i1
/* @26 */ 	lda          #192
/* @27 */ 	sbc         __i0+1
/* @28 */ 	sta         __i1+1
/* @32 */ 	lda         __free_list+0
/* @33 */ 	sta         __i0
/* @34 */ 	lda         __free_list+1
/* @35 */ 	sta         __i0+1
/* @38 */ 	lda         __i1
/* @39 */ 	sta         (__i0)
/* @40 */ 	lda         __i1+1
/* @41 */ 	ldy          #1
/* @42 */ 	sta         (__i0), Y
/* @45 */ 	lda         __free_list+0
/* @46 */ 	sta         __i0
/* @47 */ 	lda         __free_list+1
/* @48 */ 	sta         __i0+1
/* @51 */ 	iny         
/* @52 */ 	lda          #0
.InitFreeList_label_53:
/* @55 */ 	sta         (__i0), Y
/* @56 */ 	iny         
/* @58 */ 	cpy          #4
/* @59 */ 	bne         .InitFreeList_label_53
/* @60 */ 	ldy          #8
	jmp          __leave_leaf_void_nomask
.func_end_InitFreeList:
	.size InitFreeList, .func_end_InitFreeList-InitFreeList

	.global AlignSize
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

	.global ExpandHeap
	.type ExpandHeap, @function

ExpandHeap:
/* @3 */ 	stx         __result
/* @5 */ 	sty         __result+1
/* @6 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @10 */ 	stz         __i0
/* @12 */ 	stz         __i0+1
/* @13 */ 	lda         #__i0
/* @15 */ 	jsr         __result2
/* @17 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_ExpandHeap:
	.size ExpandHeap, .func_end_ExpandHeap-ExpandHeap

	.global TakeStartOfFreeBlock
	.type TakeStartOfFreeBlock, @function

TakeStartOfFreeBlock:
/* @11 */ 	stx         __result
/* @13 */ 	sty         __result+1
/* @14 */ 	ldx          #7
	jsr          __enter
	.byte        0x06,0x00,0x00		// Save mask i:6 b:0 l:0 x:0 f:0 
/* @21 */ 	ldx          #0
	jsr          __arg_value2_i4			// block
/* @25 */ 	ldx          #4
	jsr          __arg_value2_i5			// full_length
/* @34 */ 	ldx          #6
	jsr          __arg_value2_i8			// prev
/* @38 */ 	ldx          #2
	jsr          __arg_value2_i9			// num_bytes
/* @42 */ 	lda         (__i4)
/* @43 */ 	sta         __i0
/* @45 */ 	ldy          #1
/* @46 */ 	lda         (__i4), Y
/* @47 */ 	sta         __i0+1
/* @50 */ 	lda         __i5+1
/* @51 */ 	cmp         __i0+1
/* @52 */ 	bcc         .TakeStartOfFreeBlock_label_49
/* @53 */ 	bne         .TakeStartOfFreeBlock_label_59
/* @54 */ 	lda         __i5
/* @55 */ 	cmp         __i0
/* @56 */ 	bcs         .TakeStartOfFreeBlock_label_59
.TakeStartOfFreeBlock_label_49:
/* @58 */ 	bra         .TakeStartOfFreeBlock_label_81
.TakeStartOfFreeBlock_label_59:
/* @61 */ 	ldx         #%lo(.str.7)
	ldy         #%hi(.str.7)
/* @63 */ 	jsr         __pushxy
/* @65 */ 	ldx          #95
/* @67 */ 	jsr         __pushxy0
/* @69 */ 	ldx         #%lo(.str.8)
	ldy         #%hi(.str.8)
/* @70 */ 	jsr         __pushxy
/* @72 */ 	ldx         #%lo(.str.9)
	ldy         #%hi(.str.9)
/* @73 */ 	jsr         __pushxy
/* @75 */ 	ldx         #__i0
/* @76 */ 	ldy          #0
/* @77 */ 	jsr         printf
/* @79 */ 	jsr         __incsp8
/* @80 */ 	jsr         abort
.TakeStartOfFreeBlock_label_81:
/* @84 */ 	lda         (__i4)
/* @85 */ 	sta         __i0
/* @86 */ 	ldy          #1
/* @87 */ 	lda         (__i4), Y
/* @88 */ 	sta         __i0+1
/* @92 */ 	sec         
/* @93 */ 	lda         __i0
/* @94 */ 	sbc         __i5
/* @95 */ 	sta         __i1
/* @96 */ 	lda         __i0+1
/* @97 */ 	sbc         __i5+1
/* @98 */ 	sta         __i1+1
/* @101 */ 	lda         __i1
/* @102 */ 	sta         __i6
/* @103 */ 	lda         __i1+1
/* @104 */ 	sta         __i6+1
/* @107 */ 	cmp          #0
/* @108 */ 	bcc         .TakeStartOfFreeBlock_label_174
/* @109 */ 	bne         .TakeStartOfFreeBlock_label_105
/* @110 */ 	lda         __i6
/* @111 */ 	cmp          #4
/* @112 */ 	bcc         .TakeStartOfFreeBlock_label_174
.TakeStartOfFreeBlock_label_105:
/* @116 */ 	clc         
/* @117 */ 	lda         __i4
/* @118 */ 	adc         __i5
/* @119 */ 	sta         __i0
/* @120 */ 	lda         __i4+1
/* @121 */ 	adc         __i5+1
/* @122 */ 	sta         __i0+1
/* @125 */ 	lda         __i0
/* @126 */ 	sta         __i7
/* @127 */ 	lda         __i0+1
/* @128 */ 	sta         __i7+1
/* @129 */ 	lda         __i6
/* @130 */ 	sta         (__i7)
/* @131 */ 	lda         __i6+1
/* @132 */ 	ldy          #1
/* @133 */ 	sta         (__i7), Y
/* @137 */ 	iny         
/* @138 */ 	lda         (__i4), Y
/* @139 */ 	sta         __i0
/* @141 */ 	iny         
/* @142 */ 	lda         (__i4), Y
/* @143 */ 	sta         __i0+1
/* @146 */ 	lda         __i0
/* @147 */ 	dey         
/* @148 */ 	sta         (__i7), Y
/* @149 */ 	lda         __i0+1
/* @150 */ 	iny         
/* @151 */ 	sta         (__i7), Y
/* @153 */ 	lda         __i8
/* @155 */ 	bne         .TakeStartOfFreeBlock_label_165
/* @156 */ 	lda         __i8+1
/* @158 */ 	bne         .TakeStartOfFreeBlock_label_165
/* @160 */ 	lda         __i7
/* @161 */ 	sta         __free_list+0
/* @162 */ 	lda         __i7+1
/* @163 */ 	sta         __free_list+1
/* @164 */ 	bra         .TakeStartOfFreeBlock_label_172
.TakeStartOfFreeBlock_label_165:
/* @166 */ 	lda         __i7
/* @167 */ 	ldy          #2
/* @168 */ 	sta         (__i8), Y
/* @169 */ 	lda         __i7+1
/* @170 */ 	iny         
/* @171 */ 	sta         (__i8), Y
.TakeStartOfFreeBlock_label_172:
/* @173 */ 	bra         .TakeStartOfFreeBlock_label_238
.TakeStartOfFreeBlock_label_174:
/* @176 */ 	lda         __i8
/* @178 */ 	bne         .TakeStartOfFreeBlock_label_197
/* @179 */ 	lda         __i8+1
/* @181 */ 	bne         .TakeStartOfFreeBlock_label_197
/* @185 */ 	ldy          #2
/* @186 */ 	lda         (__i4), Y
/* @187 */ 	sta         __i0
/* @188 */ 	iny         
/* @189 */ 	lda         (__i4), Y
/* @190 */ 	sta         __i0+1
/* @192 */ 	lda         __i0
/* @193 */ 	sta         __free_list+0
/* @194 */ 	lda         __i0+1
/* @195 */ 	sta         __free_list+1
/* @196 */ 	bra         .TakeStartOfFreeBlock_label_214
.TakeStartOfFreeBlock_label_197:
/* @200 */ 	ldy          #2
/* @201 */ 	lda         (__i4), Y
/* @202 */ 	sta         __i0
/* @203 */ 	iny         
/* @204 */ 	lda         (__i4), Y
/* @205 */ 	sta         __i0+1
/* @208 */ 	lda         __i0
/* @209 */ 	dey         
/* @210 */ 	sta         (__i8), Y
/* @211 */ 	lda         __i0+1
/* @212 */ 	iny         
/* @213 */ 	sta         (__i8), Y
.TakeStartOfFreeBlock_label_214:
/* @217 */ 	lda         (__i4)
/* @218 */ 	sta         __i0
/* @219 */ 	ldy          #1
/* @220 */ 	lda         (__i4), Y
/* @221 */ 	sta         __i0+1
/* @225 */ 	sec         
/* @226 */ 	lda         __i0
/* @227 */ 	sbc          #2
/* @228 */ 	sta         __i1
/* @229 */ 	lda         __i0+1
/* @230 */ 	sbc          #0
/* @231 */ 	sta         __i1+1
/* @234 */ 	lda         __i1
/* @235 */ 	sta         __i9
/* @236 */ 	lda         __i1+1
/* @237 */ 	sta         __i9+1
.TakeStartOfFreeBlock_label_238:
/* @239 */ 	ldx          #8
	jsr          __load_result
/* @240 */ 	lda         #__i9
/* @242 */ 	jsr         __result2
/* @244 */ 	ldy          #10
	jmp          __leave
.func_end_TakeStartOfFreeBlock:
	.size TakeStartOfFreeBlock, .func_end_TakeStartOfFreeBlock-TakeStartOfFreeBlock

	.global malloc
	.type malloc, @function

malloc:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x05,0x00,0x00		// Save mask i:5 b:0 l:0 x:0 f:0 
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i4			// n
/* @31 */ 	lda         #%lo(__free_list)
/* @32 */ 	sta         __i0
/* @34 */ 	lda         #%hi(__free_list)
/* @35 */ 	sta         __i0+1
/* @38 */ 	lda         (__i0)
/* @40 */ 	bne         .malloc_label_47
/* @41 */ 	ldy          #1
/* @42 */ 	lda         (__i0), Y
/* @44 */ 	bne         .malloc_label_47
/* @46 */ 	jsr         InitFreeList
.malloc_label_47:
/* @48 */ 	jsr         __pushi4
/* @49 */ 	ldx         #__i4
/* @50 */ 	ldy          #0
/* @51 */ 	jsr         AlignSize
/* @53 */ 	jsr         __incsp2
/* @56 */ 	clc         
/* @57 */ 	lda         __i4
/* @58 */ 	adc          #2
/* @59 */ 	sta         __i0
/* @60 */ 	lda         __i4+1
/* @61 */ 	adc          #0
/* @62 */ 	sta         __i0+1
/* @65 */ 	lda         __i0
/* @66 */ 	sta         __i5
/* @67 */ 	lda         __i0+1
/* @68 */ 	sta         __i5+1
/* @69 */ 	lda         __free_list+0
/* @70 */ 	sta         __i6
/* @71 */ 	lda         __free_list+1
/* @72 */ 	sta         __i6+1
/* @73 */ 	stz         __i7
/* @74 */ 	stz         __i7+1
.malloc_label_75:
/* @77 */ 	lda         __i6
/* @79 */ 	bne         .malloc_label_76
/* @80 */ 	lda         __i6+1
/* @82 */ 	bne         .malloc_label_179
/* @180 */ 	jmp         .malloc_label_170
.malloc_label_179:
.malloc_label_76:
/* @86 */ 	lda         (__i6)
/* @87 */ 	sta         __i0
/* @88 */ 	ldy          #1
/* @89 */ 	lda         (__i6), Y
/* @90 */ 	sta         __i0+1
/* @94 */ 	cmp         __i5+1
/* @95 */ 	bcc         .malloc_label_136
/* @96 */ 	bne         .malloc_label_92
/* @97 */ 	lda         __i0
/* @98 */ 	cmp         __i5
/* @99 */ 	bcc         .malloc_label_136
.malloc_label_92:
/* @101 */ 	jsr         __pushi7
/* @102 */ 	jsr         __pushi5
/* @103 */ 	jsr         __pushi4
/* @104 */ 	jsr         __pushi6
/* @105 */ 	ldx         #__i4
/* @106 */ 	ldy          #0
/* @107 */ 	jsr         TakeStartOfFreeBlock
/* @109 */ 	jsr         __incsp8
/* @110 */ 	lda         __i6
/* @111 */ 	sta         __i8
/* @112 */ 	lda         __i6+1
/* @113 */ 	sta         __i8+1
/* @114 */ 	lda         __i4
/* @115 */ 	sta         (__i8)
/* @116 */ 	lda         __i4+1
/* @117 */ 	ldy          #1
/* @118 */ 	sta         (__i8), Y
/* @121 */ 	clc         
/* @122 */ 	lda         __i6
/* @123 */ 	adc          #2
/* @124 */ 	sta         __i0
/* @125 */ 	lda         __i6+1
/* @126 */ 	adc          #0
/* @127 */ 	sta         __i0+1
/* @129 */ 	ldx          #8
	jsr          __load_result
/* @130 */ 	lda         #__i0
/* @132 */ 	jsr         __result2
.malloc_label_133:
/* @134 */ 	ldy          #10
	jmp          __leave
.malloc_label_136:
/* @137 */ 	lda         __i6
/* @138 */ 	sta         __i7
/* @139 */ 	lda         __i6+1
/* @140 */ 	sta         __i7+1
/* @144 */ 	ldy          #2
/* @145 */ 	lda         (__i6), Y
/* @146 */ 	sta         __i0
/* @148 */ 	iny         
/* @149 */ 	lda         (__i6), Y
/* @150 */ 	sta         __i0+1
/* @153 */ 	lda         __i0
/* @154 */ 	sta         __i6
/* @155 */ 	lda         __i0+1
/* @156 */ 	sta         __i6+1
/* @158 */ 	lda         __i6
/* @160 */ 	bne         .malloc_label_168
/* @161 */ 	lda         __i6+1
/* @163 */ 	bne         .malloc_label_168
/* @165 */ 	ldx         #__i6
/* @166 */ 	ldy          #0
/* @167 */ 	jsr         ExpandHeap
.malloc_label_168:
/* @169 */ 	jmp         .malloc_label_75
.malloc_label_170:
/* @173 */ 	stz         __i0
/* @174 */ 	stz         __i0+1
/* @175 */ 	ldx          #8
	jsr          __load_result
/* @176 */ 	lda         #__i0
/* @177 */ 	jsr         __result2
/* @178 */ 	bra         .malloc_label_133
.func_end_malloc:
	.size malloc, .func_end_malloc-malloc

	.data
	.type   __free_list,@object
	.global __free_list
	.comm   __free_list,2,1

	.type   __initial_heap_size,@object
	.global __initial_heap_size
	.comm   __initial_heap_size,2,1

	.section ".rodata", "aMS", @progbits
.str.7:
	.asciz "block->length > full_length"
	.type .str.7, @object
	.size .str.7, 28

.str.8:
	.asciz "(null)"
	.type .str.8, @object
	.size .str.8, 1

.str.9:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.9, @object
	.size .str.9, 30

