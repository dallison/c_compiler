	.file   "main.c"
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


	.global Unpack
	.type Unpack, @function

Unpack:
/* @12 */ 	ldx          #17
	jsr          __enter_nomask
/* @18 */ 	ldx          #2
	jsr          __arg_value4_f0			// f
/* @21 */ 	lda          #__i0			// struct return address
	ldx          #0
	jsr          __arg_value2
/* @22 */ 	lda          #__f0
/* @24 */ 	ldx          #7
/* @26 */ 	jsr         __set_var_value4
/* @29 */ 	ldx          #7
	jsr          __var_addr_i1			// a
/* @33 */ 	ldy          #3
.Unpack_label_34:
/* @36 */ 	lda         (__i1), Y
/* @37 */ 	sta         __l0, Y
/* @38 */ 	dey         
/* @39 */ 	bpl         .Unpack_label_34
/* @44 */ 	lda          #0
/* @45 */ 	sta         __l1
/* @46 */ 	lda          #0
/* @48 */ 	sta         __l1+1
/* @49 */ 	lda          #0
/* @51 */ 	sta         __l1+2
/* @52 */ 	lda         __l0+3
/* @53 */ 	and          #128
/* @54 */ 	sta         __l1+3
/* @58 */ 	lda         __l1+3
/* @59 */ 	sta         __l0
/* @60 */ 	lda          #0
/* @61 */ 	sta         __l0+1
/* @62 */ 	sta         __l0+2
/* @63 */ 	sta         __l0+3
/* @66 */ 	ldx          #13
	jsr          __var_addr_i2			// u
/* @69 */ 	lda         __l0
/* @70 */ 	sta         (__i2)
/* @76 */ 	ldy          #3
.Unpack_label_77:
/* @78 */ 	lda         (__i1), Y
/* @79 */ 	sta         __l0, Y
/* @80 */ 	dey         
/* @81 */ 	bpl         .Unpack_label_77
/* @85 */ 	lda         __l0+2
/* @86 */ 	sta         __l1
/* @87 */ 	lda         __l0+3
/* @88 */ 	sta         __l1+1
/* @89 */ 	lda          #0
/* @90 */ 	sta         __l1+2
/* @91 */ 	sta         __l1+3
/* @92 */ 	ldx          #7
.Unpack_label_93:
/* @94 */ 	lsr         __l1+3
/* @95 */ 	ror         __l1+2
/* @96 */ 	ror         __l1+1
/* @97 */ 	ror         __l1
/* @98 */ 	dex         
/* @99 */ 	bne         .Unpack_label_93
/* @103 */ 	lda         __l1
/* @104 */ 	sta         __l0
/* @105 */ 	lda          #0
/* @106 */ 	sta         __l0+1
/* @107 */ 	lda          #0
/* @108 */ 	sta         __l0+2
/* @109 */ 	lda          #0
/* @110 */ 	sta         __l0+3
/* @116 */ 	lda         __l0
/* @117 */ 	ldy          #1
/* @118 */ 	sta         (__i2), Y
/* @124 */ 	ldy          #3
.Unpack_label_125:
/* @126 */ 	lda         (__i1), Y
/* @127 */ 	sta         __l0, Y
/* @128 */ 	dey         
/* @129 */ 	bpl         .Unpack_label_125
/* @133 */ 	lda         __l0
/* @134 */ 	sta         __l1
/* @135 */ 	lda         __l0+1
/* @136 */ 	sta         __l1+1
/* @137 */ 	lda         __l0+2
/* @138 */ 	and          #127
/* @139 */ 	sta         __l1+2
/* @140 */ 	lda          #0
/* @141 */ 	sta         __l1+3
/* @145 */ 	lda         __l1+2
/* @146 */ 	sta         __l0+3
/* @147 */ 	lda         __l1+1
/* @148 */ 	sta         __l0+2
/* @149 */ 	lda         __l1
/* @150 */ 	sta         __l0+1
/* @151 */ 	lda          #0
/* @152 */ 	sta         __l0
/* @156 */ 	lda         __l0
/* @157 */ 	sta         __l1
/* @158 */ 	lda         __l0+1
/* @159 */ 	sta         __l1+1
/* @160 */ 	lda         __l0+2
/* @161 */ 	sta         __l1+2
/* @162 */ 	lda         __l0+3
/* @163 */ 	ora          #128
/* @164 */ 	sta         __l1+3
/* @170 */ 	lda         __l1
/* @171 */ 	ldy          #2
/* @172 */ 	sta         (__i2), Y
/* @173 */ 	lda         __l1+1
/* @174 */ 	ldy          #3
/* @175 */ 	sta         (__i2), Y
/* @176 */ 	lda         __l1+2
/* @178 */ 	ldy          #4
/* @179 */ 	sta         (__i2), Y
/* @180 */ 	lda         __l1+3
/* @182 */ 	ldy          #5
/* @183 */ 	sta         (__i2), Y
/* @187 */ 	lda          #6
/* @189 */ 	sta         __mem_size
/* @191 */ 	lda         __i2
/* @193 */ 	sta         __mem_src
/* @194 */ 	lda         __i2+1
/* @196 */ 	sta         __mem_src+1
/* @197 */ 	lda         __i0
/* @199 */ 	sta         __mem_dest
/* @200 */ 	lda         __i0+1
/* @202 */ 	sta         __mem_dest+1
/* @204 */ 	jsr         __copymem1
.Unpack_label_205:
/* @206 */ 	ldy          #20
	jsr          __leave_void_nomask
/* @207 */ 	rts         
.func_end_Unpack:
	.size Unpack, .func_end_Unpack-Unpack

	.global Pack
	.type Pack, @function

Pack:
/* @10 */ 	stx         __result
/* @12 */ 	sty         __result+1
/* @13 */ 	ldx          #9
	jsr          __enter_leaf
	.byte        0x00,0x02,0x00		// Save mask i:0 b:0 l:1 x:0 f:0 
/* @20 */ 	lda          #0
/* @21 */ 	sta         __l0
/* @22 */ 	lda          #0
/* @24 */ 	sta         __l0+1
/* @25 */ 	lda          #0
/* @27 */ 	sta         __l0+2
/* @28 */ 	lda          #0
/* @30 */ 	sta         __l0+3
/* @31 */ 	lda          #__l0
/* @33 */ 	ldx          #7
/* @35 */ 	jsr         __set_var_value4
/* @38 */ 	ldx          #0
	jsr          __arg_addr_i0			// u
/* @41 */ 	ldy          #2
/* @42 */ 	lda         (__i0), Y
/* @43 */ 	sta         __l0
/* @44 */ 	ldy          #3
/* @45 */ 	lda         (__i0), Y
/* @46 */ 	sta         __l0+1
/* @48 */ 	ldy          #4
/* @49 */ 	lda         (__i0), Y
/* @50 */ 	sta         __l0+2
/* @52 */ 	ldy          #5
/* @53 */ 	lda         (__i0), Y
/* @54 */ 	sta         __l0+3
/* @58 */ 	lda         __l0+1
/* @59 */ 	sta         __l1
/* @60 */ 	lda         __l0+2
/* @61 */ 	sta         __l1+1
/* @62 */ 	lda         __l0+3
/* @63 */ 	sta         __l1+2
/* @64 */ 	lda          #0
/* @65 */ 	sta         __l1+3
/* @69 */ 	lda         __l1
/* @70 */ 	sta         __l0
/* @71 */ 	lda         __l1+1
/* @72 */ 	sta         __l0+1
/* @73 */ 	lda         __l1+2
/* @74 */ 	and          #127
/* @75 */ 	sta         __l0+2
/* @76 */ 	lda          #0
/* @77 */ 	sta         __l0+3
/* @80 */ 	ldx          #7
	jsr          __var_addr_i1			// bits
/* @83 */ 	ldy          #3
.Pack_label_84:
/* @86 */ 	lda         (__i1), Y
/* @87 */ 	sta         __l1, Y
/* @88 */ 	dey         
/* @89 */ 	bpl         .Pack_label_84
/* @94 */ 	lda         __l1
/* @95 */ 	ora         __l0
/* @96 */ 	sta         __l2
/* @97 */ 	lda         __l1+1
/* @98 */ 	ora         __l0+1
/* @99 */ 	sta         __l2+1
/* @100 */ 	lda         __l1+2
/* @101 */ 	ora         __l0+2
/* @102 */ 	sta         __l2+2
/* @103 */ 	lda         __l1+3
/* @104 */ 	ora         __l0+3
/* @105 */ 	sta         __l2+3
/* @111 */ 	ldy          #3
.Pack_label_112:
/* @113 */ 	lda         __l2, Y
/* @114 */ 	sta         (__i1), Y
/* @115 */ 	dey         
/* @116 */ 	bpl         .Pack_label_112
/* @122 */ 	ldy          #1
/* @123 */ 	lda         (__i0), Y
/* @124 */ 	sta         __b0
/* @128 */ 	lda         __b0
/* @129 */ 	sta         __l0
/* @130 */ 	lda          #0
/* @131 */ 	ldy          #3
.Pack_label_132:
/* @133 */ 	sta         __l0, Y
/* @134 */ 	dey         
/* @135 */ 	cpy          #0
/* @136 */ 	bne         .Pack_label_132
/* @140 */ 	lda         __l0+1
/* @141 */ 	sta         __l1+3
/* @142 */ 	lda         __l0
/* @143 */ 	sta         __l1+2
/* @144 */ 	lda          #0
/* @145 */ 	sta         __l1
/* @146 */ 	sta         __l1+1
/* @147 */ 	ldx          #7
.Pack_label_148:
/* @149 */ 	asl         __l1
/* @150 */ 	rol         __l1+1
/* @151 */ 	rol         __l1+2
/* @152 */ 	rol         __l1+3
/* @153 */ 	dex         
/* @154 */ 	bne         .Pack_label_148
/* @160 */ 	ldy          #3
.Pack_label_161:
/* @162 */ 	lda         (__i1), Y
/* @163 */ 	sta         __l0, Y
/* @164 */ 	dey         
/* @165 */ 	bpl         .Pack_label_161
/* @170 */ 	lda         __l0
/* @171 */ 	ora         __l1
/* @172 */ 	sta         __l2
/* @173 */ 	lda         __l0+1
/* @174 */ 	ora         __l1+1
/* @175 */ 	sta         __l2+1
/* @176 */ 	lda         __l0+2
/* @177 */ 	ora         __l1+2
/* @178 */ 	sta         __l2+2
/* @179 */ 	lda         __l0+3
/* @180 */ 	ora         __l1+3
/* @181 */ 	sta         __l2+3
/* @187 */ 	ldy          #3
.Pack_label_188:
/* @189 */ 	lda         __l2, Y
/* @190 */ 	sta         (__i1), Y
/* @191 */ 	dey         
/* @192 */ 	bpl         .Pack_label_188
/* @198 */ 	lda         (__i0)
/* @199 */ 	sta         __b0
/* @203 */ 	lda         __b0
/* @204 */ 	sta         __l0
/* @205 */ 	lda          #0
/* @206 */ 	ldy          #3
.Pack_label_207:
/* @208 */ 	sta         __l0, Y
/* @209 */ 	dey         
/* @210 */ 	cpy          #0
/* @211 */ 	bne         .Pack_label_207
/* @215 */ 	lda         __l0
/* @216 */ 	sta         __l1+3
/* @217 */ 	lda          #0
/* @218 */ 	sta         __l1
/* @219 */ 	sta         __l1+1
/* @220 */ 	sta         __l1+2
/* @226 */ 	ldy          #3
.Pack_label_227:
/* @228 */ 	lda         (__i1), Y
/* @229 */ 	sta         __l0, Y
/* @230 */ 	dey         
/* @231 */ 	bpl         .Pack_label_227
/* @236 */ 	lda         __l0
/* @237 */ 	ora         __l1
/* @238 */ 	sta         __l2
/* @239 */ 	lda         __l0+1
/* @240 */ 	ora         __l1+1
/* @241 */ 	sta         __l2+1
/* @242 */ 	lda         __l0+2
/* @243 */ 	ora         __l1+2
/* @244 */ 	sta         __l2+2
/* @245 */ 	lda         __l0+3
/* @246 */ 	ora         __l1+3
/* @247 */ 	sta         __l2+3
/* @253 */ 	ldy          #3
.Pack_label_254:
/* @255 */ 	lda         __l2, Y
/* @256 */ 	sta         (__i1), Y
/* @257 */ 	dey         
/* @258 */ 	bpl         .Pack_label_254
/* @264 */ 	ldy          #3
.Pack_label_265:
/* @266 */ 	lda         (__i1), Y
/* @267 */ 	sta         __f0, Y
/* @268 */ 	dey         
/* @269 */ 	bpl         .Pack_label_265
/* @271 */ 	lda         #__f0
/* @273 */ 	jsr         __result4
.Pack_label_274:
/* @275 */ 	ldy          #12
	jsr          __leave_leaf
/* @276 */ 	rts         
.func_end_Pack:
	.size Pack, .func_end_Pack-Pack

	.global FPIsZero
	.type FPIsZero, @function

FPIsZero:
/* @5 */ 	stx         __result
/* @7 */ 	sty         __result+1
/* @8 */ 	ldx          #9
	jsr          __enter_leaf_nomask
/* @14 */ 	ldx          #0
	jsr          __arg_value4_f0			// f
/* @15 */ 	lda          #__f0
/* @17 */ 	ldx          #7
/* @19 */ 	jsr         __set_var_value4
/* @22 */ 	ldx          #7
	jsr          __var_addr_i0			// a
/* @26 */ 	ldy          #3
.FPIsZero_label_27:
/* @29 */ 	lda         (__i0), Y
/* @30 */ 	sta         __l0, Y
/* @31 */ 	dey         
/* @32 */ 	bpl         .FPIsZero_label_27
/* @37 */ 	lda         __l0
/* @38 */ 	sta         __l1
/* @40 */ 	lda         __l0+1
/* @41 */ 	sta         __l1+1
/* @43 */ 	lda         __l0+2
/* @44 */ 	sta         __l1+2
/* @45 */ 	lda         __l0+3
/* @46 */ 	and          #127
/* @47 */ 	sta         __l1+3
/* @53 */ 	ldx          #1
/* @54 */ 	lda         __l1
/* @55 */ 	ora         __l1+1
/* @56 */ 	ora         __l1+2
/* @57 */ 	ora         __l1+3
/* @58 */ 	cmp          #0
/* @59 */ 	beq         .FPIsZero_label_52
.FPIsZero_label_51:
/* @60 */ 	dex         
.FPIsZero_label_52:
/* @61 */ 	stx         __b0
/* @63 */ 	lda         #__b0
/* @65 */ 	jsr         __result1
.FPIsZero_label_66:
/* @67 */ 	ldy          #12
	jsr          __leave_leaf_nomask
/* @68 */ 	rts         
.func_end_FPIsZero:
	.size FPIsZero, .func_end_FPIsZero-FPIsZero

	.global FPIsNan
	.type FPIsNan, @function

FPIsNan:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #9
	jsr          __enter_leaf_nomask
/* @13 */ 	ldx          #0
	jsr          __arg_value4_f0			// f
/* @14 */ 	lda          #__f0
/* @16 */ 	ldx          #7
/* @18 */ 	jsr         __set_var_value4
/* @21 */ 	ldx          #7
	jsr          __var_addr_i0			// a
/* @25 */ 	ldy          #3
.FPIsNan_label_26:
/* @28 */ 	lda         (__i0), Y
/* @29 */ 	sta         __l0, Y
/* @30 */ 	dey         
/* @31 */ 	bpl         .FPIsNan_label_26
/* @36 */ 	lda         __l0
/* @37 */ 	sta         __l1
/* @39 */ 	lda         __l0+1
/* @40 */ 	sta         __l1+1
/* @42 */ 	lda         __l0+2
/* @43 */ 	sta         __l1+2
/* @44 */ 	lda         __l0+3
/* @45 */ 	and          #127
/* @46 */ 	sta         __l1+3
/* @52 */ 	ldx          #1
/* @53 */ 	lda         __l1
/* @54 */ 	cmp          #255
/* @55 */ 	bne         .FPIsNan_label_50
/* @56 */ 	lda         __l1+1
/* @57 */ 	cmp          #255
/* @58 */ 	bne         .FPIsNan_label_50
/* @59 */ 	lda         __l1+2
/* @60 */ 	cmp          #255
/* @61 */ 	bne         .FPIsNan_label_50
/* @62 */ 	lda         __l1+3
/* @63 */ 	cmp          #127
/* @64 */ 	beq         .FPIsNan_label_51
.FPIsNan_label_50:
/* @65 */ 	dex         
.FPIsNan_label_51:
/* @66 */ 	stx         __b0
/* @68 */ 	lda         #__b0
/* @70 */ 	jsr         __result1
.FPIsNan_label_71:
/* @72 */ 	ldy          #12
	jsr          __leave_leaf_nomask
/* @73 */ 	rts         
.func_end_FPIsNan:
	.size FPIsNan, .func_end_FPIsNan-FPIsNan

	.global FPIsInfinity
	.type FPIsInfinity, @function

FPIsInfinity:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #9
	jsr          __enter_leaf_nomask
/* @13 */ 	ldx          #0
	jsr          __arg_value4_f0			// f
/* @14 */ 	lda          #__f0
/* @16 */ 	ldx          #7
/* @18 */ 	jsr         __set_var_value4
/* @21 */ 	ldx          #7
	jsr          __var_addr_i0			// a
/* @25 */ 	ldy          #3
.FPIsInfinity_label_26:
/* @28 */ 	lda         (__i0), Y
/* @29 */ 	sta         __l0, Y
/* @30 */ 	dey         
/* @31 */ 	bpl         .FPIsInfinity_label_26
/* @36 */ 	lda          #0
/* @37 */ 	sta         __l1
/* @38 */ 	lda          #0
/* @40 */ 	sta         __l1+1
/* @42 */ 	lda         __l0+2
/* @43 */ 	and          #128
/* @44 */ 	sta         __l1+2
/* @45 */ 	lda         __l0+3
/* @46 */ 	and          #127
/* @47 */ 	sta         __l1+3
/* @53 */ 	ldx          #1
/* @54 */ 	lda         __l1
/* @55 */ 	cmp          #0
/* @56 */ 	bne         .FPIsInfinity_label_51
/* @57 */ 	lda         __l1+1
/* @58 */ 	cmp          #0
/* @59 */ 	bne         .FPIsInfinity_label_51
/* @60 */ 	lda         __l1+2
/* @61 */ 	cmp          #128
/* @62 */ 	bne         .FPIsInfinity_label_51
/* @63 */ 	lda         __l1+3
/* @64 */ 	cmp          #127
/* @65 */ 	beq         .FPIsInfinity_label_52
.FPIsInfinity_label_51:
/* @66 */ 	dex         
.FPIsInfinity_label_52:
/* @67 */ 	stx         __b0
/* @69 */ 	lda         #__b0
/* @71 */ 	jsr         __result1
.FPIsInfinity_label_72:
/* @73 */ 	ldy          #12
	jsr          __leave_leaf_nomask
/* @74 */ 	rts         
.func_end_FPIsInfinity:
	.size FPIsInfinity, .func_end_FPIsInfinity-FPIsInfinity

	.global FPZero
	.type FPZero, @function

FPZero:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #9
	jsr          __enter_leaf_nomask
/* @12 */ 	lda          #0
/* @13 */ 	sta         __l0
/* @14 */ 	lda          #0
/* @16 */ 	sta         __l0+1
/* @17 */ 	lda          #0
/* @19 */ 	sta         __l0+2
/* @20 */ 	lda          #0
/* @22 */ 	sta         __l0+3
/* @23 */ 	lda          #__l0
/* @25 */ 	ldx          #7
/* @27 */ 	jsr         __set_var_value4
/* @30 */ 	ldx          #7
	jsr          __var_addr_i0			// a
/* @33 */ 	ldy          #3
.FPZero_label_34:
/* @36 */ 	lda         (__i0), Y
/* @37 */ 	sta         __f0, Y
/* @38 */ 	dey         
/* @39 */ 	bpl         .FPZero_label_34
/* @41 */ 	lda         #__f0
/* @43 */ 	jsr         __result4
.FPZero_label_44:
/* @45 */ 	ldy          #12
	jsr          __leave_leaf_nomask
/* @46 */ 	rts         
.func_end_FPZero:
	.size FPZero, .func_end_FPZero-FPZero

	.global FPNaN
	.type FPNaN, @function

FPNaN:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #9
	jsr          __enter_leaf_nomask
/* @12 */ 	lda          #255
/* @14 */ 	sta         __l0
/* @15 */ 	lda          #255
/* @17 */ 	sta         __l0+1
/* @18 */ 	lda          #255
/* @20 */ 	sta         __l0+2
/* @22 */ 	lda          #127
/* @24 */ 	sta         __l0+3
/* @25 */ 	lda          #__l0
/* @27 */ 	ldx          #7
/* @29 */ 	jsr         __set_var_value4
/* @32 */ 	ldx          #7
	jsr          __var_addr_i0			// a
/* @35 */ 	ldy          #3
.FPNaN_label_36:
/* @38 */ 	lda         (__i0), Y
/* @39 */ 	sta         __f0, Y
/* @40 */ 	dey         
/* @41 */ 	bpl         .FPNaN_label_36
/* @43 */ 	lda         #__f0
/* @45 */ 	jsr         __result4
.FPNaN_label_46:
/* @47 */ 	ldy          #12
	jsr          __leave_leaf_nomask
/* @48 */ 	rts         
.func_end_FPNaN:
	.size FPNaN, .func_end_FPNaN-FPNaN

	.global FPInfinity
	.type FPInfinity, @function

FPInfinity:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #9
	jsr          __enter_leaf_nomask
/* @12 */ 	lda          #0
/* @13 */ 	sta         __l0
/* @14 */ 	lda          #0
/* @16 */ 	sta         __l0+1
/* @18 */ 	lda          #128
/* @20 */ 	sta         __l0+2
/* @22 */ 	lda          #127
/* @24 */ 	sta         __l0+3
/* @25 */ 	lda          #__l0
/* @27 */ 	ldx          #7
/* @29 */ 	jsr         __set_var_value4
/* @32 */ 	ldx          #7
	jsr          __var_addr_i0			// a
/* @35 */ 	ldy          #3
.FPInfinity_label_36:
/* @38 */ 	lda         (__i0), Y
/* @39 */ 	sta         __f0, Y
/* @40 */ 	dey         
/* @41 */ 	bpl         .FPInfinity_label_36
/* @43 */ 	lda         #__f0
/* @45 */ 	jsr         __result4
.FPInfinity_label_46:
/* @47 */ 	ldy          #12
	jsr          __leave_leaf_nomask
/* @48 */ 	rts         
.func_end_FPInfinity:
	.size FPInfinity, .func_end_FPInfinity-FPInfinity

	.global Normalize
	.type Normalize, @function

Normalize:
/* @8 */ 	ldx          #7
	jsr          __enter_nomask
/* @11 */ 	lda          #__i0			// struct return address
	ldx          #0
	jsr          __arg_value2
/* @14 */ 	ldx          #2
	jsr          __arg_addr_i1			// u
/* @18 */ 	ldy          #2
/* @19 */ 	lda         (__i1), Y
/* @21 */ 	sta         __l0
/* @23 */ 	ldy          #3
/* @24 */ 	lda         (__i1), Y
/* @25 */ 	sta         __l0+1
/* @27 */ 	ldy          #4
/* @28 */ 	lda         (__i1), Y
/* @29 */ 	sta         __l0+2
/* @31 */ 	ldy          #5
/* @32 */ 	lda         (__i1), Y
/* @33 */ 	sta         __l0+3
/* @35 */ 	lda         __l0
/* @36 */ 	ora         __l0+1
/* @37 */ 	ora         __l0+2
/* @38 */ 	ora         __l0+3
/* @39 */ 	bne         .Normalize_label_64
/* @43 */ 	lda          #6
/* @45 */ 	sta         __mem_size
/* @47 */ 	lda         __i1
/* @49 */ 	sta         __mem_src
/* @50 */ 	lda         __i1+1
/* @52 */ 	sta         __mem_src+1
/* @53 */ 	lda         __i0
/* @55 */ 	sta         __mem_dest
/* @56 */ 	lda         __i0+1
/* @58 */ 	sta         __mem_dest+1
/* @60 */ 	jsr         __copymem1
.Normalize_label_61:
/* @62 */ 	ldy          #10
	jsr          __leave_void_nomask
/* @63 */ 	rts         
.Normalize_label_64:
.Normalize_label_65:
/* @68 */ 	ldx          #2
	jsr          __arg_addr_i2			// u
/* @71 */ 	ldy          #2
/* @72 */ 	lda         (__i2), Y
/* @73 */ 	sta         __l0
/* @74 */ 	ldy          #3
/* @75 */ 	lda         (__i2), Y
/* @76 */ 	sta         __l0+1
/* @77 */ 	ldy          #4
/* @78 */ 	lda         (__i2), Y
/* @79 */ 	sta         __l0+2
/* @80 */ 	ldy          #5
/* @81 */ 	lda         (__i2), Y
/* @82 */ 	sta         __l0+3
/* @86 */ 	lda          #0
/* @87 */ 	sta         __l1
/* @88 */ 	lda          #0
/* @89 */ 	sta         __l1+1
/* @90 */ 	lda          #0
/* @91 */ 	sta         __l1+2
/* @92 */ 	lda         __l0+3
/* @93 */ 	and          #128
/* @94 */ 	sta         __l1+3
/* @96 */ 	lda         __l1
/* @97 */ 	ora         __l1+1
/* @98 */ 	ora         __l1+2
/* @99 */ 	ora         __l1+3
/* @100 */ 	bne         .Normalize_label_167
/* @106 */ 	ldy          #2
/* @107 */ 	lda         (__i2), Y
/* @108 */ 	sta         __l0
/* @109 */ 	ldy          #3
/* @110 */ 	lda         (__i2), Y
/* @111 */ 	sta         __l0+1
/* @112 */ 	ldy          #4
/* @113 */ 	lda         (__i2), Y
/* @114 */ 	sta         __l0+2
/* @115 */ 	ldy          #5
/* @116 */ 	lda         (__i2), Y
/* @117 */ 	sta         __l0+3
/* @121 */ 	lda         __l0
/* @122 */ 	asl          A
/* @123 */ 	sta         __l1
/* @124 */ 	lda         __l0+1
/* @125 */ 	rol          A
/* @126 */ 	sta         __l1+1
/* @127 */ 	lda         __l0+2
/* @128 */ 	rol          A
/* @129 */ 	sta         __l1+2
/* @130 */ 	lda         __l0+3
/* @131 */ 	rol          A
/* @132 */ 	sta         __l1+3
/* @138 */ 	lda         __l1
/* @139 */ 	ldy          #2
/* @140 */ 	sta         (__i2), Y
/* @141 */ 	lda         __l1+1
/* @142 */ 	ldy          #3
/* @143 */ 	sta         (__i2), Y
/* @144 */ 	lda         __l1+2
/* @145 */ 	ldy          #4
/* @146 */ 	sta         (__i2), Y
/* @147 */ 	lda         __l1+3
/* @148 */ 	ldy          #5
/* @149 */ 	sta         (__i2), Y
/* @155 */ 	clc         
/* @156 */ 	lda         __i2
/* @157 */ 	adc          #1
/* @158 */ 	sta         __i3
/* @159 */ 	lda         __i2+1
/* @160 */ 	adc          #0
/* @161 */ 	sta         __i3+1
/* @163 */ 	lda          #__i3
/* @165 */ 	jsr         __dec1
/* @166 */ 	jmp         .Normalize_label_65
.Normalize_label_167:
/* @170 */ 	lda          #6
/* @171 */ 	sta         __mem_size
/* @173 */ 	lda         __i2
/* @174 */ 	sta         __mem_src
/* @175 */ 	lda         __i2+1
/* @176 */ 	sta         __mem_src+1
/* @177 */ 	lda         __i0
/* @178 */ 	sta         __mem_dest
/* @179 */ 	lda         __i0+1
/* @180 */ 	sta         __mem_dest+1
/* @181 */ 	jsr         __copymem1
/* @182 */ 	jmp         .Normalize_label_61
.func_end_Normalize:
	.size Normalize, .func_end_Normalize-Normalize

	.global Round
	.type Round, @function

Round:
/* @11 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x02,0x00		// Save mask i:0 b:0 l:1 x:0 f:0 
/* @19 */ 	lda          #__i1			// struct return address
	ldx          #0
	jsr          __arg_value2
/* @22 */ 	ldx          #2
	jsr          __arg_addr_i2			// v
/* @26 */ 	ldy          #2
/* @27 */ 	lda         (__i2), Y
/* @29 */ 	sta         __l1
/* @31 */ 	ldy          #3
/* @32 */ 	lda         (__i2), Y
/* @34 */ 	sta         __l1+1
/* @36 */ 	ldy          #4
/* @37 */ 	lda         (__i2), Y
/* @38 */ 	sta         __l1+2
/* @40 */ 	ldy          #5
/* @41 */ 	lda         (__i2), Y
/* @42 */ 	sta         __l1+3
/* @46 */ 	lda         __l1
/* @47 */ 	sta         __l2
/* @48 */ 	lda          #0
/* @49 */ 	sta         __l2+1
/* @50 */ 	lda          #0
/* @51 */ 	sta         __l2+2
/* @52 */ 	lda          #0
/* @53 */ 	sta         __l2+3
/* @56 */ 	lda         __l2
/* @57 */ 	sta         __i0
/* @58 */ 	lda         __l2+1
/* @59 */ 	sta         __i0+1
/* @62 */ 	lda         __i0
/* @63 */ 	and          #128
/* @64 */ 	sta         __i3
/* @65 */ 	lda          #0
/* @66 */ 	sta         __i3+1
/* @68 */ 	lda         __i3
/* @69 */ 	ora         __i3+1
/* @70 */ 	cmp          #0
/* @71 */ 	bne         .Round_label_209
/* @210 */ 	jmp         .Round_label_184
.Round_label_209:
/* @77 */ 	ldy          #2
/* @78 */ 	lda         (__i2), Y
/* @79 */ 	sta         __l1
/* @80 */ 	ldy          #3
/* @81 */ 	lda         (__i2), Y
/* @82 */ 	sta         __l1+1
/* @83 */ 	ldy          #4
/* @84 */ 	lda         (__i2), Y
/* @85 */ 	sta         __l1+2
/* @86 */ 	ldy          #5
/* @87 */ 	lda         (__i2), Y
/* @88 */ 	sta         __l1+3
/* @92 */ 	lda         __l1+1
/* @93 */ 	sta         __l2
/* @94 */ 	lda         __l1+2
/* @95 */ 	sta         __l2+1
/* @96 */ 	lda         __l1+3
/* @97 */ 	sta         __l2+2
/* @98 */ 	lda          #0
/* @99 */ 	sta         __l2+3
/* @103 */ 	clc         
/* @104 */ 	lda         __l2
/* @105 */ 	adc          #1
/* @106 */ 	sta         __l1
/* @107 */ 	lda         __l2+1
/* @108 */ 	adc          #0
/* @109 */ 	sta         __l1+1
/* @110 */ 	lda         __l2+2
/* @111 */ 	adc          #0
/* @112 */ 	sta         __l1+2
/* @113 */ 	lda         __l2+3
/* @114 */ 	adc          #0
/* @115 */ 	sta         __l1+3
/* @117 */ 	lda         __l1+2
/* @118 */ 	sta         __l0+3
/* @119 */ 	lda         __l1+1
/* @120 */ 	sta         __l0+2
/* @121 */ 	lda         __l1
/* @122 */ 	sta         __l0+1
/* @123 */ 	lda          #0
/* @124 */ 	sta         __l0
/* @126 */ 	lda         __i0
/* @127 */ 	cmp          #128
/* @128 */ 	bne         .Round_label_167
/* @129 */ 	lda         __i0+1
/* @130 */ 	cmp          #0
/* @131 */ 	bne         .Round_label_167
.Round_label_125:
/* @135 */ 	lda          #0
/* @136 */ 	sta         __l1
/* @137 */ 	lda         __l0+1
/* @138 */ 	and          #1
/* @139 */ 	sta         __l1+1
/* @140 */ 	lda          #0
/* @141 */ 	sta         __l1+2
/* @142 */ 	lda          #0
/* @143 */ 	sta         __l1+3
/* @145 */ 	lda         __l1
/* @146 */ 	ora         __l1+1
/* @147 */ 	ora         __l1+2
/* @148 */ 	ora         __l1+3
/* @149 */ 	bne         .Round_label_165
/* @153 */ 	lda         __l0
/* @154 */ 	ldy          #2
/* @155 */ 	sta         (__i2), Y
/* @156 */ 	lda         __l0+1
/* @157 */ 	ldy          #3
/* @158 */ 	sta         (__i2), Y
/* @159 */ 	lda         __l0+2
/* @160 */ 	ldy          #4
/* @161 */ 	sta         (__i2), Y
/* @162 */ 	lda         __l0+3
/* @163 */ 	ldy          #5
/* @164 */ 	sta         (__i2), Y
.Round_label_165:
/* @166 */ 	bra         .Round_label_183
.Round_label_167:
/* @171 */ 	lda         __l0
/* @172 */ 	ldy          #2
/* @173 */ 	sta         (__i2), Y
/* @174 */ 	lda         __l0+1
/* @175 */ 	ldy          #3
/* @176 */ 	sta         (__i2), Y
/* @177 */ 	lda         __l0+2
/* @178 */ 	ldy          #4
/* @179 */ 	sta         (__i2), Y
/* @180 */ 	lda         __l0+3
/* @181 */ 	ldy          #5
/* @182 */ 	sta         (__i2), Y
.Round_label_183:
.Round_label_184:
/* @188 */ 	lda          #6
/* @190 */ 	sta         __mem_size
/* @192 */ 	lda         __i2
/* @194 */ 	sta         __mem_src
/* @195 */ 	lda         __i2+1
/* @197 */ 	sta         __mem_src+1
/* @198 */ 	lda         __i1
/* @200 */ 	sta         __mem_dest
/* @201 */ 	lda         __i1+1
/* @203 */ 	sta         __mem_dest+1
/* @205 */ 	jsr         __copymem1
.Round_label_206:
/* @207 */ 	ldy          #10
	jsr          __leave_void
/* @208 */ 	rts         
.func_end_Round:
	.size Round, .func_end_Round-Round

	.global Divide64
	.type Divide64, @function

Divide64:
/* @9 */ 	ldx          #23
	jsr          __enter
	.byte        0x00,0x60,0x00		// Save mask i:0 b:0 l:0 x:3 f:0 
/* @17 */ 	ldx          #2
	jsr          __arg_value8_x0			// a
/* @20 */ 	lda          #__i1			// struct return address
	ldx          #0
	jsr          __arg_value2
/* @22 */ 	lda          #16
/* @24 */ 	sta         __mem_size
/* @26 */ 	ldx          #19
	jsr          __var_addr_i2			// qr
/* @29 */ 	lda         __i2
/* @31 */ 	sta         __mem_dest
/* @33 */ 	lda         __i2+1
/* @35 */ 	sta         __mem_dest+1
/* @37 */ 	jsr         __zeromem1
/* @43 */ 	clc         
/* @44 */ 	lda         __i2
/* @45 */ 	adc          #8
/* @46 */ 	sta         __i3
/* @47 */ 	lda         __i2+1
/* @48 */ 	adc          #0
/* @49 */ 	sta         __i3+1
/* @50 */ 	lda          #0
/* @51 */ 	sta         __i0
/* @52 */ 	lda          #0
/* @53 */ 	sta         __i0+1
.Divide64_label_54:
/* @56 */ 	lda         __i0
/* @57 */ 	cmp          #64
/* @58 */ 	lda         __i0+1
/* @59 */ 	sbc          #0
/* @60 */ 	bvc         .Divide64_label_55
/* @62 */ 	eor          #128
.Divide64_label_55:
/* @63 */ 	bmi         .Divide64_label_528
/* @529 */ 	jmp         .Divide64_label_507
.Divide64_label_528:
/* @66 */ 	ldx          #19
	jsr          __var_addr_i2			// qr
/* @70 */ 	ldy          #8
/* @71 */ 	lda         (__i2), Y
/* @72 */ 	sta         __x1
/* @74 */ 	ldy          #9
/* @75 */ 	lda         (__i2), Y
/* @76 */ 	sta         __x1+1
/* @78 */ 	ldy          #10
/* @79 */ 	lda         (__i2), Y
/* @81 */ 	sta         __x1+2
/* @83 */ 	ldy          #11
/* @84 */ 	lda         (__i2), Y
/* @86 */ 	sta         __x1+3
/* @88 */ 	ldy          #12
/* @89 */ 	lda         (__i2), Y
/* @91 */ 	sta         __x1+4
/* @93 */ 	ldy          #13
/* @94 */ 	lda         (__i2), Y
/* @96 */ 	sta         __x1+5
/* @98 */ 	ldy          #14
/* @99 */ 	lda         (__i2), Y
/* @101 */ 	sta         __x1+6
/* @103 */ 	ldy          #15
/* @104 */ 	lda         (__i2), Y
/* @106 */ 	sta         __x1+7
/* @110 */ 	lda         __x1
/* @111 */ 	asl          A
/* @112 */ 	sta         __x2
/* @113 */ 	lda         __x1+1
/* @114 */ 	rol          A
/* @115 */ 	sta         __x2+1
/* @116 */ 	lda         __x1+2
/* @117 */ 	rol          A
/* @118 */ 	sta         __x2+2
/* @119 */ 	lda         __x1+3
/* @120 */ 	rol          A
/* @121 */ 	sta         __x2+3
/* @122 */ 	lda         __x1+4
/* @123 */ 	rol          A
/* @124 */ 	sta         __x2+4
/* @125 */ 	lda         __x1+5
/* @126 */ 	rol          A
/* @127 */ 	sta         __x2+5
/* @128 */ 	lda         __x1+6
/* @129 */ 	rol          A
/* @130 */ 	sta         __x2+6
/* @131 */ 	lda         __x1+7
/* @132 */ 	rol          A
/* @133 */ 	sta         __x2+7
/* @139 */ 	lda         __x2
/* @140 */ 	ldy          #8
/* @141 */ 	sta         (__i2), Y
/* @142 */ 	lda         __x2+1
/* @143 */ 	ldy          #9
/* @144 */ 	sta         (__i2), Y
/* @145 */ 	lda         __x2+2
/* @146 */ 	ldy          #10
/* @147 */ 	sta         (__i2), Y
/* @148 */ 	lda         __x2+3
/* @149 */ 	ldy          #11
/* @150 */ 	sta         (__i2), Y
/* @151 */ 	lda         __x2+4
/* @152 */ 	ldy          #12
/* @153 */ 	sta         (__i2), Y
/* @154 */ 	lda         __x2+5
/* @155 */ 	ldy          #13
/* @156 */ 	sta         (__i2), Y
/* @157 */ 	lda         __x2+6
/* @158 */ 	ldy          #14
/* @159 */ 	sta         (__i2), Y
/* @160 */ 	lda         __x2+7
/* @161 */ 	ldy          #15
/* @162 */ 	sta         (__i2), Y
/* @165 */ 	lda          #0
/* @166 */ 	sta         __x1
/* @167 */ 	lda          #0
/* @168 */ 	sta         __x1+1
/* @169 */ 	lda          #0
/* @170 */ 	sta         __x1+2
/* @171 */ 	lda          #0
/* @172 */ 	sta         __x1+3
/* @173 */ 	lda          #0
/* @174 */ 	sta         __x1+4
/* @175 */ 	lda          #0
/* @176 */ 	sta         __x1+5
/* @177 */ 	lda          #0
/* @178 */ 	sta         __x1+6
/* @179 */ 	lda         __x0+7
/* @180 */ 	and          #128
/* @181 */ 	sta         __x1+7
/* @183 */ 	lda         __x1
/* @184 */ 	ora         __x1+1
/* @185 */ 	ora         __x1+2
/* @186 */ 	ora         __x1+3
/* @187 */ 	ora         __x1+4
/* @188 */ 	ora         __x1+5
/* @189 */ 	ora         __x1+6
/* @190 */ 	ora         __x1+7
/* @191 */ 	cmp          #0
/* @192 */ 	bne         .Divide64_label_530
/* @531 */ 	jmp         .Divide64_label_271
.Divide64_label_530:
/* @198 */ 	ldy          #8
/* @199 */ 	lda         (__i2), Y
/* @200 */ 	sta         __x1
/* @201 */ 	ldy          #9
/* @202 */ 	lda         (__i2), Y
/* @203 */ 	sta         __x1+1
/* @204 */ 	ldy          #10
/* @205 */ 	lda         (__i2), Y
/* @206 */ 	sta         __x1+2
/* @207 */ 	ldy          #11
/* @208 */ 	lda         (__i2), Y
/* @209 */ 	sta         __x1+3
/* @210 */ 	ldy          #12
/* @211 */ 	lda         (__i2), Y
/* @212 */ 	sta         __x1+4
/* @213 */ 	ldy          #13
/* @214 */ 	lda         (__i2), Y
/* @215 */ 	sta         __x1+5
/* @216 */ 	ldy          #14
/* @217 */ 	lda         (__i2), Y
/* @218 */ 	sta         __x1+6
/* @219 */ 	ldy          #15
/* @220 */ 	lda         (__i2), Y
/* @221 */ 	sta         __x1+7
/* @225 */ 	lda         __x1
/* @226 */ 	ora          #1
/* @227 */ 	sta         __x2
/* @228 */ 	lda         __x1+1
/* @229 */ 	sta         __x2+1
/* @230 */ 	lda         __x1+2
/* @231 */ 	sta         __x2+2
/* @232 */ 	lda         __x1+3
/* @233 */ 	sta         __x2+3
/* @234 */ 	lda         __x1+4
/* @235 */ 	sta         __x2+4
/* @236 */ 	lda         __x1+5
/* @237 */ 	sta         __x2+5
/* @238 */ 	lda         __x1+6
/* @239 */ 	sta         __x2+6
/* @240 */ 	lda         __x1+7
/* @241 */ 	sta         __x2+7
/* @247 */ 	lda         __x2
/* @248 */ 	ldy          #8
/* @249 */ 	sta         (__i2), Y
/* @250 */ 	lda         __x2+1
/* @251 */ 	ldy          #9
/* @252 */ 	sta         (__i2), Y
/* @253 */ 	lda         __x2+2
/* @254 */ 	ldy          #10
/* @255 */ 	sta         (__i2), Y
/* @256 */ 	lda         __x2+3
/* @257 */ 	ldy          #11
/* @258 */ 	sta         (__i2), Y
/* @259 */ 	lda         __x2+4
/* @260 */ 	ldy          #12
/* @261 */ 	sta         (__i2), Y
/* @262 */ 	lda         __x2+5
/* @263 */ 	ldy          #13
/* @264 */ 	sta         (__i2), Y
/* @265 */ 	lda         __x2+6
/* @266 */ 	ldy          #14
/* @267 */ 	sta         (__i2), Y
/* @268 */ 	lda         __x2+7
/* @269 */ 	ldy          #15
/* @270 */ 	sta         (__i2), Y
.Divide64_label_271:
/* @272 */ 	asl         __x0
/* @273 */ 	rol         __x0+1
/* @274 */ 	rol         __x0+2
/* @275 */ 	rol         __x0+3
/* @276 */ 	rol         __x0+4
/* @277 */ 	rol         __x0+5
/* @278 */ 	rol         __x0+6
/* @279 */ 	rol         __x0+7
/* @285 */ 	ldy          #7
.Divide64_label_286:
/* @288 */ 	lda         (__i2), Y
/* @289 */ 	sta         __x1, Y
/* @290 */ 	dey         
/* @291 */ 	bpl         .Divide64_label_286
/* @295 */ 	lda         __x1
/* @296 */ 	asl          A
/* @297 */ 	sta         __x2
/* @298 */ 	lda         __x1+1
/* @299 */ 	rol          A
/* @300 */ 	sta         __x2+1
/* @301 */ 	lda         __x1+2
/* @302 */ 	rol          A
/* @303 */ 	sta         __x2+2
/* @304 */ 	lda         __x1+3
/* @305 */ 	rol          A
/* @306 */ 	sta         __x2+3
/* @307 */ 	lda         __x1+4
/* @308 */ 	rol          A
/* @309 */ 	sta         __x2+4
/* @310 */ 	lda         __x1+5
/* @311 */ 	rol          A
/* @312 */ 	sta         __x2+5
/* @313 */ 	lda         __x1+6
/* @314 */ 	rol          A
/* @315 */ 	sta         __x2+6
/* @316 */ 	lda         __x1+7
/* @317 */ 	rol          A
/* @318 */ 	sta         __x2+7
/* @324 */ 	ldy          #7
.Divide64_label_325:
/* @326 */ 	lda         __x2, Y
/* @327 */ 	sta         (__i2), Y
/* @328 */ 	dey         
/* @329 */ 	bpl         .Divide64_label_325
/* @335 */ 	ldy          #8
/* @336 */ 	lda         (__i2), Y
/* @337 */ 	sta         __x1
/* @338 */ 	ldy          #9
/* @339 */ 	lda         (__i2), Y
/* @340 */ 	sta         __x1+1
/* @341 */ 	ldy          #10
/* @342 */ 	lda         (__i2), Y
/* @343 */ 	sta         __x1+2
/* @344 */ 	ldy          #11
/* @345 */ 	lda         (__i2), Y
/* @346 */ 	sta         __x1+3
/* @347 */ 	ldy          #12
/* @348 */ 	lda         (__i2), Y
/* @349 */ 	sta         __x1+4
/* @350 */ 	ldy          #13
/* @351 */ 	lda         (__i2), Y
/* @352 */ 	sta         __x1+5
/* @353 */ 	ldy          #14
/* @354 */ 	lda         (__i2), Y
/* @355 */ 	sta         __x1+6
/* @356 */ 	ldy          #15
/* @357 */ 	lda         (__i2), Y
/* @358 */ 	sta         __x1+7
/* @360 */ 	ldx          #10
	jsr          __arg_value8_x2			// b
/* @364 */ 	lda         __x1+7
/* @365 */ 	cmp         __x2+7
/* @366 */ 	bcs         .Divide64_label_532
/* @533 */ 	jmp         .Divide64_label_501
.Divide64_label_532:
/* @367 */ 	bne         .Divide64_label_363
/* @368 */ 	lda         __x1+6
/* @369 */ 	cmp         __x2+6
/* @370 */ 	bcs         .Divide64_label_534
/* @535 */ 	jmp         .Divide64_label_501
.Divide64_label_534:
/* @371 */ 	bne         .Divide64_label_363
/* @372 */ 	lda         __x1+5
/* @373 */ 	cmp         __x2+5
/* @374 */ 	bcs         .Divide64_label_536
/* @537 */ 	jmp         .Divide64_label_501
.Divide64_label_536:
/* @375 */ 	bne         .Divide64_label_363
/* @376 */ 	lda         __x1+4
/* @377 */ 	cmp         __x2+4
/* @378 */ 	bcs         .Divide64_label_538
/* @539 */ 	jmp         .Divide64_label_501
.Divide64_label_538:
/* @379 */ 	bne         .Divide64_label_363
/* @380 */ 	lda         __x1+3
/* @381 */ 	cmp         __x2+3
/* @382 */ 	bcs         .Divide64_label_540
/* @541 */ 	jmp         .Divide64_label_501
.Divide64_label_540:
/* @383 */ 	bne         .Divide64_label_363
/* @384 */ 	lda         __x1+2
/* @385 */ 	cmp         __x2+2
/* @386 */ 	bcs         .Divide64_label_542
/* @543 */ 	jmp         .Divide64_label_501
.Divide64_label_542:
/* @387 */ 	bne         .Divide64_label_363
/* @388 */ 	lda         __x1+1
/* @389 */ 	cmp         __x2+1
/* @390 */ 	bcs         .Divide64_label_544
/* @545 */ 	jmp         .Divide64_label_501
.Divide64_label_544:
/* @391 */ 	bne         .Divide64_label_363
/* @392 */ 	lda         __x1
/* @393 */ 	cmp         __x2
/* @394 */ 	bcs         .Divide64_label_546
/* @547 */ 	jmp         .Divide64_label_501
.Divide64_label_546:
.Divide64_label_363:
/* @401 */ 	clc         
/* @402 */ 	lda         __i2
/* @403 */ 	adc          #0
/* @404 */ 	sta         __i3
/* @405 */ 	lda         __i2+1
/* @406 */ 	adc          #0
/* @407 */ 	sta         __i3+1
/* @409 */ 	lda          #__i3
/* @411 */ 	jsr         __inc8
/* @413 */ 	ldx          #10
	jsr          __arg_value8_x1			// b
/* @419 */ 	ldy          #8
/* @420 */ 	lda         (__i2), Y
/* @421 */ 	sta         __x2
/* @422 */ 	ldy          #9
/* @423 */ 	lda         (__i2), Y
/* @424 */ 	sta         __x2+1
/* @425 */ 	ldy          #10
/* @426 */ 	lda         (__i2), Y
/* @427 */ 	sta         __x2+2
/* @428 */ 	ldy          #11
/* @429 */ 	lda         (__i2), Y
/* @430 */ 	sta         __x2+3
/* @431 */ 	ldy          #12
/* @432 */ 	lda         (__i2), Y
/* @433 */ 	sta         __x2+4
/* @434 */ 	ldy          #13
/* @435 */ 	lda         (__i2), Y
/* @436 */ 	sta         __x2+5
/* @437 */ 	ldy          #14
/* @438 */ 	lda         (__i2), Y
/* @439 */ 	sta         __x2+6
/* @440 */ 	ldy          #15
/* @441 */ 	lda         (__i2), Y
/* @442 */ 	sta         __x2+7
/* @447 */ 	sec         
/* @448 */ 	lda         __x2
/* @449 */ 	sbc         __x1
/* @450 */ 	sta         __x3
/* @451 */ 	lda         __x2+1
/* @452 */ 	sbc         __x1+1
/* @453 */ 	sta         __x3+1
/* @454 */ 	lda         __x2+2
/* @455 */ 	sbc         __x1+2
/* @456 */ 	sta         __x3+2
/* @457 */ 	lda         __x2+3
/* @458 */ 	sbc         __x1+3
/* @459 */ 	sta         __x3+3
/* @460 */ 	lda         __x2+4
/* @461 */ 	sbc         __x1+4
/* @462 */ 	sta         __x3+4
/* @463 */ 	lda         __x2+5
/* @464 */ 	sbc         __x1+5
/* @465 */ 	sta         __x3+5
/* @466 */ 	lda         __x2+6
/* @467 */ 	sbc         __x1+6
/* @468 */ 	sta         __x3+6
/* @469 */ 	lda         __x2+7
/* @470 */ 	sbc         __x1+7
/* @471 */ 	sta         __x3+7
/* @477 */ 	lda         __x3
/* @478 */ 	ldy          #8
/* @479 */ 	sta         (__i2), Y
/* @480 */ 	lda         __x3+1
/* @481 */ 	ldy          #9
/* @482 */ 	sta         (__i2), Y
/* @483 */ 	lda         __x3+2
/* @484 */ 	ldy          #10
/* @485 */ 	sta         (__i2), Y
/* @486 */ 	lda         __x3+3
/* @487 */ 	ldy          #11
/* @488 */ 	sta         (__i2), Y
/* @489 */ 	lda         __x3+4
/* @490 */ 	ldy          #12
/* @491 */ 	sta         (__i2), Y
/* @492 */ 	lda         __x3+5
/* @493 */ 	ldy          #13
/* @494 */ 	sta         (__i2), Y
/* @495 */ 	lda         __x3+6
/* @496 */ 	ldy          #14
/* @497 */ 	sta         (__i2), Y
/* @498 */ 	lda         __x3+7
/* @499 */ 	ldy          #15
/* @500 */ 	sta         (__i2), Y
.Divide64_label_501:
.Divide64_label_502:
/* @503 */ 	lda          #__i0
/* @505 */ 	jsr         __rinc21
/* @506 */ 	jmp         .Divide64_label_54
.Divide64_label_507:
/* @509 */ 	ldx          #19
	jsr          __var_addr_i2			// qr
/* @510 */ 	lda          #16
/* @511 */ 	sta         __mem_size
/* @513 */ 	lda         __i2
/* @515 */ 	sta         __mem_src
/* @516 */ 	lda         __i2+1
/* @518 */ 	sta         __mem_src+1
/* @519 */ 	lda         __i1
/* @520 */ 	sta         __mem_dest
/* @521 */ 	lda         __i1+1
/* @522 */ 	sta         __mem_dest+1
/* @524 */ 	jsr         __copymem1
.Divide64_label_525:
/* @526 */ 	ldy          #26
	jsr          __leave_void
/* @527 */ 	rts         
.func_end_Divide64:
	.size Divide64, .func_end_Divide64-Divide64

	.global Multiply32
	.type Multiply32, @function

Multiply32:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #13
	jsr          __enter_leaf
	.byte        0x00,0x20,0x00		// Save mask i:0 b:0 l:0 x:1 f:0 
/* @19 */ 	ldx          #0
	jsr          __arg_value4_l0			// a
/* @23 */ 	ldx          #4
	jsr          __arg_value4_l1			// b
/* @25 */ 	lda          #0
/* @26 */ 	sta         __x0
/* @28 */ 	lda          #0
/* @29 */ 	sta         __x0+1
/* @31 */ 	lda          #0
/* @32 */ 	sta         __x0+2
/* @34 */ 	lda          #0
/* @35 */ 	sta         __x0+3
/* @37 */ 	lda          #0
/* @38 */ 	sta         __x0+4
/* @40 */ 	lda          #0
/* @41 */ 	sta         __x0+5
/* @43 */ 	lda          #0
/* @44 */ 	sta         __x0+6
/* @46 */ 	lda          #0
/* @47 */ 	sta         __x0+7
/* @49 */ 	ldx          #11
	jsr          __var_addr_i0			// x
/* @50 */ 	ldy          #0
/* @51 */ 	lda         __l0
/* @52 */ 	sta         (__i0)
/* @53 */ 	ldy          #1
/* @54 */ 	lda         __l0+1
/* @55 */ 	sta         (__i0), Y
/* @56 */ 	ldy          #2
/* @57 */ 	lda         __l0+2
/* @58 */ 	sta         (__i0), Y
/* @59 */ 	ldy          #3
/* @60 */ 	lda         __l0+3
/* @61 */ 	sta         (__i0), Y
/* @62 */ 	lda          #0
/* @63 */ 	ldy          #7
.Multiply32_label_64:
/* @66 */ 	sta         (__i0), Y
/* @67 */ 	dey         
/* @68 */ 	cpy          #3
/* @69 */ 	bne         .Multiply32_label_64
.Multiply32_label_70:
/* @71 */ 	lda         __l1
/* @72 */ 	ora         __l1+1
/* @73 */ 	ora         __l1+2
/* @74 */ 	ora         __l1+3
/* @75 */ 	cmp          #0
/* @76 */ 	bne         .Multiply32_label_182
/* @183 */ 	jmp         .Multiply32_label_175
.Multiply32_label_182:
/* @79 */ 	lda         __l1
/* @80 */ 	and          #1
/* @81 */ 	sta         __l0
/* @82 */ 	lda          #0
/* @83 */ 	sta         __l0+1
/* @84 */ 	lda          #0
/* @85 */ 	sta         __l0+2
/* @86 */ 	lda          #0
/* @87 */ 	sta         __l0+3
/* @90 */ 	lda         __l0
/* @91 */ 	cmp          #1
/* @92 */ 	bne         .Multiply32_label_131
/* @93 */ 	lda         __l0+1
/* @94 */ 	cmp          #0
/* @95 */ 	bne         .Multiply32_label_131
/* @96 */ 	lda         __l0+2
/* @97 */ 	cmp          #0
/* @98 */ 	bne         .Multiply32_label_131
/* @99 */ 	lda         __l0+3
/* @100 */ 	cmp          #0
/* @101 */ 	bne         .Multiply32_label_131
.Multiply32_label_89:
/* @104 */ 	ldx          #11
	jsr          __var_value8_x1			// x
/* @106 */ 	clc         
/* @107 */ 	lda         __x0
/* @108 */ 	adc         __x1
/* @109 */ 	sta         __x0
/* @110 */ 	lda         __x0+1
/* @111 */ 	adc         __x1+1
/* @112 */ 	sta         __x0+1
/* @113 */ 	lda         __x0+2
/* @114 */ 	adc         __x1+2
/* @115 */ 	sta         __x0+2
/* @116 */ 	lda         __x0+3
/* @117 */ 	adc         __x1+3
/* @118 */ 	sta         __x0+3
/* @119 */ 	lda         __x0+4
/* @120 */ 	adc         __x1+4
/* @121 */ 	sta         __x0+4
/* @122 */ 	lda         __x0+5
/* @123 */ 	adc         __x1+5
/* @124 */ 	sta         __x0+5
/* @125 */ 	lda         __x0+6
/* @126 */ 	adc         __x1+6
/* @127 */ 	sta         __x0+6
/* @128 */ 	lda         __x0+7
/* @129 */ 	adc         __x1+7
/* @130 */ 	sta         __x0+7
.Multiply32_label_131:
/* @133 */ 	ldx          #11
	jsr          __var_value8_x1			// x
/* @135 */ 	ldx          #11
	jsr          __var_addr_i0			// x
/* @138 */ 	ldy          #0
/* @139 */ 	lda         __x1
/* @140 */ 	asl          A
/* @141 */ 	sta         (__i0)
/* @142 */ 	ldy          #1
/* @143 */ 	lda         __x1+1
/* @144 */ 	rol          A
/* @145 */ 	sta         (__i0), Y
/* @146 */ 	ldy          #2
/* @147 */ 	lda         __x1+2
/* @148 */ 	rol          A
/* @149 */ 	sta         (__i0), Y
/* @150 */ 	ldy          #3
/* @151 */ 	lda         __x1+3
/* @152 */ 	rol          A
/* @153 */ 	sta         (__i0), Y
/* @154 */ 	ldy          #4
/* @155 */ 	lda         __x1+4
/* @156 */ 	rol          A
/* @157 */ 	sta         (__i0), Y
/* @158 */ 	ldy          #5
/* @159 */ 	lda         __x1+5
/* @160 */ 	rol          A
/* @161 */ 	sta         (__i0), Y
/* @162 */ 	ldy          #6
/* @163 */ 	lda         __x1+6
/* @164 */ 	rol          A
/* @165 */ 	sta         (__i0), Y
/* @166 */ 	ldy          #7
/* @167 */ 	lda         __x1+7
/* @168 */ 	rol          A
/* @169 */ 	sta         (__i0), Y
/* @170 */ 	lsr         __l1+3
/* @171 */ 	ror         __l1+2
/* @172 */ 	ror         __l1+1
/* @173 */ 	ror         __l1
/* @174 */ 	jmp         .Multiply32_label_70
.Multiply32_label_175:
/* @176 */ 	lda         #__x0
/* @178 */ 	jsr         __result8
.Multiply32_label_179:
/* @180 */ 	ldy          #16
	jsr          __leave_leaf
/* @181 */ 	rts         
.func_end_Multiply32:
	.size Multiply32, .func_end_Multiply32-Multiply32

	.global FPMultiply
	.type FPMultiply, @function

FPMultiply:
/* @11 */ 	stx         __result
/* @13 */ 	sty         __result+1
/* @14 */ 	ldx          #37
	jsr          __enter
	.byte        0x83,0x40,0x01		// Save mask i:3 b:4 l:0 x:2 f:1 
/* @30 */ 	ldx          #4
	jsr          __arg_value4_f1			// b
/* @50 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @52 */ 	jsr         __pushf0
/* @54 */ 	ldx         #__b0
/* @55 */ 	ldy          #0
/* @56 */ 	jsr         FPIsZero
/* @58 */ 	jsr         __incsp4
/* @60 */ 	lda         __b0
/* @61 */ 	bne         .FPMultiply_label_67
/* @62 */ 	jsr         __pushf1
/* @63 */ 	ldx         #__b0
/* @64 */ 	ldy          #0
/* @65 */ 	jsr         FPIsZero
/* @66 */ 	jsr         __incsp4
.FPMultiply_label_67:
/* @69 */ 	lda         __b0
/* @70 */ 	cmp          #0
/* @71 */ 	beq         .FPMultiply_label_91
/* @73 */ 	lda          #0
/* @74 */ 	sta         __f0
/* @75 */ 	lda          #0
/* @77 */ 	sta         __f0+1
/* @78 */ 	lda          #0
/* @80 */ 	sta         __f0+2
/* @81 */ 	lda          #0
/* @83 */ 	sta         __f0+3
/* @84 */ 	ldx          #38
	jsr          __load_result
/* @85 */ 	lda         #__f0
/* @87 */ 	jsr         __result4
.FPMultiply_label_88:
/* @89 */ 	ldy          #40
	jsr          __leave
/* @90 */ 	rts         
.FPMultiply_label_91:
/* @95 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @97 */ 	jsr         __pushf0
/* @98 */ 	ldx         #__b1
/* @99 */ 	ldy          #0
/* @100 */ 	jsr         FPIsInfinity
/* @101 */ 	jsr         __incsp4
/* @103 */ 	lda         __b1
/* @104 */ 	bne         .FPMultiply_label_110
/* @105 */ 	jsr         __pushf1
/* @106 */ 	ldx         #__b1
/* @107 */ 	ldy          #0
/* @108 */ 	jsr         FPIsInfinity
/* @109 */ 	jsr         __incsp4
.FPMultiply_label_110:
/* @112 */ 	lda         __b1
/* @113 */ 	cmp          #0
/* @114 */ 	beq         .FPMultiply_label_124
/* @116 */ 	ldx         #__f0
/* @117 */ 	ldy          #0
/* @118 */ 	jsr         FPInfinity
/* @120 */ 	ldx          #38
	jsr          __load_result
/* @121 */ 	lda         #__f0
/* @122 */ 	jsr         __result4
/* @123 */ 	bra         .FPMultiply_label_88
.FPMultiply_label_124:
/* @128 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @130 */ 	jsr         __pushf0
/* @131 */ 	ldx         #__b4
/* @132 */ 	ldy          #0
/* @133 */ 	jsr         FPIsNan
/* @134 */ 	jsr         __incsp4
/* @136 */ 	lda         __b4
/* @137 */ 	bne         .FPMultiply_label_143
/* @138 */ 	jsr         __pushf1
/* @139 */ 	ldx         #__b4
/* @140 */ 	ldy          #0
/* @141 */ 	jsr         FPIsNan
/* @142 */ 	jsr         __incsp4
.FPMultiply_label_143:
/* @145 */ 	lda         __b4
/* @146 */ 	cmp          #0
/* @147 */ 	beq         .FPMultiply_label_157
/* @149 */ 	ldx         #__f0
/* @150 */ 	ldy          #0
/* @151 */ 	jsr         FPNaN
/* @153 */ 	ldx          #38
	jsr          __load_result
/* @154 */ 	lda         #__f0
/* @155 */ 	jsr         __result4
/* @156 */ 	jmp         .FPMultiply_label_88
.FPMultiply_label_157:
/* @159 */ 	ldx          #15
	jsr          __var_addr_i4			// ua
/* @161 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @163 */ 	jsr         __pushf0
/* @165 */ 	jsr         __pushi4
/* @166 */ 	jsr         Unpack
/* @168 */ 	jsr         __pullxy
/* @169 */ 	stx         __i4
/* @170 */ 	sty         __i4+1
/* @171 */ 	jsr         __incsp4
/* @173 */ 	ldx          #21
	jsr          __var_addr_i5			// ub
/* @174 */ 	jsr         __pushf1
/* @176 */ 	jsr         __pushi5
/* @177 */ 	jsr         Unpack
/* @178 */ 	jsr         __pullxy
/* @179 */ 	stx         __i5
/* @180 */ 	sty         __i5+1
/* @181 */ 	jsr         __incsp4
/* @187 */ 	lda         (__i4)
/* @188 */ 	sta         __b5
/* @192 */ 	lda         __b5
/* @193 */ 	sta         __i0
/* @194 */ 	lda          #0
/* @195 */ 	sta         __i0+1
/* @201 */ 	lda         (__i5)
/* @202 */ 	sta         __b5
/* @206 */ 	lda         __b5
/* @207 */ 	sta         __i1
/* @208 */ 	lda          #0
/* @209 */ 	sta         __i1+1
/* @214 */ 	lda         __i0
/* @215 */ 	eor         __i1
/* @216 */ 	sta         __i2
/* @217 */ 	lda         __i0+1
/* @218 */ 	eor         __i1+1
/* @219 */ 	sta         __i2+1
/* @222 */ 	lda         __i2
/* @223 */ 	sta         __b2
/* @229 */ 	ldy          #1
/* @230 */ 	lda         (__i4), Y
/* @231 */ 	sta         __b5
/* @235 */ 	lda         __b5
/* @236 */ 	sta         __i0
/* @237 */ 	lda          #0
/* @238 */ 	sta         __i0+1
/* @244 */ 	ldy          #1
/* @245 */ 	lda         (__i5), Y
/* @246 */ 	sta         __b5
/* @250 */ 	lda         __b5
/* @251 */ 	sta         __i1
/* @252 */ 	lda          #0
/* @253 */ 	sta         __i1+1
/* @258 */ 	clc         
/* @259 */ 	lda         __i0
/* @260 */ 	adc         __i1
/* @261 */ 	sta         __i2
/* @262 */ 	lda         __i0+1
/* @263 */ 	adc         __i1+1
/* @264 */ 	sta         __i2+1
/* @268 */ 	sec         
/* @269 */ 	lda         __i2
/* @270 */ 	sbc          #127
/* @271 */ 	sta         __i0
/* @272 */ 	lda         __i2+1
/* @273 */ 	sbc          #0
/* @274 */ 	sta         __i0+1
/* @277 */ 	lda         __i0
/* @278 */ 	sta         __b3
/* @284 */ 	ldy          #2
/* @285 */ 	lda         (__i4), Y
/* @286 */ 	sta         __l0
/* @287 */ 	ldy          #3
/* @288 */ 	lda         (__i4), Y
/* @289 */ 	sta         __l0+1
/* @291 */ 	ldy          #4
/* @292 */ 	lda         (__i4), Y
/* @293 */ 	sta         __l0+2
/* @295 */ 	ldy          #5
/* @296 */ 	lda         (__i4), Y
/* @297 */ 	sta         __l0+3
/* @303 */ 	ldy          #2
/* @304 */ 	lda         (__i5), Y
/* @305 */ 	sta         __l1
/* @306 */ 	ldy          #3
/* @307 */ 	lda         (__i5), Y
/* @308 */ 	sta         __l1+1
/* @309 */ 	ldy          #4
/* @310 */ 	lda         (__i5), Y
/* @311 */ 	sta         __l1+2
/* @312 */ 	ldy          #5
/* @313 */ 	lda         (__i5), Y
/* @314 */ 	sta         __l1+3
/* @316 */ 	jsr         __pushl1
/* @318 */ 	jsr         __pushl0
/* @320 */ 	ldx         #__x2
/* @321 */ 	ldy          #0
/* @322 */ 	jsr         Multiply32
/* @324 */ 	jsr         __incsp8
/* @328 */ 	ldx          #7
.FPMultiply_label_329:
/* @331 */ 	lda         __x2, X
/* @332 */ 	sta         __x1, X
/* @333 */ 	dex         
/* @334 */ 	bpl         .FPMultiply_label_329
/* @336 */ 	ldx          #27
	jsr          __var_addr_i0			// r
/* @338 */ 	lda         __b2
/* @339 */ 	sta         (__i0)
/* @342 */ 	lda         __b3
/* @343 */ 	sta         __i6
/* @345 */ 	and          #128
/* @347 */ 	beq         .FPMultiply_label_346
/* @349 */ 	lda          #255
.FPMultiply_label_346:
/* @350 */ 	sta         __i6+1
/* @354 */ 	clc         
/* @355 */ 	lda         __i6
/* @356 */ 	adc          #1
/* @357 */ 	sta         __i1
/* @358 */ 	lda         __i6+1
/* @359 */ 	adc          #0
/* @360 */ 	sta         __i1+1
/* @366 */ 	lda         __i1
/* @367 */ 	ldy          #1
/* @368 */ 	sta         (__i0), Y
/* @371 */ 	lda         __x1+4
/* @372 */ 	sta         __x0
/* @373 */ 	lda         __x1+5
/* @374 */ 	sta         __x0+1
/* @376 */ 	lda         __x1+6
/* @377 */ 	sta         __x0+2
/* @378 */ 	lda         __x1+7
/* @379 */ 	sta         __x0+3
/* @380 */ 	lda          #0
/* @381 */ 	sta         __x0+4
/* @382 */ 	sta         __x0+5
/* @383 */ 	sta         __x0+6
/* @384 */ 	sta         __x0+7
/* @388 */ 	lda         __x0
/* @389 */ 	sta         __l0
/* @390 */ 	lda         __x0+1
/* @391 */ 	sta         __l0+1
/* @392 */ 	lda         __x0+2
/* @393 */ 	sta         __l0+2
/* @394 */ 	lda         __x0+3
/* @395 */ 	sta         __l0+3
/* @401 */ 	lda         __l0
/* @402 */ 	ldy          #2
/* @403 */ 	sta         (__i0), Y
/* @404 */ 	lda         __l0+1
/* @405 */ 	ldy          #3
/* @406 */ 	sta         (__i0), Y
/* @407 */ 	lda         __l0+2
/* @408 */ 	ldy          #4
/* @409 */ 	sta         (__i0), Y
/* @410 */ 	lda         __l0+3
/* @411 */ 	ldy          #5
/* @412 */ 	sta         (__i0), Y
/* @414 */ 	ldx          #33
	jsr          __var_addr_i6			// __invented__13
/* @416 */ 	ldx          #9
	jsr          __var_addr_i1			// __invented__14
/* @419 */ 	lda          #6
/* @421 */ 	sta         __mem_size
/* @423 */ 	lda         __i0
/* @425 */ 	sta         __mem_src
/* @426 */ 	lda         __i0+1
/* @428 */ 	sta         __mem_src+1
/* @430 */ 	jsr         __pushmem1
/* @432 */ 	jsr         __pushi1
/* @433 */ 	jsr         Normalize
/* @434 */ 	jsr         __pullxy
/* @435 */ 	stx         __i1
/* @436 */ 	sty         __i1+1
/* @438 */ 	jsr         __incsp6
/* @441 */ 	lda          #6
/* @442 */ 	sta         __mem_size
/* @444 */ 	jsr         __pushmem_xy1
/* @446 */ 	jsr         __pushi6
/* @447 */ 	jsr         Round
/* @448 */ 	jsr         __pullxy
/* @449 */ 	stx         __i6
/* @450 */ 	sty         __i6+1
/* @451 */ 	jsr         __incsp6
/* @454 */ 	lda          #6
/* @455 */ 	sta         __mem_size
/* @456 */ 	jsr         __pushmem_xy1
/* @458 */ 	ldx         #__f0
/* @459 */ 	ldy          #0
/* @460 */ 	jsr         Pack
/* @461 */ 	jsr         __incsp6
/* @463 */ 	ldx          #38
	jsr          __load_result
/* @464 */ 	lda         #__f0
/* @465 */ 	jsr         __result4
/* @466 */ 	jmp         .FPMultiply_label_88
.func_end_FPMultiply:
	.size FPMultiply, .func_end_FPMultiply-FPMultiply

	.global FPDivide
	.type FPDivide, @function

FPDivide:
/* @15 */ 	stx         __result
/* @17 */ 	sty         __result+1
/* @18 */ 	ldx          #53
	jsr          __enter
	.byte        0x64,0x20,0x01		// Save mask i:4 b:3 l:0 x:1 f:1 
/* @34 */ 	ldx          #4
	jsr          __arg_value4_f1			// b
/* @52 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @54 */ 	jsr         __pushf0
/* @57 */ 	ldx         #__b0
/* @58 */ 	ldy          #0
/* @59 */ 	jsr         FPIsZero
/* @61 */ 	jsr         __incsp4
/* @63 */ 	lda         __b0
/* @64 */ 	cmp          #0
/* @65 */ 	beq         .FPDivide_label_84
/* @67 */ 	lda          #0
/* @68 */ 	sta         __f0
/* @69 */ 	lda          #0
/* @70 */ 	sta         __f0+1
/* @71 */ 	lda          #0
/* @73 */ 	sta         __f0+2
/* @74 */ 	lda          #0
/* @76 */ 	sta         __f0+3
/* @77 */ 	ldx          #54
	jsr          __load_result
/* @78 */ 	lda         #__f0
/* @80 */ 	jsr         __result4
.FPDivide_label_81:
/* @82 */ 	ldy          #56
	jsr          __leave
/* @83 */ 	rts         
.FPDivide_label_84:
/* @85 */ 	jsr         __pushf1
/* @87 */ 	ldx         #__b0
/* @88 */ 	ldy          #0
/* @89 */ 	jsr         FPIsZero
/* @90 */ 	jsr         __incsp4
/* @92 */ 	lda         __b0
/* @93 */ 	cmp          #0
/* @94 */ 	beq         .FPDivide_label_104
/* @96 */ 	ldx         #__f0
/* @97 */ 	ldy          #0
/* @98 */ 	jsr         FPInfinity
/* @100 */ 	ldx          #54
	jsr          __load_result
/* @101 */ 	lda         #__f0
/* @102 */ 	jsr         __result4
/* @103 */ 	bra         .FPDivide_label_81
.FPDivide_label_104:
/* @108 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @110 */ 	jsr         __pushf0
/* @111 */ 	ldx         #__b0
/* @112 */ 	ldy          #0
/* @113 */ 	jsr         FPIsInfinity
/* @114 */ 	jsr         __incsp4
/* @116 */ 	lda         __b0
/* @117 */ 	bne         .FPDivide_label_123
/* @118 */ 	jsr         __pushf1
/* @119 */ 	ldx         #__b0
/* @120 */ 	ldy          #0
/* @121 */ 	jsr         FPIsInfinity
/* @122 */ 	jsr         __incsp4
.FPDivide_label_123:
/* @125 */ 	lda         __b0
/* @126 */ 	cmp          #0
/* @127 */ 	beq         .FPDivide_label_137
/* @129 */ 	ldx         #__f0
/* @130 */ 	ldy          #0
/* @131 */ 	jsr         FPInfinity
/* @133 */ 	ldx          #54
	jsr          __load_result
/* @134 */ 	lda         #__f0
/* @135 */ 	jsr         __result4
/* @136 */ 	bra         .FPDivide_label_81
.FPDivide_label_137:
/* @141 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @143 */ 	jsr         __pushf0
/* @144 */ 	ldx         #__b1
/* @145 */ 	ldy          #0
/* @146 */ 	jsr         FPIsNan
/* @147 */ 	jsr         __incsp4
/* @149 */ 	lda         __b1
/* @150 */ 	bne         .FPDivide_label_156
/* @151 */ 	jsr         __pushf1
/* @152 */ 	ldx         #__b1
/* @153 */ 	ldy          #0
/* @154 */ 	jsr         FPIsNan
/* @155 */ 	jsr         __incsp4
.FPDivide_label_156:
/* @158 */ 	lda         __b1
/* @159 */ 	cmp          #0
/* @160 */ 	beq         .FPDivide_label_170
/* @162 */ 	ldx         #__f0
/* @163 */ 	ldy          #0
/* @164 */ 	jsr         FPNaN
/* @166 */ 	ldx          #54
	jsr          __load_result
/* @167 */ 	lda         #__f0
/* @168 */ 	jsr         __result4
/* @169 */ 	jmp         .FPDivide_label_81
.FPDivide_label_170:
/* @172 */ 	ldx          #15
	jsr          __var_addr_i4			// ua
/* @174 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @176 */ 	jsr         __pushf0
/* @178 */ 	jsr         __pushi4
/* @179 */ 	jsr         Unpack
/* @181 */ 	jsr         __pullxy
/* @182 */ 	stx         __i4
/* @183 */ 	sty         __i4+1
/* @184 */ 	jsr         __incsp4
/* @186 */ 	ldx          #21
	jsr          __var_addr_i5			// ub
/* @187 */ 	jsr         __pushf1
/* @189 */ 	jsr         __pushi5
/* @190 */ 	jsr         Unpack
/* @191 */ 	jsr         __pullxy
/* @192 */ 	stx         __i5
/* @193 */ 	sty         __i5+1
/* @194 */ 	jsr         __incsp4
/* @200 */ 	lda         (__i4)
/* @201 */ 	sta         __b4
/* @205 */ 	lda         __b4
/* @206 */ 	sta         __i0
/* @207 */ 	lda          #0
/* @208 */ 	sta         __i0+1
/* @214 */ 	lda         (__i5)
/* @215 */ 	sta         __b4
/* @219 */ 	lda         __b4
/* @220 */ 	sta         __i1
/* @221 */ 	lda          #0
/* @222 */ 	sta         __i1+1
/* @227 */ 	lda         __i0
/* @228 */ 	eor         __i1
/* @229 */ 	sta         __i2
/* @230 */ 	lda         __i0+1
/* @231 */ 	eor         __i1+1
/* @232 */ 	sta         __i2+1
/* @235 */ 	lda         __i2
/* @236 */ 	sta         __b2
/* @242 */ 	ldy          #1
/* @243 */ 	lda         (__i4), Y
/* @244 */ 	sta         __b4
/* @248 */ 	lda         __b4
/* @249 */ 	sta         __i0
/* @250 */ 	lda          #0
/* @251 */ 	sta         __i0+1
/* @257 */ 	ldy          #1
/* @258 */ 	lda         (__i5), Y
/* @259 */ 	sta         __b4
/* @263 */ 	lda         __b4
/* @264 */ 	sta         __i1
/* @265 */ 	lda          #0
/* @266 */ 	sta         __i1+1
/* @271 */ 	sec         
/* @272 */ 	lda         __i0
/* @273 */ 	sbc         __i1
/* @274 */ 	sta         __i2
/* @275 */ 	lda         __i0+1
/* @276 */ 	sbc         __i1+1
/* @277 */ 	sta         __i2+1
/* @281 */ 	clc         
/* @282 */ 	lda         __i2
/* @283 */ 	adc          #127
/* @284 */ 	sta         __i0
/* @285 */ 	lda         __i2+1
/* @286 */ 	adc          #0
/* @287 */ 	sta         __i0+1
/* @290 */ 	lda         __i0
/* @291 */ 	sta         __b3
/* @293 */ 	ldx          #37
	jsr          __var_addr_i0			// qr
/* @299 */ 	ldy          #2
/* @300 */ 	lda         (__i4), Y
/* @301 */ 	sta         __l0
/* @302 */ 	ldy          #3
/* @303 */ 	lda         (__i4), Y
/* @304 */ 	sta         __l0+1
/* @306 */ 	ldy          #4
/* @307 */ 	lda         (__i4), Y
/* @308 */ 	sta         __l0+2
/* @310 */ 	ldy          #5
/* @311 */ 	lda         (__i4), Y
/* @312 */ 	sta         __l0+3
/* @316 */ 	lda         __l0
/* @317 */ 	sta         __x0
/* @318 */ 	lda         __l0+1
/* @319 */ 	sta         __x0+1
/* @320 */ 	lda         __l0+2
/* @321 */ 	sta         __x0+2
/* @322 */ 	lda         __l0+3
/* @323 */ 	sta         __x0+3
/* @324 */ 	lda          #0
/* @326 */ 	ldy          #7
.FPDivide_label_327:
/* @329 */ 	sta         __x0, Y
/* @330 */ 	dey         
/* @331 */ 	cpy          #3
/* @332 */ 	bne         .FPDivide_label_327
/* @336 */ 	lda         __x0+3
/* @337 */ 	sta         __x1+7
/* @338 */ 	lda         __x0+2
/* @340 */ 	sta         __x1+6
/* @341 */ 	lda         __x0+1
/* @342 */ 	sta         __x1+5
/* @343 */ 	lda         __x0
/* @344 */ 	sta         __x1+4
/* @345 */ 	lda          #0
/* @346 */ 	sta         __x1
/* @347 */ 	sta         __x1+1
/* @348 */ 	sta         __x1+2
/* @349 */ 	sta         __x1+3
/* @355 */ 	ldy          #2
/* @356 */ 	lda         (__i5), Y
/* @357 */ 	sta         __l0
/* @358 */ 	ldy          #3
/* @359 */ 	lda         (__i5), Y
/* @360 */ 	sta         __l0+1
/* @361 */ 	ldy          #4
/* @362 */ 	lda         (__i5), Y
/* @363 */ 	sta         __l0+2
/* @364 */ 	ldy          #5
/* @365 */ 	lda         (__i5), Y
/* @366 */ 	sta         __l0+3
/* @370 */ 	lda         __l0
/* @371 */ 	sta         __x0
/* @372 */ 	lda         __l0+1
/* @373 */ 	sta         __x0+1
/* @374 */ 	lda         __l0+2
/* @375 */ 	sta         __x0+2
/* @376 */ 	lda         __l0+3
/* @377 */ 	sta         __x0+3
/* @378 */ 	lda          #0
/* @379 */ 	ldy          #7
.FPDivide_label_380:
/* @381 */ 	sta         __x0, Y
/* @382 */ 	dey         
/* @383 */ 	cpy          #3
/* @384 */ 	bne         .FPDivide_label_380
/* @386 */ 	jsr         __pushx0
/* @388 */ 	jsr         __pushx1
/* @390 */ 	jsr         __pushi0
/* @391 */ 	jsr         Divide64
/* @392 */ 	jsr         __pullxy
/* @393 */ 	stx         __i0
/* @394 */ 	sty         __i0+1
/* @396 */ 	jsr         __incsp16
.FPDivide_label_397:
/* @400 */ 	ldx          #37
	jsr          __var_addr_i6			// qr
/* @403 */ 	ldy          #7
.FPDivide_label_404:
/* @405 */ 	lda         (__i6), Y
/* @406 */ 	sta         __x0, Y
/* @407 */ 	dey         
/* @408 */ 	bpl         .FPDivide_label_404
/* @412 */ 	lda          #0
/* @413 */ 	sta         __x1
/* @414 */ 	lda          #0
/* @415 */ 	sta         __x1+1
/* @416 */ 	lda          #0
/* @417 */ 	sta         __x1+2
/* @418 */ 	lda          #0
/* @419 */ 	sta         __x1+3
/* @420 */ 	lda         __x0+4
/* @421 */ 	sta         __x1+4
/* @422 */ 	lda         __x0+5
/* @423 */ 	sta         __x1+5
/* @424 */ 	lda         __x0+6
/* @425 */ 	sta         __x1+6
/* @426 */ 	lda         __x0+7
/* @427 */ 	sta         __x1+7
/* @429 */ 	lda         __x1
/* @430 */ 	ora         __x1+1
/* @431 */ 	ora         __x1+2
/* @432 */ 	ora         __x1+3
/* @433 */ 	ora         __x1+4
/* @434 */ 	ora         __x1+5
/* @435 */ 	ora         __x1+6
/* @436 */ 	ora         __x1+7
/* @437 */ 	cmp          #0
/* @438 */ 	beq         .FPDivide_label_492
/* @444 */ 	ldy          #7
.FPDivide_label_445:
/* @446 */ 	lda         (__i6), Y
/* @447 */ 	sta         __x0, Y
/* @448 */ 	dey         
/* @449 */ 	bpl         .FPDivide_label_445
/* @453 */ 	lda         __x0+7
/* @454 */ 	lsr          A
/* @455 */ 	sta         __x1+7
/* @456 */ 	lda         __x0+6
/* @457 */ 	ror          A
/* @458 */ 	sta         __x1+6
/* @459 */ 	lda         __x0+5
/* @460 */ 	ror          A
/* @461 */ 	sta         __x1+5
/* @462 */ 	lda         __x0+4
/* @463 */ 	ror          A
/* @464 */ 	sta         __x1+4
/* @465 */ 	lda         __x0+3
/* @466 */ 	ror          A
/* @467 */ 	sta         __x1+3
/* @468 */ 	lda         __x0+2
/* @469 */ 	ror          A
/* @470 */ 	sta         __x1+2
/* @471 */ 	lda         __x0+1
/* @472 */ 	ror          A
/* @473 */ 	sta         __x1+1
/* @474 */ 	lda         __x0
/* @475 */ 	ror          A
/* @476 */ 	sta         __x1
/* @482 */ 	ldy          #7
.FPDivide_label_483:
/* @484 */ 	lda         __x1, Y
/* @485 */ 	sta         (__i6), Y
/* @486 */ 	dey         
/* @487 */ 	bpl         .FPDivide_label_483
/* @488 */ 	lda          #__b3
/* @490 */ 	jsr         __rinc1
/* @491 */ 	jmp         .FPDivide_label_397
.FPDivide_label_492:
/* @494 */ 	ldx          #43
	jsr          __var_addr_i0			// r
/* @496 */ 	lda         __b2
/* @497 */ 	sta         (__i0)
/* @499 */ 	lda         __b3
/* @500 */ 	sta         __i1
/* @501 */ 	lda          #0
/* @502 */ 	sta         __i1+1
/* @506 */ 	sec         
/* @507 */ 	lda         __i1
/* @508 */ 	sbc          #1
/* @509 */ 	sta         __i2
/* @510 */ 	lda         __i1+1
/* @511 */ 	sbc          #0
/* @512 */ 	sta         __i2+1
/* @518 */ 	lda         __i2
/* @519 */ 	ldy          #1
/* @520 */ 	sta         (__i0), Y
/* @526 */ 	ldy          #7
.FPDivide_label_527:
/* @528 */ 	lda         (__i6), Y
/* @529 */ 	sta         __x0, Y
/* @530 */ 	dey         
/* @531 */ 	bpl         .FPDivide_label_527
/* @535 */ 	lda         __x0
/* @536 */ 	sta         __l0
/* @537 */ 	lda         __x0+1
/* @538 */ 	sta         __l0+1
/* @539 */ 	lda         __x0+2
/* @540 */ 	sta         __l0+2
/* @541 */ 	lda         __x0+3
/* @542 */ 	sta         __l0+3
/* @548 */ 	lda         __l0
/* @549 */ 	ldy          #2
/* @550 */ 	sta         (__i0), Y
/* @551 */ 	lda         __l0+1
/* @552 */ 	ldy          #3
/* @553 */ 	sta         (__i0), Y
/* @554 */ 	lda         __l0+2
/* @555 */ 	ldy          #4
/* @556 */ 	sta         (__i0), Y
/* @557 */ 	lda         __l0+3
/* @558 */ 	ldy          #5
/* @559 */ 	sta         (__i0), Y
/* @561 */ 	ldx          #49
	jsr          __var_addr_i7			// __invented__17
/* @563 */ 	ldx          #9
	jsr          __var_addr_i1			// __invented__18
/* @566 */ 	lda          #6
/* @568 */ 	sta         __mem_size
/* @570 */ 	lda         __i0
/* @572 */ 	sta         __mem_src
/* @573 */ 	lda         __i0+1
/* @575 */ 	sta         __mem_src+1
/* @577 */ 	jsr         __pushmem1
/* @579 */ 	jsr         __pushi1
/* @580 */ 	jsr         Normalize
/* @581 */ 	jsr         __pullxy
/* @582 */ 	stx         __i1
/* @583 */ 	sty         __i1+1
/* @585 */ 	jsr         __incsp6
/* @588 */ 	lda          #6
/* @589 */ 	sta         __mem_size
/* @591 */ 	jsr         __pushmem_xy1
/* @593 */ 	jsr         __pushi7
/* @594 */ 	jsr         Round
/* @595 */ 	jsr         __pullxy
/* @596 */ 	stx         __i7
/* @597 */ 	sty         __i7+1
/* @598 */ 	jsr         __incsp6
/* @601 */ 	lda          #6
/* @602 */ 	sta         __mem_size
/* @603 */ 	jsr         __pushmem_xy1
/* @605 */ 	ldx         #__f0
/* @606 */ 	ldy          #0
/* @607 */ 	jsr         Pack
/* @608 */ 	jsr         __incsp6
/* @610 */ 	ldx          #54
	jsr          __load_result
/* @611 */ 	lda         #__f0
/* @612 */ 	jsr         __result4
/* @613 */ 	jmp         .FPDivide_label_81
.func_end_FPDivide:
	.size FPDivide, .func_end_FPDivide-FPDivide

	.global FPAdd
	.type FPAdd, @function

FPAdd:
/* @19 */ 	stx         __result
/* @21 */ 	sty         __result+1
/* @22 */ 	ldx          #55
	jsr          __enter
	.byte        0x84,0x40,0x01		// Save mask i:4 b:4 l:0 x:2 f:1 
/* @37 */ 	ldx          #4
	jsr          __arg_value4_f1			// b
/* @63 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @65 */ 	jsr         __pushf0
/* @67 */ 	ldx         #__b0
/* @68 */ 	ldy          #0
/* @69 */ 	jsr         FPIsZero
/* @71 */ 	jsr         __incsp4
/* @73 */ 	lda         __b0
/* @74 */ 	cmp          #0
/* @75 */ 	beq         .FPAdd_label_83
/* @76 */ 	ldx          #56
	jsr          __load_result
/* @77 */ 	lda         #__f1
/* @79 */ 	jsr         __result4
.FPAdd_label_80:
/* @81 */ 	ldy          #58
	jsr          __leave
/* @82 */ 	rts         
.FPAdd_label_83:
/* @84 */ 	jsr         __pushf1
/* @86 */ 	ldx         #__b0
/* @87 */ 	ldy          #0
/* @88 */ 	jsr         FPIsZero
/* @89 */ 	jsr         __incsp4
/* @91 */ 	lda         __b0
/* @92 */ 	cmp          #0
/* @93 */ 	beq         .FPAdd_label_101
/* @95 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @97 */ 	ldx          #56
	jsr          __load_result
/* @98 */ 	lda         #__f0
/* @99 */ 	jsr         __result4
/* @100 */ 	bra         .FPAdd_label_80
.FPAdd_label_101:
/* @105 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @107 */ 	jsr         __pushf0
/* @108 */ 	ldx         #__b0
/* @109 */ 	ldy          #0
/* @110 */ 	jsr         FPIsInfinity
/* @111 */ 	jsr         __incsp4
/* @113 */ 	lda         __b0
/* @114 */ 	bne         .FPAdd_label_120
/* @115 */ 	jsr         __pushf1
/* @116 */ 	ldx         #__b0
/* @117 */ 	ldy          #0
/* @118 */ 	jsr         FPIsInfinity
/* @119 */ 	jsr         __incsp4
.FPAdd_label_120:
/* @122 */ 	lda         __b0
/* @123 */ 	cmp          #0
/* @124 */ 	beq         .FPAdd_label_134
/* @126 */ 	ldx         #__f0
/* @127 */ 	ldy          #0
/* @128 */ 	jsr         FPInfinity
/* @130 */ 	ldx          #56
	jsr          __load_result
/* @131 */ 	lda         #__f0
/* @132 */ 	jsr         __result4
/* @133 */ 	bra         .FPAdd_label_80
.FPAdd_label_134:
/* @138 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @140 */ 	jsr         __pushf0
/* @141 */ 	ldx         #__b1
/* @142 */ 	ldy          #0
/* @143 */ 	jsr         FPIsNan
/* @144 */ 	jsr         __incsp4
/* @146 */ 	lda         __b1
/* @147 */ 	bne         .FPAdd_label_153
/* @148 */ 	jsr         __pushf1
/* @149 */ 	ldx         #__b1
/* @150 */ 	ldy          #0
/* @151 */ 	jsr         FPIsNan
/* @152 */ 	jsr         __incsp4
.FPAdd_label_153:
/* @155 */ 	lda         __b1
/* @156 */ 	cmp          #0
/* @157 */ 	beq         .FPAdd_label_167
/* @159 */ 	ldx         #__f0
/* @160 */ 	ldy          #0
/* @161 */ 	jsr         FPNaN
/* @163 */ 	ldx          #56
	jsr          __load_result
/* @164 */ 	lda         #__f0
/* @165 */ 	jsr         __result4
/* @166 */ 	jmp         .FPAdd_label_80
.FPAdd_label_167:
/* @169 */ 	ldx          #15
	jsr          __var_addr_i4			// ua
/* @171 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @173 */ 	jsr         __pushf0
/* @175 */ 	jsr         __pushi4
/* @176 */ 	jsr         Unpack
/* @178 */ 	jsr         __pullxy
/* @179 */ 	stx         __i4
/* @180 */ 	sty         __i4+1
/* @181 */ 	jsr         __incsp4
/* @183 */ 	ldx          #21
	jsr          __var_addr_i5			// ub
/* @184 */ 	jsr         __pushf1
/* @186 */ 	jsr         __pushi5
/* @187 */ 	jsr         Unpack
/* @188 */ 	jsr         __pullxy
/* @189 */ 	stx         __i5
/* @190 */ 	sty         __i5+1
/* @191 */ 	jsr         __incsp4
/* @195 */ 	ldy          #1
/* @196 */ 	lda         (__i4), Y
/* @197 */ 	sta         __b2
/* @203 */ 	ldy          #1
/* @204 */ 	lda         (__i4), Y
/* @205 */ 	sta         __b4
/* @209 */ 	lda         __b4
/* @210 */ 	sta         __i0
/* @211 */ 	lda          #0
/* @212 */ 	sta         __i0+1
/* @218 */ 	ldy          #1
/* @219 */ 	lda         (__i5), Y
/* @220 */ 	sta         __b4
/* @224 */ 	lda         __b4
/* @225 */ 	sta         __i1
/* @226 */ 	lda          #0
/* @227 */ 	sta         __i1+1
/* @231 */ 	lda         __i1+1
/* @232 */ 	cmp         __i0+1
/* @233 */ 	bcc         .FPAdd_label_230
/* @234 */ 	beq         .FPAdd_label_1139
/* @1140 */ 	jmp         .FPAdd_label_446
.FPAdd_label_1139:
/* @235 */ 	lda         __i1
/* @236 */ 	cmp         __i0
/* @237 */ 	bcc         .FPAdd_label_1141
/* @1142 */ 	jmp         .FPAdd_label_446
.FPAdd_label_1141:
.FPAdd_label_230:
/* @244 */ 	ldy          #1
/* @245 */ 	lda         (__i4), Y
/* @246 */ 	sta         __b4
/* @250 */ 	lda         __b4
/* @251 */ 	sta         __i0
/* @252 */ 	lda          #0
/* @253 */ 	sta         __i0+1
/* @259 */ 	ldy          #1
/* @260 */ 	lda         (__i5), Y
/* @261 */ 	sta         __b4
/* @265 */ 	lda         __b4
/* @266 */ 	sta         __i1
/* @267 */ 	lda          #0
/* @268 */ 	sta         __i1+1
/* @273 */ 	sec         
/* @274 */ 	lda         __i0
/* @275 */ 	sbc         __i1
/* @276 */ 	sta         __i2
/* @277 */ 	lda         __i0+1
/* @278 */ 	sbc         __i1+1
/* @279 */ 	sta         __i2+1
/* @282 */ 	lda         __i2
/* @283 */ 	sta         __b3
.FPAdd_label_284:
/* @289 */ 	lda         __b3
/* @290 */ 	sta         __b5
/* @291 */ 	lda          #__b3
/* @293 */ 	jsr         __rdec1
/* @297 */ 	lda         __b5
/* @298 */ 	sta         __i0
/* @300 */ 	and          #128
/* @302 */ 	beq         .FPAdd_label_301
/* @304 */ 	lda          #255
.FPAdd_label_301:
/* @305 */ 	sta         __i0+1
/* @308 */ 	ldx          #0
/* @311 */ 	lda          #0
/* @312 */ 	cmp         __i0
/* @313 */ 	lda          #0
/* @314 */ 	sbc         __i0+1
/* @315 */ 	bvc         .FPAdd_label_310
/* @316 */ 	eor          #128
.FPAdd_label_310:
/* @317 */ 	bpl         .FPAdd_label_307
/* @318 */ 	inx         
.FPAdd_label_307:
/* @319 */ 	stx         __b4
/* @321 */ 	lda         __b4
/* @322 */ 	cmp          #0
/* @323 */ 	beq         .FPAdd_label_358
/* @326 */ 	ldx          #21
	jsr          __var_addr_i0			// ub
/* @330 */ 	ldy          #2
/* @331 */ 	lda         (__i0), Y
/* @332 */ 	sta         __l0
/* @334 */ 	ldy          #3
/* @335 */ 	lda         (__i0), Y
/* @336 */ 	sta         __l0+1
/* @338 */ 	ldy          #4
/* @339 */ 	lda         (__i0), Y
/* @340 */ 	sta         __l0+2
/* @342 */ 	ldy          #5
/* @343 */ 	lda         (__i0), Y
/* @344 */ 	sta         __l0+3
/* @349 */ 	ldx          #1
/* @350 */ 	lda         __l0
/* @351 */ 	ora         __l0+1
/* @352 */ 	ora         __l0+2
/* @353 */ 	ora         __l0+3
/* @354 */ 	cmp          #0
/* @355 */ 	bne         .FPAdd_label_348
.FPAdd_label_347:
/* @356 */ 	dex         
.FPAdd_label_348:
/* @357 */ 	stx         __b4
.FPAdd_label_358:
/* @360 */ 	lda         __b4
/* @361 */ 	cmp          #0
/* @362 */ 	beq         .FPAdd_label_413
/* @365 */ 	ldx          #21
	jsr          __var_addr_i0			// ub
/* @368 */ 	ldy          #2
/* @369 */ 	lda         (__i0), Y
/* @370 */ 	sta         __l0
/* @371 */ 	ldy          #3
/* @372 */ 	lda         (__i0), Y
/* @373 */ 	sta         __l0+1
/* @374 */ 	ldy          #4
/* @375 */ 	lda         (__i0), Y
/* @376 */ 	sta         __l0+2
/* @377 */ 	ldy          #5
/* @378 */ 	lda         (__i0), Y
/* @379 */ 	sta         __l0+3
/* @383 */ 	lda         __l0+3
/* @384 */ 	lsr          A
/* @385 */ 	sta         __l1+3
/* @386 */ 	lda         __l0+2
/* @387 */ 	ror          A
/* @388 */ 	sta         __l1+2
/* @389 */ 	lda         __l0+1
/* @390 */ 	ror          A
/* @391 */ 	sta         __l1+1
/* @392 */ 	lda         __l0
/* @393 */ 	ror          A
/* @394 */ 	sta         __l1
/* @400 */ 	lda         __l1
/* @401 */ 	ldy          #2
/* @402 */ 	sta         (__i0), Y
/* @403 */ 	lda         __l1+1
/* @404 */ 	ldy          #3
/* @405 */ 	sta         (__i0), Y
/* @406 */ 	lda         __l1+2
/* @407 */ 	ldy          #4
/* @408 */ 	sta         (__i0), Y
/* @409 */ 	lda         __l1+3
/* @410 */ 	ldy          #5
/* @411 */ 	sta         (__i0), Y
/* @412 */ 	jmp         .FPAdd_label_284
.FPAdd_label_413:
/* @416 */ 	ldx          #21
	jsr          __var_addr_i0			// ub
/* @419 */ 	ldy          #2
/* @420 */ 	lda         (__i0), Y
/* @421 */ 	sta         __l0
/* @422 */ 	ldy          #3
/* @423 */ 	lda         (__i0), Y
/* @424 */ 	sta         __l0+1
/* @425 */ 	ldy          #4
/* @426 */ 	lda         (__i0), Y
/* @427 */ 	sta         __l0+2
/* @428 */ 	ldy          #5
/* @429 */ 	lda         (__i0), Y
/* @430 */ 	sta         __l0+3
/* @432 */ 	lda         __l0
/* @433 */ 	ora         __l0+1
/* @434 */ 	ora         __l0+2
/* @435 */ 	ora         __l0+3
/* @436 */ 	bne         .FPAdd_label_444
/* @438 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @440 */ 	ldx          #56
	jsr          __load_result
/* @441 */ 	lda         #__f0
/* @442 */ 	jsr         __result4
/* @443 */ 	jmp         .FPAdd_label_80
.FPAdd_label_444:
/* @445 */ 	jmp         .FPAdd_label_653
.FPAdd_label_446:
/* @452 */ 	ldy          #1
/* @453 */ 	lda         (__i5), Y
/* @454 */ 	sta         __b4
/* @458 */ 	lda         __b4
/* @459 */ 	sta         __i0
/* @460 */ 	lda          #0
/* @461 */ 	sta         __i0+1
/* @467 */ 	ldy          #1
/* @468 */ 	lda         (__i4), Y
/* @469 */ 	sta         __b4
/* @473 */ 	lda         __b4
/* @474 */ 	sta         __i1
/* @475 */ 	lda          #0
/* @476 */ 	sta         __i1+1
/* @481 */ 	sec         
/* @482 */ 	lda         __i0
/* @483 */ 	sbc         __i1
/* @484 */ 	sta         __i2
/* @485 */ 	lda         __i0+1
/* @486 */ 	sbc         __i1+1
/* @487 */ 	sta         __i2+1
/* @489 */ 	lda          #__i2
/* @491 */ 	ldx          #22
/* @493 */ 	jsr         __set_var_value1
.FPAdd_label_494:
/* @498 */ 	ldx          #22
	jsr          __var_value1_b5			// diff
/* @500 */ 	ldx          #22
	jsr          __var_addr_i0			// diff
/* @502 */ 	lda          #__i0
/* @504 */ 	jsr         __dec1
/* @508 */ 	lda         __b5
/* @509 */ 	sta         __i0
/* @510 */ 	and          #128
/* @512 */ 	beq         .FPAdd_label_511
/* @513 */ 	lda          #255
.FPAdd_label_511:
/* @514 */ 	sta         __i0+1
/* @517 */ 	ldx          #0
/* @520 */ 	lda          #0
/* @521 */ 	cmp         __i0
/* @522 */ 	lda          #0
/* @523 */ 	sbc         __i0+1
/* @524 */ 	bvc         .FPAdd_label_519
/* @525 */ 	eor          #128
.FPAdd_label_519:
/* @526 */ 	bpl         .FPAdd_label_516
/* @527 */ 	inx         
.FPAdd_label_516:
/* @528 */ 	stx         __b4
/* @530 */ 	lda         __b4
/* @531 */ 	cmp          #0
/* @532 */ 	beq         .FPAdd_label_563
/* @535 */ 	ldx          #15
	jsr          __var_addr_i0			// ua
/* @538 */ 	ldy          #2
/* @539 */ 	lda         (__i0), Y
/* @540 */ 	sta         __l0
/* @541 */ 	ldy          #3
/* @542 */ 	lda         (__i0), Y
/* @543 */ 	sta         __l0+1
/* @544 */ 	ldy          #4
/* @545 */ 	lda         (__i0), Y
/* @546 */ 	sta         __l0+2
/* @547 */ 	ldy          #5
/* @548 */ 	lda         (__i0), Y
/* @549 */ 	sta         __l0+3
/* @554 */ 	ldx          #1
/* @555 */ 	lda         __l0
/* @556 */ 	ora         __l0+1
/* @557 */ 	ora         __l0+2
/* @558 */ 	ora         __l0+3
/* @559 */ 	cmp          #0
/* @560 */ 	bne         .FPAdd_label_553
.FPAdd_label_552:
/* @561 */ 	dex         
.FPAdd_label_553:
/* @562 */ 	stx         __b4
.FPAdd_label_563:
/* @565 */ 	lda         __b4
/* @566 */ 	cmp          #0
/* @567 */ 	beq         .FPAdd_label_618
/* @570 */ 	ldx          #15
	jsr          __var_addr_i0			// ua
/* @573 */ 	ldy          #2
/* @574 */ 	lda         (__i0), Y
/* @575 */ 	sta         __l0
/* @576 */ 	ldy          #3
/* @577 */ 	lda         (__i0), Y
/* @578 */ 	sta         __l0+1
/* @579 */ 	ldy          #4
/* @580 */ 	lda         (__i0), Y
/* @581 */ 	sta         __l0+2
/* @582 */ 	ldy          #5
/* @583 */ 	lda         (__i0), Y
/* @584 */ 	sta         __l0+3
/* @588 */ 	lda         __l0+3
/* @589 */ 	lsr          A
/* @590 */ 	sta         __l1+3
/* @591 */ 	lda         __l0+2
/* @592 */ 	ror          A
/* @593 */ 	sta         __l1+2
/* @594 */ 	lda         __l0+1
/* @595 */ 	ror          A
/* @596 */ 	sta         __l1+1
/* @597 */ 	lda         __l0
/* @598 */ 	ror          A
/* @599 */ 	sta         __l1
/* @605 */ 	lda         __l1
/* @606 */ 	ldy          #2
/* @607 */ 	sta         (__i0), Y
/* @608 */ 	lda         __l1+1
/* @609 */ 	ldy          #3
/* @610 */ 	sta         (__i0), Y
/* @611 */ 	lda         __l1+2
/* @612 */ 	ldy          #4
/* @613 */ 	sta         (__i0), Y
/* @614 */ 	lda         __l1+3
/* @615 */ 	ldy          #5
/* @616 */ 	sta         (__i0), Y
/* @617 */ 	jmp         .FPAdd_label_494
.FPAdd_label_618:
/* @621 */ 	ldx          #15
	jsr          __var_addr_i0			// ua
/* @624 */ 	ldy          #2
/* @625 */ 	lda         (__i0), Y
/* @626 */ 	sta         __l0
/* @627 */ 	ldy          #3
/* @628 */ 	lda         (__i0), Y
/* @629 */ 	sta         __l0+1
/* @630 */ 	ldy          #4
/* @631 */ 	lda         (__i0), Y
/* @632 */ 	sta         __l0+2
/* @633 */ 	ldy          #5
/* @634 */ 	lda         (__i0), Y
/* @635 */ 	sta         __l0+3
/* @637 */ 	lda         __l0
/* @638 */ 	ora         __l0+1
/* @639 */ 	ora         __l0+2
/* @640 */ 	ora         __l0+3
/* @641 */ 	bne         .FPAdd_label_646
/* @642 */ 	ldx          #56
	jsr          __load_result
/* @643 */ 	lda         #__f1
/* @644 */ 	jsr         __result4
/* @645 */ 	jmp         .FPAdd_label_80
.FPAdd_label_646:
/* @648 */ 	ldx          #21
	jsr          __var_addr_i0			// ub
/* @650 */ 	ldy          #1
/* @651 */ 	lda         (__i0), Y
/* @652 */ 	sta         __b2
.FPAdd_label_653:
/* @659 */ 	ldy          #2
/* @660 */ 	lda         (__i4), Y
/* @661 */ 	sta         __l0
/* @662 */ 	ldy          #3
/* @663 */ 	lda         (__i4), Y
/* @664 */ 	sta         __l0+1
/* @665 */ 	ldy          #4
/* @666 */ 	lda         (__i4), Y
/* @667 */ 	sta         __l0+2
/* @668 */ 	ldy          #5
/* @669 */ 	lda         (__i4), Y
/* @670 */ 	sta         __l0+3
/* @674 */ 	lda         __l0
/* @675 */ 	sta         __x0
/* @676 */ 	lda         __l0+1
/* @677 */ 	sta         __x0+1
/* @678 */ 	lda         __l0+2
/* @679 */ 	sta         __x0+2
/* @680 */ 	lda         __l0+3
/* @681 */ 	sta         __x0+3
/* @682 */ 	lda          #0
/* @684 */ 	ldy          #7
.FPAdd_label_685:
/* @687 */ 	sta         __x0, Y
/* @688 */ 	dey         
/* @689 */ 	cpy          #3
/* @690 */ 	bne         .FPAdd_label_685
/* @693 */ 	ldx          #7
.FPAdd_label_694:
/* @695 */ 	lda         __x0, X
/* @696 */ 	sta         __x1, X
/* @697 */ 	dex         
/* @698 */ 	bpl         .FPAdd_label_694
/* @704 */ 	ldy          #2
/* @705 */ 	lda         (__i5), Y
/* @706 */ 	sta         __l0
/* @707 */ 	ldy          #3
/* @708 */ 	lda         (__i5), Y
/* @709 */ 	sta         __l0+1
/* @710 */ 	ldy          #4
/* @711 */ 	lda         (__i5), Y
/* @712 */ 	sta         __l0+2
/* @713 */ 	ldy          #5
/* @714 */ 	lda         (__i5), Y
/* @715 */ 	sta         __l0+3
/* @719 */ 	lda         __l0
/* @720 */ 	sta         __x0
/* @721 */ 	lda         __l0+1
/* @722 */ 	sta         __x0+1
/* @723 */ 	lda         __l0+2
/* @724 */ 	sta         __x0+2
/* @725 */ 	lda         __l0+3
/* @726 */ 	sta         __x0+3
/* @727 */ 	lda          #0
/* @728 */ 	ldy          #7
.FPAdd_label_729:
/* @730 */ 	sta         __x0, Y
/* @731 */ 	dey         
/* @732 */ 	cpy          #3
/* @733 */ 	bne         .FPAdd_label_729
/* @735 */ 	lda          #__x0
/* @737 */ 	ldx          #30
/* @739 */ 	jsr         __set_var_value8
/* @745 */ 	lda         (__i4)
/* @746 */ 	sta         __b4
/* @750 */ 	lda         __b4
/* @751 */ 	sta         __i0
/* @752 */ 	lda          #0
/* @753 */ 	sta         __i0+1
/* @756 */ 	lda         __i0
/* @757 */ 	cmp          #128
/* @758 */ 	bne         .FPAdd_label_774
/* @759 */ 	lda         __i0+1
/* @760 */ 	cmp          #0
/* @761 */ 	bne         .FPAdd_label_774
.FPAdd_label_755:
/* @763 */ 	sec         
/* @766 */ 	ldy          #8
/* @767 */ 	ldx          #0
.FPAdd_label_764:
/* @768 */ 	lda          #0
/* @769 */ 	sbc         __x1, X
/* @770 */ 	sta         __x1, X
/* @771 */ 	inx         
/* @772 */ 	dey         
/* @773 */ 	bne         .FPAdd_label_764
.FPAdd_label_774:
/* @780 */ 	lda         (__i5)
/* @781 */ 	sta         __b4
/* @785 */ 	lda         __b4
/* @786 */ 	sta         __i0
/* @787 */ 	lda          #0
/* @788 */ 	sta         __i0+1
/* @791 */ 	lda         __i0
/* @792 */ 	cmp          #128
/* @793 */ 	bne         .FPAdd_label_838
/* @794 */ 	lda         __i0+1
/* @795 */ 	cmp          #0
/* @796 */ 	bne         .FPAdd_label_838
.FPAdd_label_790:
/* @799 */ 	ldx          #30
	jsr          __var_value8_x0			// mB
/* @801 */ 	ldx          #30
	jsr          __var_addr_i0			// mB
/* @804 */ 	sec         
/* @805 */ 	ldy          #0
/* @806 */ 	lda          #0
/* @807 */ 	sbc         __x0
/* @808 */ 	sta         (__i0)
/* @809 */ 	ldy          #1
/* @810 */ 	lda          #0
/* @811 */ 	sbc         __x0+1
/* @812 */ 	sta         (__i0), Y
/* @813 */ 	ldy          #2
/* @814 */ 	lda          #0
/* @815 */ 	sbc         __x0+2
/* @816 */ 	sta         (__i0), Y
/* @817 */ 	ldy          #3
/* @818 */ 	lda          #0
/* @819 */ 	sbc         __x0+3
/* @820 */ 	sta         (__i0), Y
/* @821 */ 	ldy          #4
/* @822 */ 	lda          #0
/* @823 */ 	sbc         __x0+4
/* @824 */ 	sta         (__i0), Y
/* @825 */ 	ldy          #5
/* @826 */ 	lda          #0
/* @827 */ 	sbc         __x0+5
/* @828 */ 	sta         (__i0), Y
/* @830 */ 	ldy          #6
/* @831 */ 	lda          #0
/* @832 */ 	sbc         __x0+6
/* @833 */ 	sta         (__i0), Y
/* @834 */ 	ldy          #7
/* @835 */ 	lda          #0
/* @836 */ 	sbc         __x0+7
/* @837 */ 	sta         (__i0), Y
.FPAdd_label_838:
/* @840 */ 	ldx          #30
	jsr          __var_value8_x0			// mB
/* @842 */ 	ldx          #38
	jsr          __var_addr_i6			// mantissa
/* @845 */ 	clc         
/* @846 */ 	ldy          #0
/* @847 */ 	lda         __x1
/* @848 */ 	adc         __x0
/* @849 */ 	sta         (__i6)
/* @850 */ 	ldy          #1
/* @851 */ 	lda         __x1+1
/* @852 */ 	adc         __x0+1
/* @853 */ 	sta         (__i6), Y
/* @854 */ 	ldy          #2
/* @855 */ 	lda         __x1+2
/* @856 */ 	adc         __x0+2
/* @857 */ 	sta         (__i6), Y
/* @858 */ 	ldy          #3
/* @859 */ 	lda         __x1+3
/* @860 */ 	adc         __x0+3
/* @861 */ 	sta         (__i6), Y
/* @862 */ 	ldy          #4
/* @863 */ 	lda         __x1+4
/* @864 */ 	adc         __x0+4
/* @865 */ 	sta         (__i6), Y
/* @866 */ 	ldy          #5
/* @867 */ 	lda         __x1+5
/* @868 */ 	adc         __x0+5
/* @869 */ 	sta         (__i6), Y
/* @870 */ 	ldy          #6
/* @871 */ 	lda         __x1+6
/* @872 */ 	adc         __x0+6
/* @873 */ 	sta         (__i6), Y
/* @874 */ 	ldy          #7
/* @875 */ 	lda         __x1+7
/* @876 */ 	adc         __x0+7
/* @877 */ 	sta         (__i6), Y
/* @879 */ 	lda          #0
/* @880 */ 	sta         __b4
/* @881 */ 	lda          #__b4
/* @883 */ 	ldx          #39
/* @884 */ 	jsr         __set_var_value1
/* @886 */ 	ldx          #38
	jsr          __var_value8_x0			// mantissa
/* @890 */ 	lda          #0
/* @891 */ 	sta         __x2
/* @892 */ 	lda          #0
/* @893 */ 	sta         __x2+1
/* @894 */ 	lda          #0
/* @895 */ 	sta         __x2+2
/* @896 */ 	lda          #0
/* @897 */ 	sta         __x2+3
/* @898 */ 	lda          #0
/* @899 */ 	sta         __x2+4
/* @900 */ 	lda          #0
/* @901 */ 	sta         __x2+5
/* @902 */ 	lda          #0
/* @903 */ 	sta         __x2+6
/* @904 */ 	lda         __x0+7
/* @905 */ 	and          #128
/* @906 */ 	sta         __x2+7
/* @908 */ 	lda         __x2
/* @909 */ 	ora         __x2+1
/* @910 */ 	ora         __x2+2
/* @911 */ 	ora         __x2+3
/* @912 */ 	ora         __x2+4
/* @913 */ 	ora         __x2+5
/* @914 */ 	ora         __x2+6
/* @915 */ 	ora         __x2+7
/* @916 */ 	cmp          #0
/* @917 */ 	beq         .FPAdd_label_963
/* @919 */ 	lda          #128
/* @920 */ 	sta         __b4
/* @921 */ 	lda          #__b4
/* @922 */ 	ldx          #39
/* @923 */ 	jsr         __set_var_value1
/* @925 */ 	ldx          #38
	jsr          __var_value8_x0			// mantissa
/* @930 */ 	sec         
/* @931 */ 	ldy          #0
/* @932 */ 	lda          #0
/* @933 */ 	sbc         __x0
/* @934 */ 	sta         (__i6)
/* @935 */ 	ldy          #1
/* @936 */ 	lda          #0
/* @937 */ 	sbc         __x0+1
/* @938 */ 	sta         (__i6), Y
/* @939 */ 	ldy          #2
/* @940 */ 	lda          #0
/* @941 */ 	sbc         __x0+2
/* @942 */ 	sta         (__i6), Y
/* @943 */ 	ldy          #3
/* @944 */ 	lda          #0
/* @945 */ 	sbc         __x0+3
/* @946 */ 	sta         (__i6), Y
/* @947 */ 	ldy          #4
/* @948 */ 	lda          #0
/* @949 */ 	sbc         __x0+4
/* @950 */ 	sta         (__i6), Y
/* @951 */ 	ldy          #5
/* @952 */ 	lda          #0
/* @953 */ 	sbc         __x0+5
/* @954 */ 	sta         (__i6), Y
/* @955 */ 	ldy          #6
/* @956 */ 	lda          #0
/* @957 */ 	sbc         __x0+6
/* @958 */ 	sta         (__i6), Y
/* @959 */ 	ldy          #7
/* @960 */ 	lda          #0
/* @961 */ 	sbc         __x0+7
/* @962 */ 	sta         (__i6), Y
.FPAdd_label_963:
.FPAdd_label_964:
/* @966 */ 	ldx          #38
	jsr          __var_value8_x0			// mantissa
/* @970 */ 	lda          #0
/* @971 */ 	sta         __x2
/* @972 */ 	lda          #0
/* @973 */ 	sta         __x2+1
/* @974 */ 	lda          #0
/* @975 */ 	sta         __x2+2
/* @976 */ 	lda          #0
/* @977 */ 	sta         __x2+3
/* @978 */ 	lda         __x0+4
/* @979 */ 	sta         __x2+4
/* @980 */ 	lda         __x0+5
/* @981 */ 	sta         __x2+5
/* @982 */ 	lda         __x0+6
/* @983 */ 	sta         __x2+6
/* @984 */ 	lda         __x0+7
/* @985 */ 	sta         __x2+7
/* @987 */ 	lda         __x2
/* @988 */ 	ora         __x2+1
/* @989 */ 	ora         __x2+2
/* @990 */ 	ora         __x2+3
/* @991 */ 	ora         __x2+4
/* @992 */ 	ora         __x2+5
/* @993 */ 	ora         __x2+6
/* @994 */ 	ora         __x2+7
/* @995 */ 	cmp          #0
/* @996 */ 	beq         .FPAdd_label_1039
/* @998 */ 	ldx          #38
	jsr          __var_value8_x0			// mantissa
/* @1000 */ 	ldx          #38
	jsr          __var_addr_i0			// mantissa
/* @1003 */ 	ldy          #7
/* @1004 */ 	lda         __x0+7
/* @1005 */ 	lsr          A
/* @1006 */ 	sta         (__i0), Y
/* @1007 */ 	ldy          #6
/* @1008 */ 	lda         __x0+6
/* @1009 */ 	ror          A
/* @1010 */ 	sta         (__i0), Y
/* @1011 */ 	ldy          #5
/* @1012 */ 	lda         __x0+5
/* @1013 */ 	ror          A
/* @1014 */ 	sta         (__i0), Y
/* @1015 */ 	ldy          #4
/* @1016 */ 	lda         __x0+4
/* @1017 */ 	ror          A
/* @1018 */ 	sta         (__i0), Y
/* @1019 */ 	ldy          #3
/* @1020 */ 	lda         __x0+3
/* @1021 */ 	ror          A
/* @1022 */ 	sta         (__i0), Y
/* @1023 */ 	ldy          #2
/* @1024 */ 	lda         __x0+2
/* @1025 */ 	ror          A
/* @1026 */ 	sta         (__i0), Y
/* @1027 */ 	ldy          #1
/* @1028 */ 	lda         __x0+1
/* @1029 */ 	ror          A
/* @1030 */ 	sta         (__i0), Y
/* @1031 */ 	ldy          #0
/* @1032 */ 	lda         __x0
/* @1033 */ 	ror          A
/* @1034 */ 	sta         (__i0)
/* @1035 */ 	lda          #__b2
/* @1037 */ 	jsr         __rinc1
/* @1038 */ 	jmp         .FPAdd_label_964
.FPAdd_label_1039:
/* @1041 */ 	ldx          #39
	jsr          __var_value1_b4			// sign
/* @1044 */ 	ldx          #45
	jsr          __var_addr_i0			// r
/* @1047 */ 	lda         __b4
/* @1048 */ 	sta         (__i0)
/* @1052 */ 	lda         __b2
/* @1053 */ 	ldy          #1
/* @1054 */ 	sta         (__i0), Y
/* @1056 */ 	ldx          #38
	jsr          __var_value8_x0			// mantissa
/* @1060 */ 	lda         __x0
/* @1061 */ 	sta         __l0
/* @1062 */ 	lda         __x0+1
/* @1063 */ 	sta         __l0+1
/* @1064 */ 	lda         __x0+2
/* @1065 */ 	sta         __l0+2
/* @1066 */ 	lda         __x0+3
/* @1067 */ 	sta         __l0+3
/* @1073 */ 	lda         __l0
/* @1074 */ 	ldy          #2
/* @1075 */ 	sta         (__i0), Y
/* @1076 */ 	lda         __l0+1
/* @1077 */ 	ldy          #3
/* @1078 */ 	sta         (__i0), Y
/* @1079 */ 	lda         __l0+2
/* @1080 */ 	ldy          #4
/* @1081 */ 	sta         (__i0), Y
/* @1082 */ 	lda         __l0+3
/* @1083 */ 	ldy          #5
/* @1084 */ 	sta         (__i0), Y
/* @1086 */ 	ldx          #51
	jsr          __var_addr_i7			// __invented__22
/* @1088 */ 	ldx          #9
	jsr          __var_addr_i1			// __invented__23
/* @1091 */ 	lda          #6
/* @1093 */ 	sta         __mem_size
/* @1095 */ 	lda         __i0
/* @1097 */ 	sta         __mem_src
/* @1098 */ 	lda         __i0+1
/* @1100 */ 	sta         __mem_src+1
/* @1102 */ 	jsr         __pushmem1
/* @1104 */ 	jsr         __pushi1
/* @1105 */ 	jsr         Normalize
/* @1106 */ 	jsr         __pullxy
/* @1107 */ 	stx         __i1
/* @1108 */ 	sty         __i1+1
/* @1110 */ 	jsr         __incsp6
/* @1113 */ 	lda          #6
/* @1114 */ 	sta         __mem_size
/* @1116 */ 	jsr         __pushmem_xy1
/* @1118 */ 	jsr         __pushi7
/* @1119 */ 	jsr         Round
/* @1120 */ 	jsr         __pullxy
/* @1121 */ 	stx         __i7
/* @1122 */ 	sty         __i7+1
/* @1123 */ 	jsr         __incsp6
/* @1126 */ 	lda          #6
/* @1127 */ 	sta         __mem_size
/* @1128 */ 	jsr         __pushmem_xy1
/* @1130 */ 	ldx         #__f0
/* @1131 */ 	ldy          #0
/* @1132 */ 	jsr         Pack
/* @1133 */ 	jsr         __incsp6
/* @1135 */ 	ldx          #56
	jsr          __load_result
/* @1136 */ 	lda         #__f0
/* @1137 */ 	jsr         __result4
/* @1138 */ 	jmp         .FPAdd_label_80
.func_end_FPAdd:
	.size FPAdd, .func_end_FPAdd-FPAdd

	.global FPEqual
	.type FPEqual, @function

FPEqual:
/* @3 */ 	stx         __result
/* @5 */ 	sty         __result+1
/* @6 */ 	ldx          #13
	jsr          __enter_leaf
	.byte        0x00,0x00,0x01		// Save mask i:0 b:0 l:0 x:0 f:1 
/* @12 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @17 */ 	lda          #__f0
/* @19 */ 	ldx          #7
/* @21 */ 	jsr         __set_var_value4
/* @23 */ 	ldx          #4
	jsr          __arg_value4_f1			// b
/* @25 */ 	lda          #__f1
/* @27 */ 	ldx          #11
/* @28 */ 	jsr         __set_var_value4
/* @31 */ 	ldx          #7
	jsr          __var_addr_i0			// ba
/* @35 */ 	ldy          #3
.FPEqual_label_36:
/* @38 */ 	lda         (__i0), Y
/* @39 */ 	sta         __l0, Y
/* @40 */ 	dey         
/* @41 */ 	bpl         .FPEqual_label_36
/* @44 */ 	ldx          #11
	jsr          __var_addr_i0			// bb
/* @47 */ 	ldy          #3
.FPEqual_label_48:
/* @49 */ 	lda         (__i0), Y
/* @50 */ 	sta         __l1, Y
/* @51 */ 	dey         
/* @52 */ 	bpl         .FPEqual_label_48
/* @60 */ 	ldx          #1
/* @62 */ 	lda         __l0
/* @63 */ 	cmp         __l1
/* @64 */ 	bne         .FPEqual_label_57
/* @65 */ 	lda         __l0+1
/* @66 */ 	cmp         __l1+1
/* @67 */ 	bne         .FPEqual_label_57
/* @69 */ 	lda         __l0+2
/* @70 */ 	cmp         __l1+2
/* @71 */ 	bne         .FPEqual_label_57
/* @72 */ 	lda         __l0+3
/* @73 */ 	cmp         __l1+3
/* @74 */ 	beq         .FPEqual_label_58
.FPEqual_label_57:
/* @75 */ 	dex         
.FPEqual_label_58:
/* @76 */ 	stx         __b0
/* @78 */ 	lda         #__b0
/* @80 */ 	jsr         __result1
.FPEqual_label_81:
/* @82 */ 	ldy          #16
	jsr          __leave_leaf
/* @83 */ 	rts         
.func_end_FPEqual:
	.size FPEqual, .func_end_FPEqual-FPEqual

	.global FPNotEqual
	.type FPNotEqual, @function

FPNotEqual:
/* @2 */ 	stx         __result
/* @4 */ 	sty         __result+1
/* @5 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x00,0x01		// Save mask i:0 b:0 l:0 x:0 f:1 
/* @10 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @14 */ 	ldx          #4
	jsr          __arg_value4_f1			// b
/* @16 */ 	jsr         __pushf1
/* @17 */ 	jsr         __pushf0
/* @20 */ 	ldx         #__b0
/* @21 */ 	ldy          #0
/* @22 */ 	jsr         FPEqual
/* @24 */ 	jsr         __incsp8
/* @29 */ 	lda         __b0
/* @30 */ 	cmp          #0
/* @31 */ 	beq         .FPNotEqual_label_28
/* @33 */ 	lda          #255
.FPNotEqual_label_28:
/* @34 */ 	inc          A
/* @35 */ 	sta         __b1
/* @37 */ 	ldx          #8
	jsr          __load_result
/* @38 */ 	lda         #__b1
/* @40 */ 	jsr         __result1
.FPNotEqual_label_41:
/* @42 */ 	ldy          #10
	jsr          __leave
/* @43 */ 	rts         
.func_end_FPNotEqual:
	.size FPNotEqual, .func_end_FPNotEqual-FPNotEqual

	.global FPLess
	.type FPLess, @function

FPLess:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #15
	jsr          __enter
	.byte        0x00,0x00,0x01		// Save mask i:0 b:0 l:0 x:0 f:1 
/* @15 */ 	ldx          #4
	jsr          __arg_value4_f1			// b
/* @22 */ 	jsr         __pushf1
/* @24 */ 	ldx         #__b0
/* @25 */ 	ldy          #0
/* @26 */ 	jsr         FPIsInfinity
/* @28 */ 	jsr         __incsp4
/* @30 */ 	lda         __b0
/* @31 */ 	cmp          #0
/* @32 */ 	beq         .FPLess_label_43
/* @34 */ 	lda          #1
/* @35 */ 	sta         __b0
/* @36 */ 	ldx          #16
	jsr          __load_result
/* @37 */ 	lda         #__b0
/* @39 */ 	jsr         __result1
.FPLess_label_40:
/* @41 */ 	ldy          #18
	jsr          __leave
/* @42 */ 	rts         
.FPLess_label_43:
/* @45 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @47 */ 	jsr         __pushf0
/* @49 */ 	ldx         #__b0
/* @50 */ 	ldy          #0
/* @51 */ 	jsr         FPIsInfinity
/* @52 */ 	jsr         __incsp4
/* @54 */ 	lda         __b0
/* @55 */ 	cmp          #0
/* @56 */ 	beq         .FPLess_label_64
/* @58 */ 	lda          #0
/* @59 */ 	sta         __b0
/* @60 */ 	ldx          #16
	jsr          __load_result
/* @61 */ 	lda         #__b0
/* @62 */ 	jsr         __result1
/* @63 */ 	bra         .FPLess_label_40
.FPLess_label_64:
/* @68 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @70 */ 	jsr         __pushf0
/* @71 */ 	ldx         #__b0
/* @72 */ 	ldy          #0
/* @73 */ 	jsr         FPIsNan
/* @74 */ 	jsr         __incsp4
/* @76 */ 	lda         __b0
/* @77 */ 	bne         .FPLess_label_83
/* @78 */ 	jsr         __pushf1
/* @79 */ 	ldx         #__b0
/* @80 */ 	ldy          #0
/* @81 */ 	jsr         FPIsNan
/* @82 */ 	jsr         __incsp4
.FPLess_label_83:
/* @85 */ 	lda         __b0
/* @86 */ 	cmp          #0
/* @87 */ 	beq         .FPLess_label_95
/* @89 */ 	lda          #0
/* @90 */ 	sta         __b1
/* @91 */ 	ldx          #16
	jsr          __load_result
/* @92 */ 	lda         #__b1
/* @93 */ 	jsr         __result1
/* @94 */ 	bra         .FPLess_label_40
.FPLess_label_95:
/* @97 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @99 */ 	lda          #__f0
/* @101 */ 	ldx          #7
/* @103 */ 	jsr         __set_var_value4
/* @104 */ 	lda          #__f1
/* @106 */ 	ldx          #11
/* @107 */ 	jsr         __set_var_value4
/* @110 */ 	ldx          #7
	jsr          __var_addr_i0			// ba
/* @114 */ 	ldy          #3
.FPLess_label_115:
/* @117 */ 	lda         (__i0), Y
/* @118 */ 	sta         __l0, Y
/* @119 */ 	dey         
/* @120 */ 	bpl         .FPLess_label_115
/* @123 */ 	lda         __l0+3
/* @124 */ 	bpl         .FPLess_label_197
/* @127 */ 	ldx          #11
	jsr          __var_addr_i1			// bb
/* @130 */ 	ldy          #3
.FPLess_label_131:
/* @132 */ 	lda         (__i1), Y
/* @133 */ 	sta         __l0, Y
/* @134 */ 	dey         
/* @135 */ 	bpl         .FPLess_label_131
/* @138 */ 	lda         __l0+3
/* @139 */ 	bpl         .FPLess_label_189
/* @145 */ 	ldy          #3
.FPLess_label_146:
/* @147 */ 	lda         (__i1), Y
/* @148 */ 	sta         __l0, Y
/* @149 */ 	dey         
/* @150 */ 	bpl         .FPLess_label_146
/* @156 */ 	ldy          #3
.FPLess_label_157:
/* @158 */ 	lda         (__i0), Y
/* @159 */ 	sta         __l1, Y
/* @160 */ 	dey         
/* @161 */ 	bpl         .FPLess_label_157
/* @166 */ 	ldx          #0
/* @169 */ 	lda         __l0
/* @170 */ 	cmp         __l1
/* @171 */ 	lda         __l0+1
/* @172 */ 	sbc         __l1+1
/* @174 */ 	lda         __l0+2
/* @175 */ 	sbc         __l1+2
/* @176 */ 	lda         __l0+3
/* @177 */ 	sbc         __l1+3
/* @178 */ 	bvc         .FPLess_label_168
/* @180 */ 	eor          #128
.FPLess_label_168:
/* @181 */ 	bpl         .FPLess_label_165
/* @182 */ 	inx         
.FPLess_label_165:
/* @183 */ 	stx         __b1
/* @185 */ 	ldx          #16
	jsr          __load_result
/* @186 */ 	lda         #__b1
/* @187 */ 	jsr         __result1
/* @188 */ 	jmp         .FPLess_label_40
.FPLess_label_189:
/* @191 */ 	lda          #1
/* @192 */ 	sta         __b1
/* @193 */ 	ldx          #16
	jsr          __load_result
/* @194 */ 	lda         #__b1
/* @195 */ 	jsr         __result1
/* @196 */ 	jmp         .FPLess_label_40
.FPLess_label_197:
/* @200 */ 	ldx          #11
	jsr          __var_addr_i1			// bb
/* @203 */ 	ldy          #3
.FPLess_label_204:
/* @205 */ 	lda         (__i1), Y
/* @206 */ 	sta         __l0, Y
/* @207 */ 	dey         
/* @208 */ 	bpl         .FPLess_label_204
/* @211 */ 	lda         __l0+3
/* @212 */ 	bpl         .FPLess_label_220
/* @214 */ 	lda          #0
/* @215 */ 	sta         __b1
/* @216 */ 	ldx          #16
	jsr          __load_result
/* @217 */ 	lda         #__b1
/* @218 */ 	jsr         __result1
/* @219 */ 	jmp         .FPLess_label_40
.FPLess_label_220:
/* @226 */ 	ldy          #3
.FPLess_label_227:
/* @228 */ 	lda         (__i0), Y
/* @229 */ 	sta         __l0, Y
/* @230 */ 	dey         
/* @231 */ 	bpl         .FPLess_label_227
/* @237 */ 	ldy          #3
.FPLess_label_238:
/* @239 */ 	lda         (__i1), Y
/* @240 */ 	sta         __l1, Y
/* @241 */ 	dey         
/* @242 */ 	bpl         .FPLess_label_238
/* @247 */ 	ldx          #0
/* @250 */ 	lda         __l0
/* @251 */ 	cmp         __l1
/* @252 */ 	lda         __l0+1
/* @253 */ 	sbc         __l1+1
/* @254 */ 	lda         __l0+2
/* @255 */ 	sbc         __l1+2
/* @256 */ 	lda         __l0+3
/* @257 */ 	sbc         __l1+3
/* @258 */ 	bvc         .FPLess_label_249
/* @259 */ 	eor          #128
.FPLess_label_249:
/* @260 */ 	bpl         .FPLess_label_246
/* @261 */ 	inx         
.FPLess_label_246:
/* @262 */ 	stx         __b1
/* @264 */ 	ldx          #16
	jsr          __load_result
/* @265 */ 	lda         #__b1
/* @266 */ 	jsr         __result1
/* @267 */ 	jmp         .FPLess_label_40
.func_end_FPLess:
	.size FPLess, .func_end_FPLess-FPLess

	.global FPGreater
	.type FPGreater, @function

FPGreater:
/* @2 */ 	stx         __result
/* @4 */ 	sty         __result+1
/* @5 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x00,0x01		// Save mask i:0 b:0 l:0 x:0 f:1 
/* @10 */ 	ldx          #4
	jsr          __arg_value4_f0			// b
/* @14 */ 	ldx          #0
	jsr          __arg_value4_f1			// a
/* @16 */ 	jsr         __pushf1
/* @17 */ 	jsr         __pushf0
/* @20 */ 	ldx         #__b0
/* @21 */ 	ldy          #0
/* @22 */ 	jsr         FPLess
/* @24 */ 	jsr         __incsp8
/* @26 */ 	ldx          #8
	jsr          __load_result
/* @27 */ 	lda         #__b0
/* @29 */ 	jsr         __result1
.FPGreater_label_30:
/* @31 */ 	ldy          #10
	jsr          __leave
/* @32 */ 	rts         
.func_end_FPGreater:
	.size FPGreater, .func_end_FPGreater-FPGreater

	.global FPGreaterEqual
	.type FPGreaterEqual, @function

FPGreaterEqual:
/* @2 */ 	stx         __result
/* @4 */ 	sty         __result+1
/* @5 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x00,0x01		// Save mask i:0 b:0 l:0 x:0 f:1 
/* @10 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @14 */ 	ldx          #4
	jsr          __arg_value4_f1			// b
/* @16 */ 	jsr         __pushf1
/* @17 */ 	jsr         __pushf0
/* @20 */ 	ldx         #__b0
/* @21 */ 	ldy          #0
/* @22 */ 	jsr         FPLess
/* @24 */ 	jsr         __incsp8
/* @29 */ 	lda         __b0
/* @30 */ 	cmp          #0
/* @31 */ 	beq         .FPGreaterEqual_label_28
/* @33 */ 	lda          #255
.FPGreaterEqual_label_28:
/* @34 */ 	inc          A
/* @35 */ 	sta         __b1
/* @37 */ 	ldx          #8
	jsr          __load_result
/* @38 */ 	lda         #__b1
/* @40 */ 	jsr         __result1
.FPGreaterEqual_label_41:
/* @42 */ 	ldy          #10
	jsr          __leave
/* @43 */ 	rts         
.func_end_FPGreaterEqual:
	.size FPGreaterEqual, .func_end_FPGreaterEqual-FPGreaterEqual

	.global FPLessEqual
	.type FPLessEqual, @function

FPLessEqual:
/* @2 */ 	stx         __result
/* @4 */ 	sty         __result+1
/* @5 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x00,0x01		// Save mask i:0 b:0 l:0 x:0 f:1 
/* @10 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @14 */ 	ldx          #4
	jsr          __arg_value4_f1			// b
/* @16 */ 	jsr         __pushf1
/* @17 */ 	jsr         __pushf0
/* @20 */ 	ldx         #__b0
/* @21 */ 	ldy          #0
/* @22 */ 	jsr         FPGreater
/* @24 */ 	jsr         __incsp8
/* @29 */ 	lda         __b0
/* @30 */ 	cmp          #0
/* @31 */ 	beq         .FPLessEqual_label_28
/* @33 */ 	lda          #255
.FPLessEqual_label_28:
/* @34 */ 	inc          A
/* @35 */ 	sta         __b1
/* @37 */ 	ldx          #8
	jsr          __load_result
/* @38 */ 	lda         #__b1
/* @40 */ 	jsr         __result1
.FPLessEqual_label_41:
/* @42 */ 	ldy          #10
	jsr          __leave
/* @43 */ 	rts         
.func_end_FPLessEqual:
	.size FPLessEqual, .func_end_FPLessEqual-FPLessEqual

	.global ToFloat
	.type ToFloat, @function

ToFloat:
/* @13 */ 	stx         __result
/* @15 */ 	sty         __result+1
/* @16 */ 	ldx          #19
	jsr          __enter
	.byte        0x02,0x04,0x00		// Save mask i:2 b:0 l:2 x:0 f:0 
/* @22 */ 	ldx          #0
	jsr          __arg_value4_l2			// v
/* @27 */ 	lda         __l2
/* @28 */ 	ora         __l2+1
/* @30 */ 	ora         __l2+2
/* @32 */ 	ora         __l2+3
/* @33 */ 	bne         .ToFloat_label_50
/* @35 */ 	lda          #0
/* @36 */ 	sta         __f0
/* @37 */ 	lda          #0
/* @38 */ 	sta         __f0+1
/* @39 */ 	lda          #0
/* @40 */ 	sta         __f0+2
/* @41 */ 	lda          #0
/* @42 */ 	sta         __f0+3
/* @43 */ 	ldx          #20
	jsr          __load_result
/* @44 */ 	lda         #__f0
/* @46 */ 	jsr         __result4
.ToFloat_label_47:
/* @48 */ 	ldy          #22
	jsr          __leave
/* @49 */ 	rts         
.ToFloat_label_50:
/* @52 */ 	lda          #6
/* @54 */ 	sta         __mem_size
/* @56 */ 	ldx          #9
	jsr          __var_addr_i4			// u
/* @58 */ 	lda         __i4
/* @60 */ 	sta         __mem_dest
/* @61 */ 	lda         __i4+1
/* @63 */ 	sta         __mem_dest+1
/* @65 */ 	jsr         __zeromem1
/* @67 */ 	lda         __l2+3
/* @68 */ 	bpl         .ToFloat_label_86
/* @72 */ 	lda          #128
/* @73 */ 	sta         (__i4)
/* @74 */ 	sec         
/* @77 */ 	ldy          #4
/* @78 */ 	ldx          #0
.ToFloat_label_75:
/* @79 */ 	lda          #0
/* @81 */ 	sbc         __l2, X
/* @82 */ 	sta         __l2, X
/* @83 */ 	inx         
/* @84 */ 	dey         
/* @85 */ 	bne         .ToFloat_label_75
.ToFloat_label_86:
/* @90 */ 	lda          #127
/* @91 */ 	ldy          #1
/* @92 */ 	sta         (__i4), Y
.ToFloat_label_93:
/* @96 */ 	ldx          #9
	jsr          __var_addr_i5			// u
/* @99 */ 	ldy          #2
/* @100 */ 	lda         (__i5), Y
/* @101 */ 	sta         __l0
/* @102 */ 	ldy          #3
/* @103 */ 	lda         (__i5), Y
/* @104 */ 	sta         __l0+1
/* @105 */ 	ldy          #4
/* @106 */ 	lda         (__i5), Y
/* @107 */ 	sta         __l0+2
/* @109 */ 	ldy          #5
/* @110 */ 	lda         (__i5), Y
/* @111 */ 	sta         __l0+3
/* @115 */ 	lda         __l0+3
/* @116 */ 	lsr          A
/* @117 */ 	sta         __l1+3
/* @118 */ 	lda         __l0+2
/* @119 */ 	ror          A
/* @120 */ 	sta         __l1+2
/* @121 */ 	lda         __l0+1
/* @122 */ 	ror          A
/* @123 */ 	sta         __l1+1
/* @124 */ 	lda         __l0
/* @125 */ 	ror          A
/* @126 */ 	sta         __l1
/* @132 */ 	lda         __l1
/* @133 */ 	ldy          #2
/* @134 */ 	sta         (__i5), Y
/* @135 */ 	lda         __l1+1
/* @136 */ 	ldy          #3
/* @137 */ 	sta         (__i5), Y
/* @138 */ 	lda         __l1+2
/* @139 */ 	ldy          #4
/* @140 */ 	sta         (__i5), Y
/* @141 */ 	lda         __l1+3
/* @142 */ 	ldy          #5
/* @143 */ 	sta         (__i5), Y
/* @146 */ 	lda         __l2
/* @147 */ 	and          #1
/* @148 */ 	sta         __l0
/* @149 */ 	lda          #0
/* @150 */ 	sta         __l0+1
/* @151 */ 	lda          #0
/* @152 */ 	sta         __l0+2
/* @153 */ 	lda          #0
/* @154 */ 	sta         __l0+3
/* @158 */ 	lda         __l0
/* @159 */ 	sta         __l1+3
/* @160 */ 	lda          #0
/* @161 */ 	sta         __l1
/* @162 */ 	sta         __l1+1
/* @163 */ 	sta         __l1+2
/* @165 */ 	ldx          #7
.ToFloat_label_166:
/* @167 */ 	asl         __l1
/* @168 */ 	rol         __l1+1
/* @169 */ 	rol         __l1+2
/* @170 */ 	rol         __l1+3
/* @171 */ 	dex         
/* @172 */ 	bne         .ToFloat_label_166
/* @178 */ 	ldy          #2
/* @179 */ 	lda         (__i5), Y
/* @180 */ 	sta         __l0
/* @181 */ 	ldy          #3
/* @182 */ 	lda         (__i5), Y
/* @183 */ 	sta         __l0+1
/* @184 */ 	ldy          #4
/* @185 */ 	lda         (__i5), Y
/* @186 */ 	sta         __l0+2
/* @187 */ 	ldy          #5
/* @188 */ 	lda         (__i5), Y
/* @189 */ 	sta         __l0+3
/* @194 */ 	lda         __l0
/* @195 */ 	ora         __l1
/* @196 */ 	sta         __l3
/* @197 */ 	lda         __l0+1
/* @198 */ 	ora         __l1+1
/* @199 */ 	sta         __l3+1
/* @200 */ 	lda         __l0+2
/* @201 */ 	ora         __l1+2
/* @202 */ 	sta         __l3+2
/* @203 */ 	lda         __l0+3
/* @204 */ 	ora         __l1+3
/* @205 */ 	sta         __l3+3
/* @211 */ 	lda         __l3
/* @212 */ 	ldy          #2
/* @213 */ 	sta         (__i5), Y
/* @214 */ 	lda         __l3+1
/* @215 */ 	ldy          #3
/* @216 */ 	sta         (__i5), Y
/* @217 */ 	lda         __l3+2
/* @218 */ 	ldy          #4
/* @219 */ 	sta         (__i5), Y
/* @220 */ 	lda         __l3+3
/* @221 */ 	ldy          #5
/* @222 */ 	sta         (__i5), Y
/* @223 */ 	lda         __l2+3
/* @225 */ 	cmp          #128
/* @226 */ 	ror         __l2+3
/* @227 */ 	ror         __l2+2
/* @228 */ 	ror         __l2+1
/* @229 */ 	ror         __l2
/* @230 */ 	lda         __l2
/* @231 */ 	ora         __l2+1
/* @232 */ 	ora         __l2+2
/* @233 */ 	ora         __l2+3
/* @234 */ 	bne         .ToFloat_label_276
.ToFloat_label_235:
/* @237 */ 	ldx          #15
	jsr          __var_addr_i0			// __invented__24
/* @240 */ 	lda          #6
/* @241 */ 	sta         __mem_size
/* @243 */ 	lda         __i5
/* @245 */ 	sta         __mem_src
/* @246 */ 	lda         __i5+1
/* @248 */ 	sta         __mem_src+1
/* @250 */ 	jsr         __pushmem1
/* @252 */ 	jsr         __pushi0
/* @253 */ 	jsr         Round
/* @255 */ 	jsr         __pullxy
/* @256 */ 	stx         __i0
/* @257 */ 	sty         __i0+1
/* @259 */ 	jsr         __incsp6
/* @262 */ 	lda          #6
/* @263 */ 	sta         __mem_size
/* @265 */ 	jsr         __pushmem_xy1
/* @267 */ 	ldx         #__f0
/* @268 */ 	ldy          #0
/* @269 */ 	jsr         Pack
/* @270 */ 	jsr         __incsp6
/* @272 */ 	ldx          #20
	jsr          __load_result
/* @273 */ 	lda         #__f0
/* @274 */ 	jsr         __result4
/* @275 */ 	jmp         .ToFloat_label_47
.ToFloat_label_276:
/* @282 */ 	clc         
/* @283 */ 	lda         __i5
/* @284 */ 	adc          #1
/* @285 */ 	sta         __i0
/* @286 */ 	lda         __i5+1
/* @287 */ 	adc          #0
/* @288 */ 	sta         __i0+1
/* @290 */ 	lda          #__i0
/* @292 */ 	jsr         __inc1
.ToFloat_label_293:
/* @294 */ 	jmp         .ToFloat_label_93
.func_end_ToFloat:
	.size ToFloat, .func_end_ToFloat-ToFloat

	.global FromFloat
	.type FromFloat, @function

FromFloat:
/* @14 */ 	stx         __result
/* @16 */ 	sty         __result+1
/* @17 */ 	ldx          #13
	jsr          __enter
	.byte        0x41,0x02,0x01		// Save mask i:1 b:2 l:1 x:0 f:1 
/* @25 */ 	ldx          #0
	jsr          __arg_value4_f1			// f
/* @34 */ 	jsr         __pushf1
/* @36 */ 	ldx         #__b0
/* @37 */ 	ldy          #0
/* @38 */ 	jsr         FPIsZero
/* @40 */ 	jsr         __incsp4
/* @42 */ 	lda         __b0
/* @43 */ 	bne         .FromFloat_label_49
/* @44 */ 	jsr         __pushf1
/* @45 */ 	ldx         #__b0
/* @46 */ 	ldy          #0
/* @47 */ 	jsr         FPIsNan
/* @48 */ 	jsr         __incsp4
.FromFloat_label_49:
/* @51 */ 	lda         __b0
/* @52 */ 	bne         .FromFloat_label_58
/* @53 */ 	jsr         __pushf1
/* @54 */ 	ldx         #__b0
/* @55 */ 	ldy          #0
/* @56 */ 	jsr         FPIsInfinity
/* @57 */ 	jsr         __incsp4
.FromFloat_label_58:
/* @60 */ 	lda         __b0
/* @61 */ 	cmp          #0
/* @62 */ 	beq         .FromFloat_label_81
/* @64 */ 	lda          #0
/* @65 */ 	sta         __l0
/* @66 */ 	lda          #0
/* @67 */ 	sta         __l0+1
/* @68 */ 	lda          #0
/* @70 */ 	sta         __l0+2
/* @71 */ 	lda          #0
/* @73 */ 	sta         __l0+3
/* @74 */ 	ldx          #14
	jsr          __load_result
/* @75 */ 	lda         #__l0
/* @77 */ 	jsr         __result4
.FromFloat_label_78:
/* @79 */ 	ldy          #16
	jsr          __leave
/* @80 */ 	rts         
.FromFloat_label_81:
/* @83 */ 	ldx          #9
	jsr          __var_addr_i4			// u
/* @84 */ 	jsr         __pushf1
/* @86 */ 	jsr         __pushi4
/* @87 */ 	jsr         Unpack
/* @89 */ 	jsr         __pullxy
/* @90 */ 	stx         __i4
/* @91 */ 	sty         __i4+1
/* @92 */ 	jsr         __incsp4
/* @100 */ 	ldy          #1
/* @101 */ 	lda         (__i4), Y
/* @102 */ 	sta         __b3
/* @106 */ 	lda         __b3
/* @107 */ 	sta         __i0
/* @108 */ 	lda          #0
/* @109 */ 	sta         __i0+1
/* @114 */ 	ldx          #1
/* @115 */ 	lda         __i0+1
/* @116 */ 	cmp          #0
/* @117 */ 	bcc         .FromFloat_label_113
/* @118 */ 	bne         .FromFloat_label_112
/* @119 */ 	lda         __i0
/* @120 */ 	cmp          #127
/* @121 */ 	bcc         .FromFloat_label_113
.FromFloat_label_112:
/* @122 */ 	dex         
.FromFloat_label_113:
/* @123 */ 	stx         __b1
/* @125 */ 	lda         __b1
/* @126 */ 	bne         .FromFloat_label_156
/* @132 */ 	ldy          #1
/* @133 */ 	lda         (__i4), Y
/* @134 */ 	sta         __b3
/* @138 */ 	lda         __b3
/* @139 */ 	sta         __i0
/* @140 */ 	lda          #0
/* @141 */ 	sta         __i0+1
/* @146 */ 	ldx          #1
/* @147 */ 	lda          #0
/* @148 */ 	cmp         __i0+1
/* @149 */ 	bcc         .FromFloat_label_145
/* @150 */ 	bne         .FromFloat_label_144
/* @151 */ 	lda          #158
/* @152 */ 	cmp         __i0
/* @153 */ 	bcc         .FromFloat_label_145
.FromFloat_label_144:
/* @154 */ 	dex         
.FromFloat_label_145:
/* @155 */ 	stx         __b1
.FromFloat_label_156:
/* @158 */ 	lda         __b1
/* @159 */ 	cmp          #0
/* @160 */ 	beq         .FromFloat_label_174
/* @162 */ 	lda          #0
/* @163 */ 	sta         __l0
/* @164 */ 	lda          #0
/* @165 */ 	sta         __l0+1
/* @166 */ 	lda          #0
/* @167 */ 	sta         __l0+2
/* @168 */ 	lda          #0
/* @169 */ 	sta         __l0+3
/* @170 */ 	ldx          #14
	jsr          __load_result
/* @171 */ 	lda         #__l0
/* @172 */ 	jsr         __result4
/* @173 */ 	jmp         .FromFloat_label_78
.FromFloat_label_174:
/* @180 */ 	ldy          #1
/* @181 */ 	lda         (__i4), Y
/* @182 */ 	sta         __b3
/* @186 */ 	lda         __b3
/* @187 */ 	sta         __i0
/* @188 */ 	lda          #0
/* @189 */ 	sta         __i0+1
/* @193 */ 	sec         
/* @194 */ 	lda         __i0
/* @195 */ 	sbc          #127
/* @196 */ 	sta         __i1
/* @197 */ 	lda         __i0+1
/* @198 */ 	sbc          #0
/* @199 */ 	sta         __i1+1
/* @203 */ 	clc         
/* @204 */ 	lda         __i1
/* @205 */ 	adc          #1
/* @206 */ 	sta         __i0
/* @207 */ 	lda         __i1+1
/* @208 */ 	adc          #0
/* @209 */ 	sta         __i0+1
/* @212 */ 	lda         __i0
/* @213 */ 	sta         __b2
/* @214 */ 	lda          #0
/* @215 */ 	sta         __l2
/* @216 */ 	lda          #0
/* @217 */ 	sta         __l2+1
/* @218 */ 	lda          #0
/* @219 */ 	sta         __l2+2
/* @220 */ 	lda          #0
/* @221 */ 	sta         __l2+3
.FromFloat_label_222:
/* @225 */ 	lda         __b2
/* @226 */ 	sta         __i0
/* @228 */ 	and          #128
/* @230 */ 	beq         .FromFloat_label_229
/* @232 */ 	lda          #255
.FromFloat_label_229:
/* @233 */ 	sta         __i0+1
/* @236 */ 	lda          #0
/* @237 */ 	cmp         __i0
/* @238 */ 	lda          #0
/* @239 */ 	sbc         __i0+1
/* @240 */ 	bvc         .FromFloat_label_235
/* @241 */ 	eor          #128
.FromFloat_label_235:
/* @242 */ 	bmi         .FromFloat_label_390
/* @391 */ 	jmp         .FromFloat_label_350
.FromFloat_label_390:
/* @243 */ 	asl         __l2
/* @244 */ 	rol         __l2+1
/* @245 */ 	rol         __l2+2
/* @246 */ 	rol         __l2+3
/* @249 */ 	ldx          #9
	jsr          __var_addr_i0			// u
/* @252 */ 	ldy          #2
/* @253 */ 	lda         (__i0), Y
/* @254 */ 	sta         __l0
/* @255 */ 	ldy          #3
/* @256 */ 	lda         (__i0), Y
/* @257 */ 	sta         __l0+1
/* @259 */ 	ldy          #4
/* @260 */ 	lda         (__i0), Y
/* @261 */ 	sta         __l0+2
/* @263 */ 	ldy          #5
/* @264 */ 	lda         (__i0), Y
/* @265 */ 	sta         __l0+3
/* @269 */ 	lda         __l0+3
/* @270 */ 	sta         __l1
/* @271 */ 	lda          #0
/* @272 */ 	sta         __l1+1
/* @273 */ 	sta         __l1+2
/* @274 */ 	sta         __l1+3
/* @276 */ 	ldx          #7
.FromFloat_label_277:
/* @278 */ 	lsr         __l1+3
/* @279 */ 	ror         __l1+2
/* @280 */ 	ror         __l1+1
/* @281 */ 	ror         __l1
/* @282 */ 	dex         
/* @283 */ 	bne         .FromFloat_label_277
/* @285 */ 	lda         __l2
/* @286 */ 	ora         __l1
/* @287 */ 	sta         __l2
/* @288 */ 	lda         __l2+1
/* @289 */ 	ora         __l1+1
/* @290 */ 	sta         __l2+1
/* @291 */ 	lda         __l2+2
/* @292 */ 	ora         __l1+2
/* @293 */ 	sta         __l2+2
/* @294 */ 	lda         __l2+3
/* @295 */ 	ora         __l1+3
/* @296 */ 	sta         __l2+3
/* @302 */ 	ldy          #2
/* @303 */ 	lda         (__i0), Y
/* @304 */ 	sta         __l0
/* @305 */ 	ldy          #3
/* @306 */ 	lda         (__i0), Y
/* @307 */ 	sta         __l0+1
/* @308 */ 	ldy          #4
/* @309 */ 	lda         (__i0), Y
/* @310 */ 	sta         __l0+2
/* @311 */ 	ldy          #5
/* @312 */ 	lda         (__i0), Y
/* @313 */ 	sta         __l0+3
/* @317 */ 	lda         __l0
/* @318 */ 	asl          A
/* @319 */ 	sta         __l1
/* @320 */ 	lda         __l0+1
/* @321 */ 	rol          A
/* @322 */ 	sta         __l1+1
/* @323 */ 	lda         __l0+2
/* @324 */ 	rol          A
/* @325 */ 	sta         __l1+2
/* @326 */ 	lda         __l0+3
/* @327 */ 	rol          A
/* @328 */ 	sta         __l1+3
/* @334 */ 	lda         __l1
/* @335 */ 	ldy          #2
/* @336 */ 	sta         (__i0), Y
/* @337 */ 	lda         __l1+1
/* @338 */ 	ldy          #3
/* @339 */ 	sta         (__i0), Y
/* @340 */ 	lda         __l1+2
/* @341 */ 	ldy          #4
/* @342 */ 	sta         (__i0), Y
/* @343 */ 	lda         __l1+3
/* @344 */ 	ldy          #5
/* @345 */ 	sta         (__i0), Y
/* @346 */ 	lda          #__b2
/* @348 */ 	jsr         __rdec1
/* @349 */ 	jmp         .FromFloat_label_222
.FromFloat_label_350:
/* @353 */ 	ldx          #9
	jsr          __var_addr_i0			// u
/* @356 */ 	lda         (__i0)
/* @357 */ 	sta         __b3
/* @361 */ 	lda         __b3
/* @362 */ 	sta         __i0
/* @363 */ 	lda          #0
/* @364 */ 	sta         __i0+1
/* @367 */ 	lda         __i0
/* @368 */ 	cmp          #128
/* @369 */ 	bne         .FromFloat_label_385
/* @370 */ 	lda         __i0+1
/* @371 */ 	cmp          #0
/* @372 */ 	bne         .FromFloat_label_385
.FromFloat_label_366:
/* @374 */ 	sec         
/* @376 */ 	ldy          #4
/* @377 */ 	ldx          #0
.FromFloat_label_375:
/* @378 */ 	lda          #0
/* @380 */ 	sbc         __l2, X
/* @381 */ 	sta         __l2, X
/* @382 */ 	inx         
/* @383 */ 	dey         
/* @384 */ 	bne         .FromFloat_label_375
.FromFloat_label_385:
/* @386 */ 	ldx          #14
	jsr          __load_result
/* @387 */ 	lda         #__l2
/* @388 */ 	jsr         __result4
/* @389 */ 	jmp         .FromFloat_label_78
.func_end_FromFloat:
	.size FromFloat, .func_end_FromFloat-FromFloat

	.global FPSub
	.type FPSub, @function

FPSub:
/* @2 */ 	stx         __result
/* @4 */ 	sty         __result+1
/* @5 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x00,0x02		// Save mask i:0 b:0 l:0 x:0 f:2 
/* @10 */ 	ldx          #0
	jsr          __arg_value4_f0			// a
/* @14 */ 	ldx          #4
	jsr          __arg_value4_f1			// b
/* @20 */ 	ldx          #3
.FPSub_label_21:
/* @23 */ 	lda         __f1, X
/* @24 */ 	sta         __f2, X
/* @25 */ 	dex         
/* @26 */ 	bpl         .FPSub_label_21
/* @27 */ 	ldy          #3
/* @28 */ 	lda         __f2+3
/* @30 */ 	eor          #128
/* @31 */ 	sta         __f2+3
/* @33 */ 	jsr         __pushf2
/* @34 */ 	jsr         __pushf0
/* @37 */ 	ldx         #__f1
/* @38 */ 	ldy          #0
/* @39 */ 	jsr         FPAdd
/* @41 */ 	jsr         __incsp8
/* @43 */ 	ldx          #8
	jsr          __load_result
/* @44 */ 	lda         #__f1
/* @46 */ 	jsr         __result4
.FPSub_label_47:
/* @48 */ 	ldy          #10
	jsr          __leave
/* @49 */ 	rts         
.func_end_FPSub:
	.size FPSub, .func_end_FPSub-FPSub

	.global FixedIncrement128
	.type FixedIncrement128, @function

FixedIncrement128:
/* @8 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @15 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @16 */ 	lda          #0
/* @17 */ 	sta         __b0
.FixedIncrement128_label_18:
/* @20 */ 	lda         __b0
/* @21 */ 	sta         __i1
/* @22 */ 	lda          #0
/* @23 */ 	sta         __i1+1
/* @26 */ 	lda         __i1+1
/* @27 */ 	cmp          #0
/* @28 */ 	bcc         .FixedIncrement128_label_25
/* @29 */ 	bne         .FixedIncrement128_label_75
/* @30 */ 	lda         __i1
/* @31 */ 	cmp          #2
/* @32 */ 	bcs         .FixedIncrement128_label_75
.FixedIncrement128_label_25:
/* @34 */ 	lda          #__i0
/* @36 */ 	jsr         __inc8
/* @40 */ 	ldy          #7
.FixedIncrement128_label_41:
/* @43 */ 	lda         (__i0), Y
/* @44 */ 	sta         __x0, Y
/* @45 */ 	dey         
/* @46 */ 	bpl         .FixedIncrement128_label_41
/* @48 */ 	lda         __x0
/* @49 */ 	ora         __x0+1
/* @51 */ 	ora         __x0+2
/* @53 */ 	ora         __x0+3
/* @55 */ 	ora         __x0+4
/* @57 */ 	ora         __x0+5
/* @59 */ 	ora         __x0+6
/* @60 */ 	ora         __x0+7
/* @61 */ 	cmp          #0
/* @62 */ 	beq         .FixedIncrement128_label_64
/* @63 */ 	bra         .FixedIncrement128_label_75
.FixedIncrement128_label_64:
.FixedIncrement128_label_65:
/* @66 */ 	lda          #__i0
/* @68 */ 	ldx          #8
/* @70 */ 	jsr         __rinc2
/* @71 */ 	lda          #__b0
/* @73 */ 	jsr         __rinc1
/* @74 */ 	bra         .FixedIncrement128_label_18
.FixedIncrement128_label_75:
/* @76 */ 	ldy          #8
	jsr          __leave_leaf_void_nomask
/* @77 */ 	rts         
.func_end_FixedIncrement128:
	.size FixedIncrement128, .func_end_FixedIncrement128-FixedIncrement128

	.global FixedIncrement256
	.type FixedIncrement256, @function

FixedIncrement256:
/* @8 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @15 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @16 */ 	lda          #0
/* @17 */ 	sta         __b0
.FixedIncrement256_label_18:
/* @20 */ 	lda         __b0
/* @21 */ 	sta         __i1
/* @22 */ 	lda          #0
/* @23 */ 	sta         __i1+1
/* @26 */ 	lda         __i1+1
/* @27 */ 	cmp          #0
/* @28 */ 	bcc         .FixedIncrement256_label_25
/* @29 */ 	bne         .FixedIncrement256_label_75
/* @30 */ 	lda         __i1
/* @31 */ 	cmp          #4
/* @32 */ 	bcs         .FixedIncrement256_label_75
.FixedIncrement256_label_25:
/* @34 */ 	lda          #__i0
/* @36 */ 	jsr         __inc8
/* @40 */ 	ldy          #7
.FixedIncrement256_label_41:
/* @43 */ 	lda         (__i0), Y
/* @44 */ 	sta         __x0, Y
/* @45 */ 	dey         
/* @46 */ 	bpl         .FixedIncrement256_label_41
/* @48 */ 	lda         __x0
/* @49 */ 	ora         __x0+1
/* @51 */ 	ora         __x0+2
/* @53 */ 	ora         __x0+3
/* @55 */ 	ora         __x0+4
/* @57 */ 	ora         __x0+5
/* @59 */ 	ora         __x0+6
/* @60 */ 	ora         __x0+7
/* @61 */ 	cmp          #0
/* @62 */ 	beq         .FixedIncrement256_label_64
/* @63 */ 	bra         .FixedIncrement256_label_75
.FixedIncrement256_label_64:
.FixedIncrement256_label_65:
/* @66 */ 	lda          #__i0
/* @68 */ 	ldx          #8
/* @70 */ 	jsr         __rinc2
/* @71 */ 	lda          #__b0
/* @73 */ 	jsr         __rinc1
/* @74 */ 	bra         .FixedIncrement256_label_18
.FixedIncrement256_label_75:
/* @76 */ 	ldy          #8
	jsr          __leave_leaf_void_nomask
/* @77 */ 	rts         
.func_end_FixedIncrement256:
	.size FixedIncrement256, .func_end_FixedIncrement256-FixedIncrement256

	.global FixedAdd64
	.type FixedAdd64, @function

FixedAdd64:
/* @3 */ 	stx         __result
/* @5 */ 	sty         __result+1
/* @6 */ 	ldx          #13
	jsr          __enter_leaf
	.byte        0x00,0x60,0x00		// Save mask i:0 b:0 l:0 x:3 f:0 
/* @13 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @17 */ 	ldx          #2
	jsr          __arg_value2_i1			// b
/* @25 */ 	ldy          #7
.FixedAdd64_label_26:
/* @28 */ 	lda         (__i0), Y
/* @29 */ 	sta         __x1, Y
/* @30 */ 	dey         
/* @31 */ 	bpl         .FixedAdd64_label_26
/* @34 */ 	ldy          #7
.FixedAdd64_label_35:
/* @36 */ 	lda         (__i1), Y
/* @37 */ 	sta         __x2, Y
/* @38 */ 	dey         
/* @39 */ 	bpl         .FixedAdd64_label_35
/* @44 */ 	clc         
/* @46 */ 	lda         __x1
/* @47 */ 	adc         __x2
/* @48 */ 	sta         __x3
/* @50 */ 	lda         __x1+1
/* @51 */ 	adc         __x2+1
/* @52 */ 	sta         __x3+1
/* @54 */ 	lda         __x1+2
/* @55 */ 	adc         __x2+2
/* @56 */ 	sta         __x3+2
/* @58 */ 	lda         __x1+3
/* @59 */ 	adc         __x2+3
/* @60 */ 	sta         __x3+3
/* @62 */ 	lda         __x1+4
/* @63 */ 	adc         __x2+4
/* @64 */ 	sta         __x3+4
/* @66 */ 	lda         __x1+5
/* @67 */ 	adc         __x2+5
/* @68 */ 	sta         __x3+5
/* @70 */ 	lda         __x1+6
/* @71 */ 	adc         __x2+6
/* @72 */ 	sta         __x3+6
/* @73 */ 	lda         __x1+7
/* @74 */ 	adc         __x2+7
/* @75 */ 	sta         __x3+7
/* @77 */ 	ldx          #4
	jsr          __arg_value8_x1			// carry_in
/* @80 */ 	clc         
/* @81 */ 	lda         __x3
/* @82 */ 	adc         __x1
/* @83 */ 	sta         __x0
/* @84 */ 	lda         __x3+1
/* @85 */ 	adc         __x1+1
/* @86 */ 	sta         __x0+1
/* @87 */ 	lda         __x3+2
/* @88 */ 	adc         __x1+2
/* @89 */ 	sta         __x0+2
/* @90 */ 	lda         __x3+3
/* @91 */ 	adc         __x1+3
/* @92 */ 	sta         __x0+3
/* @93 */ 	lda         __x3+4
/* @94 */ 	adc         __x1+4
/* @95 */ 	sta         __x0+4
/* @96 */ 	lda         __x3+5
/* @97 */ 	adc         __x1+5
/* @98 */ 	sta         __x0+5
/* @99 */ 	lda         __x3+6
/* @100 */ 	adc         __x1+6
/* @101 */ 	sta         __x0+6
/* @102 */ 	lda         __x3+7
/* @103 */ 	adc         __x1+7
/* @104 */ 	sta         __x0+7
/* @107 */ 	ldy          #7
.FixedAdd64_label_108:
/* @109 */ 	lda         (__i0), Y
/* @110 */ 	sta         __x1, Y
/* @111 */ 	dey         
/* @112 */ 	bpl         .FixedAdd64_label_108
/* @118 */ 	ldx          #1
/* @119 */ 	lda         __x0+7
/* @120 */ 	cmp         __x1+7
/* @121 */ 	bcc         .FixedAdd64_label_117
/* @122 */ 	bne         .FixedAdd64_label_116
/* @123 */ 	lda         __x0+6
/* @124 */ 	cmp         __x1+6
/* @125 */ 	bcc         .FixedAdd64_label_117
/* @126 */ 	bne         .FixedAdd64_label_116
/* @127 */ 	lda         __x0+5
/* @128 */ 	cmp         __x1+5
/* @129 */ 	bcc         .FixedAdd64_label_117
/* @130 */ 	bne         .FixedAdd64_label_116
/* @131 */ 	lda         __x0+4
/* @132 */ 	cmp         __x1+4
/* @133 */ 	bcc         .FixedAdd64_label_117
/* @134 */ 	bne         .FixedAdd64_label_116
/* @135 */ 	lda         __x0+3
/* @136 */ 	cmp         __x1+3
/* @137 */ 	bcc         .FixedAdd64_label_117
/* @138 */ 	bne         .FixedAdd64_label_116
/* @139 */ 	lda         __x0+2
/* @140 */ 	cmp         __x1+2
/* @141 */ 	bcc         .FixedAdd64_label_117
/* @142 */ 	bne         .FixedAdd64_label_116
/* @143 */ 	lda         __x0+1
/* @144 */ 	cmp         __x1+1
/* @145 */ 	bcc         .FixedAdd64_label_117
/* @146 */ 	bne         .FixedAdd64_label_116
/* @147 */ 	lda         __x0
/* @148 */ 	cmp         __x1
/* @149 */ 	bcc         .FixedAdd64_label_117
.FixedAdd64_label_116:
/* @150 */ 	dex         
.FixedAdd64_label_117:
/* @151 */ 	stx         __b0
/* @155 */ 	ldx          #11
	jsr          __var_addr_i2			// carry_out
/* @156 */ 	ldy          #0
/* @157 */ 	lda         __b0
/* @158 */ 	sta         (__i2)
/* @159 */ 	lda          #0
/* @160 */ 	ldy          #7
.FixedAdd64_label_161:
/* @162 */ 	sta         (__i2), Y
/* @163 */ 	dey         
/* @164 */ 	cpy          #0
/* @165 */ 	bne         .FixedAdd64_label_161
/* @166 */ 	ldy          #7
.FixedAdd64_label_167:
/* @168 */ 	lda         __x0, Y
/* @169 */ 	sta         (__i0), Y
/* @170 */ 	dey         
/* @171 */ 	bpl         .FixedAdd64_label_167
/* @173 */ 	ldx          #11
	jsr          __var_value8_x1			// carry_out
/* @175 */ 	lda         #__x1
/* @177 */ 	jsr         __result8
.FixedAdd64_label_178:
/* @179 */ 	ldy          #16
	jsr          __leave_leaf
/* @180 */ 	rts         
.func_end_FixedAdd64:
	.size FixedAdd64, .func_end_FixedAdd64-FixedAdd64

	.global FixedAdd256
	.type FixedAdd256, @function

FixedAdd256:
/* @6 */ 	ldx          #7
	jsr          __enter
	.byte        0x03,0x40,0x00		// Save mask i:3 b:0 l:0 x:2 f:0 
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i5			// a
/* @20 */ 	ldx          #2
	jsr          __arg_value2_i6			// b
/* @22 */ 	lda          #0
/* @23 */ 	sta         __x1
/* @25 */ 	lda          #0
/* @26 */ 	sta         __x1+1
/* @28 */ 	lda          #0
/* @29 */ 	sta         __x1+2
/* @31 */ 	lda          #0
/* @32 */ 	sta         __x1+3
/* @34 */ 	lda          #0
/* @35 */ 	sta         __x1+4
/* @37 */ 	lda          #0
/* @38 */ 	sta         __x1+5
/* @40 */ 	lda          #0
/* @41 */ 	sta         __x1+6
/* @43 */ 	lda          #0
/* @44 */ 	sta         __x1+7
/* @45 */ 	lda          #0
/* @46 */ 	sta         __i4
/* @47 */ 	lda          #0
/* @48 */ 	sta         __i4+1
.FixedAdd256_label_49:
/* @51 */ 	lda         __i4
/* @52 */ 	cmp          #4
/* @53 */ 	lda         __i4+1
/* @54 */ 	sbc          #0
/* @55 */ 	bvc         .FixedAdd256_label_50
/* @57 */ 	eor          #128
.FixedAdd256_label_50:
/* @58 */ 	bpl         .FixedAdd256_label_90
/* @59 */ 	jsr         __pushx1
/* @60 */ 	jsr         __pushi6
/* @61 */ 	jsr         __pushi5
/* @63 */ 	ldx         #__x2
/* @64 */ 	ldy          #0
/* @65 */ 	jsr         FixedAdd64
/* @67 */ 	jsr         __incsp12
/* @70 */ 	ldx          #7
.FixedAdd256_label_71:
/* @73 */ 	lda         __x2, X
/* @74 */ 	sta         __x1, X
/* @75 */ 	dex         
/* @76 */ 	bpl         .FixedAdd256_label_71
.FixedAdd256_label_77:
/* @78 */ 	lda          #__i5
/* @80 */ 	ldx          #8
/* @82 */ 	jsr         __rinc2
/* @83 */ 	lda          #__i6
/* @84 */ 	ldx          #8
/* @85 */ 	jsr         __rinc2
/* @86 */ 	lda          #__i4
/* @88 */ 	jsr         __rinc21
/* @89 */ 	bra         .FixedAdd256_label_49
.FixedAdd256_label_90:
/* @91 */ 	ldy          #10
	jsr          __leave_void
/* @92 */ 	rts         
.func_end_FixedAdd256:
	.size FixedAdd256, .func_end_FixedAdd256-FixedAdd256

	.global FixedAdd128
	.type FixedAdd128, @function

FixedAdd128:
/* @6 */ 	ldx          #7
	jsr          __enter
	.byte        0x03,0x40,0x00		// Save mask i:3 b:0 l:0 x:2 f:0 
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i5			// a
/* @20 */ 	ldx          #2
	jsr          __arg_value2_i6			// b
/* @22 */ 	lda          #0
/* @23 */ 	sta         __x1
/* @25 */ 	lda          #0
/* @26 */ 	sta         __x1+1
/* @28 */ 	lda          #0
/* @29 */ 	sta         __x1+2
/* @31 */ 	lda          #0
/* @32 */ 	sta         __x1+3
/* @34 */ 	lda          #0
/* @35 */ 	sta         __x1+4
/* @37 */ 	lda          #0
/* @38 */ 	sta         __x1+5
/* @40 */ 	lda          #0
/* @41 */ 	sta         __x1+6
/* @43 */ 	lda          #0
/* @44 */ 	sta         __x1+7
/* @45 */ 	lda          #0
/* @46 */ 	sta         __i4
/* @47 */ 	lda          #0
/* @48 */ 	sta         __i4+1
.FixedAdd128_label_49:
/* @51 */ 	lda         __i4
/* @52 */ 	cmp          #2
/* @53 */ 	lda         __i4+1
/* @54 */ 	sbc          #0
/* @55 */ 	bvc         .FixedAdd128_label_50
/* @57 */ 	eor          #128
.FixedAdd128_label_50:
/* @58 */ 	bpl         .FixedAdd128_label_90
/* @59 */ 	jsr         __pushx1
/* @60 */ 	jsr         __pushi6
/* @61 */ 	jsr         __pushi5
/* @63 */ 	ldx         #__x2
/* @64 */ 	ldy          #0
/* @65 */ 	jsr         FixedAdd64
/* @67 */ 	jsr         __incsp12
/* @70 */ 	ldx          #7
.FixedAdd128_label_71:
/* @73 */ 	lda         __x2, X
/* @74 */ 	sta         __x1, X
/* @75 */ 	dex         
/* @76 */ 	bpl         .FixedAdd128_label_71
.FixedAdd128_label_77:
/* @78 */ 	lda          #__i5
/* @80 */ 	ldx          #8
/* @82 */ 	jsr         __rinc2
/* @83 */ 	lda          #__i6
/* @84 */ 	ldx          #8
/* @85 */ 	jsr         __rinc2
/* @86 */ 	lda          #__i4
/* @88 */ 	jsr         __rinc21
/* @89 */ 	bra         .FixedAdd128_label_49
.FixedAdd128_label_90:
/* @91 */ 	ldy          #10
	jsr          __leave_void
/* @92 */ 	rts         
.func_end_FixedAdd128:
	.size FixedAdd128, .func_end_FixedAdd128-FixedAdd128

	.global FixedROL64
	.type FixedROL64, @function

FixedROL64:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x00,0x60,0x00		// Save mask i:0 b:0 l:0 x:3 f:0 
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @22 */ 	ldy          #7
.FixedROL64_label_23:
/* @25 */ 	lda         (__i0), Y
/* @26 */ 	sta         __x1, Y
/* @27 */ 	dey         
/* @28 */ 	bpl         .FixedROL64_label_23
/* @33 */ 	lda          #0
/* @34 */ 	sta         __x2
/* @35 */ 	lda          #0
/* @37 */ 	sta         __x2+1
/* @38 */ 	lda          #0
/* @40 */ 	sta         __x2+2
/* @41 */ 	lda          #0
/* @43 */ 	sta         __x2+3
/* @44 */ 	lda          #0
/* @46 */ 	sta         __x2+4
/* @47 */ 	lda          #0
/* @49 */ 	sta         __x2+5
/* @50 */ 	lda          #0
/* @52 */ 	sta         __x2+6
/* @53 */ 	lda         __x1+7
/* @54 */ 	and          #128
/* @55 */ 	sta         __x2+7
/* @61 */ 	ldx          #1
/* @62 */ 	lda         __x2
/* @63 */ 	ora         __x2+1
/* @64 */ 	ora         __x2+2
/* @65 */ 	ora         __x2+3
/* @66 */ 	ora         __x2+4
/* @67 */ 	ora         __x2+5
/* @68 */ 	ora         __x2+6
/* @69 */ 	ora         __x2+7
/* @70 */ 	cmp          #0
/* @71 */ 	bne         .FixedROL64_label_60
.FixedROL64_label_59:
/* @72 */ 	dex         
.FixedROL64_label_60:
/* @73 */ 	stx         __b0
/* @76 */ 	lda         __b0
/* @77 */ 	sta         __x0
/* @78 */ 	lda          #0
/* @79 */ 	sta         __x0+1
/* @80 */ 	sta         __x0+2
/* @81 */ 	sta         __x0+3
/* @82 */ 	sta         __x0+4
/* @83 */ 	sta         __x0+5
/* @84 */ 	sta         __x0+6
/* @85 */ 	sta         __x0+7
/* @88 */ 	lda         __i0
/* @89 */ 	sta         __i1
/* @90 */ 	lda         __i0+1
/* @91 */ 	sta         __i1+1
/* @96 */ 	ldy          #7
.FixedROL64_label_97:
/* @98 */ 	lda         (__i1), Y
/* @99 */ 	sta         __x1, Y
/* @100 */ 	dey         
/* @101 */ 	bpl         .FixedROL64_label_97
/* @105 */ 	lda         __x1
/* @106 */ 	asl          A
/* @107 */ 	sta         __x2
/* @108 */ 	lda         __x1+1
/* @109 */ 	rol          A
/* @110 */ 	sta         __x2+1
/* @111 */ 	lda         __x1+2
/* @112 */ 	rol          A
/* @113 */ 	sta         __x2+2
/* @114 */ 	lda         __x1+3
/* @115 */ 	rol          A
/* @116 */ 	sta         __x2+3
/* @117 */ 	lda         __x1+4
/* @118 */ 	rol          A
/* @119 */ 	sta         __x2+4
/* @120 */ 	lda         __x1+5
/* @121 */ 	rol          A
/* @122 */ 	sta         __x2+5
/* @123 */ 	lda         __x1+6
/* @124 */ 	rol          A
/* @125 */ 	sta         __x2+6
/* @126 */ 	lda         __x1+7
/* @127 */ 	rol          A
/* @128 */ 	sta         __x2+7
/* @133 */ 	ldy          #7
.FixedROL64_label_134:
/* @135 */ 	lda         __x2, Y
/* @136 */ 	sta         (__i1), Y
/* @137 */ 	dey         
/* @138 */ 	bpl         .FixedROL64_label_134
/* @140 */ 	ldx          #2
	jsr          __arg_value8_x1			// carry_in
/* @143 */ 	lda         __i0
/* @144 */ 	sta         __i1
/* @145 */ 	lda         __i0+1
/* @146 */ 	sta         __i1+1
/* @151 */ 	ldy          #7
.FixedROL64_label_152:
/* @153 */ 	lda         (__i1), Y
/* @154 */ 	sta         __x2, Y
/* @155 */ 	dey         
/* @156 */ 	bpl         .FixedROL64_label_152
/* @161 */ 	lda         __x2
/* @162 */ 	ora         __x1
/* @163 */ 	sta         __x3
/* @164 */ 	lda         __x2+1
/* @165 */ 	ora         __x1+1
/* @166 */ 	sta         __x3+1
/* @167 */ 	lda         __x2+2
/* @168 */ 	ora         __x1+2
/* @169 */ 	sta         __x3+2
/* @170 */ 	lda         __x2+3
/* @171 */ 	ora         __x1+3
/* @172 */ 	sta         __x3+3
/* @173 */ 	lda         __x2+4
/* @174 */ 	ora         __x1+4
/* @175 */ 	sta         __x3+4
/* @176 */ 	lda         __x2+5
/* @177 */ 	ora         __x1+5
/* @178 */ 	sta         __x3+5
/* @179 */ 	lda         __x2+6
/* @180 */ 	ora         __x1+6
/* @181 */ 	sta         __x3+6
/* @182 */ 	lda         __x2+7
/* @183 */ 	ora         __x1+7
/* @184 */ 	sta         __x3+7
/* @189 */ 	ldy          #7
.FixedROL64_label_190:
/* @191 */ 	lda         __x3, Y
/* @192 */ 	sta         (__i1), Y
/* @193 */ 	dey         
/* @194 */ 	bpl         .FixedROL64_label_190
/* @195 */ 	lda         #__x0
/* @197 */ 	jsr         __result8
.FixedROL64_label_198:
/* @199 */ 	ldy          #8
	jsr          __leave_leaf
/* @200 */ 	rts         
.func_end_FixedROL64:
	.size FixedROL64, .func_end_FixedROL64-FixedROL64

	.global FixedASL64
	.type FixedASL64, @function

FixedASL64:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x00,0x40,0x00		// Save mask i:0 b:0 l:0 x:2 f:0 
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @20 */ 	ldy          #7
.FixedASL64_label_21:
/* @23 */ 	lda         (__i0), Y
/* @24 */ 	sta         __x1, Y
/* @25 */ 	dey         
/* @26 */ 	bpl         .FixedASL64_label_21
/* @31 */ 	lda          #0
/* @32 */ 	sta         __x2
/* @33 */ 	lda          #0
/* @35 */ 	sta         __x2+1
/* @36 */ 	lda          #0
/* @38 */ 	sta         __x2+2
/* @39 */ 	lda          #0
/* @41 */ 	sta         __x2+3
/* @42 */ 	lda          #0
/* @44 */ 	sta         __x2+4
/* @45 */ 	lda          #0
/* @47 */ 	sta         __x2+5
/* @48 */ 	lda          #0
/* @50 */ 	sta         __x2+6
/* @51 */ 	lda         __x1+7
/* @52 */ 	and          #128
/* @53 */ 	sta         __x2+7
/* @59 */ 	ldx          #1
/* @60 */ 	lda         __x2
/* @61 */ 	ora         __x2+1
/* @62 */ 	ora         __x2+2
/* @63 */ 	ora         __x2+3
/* @64 */ 	ora         __x2+4
/* @65 */ 	ora         __x2+5
/* @66 */ 	ora         __x2+6
/* @67 */ 	ora         __x2+7
/* @68 */ 	cmp          #0
/* @69 */ 	bne         .FixedASL64_label_58
.FixedASL64_label_57:
/* @70 */ 	dex         
.FixedASL64_label_58:
/* @71 */ 	stx         __b0
/* @74 */ 	lda         __b0
/* @75 */ 	sta         __x0
/* @76 */ 	lda          #0
/* @77 */ 	sta         __x0+1
/* @78 */ 	sta         __x0+2
/* @79 */ 	sta         __x0+3
/* @80 */ 	sta         __x0+4
/* @81 */ 	sta         __x0+5
/* @82 */ 	sta         __x0+6
/* @83 */ 	sta         __x0+7
/* @86 */ 	lda         __i0
/* @87 */ 	sta         __i1
/* @88 */ 	lda         __i0+1
/* @89 */ 	sta         __i1+1
/* @94 */ 	ldy          #7
.FixedASL64_label_95:
/* @96 */ 	lda         (__i1), Y
/* @97 */ 	sta         __x1, Y
/* @98 */ 	dey         
/* @99 */ 	bpl         .FixedASL64_label_95
/* @103 */ 	lda         __x1
/* @104 */ 	asl          A
/* @105 */ 	sta         __x2
/* @106 */ 	lda         __x1+1
/* @107 */ 	rol          A
/* @108 */ 	sta         __x2+1
/* @109 */ 	lda         __x1+2
/* @110 */ 	rol          A
/* @111 */ 	sta         __x2+2
/* @112 */ 	lda         __x1+3
/* @113 */ 	rol          A
/* @114 */ 	sta         __x2+3
/* @115 */ 	lda         __x1+4
/* @116 */ 	rol          A
/* @117 */ 	sta         __x2+4
/* @118 */ 	lda         __x1+5
/* @119 */ 	rol          A
/* @120 */ 	sta         __x2+5
/* @121 */ 	lda         __x1+6
/* @122 */ 	rol          A
/* @123 */ 	sta         __x2+6
/* @124 */ 	lda         __x1+7
/* @125 */ 	rol          A
/* @126 */ 	sta         __x2+7
/* @131 */ 	ldy          #7
.FixedASL64_label_132:
/* @133 */ 	lda         __x2, Y
/* @134 */ 	sta         (__i1), Y
/* @135 */ 	dey         
/* @136 */ 	bpl         .FixedASL64_label_132
/* @137 */ 	lda         #__x0
/* @139 */ 	jsr         __result8
.FixedASL64_label_140:
/* @141 */ 	ldy          #8
	jsr          __leave_leaf
/* @142 */ 	rts         
.func_end_FixedASL64:
	.size FixedASL64, .func_end_FixedASL64-FixedASL64

	.global FixedROR64
	.type FixedROR64, @function

FixedROR64:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x00,0x60,0x00		// Save mask i:0 b:0 l:0 x:3 f:0 
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @22 */ 	ldy          #7
.FixedROR64_label_23:
/* @25 */ 	lda         (__i0), Y
/* @26 */ 	sta         __x1, Y
/* @27 */ 	dey         
/* @28 */ 	bpl         .FixedROR64_label_23
/* @33 */ 	lda         __x1
/* @34 */ 	and          #1
/* @35 */ 	sta         __x2
/* @36 */ 	lda          #0
/* @38 */ 	sta         __x2+1
/* @39 */ 	lda          #0
/* @41 */ 	sta         __x2+2
/* @42 */ 	lda          #0
/* @44 */ 	sta         __x2+3
/* @45 */ 	lda          #0
/* @47 */ 	sta         __x2+4
/* @48 */ 	lda          #0
/* @50 */ 	sta         __x2+5
/* @51 */ 	lda          #0
/* @53 */ 	sta         __x2+6
/* @54 */ 	lda          #0
/* @55 */ 	sta         __x2+7
/* @61 */ 	ldx          #1
/* @62 */ 	lda         __x2
/* @63 */ 	ora         __x2+1
/* @64 */ 	ora         __x2+2
/* @65 */ 	ora         __x2+3
/* @66 */ 	ora         __x2+4
/* @67 */ 	ora         __x2+5
/* @68 */ 	ora         __x2+6
/* @69 */ 	ora         __x2+7
/* @70 */ 	cmp          #0
/* @71 */ 	bne         .FixedROR64_label_60
.FixedROR64_label_59:
/* @72 */ 	dex         
.FixedROR64_label_60:
/* @73 */ 	stx         __b0
/* @76 */ 	lda         __b0
/* @77 */ 	sta         __x0
/* @78 */ 	lda          #0
/* @79 */ 	sta         __x0+1
/* @80 */ 	sta         __x0+2
/* @81 */ 	sta         __x0+3
/* @82 */ 	sta         __x0+4
/* @83 */ 	sta         __x0+5
/* @84 */ 	sta         __x0+6
/* @85 */ 	sta         __x0+7
/* @88 */ 	lda         __i0
/* @89 */ 	sta         __i1
/* @90 */ 	lda         __i0+1
/* @91 */ 	sta         __i1+1
/* @96 */ 	ldy          #7
.FixedROR64_label_97:
/* @98 */ 	lda         (__i1), Y
/* @99 */ 	sta         __x1, Y
/* @100 */ 	dey         
/* @101 */ 	bpl         .FixedROR64_label_97
/* @105 */ 	lda         __x1+7
/* @106 */ 	lsr          A
/* @107 */ 	sta         __x2+7
/* @108 */ 	lda         __x1+6
/* @109 */ 	ror          A
/* @110 */ 	sta         __x2+6
/* @111 */ 	lda         __x1+5
/* @112 */ 	ror          A
/* @113 */ 	sta         __x2+5
/* @114 */ 	lda         __x1+4
/* @115 */ 	ror          A
/* @116 */ 	sta         __x2+4
/* @117 */ 	lda         __x1+3
/* @118 */ 	ror          A
/* @119 */ 	sta         __x2+3
/* @120 */ 	lda         __x1+2
/* @121 */ 	ror          A
/* @122 */ 	sta         __x2+2
/* @123 */ 	lda         __x1+1
/* @124 */ 	ror          A
/* @125 */ 	sta         __x2+1
/* @126 */ 	lda         __x1
/* @127 */ 	ror          A
/* @128 */ 	sta         __x2
/* @133 */ 	ldy          #7
.FixedROR64_label_134:
/* @135 */ 	lda         __x2, Y
/* @136 */ 	sta         (__i1), Y
/* @137 */ 	dey         
/* @138 */ 	bpl         .FixedROR64_label_134
/* @140 */ 	ldx          #2
	jsr          __arg_value8_x1			// carry_in
/* @144 */ 	lda         __x1
/* @145 */ 	sta         __x2+7
/* @146 */ 	lda          #0
/* @147 */ 	sta         __x2
/* @148 */ 	sta         __x2+1
/* @149 */ 	sta         __x2+2
/* @150 */ 	sta         __x2+3
/* @151 */ 	sta         __x2+4
/* @152 */ 	sta         __x2+5
/* @153 */ 	sta         __x2+6
/* @154 */ 	ldx          #7
.FixedROR64_label_155:
/* @156 */ 	asl         __x2
/* @157 */ 	rol         __x2+1
/* @158 */ 	rol         __x2+2
/* @159 */ 	rol         __x2+3
/* @160 */ 	rol         __x2+4
/* @161 */ 	rol         __x2+5
/* @162 */ 	rol         __x2+6
/* @163 */ 	rol         __x2+7
/* @164 */ 	dex         
/* @165 */ 	bne         .FixedROR64_label_155
/* @168 */ 	lda         __i0
/* @169 */ 	sta         __i1
/* @170 */ 	lda         __i0+1
/* @171 */ 	sta         __i1+1
/* @176 */ 	ldy          #7
.FixedROR64_label_177:
/* @178 */ 	lda         (__i1), Y
/* @179 */ 	sta         __x1, Y
/* @180 */ 	dey         
/* @181 */ 	bpl         .FixedROR64_label_177
/* @186 */ 	lda         __x1
/* @187 */ 	ora         __x2
/* @188 */ 	sta         __x3
/* @189 */ 	lda         __x1+1
/* @190 */ 	ora         __x2+1
/* @191 */ 	sta         __x3+1
/* @192 */ 	lda         __x1+2
/* @193 */ 	ora         __x2+2
/* @194 */ 	sta         __x3+2
/* @195 */ 	lda         __x1+3
/* @196 */ 	ora         __x2+3
/* @197 */ 	sta         __x3+3
/* @198 */ 	lda         __x1+4
/* @199 */ 	ora         __x2+4
/* @200 */ 	sta         __x3+4
/* @201 */ 	lda         __x1+5
/* @202 */ 	ora         __x2+5
/* @203 */ 	sta         __x3+5
/* @204 */ 	lda         __x1+6
/* @205 */ 	ora         __x2+6
/* @206 */ 	sta         __x3+6
/* @207 */ 	lda         __x1+7
/* @208 */ 	ora         __x2+7
/* @209 */ 	sta         __x3+7
/* @214 */ 	ldy          #7
.FixedROR64_label_215:
/* @216 */ 	lda         __x3, Y
/* @217 */ 	sta         (__i1), Y
/* @218 */ 	dey         
/* @219 */ 	bpl         .FixedROR64_label_215
/* @220 */ 	lda         #__x0
/* @222 */ 	jsr         __result8
.FixedROR64_label_223:
/* @224 */ 	ldy          #8
	jsr          __leave_leaf
/* @225 */ 	rts         
.func_end_FixedROR64:
	.size FixedROR64, .func_end_FixedROR64-FixedROR64

	.global FixedLSR64
	.type FixedLSR64, @function

FixedLSR64:
/* @5 */ 	stx         __result
/* @7 */ 	sty         __result+1
/* @8 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x00,0x40,0x00		// Save mask i:0 b:0 l:0 x:2 f:0 
/* @15 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @19 */ 	ldy          #7
.FixedLSR64_label_20:
/* @22 */ 	lda         (__i0), Y
/* @23 */ 	sta         __x1, Y
/* @24 */ 	dey         
/* @25 */ 	bpl         .FixedLSR64_label_20
/* @30 */ 	lda         __x1
/* @31 */ 	and          #1
/* @32 */ 	sta         __x2
/* @33 */ 	lda          #0
/* @35 */ 	sta         __x2+1
/* @36 */ 	lda          #0
/* @38 */ 	sta         __x2+2
/* @39 */ 	lda          #0
/* @41 */ 	sta         __x2+3
/* @42 */ 	lda          #0
/* @44 */ 	sta         __x2+4
/* @45 */ 	lda          #0
/* @47 */ 	sta         __x2+5
/* @48 */ 	lda          #0
/* @50 */ 	sta         __x2+6
/* @51 */ 	lda          #0
/* @52 */ 	sta         __x2+7
/* @58 */ 	ldx          #1
/* @59 */ 	lda         __x2
/* @60 */ 	ora         __x2+1
/* @61 */ 	ora         __x2+2
/* @62 */ 	ora         __x2+3
/* @63 */ 	ora         __x2+4
/* @64 */ 	ora         __x2+5
/* @65 */ 	ora         __x2+6
/* @66 */ 	ora         __x2+7
/* @67 */ 	cmp          #0
/* @68 */ 	bne         .FixedLSR64_label_57
.FixedLSR64_label_56:
/* @69 */ 	dex         
.FixedLSR64_label_57:
/* @70 */ 	stx         __b0
/* @73 */ 	lda         __b0
/* @74 */ 	sta         __x0
/* @75 */ 	lda          #0
/* @76 */ 	sta         __x0+1
/* @77 */ 	sta         __x0+2
/* @78 */ 	sta         __x0+3
/* @79 */ 	sta         __x0+4
/* @80 */ 	sta         __x0+5
/* @81 */ 	sta         __x0+6
/* @82 */ 	sta         __x0+7
/* @85 */ 	lda         __i0
/* @86 */ 	sta         __i1
/* @87 */ 	lda         __i0+1
/* @88 */ 	sta         __i1+1
/* @93 */ 	ldy          #7
.FixedLSR64_label_94:
/* @95 */ 	lda         (__i1), Y
/* @96 */ 	sta         __x1, Y
/* @97 */ 	dey         
/* @98 */ 	bpl         .FixedLSR64_label_94
/* @102 */ 	lda         __x1+7
/* @103 */ 	lsr          A
/* @104 */ 	sta         __x2+7
/* @105 */ 	lda         __x1+6
/* @106 */ 	ror          A
/* @107 */ 	sta         __x2+6
/* @108 */ 	lda         __x1+5
/* @109 */ 	ror          A
/* @110 */ 	sta         __x2+5
/* @111 */ 	lda         __x1+4
/* @112 */ 	ror          A
/* @113 */ 	sta         __x2+4
/* @114 */ 	lda         __x1+3
/* @115 */ 	ror          A
/* @116 */ 	sta         __x2+3
/* @117 */ 	lda         __x1+2
/* @118 */ 	ror          A
/* @119 */ 	sta         __x2+2
/* @120 */ 	lda         __x1+1
/* @121 */ 	ror          A
/* @122 */ 	sta         __x2+1
/* @123 */ 	lda         __x1
/* @124 */ 	ror          A
/* @125 */ 	sta         __x2
/* @130 */ 	ldy          #7
.FixedLSR64_label_131:
/* @132 */ 	lda         __x2, Y
/* @133 */ 	sta         (__i1), Y
/* @134 */ 	dey         
/* @135 */ 	bpl         .FixedLSR64_label_131
/* @136 */ 	lda         #__x0
/* @138 */ 	jsr         __result8
.FixedLSR64_label_139:
/* @140 */ 	ldy          #8
	jsr          __leave_leaf
/* @141 */ 	rts         
.func_end_FixedLSR64:
	.size FixedLSR64, .func_end_FixedLSR64-FixedLSR64

	.global FixedLShift128
	.type FixedLShift128, @function

FixedLShift128:
/* @4 */ 	ldx          #7
	jsr          __enter
	.byte        0x02,0x40,0x00		// Save mask i:2 b:0 l:0 x:2 f:0 
/* @13 */ 	ldx          #0
	jsr          __arg_value2_i4			// a
/* @19 */ 	lda         __i4
/* @20 */ 	sta         __i0
/* @22 */ 	lda         __i4+1
/* @23 */ 	sta         __i0+1
/* @24 */ 	lda          #__i4
/* @26 */ 	ldx          #8
/* @28 */ 	jsr         __rinc2
/* @30 */ 	jsr         __pushi0
/* @32 */ 	ldx         #__x2
/* @33 */ 	ldy          #0
/* @34 */ 	jsr         FixedASL64
/* @36 */ 	jsr         __incsp2
/* @40 */ 	ldx          #7
.FixedLShift128_label_41:
/* @43 */ 	lda         __x2, X
/* @44 */ 	sta         __x1, X
/* @45 */ 	dex         
/* @46 */ 	bpl         .FixedLShift128_label_41
/* @47 */ 	lda          #0
/* @48 */ 	sta         __i5
/* @49 */ 	lda          #0
/* @50 */ 	sta         __i5+1
.FixedLShift128_label_51:
/* @53 */ 	lda         __i5
/* @54 */ 	cmp          #1
/* @55 */ 	lda         __i5+1
/* @56 */ 	sbc          #0
/* @57 */ 	bvc         .FixedLShift128_label_52
/* @59 */ 	eor          #128
.FixedLShift128_label_52:
/* @60 */ 	bpl         .FixedLShift128_label_92
/* @63 */ 	lda         __i4
/* @64 */ 	sta         __i0
/* @65 */ 	lda         __i4+1
/* @66 */ 	sta         __i0+1
/* @67 */ 	lda          #__i4
/* @68 */ 	ldx          #8
/* @69 */ 	jsr         __rinc2
/* @70 */ 	jsr         __pushx1
/* @72 */ 	jsr         __pushi0
/* @74 */ 	ldx         #__x2
/* @75 */ 	ldy          #0
/* @76 */ 	jsr         FixedROL64
/* @78 */ 	jsr         __incsp10
/* @81 */ 	ldx          #7
.FixedLShift128_label_82:
/* @83 */ 	lda         __x2, X
/* @84 */ 	sta         __x1, X
/* @85 */ 	dex         
/* @86 */ 	bpl         .FixedLShift128_label_82
.FixedLShift128_label_87:
/* @88 */ 	lda          #__i5
/* @90 */ 	jsr         __rinc21
/* @91 */ 	bra         .FixedLShift128_label_51
.FixedLShift128_label_92:
/* @93 */ 	ldy          #10
	jsr          __leave_void
/* @94 */ 	rts         
.func_end_FixedLShift128:
	.size FixedLShift128, .func_end_FixedLShift128-FixedLShift128

	.global FixedLShift256
	.type FixedLShift256, @function

FixedLShift256:
/* @5 */ 	ldx          #7
	jsr          __enter
	.byte        0x02,0x40,0x00		// Save mask i:2 b:0 l:0 x:2 f:0 
/* @14 */ 	ldx          #0
	jsr          __arg_value2_i4			// a
/* @20 */ 	lda         __i4
/* @21 */ 	sta         __i0
/* @23 */ 	lda         __i4+1
/* @24 */ 	sta         __i0+1
/* @25 */ 	lda          #__i4
/* @27 */ 	ldx          #8
/* @29 */ 	jsr         __rinc2
/* @31 */ 	jsr         __pushi0
/* @33 */ 	ldx         #__x2
/* @34 */ 	ldy          #0
/* @35 */ 	jsr         FixedASL64
/* @37 */ 	jsr         __incsp2
/* @41 */ 	ldx          #7
.FixedLShift256_label_42:
/* @44 */ 	lda         __x2, X
/* @45 */ 	sta         __x1, X
/* @46 */ 	dex         
/* @47 */ 	bpl         .FixedLShift256_label_42
/* @48 */ 	lda          #0
/* @49 */ 	sta         __i5
/* @50 */ 	lda          #0
/* @51 */ 	sta         __i5+1
.FixedLShift256_label_52:
/* @54 */ 	lda         __i5
/* @55 */ 	cmp          #3
/* @56 */ 	lda         __i5+1
/* @57 */ 	sbc          #0
/* @58 */ 	bvc         .FixedLShift256_label_53
/* @60 */ 	eor          #128
.FixedLShift256_label_53:
/* @61 */ 	bpl         .FixedLShift256_label_93
/* @64 */ 	lda         __i4
/* @65 */ 	sta         __i0
/* @66 */ 	lda         __i4+1
/* @67 */ 	sta         __i0+1
/* @68 */ 	lda          #__i4
/* @69 */ 	ldx          #8
/* @70 */ 	jsr         __rinc2
/* @71 */ 	jsr         __pushx1
/* @73 */ 	jsr         __pushi0
/* @75 */ 	ldx         #__x2
/* @76 */ 	ldy          #0
/* @77 */ 	jsr         FixedROL64
/* @79 */ 	jsr         __incsp10
/* @82 */ 	ldx          #7
.FixedLShift256_label_83:
/* @84 */ 	lda         __x2, X
/* @85 */ 	sta         __x1, X
/* @86 */ 	dex         
/* @87 */ 	bpl         .FixedLShift256_label_83
.FixedLShift256_label_88:
/* @89 */ 	lda          #__i5
/* @91 */ 	jsr         __rinc21
/* @92 */ 	bra         .FixedLShift256_label_52
.FixedLShift256_label_93:
/* @94 */ 	ldy          #10
	jsr          __leave_void
/* @95 */ 	rts         
.func_end_FixedLShift256:
	.size FixedLShift256, .func_end_FixedLShift256-FixedLShift256

	.global FixedRShift256
	.type FixedRShift256, @function

FixedRShift256:
/* @6 */ 	ldx          #7
	jsr          __enter
	.byte        0x02,0x40,0x00		// Save mask i:2 b:0 l:0 x:2 f:0 
/* @12 */ 	ldx          #0
	jsr          __arg_value2_i4			// a
/* @20 */ 	lda          #3
/* @22 */ 	sta         __i0
/* @23 */ 	lda          #0
/* @25 */ 	sta         __i0+1
/* @28 */ 	lda          #8
/* @29 */ 	sta         __i1
/* @30 */ 	lda          #0
/* @31 */ 	sta         __i1+1
/* @36 */ 	lda          #__i2
/* @37 */ 	ldx          #__i0
/* @38 */ 	ldy          #__i1
/* @40 */ 	jsr         __smul2
/* @42 */ 	clc         
/* @43 */ 	lda         __i4
/* @44 */ 	adc         __i2
/* @45 */ 	sta         __i4
/* @46 */ 	lda         __i4+1
/* @47 */ 	adc         __i2+1
/* @48 */ 	sta         __i4+1
/* @51 */ 	lda         __i4
/* @52 */ 	sta         __i0
/* @53 */ 	lda         __i4+1
/* @54 */ 	sta         __i0+1
/* @55 */ 	lda          #__i4
/* @56 */ 	ldx          #8
/* @58 */ 	jsr         __rdec2
/* @60 */ 	jsr         __pushi0
/* @62 */ 	ldx         #__x2
/* @63 */ 	ldy          #0
/* @64 */ 	jsr         FixedLSR64
/* @66 */ 	jsr         __incsp2
/* @70 */ 	ldx          #7
.FixedRShift256_label_71:
/* @73 */ 	lda         __x2, X
/* @74 */ 	sta         __x1, X
/* @75 */ 	dex         
/* @76 */ 	bpl         .FixedRShift256_label_71
/* @77 */ 	lda          #0
/* @78 */ 	sta         __i5
/* @79 */ 	lda          #0
/* @80 */ 	sta         __i5+1
.FixedRShift256_label_81:
/* @83 */ 	lda         __i5
/* @84 */ 	cmp          #3
/* @85 */ 	lda         __i5+1
/* @86 */ 	sbc          #0
/* @87 */ 	bvc         .FixedRShift256_label_82
/* @89 */ 	eor          #128
.FixedRShift256_label_82:
/* @90 */ 	bpl         .FixedRShift256_label_122
/* @93 */ 	lda         __i4
/* @94 */ 	sta         __i0
/* @95 */ 	lda         __i4+1
/* @96 */ 	sta         __i0+1
/* @97 */ 	lda          #__i4
/* @98 */ 	ldx          #8
/* @99 */ 	jsr         __rdec2
/* @100 */ 	jsr         __pushx1
/* @102 */ 	jsr         __pushi0
/* @104 */ 	ldx         #__x2
/* @105 */ 	ldy          #0
/* @106 */ 	jsr         FixedROR64
/* @108 */ 	jsr         __incsp10
/* @111 */ 	ldx          #7
.FixedRShift256_label_112:
/* @113 */ 	lda         __x2, X
/* @114 */ 	sta         __x1, X
/* @115 */ 	dex         
/* @116 */ 	bpl         .FixedRShift256_label_112
.FixedRShift256_label_117:
/* @118 */ 	lda          #__i5
/* @120 */ 	jsr         __rinc21
/* @121 */ 	bra         .FixedRShift256_label_81
.FixedRShift256_label_122:
/* @123 */ 	ldy          #10
	jsr          __leave_void
/* @124 */ 	rts         
.func_end_FixedRShift256:
	.size FixedRShift256, .func_end_FixedRShift256-FixedRShift256

	.global FixedMultiplyByTen256
	.type FixedMultiplyByTen256, @function

FixedMultiplyByTen256:
/* @6 */ 	ldx          #39
	jsr          __enter
	.byte        0x21,0x00,0x00		// Save mask i:1 b:1 l:0 x:0 f:0 
/* @15 */ 	ldx          #0
	jsr          __arg_value2_i4			// a
/* @19 */ 	ldx          #35
	jsr          __var_addr_i0			// t
/* @21 */ 	lda          #32
/* @23 */ 	sta         __mem_size
/* @24 */ 	lda          #0
/* @26 */ 	sta         __mem_size+1
/* @27 */ 	lda         __i4
/* @29 */ 	sta         __mem_src
/* @30 */ 	lda         __i4+1
/* @32 */ 	sta         __mem_src+1
/* @34 */ 	lda         __i0
/* @36 */ 	sta         __mem_dest
/* @37 */ 	lda         __i0+1
/* @39 */ 	sta         __mem_dest+1
/* @41 */ 	jsr         __builtin_memcpy
/* @42 */ 	lda          #0
/* @43 */ 	sta         __b2
.FixedMultiplyByTen256_label_44:
/* @46 */ 	lda         __b2
/* @47 */ 	sta         __i0
/* @48 */ 	lda          #0
/* @49 */ 	sta         __i0+1
/* @52 */ 	lda         __i0+1
/* @53 */ 	cmp          #0
/* @54 */ 	bcc         .FixedMultiplyByTen256_label_51
/* @55 */ 	bne         .FixedMultiplyByTen256_label_72
/* @56 */ 	lda         __i0
/* @57 */ 	cmp          #3
/* @58 */ 	bcs         .FixedMultiplyByTen256_label_72
.FixedMultiplyByTen256_label_51:
/* @61 */ 	ldx          #35
	jsr          __var_addr_i0			// t
/* @63 */ 	jsr         __pushi0
/* @64 */ 	jsr         FixedLShift256
/* @66 */ 	jsr         __incsp2
.FixedMultiplyByTen256_label_67:
/* @68 */ 	lda          #__b2
/* @70 */ 	jsr         __rinc1
/* @71 */ 	bra         .FixedMultiplyByTen256_label_44
.FixedMultiplyByTen256_label_72:
/* @73 */ 	jsr         __pushi4
/* @74 */ 	jsr         FixedLShift256
/* @75 */ 	jsr         __incsp2
/* @77 */ 	ldx          #35
	jsr          __var_addr_i0			// t
/* @79 */ 	jsr         __pushi0
/* @80 */ 	jsr         __pushi4
/* @81 */ 	jsr         FixedAdd256
/* @83 */ 	jsr         __incsp4
/* @84 */ 	ldy          #42
	jsr          __leave_void
/* @85 */ 	rts         
.func_end_FixedMultiplyByTen256:
	.size FixedMultiplyByTen256, .func_end_FixedMultiplyByTen256-FixedMultiplyByTen256

	.global FixedMultiplyByTen128
	.type FixedMultiplyByTen128, @function

FixedMultiplyByTen128:
/* @6 */ 	ldx          #23
	jsr          __enter
	.byte        0x21,0x00,0x00		// Save mask i:1 b:1 l:0 x:0 f:0 
/* @15 */ 	ldx          #0
	jsr          __arg_value2_i4			// a
/* @19 */ 	ldx          #19
	jsr          __var_addr_i0			// t
/* @21 */ 	lda          #16
/* @23 */ 	sta         __mem_size
/* @24 */ 	lda          #0
/* @26 */ 	sta         __mem_size+1
/* @27 */ 	lda         __i4
/* @29 */ 	sta         __mem_src
/* @30 */ 	lda         __i4+1
/* @32 */ 	sta         __mem_src+1
/* @34 */ 	lda         __i0
/* @36 */ 	sta         __mem_dest
/* @37 */ 	lda         __i0+1
/* @39 */ 	sta         __mem_dest+1
/* @41 */ 	jsr         __builtin_memcpy
/* @42 */ 	lda          #0
/* @43 */ 	sta         __b2
.FixedMultiplyByTen128_label_44:
/* @46 */ 	lda         __b2
/* @47 */ 	sta         __i0
/* @48 */ 	lda          #0
/* @49 */ 	sta         __i0+1
/* @52 */ 	lda         __i0+1
/* @53 */ 	cmp          #0
/* @54 */ 	bcc         .FixedMultiplyByTen128_label_51
/* @55 */ 	bne         .FixedMultiplyByTen128_label_72
/* @56 */ 	lda         __i0
/* @57 */ 	cmp          #3
/* @58 */ 	bcs         .FixedMultiplyByTen128_label_72
.FixedMultiplyByTen128_label_51:
/* @61 */ 	ldx          #19
	jsr          __var_addr_i0			// t
/* @63 */ 	jsr         __pushi0
/* @64 */ 	jsr         FixedLShift128
/* @66 */ 	jsr         __incsp2
.FixedMultiplyByTen128_label_67:
/* @68 */ 	lda          #__b2
/* @70 */ 	jsr         __rinc1
/* @71 */ 	bra         .FixedMultiplyByTen128_label_44
.FixedMultiplyByTen128_label_72:
/* @73 */ 	jsr         __pushi4
/* @74 */ 	jsr         FixedLShift128
/* @75 */ 	jsr         __incsp2
/* @77 */ 	ldx          #19
	jsr          __var_addr_i0			// t
/* @79 */ 	jsr         __pushi0
/* @80 */ 	jsr         __pushi4
/* @81 */ 	jsr         FixedAdd128
/* @83 */ 	jsr         __incsp4
/* @84 */ 	ldy          #26
	jsr          __leave_void
/* @85 */ 	rts         
.func_end_FixedMultiplyByTen128:
	.size FixedMultiplyByTen128, .func_end_FixedMultiplyByTen128-FixedMultiplyByTen128

	.global FixedDivMod64ByTen64
	.type FixedDivMod64ByTen64, @function

FixedDivMod64ByTen64:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x00,0x40,0x00		// Save mask i:0 b:0 l:0 x:2 f:0 
/* @14 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @18 */ 	ldy          #7
.FixedDivMod64ByTen64_label_19:
/* @21 */ 	lda         (__i0), Y
/* @22 */ 	sta         __x0, Y
/* @23 */ 	dey         
/* @24 */ 	bpl         .FixedDivMod64ByTen64_label_19
/* @28 */ 	lda          #10
/* @30 */ 	sta         __x1
/* @31 */ 	lda          #0
/* @33 */ 	sta         __x1+1
/* @34 */ 	lda          #0
/* @36 */ 	sta         __x1+2
/* @37 */ 	lda          #0
/* @39 */ 	sta         __x1+3
/* @40 */ 	lda          #0
/* @42 */ 	sta         __x1+4
/* @43 */ 	lda          #0
/* @45 */ 	sta         __x1+5
/* @46 */ 	lda          #0
/* @48 */ 	sta         __x1+6
/* @49 */ 	lda          #0
/* @50 */ 	sta         __x1+7
/* @55 */ 	lda          #__x2
/* @56 */ 	ldx          #__x0
/* @57 */ 	ldy          #__x1
/* @59 */ 	jsr         __umod8
/* @62 */ 	lda         __x2
/* @63 */ 	sta         __b0
/* @66 */ 	lda         __i0
/* @67 */ 	sta         __i1
/* @68 */ 	lda         __i0+1
/* @69 */ 	sta         __i1+1
/* @74 */ 	ldy          #7
.FixedDivMod64ByTen64_label_75:
/* @76 */ 	lda         (__i1), Y
/* @77 */ 	sta         __x0, Y
/* @78 */ 	dey         
/* @79 */ 	bpl         .FixedDivMod64ByTen64_label_75
/* @83 */ 	lda          #10
/* @84 */ 	sta         __x1
/* @85 */ 	lda          #0
/* @86 */ 	sta         __x1+1
/* @87 */ 	lda          #0
/* @88 */ 	sta         __x1+2
/* @89 */ 	lda          #0
/* @90 */ 	sta         __x1+3
/* @91 */ 	lda          #0
/* @92 */ 	sta         __x1+4
/* @93 */ 	lda          #0
/* @94 */ 	sta         __x1+5
/* @95 */ 	lda          #0
/* @96 */ 	sta         __x1+6
/* @97 */ 	lda          #0
/* @98 */ 	sta         __x1+7
/* @103 */ 	lda          #__x2
/* @104 */ 	ldx          #__x0
/* @105 */ 	ldy          #__x1
/* @107 */ 	jsr         __udiv8
/* @112 */ 	ldy          #7
.FixedDivMod64ByTen64_label_113:
/* @114 */ 	lda         __x2, Y
/* @115 */ 	sta         (__i1), Y
/* @116 */ 	dey         
/* @117 */ 	bpl         .FixedDivMod64ByTen64_label_113
/* @118 */ 	lda         #__b0
/* @120 */ 	jsr         __result1
.FixedDivMod64ByTen64_label_121:
/* @122 */ 	ldy          #8
	jsr          __leave_leaf
/* @123 */ 	rts         
.func_end_FixedDivMod64ByTen64:
	.size FixedDivMod64ByTen64, .func_end_FixedDivMod64ByTen64-FixedDivMod64ByTen64

	.global FixedIsZeroUpper128
	.type FixedIsZeroUpper128, @function

FixedIsZeroUpper128:
/* @9 */ 	stx         __result
/* @11 */ 	sty         __result+1
/* @12 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @19 */ 	ldx          #0
	jsr          __arg_value2_i1			// a
/* @24 */ 	clc         
/* @25 */ 	lda         __i1
/* @26 */ 	adc          #16
/* @27 */ 	sta         __i2
/* @28 */ 	lda         __i1+1
/* @29 */ 	adc          #0
/* @30 */ 	sta         __i2+1
/* @32 */ 	lda         __i2
/* @33 */ 	sta         __i0
/* @34 */ 	lda         __i2+1
/* @35 */ 	sta         __i0+1
/* @36 */ 	lda          #0
/* @37 */ 	sta         __b0
.FixedIsZeroUpper128_label_38:
/* @40 */ 	lda         __b0
/* @41 */ 	sta         __i1
/* @42 */ 	lda          #0
/* @43 */ 	sta         __i1+1
/* @46 */ 	lda         __i1+1
/* @47 */ 	cmp          #0
/* @48 */ 	bcc         .FixedIsZeroUpper128_label_45
/* @49 */ 	bne         .FixedIsZeroUpper128_label_108
/* @50 */ 	lda         __i1
/* @51 */ 	cmp          #2
/* @52 */ 	bcs         .FixedIsZeroUpper128_label_108
.FixedIsZeroUpper128_label_45:
/* @56 */ 	lda         __i0
/* @57 */ 	sta         __i1
/* @58 */ 	lda         __i0+1
/* @59 */ 	sta         __i1+1
/* @60 */ 	lda          #__i0
/* @62 */ 	ldx          #8
/* @64 */ 	jsr         __rinc2
/* @70 */ 	ldy          #7
.FixedIsZeroUpper128_label_71:
/* @73 */ 	lda         (__i1), Y
/* @74 */ 	sta         __x0, Y
/* @75 */ 	dey         
/* @76 */ 	bpl         .FixedIsZeroUpper128_label_71
/* @78 */ 	lda         __x0
/* @79 */ 	ora         __x0+1
/* @81 */ 	ora         __x0+2
/* @83 */ 	ora         __x0+3
/* @85 */ 	ora         __x0+4
/* @87 */ 	ora         __x0+5
/* @89 */ 	ora         __x0+6
/* @90 */ 	ora         __x0+7
/* @91 */ 	cmp          #0
/* @92 */ 	beq         .FixedIsZeroUpper128_label_102
/* @94 */ 	lda          #0
/* @95 */ 	sta         __b1
/* @96 */ 	lda         #__b1
/* @98 */ 	jsr         __result1
.FixedIsZeroUpper128_label_99:
/* @100 */ 	ldy          #8
	jsr          __leave_leaf_nomask
/* @101 */ 	rts         
.FixedIsZeroUpper128_label_102:
.FixedIsZeroUpper128_label_103:
/* @104 */ 	lda          #__b0
/* @106 */ 	jsr         __rinc1
/* @107 */ 	bra         .FixedIsZeroUpper128_label_38
.FixedIsZeroUpper128_label_108:
/* @110 */ 	lda          #1
/* @111 */ 	sta         __b1
/* @112 */ 	lda         #__b1
/* @113 */ 	jsr         __result1
/* @114 */ 	bra         .FixedIsZeroUpper128_label_99
.func_end_FixedIsZeroUpper128:
	.size FixedIsZeroUpper128, .func_end_FixedIsZeroUpper128-FixedIsZeroUpper128

	.global FixedIsOne128
	.type FixedIsOne128, @function

FixedIsOne128:
/* @9 */ 	stx         __result
/* @11 */ 	sty         __result+1
/* @12 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @22 */ 	lda         __i0
/* @23 */ 	sta         __i1
/* @24 */ 	lda         __i0+1
/* @25 */ 	sta         __i1+1
/* @26 */ 	lda          #__i0
/* @28 */ 	ldx          #8
/* @30 */ 	jsr         __rinc2
/* @36 */ 	ldy          #7
.FixedIsOne128_label_37:
/* @39 */ 	lda         (__i1), Y
/* @40 */ 	sta         __x0, Y
/* @41 */ 	dey         
/* @42 */ 	bpl         .FixedIsOne128_label_37
/* @45 */ 	lda         __x0
/* @46 */ 	cmp          #1
/* @47 */ 	bne         .FixedIsOne128_label_44
/* @48 */ 	lda         __x0+1
/* @49 */ 	cmp          #0
/* @50 */ 	bne         .FixedIsOne128_label_44
/* @52 */ 	lda         __x0+2
/* @53 */ 	cmp          #0
/* @54 */ 	bne         .FixedIsOne128_label_44
/* @56 */ 	lda         __x0+3
/* @57 */ 	cmp          #0
/* @58 */ 	bne         .FixedIsOne128_label_44
/* @60 */ 	lda         __x0+4
/* @61 */ 	cmp          #0
/* @62 */ 	bne         .FixedIsOne128_label_44
/* @64 */ 	lda         __x0+5
/* @65 */ 	cmp          #0
/* @66 */ 	bne         .FixedIsOne128_label_44
/* @68 */ 	lda         __x0+6
/* @69 */ 	cmp          #0
/* @70 */ 	bne         .FixedIsOne128_label_44
/* @71 */ 	lda         __x0+7
/* @72 */ 	cmp          #0
/* @73 */ 	beq         .FixedIsOne128_label_84
.FixedIsOne128_label_44:
/* @76 */ 	lda          #0
/* @77 */ 	sta         __b1
/* @78 */ 	lda         #__b1
/* @80 */ 	jsr         __result1
.FixedIsOne128_label_81:
/* @82 */ 	ldy          #8
	jsr          __leave_leaf_nomask
/* @83 */ 	rts         
.FixedIsOne128_label_84:
/* @85 */ 	lda          #1
/* @86 */ 	sta         __b0
.FixedIsOne128_label_87:
/* @89 */ 	lda         __b0
/* @90 */ 	sta         __i1
/* @91 */ 	lda          #0
/* @92 */ 	sta         __i1+1
/* @95 */ 	lda         __i1+1
/* @96 */ 	cmp          #0
/* @97 */ 	bcc         .FixedIsOne128_label_94
/* @98 */ 	bne         .FixedIsOne128_label_145
/* @99 */ 	lda         __i1
/* @100 */ 	cmp          #2
/* @101 */ 	bcs         .FixedIsOne128_label_145
.FixedIsOne128_label_94:
/* @105 */ 	lda         __i0
/* @106 */ 	sta         __i1
/* @107 */ 	lda         __i0+1
/* @108 */ 	sta         __i1+1
/* @109 */ 	lda          #__i0
/* @110 */ 	ldx          #8
/* @111 */ 	jsr         __rinc2
/* @116 */ 	ldy          #7
.FixedIsOne128_label_117:
/* @118 */ 	lda         (__i1), Y
/* @119 */ 	sta         __x0, Y
/* @120 */ 	dey         
/* @121 */ 	bpl         .FixedIsOne128_label_117
/* @123 */ 	lda         __x0
/* @124 */ 	ora         __x0+1
/* @125 */ 	ora         __x0+2
/* @126 */ 	ora         __x0+3
/* @127 */ 	ora         __x0+4
/* @128 */ 	ora         __x0+5
/* @129 */ 	ora         __x0+6
/* @130 */ 	ora         __x0+7
/* @131 */ 	cmp          #0
/* @132 */ 	beq         .FixedIsOne128_label_139
/* @134 */ 	lda          #0
/* @135 */ 	sta         __b1
/* @136 */ 	lda         #__b1
/* @137 */ 	jsr         __result1
/* @138 */ 	bra         .FixedIsOne128_label_81
.FixedIsOne128_label_139:
.FixedIsOne128_label_140:
/* @141 */ 	lda          #__b0
/* @143 */ 	jsr         __rinc1
/* @144 */ 	bra         .FixedIsOne128_label_87
.FixedIsOne128_label_145:
/* @147 */ 	lda          #1
/* @148 */ 	sta         __b1
/* @149 */ 	lda         #__b1
/* @150 */ 	jsr         __result1
/* @151 */ 	bra         .FixedIsOne128_label_81
.func_end_FixedIsOne128:
	.size FixedIsOne128, .func_end_FixedIsOne128-FixedIsOne128

	.global FixedIsZero128
	.type FixedIsZero128, @function

FixedIsZero128:
/* @8 */ 	stx         __result
/* @10 */ 	sty         __result+1
/* @11 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i1			// v
/* @21 */ 	lda         __i1
/* @22 */ 	sta         __i0
/* @23 */ 	lda         __i1+1
/* @24 */ 	sta         __i0+1
/* @25 */ 	lda          #0
/* @26 */ 	sta         __b0
.FixedIsZero128_label_27:
/* @29 */ 	lda         __b0
/* @30 */ 	sta         __i1
/* @31 */ 	lda          #0
/* @32 */ 	sta         __i1+1
/* @35 */ 	lda         __i1+1
/* @36 */ 	cmp          #0
/* @37 */ 	bcc         .FixedIsZero128_label_34
/* @38 */ 	bne         .FixedIsZero128_label_97
/* @39 */ 	lda         __i1
/* @40 */ 	cmp          #2
/* @41 */ 	bcs         .FixedIsZero128_label_97
.FixedIsZero128_label_34:
/* @45 */ 	lda         __i0
/* @46 */ 	sta         __i1
/* @47 */ 	lda         __i0+1
/* @48 */ 	sta         __i1+1
/* @49 */ 	lda          #__i0
/* @51 */ 	ldx          #8
/* @53 */ 	jsr         __rinc2
/* @59 */ 	ldy          #7
.FixedIsZero128_label_60:
/* @62 */ 	lda         (__i1), Y
/* @63 */ 	sta         __x0, Y
/* @64 */ 	dey         
/* @65 */ 	bpl         .FixedIsZero128_label_60
/* @67 */ 	lda         __x0
/* @68 */ 	ora         __x0+1
/* @70 */ 	ora         __x0+2
/* @72 */ 	ora         __x0+3
/* @74 */ 	ora         __x0+4
/* @76 */ 	ora         __x0+5
/* @78 */ 	ora         __x0+6
/* @79 */ 	ora         __x0+7
/* @80 */ 	cmp          #0
/* @81 */ 	beq         .FixedIsZero128_label_91
/* @83 */ 	lda          #0
/* @84 */ 	sta         __b1
/* @85 */ 	lda         #__b1
/* @87 */ 	jsr         __result1
.FixedIsZero128_label_88:
/* @89 */ 	ldy          #8
	jsr          __leave_leaf_nomask
/* @90 */ 	rts         
.FixedIsZero128_label_91:
.FixedIsZero128_label_92:
/* @93 */ 	lda          #__b0
/* @95 */ 	jsr         __rinc1
/* @96 */ 	bra         .FixedIsZero128_label_27
.FixedIsZero128_label_97:
/* @99 */ 	lda          #1
/* @100 */ 	sta         __b1
/* @101 */ 	lda         #__b1
/* @102 */ 	jsr         __result1
/* @103 */ 	bra         .FixedIsZero128_label_88
.func_end_FixedIsZero128:
	.size FixedIsZero128, .func_end_FixedIsZero128-FixedIsZero128

	.global FixedIsZero256
	.type FixedIsZero256, @function

FixedIsZero256:
/* @8 */ 	stx         __result
/* @10 */ 	sty         __result+1
/* @11 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i1			// v
/* @21 */ 	lda         __i1
/* @22 */ 	sta         __i0
/* @23 */ 	lda         __i1+1
/* @24 */ 	sta         __i0+1
/* @25 */ 	lda          #0
/* @26 */ 	sta         __b0
.FixedIsZero256_label_27:
/* @29 */ 	lda         __b0
/* @30 */ 	sta         __i1
/* @31 */ 	lda          #0
/* @32 */ 	sta         __i1+1
/* @35 */ 	lda         __i1+1
/* @36 */ 	cmp          #0
/* @37 */ 	bcc         .FixedIsZero256_label_34
/* @38 */ 	bne         .FixedIsZero256_label_97
/* @39 */ 	lda         __i1
/* @40 */ 	cmp          #4
/* @41 */ 	bcs         .FixedIsZero256_label_97
.FixedIsZero256_label_34:
/* @45 */ 	lda         __i0
/* @46 */ 	sta         __i1
/* @47 */ 	lda         __i0+1
/* @48 */ 	sta         __i1+1
/* @49 */ 	lda          #__i0
/* @51 */ 	ldx          #8
/* @53 */ 	jsr         __rinc2
/* @59 */ 	ldy          #7
.FixedIsZero256_label_60:
/* @62 */ 	lda         (__i1), Y
/* @63 */ 	sta         __x0, Y
/* @64 */ 	dey         
/* @65 */ 	bpl         .FixedIsZero256_label_60
/* @67 */ 	lda         __x0
/* @68 */ 	ora         __x0+1
/* @70 */ 	ora         __x0+2
/* @72 */ 	ora         __x0+3
/* @74 */ 	ora         __x0+4
/* @76 */ 	ora         __x0+5
/* @78 */ 	ora         __x0+6
/* @79 */ 	ora         __x0+7
/* @80 */ 	cmp          #0
/* @81 */ 	beq         .FixedIsZero256_label_91
/* @83 */ 	lda          #0
/* @84 */ 	sta         __b1
/* @85 */ 	lda         #__b1
/* @87 */ 	jsr         __result1
.FixedIsZero256_label_88:
/* @89 */ 	ldy          #8
	jsr          __leave_leaf_nomask
/* @90 */ 	rts         
.FixedIsZero256_label_91:
.FixedIsZero256_label_92:
/* @93 */ 	lda          #__b0
/* @95 */ 	jsr         __rinc1
/* @96 */ 	bra         .FixedIsZero256_label_27
.FixedIsZero256_label_97:
/* @99 */ 	lda          #1
/* @100 */ 	sta         __b1
/* @101 */ 	lda         #__b1
/* @102 */ 	jsr         __result1
/* @103 */ 	bra         .FixedIsZero256_label_88
.func_end_FixedIsZero256:
	.size FixedIsZero256, .func_end_FixedIsZero256-FixedIsZero256

	.global FixedDivideByTen128
	.type FixedDivideByTen128, @function

FixedDivideByTen128:
/* @13 */ 	stx         __result
/* @15 */ 	sty         __result+1
/* @16 */ 	ldx          #23
	jsr          __enter
	.byte        0x23,0x20,0x00		// Save mask i:3 b:1 l:0 x:1 f:0 
/* @31 */ 	ldx          #0
	jsr          __arg_value2_i5			// a
/* @33 */ 	lda          #16
/* @35 */ 	sta         __mem_size
/* @37 */ 	ldx          #19
	jsr          __var_addr_i1			// quotient
/* @39 */ 	lda         __i1
/* @41 */ 	sta         __mem_dest
/* @42 */ 	lda         __i1+1
/* @44 */ 	sta         __mem_dest+1
/* @46 */ 	jsr         __zeromem1
/* @47 */ 	lda          #0
/* @48 */ 	sta         __b2
/* @49 */ 	lda          #1
/* @50 */ 	sta         __i0
/* @51 */ 	lda          #0
/* @52 */ 	sta         __i0+1
/* @53 */ 	lda          #0
/* @54 */ 	sta         __i4
/* @55 */ 	lda          #0
/* @56 */ 	sta         __i4+1
.FixedDivideByTen128_label_57:
/* @59 */ 	lda         __i4+1
/* @60 */ 	cmp          #0
/* @61 */ 	bcc         .FixedDivideByTen128_label_58
/* @62 */ 	beq         .FixedDivideByTen128_label_212
/* @213 */ 	jmp         .FixedDivideByTen128_label_183
.FixedDivideByTen128_label_212:
/* @63 */ 	lda         __i4
/* @64 */ 	cmp          #128
/* @65 */ 	bcc         .FixedDivideByTen128_label_214
/* @215 */ 	jmp         .FixedDivideByTen128_label_183
.FixedDivideByTen128_label_214:
.FixedDivideByTen128_label_58:
/* @67 */ 	asl         __b2
/* @71 */ 	ldy          #8
/* @72 */ 	lda         (__i5), Y
/* @73 */ 	sta         __x0
/* @75 */ 	ldy          #9
/* @76 */ 	lda         (__i5), Y
/* @77 */ 	sta         __x0+1
/* @78 */ 	ldy          #10
/* @79 */ 	lda         (__i5), Y
/* @81 */ 	sta         __x0+2
/* @83 */ 	ldy          #11
/* @84 */ 	lda         (__i5), Y
/* @86 */ 	sta         __x0+3
/* @88 */ 	ldy          #12
/* @89 */ 	lda         (__i5), Y
/* @91 */ 	sta         __x0+4
/* @93 */ 	ldy          #13
/* @94 */ 	lda         (__i5), Y
/* @96 */ 	sta         __x0+5
/* @98 */ 	ldy          #14
/* @99 */ 	lda         (__i5), Y
/* @101 */ 	sta         __x0+6
/* @103 */ 	ldy          #15
/* @104 */ 	lda         (__i5), Y
/* @106 */ 	sta         __x0+7
/* @110 */ 	lda          #0
/* @111 */ 	sta         __x1
/* @112 */ 	lda          #0
/* @113 */ 	sta         __x1+1
/* @114 */ 	lda          #0
/* @115 */ 	sta         __x1+2
/* @116 */ 	lda          #0
/* @117 */ 	sta         __x1+3
/* @118 */ 	lda          #0
/* @119 */ 	sta         __x1+4
/* @120 */ 	lda          #0
/* @121 */ 	sta         __x1+5
/* @122 */ 	lda          #0
/* @123 */ 	sta         __x1+6
/* @124 */ 	lda         __x0+7
/* @125 */ 	and          #128
/* @126 */ 	sta         __x1+7
/* @128 */ 	lda         __x1
/* @129 */ 	ora         __x1+1
/* @130 */ 	ora         __x1+2
/* @131 */ 	ora         __x1+3
/* @132 */ 	ora         __x1+4
/* @133 */ 	ora         __x1+5
/* @134 */ 	ora         __x1+6
/* @135 */ 	ora         __x1+7
/* @136 */ 	cmp          #0
/* @137 */ 	beq         .FixedDivideByTen128_label_141
/* @138 */ 	lda         __b2
/* @139 */ 	ora          #1
/* @140 */ 	sta         __b2
.FixedDivideByTen128_label_141:
/* @142 */ 	jsr         __pushi5
/* @143 */ 	jsr         FixedLShift128
/* @145 */ 	jsr         __incsp2
/* @147 */ 	ldx          #19
	jsr          __var_addr_i6			// quotient
/* @149 */ 	jsr         __pushi6
/* @150 */ 	jsr         FixedLShift128
/* @151 */ 	jsr         __incsp2
/* @153 */ 	lda         __b2
/* @154 */ 	sta         __i0
/* @155 */ 	lda          #0
/* @156 */ 	sta         __i0+1
/* @159 */ 	lda         __i0+1
/* @160 */ 	cmp          #0
/* @161 */ 	bcc         .FixedDivideByTen128_label_177
/* @162 */ 	bne         .FixedDivideByTen128_label_158
/* @163 */ 	lda         __i0
/* @164 */ 	cmp          #10
/* @165 */ 	bcc         .FixedDivideByTen128_label_177
.FixedDivideByTen128_label_158:
/* @170 */ 	jsr         __pushi6
/* @171 */ 	jsr         FixedIncrement128
/* @172 */ 	jsr         __incsp2
/* @173 */ 	sec         
/* @174 */ 	lda         __b2
/* @175 */ 	sbc          #10
/* @176 */ 	sta         __b2
.FixedDivideByTen128_label_177:
.FixedDivideByTen128_label_178:
/* @179 */ 	lda          #__i4
/* @181 */ 	jsr         __rinc21
/* @182 */ 	jmp         .FixedDivideByTen128_label_57
.FixedDivideByTen128_label_183:
/* @185 */ 	ldx          #19
	jsr          __var_addr_i0			// quotient
/* @187 */ 	lda          #16
/* @188 */ 	sta         __mem_size
/* @189 */ 	lda          #0
/* @191 */ 	sta         __mem_size+1
/* @193 */ 	lda         __i0
/* @195 */ 	sta         __mem_src
/* @196 */ 	lda         __i0+1
/* @198 */ 	sta         __mem_src+1
/* @199 */ 	lda         __i5
/* @200 */ 	sta         __mem_dest
/* @201 */ 	lda         __i5+1
/* @202 */ 	sta         __mem_dest+1
/* @204 */ 	jsr         __builtin_memcpy
/* @205 */ 	ldx          #24
	jsr          __load_result
/* @206 */ 	lda         #__b2
/* @208 */ 	jsr         __result1
.FixedDivideByTen128_label_209:
/* @210 */ 	ldy          #26
	jsr          __leave
/* @211 */ 	rts         
.func_end_FixedDivideByTen128:
	.size FixedDivideByTen128, .func_end_FixedDivideByTen128-FixedDivideByTen128

	.global FixedDivideByTen256
	.type FixedDivideByTen256, @function

FixedDivideByTen256:
/* @16 */ 	stx         __result
/* @18 */ 	sty         __result+1
/* @19 */ 	ldx          #39
	jsr          __enter
	.byte        0x24,0x20,0x00		// Save mask i:4 b:1 l:0 x:1 f:0 
/* @28 */ 	ldx          #0
	jsr          __arg_value2_i4			// a
/* @39 */ 	clc         
/* @40 */ 	lda         __i4
/* @41 */ 	adc          #16
/* @42 */ 	sta         __i0
/* @43 */ 	lda         __i4+1
/* @44 */ 	adc          #0
/* @45 */ 	sta         __i0+1
/* @47 */ 	jsr         __pushi0
/* @49 */ 	ldx         #__b0
/* @50 */ 	ldy          #0
/* @51 */ 	jsr         FixedIsZero128
/* @53 */ 	jsr         __incsp2
/* @55 */ 	lda         __b0
/* @56 */ 	cmp          #0
/* @57 */ 	beq         .FixedDivideByTen256_label_72
/* @58 */ 	jsr         __pushi4
/* @60 */ 	ldx         #__b0
/* @61 */ 	ldy          #0
/* @62 */ 	jsr         FixedDivideByTen128
/* @63 */ 	jsr         __incsp2
/* @65 */ 	ldx          #40
	jsr          __load_result
/* @66 */ 	lda         #__b0
/* @68 */ 	jsr         __result1
.FixedDivideByTen256_label_69:
/* @70 */ 	ldy          #42
	jsr          __leave
/* @71 */ 	rts         
.FixedDivideByTen256_label_72:
/* @74 */ 	lda          #32
/* @76 */ 	sta         __mem_size
/* @78 */ 	ldx          #35
	jsr          __var_addr_i0			// quotient
/* @80 */ 	lda         __i0
/* @82 */ 	sta         __mem_dest
/* @83 */ 	lda         __i0+1
/* @85 */ 	sta         __mem_dest+1
/* @87 */ 	jsr         __zeromem1
/* @88 */ 	lda          #0
/* @89 */ 	sta         __b2
/* @90 */ 	lda          #3
/* @91 */ 	sta         __i5
/* @92 */ 	lda          #0
/* @93 */ 	sta         __i5+1
/* @94 */ 	lda          #0
/* @95 */ 	sta         __i6
/* @96 */ 	lda          #0
/* @97 */ 	sta         __i6+1
.FixedDivideByTen256_label_98:
/* @100 */ 	lda         __i6+1
/* @101 */ 	cmp          #1
/* @102 */ 	bcc         .FixedDivideByTen256_label_99
/* @103 */ 	beq         .FixedDivideByTen256_label_250
/* @251 */ 	jmp         .FixedDivideByTen256_label_224
.FixedDivideByTen256_label_250:
/* @104 */ 	lda         __i6
/* @105 */ 	cmp          #0
/* @106 */ 	bcc         .FixedDivideByTen256_label_252
/* @253 */ 	jmp         .FixedDivideByTen256_label_224
.FixedDivideByTen256_label_252:
.FixedDivideByTen256_label_99:
/* @108 */ 	asl         __b2
/* @112 */ 	ldy          #24
/* @113 */ 	lda         (__i4), Y
/* @114 */ 	sta         __x0
/* @116 */ 	ldy          #25
/* @117 */ 	lda         (__i4), Y
/* @118 */ 	sta         __x0+1
/* @120 */ 	ldy          #26
/* @121 */ 	lda         (__i4), Y
/* @123 */ 	sta         __x0+2
/* @125 */ 	ldy          #27
/* @126 */ 	lda         (__i4), Y
/* @128 */ 	sta         __x0+3
/* @130 */ 	ldy          #28
/* @131 */ 	lda         (__i4), Y
/* @133 */ 	sta         __x0+4
/* @135 */ 	ldy          #29
/* @136 */ 	lda         (__i4), Y
/* @138 */ 	sta         __x0+5
/* @140 */ 	ldy          #30
/* @141 */ 	lda         (__i4), Y
/* @143 */ 	sta         __x0+6
/* @145 */ 	ldy          #31
/* @146 */ 	lda         (__i4), Y
/* @148 */ 	sta         __x0+7
/* @152 */ 	lda          #0
/* @153 */ 	sta         __x1
/* @154 */ 	lda          #0
/* @155 */ 	sta         __x1+1
/* @156 */ 	lda          #0
/* @157 */ 	sta         __x1+2
/* @158 */ 	lda          #0
/* @159 */ 	sta         __x1+3
/* @160 */ 	lda          #0
/* @161 */ 	sta         __x1+4
/* @162 */ 	lda          #0
/* @163 */ 	sta         __x1+5
/* @164 */ 	lda          #0
/* @165 */ 	sta         __x1+6
/* @166 */ 	lda         __x0+7
/* @167 */ 	and          #128
/* @168 */ 	sta         __x1+7
/* @170 */ 	lda         __x1
/* @171 */ 	ora         __x1+1
/* @172 */ 	ora         __x1+2
/* @173 */ 	ora         __x1+3
/* @174 */ 	ora         __x1+4
/* @175 */ 	ora         __x1+5
/* @176 */ 	ora         __x1+6
/* @177 */ 	ora         __x1+7
/* @178 */ 	cmp          #0
/* @179 */ 	beq         .FixedDivideByTen256_label_183
/* @180 */ 	lda         __b2
/* @181 */ 	ora          #1
/* @182 */ 	sta         __b2
.FixedDivideByTen256_label_183:
/* @184 */ 	jsr         __pushi4
/* @185 */ 	jsr         FixedLShift256
/* @186 */ 	jsr         __incsp2
/* @188 */ 	ldx          #35
	jsr          __var_addr_i7			// quotient
/* @190 */ 	jsr         __pushi7
/* @191 */ 	jsr         FixedLShift256
/* @192 */ 	jsr         __incsp2
/* @194 */ 	lda         __b2
/* @195 */ 	sta         __i0
/* @196 */ 	lda          #0
/* @197 */ 	sta         __i0+1
/* @200 */ 	lda         __i0+1
/* @201 */ 	cmp          #0
/* @202 */ 	bcc         .FixedDivideByTen256_label_218
/* @203 */ 	bne         .FixedDivideByTen256_label_199
/* @204 */ 	lda         __i0
/* @205 */ 	cmp          #10
/* @206 */ 	bcc         .FixedDivideByTen256_label_218
.FixedDivideByTen256_label_199:
/* @211 */ 	jsr         __pushi7
/* @212 */ 	jsr         FixedIncrement256
/* @213 */ 	jsr         __incsp2
/* @214 */ 	sec         
/* @215 */ 	lda         __b2
/* @216 */ 	sbc          #10
/* @217 */ 	sta         __b2
.FixedDivideByTen256_label_218:
.FixedDivideByTen256_label_219:
/* @220 */ 	lda          #__i6
/* @222 */ 	jsr         __rinc21
/* @223 */ 	jmp         .FixedDivideByTen256_label_98
.FixedDivideByTen256_label_224:
/* @226 */ 	ldx          #35
	jsr          __var_addr_i0			// quotient
/* @228 */ 	lda          #32
/* @229 */ 	sta         __mem_size
/* @230 */ 	lda          #0
/* @232 */ 	sta         __mem_size+1
/* @234 */ 	lda         __i0
/* @236 */ 	sta         __mem_src
/* @237 */ 	lda         __i0+1
/* @239 */ 	sta         __mem_src+1
/* @240 */ 	lda         __i4
/* @241 */ 	sta         __mem_dest
/* @242 */ 	lda         __i4+1
/* @243 */ 	sta         __mem_dest+1
/* @245 */ 	jsr         __builtin_memcpy
/* @246 */ 	ldx          #40
	jsr          __load_result
/* @247 */ 	lda         #__b2
/* @248 */ 	jsr         __result1
/* @249 */ 	jmp         .FixedDivideByTen256_label_69
.func_end_FixedDivideByTen256:
	.size FixedDivideByTen256, .func_end_FixedDivideByTen256-FixedDivideByTen256

	.global FixedRound
	.type FixedRound, @function

FixedRound:
/* @7 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x20,0x00		// Save mask i:1 b:0 l:0 x:1 f:0 
/* @12 */ 	ldx          #0
	jsr          __arg_value2_i4			// v
/* @16 */ 	ldy          #8
/* @17 */ 	lda         (__i4), Y
/* @19 */ 	sta         __x0
/* @21 */ 	ldy          #9
/* @22 */ 	lda         (__i4), Y
/* @24 */ 	sta         __x0+1
/* @26 */ 	ldy          #10
/* @27 */ 	lda         (__i4), Y
/* @29 */ 	sta         __x0+2
/* @31 */ 	ldy          #11
/* @32 */ 	lda         (__i4), Y
/* @34 */ 	sta         __x0+3
/* @36 */ 	ldy          #12
/* @37 */ 	lda         (__i4), Y
/* @39 */ 	sta         __x0+4
/* @41 */ 	ldy          #13
/* @42 */ 	lda         (__i4), Y
/* @44 */ 	sta         __x0+5
/* @46 */ 	ldy          #14
/* @47 */ 	lda         (__i4), Y
/* @49 */ 	sta         __x0+6
/* @51 */ 	ldy          #15
/* @52 */ 	lda         (__i4), Y
/* @54 */ 	sta         __x0+7
/* @58 */ 	lda          #0
/* @59 */ 	sta         __x1
/* @60 */ 	lda          #0
/* @61 */ 	sta         __x1+1
/* @62 */ 	lda          #0
/* @63 */ 	sta         __x1+2
/* @64 */ 	lda          #0
/* @65 */ 	sta         __x1+3
/* @66 */ 	lda          #0
/* @67 */ 	sta         __x1+4
/* @68 */ 	lda          #0
/* @69 */ 	sta         __x1+5
/* @70 */ 	lda          #0
/* @71 */ 	sta         __x1+6
/* @72 */ 	lda         __x0+7
/* @73 */ 	and          #128
/* @74 */ 	sta         __x1+7
/* @76 */ 	lda         __x1
/* @77 */ 	ora         __x1+1
/* @78 */ 	ora         __x1+2
/* @79 */ 	ora         __x1+3
/* @80 */ 	ora         __x1+4
/* @81 */ 	ora         __x1+5
/* @82 */ 	ora         __x1+6
/* @83 */ 	ora         __x1+7
/* @84 */ 	cmp          #0
/* @85 */ 	beq         .FixedRound_label_100
/* @88 */ 	clc         
/* @89 */ 	lda         __i4
/* @90 */ 	adc          #16
/* @91 */ 	sta         __i0
/* @92 */ 	lda         __i4+1
/* @93 */ 	adc          #0
/* @94 */ 	sta         __i0+1
/* @96 */ 	jsr         __pushi0
/* @97 */ 	jsr         FixedIncrement128
/* @99 */ 	jsr         __incsp2
.FixedRound_label_100:
/* @101 */ 	ldy          #10
	jsr          __leave_void
/* @102 */ 	rts         
.func_end_FixedRound:
	.size FixedRound, .func_end_FixedRound-FixedRound

	.global FixedIsLessThanTen128
	.type FixedIsLessThanTen128, @function

FixedIsLessThanTen128:
/* @10 */ 	stx         __result
/* @12 */ 	sty         __result+1
/* @13 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @17 */ 	ldx          #0
	jsr          __arg_value2_i0			// v
/* @24 */ 	ldy          #7
.FixedIsLessThanTen128_label_25:
/* @27 */ 	lda         (__i0), Y
/* @28 */ 	sta         __x0, Y
/* @29 */ 	dey         
/* @30 */ 	bpl         .FixedIsLessThanTen128_label_25
/* @33 */ 	lda         __x0+7
/* @34 */ 	cmp          #0
/* @35 */ 	bcc         .FixedIsLessThanTen128_label_79
/* @36 */ 	bne         .FixedIsLessThanTen128_label_32
/* @38 */ 	lda         __x0+6
/* @39 */ 	cmp          #0
/* @40 */ 	bcc         .FixedIsLessThanTen128_label_79
/* @41 */ 	bne         .FixedIsLessThanTen128_label_32
/* @43 */ 	lda         __x0+5
/* @44 */ 	cmp          #0
/* @45 */ 	bcc         .FixedIsLessThanTen128_label_79
/* @46 */ 	bne         .FixedIsLessThanTen128_label_32
/* @48 */ 	lda         __x0+4
/* @49 */ 	cmp          #0
/* @50 */ 	bcc         .FixedIsLessThanTen128_label_79
/* @51 */ 	bne         .FixedIsLessThanTen128_label_32
/* @53 */ 	lda         __x0+3
/* @54 */ 	cmp          #0
/* @55 */ 	bcc         .FixedIsLessThanTen128_label_79
/* @56 */ 	bne         .FixedIsLessThanTen128_label_32
/* @58 */ 	lda         __x0+2
/* @59 */ 	cmp          #0
/* @60 */ 	bcc         .FixedIsLessThanTen128_label_79
/* @61 */ 	bne         .FixedIsLessThanTen128_label_32
/* @62 */ 	lda         __x0+1
/* @63 */ 	cmp          #0
/* @64 */ 	bcc         .FixedIsLessThanTen128_label_79
/* @65 */ 	bne         .FixedIsLessThanTen128_label_32
/* @66 */ 	lda         __x0
/* @67 */ 	cmp          #10
/* @68 */ 	bcc         .FixedIsLessThanTen128_label_79
.FixedIsLessThanTen128_label_32:
/* @71 */ 	lda          #0
/* @72 */ 	sta         __b0
/* @73 */ 	lda         #__b0
/* @75 */ 	jsr         __result1
.FixedIsLessThanTen128_label_76:
/* @77 */ 	ldy          #8
	jsr          __leave_leaf_nomask
/* @78 */ 	rts         
.FixedIsLessThanTen128_label_79:
/* @80 */ 	lda          #1
/* @81 */ 	sta         __i1
/* @82 */ 	lda          #0
/* @83 */ 	sta         __i1+1
.FixedIsLessThanTen128_label_84:
/* @86 */ 	lda         __i1
/* @87 */ 	cmp          #2
/* @88 */ 	lda         __i1+1
/* @89 */ 	sbc          #0
/* @90 */ 	bvc         .FixedIsLessThanTen128_label_85
/* @92 */ 	eor          #128
.FixedIsLessThanTen128_label_85:
/* @93 */ 	bpl         .FixedIsLessThanTen128_label_151
/* @96 */ 	lda         __i1
/* @97 */ 	asl          A
/* @98 */ 	sta         __i2
/* @99 */ 	lda         __i1+1
/* @100 */ 	rol          A
/* @101 */ 	sta         __i2+1
/* @102 */ 	ldx          #2
.FixedIsLessThanTen128_label_103:
/* @104 */ 	asl         __i2
/* @105 */ 	rol         __i2+1
/* @106 */ 	dex         
/* @107 */ 	bne         .FixedIsLessThanTen128_label_103
/* @111 */ 	clc         
/* @112 */ 	lda         __i0
/* @113 */ 	adc         __i2
/* @114 */ 	sta         __i3
/* @115 */ 	lda         __i0+1
/* @116 */ 	adc         __i2+1
/* @117 */ 	sta         __i3+1
/* @122 */ 	ldy          #7
.FixedIsLessThanTen128_label_123:
/* @124 */ 	lda         (__i3), Y
/* @125 */ 	sta         __x0, Y
/* @126 */ 	dey         
/* @127 */ 	bpl         .FixedIsLessThanTen128_label_123
/* @129 */ 	lda         __x0
/* @130 */ 	ora         __x0+1
/* @131 */ 	ora         __x0+2
/* @132 */ 	ora         __x0+3
/* @133 */ 	ora         __x0+4
/* @134 */ 	ora         __x0+5
/* @135 */ 	ora         __x0+6
/* @136 */ 	ora         __x0+7
/* @137 */ 	cmp          #0
/* @138 */ 	beq         .FixedIsLessThanTen128_label_145
/* @140 */ 	lda          #0
/* @141 */ 	sta         __b0
/* @142 */ 	lda         #__b0
/* @143 */ 	jsr         __result1
/* @144 */ 	bra         .FixedIsLessThanTen128_label_76
.FixedIsLessThanTen128_label_145:
.FixedIsLessThanTen128_label_146:
/* @147 */ 	lda          #__i1
/* @149 */ 	jsr         __rinc21
/* @150 */ 	bra         .FixedIsLessThanTen128_label_84
.FixedIsLessThanTen128_label_151:
/* @153 */ 	lda          #1
/* @154 */ 	sta         __b0
/* @155 */ 	lda         #__b0
/* @156 */ 	jsr         __result1
/* @157 */ 	bra         .FixedIsLessThanTen128_label_76
.func_end_FixedIsLessThanTen128:
	.size FixedIsLessThanTen128, .func_end_FixedIsLessThanTen128-FixedIsLessThanTen128

	.global CollectInteger
	.type CollectInteger, @function

CollectInteger:
/* @9 */ 	stx         __result
/* @11 */ 	sty         __result+1
/* @12 */ 	ldx          #23
	jsr          __enter
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
/* @21 */ 	ldx          #0
	jsr          __arg_value2_i4			// p
/* @25 */ 	ldx          #2
	jsr          __arg_value2_i5			// fx
/* @27 */ 	lda          #16
/* @29 */ 	sta         __mem_size
/* @31 */ 	ldx          #19
	jsr          __var_addr_i0			// n
/* @34 */ 	lda         __i0
/* @36 */ 	sta         __mem_dest
/* @38 */ 	lda         __i0+1
/* @40 */ 	sta         __mem_dest+1
/* @42 */ 	jsr         __zeromem1
.CollectInteger_label_43:
/* @46 */ 	lda         (__i4)
/* @47 */ 	sta         __b0
/* @51 */ 	lda         __b0
/* @52 */ 	sta         __i0
/* @54 */ 	and          #128
/* @56 */ 	beq         .CollectInteger_label_55
/* @58 */ 	lda          #255
.CollectInteger_label_55:
/* @59 */ 	sta         __i0+1
/* @62 */ 	ldx         __i0
/* @63 */ 	ldy         __i0+1
/* @65 */ 	jsr         __builtin_isdigit
/* @66 */ 	sta         __i1
/* @67 */ 	stz         __i1+1
/* @69 */ 	lda         __i1
/* @70 */ 	ora         __i1+1
/* @71 */ 	cmp          #0
/* @72 */ 	beq         .CollectInteger_label_140
/* @75 */ 	lda         (__i4)
/* @76 */ 	sta         __b0
/* @80 */ 	lda         __b0
/* @81 */ 	sta         __i0
/* @82 */ 	and          #128
/* @84 */ 	beq         .CollectInteger_label_83
/* @85 */ 	lda          #255
.CollectInteger_label_83:
/* @86 */ 	sta         __i0+1
/* @90 */ 	sec         
/* @91 */ 	lda         __i0
/* @92 */ 	sbc          #48
/* @93 */ 	sta         __i1
/* @94 */ 	lda         __i0+1
/* @95 */ 	sbc          #0
/* @96 */ 	sta         __i1+1
/* @100 */ 	lda         __i1
/* @101 */ 	sta         __x0
/* @102 */ 	lda         __i1+1
/* @103 */ 	sta         __x0+1
/* @104 */ 	lda          #0
/* @106 */ 	ldy          #7
.CollectInteger_label_107:
/* @109 */ 	sta         __x0, Y
/* @110 */ 	dey         
/* @111 */ 	cpy          #1
/* @112 */ 	bne         .CollectInteger_label_107
/* @115 */ 	ldx          #19
	jsr          __var_addr_i6			// n
/* @118 */ 	ldy          #7
.CollectInteger_label_119:
/* @120 */ 	lda         __x0, Y
/* @121 */ 	sta         (__i6), Y
/* @122 */ 	dey         
/* @123 */ 	bpl         .CollectInteger_label_119
/* @124 */ 	jsr         __pushi5
/* @125 */ 	jsr         FixedMultiplyByTen128
/* @127 */ 	jsr         __incsp2
/* @131 */ 	jsr         __pushi6
/* @132 */ 	jsr         __pushi5
/* @133 */ 	jsr         FixedAdd128
/* @135 */ 	jsr         __incsp4
/* @136 */ 	lda          #__i4
/* @138 */ 	jsr         __rinc21
/* @139 */ 	jmp         .CollectInteger_label_43
.CollectInteger_label_140:
/* @141 */ 	ldx          #24
	jsr          __load_result
/* @142 */ 	lda         #__i4
/* @144 */ 	jsr         __result2
.CollectInteger_label_145:
/* @146 */ 	ldy          #26
	jsr          __leave
/* @147 */ 	rts         
.func_end_CollectInteger:
	.size CollectInteger, .func_end_CollectInteger-CollectInteger

	.global CollectPossibleSign
	.type CollectPossibleSign, @function

CollectPossibleSign:
/* @9 */ 	stx         __result
/* @11 */ 	sty         __result+1
/* @12 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i0			// p
/* @20 */ 	ldx          #2
	jsr          __arg_value2_i1			// neg
/* @24 */ 	lda         (__i0)
/* @25 */ 	sta         __b0
/* @29 */ 	lda         __b0
/* @30 */ 	sta         __i2
/* @32 */ 	and          #128
/* @34 */ 	beq         .CollectPossibleSign_label_33
/* @36 */ 	lda          #255
.CollectPossibleSign_label_33:
/* @37 */ 	sta         __i2+1
/* @40 */ 	lda         __i2
/* @41 */ 	cmp          #45
/* @42 */ 	bne         .CollectPossibleSign_label_52
/* @43 */ 	lda         __i2+1
/* @44 */ 	cmp          #0
/* @45 */ 	bne         .CollectPossibleSign_label_52
.CollectPossibleSign_label_39:
/* @47 */ 	lda          #1
/* @48 */ 	sta         (__i1)
/* @49 */ 	lda          #__i0
/* @51 */ 	jsr         __rinc21
.CollectPossibleSign_label_52:
/* @55 */ 	lda         (__i0)
/* @56 */ 	sta         __b0
/* @60 */ 	lda         __b0
/* @61 */ 	sta         __i2
/* @62 */ 	and          #128
/* @64 */ 	beq         .CollectPossibleSign_label_63
/* @65 */ 	lda          #255
.CollectPossibleSign_label_63:
/* @66 */ 	sta         __i2+1
/* @69 */ 	lda         __i2
/* @70 */ 	cmp          #43
/* @71 */ 	bne         .CollectPossibleSign_label_78
/* @72 */ 	lda         __i2+1
/* @73 */ 	cmp          #0
/* @74 */ 	bne         .CollectPossibleSign_label_78
.CollectPossibleSign_label_68:
/* @76 */ 	lda          #__i0
/* @77 */ 	jsr         __rinc21
.CollectPossibleSign_label_78:
/* @79 */ 	lda         #__i0
/* @81 */ 	jsr         __result2
.CollectPossibleSign_label_82:
/* @83 */ 	ldy          #8
	jsr          __leave_leaf_nomask
/* @84 */ 	rts         
.func_end_CollectPossibleSign:
	.size CollectPossibleSign, .func_end_CollectPossibleSign-CollectPossibleSign

	.global CollectAndHandleExponent
	.type CollectAndHandleExponent, @function

CollectAndHandleExponent:
/* @9 */ 	ldx          #8
	jsr          __enter
	.byte        0x05,0x00,0x00		// Save mask i:5 b:0 l:0 x:0 f:0 
/* @19 */ 	ldx          #0
	jsr          __arg_value2_i4			// p
/* @27 */ 	ldx          #2
	jsr          __arg_value2_i7			// fx
/* @31 */ 	lda          #0
/* @32 */ 	sta         __b0
/* @33 */ 	lda          #__b0
/* @35 */ 	ldx          #4
/* @37 */ 	jsr         __set_var_value1
/* @39 */ 	ldx          #4
	jsr          __var_addr_i0			// negative
/* @41 */ 	jsr         __pushi0
/* @42 */ 	jsr         __pushi4
/* @44 */ 	ldx         #__i0
/* @45 */ 	ldy          #0
/* @46 */ 	jsr         CollectPossibleSign
/* @48 */ 	jsr         __incsp4
/* @51 */ 	lda         __i0
/* @52 */ 	sta         __i4
/* @54 */ 	lda         __i0+1
/* @55 */ 	sta         __i4+1
/* @56 */ 	lda          #0
/* @57 */ 	sta         __i5
/* @58 */ 	lda          #0
/* @59 */ 	sta         __i5+1
.CollectAndHandleExponent_label_60:
/* @63 */ 	lda         (__i4)
/* @64 */ 	sta         __b0
/* @68 */ 	lda         __b0
/* @69 */ 	sta         __i0
/* @71 */ 	and          #128
/* @73 */ 	beq         .CollectAndHandleExponent_label_72
/* @75 */ 	lda          #255
.CollectAndHandleExponent_label_72:
/* @76 */ 	sta         __i0+1
/* @79 */ 	ldx         __i0
/* @80 */ 	ldy         __i0+1
/* @82 */ 	jsr         __builtin_isdigit
/* @83 */ 	sta         __i1
/* @84 */ 	stz         __i1+1
/* @86 */ 	lda         __i1
/* @87 */ 	ora         __i1+1
/* @88 */ 	cmp          #0
/* @89 */ 	beq         .CollectAndHandleExponent_label_141
/* @92 */ 	lda          #__i0
/* @93 */ 	ldx          #__i5
/* @95 */ 	jsr         __smul2_10
/* @98 */ 	lda         __i4
/* @99 */ 	sta         __i1
/* @100 */ 	lda         __i4+1
/* @101 */ 	sta         __i1+1
/* @102 */ 	lda          #__i4
/* @104 */ 	jsr         __rinc21
/* @109 */ 	lda         (__i1)
/* @110 */ 	sta         __b0
/* @114 */ 	lda         __b0
/* @115 */ 	sta         __i1
/* @116 */ 	and          #128
/* @118 */ 	beq         .CollectAndHandleExponent_label_117
/* @119 */ 	lda          #255
.CollectAndHandleExponent_label_117:
/* @120 */ 	sta         __i1+1
/* @125 */ 	clc         
/* @126 */ 	lda         __i0
/* @127 */ 	adc         __i1
/* @128 */ 	sta         __i2
/* @129 */ 	lda         __i0+1
/* @130 */ 	adc         __i1+1
/* @131 */ 	sta         __i2+1
/* @133 */ 	sec         
/* @134 */ 	lda         __i2
/* @135 */ 	sbc          #48
/* @136 */ 	sta         __i5
/* @137 */ 	lda         __i2+1
/* @138 */ 	sbc          #0
/* @139 */ 	sta         __i5+1
/* @140 */ 	bra         .CollectAndHandleExponent_label_60
.CollectAndHandleExponent_label_141:
/* @143 */ 	ldx          #4
	jsr          __var_value1_b0			// negative
/* @145 */ 	lda         __b0
/* @146 */ 	cmp          #0
/* @147 */ 	beq         .CollectAndHandleExponent_label_174
/* @148 */ 	lda          #0
/* @149 */ 	sta         __i6
/* @150 */ 	lda          #0
/* @151 */ 	sta         __i6+1
.CollectAndHandleExponent_label_152:
/* @154 */ 	lda         __i6
/* @155 */ 	cmp         __i5
/* @156 */ 	lda         __i6+1
/* @157 */ 	sbc         __i5+1
/* @158 */ 	bvc         .CollectAndHandleExponent_label_153
/* @159 */ 	eor          #128
.CollectAndHandleExponent_label_153:
/* @160 */ 	bpl         .CollectAndHandleExponent_label_172
/* @161 */ 	jsr         __pushi7
/* @163 */ 	ldx         #__b0
/* @164 */ 	ldy          #0
/* @165 */ 	jsr         FixedDivideByTen256
/* @167 */ 	jsr         __incsp2
.CollectAndHandleExponent_label_168:
/* @169 */ 	lda          #__i6
/* @170 */ 	jsr         __rinc21
/* @171 */ 	bra         .CollectAndHandleExponent_label_152
.CollectAndHandleExponent_label_172:
/* @173 */ 	bra         .CollectAndHandleExponent_label_196
.CollectAndHandleExponent_label_174:
/* @175 */ 	lda          #0
/* @176 */ 	sta         __i8
/* @177 */ 	lda          #0
/* @178 */ 	sta         __i8+1
.CollectAndHandleExponent_label_179:
/* @181 */ 	lda         __i8
/* @182 */ 	cmp         __i5
/* @183 */ 	lda         __i8+1
/* @184 */ 	sbc         __i5+1
/* @185 */ 	bvc         .CollectAndHandleExponent_label_180
/* @186 */ 	eor          #128
.CollectAndHandleExponent_label_180:
/* @187 */ 	bpl         .CollectAndHandleExponent_label_195
/* @188 */ 	jsr         __pushi7
/* @189 */ 	jsr         FixedMultiplyByTen256
/* @190 */ 	jsr         __incsp2
.CollectAndHandleExponent_label_191:
/* @192 */ 	lda          #__i8
/* @193 */ 	jsr         __rinc21
/* @194 */ 	bra         .CollectAndHandleExponent_label_179
.CollectAndHandleExponent_label_195:
.CollectAndHandleExponent_label_196:
/* @197 */ 	ldy          #11
	jsr          __leave_void
/* @198 */ 	rts         
.func_end_CollectAndHandleExponent:
	.size CollectAndHandleExponent, .func_end_CollectAndHandleExponent-CollectAndHandleExponent

	.global NormalizeAndGetExponent
	.type NormalizeAndGetExponent, @function

NormalizeAndGetExponent:
/* @5 */ 	stx         __result
/* @7 */ 	sty         __result+1
/* @8 */ 	ldx          #7
	jsr          __enter
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @19 */ 	ldx          #0
	jsr          __arg_value2_i5			// fx
/* @21 */ 	lda          #127
/* @22 */ 	sta         __i4
/* @24 */ 	lda          #0
/* @25 */ 	sta         __i4+1
/* @28 */ 	clc         
/* @29 */ 	lda         __i5
/* @30 */ 	adc          #16
/* @31 */ 	sta         __i0
/* @32 */ 	lda         __i5+1
/* @33 */ 	adc          #0
/* @34 */ 	sta         __i0+1
/* @36 */ 	jsr         __pushi0
/* @38 */ 	ldx         #__b0
/* @39 */ 	ldy          #0
/* @40 */ 	jsr         FixedIsZero128
/* @42 */ 	jsr         __incsp2
/* @44 */ 	lda         __b0
/* @45 */ 	cmp          #0
/* @46 */ 	beq         .NormalizeAndGetExponent_label_88
.NormalizeAndGetExponent_label_47:
/* @50 */ 	clc         
/* @51 */ 	lda         __i5
/* @52 */ 	adc          #16
/* @53 */ 	sta         __i0
/* @54 */ 	lda         __i5+1
/* @55 */ 	adc          #0
/* @56 */ 	sta         __i0+1
/* @58 */ 	jsr         __pushi0
/* @60 */ 	ldx         #__b0
/* @61 */ 	ldy          #0
/* @62 */ 	jsr         FixedIsOne128
/* @63 */ 	jsr         __incsp2
/* @68 */ 	lda         __b0
/* @69 */ 	cmp          #0
/* @70 */ 	beq         .NormalizeAndGetExponent_label_67
/* @72 */ 	lda          #255
.NormalizeAndGetExponent_label_67:
/* @73 */ 	inc          A
/* @74 */ 	sta         __b1
/* @76 */ 	lda         __b1
/* @77 */ 	cmp          #0
/* @78 */ 	beq         .NormalizeAndGetExponent_label_86
/* @79 */ 	jsr         __pushi5
/* @80 */ 	jsr         FixedLShift256
/* @81 */ 	jsr         __incsp2
/* @82 */ 	lda          #__i4
/* @84 */ 	jsr         __rdec21
/* @85 */ 	bra         .NormalizeAndGetExponent_label_47
.NormalizeAndGetExponent_label_86:
/* @87 */ 	bra         .NormalizeAndGetExponent_label_128
.NormalizeAndGetExponent_label_88:
.NormalizeAndGetExponent_label_89:
/* @92 */ 	clc         
/* @93 */ 	lda         __i5
/* @94 */ 	adc          #16
/* @95 */ 	sta         __i0
/* @96 */ 	lda         __i5+1
/* @97 */ 	adc          #0
/* @98 */ 	sta         __i0+1
/* @100 */ 	jsr         __pushi0
/* @102 */ 	ldx         #__b0
/* @103 */ 	ldy          #0
/* @104 */ 	jsr         FixedIsOne128
/* @105 */ 	jsr         __incsp2
/* @110 */ 	lda         __b0
/* @111 */ 	cmp          #0
/* @112 */ 	beq         .NormalizeAndGetExponent_label_109
/* @113 */ 	lda          #255
.NormalizeAndGetExponent_label_109:
/* @114 */ 	inc          A
/* @115 */ 	sta         __b1
/* @117 */ 	lda         __b1
/* @118 */ 	cmp          #0
/* @119 */ 	beq         .NormalizeAndGetExponent_label_127
/* @120 */ 	jsr         __pushi5
/* @121 */ 	jsr         FixedRShift256
/* @122 */ 	jsr         __incsp2
/* @123 */ 	lda          #__i4
/* @125 */ 	jsr         __rinc21
/* @126 */ 	bra         .NormalizeAndGetExponent_label_89
.NormalizeAndGetExponent_label_127:
.NormalizeAndGetExponent_label_128:
/* @129 */ 	ldx          #8
	jsr          __load_result
/* @130 */ 	lda         #__i4
/* @132 */ 	jsr         __result2
.NormalizeAndGetExponent_label_133:
/* @134 */ 	ldy          #10
	jsr          __leave
/* @135 */ 	rts         
.func_end_NormalizeAndGetExponent:
	.size NormalizeAndGetExponent, .func_end_NormalizeAndGetExponent-NormalizeAndGetExponent

	.global ASCIIToFloat
	.type ASCIIToFloat, @function

ASCIIToFloat:
/* @24 */ 	stx         __result
/* @26 */ 	sty         __result+1
/* @27 */ 	ldx          #58
	jsr          __enter
	.byte        0x28,0x22,0x00		// Save mask i:8 b:1 l:1 x:1 f:0 
/* @44 */ 	ldx          #0
	jsr          __arg_value2_i5			// p
/* @62 */ 	lda          #32
/* @64 */ 	sta         __mem_size
/* @66 */ 	ldx          #54
	jsr          __var_addr_i10			// fx
/* @68 */ 	lda         __i10
/* @70 */ 	sta         __mem_dest
/* @72 */ 	lda         __i10+1
/* @74 */ 	sta         __mem_dest+1
/* @76 */ 	jsr         __zeromem1
/* @78 */ 	lda          #0
/* @79 */ 	sta         __b0
/* @80 */ 	lda          #__b0
/* @82 */ 	ldx          #10
/* @84 */ 	jsr         __set_var_value1
/* @86 */ 	ldx          #10
	jsr          __var_addr_i0			// negative
/* @88 */ 	jsr         __pushi0
/* @89 */ 	jsr         __pushi5
/* @91 */ 	ldx         #__i0
/* @92 */ 	ldy          #0
/* @93 */ 	jsr         CollectPossibleSign
/* @95 */ 	jsr         __incsp4
/* @98 */ 	lda         __i0
/* @99 */ 	sta         __i5
/* @100 */ 	lda         __i0+1
/* @101 */ 	sta         __i5+1
/* @107 */ 	clc         
/* @108 */ 	lda         __i10
/* @109 */ 	adc          #16
/* @110 */ 	sta         __i1
/* @111 */ 	lda         __i10+1
/* @112 */ 	adc          #0
/* @113 */ 	sta         __i1+1
/* @115 */ 	jsr         __pushi1
/* @116 */ 	jsr         __pushi5
/* @118 */ 	ldx         #__i1
/* @119 */ 	ldy          #0
/* @120 */ 	jsr         CollectInteger
/* @121 */ 	jsr         __incsp4
/* @124 */ 	lda         __i1
/* @125 */ 	sta         __i5
/* @126 */ 	lda         __i1+1
/* @127 */ 	sta         __i5+1
/* @130 */ 	lda         (__i5)
/* @131 */ 	sta         __b0
/* @135 */ 	lda         __b0
/* @136 */ 	sta         __i0
/* @138 */ 	and          #128
/* @140 */ 	beq         .ASCIIToFloat_label_139
/* @142 */ 	lda          #255
.ASCIIToFloat_label_139:
/* @143 */ 	sta         __i0+1
/* @146 */ 	lda         __i0
/* @147 */ 	cmp          #46
/* @148 */ 	bne         .ASCIIToFloat_label_229
/* @149 */ 	lda         __i0+1
/* @150 */ 	cmp          #0
/* @151 */ 	bne         .ASCIIToFloat_label_229
.ASCIIToFloat_label_145:
/* @153 */ 	lda          #__i5
/* @155 */ 	jsr         __rinc21
/* @156 */ 	lda         __i5
/* @157 */ 	sta         __i6
/* @158 */ 	lda         __i5+1
/* @159 */ 	sta         __i6+1
/* @165 */ 	clc         
/* @166 */ 	lda         __i10
/* @167 */ 	adc          #16
/* @168 */ 	sta         __i0
/* @169 */ 	lda         __i10+1
/* @170 */ 	adc          #0
/* @171 */ 	sta         __i0+1
/* @173 */ 	jsr         __pushi0
/* @174 */ 	jsr         __pushi5
/* @176 */ 	ldx         #__i0
/* @177 */ 	ldy          #0
/* @178 */ 	jsr         CollectInteger
/* @179 */ 	jsr         __incsp4
/* @182 */ 	lda         __i0
/* @183 */ 	sta         __i5
/* @184 */ 	lda         __i0+1
/* @185 */ 	sta         __i5+1
/* @188 */ 	sec         
/* @189 */ 	lda         __i5
/* @190 */ 	sbc         __i6
/* @191 */ 	sta         __i0
/* @192 */ 	lda         __i5+1
/* @193 */ 	sbc         __i6+1
/* @194 */ 	sta         __i0+1
/* @197 */ 	lda         __i0
/* @198 */ 	sta         __i7
/* @199 */ 	lda         __i0+1
/* @200 */ 	sta         __i7+1
/* @201 */ 	lda          #0
/* @202 */ 	sta         __i8
/* @203 */ 	lda          #0
/* @204 */ 	sta         __i8+1
.ASCIIToFloat_label_205:
/* @207 */ 	lda         __i8
/* @208 */ 	cmp         __i7
/* @209 */ 	lda         __i8+1
/* @210 */ 	sbc         __i7+1
/* @211 */ 	bvc         .ASCIIToFloat_label_206
/* @212 */ 	eor          #128
.ASCIIToFloat_label_206:
/* @213 */ 	bpl         .ASCIIToFloat_label_228
/* @215 */ 	ldx          #54
	jsr          __var_addr_i0			// fx
/* @217 */ 	jsr         __pushi0
/* @219 */ 	ldx         #__b0
/* @220 */ 	ldy          #0
/* @221 */ 	jsr         FixedDivideByTen256
/* @223 */ 	jsr         __incsp2
.ASCIIToFloat_label_224:
/* @225 */ 	lda          #__i8
/* @226 */ 	jsr         __rinc21
/* @227 */ 	bra         .ASCIIToFloat_label_205
.ASCIIToFloat_label_228:
.ASCIIToFloat_label_229:
/* @234 */ 	lda         (__i5)
/* @235 */ 	sta         __b0
/* @239 */ 	lda         __b0
/* @240 */ 	sta         __i0
/* @241 */ 	and          #128
/* @243 */ 	beq         .ASCIIToFloat_label_242
/* @244 */ 	lda          #255
.ASCIIToFloat_label_242:
/* @245 */ 	sta         __i0+1
/* @250 */ 	ldx          #1
/* @251 */ 	lda         __i0
/* @252 */ 	cmp          #101
/* @253 */ 	bne         .ASCIIToFloat_label_248
/* @254 */ 	lda         __i0+1
/* @255 */ 	cmp          #0
/* @256 */ 	beq         .ASCIIToFloat_label_249
.ASCIIToFloat_label_248:
/* @257 */ 	dex         
.ASCIIToFloat_label_249:
/* @258 */ 	stx         __b2
/* @260 */ 	lda         __b2
/* @261 */ 	bne         .ASCIIToFloat_label_289
/* @264 */ 	lda         (__i5)
/* @265 */ 	sta         __b0
/* @269 */ 	lda         __b0
/* @270 */ 	sta         __i0
/* @271 */ 	and          #128
/* @273 */ 	beq         .ASCIIToFloat_label_272
/* @274 */ 	lda          #255
.ASCIIToFloat_label_272:
/* @275 */ 	sta         __i0+1
/* @280 */ 	ldx          #1
/* @281 */ 	lda         __i0
/* @282 */ 	cmp          #69
/* @283 */ 	bne         .ASCIIToFloat_label_278
/* @284 */ 	lda         __i0+1
/* @285 */ 	cmp          #0
/* @286 */ 	beq         .ASCIIToFloat_label_279
.ASCIIToFloat_label_278:
/* @287 */ 	dex         
.ASCIIToFloat_label_279:
/* @288 */ 	stx         __b2
.ASCIIToFloat_label_289:
/* @291 */ 	lda         __b2
/* @292 */ 	cmp          #0
/* @293 */ 	beq         .ASCIIToFloat_label_311
/* @296 */ 	clc         
/* @297 */ 	lda         __i5
/* @298 */ 	adc          #1
/* @299 */ 	sta         __i0
/* @300 */ 	lda         __i5+1
/* @301 */ 	adc          #0
/* @302 */ 	sta         __i0+1
/* @306 */ 	jsr         __pushi10
/* @308 */ 	jsr         __pushi0
/* @309 */ 	jsr         CollectAndHandleExponent
/* @310 */ 	jsr         __incsp4
.ASCIIToFloat_label_311:
/* @315 */ 	jsr         __pushi10
/* @317 */ 	ldx         #__b0
/* @318 */ 	ldy          #0
/* @319 */ 	jsr         FixedIsZero256
/* @320 */ 	jsr         __incsp2
/* @322 */ 	lda         __b0
/* @323 */ 	cmp          #0
/* @324 */ 	beq         .ASCIIToFloat_label_343
/* @326 */ 	lda          #0
/* @327 */ 	sta         __f0
/* @328 */ 	lda          #0
/* @329 */ 	sta         __f0+1
/* @330 */ 	lda          #0
/* @332 */ 	sta         __f0+2
/* @333 */ 	lda          #0
/* @335 */ 	sta         __f0+3
/* @336 */ 	ldx          #59
	jsr          __load_result
/* @337 */ 	lda         #__f0
/* @339 */ 	jsr         __result4
.ASCIIToFloat_label_340:
/* @341 */ 	ldy          #61
	jsr          __leave
/* @342 */ 	rts         
.ASCIIToFloat_label_343:
/* @347 */ 	jsr         __pushi10
/* @349 */ 	ldx         #__i0
/* @350 */ 	ldy          #0
/* @351 */ 	jsr         NormalizeAndGetExponent
/* @352 */ 	jsr         __incsp2
/* @355 */ 	lda         __i0
/* @356 */ 	sta         __i9
/* @357 */ 	lda         __i0+1
/* @358 */ 	sta         __i9+1
/* @365 */ 	ldy          #8
/* @366 */ 	lda         (__i10), Y
/* @367 */ 	sta         __x0
/* @369 */ 	ldy          #9
/* @370 */ 	lda         (__i10), Y
/* @371 */ 	sta         __x0+1
/* @372 */ 	ldy          #10
/* @373 */ 	lda         (__i10), Y
/* @374 */ 	sta         __x0+2
/* @376 */ 	ldy          #11
/* @377 */ 	lda         (__i10), Y
/* @378 */ 	sta         __x0+3
/* @380 */ 	ldy          #12
/* @381 */ 	lda         (__i10), Y
/* @383 */ 	sta         __x0+4
/* @385 */ 	ldy          #13
/* @386 */ 	lda         (__i10), Y
/* @388 */ 	sta         __x0+5
/* @390 */ 	ldy          #14
/* @391 */ 	lda         (__i10), Y
/* @393 */ 	sta         __x0+6
/* @395 */ 	ldy          #15
/* @396 */ 	lda         (__i10), Y
/* @398 */ 	sta         __x0+7
/* @402 */ 	lda         __x0+4
/* @403 */ 	sta         __x1
/* @404 */ 	lda         __x0+5
/* @405 */ 	sta         __x1+1
/* @406 */ 	lda         __x0+6
/* @407 */ 	sta         __x1+2
/* @408 */ 	lda         __x0+7
/* @409 */ 	sta         __x1+3
/* @410 */ 	lda          #0
/* @411 */ 	sta         __x1+4
/* @412 */ 	sta         __x1+5
/* @413 */ 	sta         __x1+6
/* @414 */ 	sta         __x1+7
/* @418 */ 	lda         __x1
/* @419 */ 	sta         __l0
/* @420 */ 	lda         __x1+1
/* @421 */ 	sta         __l0+1
/* @422 */ 	lda         __x1+2
/* @423 */ 	sta         __l0+2
/* @424 */ 	lda         __x1+3
/* @425 */ 	sta         __l0+3
/* @429 */ 	lda         __l0+3
/* @430 */ 	lsr          A
/* @431 */ 	sta         __l1+3
/* @432 */ 	lda         __l0+2
/* @433 */ 	ror          A
/* @434 */ 	sta         __l1+2
/* @435 */ 	lda         __l0+1
/* @436 */ 	ror          A
/* @437 */ 	sta         __l1+1
/* @438 */ 	lda         __l0
/* @439 */ 	ror          A
/* @440 */ 	sta         __l1
/* @442 */ 	lda         __l1
/* @443 */ 	sta         __l2
/* @444 */ 	lda         __l1+1
/* @445 */ 	sta         __l2+1
/* @446 */ 	lda         __l1+2
/* @447 */ 	sta         __l2+2
/* @448 */ 	lda         __l1+3
/* @449 */ 	ora          #128
/* @450 */ 	sta         __l2+3
/* @452 */ 	ldx          #10
	jsr          __var_value1_b0			// negative
/* @454 */ 	lda         __b0
/* @455 */ 	cmp          #0
/* @456 */ 	beq         .ASCIIToFloat_label_463
/* @458 */ 	lda          #128
/* @459 */ 	sta         __i4
/* @460 */ 	lda          #0
/* @461 */ 	sta         __i4+1
/* @462 */ 	bra         .ASCIIToFloat_label_469
.ASCIIToFloat_label_463:
/* @465 */ 	lda          #0
/* @466 */ 	sta         __i4
/* @467 */ 	lda          #0
/* @468 */ 	sta         __i4+1
.ASCIIToFloat_label_469:
/* @472 */ 	ldx          #16
	jsr          __var_addr_i0			// u
/* @475 */ 	lda         __i4
/* @476 */ 	sta         (__i0)
/* @479 */ 	lda         __i9
/* @480 */ 	sta         __i1
/* @481 */ 	lda         __i9+1
/* @482 */ 	sta         __i1+1
/* @488 */ 	lda         __i1
/* @489 */ 	ldy          #1
/* @490 */ 	sta         (__i0), Y
/* @494 */ 	lda         __l2
/* @495 */ 	ldy          #2
/* @496 */ 	sta         (__i0), Y
/* @497 */ 	lda         __l2+1
/* @498 */ 	ldy          #3
/* @499 */ 	sta         (__i0), Y
/* @500 */ 	lda         __l2+2
/* @501 */ 	ldy          #4
/* @502 */ 	sta         (__i0), Y
/* @503 */ 	lda         __l2+3
/* @504 */ 	ldy          #5
/* @505 */ 	sta         (__i0), Y
/* @507 */ 	ldx          #22
	jsr          __var_addr_i11			// __invented__27
/* @509 */ 	ldx          #9
	jsr          __var_addr_i1			// __invented__28
/* @512 */ 	lda          #6
/* @513 */ 	sta         __mem_size
/* @515 */ 	lda         __i0
/* @517 */ 	sta         __mem_src
/* @518 */ 	lda         __i0+1
/* @520 */ 	sta         __mem_src+1
/* @522 */ 	jsr         __pushmem1
/* @524 */ 	jsr         __pushi1
/* @525 */ 	jsr         Normalize
/* @527 */ 	jsr         __pullxy
/* @528 */ 	stx         __i1
/* @529 */ 	sty         __i1+1
/* @531 */ 	jsr         __incsp6
/* @534 */ 	lda          #6
/* @535 */ 	sta         __mem_size
/* @537 */ 	jsr         __pushmem_xy1
/* @539 */ 	jsr         __pushi11
/* @540 */ 	jsr         Round
/* @541 */ 	jsr         __pullxy
/* @542 */ 	stx         __i11
/* @543 */ 	sty         __i11+1
/* @544 */ 	jsr         __incsp6
/* @547 */ 	lda          #6
/* @548 */ 	sta         __mem_size
/* @549 */ 	jsr         __pushmem_xy1
/* @551 */ 	ldx         #__f0
/* @552 */ 	ldy          #0
/* @553 */ 	jsr         Pack
/* @554 */ 	jsr         __incsp6
/* @556 */ 	ldx          #59
	jsr          __load_result
/* @557 */ 	lda         #__f0
/* @558 */ 	jsr         __result4
/* @559 */ 	jmp         .ASCIIToFloat_label_340
.func_end_ASCIIToFloat:
	.size ASCIIToFloat, .func_end_ASCIIToFloat-ASCIIToFloat

	.global FixFloat
	.type FixFloat, @function

FixFloat:
/* @12 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x20,0x01		// Save mask i:1 b:0 l:0 x:1 f:1 
/* @22 */ 	ldx          #2
	jsr          __arg_value4_f1			// f
/* @26 */ 	ldx          #0
	jsr          __arg_value2_i4			// printer
/* @27 */ 	jsr         __pushf1
/* @30 */ 	ldx         #__b0
/* @31 */ 	ldy          #0
/* @32 */ 	jsr         FPIsZero
/* @34 */ 	jsr         __incsp4
/* @36 */ 	lda         __b0
/* @37 */ 	cmp          #0
/* @38 */ 	beq         .FixFloat_label_42
.FixFloat_label_39:
/* @40 */ 	ldy          #10
	jsr          __leave_void
/* @41 */ 	rts         
.FixFloat_label_42:
/* @43 */ 	jsr         __pushf1
/* @45 */ 	ldx         #__b0
/* @46 */ 	ldy          #0
/* @47 */ 	jsr         FPIsNan
/* @48 */ 	jsr         __incsp4
/* @50 */ 	lda         __b0
/* @51 */ 	cmp          #0
/* @52 */ 	beq         .FixFloat_label_58
/* @53 */ 	lda          #1
/* @55 */ 	ldy          #6
/* @56 */ 	sta         (__i4), Y
/* @57 */ 	bra         .FixFloat_label_39
.FixFloat_label_58:
/* @59 */ 	jsr         __pushf1
/* @61 */ 	ldx         #__b0
/* @62 */ 	ldy          #0
/* @63 */ 	jsr         FPIsInfinity
/* @64 */ 	jsr         __incsp4
/* @66 */ 	lda         __b0
/* @67 */ 	cmp          #0
/* @68 */ 	beq         .FixFloat_label_73
/* @69 */ 	lda          #2
/* @70 */ 	ldy          #6
/* @71 */ 	sta         (__i4), Y
/* @72 */ 	bra         .FixFloat_label_39
.FixFloat_label_73:
/* @76 */ 	clc         
/* @77 */ 	lda         __i4
/* @78 */ 	adc          #0
/* @79 */ 	sta         __i0
/* @80 */ 	lda         __i4+1
/* @81 */ 	adc          #0
/* @82 */ 	sta         __i0+1
/* @83 */ 	jsr         __pushf1
/* @85 */ 	jsr         __pushi0
/* @86 */ 	jsr         Unpack
/* @88 */ 	jsr         __pullxy
/* @89 */ 	stx         __i0
/* @90 */ 	sty         __i0+1
/* @91 */ 	jsr         __incsp4
/* @94 */ 	ldy          #2
/* @95 */ 	lda         (__i4), Y
/* @96 */ 	sta         __l0
/* @98 */ 	ldy          #3
/* @99 */ 	lda         (__i4), Y
/* @100 */ 	sta         __l0+1
/* @102 */ 	ldy          #4
/* @103 */ 	lda         (__i4), Y
/* @104 */ 	sta         __l0+2
/* @106 */ 	ldy          #5
/* @107 */ 	lda         (__i4), Y
/* @108 */ 	sta         __l0+3
/* @112 */ 	lda         __l0
/* @113 */ 	sta         __x0
/* @114 */ 	lda         __l0+1
/* @115 */ 	sta         __x0+1
/* @116 */ 	lda         __l0+2
/* @117 */ 	sta         __x0+2
/* @118 */ 	lda         __l0+3
/* @119 */ 	sta         __x0+3
/* @120 */ 	lda          #0
/* @122 */ 	ldy          #7
.FixFloat_label_123:
/* @125 */ 	sta         __x0, Y
/* @126 */ 	dey         
/* @127 */ 	cpy          #3
/* @128 */ 	bne         .FixFloat_label_123
/* @132 */ 	lda         __x0+3
/* @133 */ 	sta         __x1+7
/* @134 */ 	lda         __x0+2
/* @135 */ 	sta         __x1+6
/* @136 */ 	lda         __x0+1
/* @137 */ 	sta         __x1+5
/* @138 */ 	lda         __x0
/* @139 */ 	sta         __x1+4
/* @140 */ 	lda          #0
/* @141 */ 	sta         __x1
/* @142 */ 	sta         __x1+1
/* @143 */ 	sta         __x1+2
/* @144 */ 	sta         __x1+3
/* @147 */ 	lda         __x1
/* @149 */ 	ldy          #15
/* @150 */ 	sta         (__i4), Y
/* @151 */ 	lda         __x1+1
/* @153 */ 	ldy          #16
/* @154 */ 	sta         (__i4), Y
/* @155 */ 	lda         __x1+2
/* @157 */ 	ldy          #17
/* @158 */ 	sta         (__i4), Y
/* @159 */ 	lda         __x1+3
/* @161 */ 	ldy          #18
/* @162 */ 	sta         (__i4), Y
/* @163 */ 	lda         __x1+4
/* @165 */ 	ldy          #19
/* @166 */ 	sta         (__i4), Y
/* @167 */ 	lda         __x1+5
/* @169 */ 	ldy          #20
/* @170 */ 	sta         (__i4), Y
/* @171 */ 	lda         __x1+6
/* @173 */ 	ldy          #21
/* @174 */ 	sta         (__i4), Y
/* @175 */ 	lda         __x1+7
/* @177 */ 	ldy          #22
/* @178 */ 	sta         (__i4), Y
/* @181 */ 	ldy          #1
/* @182 */ 	lda         (__i4), Y
/* @183 */ 	sta         __b0
/* @187 */ 	lda         __b0
/* @188 */ 	sta         __i0
/* @189 */ 	lda          #0
/* @190 */ 	sta         __i0+1
/* @193 */ 	lda         __i0+1
/* @194 */ 	cmp          #0
/* @195 */ 	bcc         .FixFloat_label_192
/* @196 */ 	bne         .FixFloat_label_264
/* @197 */ 	lda         __i0
/* @198 */ 	cmp          #127
/* @199 */ 	bcs         .FixFloat_label_264
.FixFloat_label_192:
.FixFloat_label_201:
/* @204 */ 	ldy          #1
/* @205 */ 	lda         (__i4), Y
/* @206 */ 	sta         __b0
/* @210 */ 	lda         __b0
/* @211 */ 	sta         __i0
/* @212 */ 	lda          #0
/* @213 */ 	sta         __i0+1
/* @216 */ 	lda         __i0+1
/* @217 */ 	cmp          #0
/* @218 */ 	bcc         .FixFloat_label_215
/* @219 */ 	bne         .FixFloat_label_262
/* @220 */ 	lda         __i0
/* @221 */ 	cmp          #126
/* @222 */ 	bcs         .FixFloat_label_262
.FixFloat_label_215:
/* @226 */ 	clc         
/* @227 */ 	lda         __i4
/* @228 */ 	adc          #7
/* @229 */ 	sta         __i0
/* @230 */ 	lda         __i4+1
/* @231 */ 	adc          #0
/* @232 */ 	sta         __i0+1
/* @234 */ 	jsr         __pushi0
/* @235 */ 	jsr         FixedRShift256
/* @237 */ 	jsr         __incsp2
/* @240 */ 	clc         
/* @241 */ 	lda         __i4
/* @242 */ 	adc          #0
/* @243 */ 	sta         __i0
/* @244 */ 	lda         __i4+1
/* @245 */ 	adc          #0
/* @246 */ 	sta         __i0+1
/* @250 */ 	clc         
/* @251 */ 	lda         __i0
/* @252 */ 	adc          #1
/* @253 */ 	sta         __i1
/* @254 */ 	lda         __i0+1
/* @255 */ 	adc          #0
/* @256 */ 	sta         __i1+1
/* @258 */ 	lda          #__i1
/* @260 */ 	jsr         __inc1
/* @261 */ 	bra         .FixFloat_label_201
.FixFloat_label_262:
/* @263 */ 	bra         .FixFloat_label_326
.FixFloat_label_264:
.FixFloat_label_265:
/* @268 */ 	ldy          #1
/* @269 */ 	lda         (__i4), Y
/* @270 */ 	sta         __b0
/* @274 */ 	lda         __b0
/* @275 */ 	sta         __i0
/* @276 */ 	lda          #0
/* @277 */ 	sta         __i0+1
/* @280 */ 	lda         __i0+1
/* @281 */ 	cmp          #0
/* @282 */ 	bcc         .FixFloat_label_325
/* @283 */ 	bne         .FixFloat_label_279
/* @284 */ 	lda         __i0
/* @285 */ 	cmp          #127
/* @286 */ 	bcc         .FixFloat_label_325
.FixFloat_label_279:
/* @290 */ 	clc         
/* @291 */ 	lda         __i4
/* @292 */ 	adc          #7
/* @293 */ 	sta         __i0
/* @294 */ 	lda         __i4+1
/* @295 */ 	adc          #0
/* @296 */ 	sta         __i0+1
/* @298 */ 	jsr         __pushi0
/* @299 */ 	jsr         FixedLShift256
/* @300 */ 	jsr         __incsp2
/* @303 */ 	clc         
/* @304 */ 	lda         __i4
/* @305 */ 	adc          #0
/* @306 */ 	sta         __i0
/* @307 */ 	lda         __i4+1
/* @308 */ 	adc          #0
/* @309 */ 	sta         __i0+1
/* @313 */ 	clc         
/* @314 */ 	lda         __i0
/* @315 */ 	adc          #1
/* @316 */ 	sta         __i1
/* @317 */ 	lda         __i0+1
/* @318 */ 	adc          #0
/* @319 */ 	sta         __i1+1
/* @321 */ 	lda          #__i1
/* @323 */ 	jsr         __dec1
/* @324 */ 	bra         .FixFloat_label_265
.FixFloat_label_325:
.FixFloat_label_326:
/* @327 */ 	jmp         .FixFloat_label_39
.func_end_FixFloat:
	.size FixFloat, .func_end_FixFloat-FixFloat

	.global WriteZero
	.type WriteZero, @function

WriteZero:
/* @8 */ 	stx         __result
/* @10 */ 	sty         __result+1
/* @11 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @18 */ 	ldx          #2
	jsr          __arg_value2_i1			// buf
/* @22 */ 	ldx          #4
	jsr          __arg_value2_i2			// size
/* @28 */ 	ldx          #0
	jsr          __arg_value2_i4			// precision
/* @31 */ 	clc         
/* @32 */ 	lda         __i1
/* @33 */ 	adc         __i2
/* @34 */ 	sta         __i5
/* @36 */ 	lda         __i1+1
/* @37 */ 	adc         __i2+1
/* @38 */ 	sta         __i5+1
/* @40 */ 	sec         
/* @41 */ 	lda         __i5
/* @42 */ 	sbc          #1
/* @43 */ 	sta         __i0
/* @44 */ 	lda         __i5+1
/* @45 */ 	sbc          #0
/* @46 */ 	sta         __i0+1
/* @47 */ 	lda         __i0
/* @48 */ 	sta         __i3
/* @49 */ 	lda         __i0+1
/* @50 */ 	sta         __i3+1
/* @53 */ 	lda         __i3
/* @54 */ 	sta         __i5
/* @55 */ 	lda         __i3+1
/* @56 */ 	sta         __i5+1
/* @57 */ 	lda          #__i3
/* @59 */ 	jsr         __rdec21
/* @62 */ 	lda          #0
/* @63 */ 	sta         (__i5)
.WriteZero_label_64:
/* @66 */ 	lda          #0
/* @67 */ 	cmp         __i4
/* @68 */ 	lda          #0
/* @69 */ 	sbc         __i4+1
/* @70 */ 	bvc         .WriteZero_label_65
/* @72 */ 	eor          #128
.WriteZero_label_65:
/* @73 */ 	bpl         .WriteZero_label_89
/* @76 */ 	lda         __i3
/* @77 */ 	sta         __i0
/* @78 */ 	lda         __i3+1
/* @79 */ 	sta         __i0+1
/* @80 */ 	lda          #__i3
/* @81 */ 	jsr         __rdec21
/* @84 */ 	lda          #48
/* @85 */ 	sta         (__i0)
/* @86 */ 	lda          #__i4
/* @87 */ 	jsr         __rdec21
/* @88 */ 	bra         .WriteZero_label_64
.WriteZero_label_89:
/* @92 */ 	lda         __i3
/* @93 */ 	sta         __i0
/* @94 */ 	lda         __i3+1
/* @95 */ 	sta         __i0+1
/* @96 */ 	lda          #__i3
/* @97 */ 	jsr         __rdec21
/* @100 */ 	lda          #46
/* @101 */ 	sta         (__i0)
/* @104 */ 	lda         __i3
/* @105 */ 	sta         __i0
/* @106 */ 	lda         __i3+1
/* @107 */ 	sta         __i0+1
/* @108 */ 	lda          #__i3
/* @109 */ 	jsr         __rdec21
/* @112 */ 	lda          #48
/* @113 */ 	sta         (__i0)
/* @116 */ 	clc         
/* @117 */ 	lda         __i3
/* @118 */ 	adc          #1
/* @119 */ 	sta         __i0
/* @120 */ 	lda         __i3+1
/* @121 */ 	adc          #0
/* @122 */ 	sta         __i0+1
/* @124 */ 	lda         #__i0
/* @126 */ 	jsr         __result2
.WriteZero_label_127:
/* @128 */ 	ldy          #8
	jsr          __leave_leaf
/* @129 */ 	rts         
.func_end_WriteZero:
	.size WriteZero, .func_end_WriteZero-WriteZero

	.global WriteNanInf
	.type WriteNanInf, @function

WriteNanInf:
/* @12 */ 	stx         __result
/* @14 */ 	sty         __result+1
/* @15 */ 	ldx          #7
	jsr          __enter
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
/* @23 */ 	ldx          #4
	jsr          __arg_value2_i0			// buf
/* @27 */ 	ldx          #6
	jsr          __arg_value2_i1			// size
/* @33 */ 	ldx          #0
	jsr          __arg_value1_b0			// sign
/* @37 */ 	ldx          #2
	jsr          __arg_value2_i2			// v
/* @40 */ 	clc         
/* @42 */ 	lda         __i0
/* @43 */ 	adc         __i1
/* @44 */ 	sta         __i3
/* @46 */ 	lda         __i0+1
/* @47 */ 	adc         __i1+1
/* @48 */ 	sta         __i3+1
/* @50 */ 	sec         
/* @51 */ 	lda         __i3
/* @52 */ 	sbc          #4
/* @53 */ 	sta         __i5
/* @54 */ 	lda         __i3+1
/* @55 */ 	sbc          #0
/* @56 */ 	sta         __i5+1
/* @57 */ 	lda         __i5
/* @58 */ 	sta         __i6
/* @59 */ 	lda         __i5+1
/* @60 */ 	sta         __i6+1
/* @63 */ 	lda         __b0
/* @64 */ 	sta         __i3
/* @66 */ 	and          #128
/* @68 */ 	beq         .WriteNanInf_label_67
/* @70 */ 	lda          #255
.WriteNanInf_label_67:
/* @71 */ 	sta         __i3+1
/* @73 */ 	lda         __i3
/* @74 */ 	ora         __i3+1
/* @75 */ 	cmp          #0
/* @76 */ 	beq         .WriteNanInf_label_90
/* @79 */ 	lda         __i6
/* @80 */ 	sta         __i0
/* @81 */ 	lda         __i6+1
/* @82 */ 	sta         __i0+1
/* @83 */ 	lda          #__i6
/* @85 */ 	jsr         __rinc21
/* @88 */ 	lda          #45
/* @89 */ 	sta         (__i0)
.WriteNanInf_label_90:
/* @92 */ 	lda         __i2
/* @93 */ 	cmp          #1
/* @94 */ 	bne         .WriteNanInf_label_105
/* @95 */ 	lda         __i2+1
/* @96 */ 	cmp          #0
/* @97 */ 	bne         .WriteNanInf_label_105
.WriteNanInf_label_91:
/* @100 */ 	lda         #%lo(.str.107)
/* @101 */ 	sta         __i4
/* @102 */ 	lda         #%hi(.str.107)
/* @103 */ 	sta         __i4+1
/* @104 */ 	bra         .WriteNanInf_label_111
.WriteNanInf_label_105:
/* @107 */ 	lda         #%lo(.str.108)
/* @108 */ 	sta         __i4
/* @109 */ 	lda         #%hi(.str.108)
/* @110 */ 	sta         __i4+1
.WriteNanInf_label_111:
/* @113 */ 	jsr         __pushi4
/* @114 */ 	jsr         __pushi6
/* @116 */ 	ldx         #__i0
/* @117 */ 	ldy          #0
/* @118 */ 	jsr         strcpy
/* @120 */ 	jsr         __incsp4
/* @121 */ 	ldx          #8
	jsr          __load_result
/* @122 */ 	lda         #__i5
/* @124 */ 	jsr         __result2
.WriteNanInf_label_125:
/* @126 */ 	ldy          #10
	jsr          __leave
/* @127 */ 	rts         
.func_end_WriteNanInf:
	.size WriteNanInf, .func_end_WriteNanInf-WriteNanInf

	.global WriteExponent
	.type WriteExponent, @function

WriteExponent:
/* @17 */ 	stx         __result
/* @19 */ 	sty         __result+1
/* @20 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x42,0x00,0x00		// Save mask i:2 b:2 l:0 x:0 f:0 
/* @27 */ 	ldx          #0
	jsr          __arg_value2_i0			// exp
/* @35 */ 	ldx          #2
	jsr          __arg_value2_i2			// buf
/* @36 */ 	lda          #0
/* @37 */ 	sta         __b1
/* @39 */ 	lda         __i0+1
/* @40 */ 	bpl         .WriteExponent_label_50
/* @41 */ 	sec         
/* @42 */ 	lda          #0
/* @43 */ 	sbc         __i0
/* @44 */ 	sta         __i0
/* @45 */ 	lda          #0
/* @46 */ 	sbc         __i0+1
/* @47 */ 	sta         __i0+1
/* @48 */ 	lda          #1
/* @49 */ 	sta         __b1
.WriteExponent_label_50:
/* @51 */ 	lda          #2
/* @52 */ 	sta         __i1
/* @53 */ 	lda          #0
/* @54 */ 	sta         __i1+1
.WriteExponent_label_55:
/* @61 */ 	ldx          #1
/* @62 */ 	lda         __i0
/* @63 */ 	ora         __i0+1
/* @64 */ 	cmp          #0
/* @65 */ 	bne         .WriteExponent_label_60
.WriteExponent_label_59:
/* @66 */ 	dex         
.WriteExponent_label_60:
/* @67 */ 	stx         __b3
/* @69 */ 	lda         __b3
/* @70 */ 	bne         .WriteExponent_label_85
/* @73 */ 	ldx          #0
/* @75 */ 	lda          #0
/* @76 */ 	cmp         __i1
/* @77 */ 	lda          #0
/* @78 */ 	sbc         __i1+1
/* @79 */ 	bvc         .WriteExponent_label_74
/* @81 */ 	eor          #128
.WriteExponent_label_74:
/* @82 */ 	bpl         .WriteExponent_label_72
/* @83 */ 	inx         
.WriteExponent_label_72:
/* @84 */ 	stx         __b3
.WriteExponent_label_85:
/* @87 */ 	lda         __b3
/* @88 */ 	cmp          #0
/* @89 */ 	beq         .WriteExponent_label_152
/* @92 */ 	lda          #10
/* @93 */ 	sta         __i3
/* @94 */ 	lda          #0
/* @95 */ 	sta         __i3+1
/* @99 */ 	lda          #__i4
/* @100 */ 	ldx          #__i0
/* @101 */ 	ldy          #__i3
/* @103 */ 	jsr         __smod2
/* @106 */ 	lda         __i4
/* @107 */ 	sta         __b2
/* @110 */ 	lda         __i2
/* @111 */ 	sta         __i3
/* @112 */ 	lda         __i2+1
/* @113 */ 	sta         __i3+1
/* @114 */ 	lda          #__i2
/* @116 */ 	jsr         __rdec21
/* @118 */ 	lda         __b2
/* @119 */ 	sta         __i4
/* @120 */ 	lda          #0
/* @121 */ 	sta         __i4+1
/* @125 */ 	clc         
/* @126 */ 	lda         __i4
/* @127 */ 	adc          #48
/* @128 */ 	sta         __i5
/* @129 */ 	lda         __i4+1
/* @130 */ 	adc          #0
/* @131 */ 	sta         __i5+1
/* @136 */ 	lda         __i5
/* @137 */ 	sta         (__i3)
/* @139 */ 	lda          #10
/* @140 */ 	sta         __i3
/* @141 */ 	lda          #0
/* @142 */ 	sta         __i3+1
/* @144 */ 	lda          #__i0
/* @145 */ 	ldx          #__i0
/* @146 */ 	ldy          #__i3
/* @148 */ 	jsr         __sdiv2
/* @149 */ 	lda          #__i1
/* @150 */ 	jsr         __rdec21
/* @151 */ 	bra         .WriteExponent_label_55
.WriteExponent_label_152:
/* @155 */ 	lda         __i2
/* @156 */ 	sta         __i3
/* @157 */ 	lda         __i2+1
/* @158 */ 	sta         __i3+1
/* @159 */ 	lda          #__i2
/* @160 */ 	jsr         __rdec21
/* @161 */ 	lda         __b1
/* @162 */ 	cmp          #0
/* @163 */ 	beq         .WriteExponent_label_168
/* @165 */ 	lda          #45
/* @166 */ 	sta         __b0
/* @167 */ 	bra         .WriteExponent_label_172
.WriteExponent_label_168:
/* @170 */ 	lda          #43
/* @171 */ 	sta         __b0
.WriteExponent_label_172:
/* @177 */ 	lda         __b0
/* @178 */ 	sta         (__i3)
/* @179 */ 	lda         #__i2
/* @181 */ 	jsr         __result2
.WriteExponent_label_182:
/* @183 */ 	ldy          #8
	jsr          __leave_leaf
/* @184 */ 	rts         
.func_end_WriteExponent:
	.size WriteExponent, .func_end_WriteExponent-WriteExponent

	.global CalculateExponent
	.type CalculateExponent, @function

CalculateExponent:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #39
	jsr          __enter
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @21 */ 	ldx          #0
	jsr          __arg_value2_i0			// v
/* @25 */ 	ldx          #35
	jsr          __var_addr_i1			// t
/* @28 */ 	lda          #32
/* @30 */ 	sta         __mem_size
/* @32 */ 	lda          #0
/* @34 */ 	sta         __mem_size+1
/* @35 */ 	lda         __i0
/* @37 */ 	sta         __mem_src
/* @38 */ 	lda         __i0+1
/* @40 */ 	sta         __mem_src+1
/* @42 */ 	lda         __i1
/* @44 */ 	sta         __mem_dest
/* @45 */ 	lda         __i1+1
/* @47 */ 	sta         __mem_dest+1
/* @49 */ 	jsr         __builtin_memcpy
/* @50 */ 	lda          #0
/* @51 */ 	sta         __i4
/* @52 */ 	lda          #0
/* @53 */ 	sta         __i4+1
/* @59 */ 	clc         
/* @60 */ 	lda         __i1
/* @61 */ 	adc          #16
/* @62 */ 	sta         __i2
/* @63 */ 	lda         __i1+1
/* @64 */ 	adc          #0
/* @65 */ 	sta         __i2+1
/* @67 */ 	jsr         __pushi2
/* @69 */ 	ldx         #__b0
/* @70 */ 	ldy          #0
/* @71 */ 	jsr         FixedIsZero128
/* @73 */ 	jsr         __incsp2
/* @75 */ 	lda         __b0
/* @76 */ 	cmp          #0
/* @77 */ 	beq         .CalculateExponent_label_114
.CalculateExponent_label_78:
/* @80 */ 	ldx          #35
	jsr          __var_addr_i5			// t
/* @84 */ 	clc         
/* @85 */ 	lda         __i5
/* @86 */ 	adc          #16
/* @87 */ 	sta         __i0
/* @88 */ 	lda         __i5+1
/* @89 */ 	adc          #0
/* @90 */ 	sta         __i0+1
/* @92 */ 	jsr         __pushi0
/* @94 */ 	ldx         #__b0
/* @95 */ 	ldy          #0
/* @96 */ 	jsr         FixedIsZero128
/* @97 */ 	jsr         __incsp2
/* @99 */ 	lda         __b0
/* @100 */ 	cmp          #0
/* @101 */ 	beq         .CalculateExponent_label_112
/* @105 */ 	jsr         __pushi5
/* @106 */ 	jsr         FixedMultiplyByTen256
/* @107 */ 	jsr         __incsp2
/* @108 */ 	lda          #__i4
/* @110 */ 	jsr         __rdec21
/* @111 */ 	bra         .CalculateExponent_label_78
.CalculateExponent_label_112:
/* @113 */ 	bra         .CalculateExponent_label_164
.CalculateExponent_label_114:
.CalculateExponent_label_115:
/* @117 */ 	ldx          #35
	jsr          __var_addr_i5			// t
/* @121 */ 	clc         
/* @122 */ 	lda         __i5
/* @123 */ 	adc          #16
/* @124 */ 	sta         __i0
/* @125 */ 	lda         __i5+1
/* @126 */ 	adc          #0
/* @127 */ 	sta         __i0+1
/* @129 */ 	jsr         __pushi0
/* @131 */ 	ldx         #__b0
/* @132 */ 	ldy          #0
/* @133 */ 	jsr         FixedIsLessThanTen128
/* @134 */ 	jsr         __incsp2
/* @139 */ 	lda         __b0
/* @140 */ 	cmp          #0
/* @141 */ 	beq         .CalculateExponent_label_138
/* @143 */ 	lda          #255
.CalculateExponent_label_138:
/* @144 */ 	inc          A
/* @145 */ 	sta         __b1
/* @147 */ 	lda         __b1
/* @148 */ 	cmp          #0
/* @149 */ 	beq         .CalculateExponent_label_163
/* @153 */ 	jsr         __pushi5
/* @155 */ 	ldx         #__b0
/* @156 */ 	ldy          #0
/* @157 */ 	jsr         FixedDivideByTen256
/* @158 */ 	jsr         __incsp2
/* @159 */ 	lda          #__i4
/* @161 */ 	jsr         __rinc21
/* @162 */ 	bra         .CalculateExponent_label_115
.CalculateExponent_label_163:
.CalculateExponent_label_164:
/* @165 */ 	ldx          #40
	jsr          __load_result
/* @166 */ 	lda         #__i4
/* @168 */ 	jsr         __result2
.CalculateExponent_label_169:
/* @170 */ 	ldy          #42
	jsr          __leave
/* @171 */ 	rts         
.func_end_CalculateExponent:
	.size CalculateExponent, .func_end_CalculateExponent-CalculateExponent

	.global PrintFixedPointScientific
	.type PrintFixedPointScientific, @function

PrintFixedPointScientific:
/* @18 */ 	stx         __result
/* @20 */ 	sty         __result+1
/* @21 */ 	ldx          #13
	jsr          __enter
	.byte        0x29,0x00,0x00		// Save mask i:9 b:1 l:0 x:0 f:0 
/* @38 */ 	ldx          #4
	jsr          __arg_value2_i4			// buf
/* @42 */ 	ldx          #6
	jsr          __arg_value2_i5			// size
/* @50 */ 	ldx          #2
	jsr          __arg_value2_i8			// precision
/* @66 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @72 */ 	ldy          #6
/* @73 */ 	lda         (__i0), Y
/* @74 */ 	sta         __b0
/* @78 */ 	lda         __b0
/* @79 */ 	sta         __i0
/* @80 */ 	lda          #0
/* @82 */ 	sta         __i0+1
/* @84 */ 	lda         __i0
/* @85 */ 	ora         __i0+1
/* @86 */ 	cmp          #0
/* @87 */ 	beq         .PrintFixedPointScientific_label_139
/* @89 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @94 */ 	lda         (__i0)
/* @95 */ 	sta         __b0
/* @97 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @102 */ 	ldy          #6
/* @103 */ 	lda         (__i0), Y
/* @104 */ 	sta         __b1
/* @108 */ 	lda         __b1
/* @109 */ 	sta         __i0
/* @111 */ 	and          #128
/* @113 */ 	beq         .PrintFixedPointScientific_label_112
/* @115 */ 	lda          #255
.PrintFixedPointScientific_label_112:
/* @116 */ 	sta         __i0+1
/* @117 */ 	jsr         __pushi5
/* @118 */ 	jsr         __pushi4
/* @120 */ 	jsr         __pushi0
/* @122 */ 	lda         __b0
/* @124 */ 	jsr         __pusha
/* @126 */ 	ldx         #__i0
/* @127 */ 	ldy          #0
/* @128 */ 	jsr         WriteNanInf
/* @130 */ 	jsr         __incsp8
/* @132 */ 	ldx          #14
	jsr          __load_result
/* @133 */ 	lda         #__i0
/* @135 */ 	jsr         __result2
.PrintFixedPointScientific_label_136:
/* @137 */ 	ldy          #16
	jsr          __leave
/* @138 */ 	rts         
.PrintFixedPointScientific_label_139:
/* @141 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @145 */ 	clc         
/* @146 */ 	lda         __i0
/* @147 */ 	adc          #7
/* @148 */ 	sta         __i1
/* @149 */ 	lda         __i0+1
/* @150 */ 	adc          #0
/* @151 */ 	sta         __i1+1
/* @153 */ 	jsr         __pushi1
/* @155 */ 	ldx         #__b0
/* @156 */ 	ldy          #0
/* @157 */ 	jsr         FixedIsZero256
/* @159 */ 	jsr         __incsp2
/* @161 */ 	lda         __b0
/* @162 */ 	cmp          #0
/* @163 */ 	bne         .PrintFixedPointScientific_label_631
/* @632 */ 	jmp         .PrintFixedPointScientific_label_271
.PrintFixedPointScientific_label_631:
/* @166 */ 	clc         
/* @167 */ 	lda         __i4
/* @168 */ 	adc         __i5
/* @169 */ 	sta         __i0
/* @170 */ 	lda         __i4+1
/* @171 */ 	adc         __i5+1
/* @172 */ 	sta         __i0+1
/* @174 */ 	sec         
/* @175 */ 	lda         __i0
/* @176 */ 	sbc          #1
/* @177 */ 	sta         __i6
/* @178 */ 	lda         __i0+1
/* @179 */ 	sbc          #0
/* @180 */ 	sta         __i6+1
/* @183 */ 	lda         __i6
/* @184 */ 	sta         __i0
/* @185 */ 	lda         __i6+1
/* @186 */ 	sta         __i0+1
/* @187 */ 	lda          #__i6
/* @189 */ 	jsr         __rdec21
/* @192 */ 	lda          #0
/* @193 */ 	sta         (__i0)
/* @194 */ 	jsr         __pushi6
/* @195 */ 	ldx          #0
/* @197 */ 	jsr         __pushxy0
/* @199 */ 	ldx         #__i0
/* @200 */ 	ldy          #0
/* @201 */ 	jsr         WriteExponent
/* @203 */ 	jsr         __incsp4
/* @206 */ 	lda         __i0
/* @207 */ 	sta         __i7
/* @208 */ 	lda         __i0+1
/* @209 */ 	sta         __i7+1
/* @212 */ 	lda         __i7
/* @213 */ 	sta         __i1
/* @214 */ 	lda         __i7+1
/* @215 */ 	sta         __i1+1
/* @216 */ 	lda          #__i7
/* @217 */ 	jsr         __rdec21
/* @220 */ 	lda          #101
/* @221 */ 	sta         (__i1)
/* @224 */ 	sec         
/* @225 */ 	lda         __i7
/* @226 */ 	sbc         __i4
/* @227 */ 	sta         __i1
/* @228 */ 	lda         __i7+1
/* @229 */ 	sbc         __i4+1
/* @230 */ 	sta         __i1+1
/* @234 */ 	clc         
/* @235 */ 	lda         __i1
/* @236 */ 	adc          #1
/* @237 */ 	sta         __l0
/* @238 */ 	lda         __i1+1
/* @239 */ 	adc          #0
/* @240 */ 	sta         __l0+1
/* @242 */ 	lda         __i1+2
/* @243 */ 	adc          #0
/* @244 */ 	sta         __l0+2
/* @246 */ 	lda         __i1+3
/* @247 */ 	adc          #0
/* @248 */ 	sta         __l0+3
/* @252 */ 	lda         __l0
/* @253 */ 	sta         __i1
/* @254 */ 	lda         __l0+1
/* @255 */ 	sta         __i1+1
/* @257 */ 	jsr         __pushi1
/* @258 */ 	jsr         __pushi4
/* @259 */ 	jsr         __pushi8
/* @261 */ 	ldx         #__i1
/* @262 */ 	ldy          #0
/* @263 */ 	jsr         WriteZero
/* @265 */ 	jsr         __incsp6
/* @267 */ 	ldx          #14
	jsr          __load_result
/* @268 */ 	lda         #__i1
/* @269 */ 	jsr         __result2
/* @270 */ 	jmp         .PrintFixedPointScientific_label_136
.PrintFixedPointScientific_label_271:
/* @273 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @277 */ 	clc         
/* @278 */ 	lda         __i0
/* @279 */ 	adc          #7
/* @280 */ 	sta         __i1
/* @281 */ 	lda         __i0+1
/* @282 */ 	adc          #0
/* @283 */ 	sta         __i1+1
/* @285 */ 	jsr         __pushi1
/* @287 */ 	ldx         #__i0
/* @288 */ 	ldy          #0
/* @289 */ 	jsr         CalculateExponent
/* @290 */ 	jsr         __incsp2
/* @293 */ 	lda         __i0
/* @294 */ 	sta         __i9
/* @295 */ 	lda         __i0+1
/* @296 */ 	sta         __i9+1
/* @297 */ 	sec         
/* @298 */ 	lda         __i8
/* @299 */ 	sbc         __i9
/* @300 */ 	sta         __i10
/* @301 */ 	lda         __i8+1
/* @302 */ 	sbc         __i9+1
/* @303 */ 	sta         __i10+1
/* @305 */ 	lda         __i10+1
/* @306 */ 	bpl         .PrintFixedPointScientific_label_341
/* @307 */ 	lda         __i10
/* @308 */ 	sta         __i11
/* @309 */ 	lda         __i10+1
/* @310 */ 	sta         __i11+1
.PrintFixedPointScientific_label_311:
/* @313 */ 	lda         __i11+1
/* @314 */ 	bpl         .PrintFixedPointScientific_label_339
/* @316 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @320 */ 	clc         
/* @321 */ 	lda         __i0
/* @322 */ 	adc          #7
/* @323 */ 	sta         __i1
/* @324 */ 	lda         __i0+1
/* @325 */ 	adc          #0
/* @326 */ 	sta         __i1+1
/* @328 */ 	jsr         __pushi1
/* @330 */ 	ldx         #__b0
/* @331 */ 	ldy          #0
/* @332 */ 	jsr         FixedDivideByTen256
/* @333 */ 	jsr         __incsp2
.PrintFixedPointScientific_label_334:
/* @335 */ 	lda          #__i11
/* @337 */ 	jsr         __rinc21
/* @338 */ 	bra         .PrintFixedPointScientific_label_311
.PrintFixedPointScientific_label_339:
/* @340 */ 	bra         .PrintFixedPointScientific_label_389
.PrintFixedPointScientific_label_341:
/* @343 */ 	lda          #0
/* @344 */ 	sta         __i12
/* @345 */ 	lda          #0
/* @346 */ 	sta         __i12+1
/* @347 */ 	lda          #__i12
/* @349 */ 	ldx          #5
/* @351 */ 	jsr         __set_var_value2
.PrintFixedPointScientific_label_352:
/* @354 */ 	ldx          #5
	jsr          __var_value2_i0			// i
/* @357 */ 	lda         __i0
/* @358 */ 	cmp         __i10
/* @359 */ 	lda         __i0+1
/* @360 */ 	sbc         __i10+1
/* @361 */ 	bvc         .PrintFixedPointScientific_label_356
/* @362 */ 	eor          #128
.PrintFixedPointScientific_label_356:
/* @363 */ 	bpl         .PrintFixedPointScientific_label_388
/* @365 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @369 */ 	clc         
/* @370 */ 	lda         __i0
/* @371 */ 	adc          #7
/* @372 */ 	sta         __i12
/* @373 */ 	lda         __i0+1
/* @374 */ 	adc          #0
/* @375 */ 	sta         __i12+1
/* @377 */ 	jsr         __pushi12
/* @378 */ 	jsr         FixedMultiplyByTen256
/* @379 */ 	jsr         __incsp2
.PrintFixedPointScientific_label_380:
/* @382 */ 	ldx          #5
	jsr          __var_addr_i0			// i
/* @384 */ 	lda          #__i0
/* @386 */ 	jsr         __inc21
/* @387 */ 	bra         .PrintFixedPointScientific_label_352
.PrintFixedPointScientific_label_388:
.PrintFixedPointScientific_label_389:
/* @391 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @395 */ 	clc         
/* @396 */ 	lda         __i0
/* @397 */ 	adc          #7
/* @398 */ 	sta         __i1
/* @399 */ 	lda         __i0+1
/* @400 */ 	adc          #0
/* @401 */ 	sta         __i1+1
/* @403 */ 	jsr         __pushi1
/* @404 */ 	jsr         FixedRound
/* @405 */ 	jsr         __incsp2
/* @408 */ 	clc         
/* @409 */ 	lda         __i4
/* @410 */ 	adc         __i5
/* @411 */ 	sta         __i0
/* @412 */ 	lda         __i4+1
/* @413 */ 	adc         __i5+1
/* @414 */ 	sta         __i0+1
/* @416 */ 	ldx          #7
	jsr          __var_addr_i1			// end
/* @419 */ 	sec         
/* @420 */ 	ldy          #0
/* @421 */ 	lda         __i0
/* @422 */ 	sbc          #1
/* @423 */ 	sta         (__i1)
/* @424 */ 	ldy          #1
/* @425 */ 	lda         __i0+1
/* @426 */ 	sbc          #0
/* @427 */ 	sta         (__i1), Y
/* @429 */ 	ldx          #7
	jsr          __var_value2_i0			// end
/* @433 */ 	lda          #__i1
/* @435 */ 	jsr         __dec21
/* @438 */ 	lda          #0
/* @439 */ 	sta         (__i0)
/* @441 */ 	ldx          #7
	jsr          __var_value2_i0			// end
/* @443 */ 	jsr         __pushi0
/* @444 */ 	jsr         __pushi9
/* @446 */ 	ldx         #__i0
/* @447 */ 	ldy          #0
/* @448 */ 	jsr         WriteExponent
/* @449 */ 	jsr         __incsp4
/* @451 */ 	lda          #__i0
/* @453 */ 	ldx          #9
/* @454 */ 	jsr         __set_var_value2
/* @456 */ 	ldx          #9
	jsr          __var_value2_i0			// p
/* @458 */ 	ldx          #9
	jsr          __var_addr_i1			// p
/* @460 */ 	lda          #__i1
/* @461 */ 	jsr         __dec21
/* @464 */ 	lda          #101
/* @465 */ 	sta         (__i0)
.PrintFixedPointScientific_label_466:
/* @468 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @472 */ 	clc         
/* @473 */ 	lda         __i0
/* @474 */ 	adc          #7
/* @475 */ 	sta         __i1
/* @476 */ 	lda         __i0+1
/* @477 */ 	adc          #0
/* @478 */ 	sta         __i1+1
/* @482 */ 	clc         
/* @483 */ 	lda         __i1
/* @484 */ 	adc          #16
/* @485 */ 	sta         __i0
/* @486 */ 	lda         __i1+1
/* @487 */ 	adc          #0
/* @488 */ 	sta         __i0+1
/* @490 */ 	jsr         __pushi0
/* @492 */ 	ldx         #__b0
/* @493 */ 	ldy          #0
/* @494 */ 	jsr         FixedIsZero128
/* @495 */ 	jsr         __incsp2
/* @500 */ 	lda         __b0
/* @501 */ 	cmp          #0
/* @502 */ 	beq         .PrintFixedPointScientific_label_499
/* @503 */ 	lda          #255
.PrintFixedPointScientific_label_499:
/* @504 */ 	inc          A
/* @505 */ 	sta         __b1
/* @507 */ 	lda         __b1
/* @508 */ 	cmp          #0
/* @509 */ 	bne         .PrintFixedPointScientific_label_633
/* @634 */ 	jmp         .PrintFixedPointScientific_label_589
.PrintFixedPointScientific_label_633:
/* @510 */ 	lda         __i8
/* @511 */ 	ora         __i8+1
/* @512 */ 	bne         .PrintFixedPointScientific_label_524
/* @514 */ 	ldx          #9
	jsr          __var_value2_i0			// p
/* @516 */ 	ldx          #9
	jsr          __var_addr_i1			// p
/* @518 */ 	lda          #__i1
/* @519 */ 	jsr         __dec21
/* @522 */ 	lda          #46
/* @523 */ 	sta         (__i0)
.PrintFixedPointScientific_label_524:
/* @526 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @530 */ 	clc         
/* @531 */ 	lda         __i0
/* @532 */ 	adc          #7
/* @533 */ 	sta         __i1
/* @534 */ 	lda         __i0+1
/* @535 */ 	adc          #0
/* @536 */ 	sta         __i1+1
/* @540 */ 	clc         
/* @541 */ 	lda         __i1
/* @542 */ 	adc          #16
/* @543 */ 	sta         __i0
/* @544 */ 	lda         __i1+1
/* @545 */ 	adc          #0
/* @546 */ 	sta         __i0+1
/* @548 */ 	jsr         __pushi0
/* @550 */ 	ldx         #__b0
/* @551 */ 	ldy          #0
/* @552 */ 	jsr         FixedDivideByTen128
/* @553 */ 	jsr         __incsp2
/* @556 */ 	lda         __b0
/* @557 */ 	sta         __b2
/* @559 */ 	ldx          #9
	jsr          __var_value2_i0			// p
/* @561 */ 	ldx          #9
	jsr          __var_addr_i1			// p
/* @563 */ 	lda          #__i1
/* @564 */ 	jsr         __dec21
/* @566 */ 	lda         __b2
/* @567 */ 	sta         __i1
/* @568 */ 	lda          #0
/* @569 */ 	sta         __i1+1
/* @573 */ 	clc         
/* @574 */ 	lda         __i1
/* @575 */ 	adc          #48
/* @576 */ 	sta         __i2
/* @577 */ 	lda         __i1+1
/* @578 */ 	adc          #0
/* @579 */ 	sta         __i2+1
/* @584 */ 	lda         __i2
/* @585 */ 	sta         (__i0)
/* @586 */ 	lda          #__i8
/* @587 */ 	jsr         __rdec21
/* @588 */ 	jmp         .PrintFixedPointScientific_label_466
.PrintFixedPointScientific_label_589:
/* @591 */ 	ldx          #0
	jsr          __arg_value2_i0			// printer
/* @596 */ 	lda         (__i0)
/* @597 */ 	sta         __b0
/* @599 */ 	lda         __b0
/* @600 */ 	cmp          #0
/* @601 */ 	beq         .PrintFixedPointScientific_label_613
/* @603 */ 	ldx          #9
	jsr          __var_value2_i0			// p
/* @605 */ 	ldx          #9
	jsr          __var_addr_i1			// p
/* @607 */ 	lda          #__i1
/* @608 */ 	jsr         __dec21
/* @611 */ 	lda          #45
/* @612 */ 	sta         (__i0)
.PrintFixedPointScientific_label_613:
/* @615 */ 	ldx          #9
	jsr          __var_value2_i0			// p
/* @619 */ 	clc         
/* @620 */ 	lda         __i0
/* @621 */ 	adc          #1
/* @622 */ 	sta         __i1
/* @623 */ 	lda         __i0+1
/* @624 */ 	adc          #0
/* @625 */ 	sta         __i1+1
/* @627 */ 	ldx          #14
	jsr          __load_result
/* @628 */ 	lda         #__i1
/* @629 */ 	jsr         __result2
/* @630 */ 	jmp         .PrintFixedPointScientific_label_136
.func_end_PrintFixedPointScientific:
	.size PrintFixedPointScientific, .func_end_PrintFixedPointScientific-PrintFixedPointScientific

	.global PrintFixedPoint
	.type PrintFixedPoint, @function

PrintFixedPoint:
/* @17 */ 	stx         __result
/* @19 */ 	sty         __result+1
/* @20 */ 	ldx          #10
	jsr          __enter
	.byte        0x47,0x00,0x00		// Save mask i:7 b:2 l:0 x:0 f:0 
/* @34 */ 	ldx          #4
	jsr          __arg_value2_i4			// buf
/* @38 */ 	ldx          #6
	jsr          __arg_value2_i5			// size
/* @42 */ 	ldx          #2
	jsr          __arg_value2_i6			// precision
/* @56 */ 	ldx          #0
	jsr          __arg_value2_i9			// printer
/* @60 */ 	ldy          #6
/* @61 */ 	lda         (__i9), Y
/* @62 */ 	sta         __b0
/* @66 */ 	lda         __b0
/* @67 */ 	sta         __i0
/* @68 */ 	lda          #0
/* @69 */ 	sta         __i0+1
/* @71 */ 	lda         __i0
/* @72 */ 	ora         __i0+1
/* @73 */ 	cmp          #0
/* @74 */ 	beq         .PrintFixedPoint_label_118
/* @77 */ 	lda         (__i9)
/* @78 */ 	sta         __b0
/* @81 */ 	ldy          #6
/* @82 */ 	lda         (__i9), Y
/* @83 */ 	sta         __b1
/* @87 */ 	lda         __b1
/* @88 */ 	sta         __i0
/* @90 */ 	and          #128
/* @92 */ 	beq         .PrintFixedPoint_label_91
/* @94 */ 	lda          #255
.PrintFixedPoint_label_91:
/* @95 */ 	sta         __i0+1
/* @96 */ 	jsr         __pushi5
/* @97 */ 	jsr         __pushi4
/* @99 */ 	jsr         __pushi0
/* @101 */ 	lda         __b0
/* @103 */ 	jsr         __pusha
/* @105 */ 	ldx         #__i0
/* @106 */ 	ldy          #0
/* @107 */ 	jsr         WriteNanInf
/* @109 */ 	jsr         __incsp8
/* @111 */ 	ldx          #11
	jsr          __load_result
/* @112 */ 	lda         #__i0
/* @114 */ 	jsr         __result2
.PrintFixedPoint_label_115:
/* @116 */ 	ldy          #13
	jsr          __leave
/* @117 */ 	rts         
.PrintFixedPoint_label_118:
/* @121 */ 	clc         
/* @122 */ 	lda         __i9
/* @123 */ 	adc          #7
/* @124 */ 	sta         __i0
/* @125 */ 	lda         __i9+1
/* @126 */ 	adc          #0
/* @127 */ 	sta         __i0+1
/* @129 */ 	jsr         __pushi0
/* @131 */ 	ldx         #__b0
/* @132 */ 	ldy          #0
/* @133 */ 	jsr         FixedIsZero256
/* @135 */ 	jsr         __incsp2
/* @137 */ 	lda         __b0
/* @138 */ 	cmp          #0
/* @139 */ 	beq         .PrintFixedPoint_label_154
/* @140 */ 	jsr         __pushi5
/* @141 */ 	jsr         __pushi4
/* @142 */ 	jsr         __pushi6
/* @144 */ 	ldx         #__i0
/* @145 */ 	ldy          #0
/* @146 */ 	jsr         WriteZero
/* @148 */ 	jsr         __incsp6
/* @150 */ 	ldx          #11
	jsr          __load_result
/* @151 */ 	lda         #__i0
/* @152 */ 	jsr         __result2
/* @153 */ 	bra         .PrintFixedPoint_label_115
.PrintFixedPoint_label_154:
/* @157 */ 	clc         
/* @158 */ 	lda         __i4
/* @159 */ 	adc         __i5
/* @160 */ 	sta         __i0
/* @161 */ 	lda         __i4+1
/* @162 */ 	adc         __i5+1
/* @163 */ 	sta         __i0+1
/* @165 */ 	sec         
/* @166 */ 	lda         __i0
/* @167 */ 	sbc          #1
/* @168 */ 	sta         __i7
/* @169 */ 	lda         __i0+1
/* @170 */ 	sbc          #0
/* @171 */ 	sta         __i7+1
/* @172 */ 	lda         __i7
/* @173 */ 	sta         __i8
/* @174 */ 	lda         __i7+1
/* @175 */ 	sta         __i8+1
/* @178 */ 	lda         __i8
/* @179 */ 	sta         __i0
/* @180 */ 	lda         __i8+1
/* @181 */ 	sta         __i0+1
/* @182 */ 	lda          #__i8
/* @184 */ 	jsr         __rdec21
/* @187 */ 	lda          #0
/* @188 */ 	sta         (__i0)
/* @189 */ 	lda          #0
/* @190 */ 	sta         __b3
/* @192 */ 	lda          #0
/* @193 */ 	sta         __b0
/* @194 */ 	lda          #__b0
/* @196 */ 	ldx          #4
/* @198 */ 	jsr         __set_var_value1
.PrintFixedPoint_label_199:
/* @201 */ 	ldx          #4
	jsr          __var_value1_b0			// i
/* @205 */ 	lda         __b0
/* @206 */ 	sta         __i0
/* @207 */ 	and          #128
/* @209 */ 	beq         .PrintFixedPoint_label_208
/* @210 */ 	lda          #255
.PrintFixedPoint_label_208:
/* @211 */ 	sta         __i0+1
/* @214 */ 	lda         __i0
/* @215 */ 	cmp         __i6
/* @216 */ 	lda         __i0+1
/* @217 */ 	sbc         __i6+1
/* @218 */ 	bvc         .PrintFixedPoint_label_213
/* @219 */ 	eor          #128
.PrintFixedPoint_label_213:
/* @220 */ 	bpl         .PrintFixedPoint_label_276
/* @223 */ 	clc         
/* @224 */ 	lda         __i9
/* @225 */ 	adc          #7
/* @226 */ 	sta         __i0
/* @227 */ 	lda         __i9+1
/* @228 */ 	adc          #0
/* @229 */ 	sta         __i0+1
/* @231 */ 	jsr         __pushi0
/* @232 */ 	jsr         FixedMultiplyByTen256
/* @233 */ 	jsr         __incsp2
/* @236 */ 	clc         
/* @237 */ 	lda         __i9
/* @238 */ 	adc          #7
/* @239 */ 	sta         __i0
/* @240 */ 	lda         __i9+1
/* @241 */ 	adc          #0
/* @242 */ 	sta         __i0+1
/* @246 */ 	clc         
/* @247 */ 	lda         __i0
/* @248 */ 	adc          #16
/* @249 */ 	sta         __i1
/* @250 */ 	lda         __i0+1
/* @251 */ 	adc          #0
/* @252 */ 	sta         __i1+1
/* @254 */ 	jsr         __pushi1
/* @256 */ 	ldx         #__b0
/* @257 */ 	ldy          #0
/* @258 */ 	jsr         FixedIsZero128
/* @259 */ 	jsr         __incsp2
/* @261 */ 	lda         __b0
/* @262 */ 	cmp          #0
/* @263 */ 	beq         .PrintFixedPoint_label_267
/* @264 */ 	lda          #__b3
/* @266 */ 	jsr         __rinc1
.PrintFixedPoint_label_267:
.PrintFixedPoint_label_268:
/* @270 */ 	ldx          #4
	jsr          __var_addr_i0			// i
/* @272 */ 	lda          #__i0
/* @274 */ 	jsr         __inc1
/* @275 */ 	bra         .PrintFixedPoint_label_199
.PrintFixedPoint_label_276:
/* @279 */ 	clc         
/* @280 */ 	lda         __i9
/* @281 */ 	adc          #7
/* @282 */ 	sta         __i0
/* @283 */ 	lda         __i9+1
/* @284 */ 	adc          #0
/* @285 */ 	sta         __i0+1
/* @287 */ 	jsr         __pushi0
/* @288 */ 	jsr         FixedRound
/* @289 */ 	jsr         __incsp2
/* @291 */ 	lda          #0
/* @292 */ 	sta         __b0
/* @293 */ 	lda          #__b0
/* @295 */ 	ldx          #5
/* @296 */ 	jsr         __set_var_value1
.PrintFixedPoint_label_297:
/* @300 */ 	clc         
/* @301 */ 	lda         __i9
/* @302 */ 	adc          #7
/* @303 */ 	sta         __i10
/* @304 */ 	lda         __i9+1
/* @305 */ 	adc          #0
/* @306 */ 	sta         __i10+1
/* @310 */ 	clc         
/* @311 */ 	lda         __i10
/* @312 */ 	adc          #16
/* @313 */ 	sta         __i0
/* @314 */ 	lda         __i10+1
/* @315 */ 	adc          #0
/* @316 */ 	sta         __i0+1
/* @318 */ 	jsr         __pushi0
/* @320 */ 	ldx         #__b0
/* @321 */ 	ldy          #0
/* @322 */ 	jsr         FixedIsZero128
/* @323 */ 	jsr         __incsp2
/* @328 */ 	lda         __b0
/* @329 */ 	cmp          #0
/* @330 */ 	beq         .PrintFixedPoint_label_327
/* @331 */ 	lda          #255
.PrintFixedPoint_label_327:
/* @332 */ 	inc          A
/* @333 */ 	sta         __b1
/* @335 */ 	lda         __b1
/* @336 */ 	cmp          #0
/* @337 */ 	bne         .PrintFixedPoint_label_556
/* @557 */ 	jmp         .PrintFixedPoint_label_426
.PrintFixedPoint_label_556:
/* @338 */ 	lda         __i6
/* @339 */ 	ora         __i6+1
/* @340 */ 	bne         .PrintFixedPoint_label_359
/* @343 */ 	lda         __i8
/* @344 */ 	sta         __i0
/* @345 */ 	lda         __i8+1
/* @346 */ 	sta         __i0+1
/* @347 */ 	lda          #__i8
/* @348 */ 	jsr         __rdec21
/* @351 */ 	lda          #46
/* @352 */ 	sta         (__i0)
/* @354 */ 	lda          #1
/* @355 */ 	sta         __b0
/* @356 */ 	lda          #__b0
/* @357 */ 	ldx          #5
/* @358 */ 	jsr         __set_var_value1
.PrintFixedPoint_label_359:
/* @362 */ 	clc         
/* @363 */ 	lda         __i9
/* @364 */ 	adc          #7
/* @365 */ 	sta         __i0
/* @366 */ 	lda         __i9+1
/* @367 */ 	adc          #0
/* @368 */ 	sta         __i0+1
/* @372 */ 	clc         
/* @373 */ 	lda         __i0
/* @374 */ 	adc          #16
/* @375 */ 	sta         __i1
/* @376 */ 	lda         __i0+1
/* @377 */ 	adc          #0
/* @378 */ 	sta         __i1+1
/* @380 */ 	jsr         __pushi1
/* @382 */ 	ldx         #__b0
/* @383 */ 	ldy          #0
/* @384 */ 	jsr         FixedDivideByTen128
/* @385 */ 	jsr         __incsp2
/* @387 */ 	lda          #__b0
/* @388 */ 	ldx          #6
/* @389 */ 	jsr         __set_var_value1
/* @392 */ 	lda         __i8
/* @393 */ 	sta         __i0
/* @394 */ 	lda         __i8+1
/* @395 */ 	sta         __i0+1
/* @396 */ 	lda          #__i8
/* @397 */ 	jsr         __rdec21
/* @399 */ 	ldx          #6
	jsr          __var_value1_b0			// r
/* @403 */ 	lda         __b0
/* @404 */ 	sta         __i1
/* @405 */ 	lda          #0
/* @406 */ 	sta         __i1+1
/* @410 */ 	clc         
/* @411 */ 	lda         __i1
/* @412 */ 	adc          #48
/* @413 */ 	sta         __i2
/* @414 */ 	lda         __i1+1
/* @415 */ 	adc          #0
/* @416 */ 	sta         __i2+1
/* @421 */ 	lda         __i2
/* @422 */ 	sta         (__i0)
/* @423 */ 	lda          #__i6
/* @424 */ 	jsr         __rdec21
/* @425 */ 	jmp         .PrintFixedPoint_label_297
.PrintFixedPoint_label_426:
/* @427 */ 	lda          #0
/* @428 */ 	sta         __b2
.PrintFixedPoint_label_429:
/* @431 */ 	lda         __b2
/* @432 */ 	sta         __i0
/* @433 */ 	lda          #0
/* @434 */ 	sta         __i0+1
/* @437 */ 	lda         __b3
/* @438 */ 	sta         __i1
/* @439 */ 	and          #128
/* @441 */ 	beq         .PrintFixedPoint_label_440
/* @442 */ 	lda          #255
.PrintFixedPoint_label_440:
/* @443 */ 	sta         __i1+1
/* @447 */ 	lda         __i0+1
/* @448 */ 	cmp         __i1+1
/* @449 */ 	bcc         .PrintFixedPoint_label_446
/* @450 */ 	bne         .PrintFixedPoint_label_471
/* @451 */ 	lda         __i0
/* @452 */ 	cmp         __i1
/* @453 */ 	bcs         .PrintFixedPoint_label_471
.PrintFixedPoint_label_446:
/* @457 */ 	lda         __i8
/* @458 */ 	sta         __i0
/* @459 */ 	lda         __i8+1
/* @460 */ 	sta         __i0+1
/* @461 */ 	lda          #__i8
/* @462 */ 	jsr         __rdec21
/* @465 */ 	lda          #48
/* @466 */ 	sta         (__i0)
.PrintFixedPoint_label_467:
/* @468 */ 	lda          #__b2
/* @469 */ 	jsr         __rinc1
/* @470 */ 	bra         .PrintFixedPoint_label_429
.PrintFixedPoint_label_471:
/* @473 */ 	ldx          #5
	jsr          __var_value1_b0			// point_printed
/* @478 */ 	lda         __b0
/* @479 */ 	cmp          #0
/* @480 */ 	beq         .PrintFixedPoint_label_477
/* @481 */ 	lda          #255
.PrintFixedPoint_label_477:
/* @482 */ 	inc          A
/* @483 */ 	sta         __b1
/* @485 */ 	lda         __b1
/* @486 */ 	cmp          #0
/* @487 */ 	beq         .PrintFixedPoint_label_512
/* @490 */ 	lda         __i8
/* @491 */ 	sta         __i0
/* @492 */ 	lda         __i8+1
/* @493 */ 	sta         __i0+1
/* @494 */ 	lda          #__i8
/* @495 */ 	jsr         __rdec21
/* @498 */ 	lda          #46
/* @499 */ 	sta         (__i0)
/* @502 */ 	lda         __i8
/* @503 */ 	sta         __i0
/* @504 */ 	lda         __i8+1
/* @505 */ 	sta         __i0+1
/* @506 */ 	lda          #__i8
/* @507 */ 	jsr         __rdec21
/* @510 */ 	lda          #48
/* @511 */ 	sta         (__i0)
.PrintFixedPoint_label_512:
/* @515 */ 	lda         (__i9)
/* @516 */ 	sta         __b0
/* @520 */ 	lda         __b0
/* @521 */ 	sta         __i0
/* @522 */ 	lda          #0
/* @523 */ 	sta         __i0+1
/* @525 */ 	lda         __i0
/* @526 */ 	ora         __i0+1
/* @527 */ 	cmp          #0
/* @528 */ 	beq         .PrintFixedPoint_label_541
/* @531 */ 	lda         __i8
/* @532 */ 	sta         __i0
/* @533 */ 	lda         __i8+1
/* @534 */ 	sta         __i0+1
/* @535 */ 	lda          #__i8
/* @536 */ 	jsr         __rdec21
/* @539 */ 	lda          #45
/* @540 */ 	sta         (__i0)
.PrintFixedPoint_label_541:
/* @544 */ 	clc         
/* @545 */ 	lda         __i8
/* @546 */ 	adc          #1
/* @547 */ 	sta         __i0
/* @548 */ 	lda         __i8+1
/* @549 */ 	adc          #0
/* @550 */ 	sta         __i0+1
/* @552 */ 	ldx          #11
	jsr          __load_result
/* @553 */ 	lda         #__i0
/* @554 */ 	jsr         __result2
/* @555 */ 	jmp         .PrintFixedPoint_label_115
.func_end_PrintFixedPoint:
	.size PrintFixedPoint, .func_end_PrintFixedPoint-PrintFixedPoint

	.global FloatToASCII
	.type FloatToASCII, @function

FloatToASCII:
/* @3 */ 	stx         __result
/* @5 */ 	sty         __result+1
/* @6 */ 	ldx          #46
	jsr          __enter
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
/* @14 */ 	ldx          #0
	jsr          __arg_value4_f0			// f
/* @18 */ 	ldx          #4
	jsr          __arg_value2_i4			// precision
/* @22 */ 	ldx          #6
	jsr          __arg_value2_i5			// buf
/* @26 */ 	ldx          #8
	jsr          __arg_value2_i6			// size
/* @28 */ 	lda          #39
/* @30 */ 	sta         __mem_size
/* @32 */ 	ldx          #42
	jsr          __var_addr_i7			// printer
/* @34 */ 	lda         __i7
/* @36 */ 	sta         __mem_dest
/* @38 */ 	lda         __i7+1
/* @40 */ 	sta         __mem_dest+1
/* @42 */ 	jsr         __zeromem1
/* @45 */ 	jsr         __pushf0
/* @47 */ 	jsr         __pushi7
/* @48 */ 	jsr         FixFloat
/* @50 */ 	jsr         __incsp6
/* @53 */ 	jsr         __pushi6
/* @54 */ 	jsr         __pushi5
/* @55 */ 	jsr         __pushi4
/* @57 */ 	jsr         __pushi7
/* @59 */ 	ldx         #__i0
/* @60 */ 	ldy          #0
/* @61 */ 	jsr         PrintFixedPoint
/* @63 */ 	jsr         __incsp8
/* @65 */ 	ldx          #47
	jsr          __load_result
/* @66 */ 	lda         #__i0
/* @68 */ 	jsr         __result2
.FloatToASCII_label_69:
/* @70 */ 	ldy          #49
	jsr          __leave
/* @71 */ 	rts         
.func_end_FloatToASCII:
	.size FloatToASCII, .func_end_FloatToASCII-FloatToASCII

	.global FloatToASCIIScientific
	.type FloatToASCIIScientific, @function

FloatToASCIIScientific:
/* @3 */ 	stx         __result
/* @5 */ 	sty         __result+1
/* @6 */ 	ldx          #46
	jsr          __enter
	.byte        0x04,0x00,0x00		// Save mask i:4 b:0 l:0 x:0 f:0 
/* @14 */ 	ldx          #0
	jsr          __arg_value4_f0			// f
/* @18 */ 	ldx          #4
	jsr          __arg_value2_i4			// precision
/* @22 */ 	ldx          #6
	jsr          __arg_value2_i5			// buf
/* @26 */ 	ldx          #8
	jsr          __arg_value2_i6			// size
/* @28 */ 	lda          #39
/* @30 */ 	sta         __mem_size
/* @32 */ 	ldx          #42
	jsr          __var_addr_i7			// printer
/* @34 */ 	lda         __i7
/* @36 */ 	sta         __mem_dest
/* @38 */ 	lda         __i7+1
/* @40 */ 	sta         __mem_dest+1
/* @42 */ 	jsr         __zeromem1
/* @45 */ 	jsr         __pushf0
/* @47 */ 	jsr         __pushi7
/* @48 */ 	jsr         FixFloat
/* @50 */ 	jsr         __incsp6
/* @53 */ 	jsr         __pushi6
/* @54 */ 	jsr         __pushi5
/* @55 */ 	jsr         __pushi4
/* @57 */ 	jsr         __pushi7
/* @59 */ 	ldx         #__i0
/* @60 */ 	ldy          #0
/* @61 */ 	jsr         PrintFixedPointScientific
/* @63 */ 	jsr         __incsp8
/* @65 */ 	ldx          #47
	jsr          __load_result
/* @66 */ 	lda         #__i0
/* @68 */ 	jsr         __result2
.FloatToASCIIScientific_label_69:
/* @70 */ 	ldy          #49
	jsr          __leave
/* @71 */ 	rts         
.func_end_FloatToASCIIScientific:
	.size FloatToASCIIScientific, .func_end_FloatToASCIIScientific-FloatToASCIIScientific

	.global main
	.type main, @function

main:
/* @36 */ 	stx         __result
/* @38 */ 	sty         __result+1
/* @39 */ 	ldx          #108
	ldy          #1
	jsr          __enter+2
	.byte        0x46,0x06,0x03		// Save mask i:6 b:2 l:3 x:0 f:3 
/* @119 */ 	lda          #1
/* @120 */ 	sta         __b0
/* @121 */ 	lda          #__b0
/* @123 */ 	ldx          #100
/* @124 */ 	ldy          #1
/* @126 */ 	jsr         __set_var_value1b
/* @128 */ 	lda          #0
/* @129 */ 	sta         __f0
/* @131 */ 	lda          #240
/* @132 */ 	sta         __f0+1
/* @134 */ 	lda          #179
/* @136 */ 	sta         __f0+2
/* @138 */ 	lda          #197
/* @140 */ 	sta         __f0+3
/* @141 */ 	lda          #__f0
/* @143 */ 	ldx          #7
/* @144 */ 	ldy          #1
/* @146 */ 	jsr         __set_var_value4b
/* @148 */ 	lda          #0
/* @149 */ 	sta         __f0
/* @151 */ 	lda          #214
/* @152 */ 	sta         __f0+1
/* @154 */ 	lda          #136
/* @155 */ 	sta         __f0+2
/* @157 */ 	lda          #71
/* @158 */ 	sta         __f0+3
/* @159 */ 	lda          #__f0
/* @161 */ 	ldx          #11
/* @162 */ 	ldy          #1
/* @163 */ 	jsr         __set_var_value4b
/* @165 */ 	ldx          #7
	ldy          #1
	jsr          __var_value4b_f0			// a
/* @167 */ 	ldx          #11
	ldy          #1
	jsr          __var_value4b_f2			// b
/* @171 */ 	ldx          #15
	ldy          #1
	jsr          __var_addrb_i6			// c
/* @176 */ 	lda          #__f3
/* @177 */ 	ldx          #__f0
/* @178 */ 	ldy          #__f2
/* @180 */ 	jsr         __fdiv
/* @182 */ 	ldy          #0
/* @183 */ 	lda         __f3
/* @184 */ 	sta         (__i6)
/* @185 */ 	ldy          #1
/* @186 */ 	lda         __f3+1
/* @187 */ 	sta         (__i6), Y
/* @188 */ 	ldy          #2
/* @189 */ 	lda         __f3+2
/* @190 */ 	sta         (__i6), Y
/* @191 */ 	ldy          #3
/* @192 */ 	lda         __f3+3
/* @193 */ 	sta         (__i6), Y
/* @195 */ 	ldx          #7
	ldy          #1
	jsr          __var_value4b_f0			// a
/* @197 */ 	ldx          #11
	ldy          #1
	jsr          __var_value4b_f2			// b
/* @199 */ 	jsr         __pushf2
/* @201 */ 	jsr         __pushf0
/* @203 */ 	ldx         #__f0
/* @204 */ 	ldy          #0
/* @205 */ 	jsr         FPDivide
/* @207 */ 	jsr         __incsp8
/* @209 */ 	lda          #__f0
/* @211 */ 	ldx          #19
/* @212 */ 	ldy          #1
/* @213 */ 	jsr         __set_var_value4b
/* @218 */ 	ldy          #3
.main_label_219:
/* @221 */ 	lda         (__i6), Y
/* @222 */ 	sta         __l2, Y
/* @223 */ 	dey         
/* @224 */ 	bpl         .main_label_219
/* @226 */ 	ldx          #19
	ldy          #1
	jsr          __var_addrb_i0			// d
/* @229 */ 	ldy          #3
.main_label_230:
/* @231 */ 	lda         (__i0), Y
/* @232 */ 	sta         __l3, Y
/* @233 */ 	dey         
/* @234 */ 	bpl         .main_label_230
/* @236 */ 	ldx          #7
	ldy          #1
	jsr          __var_addrb_i0			// a
/* @238 */ 	ldx          #23
	ldy          #1
	jsr          __var_addrb_i1			// g
/* @242 */ 	lda         (__i0)
/* @243 */ 	sta         (__i1)
/* @244 */ 	ldy          #1
/* @245 */ 	lda         (__i0), Y
/* @246 */ 	ldy          #1
/* @247 */ 	sta         (__i1), Y
/* @248 */ 	ldy          #2
/* @249 */ 	lda         (__i0), Y
/* @250 */ 	ldy          #2
/* @251 */ 	sta         (__i1), Y
/* @252 */ 	ldy          #3
/* @253 */ 	lda         (__i0), Y
/* @254 */ 	ldy          #3
/* @255 */ 	sta         (__i1), Y
/* @257 */ 	ldx          #11
	ldy          #1
	jsr          __var_addrb_i7			// b
/* @259 */ 	ldx          #27
	ldy          #1
	jsr          __var_addrb_i8			// h
/* @263 */ 	lda         (__i7)
/* @264 */ 	sta         (__i8)
/* @265 */ 	ldy          #1
/* @266 */ 	lda         (__i7), Y
/* @267 */ 	ldy          #1
/* @268 */ 	sta         (__i8), Y
/* @269 */ 	ldy          #2
/* @270 */ 	lda         (__i7), Y
/* @271 */ 	ldy          #2
/* @272 */ 	sta         (__i8), Y
/* @273 */ 	ldy          #3
/* @274 */ 	lda         (__i7), Y
/* @275 */ 	ldy          #3
/* @276 */ 	sta         (__i8), Y
/* @279 */ 	lda         #%lo(.str.110)
/* @280 */ 	sta         __i7
/* @281 */ 	lda         #%hi(.str.110)
/* @282 */ 	sta         __i7+1
/* @284 */ 	ldx          #7
	ldy          #1
	jsr          __var_value4b_f2			// a
/* @1746 */ 	jsr          __spill4
	.byte __f2
	.byte 0x68,0x01
/* @286 */ 	ldx          #23
	ldy          #1
	jsr          __var_value4b_l4			// g
/* @288 */ 	ldx          #11
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @290 */ 	ldx          #27
	ldy          #1
	jsr          __var_value4b_l0			// h
/* @292 */ 	ldx          #15
	ldy          #1
	jsr          __var_value4b_f3			// c
/* @294 */ 	ldx          #19
	ldy          #1
	jsr          __var_value4b_f2			// d
/* @295 */ 	jsr         __pushl3
/* @297 */ 	jsr         __pushf2
/* @298 */ 	jsr         __pushl2
/* @300 */ 	jsr         __pushf3
/* @302 */ 	jsr         __pushl0
/* @304 */ 	jsr         __pushf0
/* @306 */ 	jsr         __pushl4
/* @1747 */ 	jsr          __reload4
	.byte __f2
	.byte 0x68,0x01
/* @308 */ 	jsr         __pushf2
/* @310 */ 	jsr         __pushi7
/* @312 */ 	ldx         #__i0
/* @313 */ 	ldy          #0
/* @314 */ 	jsr         printf
/* @316 */ 	lda          #34
/* @318 */ 	sta         __t0
/* @320 */ 	jsr         __incsp
/* @321 */ 	lda          #0
/* @322 */ 	sta         __i4
/* @323 */ 	lda          #0
/* @324 */ 	sta         __i4+1
.main_label_325:
/* @327 */ 	lda         __i4
/* @328 */ 	cmp          #1
/* @329 */ 	lda         __i4+1
/* @330 */ 	sbc          #0
/* @331 */ 	bvc         .main_label_326
/* @333 */ 	eor          #128
.main_label_326:
/* @334 */ 	bmi         .main_label_1748
/* @1749 */ 	jmp         .main_label_1560
.main_label_1748:
/* @336 */ 	ldx         #__i0
/* @337 */ 	ldy          #0
/* @338 */ 	jsr         rand
/* @343 */ 	lda          #__f2
/* @344 */ 	ldx          #__i0
/* @346 */ 	jsr         __i2tof
/* @348 */ 	ldx         #__i1
/* @349 */ 	ldy          #0
/* @350 */ 	jsr         rand
/* @355 */ 	lda          #__f0
/* @356 */ 	ldx          #__i1
/* @357 */ 	jsr         __i2tof
/* @359 */ 	lda          #0
/* @360 */ 	sta         __f2
/* @361 */ 	lda          #0
/* @362 */ 	sta         __f2+1
/* @363 */ 	lda          #128
/* @364 */ 	sta         __f2+2
/* @366 */ 	lda          #63
/* @367 */ 	sta         __f2+3
/* @373 */ 	lda          #__f3
/* @374 */ 	ldx          #__f2
/* @375 */ 	ldy          #__f0
/* @376 */ 	jsr         __fdiv
/* @381 */ 	lda          #__f1
/* @382 */ 	ldx          #__f2
/* @383 */ 	ldy          #__f3
/* @385 */ 	jsr         __fadd
/* @387 */ 	ldx         #__i2
/* @388 */ 	ldy          #0
/* @389 */ 	jsr         rand
/* @394 */ 	lda          #__f0
/* @395 */ 	ldx          #__i2
/* @396 */ 	jsr         __i2tof
/* @398 */ 	ldx         #__i3
/* @399 */ 	ldy          #0
/* @400 */ 	jsr         rand
/* @405 */ 	lda          #__f0
/* @406 */ 	ldx          #__i3
/* @407 */ 	jsr         __i2tof
/* @409 */ 	lda          #0
/* @410 */ 	sta         __f2
/* @411 */ 	lda          #0
/* @412 */ 	sta         __f2+1
/* @413 */ 	lda          #128
/* @414 */ 	sta         __f2+2
/* @415 */ 	lda          #63
/* @416 */ 	sta         __f2+3
/* @422 */ 	lda          #__f3
/* @423 */ 	ldx          #__f2
/* @424 */ 	ldy          #__f0
/* @425 */ 	jsr         __fdiv
/* @429 */ 	ldx          #31
	ldy          #1
	jsr          __var_addrb_i6			// b
/* @434 */ 	lda          #__f0
/* @435 */ 	ldx          #__f0
/* @436 */ 	ldy          #__f3
/* @437 */ 	jsr         __fadd
/* @439 */ 	ldy          #0
/* @440 */ 	lda         __f0
/* @441 */ 	sta         (__i6)
/* @442 */ 	ldy          #1
/* @443 */ 	lda         __f0+1
/* @444 */ 	sta         (__i6), Y
/* @445 */ 	ldy          #2
/* @446 */ 	lda         __f0+2
/* @447 */ 	sta         (__i6), Y
/* @448 */ 	ldy          #3
/* @449 */ 	lda         __f0+3
/* @450 */ 	sta         (__i6), Y
/* @452 */ 	ldx         #__i7
/* @453 */ 	ldy          #0
/* @454 */ 	jsr         rand
/* @457 */ 	ldx          #35
	ldy          #1
	jsr          __var_addrb_i8			// v
/* @459 */ 	ldy          #0
/* @460 */ 	lda         __i7
/* @461 */ 	sta         (__i8)
/* @462 */ 	ldy          #1
/* @463 */ 	lda         __i7+1
/* @464 */ 	sta         (__i8), Y
/* @465 */ 	and          #128
/* @467 */ 	beq         .main_label_466
/* @469 */ 	lda          #255
.main_label_466:
/* @470 */ 	ldy          #2
/* @471 */ 	sta         (__i8), Y
/* @472 */ 	ldy          #3
/* @473 */ 	sta         (__i8), Y
/* @475 */ 	ldx         #__i9
/* @476 */ 	ldy          #0
/* @477 */ 	jsr         rand
/* @481 */ 	lda         __i9
/* @482 */ 	and          #1
/* @483 */ 	sta         __i0
/* @484 */ 	lda          #0
/* @485 */ 	sta         __i0+1
/* @487 */ 	lda         __i0
/* @488 */ 	ora         __i0+1
/* @489 */ 	cmp          #0
/* @490 */ 	beq         .main_label_495
/* @491 */ 	ldy          #3
/* @492 */ 	lda         __f1+3
/* @493 */ 	eor          #128
/* @494 */ 	sta         __f1+3
.main_label_495:
/* @497 */ 	ldx         #__i0
/* @498 */ 	ldy          #0
/* @499 */ 	jsr         rand
/* @503 */ 	lda         __i0
/* @504 */ 	and          #1
/* @505 */ 	sta         __i1
/* @506 */ 	lda          #0
/* @507 */ 	sta         __i1+1
/* @509 */ 	lda         __i1
/* @510 */ 	ora         __i1+1
/* @511 */ 	cmp          #0
/* @512 */ 	beq         .main_label_530
/* @514 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @520 */ 	ldy          #3
.main_label_521:
/* @522 */ 	lda         __f0, Y
/* @523 */ 	sta         (__i6), Y
/* @524 */ 	dey         
/* @525 */ 	bpl         .main_label_521
/* @526 */ 	ldy          #3
/* @527 */ 	lda         (__i6), Y
/* @528 */ 	eor          #128
/* @529 */ 	sta         (__i6), Y
.main_label_530:
/* @533 */ 	lda         #%lo(.str.111)
/* @534 */ 	sta         __i0
/* @535 */ 	lda         #%hi(.str.111)
/* @536 */ 	sta         __i0+1
/* @537 */ 	jsr         __pushi4
/* @539 */ 	jsr         __pushi0
/* @541 */ 	ldx         #__i0
/* @542 */ 	ldy          #0
/* @543 */ 	jsr         printf
/* @545 */ 	jsr         __incsp4
/* @547 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @550 */ 	ldx          #39
	ldy          #1
	jsr          __var_addrb_i7			// c
/* @554 */ 	lda          #__f2
/* @555 */ 	ldx          #__f1
/* @556 */ 	ldy          #__f0
/* @558 */ 	jsr         __fmul
/* @560 */ 	ldy          #0
/* @561 */ 	lda         __f2
/* @562 */ 	sta         (__i7)
/* @563 */ 	ldy          #1
/* @564 */ 	lda         __f2+1
/* @565 */ 	sta         (__i7), Y
/* @566 */ 	ldy          #2
/* @567 */ 	lda         __f2+2
/* @568 */ 	sta         (__i7), Y
/* @569 */ 	ldy          #3
/* @570 */ 	lda         __f2+3
/* @571 */ 	sta         (__i7), Y
/* @573 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @575 */ 	jsr         __pushf0
/* @576 */ 	jsr         __pushf1
/* @578 */ 	ldx         #__f0
/* @579 */ 	ldy          #0
/* @580 */ 	jsr         FPMultiply
/* @581 */ 	jsr         __incsp8
/* @583 */ 	lda          #__f0
/* @585 */ 	ldx          #43
/* @586 */ 	ldy          #1
/* @587 */ 	jsr         __set_var_value4b
/* @589 */ 	ldx          #39
	ldy          #1
	jsr          __var_value4b_f0			// c
/* @591 */ 	ldx          #43
	ldy          #1
	jsr          __var_value4b_f2			// d
/* @595 */ 	lda         __f0
/* @596 */ 	cmp         __f2
/* @597 */ 	bne         .main_label_594
/* @598 */ 	lda         __f0+1
/* @599 */ 	cmp         __f2+1
/* @600 */ 	bne         .main_label_594
/* @601 */ 	lda         __f0+2
/* @602 */ 	cmp         __f2+2
/* @603 */ 	bne         .main_label_594
/* @604 */ 	lda         __f0+3
/* @605 */ 	cmp         __f2+3
/* @606 */ 	bne         .main_label_1750
/* @1751 */ 	jmp         .main_label_694
.main_label_1750:
.main_label_594:
/* @611 */ 	ldx          #47
	ldy          #1
	jsr          __var_addrb_i0			// e
/* @615 */ 	lda         (__i7)
/* @616 */ 	sta         (__i0)
/* @617 */ 	ldy          #1
/* @618 */ 	lda         (__i7), Y
/* @619 */ 	ldy          #1
/* @620 */ 	sta         (__i0), Y
/* @621 */ 	ldy          #2
/* @622 */ 	lda         (__i7), Y
/* @623 */ 	ldy          #2
/* @624 */ 	sta         (__i0), Y
/* @625 */ 	ldy          #3
/* @626 */ 	lda         (__i7), Y
/* @627 */ 	ldy          #3
/* @628 */ 	sta         (__i0), Y
/* @630 */ 	ldx          #43
	ldy          #1
	jsr          __var_addrb_i0			// d
/* @632 */ 	ldx          #51
	ldy          #1
	jsr          __var_addrb_i1			// f
/* @636 */ 	lda         (__i0)
/* @637 */ 	sta         (__i1)
/* @638 */ 	ldy          #1
/* @639 */ 	lda         (__i0), Y
/* @640 */ 	ldy          #1
/* @641 */ 	sta         (__i1), Y
/* @642 */ 	ldy          #2
/* @643 */ 	lda         (__i0), Y
/* @644 */ 	ldy          #2
/* @645 */ 	sta         (__i1), Y
/* @646 */ 	ldy          #3
/* @647 */ 	lda         (__i0), Y
/* @648 */ 	ldy          #3
/* @649 */ 	sta         (__i1), Y
/* @652 */ 	lda         #%lo(.str.112)
/* @653 */ 	sta         __i0
/* @654 */ 	lda         #%hi(.str.112)
/* @655 */ 	sta         __i0+1
/* @657 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @659 */ 	ldx          #39
	ldy          #1
	jsr          __var_value4b_f2			// c
/* @661 */ 	ldx          #47
	ldy          #1
	jsr          __var_value4b_l0			// e
/* @663 */ 	ldx          #43
	ldy          #1
	jsr          __var_value4b_f3			// d
/* @665 */ 	ldx          #51
	ldy          #1
	jsr          __var_value4b_l1			// f
/* @667 */ 	jsr         __pushl1
/* @669 */ 	jsr         __pushf3
/* @671 */ 	jsr         __pushl0
/* @673 */ 	jsr         __pushf2
/* @675 */ 	jsr         __pushf0
/* @676 */ 	jsr         __pushf1
/* @678 */ 	jsr         __pushi0
/* @680 */ 	ldx         #__i0
/* @681 */ 	ldy          #0
/* @682 */ 	jsr         printf
/* @684 */ 	lda          #26
/* @685 */ 	sta         __t0
/* @686 */ 	jsr         __incsp
/* @688 */ 	lda          #0
/* @689 */ 	sta         __b0
/* @690 */ 	lda          #__b0
/* @691 */ 	ldx          #100
/* @692 */ 	ldy          #1
/* @693 */ 	jsr         __set_var_value1b
.main_label_694:
/* @697 */ 	lda         #%lo(.str.113)
/* @698 */ 	sta         __i0
/* @699 */ 	lda         #%hi(.str.113)
/* @700 */ 	sta         __i0+1
/* @701 */ 	jsr         __pushi4
/* @703 */ 	jsr         __pushi0
/* @705 */ 	ldx         #__i0
/* @706 */ 	ldy          #0
/* @707 */ 	jsr         printf
/* @708 */ 	jsr         __incsp4
/* @710 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @717 */ 	lda          #__f2
/* @718 */ 	ldx          #__f1
/* @719 */ 	ldy          #__f0
/* @720 */ 	jsr         __fdiv
/* @722 */ 	ldy          #0
/* @723 */ 	lda         __f2
/* @724 */ 	sta         (__i7)
/* @725 */ 	ldy          #1
/* @726 */ 	lda         __f2+1
/* @727 */ 	sta         (__i7), Y
/* @728 */ 	ldy          #2
/* @729 */ 	lda         __f2+2
/* @730 */ 	sta         (__i7), Y
/* @731 */ 	ldy          #3
/* @732 */ 	lda         __f2+3
/* @733 */ 	sta         (__i7), Y
/* @735 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @737 */ 	jsr         __pushf0
/* @738 */ 	jsr         __pushf1
/* @740 */ 	ldx         #__f0
/* @741 */ 	ldy          #0
/* @742 */ 	jsr         FPDivide
/* @743 */ 	jsr         __incsp8
/* @745 */ 	lda          #__f0
/* @746 */ 	ldx          #43
/* @747 */ 	ldy          #1
/* @748 */ 	jsr         __set_var_value4b
/* @750 */ 	ldx          #39
	ldy          #1
	jsr          __var_value4b_f0			// c
/* @752 */ 	ldx          #43
	ldy          #1
	jsr          __var_value4b_f2			// d
/* @756 */ 	lda         __f0
/* @757 */ 	cmp         __f2
/* @758 */ 	bne         .main_label_755
/* @759 */ 	lda         __f0+1
/* @760 */ 	cmp         __f2+1
/* @761 */ 	bne         .main_label_755
/* @762 */ 	lda         __f0+2
/* @763 */ 	cmp         __f2+2
/* @764 */ 	bne         .main_label_755
/* @765 */ 	lda         __f0+3
/* @766 */ 	cmp         __f2+3
/* @767 */ 	bne         .main_label_1752
/* @1753 */ 	jmp         .main_label_854
.main_label_1752:
.main_label_755:
/* @772 */ 	ldx          #55
	ldy          #1
	jsr          __var_addrb_i0			// e
/* @776 */ 	lda         (__i7)
/* @777 */ 	sta         (__i0)
/* @778 */ 	ldy          #1
/* @779 */ 	lda         (__i7), Y
/* @780 */ 	ldy          #1
/* @781 */ 	sta         (__i0), Y
/* @782 */ 	ldy          #2
/* @783 */ 	lda         (__i7), Y
/* @784 */ 	ldy          #2
/* @785 */ 	sta         (__i0), Y
/* @786 */ 	ldy          #3
/* @787 */ 	lda         (__i7), Y
/* @788 */ 	ldy          #3
/* @789 */ 	sta         (__i0), Y
/* @791 */ 	ldx          #43
	ldy          #1
	jsr          __var_addrb_i0			// d
/* @793 */ 	ldx          #59
	ldy          #1
	jsr          __var_addrb_i1			// f
/* @797 */ 	lda         (__i0)
/* @798 */ 	sta         (__i1)
/* @799 */ 	ldy          #1
/* @800 */ 	lda         (__i0), Y
/* @801 */ 	ldy          #1
/* @802 */ 	sta         (__i1), Y
/* @803 */ 	ldy          #2
/* @804 */ 	lda         (__i0), Y
/* @805 */ 	ldy          #2
/* @806 */ 	sta         (__i1), Y
/* @807 */ 	ldy          #3
/* @808 */ 	lda         (__i0), Y
/* @809 */ 	ldy          #3
/* @810 */ 	sta         (__i1), Y
/* @813 */ 	lda         #%lo(.str.114)
/* @814 */ 	sta         __i0
/* @815 */ 	lda         #%hi(.str.114)
/* @816 */ 	sta         __i0+1
/* @818 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @820 */ 	ldx          #39
	ldy          #1
	jsr          __var_value4b_f2			// c
/* @822 */ 	ldx          #55
	ldy          #1
	jsr          __var_value4b_l0			// e
/* @824 */ 	ldx          #43
	ldy          #1
	jsr          __var_value4b_f3			// d
/* @826 */ 	ldx          #59
	ldy          #1
	jsr          __var_value4b_l1			// f
/* @828 */ 	jsr         __pushl1
/* @830 */ 	jsr         __pushf3
/* @832 */ 	jsr         __pushl0
/* @834 */ 	jsr         __pushf2
/* @836 */ 	jsr         __pushf0
/* @837 */ 	jsr         __pushf1
/* @839 */ 	jsr         __pushi0
/* @841 */ 	ldx         #__i0
/* @842 */ 	ldy          #0
/* @843 */ 	jsr         printf
/* @844 */ 	lda          #26
/* @845 */ 	sta         __t0
/* @846 */ 	jsr         __incsp
/* @848 */ 	lda          #0
/* @849 */ 	sta         __b0
/* @850 */ 	lda          #__b0
/* @851 */ 	ldx          #100
/* @852 */ 	ldy          #1
/* @853 */ 	jsr         __set_var_value1b
.main_label_854:
/* @857 */ 	lda         #%lo(.str.115)
/* @858 */ 	sta         __i0
/* @859 */ 	lda         #%hi(.str.115)
/* @860 */ 	sta         __i0+1
/* @861 */ 	jsr         __pushi4
/* @863 */ 	jsr         __pushi0
/* @865 */ 	ldx         #__i0
/* @866 */ 	ldy          #0
/* @867 */ 	jsr         printf
/* @868 */ 	jsr         __incsp4
/* @870 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @877 */ 	lda          #__f2
/* @878 */ 	ldx          #__f1
/* @879 */ 	ldy          #__f0
/* @880 */ 	jsr         __fadd
/* @882 */ 	ldy          #0
/* @883 */ 	lda         __f2
/* @884 */ 	sta         (__i7)
/* @885 */ 	ldy          #1
/* @886 */ 	lda         __f2+1
/* @887 */ 	sta         (__i7), Y
/* @888 */ 	ldy          #2
/* @889 */ 	lda         __f2+2
/* @890 */ 	sta         (__i7), Y
/* @891 */ 	ldy          #3
/* @892 */ 	lda         __f2+3
/* @893 */ 	sta         (__i7), Y
/* @895 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @897 */ 	jsr         __pushf0
/* @898 */ 	jsr         __pushf1
/* @900 */ 	ldx         #__f0
/* @901 */ 	ldy          #0
/* @902 */ 	jsr         FPAdd
/* @903 */ 	jsr         __incsp8
/* @905 */ 	lda          #__f0
/* @906 */ 	ldx          #43
/* @907 */ 	ldy          #1
/* @908 */ 	jsr         __set_var_value4b
/* @910 */ 	ldx          #39
	ldy          #1
	jsr          __var_value4b_f0			// c
/* @912 */ 	ldx          #43
	ldy          #1
	jsr          __var_value4b_f2			// d
/* @916 */ 	lda         __f0
/* @917 */ 	cmp         __f2
/* @918 */ 	bne         .main_label_915
/* @919 */ 	lda         __f0+1
/* @920 */ 	cmp         __f2+1
/* @921 */ 	bne         .main_label_915
/* @922 */ 	lda         __f0+2
/* @923 */ 	cmp         __f2+2
/* @924 */ 	bne         .main_label_915
/* @925 */ 	lda         __f0+3
/* @926 */ 	cmp         __f2+3
/* @927 */ 	bne         .main_label_1754
/* @1755 */ 	jmp         .main_label_1014
.main_label_1754:
.main_label_915:
/* @932 */ 	ldx          #63
	ldy          #1
	jsr          __var_addrb_i0			// e
/* @936 */ 	lda         (__i7)
/* @937 */ 	sta         (__i0)
/* @938 */ 	ldy          #1
/* @939 */ 	lda         (__i7), Y
/* @940 */ 	ldy          #1
/* @941 */ 	sta         (__i0), Y
/* @942 */ 	ldy          #2
/* @943 */ 	lda         (__i7), Y
/* @944 */ 	ldy          #2
/* @945 */ 	sta         (__i0), Y
/* @946 */ 	ldy          #3
/* @947 */ 	lda         (__i7), Y
/* @948 */ 	ldy          #3
/* @949 */ 	sta         (__i0), Y
/* @951 */ 	ldx          #43
	ldy          #1
	jsr          __var_addrb_i0			// d
/* @953 */ 	ldx          #67
	ldy          #1
	jsr          __var_addrb_i1			// f
/* @957 */ 	lda         (__i0)
/* @958 */ 	sta         (__i1)
/* @959 */ 	ldy          #1
/* @960 */ 	lda         (__i0), Y
/* @961 */ 	ldy          #1
/* @962 */ 	sta         (__i1), Y
/* @963 */ 	ldy          #2
/* @964 */ 	lda         (__i0), Y
/* @965 */ 	ldy          #2
/* @966 */ 	sta         (__i1), Y
/* @967 */ 	ldy          #3
/* @968 */ 	lda         (__i0), Y
/* @969 */ 	ldy          #3
/* @970 */ 	sta         (__i1), Y
/* @973 */ 	lda         #%lo(.str.116)
/* @974 */ 	sta         __i0
/* @975 */ 	lda         #%hi(.str.116)
/* @976 */ 	sta         __i0+1
/* @978 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @980 */ 	ldx          #39
	ldy          #1
	jsr          __var_value4b_f2			// c
/* @982 */ 	ldx          #63
	ldy          #1
	jsr          __var_value4b_l0			// e
/* @984 */ 	ldx          #43
	ldy          #1
	jsr          __var_value4b_f3			// d
/* @986 */ 	ldx          #67
	ldy          #1
	jsr          __var_value4b_l1			// f
/* @988 */ 	jsr         __pushl1
/* @990 */ 	jsr         __pushf3
/* @992 */ 	jsr         __pushl0
/* @994 */ 	jsr         __pushf2
/* @996 */ 	jsr         __pushf0
/* @997 */ 	jsr         __pushf1
/* @999 */ 	jsr         __pushi0
/* @1001 */ 	ldx         #__i0
/* @1002 */ 	ldy          #0
/* @1003 */ 	jsr         printf
/* @1004 */ 	lda          #26
/* @1005 */ 	sta         __t0
/* @1006 */ 	jsr         __incsp
/* @1008 */ 	lda          #0
/* @1009 */ 	sta         __b0
/* @1010 */ 	lda          #__b0
/* @1011 */ 	ldx          #100
/* @1012 */ 	ldy          #1
/* @1013 */ 	jsr         __set_var_value1b
.main_label_1014:
/* @1017 */ 	lda         #%lo(.str.117)
/* @1018 */ 	sta         __i0
/* @1019 */ 	lda         #%hi(.str.117)
/* @1020 */ 	sta         __i0+1
/* @1021 */ 	jsr         __pushi4
/* @1023 */ 	jsr         __pushi0
/* @1025 */ 	ldx         #__i0
/* @1026 */ 	ldy          #0
/* @1027 */ 	jsr         printf
/* @1028 */ 	jsr         __incsp4
/* @1030 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @1037 */ 	lda          #__f2
/* @1038 */ 	ldx          #__f1
/* @1039 */ 	ldy          #__f0
/* @1041 */ 	jsr         __fsub
/* @1043 */ 	ldy          #0
/* @1044 */ 	lda         __f2
/* @1045 */ 	sta         (__i7)
/* @1046 */ 	ldy          #1
/* @1047 */ 	lda         __f2+1
/* @1048 */ 	sta         (__i7), Y
/* @1049 */ 	ldy          #2
/* @1050 */ 	lda         __f2+2
/* @1051 */ 	sta         (__i7), Y
/* @1052 */ 	ldy          #3
/* @1053 */ 	lda         __f2+3
/* @1054 */ 	sta         (__i7), Y
/* @1056 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @1058 */ 	jsr         __pushf0
/* @1059 */ 	jsr         __pushf1
/* @1061 */ 	ldx         #__f0
/* @1062 */ 	ldy          #0
/* @1063 */ 	jsr         FPSub
/* @1064 */ 	jsr         __incsp8
/* @1066 */ 	lda          #__f0
/* @1067 */ 	ldx          #43
/* @1068 */ 	ldy          #1
/* @1069 */ 	jsr         __set_var_value4b
/* @1071 */ 	ldx          #39
	ldy          #1
	jsr          __var_value4b_f0			// c
/* @1073 */ 	ldx          #43
	ldy          #1
	jsr          __var_value4b_f2			// d
/* @1077 */ 	lda         __f0
/* @1078 */ 	cmp         __f2
/* @1079 */ 	bne         .main_label_1076
/* @1080 */ 	lda         __f0+1
/* @1081 */ 	cmp         __f2+1
/* @1082 */ 	bne         .main_label_1076
/* @1083 */ 	lda         __f0+2
/* @1084 */ 	cmp         __f2+2
/* @1085 */ 	bne         .main_label_1076
/* @1086 */ 	lda         __f0+3
/* @1087 */ 	cmp         __f2+3
/* @1088 */ 	bne         .main_label_1756
/* @1757 */ 	jmp         .main_label_1175
.main_label_1756:
.main_label_1076:
/* @1093 */ 	ldx          #71
	ldy          #1
	jsr          __var_addrb_i0			// e
/* @1097 */ 	lda         (__i7)
/* @1098 */ 	sta         (__i0)
/* @1099 */ 	ldy          #1
/* @1100 */ 	lda         (__i7), Y
/* @1101 */ 	ldy          #1
/* @1102 */ 	sta         (__i0), Y
/* @1103 */ 	ldy          #2
/* @1104 */ 	lda         (__i7), Y
/* @1105 */ 	ldy          #2
/* @1106 */ 	sta         (__i0), Y
/* @1107 */ 	ldy          #3
/* @1108 */ 	lda         (__i7), Y
/* @1109 */ 	ldy          #3
/* @1110 */ 	sta         (__i0), Y
/* @1112 */ 	ldx          #43
	ldy          #1
	jsr          __var_addrb_i0			// d
/* @1114 */ 	ldx          #75
	ldy          #1
	jsr          __var_addrb_i1			// f
/* @1118 */ 	lda         (__i0)
/* @1119 */ 	sta         (__i1)
/* @1120 */ 	ldy          #1
/* @1121 */ 	lda         (__i0), Y
/* @1122 */ 	ldy          #1
/* @1123 */ 	sta         (__i1), Y
/* @1124 */ 	ldy          #2
/* @1125 */ 	lda         (__i0), Y
/* @1126 */ 	ldy          #2
/* @1127 */ 	sta         (__i1), Y
/* @1128 */ 	ldy          #3
/* @1129 */ 	lda         (__i0), Y
/* @1130 */ 	ldy          #3
/* @1131 */ 	sta         (__i1), Y
/* @1134 */ 	lda         #%lo(.str.118)
/* @1135 */ 	sta         __i0
/* @1136 */ 	lda         #%hi(.str.118)
/* @1137 */ 	sta         __i0+1
/* @1139 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @1141 */ 	ldx          #39
	ldy          #1
	jsr          __var_value4b_f2			// c
/* @1143 */ 	ldx          #71
	ldy          #1
	jsr          __var_value4b_l0			// e
/* @1145 */ 	ldx          #43
	ldy          #1
	jsr          __var_value4b_f3			// d
/* @1147 */ 	ldx          #75
	ldy          #1
	jsr          __var_value4b_l1			// f
/* @1149 */ 	jsr         __pushl1
/* @1151 */ 	jsr         __pushf3
/* @1153 */ 	jsr         __pushl0
/* @1155 */ 	jsr         __pushf2
/* @1157 */ 	jsr         __pushf0
/* @1158 */ 	jsr         __pushf1
/* @1160 */ 	jsr         __pushi0
/* @1162 */ 	ldx         #__i0
/* @1163 */ 	ldy          #0
/* @1164 */ 	jsr         printf
/* @1165 */ 	lda          #26
/* @1166 */ 	sta         __t0
/* @1167 */ 	jsr         __incsp
/* @1169 */ 	lda          #0
/* @1170 */ 	sta         __b0
/* @1171 */ 	lda          #__b0
/* @1172 */ 	ldx          #100
/* @1173 */ 	ldy          #1
/* @1174 */ 	jsr         __set_var_value1b
.main_label_1175:
/* @1178 */ 	lda         #%lo(.str.119)
/* @1179 */ 	sta         __i0
/* @1180 */ 	lda         #%hi(.str.119)
/* @1181 */ 	sta         __i0+1
/* @1182 */ 	jsr         __pushi4
/* @1184 */ 	jsr         __pushi0
/* @1186 */ 	ldx         #__i0
/* @1187 */ 	ldy          #0
/* @1188 */ 	jsr         printf
/* @1189 */ 	jsr         __incsp4
/* @1191 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @1193 */ 	ldx          #0
/* @1196 */ 	lda         __f1
/* @1197 */ 	cmp         __f0
/* @1198 */ 	lda         __f1+1
/* @1199 */ 	sbc         __f0+1
/* @1200 */ 	lda         __f1+2
/* @1201 */ 	sbc         __f0+2
/* @1202 */ 	lda         __f1+3
/* @1203 */ 	sbc         __f0+3
/* @1204 */ 	bvc         .main_label_1195
/* @1205 */ 	eor          #128
.main_label_1195:
/* @1206 */ 	bpl         .main_label_1192
/* @1207 */ 	inx         
.main_label_1192:
/* @1208 */ 	stx         __b2
/* @1210 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @1212 */ 	jsr         __pushf0
/* @1213 */ 	jsr         __pushf1
/* @1215 */ 	ldx         #__b0
/* @1216 */ 	ldy          #0
/* @1217 */ 	jsr         FPLess
/* @1218 */ 	jsr         __incsp8
/* @1221 */ 	lda         __b0
/* @1222 */ 	sta         __b3
/* @1224 */ 	lda         __b2
/* @1225 */ 	sta         __i0
/* @1226 */ 	lda          #0
/* @1227 */ 	sta         __i0+1
/* @1229 */ 	lda         __b3
/* @1230 */ 	sta         __i1
/* @1231 */ 	lda          #0
/* @1232 */ 	sta         __i1+1
/* @1236 */ 	lda         __i0
/* @1237 */ 	cmp         __i1
/* @1238 */ 	bne         .main_label_1235
/* @1239 */ 	lda         __i0+1
/* @1240 */ 	cmp         __i1+1
/* @1241 */ 	beq         .main_label_1274
.main_label_1235:
/* @1245 */ 	lda         #%lo(.str.120)
/* @1246 */ 	sta         __i0
/* @1247 */ 	lda         #%hi(.str.120)
/* @1248 */ 	sta         __i0+1
/* @1250 */ 	ldx          #31
	ldy          #1
	jsr          __var_value4b_f0			// b
/* @1251 */ 	lda         __b3
/* @1253 */ 	jsr         __pusha
/* @1254 */ 	lda         __b2
/* @1255 */ 	jsr         __pusha
/* @1257 */ 	jsr         __pushf0
/* @1258 */ 	jsr         __pushf1
/* @1260 */ 	jsr         __pushi0
/* @1262 */ 	ldx         #__i0
/* @1263 */ 	ldy          #0
/* @1264 */ 	jsr         printf
/* @1266 */ 	jsr         __incsp14
/* @1268 */ 	lda          #0
/* @1269 */ 	sta         __b0
/* @1270 */ 	lda          #__b0
/* @1271 */ 	ldx          #100
/* @1272 */ 	ldy          #1
/* @1273 */ 	jsr         __set_var_value1b
.main_label_1274:
/* @1277 */ 	lda         #%lo(.str.121)
/* @1278 */ 	sta         __i0
/* @1279 */ 	lda         #%hi(.str.121)
/* @1280 */ 	sta         __i0+1
/* @1281 */ 	jsr         __pushi4
/* @1283 */ 	jsr         __pushi0
/* @1285 */ 	ldx         #__i0
/* @1286 */ 	ldy          #0
/* @1287 */ 	jsr         printf
/* @1288 */ 	jsr         __incsp4
/* @1290 */ 	ldx          #35
	ldy          #1
	jsr          __var_value4b_l0			// v
/* @1296 */ 	lda          #__i7
/* @1297 */ 	ldx          #__l0
/* @1299 */ 	jsr         __i4tof
/* @1301 */ 	ldx          #35
	ldy          #1
	jsr          __var_value4b_l0			// v
/* @1303 */ 	jsr         __pushl0
/* @1305 */ 	ldx         #__f0
/* @1306 */ 	ldy          #0
/* @1307 */ 	jsr         ToFloat
/* @1308 */ 	jsr         __incsp4
/* @1310 */ 	lda          #__f0
/* @1311 */ 	ldx          #43
/* @1312 */ 	ldy          #1
/* @1313 */ 	jsr         __set_var_value4b
/* @1315 */ 	ldx          #39
	ldy          #1
	jsr          __var_value4b_f0			// c
/* @1317 */ 	ldx          #43
	ldy          #1
	jsr          __var_value4b_f2			// d
/* @1321 */ 	lda         __f0
/* @1322 */ 	cmp         __f2
/* @1323 */ 	bne         .main_label_1320
/* @1324 */ 	lda         __f0+1
/* @1325 */ 	cmp         __f2+1
/* @1326 */ 	bne         .main_label_1320
/* @1327 */ 	lda         __f0+2
/* @1328 */ 	cmp         __f2+2
/* @1329 */ 	bne         .main_label_1320
/* @1330 */ 	lda         __f0+3
/* @1331 */ 	cmp         __f2+3
/* @1332 */ 	bne         .main_label_1758
/* @1759 */ 	jmp         .main_label_1419
.main_label_1758:
.main_label_1320:
/* @1337 */ 	ldx          #79
	ldy          #1
	jsr          __var_addrb_i0			// e
/* @1341 */ 	lda         (__i7)
/* @1342 */ 	sta         (__i0)
/* @1343 */ 	ldy          #1
/* @1344 */ 	lda         (__i7), Y
/* @1345 */ 	ldy          #1
/* @1346 */ 	sta         (__i0), Y
/* @1347 */ 	ldy          #2
/* @1348 */ 	lda         (__i7), Y
/* @1349 */ 	ldy          #2
/* @1350 */ 	sta         (__i0), Y
/* @1351 */ 	ldy          #3
/* @1352 */ 	lda         (__i7), Y
/* @1353 */ 	ldy          #3
/* @1354 */ 	sta         (__i0), Y
/* @1356 */ 	ldx          #43
	ldy          #1
	jsr          __var_addrb_i0			// d
/* @1358 */ 	ldx          #83
	ldy          #1
	jsr          __var_addrb_i1			// f
/* @1362 */ 	lda         (__i0)
/* @1363 */ 	sta         (__i1)
/* @1364 */ 	ldy          #1
/* @1365 */ 	lda         (__i0), Y
/* @1366 */ 	ldy          #1
/* @1367 */ 	sta         (__i1), Y
/* @1368 */ 	ldy          #2
/* @1369 */ 	lda         (__i0), Y
/* @1370 */ 	ldy          #2
/* @1371 */ 	sta         (__i1), Y
/* @1372 */ 	ldy          #3
/* @1373 */ 	lda         (__i0), Y
/* @1374 */ 	ldy          #3
/* @1375 */ 	sta         (__i1), Y
/* @1378 */ 	lda         #%lo(.str.122)
/* @1379 */ 	sta         __i0
/* @1380 */ 	lda         #%hi(.str.122)
/* @1381 */ 	sta         __i0+1
/* @1383 */ 	ldx          #35
	ldy          #1
	jsr          __var_value4b_l0			// v
/* @1385 */ 	ldx          #39
	ldy          #1
	jsr          __var_value4b_f0			// c
/* @1387 */ 	ldx          #79
	ldy          #1
	jsr          __var_value4b_l1			// e
/* @1389 */ 	ldx          #43
	ldy          #1
	jsr          __var_value4b_f2			// d
/* @1391 */ 	ldx          #83
	ldy          #1
	jsr          __var_value4b_l2			// f
/* @1393 */ 	jsr         __pushl2
/* @1395 */ 	jsr         __pushf2
/* @1397 */ 	jsr         __pushl1
/* @1399 */ 	jsr         __pushf0
/* @1401 */ 	jsr         __pushl0
/* @1403 */ 	jsr         __pushi0
/* @1405 */ 	ldx         #__i0
/* @1406 */ 	ldy          #0
/* @1407 */ 	jsr         printf
/* @1409 */ 	lda          #22
/* @1410 */ 	sta         __t0
/* @1411 */ 	jsr         __incsp
/* @1413 */ 	lda          #0
/* @1414 */ 	sta         __b0
/* @1415 */ 	lda          #__b0
/* @1416 */ 	ldx          #100
/* @1417 */ 	ldy          #1
/* @1418 */ 	jsr         __set_var_value1b
.main_label_1419:
/* @1422 */ 	lda         #%lo(.str.123)
/* @1423 */ 	sta         __i0
/* @1424 */ 	lda         #%hi(.str.123)
/* @1425 */ 	sta         __i0+1
/* @1426 */ 	jsr         __pushi4
/* @1428 */ 	jsr         __pushi0
/* @1430 */ 	ldx         #__i0
/* @1431 */ 	ldy          #0
/* @1432 */ 	jsr         printf
/* @1433 */ 	jsr         __incsp4
/* @1437 */ 	lda          #__i8
/* @1438 */ 	ldx          #__f1
/* @1440 */ 	jsr         __ftoi4
/* @1441 */ 	jsr         __pushf1
/* @1443 */ 	ldx         #__l0
/* @1444 */ 	ldy          #0
/* @1445 */ 	jsr         FromFloat
/* @1446 */ 	jsr         __incsp4
/* @1448 */ 	lda          #__l0
/* @1450 */ 	ldx          #87
/* @1451 */ 	ldy          #1
/* @1452 */ 	jsr         __set_var_value4b
/* @1454 */ 	ldx          #35
	ldy          #1
	jsr          __var_value4b_l0			// v
/* @1456 */ 	ldx          #87
	ldy          #1
	jsr          __var_value4b_l1			// w
/* @1460 */ 	lda         __l0
/* @1461 */ 	cmp         __l1
/* @1462 */ 	bne         .main_label_1459
/* @1463 */ 	lda         __l0+1
/* @1464 */ 	cmp         __l1+1
/* @1465 */ 	bne         .main_label_1459
/* @1466 */ 	lda         __l0+2
/* @1467 */ 	cmp         __l1+2
/* @1468 */ 	bne         .main_label_1459
/* @1469 */ 	lda         __l0+3
/* @1470 */ 	cmp         __l1+3
/* @1471 */ 	bne         .main_label_1760
/* @1761 */ 	jmp         .main_label_1554
.main_label_1760:
.main_label_1459:
/* @1476 */ 	ldx          #91
	ldy          #1
	jsr          __var_addrb_i0			// e
/* @1480 */ 	lda         (__i8)
/* @1481 */ 	sta         (__i0)
/* @1482 */ 	ldy          #1
/* @1483 */ 	lda         (__i8), Y
/* @1484 */ 	ldy          #1
/* @1485 */ 	sta         (__i0), Y
/* @1486 */ 	ldy          #2
/* @1487 */ 	lda         (__i8), Y
/* @1488 */ 	ldy          #2
/* @1489 */ 	sta         (__i0), Y
/* @1490 */ 	ldy          #3
/* @1491 */ 	lda         (__i8), Y
/* @1492 */ 	ldy          #3
/* @1493 */ 	sta         (__i0), Y
/* @1495 */ 	ldx          #87
	ldy          #1
	jsr          __var_addrb_i0			// w
/* @1497 */ 	ldx          #95
	ldy          #1
	jsr          __var_addrb_i1			// f
/* @1501 */ 	lda         (__i0)
/* @1502 */ 	sta         (__i1)
/* @1503 */ 	ldy          #1
/* @1504 */ 	lda         (__i0), Y
/* @1505 */ 	ldy          #1
/* @1506 */ 	sta         (__i1), Y
/* @1507 */ 	ldy          #2
/* @1508 */ 	lda         (__i0), Y
/* @1509 */ 	ldy          #2
/* @1510 */ 	sta         (__i1), Y
/* @1511 */ 	ldy          #3
/* @1512 */ 	lda         (__i0), Y
/* @1513 */ 	ldy          #3
/* @1514 */ 	sta         (__i1), Y
/* @1517 */ 	lda         #%lo(.str.124)
/* @1518 */ 	sta         __i0
/* @1519 */ 	lda         #%hi(.str.124)
/* @1520 */ 	sta         __i0+1
/* @1522 */ 	ldx          #35
	ldy          #1
	jsr          __var_value4b_l0			// v
/* @1524 */ 	ldx          #91
	ldy          #1
	jsr          __var_value4b_l1			// e
/* @1526 */ 	ldx          #87
	ldy          #1
	jsr          __var_value4b_l2			// w
/* @1528 */ 	ldx          #95
	ldy          #1
	jsr          __var_value4b_l3			// f
/* @1530 */ 	jsr         __pushl3
/* @1532 */ 	jsr         __pushl2
/* @1534 */ 	jsr         __pushl1
/* @1536 */ 	jsr         __pushl0
/* @1537 */ 	jsr         __pushf1
/* @1539 */ 	jsr         __pushi0
/* @1541 */ 	ldx         #__i0
/* @1542 */ 	ldy          #0
/* @1543 */ 	jsr         printf
/* @1544 */ 	lda          #22
/* @1545 */ 	sta         __t0
/* @1546 */ 	jsr         __incsp
/* @1548 */ 	lda          #0
/* @1549 */ 	sta         __b0
/* @1550 */ 	lda          #__b0
/* @1551 */ 	ldx          #100
/* @1552 */ 	ldy          #1
/* @1553 */ 	jsr         __set_var_value1b
.main_label_1554:
.main_label_1555:
/* @1556 */ 	lda          #__i4
/* @1558 */ 	jsr         __rinc21
/* @1559 */ 	jmp         .main_label_325
.main_label_1560:
/* @1562 */ 	ldx          #100
	ldy          #1
	jsr          __var_value1b_b0			// ok
/* @1564 */ 	lda         __b0
/* @1565 */ 	cmp          #0
/* @1566 */ 	beq         .main_label_1581
/* @1569 */ 	lda         #%lo(.str.125)
/* @1570 */ 	sta         __i0
/* @1571 */ 	lda         #%hi(.str.125)
/* @1572 */ 	sta         __i0+1
/* @1574 */ 	jsr         __pushi0
/* @1576 */ 	ldx         #__i0
/* @1577 */ 	ldy          #0
/* @1578 */ 	jsr         printf
/* @1580 */ 	jsr         __incsp2
.main_label_1581:
/* @1584 */ 	lda         #%lo(.str.126)
/* @1585 */ 	sta         __i0
/* @1586 */ 	lda         #%hi(.str.126)
/* @1587 */ 	sta         __i0+1
/* @1589 */ 	jsr         __pushi0
/* @1591 */ 	ldx         #__f0
/* @1592 */ 	ldy          #0
/* @1593 */ 	jsr         ASCIIToFloat
/* @1594 */ 	jsr         __incsp2
/* @1596 */ 	lda          #__f0
/* @1598 */ 	ldx          #99
/* @1599 */ 	ldy          #1
/* @1600 */ 	jsr         __set_var_value4b
/* @1603 */ 	lda         #%lo(.str.127)
/* @1604 */ 	sta         __i0
/* @1605 */ 	lda         #%hi(.str.127)
/* @1606 */ 	sta         __i0+1
/* @1608 */ 	ldx          #99
	ldy          #1
	jsr          __var_value4b_f0			// f
/* @1610 */ 	jsr         __pushf0
/* @1612 */ 	jsr         __pushi0
/* @1614 */ 	ldx         #__i0
/* @1615 */ 	ldy          #0
/* @1616 */ 	jsr         printf
/* @1618 */ 	jsr         __incsp6
/* @1620 */ 	ldx          #99
	ldy          #1
	jsr          __var_value4b_f0			// f
/* @1622 */ 	ldx          #3
	ldy          #1
	jsr          __var_addrb_i6			// buf
/* @1623 */ 	ldx          #0
/* @1624 */ 	ldy          #1
/* @1626 */ 	jsr         __pushxy
/* @1628 */ 	jsr         __pushi6
/* @1630 */ 	ldx          #6
/* @1632 */ 	jsr         __pushxy0
/* @1634 */ 	jsr         __pushf0
/* @1636 */ 	ldx         #__i0
/* @1637 */ 	ldy          #0
/* @1638 */ 	jsr         FloatToASCII
/* @1640 */ 	jsr         __incsp10
/* @1643 */ 	lda         __i0
/* @1644 */ 	sta         __i5
/* @1645 */ 	lda         __i0+1
/* @1646 */ 	sta         __i5+1
/* @1649 */ 	lda         #%lo(.str.128)
/* @1650 */ 	sta         __i1
/* @1651 */ 	lda         #%hi(.str.128)
/* @1652 */ 	sta         __i1+1
/* @1653 */ 	jsr         __pushi5
/* @1655 */ 	jsr         __pushi1
/* @1657 */ 	ldx         #__i1
/* @1658 */ 	ldy          #0
/* @1659 */ 	jsr         printf
/* @1660 */ 	jsr         __incsp4
/* @1663 */ 	ldx          #0
/* @1664 */ 	ldy          #1
/* @1665 */ 	jsr         __pushxy
/* @1667 */ 	jsr         __pushi6
/* @1668 */ 	ldx          #6
/* @1669 */ 	jsr         __pushxy0
/* @1670 */ 	ldx         #%lo(.lit.136)
	ldy         #%hi(.lit.136)
/* @1672 */ 	jsr         __push4xy
/* @1674 */ 	ldx         #__i1
/* @1675 */ 	ldy          #0
/* @1676 */ 	jsr         FloatToASCII
/* @1677 */ 	jsr         __incsp10
/* @1680 */ 	lda         __i1
/* @1681 */ 	sta         __i5
/* @1682 */ 	lda         __i1+1
/* @1683 */ 	sta         __i5+1
/* @1686 */ 	lda         #%lo(.str.129)
/* @1687 */ 	sta         __i2
/* @1688 */ 	lda         #%hi(.str.129)
/* @1689 */ 	sta         __i2+1
/* @1690 */ 	jsr         __pushi5
/* @1692 */ 	jsr         __pushi2
/* @1694 */ 	ldx         #__i2
/* @1695 */ 	ldy          #0
/* @1696 */ 	jsr         printf
/* @1697 */ 	jsr         __incsp4
/* @1700 */ 	ldx          #0
/* @1701 */ 	ldy          #1
/* @1702 */ 	jsr         __pushxy
/* @1704 */ 	jsr         __pushi6
/* @1705 */ 	ldx          #6
/* @1706 */ 	jsr         __pushxy0
/* @1707 */ 	ldx         #%lo(.lit.137)
	ldy         #%hi(.lit.137)
/* @1708 */ 	jsr         __push4xy
/* @1710 */ 	ldx         #__i2
/* @1711 */ 	ldy          #0
/* @1712 */ 	jsr         FloatToASCIIScientific
/* @1713 */ 	jsr         __incsp10
/* @1716 */ 	lda         __i2
/* @1717 */ 	sta         __i5
/* @1718 */ 	lda         __i2+1
/* @1719 */ 	sta         __i5+1
/* @1722 */ 	lda         #%lo(.str.130)
/* @1723 */ 	sta         __i3
/* @1724 */ 	lda         #%hi(.str.130)
/* @1725 */ 	sta         __i3+1
/* @1726 */ 	jsr         __pushi5
/* @1728 */ 	jsr         __pushi3
/* @1730 */ 	ldx         #__i3
/* @1731 */ 	ldy          #0
/* @1732 */ 	jsr         printf
/* @1733 */ 	jsr         __incsp4
/* @1735 */ 	lda          #0
/* @1736 */ 	sta         __i0
/* @1737 */ 	lda          #0
/* @1738 */ 	sta         __i0+1
/* @1739 */ 	ldx          #109
	ldy          #1
	jsr          __load_result+2
/* @1740 */ 	lda         #__i0
/* @1742 */ 	jsr         __result2
/* @1743 */ 	ldy          #111
	ldx          #1
	jsr          __leave+2
/* @1744 */ 	rts         
.func_end_main:
	.size main, .func_end_main-main

	.data
	.section ".rodata", "aMS", @progbits
.str.107:
	.asciz "nan"
	.type .str.107, @object
	.size .str.107, 4

.str.108:
	.asciz "inf"
	.type .str.108, @object
	.size .str.108, 4

.str.110:
	.asciz "%g(%lx) and %g(%lx): right: %f(%lx), wrong: %f(%lx)\n"
	.type .str.110, @object
	.size .str.110, 53

.str.111:
	.asciz "multiply %d\n"
	.type .str.111, @object
	.size .str.111, 13

.str.112:
	.asciz "Multiply wrong %g and %g: right: %f(%lx), wrong: %f(%lx)\n"
	.type .str.112, @object
	.size .str.112, 58

.str.113:
	.asciz "divide %d\n"
	.type .str.113, @object
	.size .str.113, 11

.str.114:
	.asciz "Divide wrong %g and %g: right: %f(%lx), wrong: %f(%lx)\n"
	.type .str.114, @object
	.size .str.114, 56

.str.115:
	.asciz "add %d\n"
	.type .str.115, @object
	.size .str.115, 8

.str.116:
	.asciz "Add wrong %g and %g: right: %f(%lx), wrong: %f(%lx)\n"
	.type .str.116, @object
	.size .str.116, 53

.str.117:
	.asciz "sub %d\n"
	.type .str.117, @object
	.size .str.117, 8

.str.118:
	.asciz "Subtract wrong %g and %g: right: %f(%lx), wrong: %f(%lx)\n"
	.type .str.118, @object
	.size .str.118, 58

.str.119:
	.asciz "less %d\n"
	.type .str.119, @object
	.size .str.119, 9

.str.120:
	.asciz "Less wrong: %g and %g: right %d, wrong %d\n"
	.type .str.120, @object
	.size .str.120, 43

.str.121:
	.asciz "to float %d\n"
	.type .str.121, @object
	.size .str.121, 13

.str.122:
	.asciz "ToFloat wrong %d: right: %f(%lx), wrong: %f(%lx)\n"
	.type .str.122, @object
	.size .str.122, 50

.str.123:
	.asciz "from float %d\n"
	.type .str.123, @object
	.size .str.123, 15

.str.124:
	.asciz "FromFloat wrong %g: right: %d(%lx), wrong: %d(%lx)\n"
	.type .str.124, @object
	.size .str.124, 52

.str.125:
	.asciz "OK\n"
	.type .str.125, @object
	.size .str.125, 4

.str.126:
	.asciz "1.525"
	.type .str.126, @object
	.size .str.126, 6

.str.127:
	.asciz "%f\n"
	.type .str.127, @object
	.size .str.127, 4

.str.128:
	.asciz "%s\n"
	.type .str.128, @object
	.size .str.128, 4

.str.129:
	.asciz "%s\n"
	.type .str.129, @object
	.size .str.129, 4

.str.130:
	.asciz "%s\n"
	.type .str.130, @object
	.size .str.130, 4

.lit.136:
	.byte 0x6f
	.byte 0x12
	.byte 0x83
	.byte 0x3a
	.type .lit.136, @object
	.size .lit.136, 4

.lit.137:
	.byte 0xe2
	.byte 0x71
	.byte 0xc4
	.byte 0x79
	.type .lit.137, @object
	.size .lit.137, 4

