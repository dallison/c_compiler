	.file   "/Users/dallison/Google Drive/c_compiler/libc/ffuncs.c"
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


	.local  EnsureBuffer
	.type EnsureBuffer, @function

EnsureBuffer:
/* @1 */ 	rts         
.func_end_EnsureBuffer:
	.size EnsureBuffer, .func_end_EnsureBuffer-EnsureBuffer

	.global setbuf
	.type setbuf, @function

setbuf:
/* @7 */ 	ldx          #7
	jsr          __enter
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
/* @12 */ 	lda          #__i5			// stream
	ldx          #0
	jsr          __arg_value2
/* @16 */ 	lda          #__i6			// buf
	ldx          #2
	jsr          __arg_value2
/* @18 */ 	lda         __i6
/* @20 */ 	ora         __i6+1
/* @21 */ 	beq         .setbuf_label_29
/* @24 */ 	lda          #1
/* @25 */ 	sta         __i4
/* @26 */ 	lda          #0
/* @27 */ 	sta         __i4+1
/* @28 */ 	bra         .setbuf_label_36
.setbuf_label_29:
/* @32 */ 	lda          #3
/* @33 */ 	sta         __i4
/* @34 */ 	lda          #0
/* @35 */ 	sta         __i4+1
.setbuf_label_36:
/* @40 */ 	lda         __i4+1
/* @42 */ 	and          #128
/* @43 */ 	ora         __i4
/* @44 */ 	sta         __b0
/* @45 */ 	ldx          #128
/* @47 */ 	jsr         __pushxy0
/* @49 */ 	lda         __b0
/* @51 */ 	jsr         __pusha
/* @52 */ 	ldx         #__i6
/* @54 */ 	jsr         __pushreg2
/* @55 */ 	ldx         #__i5
/* @56 */ 	jsr         __pushreg2
/* @58 */ 	ldx         #__i0
/* @59 */ 	ldy          #0
/* @60 */ 	jsr         setvbuf
/* @62 */ 	ldx          #8
/* @64 */ 	jsr         __incsp
/* @65 */ 	ldy          #10
	jsr          __leave_void
/* @66 */ 	rts         
.func_end_setbuf:
	.size setbuf, .func_end_setbuf-setbuf

	.global setvbuf
	.type setvbuf, @function

setvbuf:
/* @11 */ 	stx         __result
/* @13 */ 	sty         __result+1
/* @14 */ 	ldx          #7
	jsr          __enter
	.byte        0x63,0x00,0x00		// Save mask i:3 b:3 l:0 x:0 f:0 
/* @19 */ 	lda          #__b2			// mode
	ldx          #4
	jsr          __arg_value1
/* @23 */ 	lda          #__i4			// stream
	ldx          #0
	jsr          __arg_value2
/* @27 */ 	lda          #__i5			// buf
	ldx          #2
	jsr          __arg_value2
/* @31 */ 	lda          #__i6			// size
	ldx          #6
	jsr          __arg_value2
/* @40 */ 	ldx          #0
/* @41 */ 	lda         __b2
/* @42 */ 	cmp          #3
/* @43 */ 	beq         .setvbuf_label_37
.setvbuf_label_38:
/* @44 */ 	inx         
.setvbuf_label_37:
/* @45 */ 	stx         __b4
/* @47 */ 	lda         __b4
/* @48 */ 	beq         .setvbuf_label_58
/* @52 */ 	ldx          #0
/* @53 */ 	lda         __b2
/* @54 */ 	cmp          #2
/* @55 */ 	beq         .setvbuf_label_50
.setvbuf_label_51:
/* @56 */ 	inx         
.setvbuf_label_50:
/* @57 */ 	stx         __b4
.setvbuf_label_58:
/* @62 */ 	lda         __b4
/* @63 */ 	sta         __b3
/* @65 */ 	lda         __b4
/* @66 */ 	beq         .setvbuf_label_76
/* @70 */ 	ldx          #0
/* @71 */ 	lda         __b2
/* @72 */ 	cmp          #1
/* @73 */ 	beq         .setvbuf_label_68
.setvbuf_label_69:
/* @74 */ 	inx         
.setvbuf_label_68:
/* @75 */ 	stx         __b3
.setvbuf_label_76:
/* @78 */ 	lda         __b3
/* @79 */ 	beq         .setvbuf_label_93
/* @82 */ 	lda          #255
/* @83 */ 	sta         __i0
/* @84 */ 	lda          #255
/* @85 */ 	sta         __i0+1
/* @86 */ 	ldx          #8
	jsr          __load_result
/* @87 */ 	lda         #__i0
/* @89 */ 	jsr         __result2
.setvbuf_label_90:
/* @91 */ 	ldy          #10
	jsr          __leave
/* @92 */ 	rts         
.setvbuf_label_93:
/* @96 */ 	ldy          #2
/* @97 */ 	lda         (__i4), Y
/* @98 */ 	sta         __i0
/* @99 */ 	ldy          #3
/* @100 */ 	lda         (__i4), Y
/* @101 */ 	sta         __i0+1
/* @104 */ 	lda         __i0
/* @105 */ 	cmp          #0
/* @106 */ 	bne         .setvbuf_label_103
/* @107 */ 	lda         __i0+1
/* @108 */ 	cmp          #0
/* @109 */ 	beq         .setvbuf_label_127
.setvbuf_label_103:
/* @113 */ 	ldy          #2
/* @114 */ 	lda         (__i4), Y
/* @115 */ 	sta         __i0
/* @116 */ 	ldy          #3
/* @117 */ 	lda         (__i4), Y
/* @118 */ 	sta         __i0+1
/* @120 */ 	ldx         #__i0
/* @122 */ 	jsr         __pushreg2
/* @123 */ 	jsr         free
/* @124 */ 	ldx          #2
/* @126 */ 	jsr         __incsp
.setvbuf_label_127:
/* @128 */ 	lda         __i5
/* @129 */ 	ldy          #2
/* @130 */ 	sta         (__i4), Y
/* @131 */ 	lda         __i5+1
/* @132 */ 	ldy          #3
/* @133 */ 	sta         (__i4), Y
/* @134 */ 	lda         __i6
/* @136 */ 	ldy          #4
/* @137 */ 	sta         (__i4), Y
/* @138 */ 	lda         __i6+1
/* @140 */ 	ldy          #5
/* @141 */ 	sta         (__i4), Y
/* @144 */ 	lda         __b2
/* @145 */ 	sta         __i0
/* @147 */ 	and          #128
/* @149 */ 	beq         .setvbuf_label_148
/* @150 */ 	lda          #255
.setvbuf_label_148:
/* @151 */ 	sta         __i0+1
/* @154 */ 	lda         __i0
/* @156 */ 	ldy          #12
/* @157 */ 	sta         (__i4), Y
/* @158 */ 	lda         __i0+1
/* @160 */ 	ldy          #13
/* @161 */ 	sta         (__i4), Y
/* @163 */ 	lda          #0
/* @164 */ 	sta         __i0
/* @165 */ 	lda          #0
/* @166 */ 	sta         __i0+1
/* @167 */ 	ldx          #8
	jsr          __load_result
/* @168 */ 	lda         #__i0
/* @169 */ 	jsr         __result2
/* @170 */ 	bra         .setvbuf_label_90
.func_end_setvbuf:
	.size setvbuf, .func_end_setvbuf-setvbuf

	.global fwrite
	.type fwrite, @function

fwrite:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x07,0x00,0x00		// Save mask i:7 b:0 l:0 x:0 f:0 
/* @14 */ 	lda          #__i4			// stream
	ldx          #6
	jsr          __arg_value2
/* @18 */ 	lda          #__i0			// ptr
	ldx          #0
	jsr          __arg_value2
/* @25 */ 	lda          #__i6			// size
	ldx          #2
	jsr          __arg_value2
/* @29 */ 	lda          #__i7			// n
	ldx          #4
	jsr          __arg_value2
/* @37 */ 	lda         __i0
/* @38 */ 	sta         __i10
/* @40 */ 	lda         __i0+1
/* @41 */ 	sta         __i10+1
/* @42 */ 	lda          #__i5
/* @43 */ 	ldx          #__i6
/* @44 */ 	ldy          #__i7
/* @46 */ 	jsr         __umul2
/* @47 */ 	lda          #0
/* @48 */ 	sta         __i8
/* @49 */ 	lda          #0
/* @50 */ 	sta         __i8+1
.fwrite_label_51:
/* @52 */ 	lda         __i5
/* @53 */ 	ora         __i5+1
/* @54 */ 	beq         .fwrite_label_113
/* @57 */ 	lda         __i10
/* @58 */ 	sta         __i0
/* @59 */ 	lda         __i10+1
/* @60 */ 	sta         __i0+1
/* @61 */ 	lda          #__i10
/* @63 */ 	jsr         __rinc21
/* @68 */ 	lda         (__i0)
/* @69 */ 	sta         __b0
/* @70 */ 	ldx         #__i4
/* @72 */ 	jsr         __pushreg2
/* @74 */ 	lda         __b0
/* @76 */ 	jsr         __pusha
/* @78 */ 	ldx         #__i0
/* @79 */ 	ldy          #0
/* @80 */ 	jsr         fputc
/* @82 */ 	ldx          #4
/* @84 */ 	jsr         __incsp
/* @87 */ 	lda         __i0
/* @88 */ 	sta         __i9
/* @89 */ 	lda         __i0+1
/* @90 */ 	sta         __i9+1
/* @92 */ 	lda         __i9
/* @93 */ 	cmp          #255
/* @94 */ 	bne         .fwrite_label_106
/* @95 */ 	lda         __i9+1
/* @96 */ 	cmp          #255
/* @97 */ 	bne         .fwrite_label_106
.fwrite_label_91:
/* @99 */ 	ldx          #8
	jsr          __load_result
/* @100 */ 	lda         #__i7
/* @102 */ 	jsr         __result2
.fwrite_label_103:
/* @104 */ 	ldy          #10
	jsr          __leave
/* @105 */ 	rts         
.fwrite_label_106:
/* @107 */ 	lda          #__i8
/* @108 */ 	jsr         __rinc21
/* @109 */ 	lda          #__i5
/* @111 */ 	jsr         __rdec21
/* @112 */ 	bra         .fwrite_label_51
.fwrite_label_113:
/* @116 */ 	lda          #__i0
/* @117 */ 	ldx          #__i8
/* @118 */ 	ldy          #__i6
/* @120 */ 	jsr         __sdiv2
/* @122 */ 	ldx          #8
	jsr          __load_result
/* @123 */ 	lda         #__i0
/* @124 */ 	jsr         __result2
/* @125 */ 	bra         .fwrite_label_103
.func_end_fwrite:
	.size fwrite, .func_end_fwrite-fwrite

	.global fputc
	.type fputc, @function

fputc:
/* @13 */ 	stx         __result
/* @15 */ 	sty         __result+1
/* @16 */ 	ldx          #17
	jsr          __enter
	.byte        0x63,0x00,0x00		// Save mask i:3 b:3 l:0 x:0 f:0 
/* @23 */ 	lda          #__i4			// stream
	ldx          #2
	jsr          __arg_value2
/* @29 */ 	lda          #__b3			// c
	ldx          #0
	jsr          __arg_value1
/* @35 */ 	ldx         #__i4
/* @37 */ 	jsr         __pushreg2
/* @38 */ 	jsr         EnsureBuffer
/* @40 */ 	ldx          #2
/* @42 */ 	jsr         __incsp
/* @45 */ 	ldy          #2
/* @46 */ 	lda         (__i4), Y
/* @47 */ 	sta         __i0
/* @49 */ 	ldy          #3
/* @50 */ 	lda         (__i4), Y
/* @52 */ 	sta         __i0+1
/* @55 */ 	lda         __i0
/* @56 */ 	cmp          #0
/* @57 */ 	beq         .fputc_label_320
/* @321 */ 	jmp         .fputc_label_150
.fputc_label_320:
/* @58 */ 	lda         __i0+1
/* @59 */ 	cmp          #0
/* @60 */ 	bne         .fputc_label_150
.fputc_label_54:
/* @62 */ 	lda          #10
/* @64 */ 	sta         __mem_size
/* @66 */ 	lda          #__i0			// ch
	ldx          #13
	jsr          __var_addr
/* @68 */ 	lda         __i0
/* @70 */ 	sta         __mem_dest
/* @71 */ 	lda         __i0+1
/* @73 */ 	sta         __mem_dest+1
/* @75 */ 	jsr         __zeromem1
/* @76 */ 	lda          #__b3
/* @78 */ 	ldx          #13
/* @80 */ 	jsr         __set_var_value1
/* @83 */ 	lda         (__i4)
/* @84 */ 	sta         __i1
/* @85 */ 	ldy          #1
/* @86 */ 	lda         (__i4), Y
/* @87 */ 	sta         __i1+1
/* @90 */ 	ldx          #1
/* @92 */ 	jsr         __pushxy0
/* @94 */ 	ldx         #__i0
/* @95 */ 	jsr         __pushreg2
/* @97 */ 	ldx         #__i1
/* @98 */ 	jsr         __pushreg2
/* @100 */ 	ldx         #__i0
/* @101 */ 	ldy          #0
/* @102 */ 	jsr         write
/* @104 */ 	ldx          #6
/* @105 */ 	jsr         __incsp
/* @108 */ 	lda         __i0
/* @109 */ 	sta         __i5
/* @110 */ 	lda         __i0+1
/* @111 */ 	sta         __i5+1
/* @113 */ 	lda         __i5
/* @114 */ 	cmp          #1
/* @115 */ 	bne         .fputc_label_124
/* @116 */ 	lda         __i5+1
/* @117 */ 	cmp          #0
/* @118 */ 	bne         .fputc_label_124
.fputc_label_112:
/* @121 */ 	lda         __b3
/* @122 */ 	sta         __b2
/* @123 */ 	bra         .fputc_label_129
.fputc_label_124:
/* @127 */ 	lda          #255
/* @128 */ 	sta         __b2
.fputc_label_129:
/* @133 */ 	lda         __b2
/* @134 */ 	sta         __i0
/* @136 */ 	and          #128
/* @138 */ 	beq         .fputc_label_137
/* @140 */ 	lda          #255
.fputc_label_137:
/* @141 */ 	sta         __i0+1
/* @143 */ 	ldx          #18
	jsr          __load_result
/* @144 */ 	lda         #__i0
/* @146 */ 	jsr         __result2
.fputc_label_147:
/* @148 */ 	ldy          #20
	jsr          __leave
/* @149 */ 	rts         
.fputc_label_150:
/* @153 */ 	ldy          #10
/* @154 */ 	lda         (__i4), Y
/* @155 */ 	sta         __i0
/* @157 */ 	ldy          #11
/* @158 */ 	lda         (__i4), Y
/* @159 */ 	sta         __i0+1
/* @163 */ 	ldy          #4
/* @164 */ 	lda         (__i4), Y
/* @165 */ 	sta         __i1
/* @167 */ 	ldy          #5
/* @168 */ 	lda         (__i4), Y
/* @169 */ 	sta         __i1+1
/* @173 */ 	lda         __i0
/* @174 */ 	cmp         __i1
/* @175 */ 	bne         .fputc_label_202
/* @176 */ 	lda         __i0+1
/* @177 */ 	cmp         __i1+1
/* @178 */ 	bne         .fputc_label_202
.fputc_label_172:
/* @180 */ 	ldx         #__i4
/* @181 */ 	jsr         __pushreg2
/* @183 */ 	ldx         #__i0
/* @184 */ 	ldy          #0
/* @185 */ 	jsr         fflush
/* @186 */ 	ldx          #2
/* @187 */ 	jsr         __incsp
/* @190 */ 	lda         __i0
/* @191 */ 	sta         __i6
/* @192 */ 	lda         __i0+1
/* @193 */ 	sta         __i6+1
/* @194 */ 	lda         __i6
/* @195 */ 	ora         __i6+1
/* @196 */ 	beq         .fputc_label_201
/* @197 */ 	ldx          #18
	jsr          __load_result
/* @198 */ 	lda         #__i6
/* @199 */ 	jsr         __result2
/* @200 */ 	bra         .fputc_label_147
.fputc_label_201:
.fputc_label_202:
/* @205 */ 	ldy          #2
/* @206 */ 	lda         (__i4), Y
/* @207 */ 	sta         __i0
/* @208 */ 	ldy          #3
/* @209 */ 	lda         (__i4), Y
/* @210 */ 	sta         __i0+1
/* @213 */ 	lda         __i4
/* @214 */ 	sta         __i1
/* @215 */ 	lda         __i4+1
/* @216 */ 	sta         __i1+1
/* @220 */ 	clc         
/* @221 */ 	lda         __i1
/* @222 */ 	adc          #10
/* @223 */ 	sta         __i2
/* @224 */ 	lda         __i1+1
/* @225 */ 	adc          #0
/* @226 */ 	sta         __i2+1
/* @231 */ 	ldy          #10
/* @232 */ 	lda         (__i1), Y
/* @233 */ 	sta         __i3
/* @234 */ 	ldy          #11
/* @235 */ 	lda         (__i1), Y
/* @236 */ 	sta         __i3+1
/* @238 */ 	lda          #__i2
/* @240 */ 	jsr         __inc21
/* @246 */ 	clc         
/* @247 */ 	lda         __i0
/* @248 */ 	adc         __i3
/* @249 */ 	sta         __i1
/* @250 */ 	lda         __i0+1
/* @251 */ 	adc         __i3+1
/* @252 */ 	sta         __i1+1
/* @255 */ 	lda         __b3
/* @256 */ 	sta         (__i1)
/* @262 */ 	ldx          #1
/* @263 */ 	lda         __b3
/* @264 */ 	cmp          #10
/* @265 */ 	beq         .fputc_label_261
.fputc_label_260:
/* @266 */ 	dex         
.fputc_label_261:
/* @267 */ 	stx         __b4
/* @269 */ 	lda         __b4
/* @270 */ 	beq         .fputc_label_293
/* @274 */ 	ldy          #12
/* @275 */ 	lda         (__i4), Y
/* @276 */ 	sta         __i0
/* @277 */ 	ldy          #13
/* @278 */ 	lda         (__i4), Y
/* @279 */ 	sta         __i0+1
/* @284 */ 	ldx          #1
/* @285 */ 	lda         __i0
/* @286 */ 	cmp          #2
/* @287 */ 	bne         .fputc_label_282
/* @288 */ 	lda         __i0+1
/* @289 */ 	cmp          #0
/* @290 */ 	beq         .fputc_label_283
.fputc_label_282:
/* @291 */ 	dex         
.fputc_label_283:
/* @292 */ 	stx         __b4
.fputc_label_293:
/* @295 */ 	lda         __b4
/* @296 */ 	beq         .fputc_label_310
/* @297 */ 	ldx         #__i4
/* @298 */ 	jsr         __pushreg2
/* @300 */ 	ldx         #__i0
/* @301 */ 	ldy          #0
/* @302 */ 	jsr         fflush
/* @303 */ 	ldx          #2
/* @304 */ 	jsr         __incsp
/* @306 */ 	ldx          #18
	jsr          __load_result
/* @307 */ 	lda         #__i0
/* @308 */ 	jsr         __result2
/* @309 */ 	jmp         .fputc_label_147
.fputc_label_310:
/* @312 */ 	lda          #0
/* @313 */ 	sta         __i0
/* @314 */ 	lda          #0
/* @315 */ 	sta         __i0+1
/* @316 */ 	ldx          #18
	jsr          __load_result
/* @317 */ 	lda         #__i0
/* @318 */ 	jsr         __result2
/* @319 */ 	jmp         .fputc_label_147
.func_end_fputc:
	.size fputc, .func_end_fputc-fputc

	.global putchar
	.type putchar, @function

putchar:
/* @2 */ 	stx         __result
/* @4 */ 	sty         __result+1
/* @5 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x00,0x00		// Save mask i:0 b:0 l:0 x:0 f:0 
/* @11 */ 	lda          #__b0			// c
	ldx          #0
	jsr          __arg_value1
/* @15 */ 	lda         stdout+0
/* @16 */ 	sta         __i0
/* @18 */ 	lda         stdout+1
/* @19 */ 	sta         __i0+1
/* @20 */ 	ldx         #__i0
/* @22 */ 	jsr         __pushreg2
/* @23 */ 	lda         __b0
/* @25 */ 	jsr         __pusha
/* @27 */ 	ldx         #__i0
/* @28 */ 	ldy          #0
/* @29 */ 	jsr         fputc
/* @31 */ 	ldx          #4
/* @33 */ 	jsr         __incsp
/* @35 */ 	ldx          #8
	jsr          __load_result
/* @36 */ 	lda         #__i0
/* @38 */ 	jsr         __result2
.putchar_label_39:
/* @40 */ 	ldy          #10
	jsr          __leave
/* @41 */ 	rts         
.func_end_putchar:
	.size putchar, .func_end_putchar-putchar

	.global fputs
	.type fputs, @function

fputs:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @17 */ 	lda          #__i0			// str
	ldx          #0
	jsr          __arg_value2
/* @21 */ 	lda          #__i5			// stream
	ldx          #2
	jsr          __arg_value2
/* @22 */ 	lda         __i0
/* @23 */ 	sta         __i4
/* @25 */ 	lda         __i0+1
/* @26 */ 	sta         __i4+1
.fputs_label_27:
/* @30 */ 	lda         (__i4)
/* @31 */ 	sta         __b0
/* @33 */ 	lda         __b0
/* @34 */ 	beq         .fputs_label_89
/* @37 */ 	lda         __i4
/* @38 */ 	sta         __i0
/* @39 */ 	lda         __i4+1
/* @40 */ 	sta         __i0+1
/* @41 */ 	lda          #__i4
/* @43 */ 	jsr         __rinc21
/* @48 */ 	lda         (__i0)
/* @49 */ 	sta         __b0
/* @50 */ 	ldx         #__i5
/* @52 */ 	jsr         __pushreg2
/* @54 */ 	lda         __b0
/* @56 */ 	jsr         __pusha
/* @58 */ 	ldx         #__i0
/* @59 */ 	ldy          #0
/* @60 */ 	jsr         fputc
/* @62 */ 	ldx          #4
/* @64 */ 	jsr         __incsp
/* @67 */ 	lda         __i0
/* @68 */ 	cmp          #255
/* @69 */ 	bne         .fputs_label_87
/* @70 */ 	lda         __i0+1
/* @71 */ 	cmp          #255
/* @72 */ 	bne         .fputs_label_87
.fputs_label_66:
/* @76 */ 	lda          #255
/* @77 */ 	sta         __i0
/* @78 */ 	lda          #255
/* @79 */ 	sta         __i0+1
/* @80 */ 	ldx          #8
	jsr          __load_result
/* @81 */ 	lda         #__i0
/* @83 */ 	jsr         __result2
.fputs_label_84:
/* @85 */ 	ldy          #10
	jsr          __leave
/* @86 */ 	rts         
.fputs_label_87:
/* @88 */ 	bra         .fputs_label_27
.fputs_label_89:
/* @91 */ 	lda          #0
/* @92 */ 	sta         __i0
/* @93 */ 	lda          #0
/* @94 */ 	sta         __i0+1
/* @95 */ 	ldx          #8
	jsr          __load_result
/* @96 */ 	lda         #__i0
/* @97 */ 	jsr         __result2
/* @98 */ 	bra         .fputs_label_84
.func_end_fputs:
	.size fputs, .func_end_fputs-fputs

	.global fflush
	.type fflush, @function

fflush:
/* @9 */ 	stx         __result
/* @11 */ 	sty         __result+1
/* @12 */ 	ldx          #7
	jsr          __enter
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
/* @18 */ 	lda          #__i4			// stream
	ldx          #0
	jsr          __arg_value2
/* @25 */ 	ldx         #__i4
/* @27 */ 	jsr         __pushreg2
/* @28 */ 	jsr         EnsureBuffer
/* @30 */ 	ldx          #2
/* @32 */ 	jsr         __incsp
/* @35 */ 	ldy          #2
/* @36 */ 	lda         (__i4), Y
/* @37 */ 	sta         __i0
/* @39 */ 	ldy          #3
/* @40 */ 	lda         (__i4), Y
/* @41 */ 	sta         __i0+1
/* @44 */ 	lda         __i0
/* @45 */ 	cmp          #0
/* @46 */ 	bne         .fflush_label_43
/* @47 */ 	lda         __i0+1
/* @48 */ 	cmp          #0
/* @49 */ 	bne         .fflush_label_171
/* @172 */ 	jmp         .fflush_label_161
.fflush_label_171:
.fflush_label_43:
/* @54 */ 	ldy          #10
/* @55 */ 	lda         (__i4), Y
/* @56 */ 	sta         __i0
/* @58 */ 	ldy          #11
/* @59 */ 	lda         (__i4), Y
/* @60 */ 	sta         __i0+1
/* @63 */ 	lda         __i0
/* @64 */ 	sta         __i5
/* @65 */ 	lda         __i0+1
/* @66 */ 	sta         __i5+1
/* @68 */ 	lda          #0
/* @69 */ 	sta         __i0
/* @70 */ 	lda          #0
/* @71 */ 	sta         __i0+1
/* @73 */ 	lda         __i0
/* @74 */ 	ldy          #10
/* @75 */ 	sta         (__i4), Y
/* @76 */ 	lda         __i0+1
/* @77 */ 	ldy          #11
/* @78 */ 	sta         (__i4), Y
/* @81 */ 	lda         (__i4)
/* @82 */ 	sta         __i0
/* @83 */ 	ldy          #1
/* @84 */ 	lda         (__i4), Y
/* @85 */ 	sta         __i0+1
/* @88 */ 	ldy          #2
/* @89 */ 	lda         (__i4), Y
/* @90 */ 	sta         __i1
/* @91 */ 	ldy          #3
/* @92 */ 	lda         (__i4), Y
/* @93 */ 	sta         __i1+1
/* @94 */ 	ldx         #__i5
/* @95 */ 	jsr         __pushreg2
/* @97 */ 	ldx         #__i1
/* @98 */ 	jsr         __pushreg2
/* @100 */ 	ldx         #__i0
/* @101 */ 	jsr         __pushreg2
/* @103 */ 	ldx         #__i0
/* @104 */ 	ldy          #0
/* @105 */ 	jsr         write
/* @107 */ 	ldx          #6
/* @108 */ 	jsr         __incsp
/* @111 */ 	lda         __i0
/* @112 */ 	sta         __i6
/* @113 */ 	lda         __i0+1
/* @114 */ 	sta         __i6+1
/* @116 */ 	sec         
/* @117 */ 	lda          #0
/* @118 */ 	cmp         __i6
/* @119 */ 	lda          #0
/* @120 */ 	sbc         __i6+1
/* @121 */ 	bvc         .fflush_label_115
/* @123 */ 	eor          #128
.fflush_label_115:
/* @124 */ 	bmi         .fflush_label_160
/* @125 */ 	lda         __i6
/* @126 */ 	ora         __i6+1
/* @127 */ 	bne         .fflush_label_137
/* @129 */ 	lda          #1
/* @130 */ 	sta         __b0
/* @132 */ 	lda         __b0
/* @134 */ 	ldy          #17
/* @135 */ 	sta         (__i4), Y
/* @136 */ 	bra         .fflush_label_146
.fflush_label_137:
/* @139 */ 	lda          #1
/* @140 */ 	sta         __b0
/* @142 */ 	lda         __b0
/* @144 */ 	ldy          #18
/* @145 */ 	sta         (__i4), Y
.fflush_label_146:
/* @149 */ 	lda          #255
/* @150 */ 	sta         __i0
/* @151 */ 	lda          #255
/* @152 */ 	sta         __i0+1
/* @153 */ 	ldx          #8
	jsr          __load_result
/* @154 */ 	lda         #__i0
/* @156 */ 	jsr         __result2
.fflush_label_157:
/* @158 */ 	ldy          #10
	jsr          __leave
/* @159 */ 	rts         
.fflush_label_160:
.fflush_label_161:
/* @163 */ 	lda          #0
/* @164 */ 	sta         __i0
/* @165 */ 	lda          #0
/* @166 */ 	sta         __i0+1
/* @167 */ 	ldx          #8
	jsr          __load_result
/* @168 */ 	lda         #__i0
/* @169 */ 	jsr         __result2
/* @170 */ 	bra         .fflush_label_157
.func_end_fflush:
	.size fflush, .func_end_fflush-fflush

	.global puts
	.type puts, @function

puts:
/* @2 */ 	stx         __result
/* @4 */ 	sty         __result+1
/* @5 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x00,0x00		// Save mask i:0 b:0 l:0 x:0 f:0 
/* @11 */ 	lda          #__i0			// stream
	ldx          #0
	jsr          __arg_value2
/* @15 */ 	lda         stdout+0
/* @16 */ 	sta         __i1
/* @18 */ 	lda         stdout+1
/* @19 */ 	sta         __i1+1
/* @20 */ 	ldx         #__i1
/* @22 */ 	jsr         __pushreg2
/* @23 */ 	ldx         #__i0
/* @24 */ 	jsr         __pushreg2
/* @26 */ 	ldx         #__i0
/* @27 */ 	ldy          #0
/* @28 */ 	jsr         fputs
/* @30 */ 	ldx          #4
/* @32 */ 	jsr         __incsp
/* @34 */ 	ldx          #8
	jsr          __load_result
/* @35 */ 	lda         #__i0
/* @37 */ 	jsr         __result2
.puts_label_38:
/* @39 */ 	ldy          #10
	jsr          __leave
/* @40 */ 	rts         
.func_end_puts:
	.size puts, .func_end_puts-puts

	.global getc
	.type getc, @function

getc:
/* @16 */ 	stx         __result
/* @18 */ 	sty         __result+1
/* @19 */ 	ldx          #8
	jsr          __enter
	.byte        0x24,0x00,0x00		// Save mask i:4 b:1 l:0 x:0 f:0 
/* @25 */ 	lda          #__i4			// stream
	ldx          #0
	jsr          __arg_value2
/* @35 */ 	ldx         #__i4
/* @37 */ 	jsr         __pushreg2
/* @38 */ 	jsr         EnsureBuffer
/* @40 */ 	ldx          #2
/* @42 */ 	jsr         __incsp
/* @45 */ 	ldy          #2
/* @46 */ 	lda         (__i4), Y
/* @47 */ 	sta         __i0
/* @49 */ 	ldy          #3
/* @50 */ 	lda         (__i4), Y
/* @51 */ 	sta         __i0+1
/* @54 */ 	lda         __i0
/* @55 */ 	cmp          #0
/* @56 */ 	beq         .getc_label_663
/* @664 */ 	jmp         .getc_label_160
.getc_label_663:
/* @57 */ 	lda         __i0+1
/* @58 */ 	cmp          #0
/* @59 */ 	beq         .getc_label_665
/* @666 */ 	jmp         .getc_label_160
.getc_label_665:
.getc_label_53:
/* @63 */ 	lda         (__i4)
/* @64 */ 	sta         __i0
/* @65 */ 	ldy          #1
/* @66 */ 	lda         (__i4), Y
/* @67 */ 	sta         __i0+1
/* @69 */ 	lda          #__i7			// rbuf
	ldx          #4
	jsr          __var_addr
/* @70 */ 	ldx          #1
/* @72 */ 	jsr         __pushxy0
/* @74 */ 	ldx         #__i7
/* @75 */ 	jsr         __pushreg2
/* @77 */ 	ldx         #__i0
/* @78 */ 	jsr         __pushreg2
/* @80 */ 	ldx         #__i0
/* @81 */ 	ldy          #0
/* @82 */ 	jsr         read
/* @84 */ 	ldx          #6
/* @85 */ 	jsr         __incsp
/* @88 */ 	lda         __i0
/* @89 */ 	sta         __i5
/* @90 */ 	lda         __i0+1
/* @91 */ 	sta         __i5+1
/* @93 */ 	sec         
/* @94 */ 	lda          #0
/* @95 */ 	cmp         __i5
/* @96 */ 	lda          #0
/* @97 */ 	sbc         __i5+1
/* @98 */ 	bvc         .getc_label_92
/* @100 */ 	eor          #128
.getc_label_92:
/* @101 */ 	bmi         .getc_label_137
/* @103 */ 	lda         __i5+1
/* @104 */ 	bpl         .getc_label_114
/* @106 */ 	lda          #1
/* @107 */ 	sta         __b0
/* @109 */ 	lda         __b0
/* @111 */ 	ldy          #18
/* @112 */ 	sta         (__i4), Y
/* @113 */ 	bra         .getc_label_123
.getc_label_114:
/* @116 */ 	lda          #1
/* @117 */ 	sta         __b0
/* @119 */ 	lda         __b0
/* @121 */ 	ldy          #17
/* @122 */ 	sta         (__i4), Y
.getc_label_123:
/* @126 */ 	lda          #255
/* @127 */ 	sta         __i0
/* @128 */ 	lda          #255
/* @129 */ 	sta         __i0+1
/* @130 */ 	ldx          #9
	jsr          __load_result
/* @131 */ 	lda         #__i0
/* @133 */ 	jsr         __result2
.getc_label_134:
/* @135 */ 	ldy          #11
	jsr          __leave
/* @136 */ 	rts         
.getc_label_137:
/* @143 */ 	lda         (__i7)
/* @144 */ 	sta         __b0
/* @148 */ 	lda         __b0
/* @149 */ 	sta         __i0
/* @150 */ 	and          #128
/* @152 */ 	beq         .getc_label_151
/* @153 */ 	lda          #255
.getc_label_151:
/* @154 */ 	sta         __i0+1
/* @156 */ 	ldx          #9
	jsr          __load_result
/* @157 */ 	lda         #__i0
/* @158 */ 	jsr         __result2
/* @159 */ 	bra         .getc_label_134
.getc_label_160:
/* @164 */ 	ldy          #16
/* @165 */ 	lda         (__i4), Y
/* @166 */ 	sta         __b0
/* @169 */ 	sec         
/* @170 */ 	lda          #0
/* @171 */ 	cmp         __b0
/* @172 */ 	bvc         .getc_label_168
/* @173 */ 	eor          #128
.getc_label_168:
/* @174 */ 	bpl         .getc_label_245
/* @177 */ 	ldy          #2
/* @178 */ 	lda         (__i4), Y
/* @179 */ 	sta         __i0
/* @180 */ 	ldy          #3
/* @181 */ 	lda         (__i4), Y
/* @182 */ 	sta         __i0+1
/* @185 */ 	clc         
/* @186 */ 	lda         __i4
/* @187 */ 	adc          #16
/* @188 */ 	sta         __b0
/* @189 */ 	lda         __i4+1
/* @190 */ 	adc          #0
/* @191 */ 	sta         __b0+1
/* @193 */ 	lda          #__b0
/* @195 */ 	jsr         __dec1
/* @200 */ 	lda         __b0
/* @201 */ 	sta         __b1
/* @205 */ 	lda         __b1
/* @206 */ 	sta         __i1
/* @207 */ 	and          #128
/* @209 */ 	beq         .getc_label_208
/* @210 */ 	lda          #255
.getc_label_208:
/* @211 */ 	sta         __i1+1
/* @217 */ 	clc         
/* @218 */ 	lda         __i0
/* @219 */ 	adc         __i1
/* @220 */ 	sta         __i2
/* @221 */ 	lda         __i0+1
/* @222 */ 	adc         __i1+1
/* @223 */ 	sta         __i2+1
/* @228 */ 	lda         (__i2)
/* @229 */ 	sta         __b0
/* @233 */ 	lda         __b0
/* @234 */ 	sta         __i0
/* @235 */ 	and          #128
/* @237 */ 	beq         .getc_label_236
/* @238 */ 	lda          #255
.getc_label_236:
/* @239 */ 	sta         __i0+1
/* @241 */ 	ldx          #9
	jsr          __load_result
/* @242 */ 	lda         #__i0
/* @243 */ 	jsr         __result2
/* @244 */ 	jmp         .getc_label_134
.getc_label_245:
/* @248 */ 	ldy          #6
/* @249 */ 	lda         (__i4), Y
/* @250 */ 	sta         __i0
/* @252 */ 	ldy          #7
/* @253 */ 	lda         (__i4), Y
/* @254 */ 	sta         __i0+1
/* @258 */ 	ldy          #8
/* @259 */ 	lda         (__i4), Y
/* @260 */ 	sta         __i1
/* @262 */ 	ldy          #9
/* @263 */ 	lda         (__i4), Y
/* @264 */ 	sta         __i1+1
/* @268 */ 	sec         
/* @269 */ 	lda         __i0
/* @270 */ 	cmp         __i1
/* @271 */ 	lda         __i0+1
/* @272 */ 	sbc         __i1+1
/* @273 */ 	bvc         .getc_label_267
/* @274 */ 	eor          #128
.getc_label_267:
/* @275 */ 	bpl         .getc_label_347
/* @278 */ 	ldy          #2
/* @279 */ 	lda         (__i4), Y
/* @280 */ 	sta         __i0
/* @281 */ 	ldy          #3
/* @282 */ 	lda         (__i4), Y
/* @283 */ 	sta         __i0+1
/* @286 */ 	lda         __i4
/* @287 */ 	sta         __i1
/* @288 */ 	lda         __i4+1
/* @289 */ 	sta         __i1+1
/* @293 */ 	clc         
/* @294 */ 	lda         __i1
/* @295 */ 	adc          #6
/* @296 */ 	sta         __i2
/* @297 */ 	lda         __i1+1
/* @298 */ 	adc          #0
/* @299 */ 	sta         __i2+1
/* @304 */ 	ldy          #6
/* @305 */ 	lda         (__i1), Y
/* @306 */ 	sta         __i3
/* @307 */ 	ldy          #7
/* @308 */ 	lda         (__i1), Y
/* @309 */ 	sta         __i3+1
/* @311 */ 	lda          #__i2
/* @313 */ 	jsr         __inc21
/* @319 */ 	clc         
/* @320 */ 	lda         __i0
/* @321 */ 	adc         __i3
/* @322 */ 	sta         __i1
/* @323 */ 	lda         __i0+1
/* @324 */ 	adc         __i3+1
/* @325 */ 	sta         __i1+1
/* @330 */ 	lda         (__i1)
/* @331 */ 	sta         __b0
/* @335 */ 	lda         __b0
/* @336 */ 	sta         __i0
/* @337 */ 	and          #128
/* @339 */ 	beq         .getc_label_338
/* @340 */ 	lda          #255
.getc_label_338:
/* @341 */ 	sta         __i0+1
/* @343 */ 	ldx          #9
	jsr          __load_result
/* @344 */ 	lda         #__i0
/* @345 */ 	jsr         __result2
/* @346 */ 	jmp         .getc_label_134
.getc_label_347:
/* @349 */ 	lda          #0
/* @350 */ 	sta         __i0
/* @351 */ 	lda          #0
/* @352 */ 	sta         __i0+1
/* @354 */ 	lda         __i0
/* @355 */ 	ldy          #6
/* @356 */ 	sta         (__i4), Y
/* @357 */ 	lda         __i0+1
/* @358 */ 	ldy          #7
/* @359 */ 	sta         (__i4), Y
/* @361 */ 	lda          #0
/* @362 */ 	sta         __i0
/* @363 */ 	lda          #0
/* @364 */ 	sta         __i0+1
/* @366 */ 	lda         __i0
/* @367 */ 	ldy          #8
/* @368 */ 	sta         (__i4), Y
/* @369 */ 	lda         __i0+1
/* @370 */ 	ldy          #9
/* @371 */ 	sta         (__i4), Y
.getc_label_372:
/* @375 */ 	lda         (__i4)
/* @376 */ 	sta         __i0
/* @377 */ 	ldy          #1
/* @378 */ 	lda         (__i4), Y
/* @379 */ 	sta         __i0+1
/* @382 */ 	ldy          #2
/* @383 */ 	lda         (__i4), Y
/* @384 */ 	sta         __i1
/* @385 */ 	ldy          #3
/* @386 */ 	lda         (__i4), Y
/* @387 */ 	sta         __i1+1
/* @390 */ 	ldy          #8
/* @391 */ 	lda         (__i4), Y
/* @392 */ 	sta         __i2
/* @393 */ 	ldy          #9
/* @394 */ 	lda         (__i4), Y
/* @395 */ 	sta         __i2+1
/* @401 */ 	clc         
/* @402 */ 	lda         __i1
/* @403 */ 	adc         __i2
/* @404 */ 	sta         __i3
/* @405 */ 	lda         __i1+1
/* @406 */ 	adc         __i2+1
/* @407 */ 	sta         __i3+1
/* @408 */ 	ldx          #1
/* @409 */ 	jsr         __pushxy0
/* @411 */ 	ldx         #__i3
/* @412 */ 	jsr         __pushreg2
/* @414 */ 	ldx         #__i0
/* @415 */ 	jsr         __pushreg2
/* @417 */ 	ldx         #__i0
/* @418 */ 	ldy          #0
/* @419 */ 	jsr         read
/* @420 */ 	ldx          #6
/* @421 */ 	jsr         __incsp
/* @424 */ 	lda         __i0
/* @425 */ 	sta         __i6
/* @426 */ 	lda         __i0+1
/* @427 */ 	sta         __i6+1
/* @429 */ 	sec         
/* @430 */ 	lda          #0
/* @431 */ 	cmp         __i6
/* @432 */ 	lda          #0
/* @433 */ 	sbc         __i6+1
/* @434 */ 	bvc         .getc_label_428
/* @435 */ 	eor          #128
.getc_label_428:
/* @436 */ 	bmi         .getc_label_466
/* @438 */ 	lda         __i6+1
/* @439 */ 	bpl         .getc_label_448
/* @441 */ 	lda          #1
/* @442 */ 	sta         __b0
/* @444 */ 	lda         __b0
/* @445 */ 	ldy          #18
/* @446 */ 	sta         (__i4), Y
/* @447 */ 	bra         .getc_label_456
.getc_label_448:
/* @450 */ 	lda          #1
/* @451 */ 	sta         __b0
/* @453 */ 	lda         __b0
/* @454 */ 	ldy          #17
/* @455 */ 	sta         (__i4), Y
.getc_label_456:
/* @458 */ 	lda          #255
/* @459 */ 	sta         __i0
/* @460 */ 	lda          #255
/* @461 */ 	sta         __i0+1
/* @462 */ 	ldx          #9
	jsr          __load_result
/* @463 */ 	lda         #__i0
/* @464 */ 	jsr         __result2
/* @465 */ 	jmp         .getc_label_134
.getc_label_466:
/* @469 */ 	ldy          #2
/* @470 */ 	lda         (__i4), Y
/* @471 */ 	sta         __i0
/* @472 */ 	ldy          #3
/* @473 */ 	lda         (__i4), Y
/* @474 */ 	sta         __i0+1
/* @477 */ 	ldy          #8
/* @478 */ 	lda         (__i4), Y
/* @479 */ 	sta         __i1
/* @480 */ 	ldy          #9
/* @481 */ 	lda         (__i4), Y
/* @482 */ 	sta         __i1+1
/* @488 */ 	clc         
/* @489 */ 	lda         __i0
/* @490 */ 	adc         __i1
/* @491 */ 	sta         __i2
/* @492 */ 	lda         __i0+1
/* @493 */ 	adc         __i1+1
/* @494 */ 	sta         __i2+1
/* @499 */ 	lda         (__i2)
/* @500 */ 	sta         __b0
/* @503 */ 	lda         __b0
/* @504 */ 	sta         __b2
/* @507 */ 	clc         
/* @508 */ 	lda         __i4
/* @509 */ 	adc          #8
/* @510 */ 	sta         __i0
/* @511 */ 	lda         __i4+1
/* @512 */ 	adc          #0
/* @513 */ 	sta         __i0+1
/* @515 */ 	lda          #__i0
/* @516 */ 	jsr         __inc21
/* @522 */ 	ldy          #12
/* @523 */ 	lda         (__i4), Y
/* @524 */ 	sta         __i0
/* @526 */ 	ldy          #13
/* @527 */ 	lda         (__i4), Y
/* @528 */ 	sta         __i0+1
/* @533 */ 	ldx          #1
/* @534 */ 	lda         __i0
/* @535 */ 	cmp          #2
/* @536 */ 	bne         .getc_label_531
/* @537 */ 	lda         __i0+1
/* @538 */ 	cmp          #0
/* @539 */ 	beq         .getc_label_532
.getc_label_531:
/* @540 */ 	dex         
.getc_label_532:
/* @541 */ 	stx         __b0
/* @543 */ 	lda         __b0
/* @544 */ 	beq         .getc_label_554
/* @548 */ 	ldx          #1
/* @549 */ 	lda         __b2
/* @550 */ 	cmp          #10
/* @551 */ 	beq         .getc_label_547
.getc_label_546:
/* @552 */ 	dex         
.getc_label_547:
/* @553 */ 	stx         __b0
.getc_label_554:
/* @556 */ 	lda         __b0
/* @557 */ 	beq         .getc_label_559
/* @558 */ 	bra         .getc_label_592
.getc_label_559:
/* @562 */ 	ldy          #8
/* @563 */ 	lda         (__i4), Y
/* @564 */ 	sta         __i0
/* @565 */ 	ldy          #9
/* @566 */ 	lda         (__i4), Y
/* @567 */ 	sta         __i0+1
/* @571 */ 	ldy          #4
/* @572 */ 	lda         (__i4), Y
/* @573 */ 	sta         __i1
/* @575 */ 	ldy          #5
/* @576 */ 	lda         (__i4), Y
/* @577 */ 	sta         __i1+1
/* @581 */ 	lda         __i0
/* @582 */ 	cmp         __i1
/* @583 */ 	bne         .getc_label_589
/* @584 */ 	lda         __i0+1
/* @585 */ 	cmp         __i1+1
/* @586 */ 	bne         .getc_label_589
.getc_label_580:
/* @588 */ 	bra         .getc_label_592
.getc_label_589:
.getc_label_590:
/* @591 */ 	jmp         .getc_label_372
.getc_label_592:
/* @595 */ 	ldy          #2
/* @596 */ 	lda         (__i4), Y
/* @597 */ 	sta         __i0
/* @598 */ 	ldy          #3
/* @599 */ 	lda         (__i4), Y
/* @600 */ 	sta         __i0+1
/* @603 */ 	lda         __i4
/* @604 */ 	sta         __i1
/* @605 */ 	lda         __i4+1
/* @606 */ 	sta         __i1+1
/* @610 */ 	clc         
/* @611 */ 	lda         __i1
/* @612 */ 	adc          #6
/* @613 */ 	sta         __i2
/* @614 */ 	lda         __i1+1
/* @615 */ 	adc          #0
/* @616 */ 	sta         __i2+1
/* @621 */ 	ldy          #6
/* @622 */ 	lda         (__i1), Y
/* @623 */ 	sta         __i3
/* @624 */ 	ldy          #7
/* @625 */ 	lda         (__i1), Y
/* @626 */ 	sta         __i3+1
/* @628 */ 	lda          #__i2
/* @629 */ 	jsr         __inc21
/* @635 */ 	clc         
/* @636 */ 	lda         __i0
/* @637 */ 	adc         __i3
/* @638 */ 	sta         __i1
/* @639 */ 	lda         __i0+1
/* @640 */ 	adc         __i3+1
/* @641 */ 	sta         __i1+1
/* @646 */ 	lda         (__i1)
/* @647 */ 	sta         __b1
/* @651 */ 	lda         __b1
/* @652 */ 	sta         __i0
/* @653 */ 	and          #128
/* @655 */ 	beq         .getc_label_654
/* @656 */ 	lda          #255
.getc_label_654:
/* @657 */ 	sta         __i0+1
/* @659 */ 	ldx          #9
	jsr          __load_result
/* @660 */ 	lda         #__i0
/* @661 */ 	jsr         __result2
/* @662 */ 	jmp         .getc_label_134
.func_end_getc:
	.size getc, .func_end_getc-getc

	.global fgetc
	.type fgetc, @function

fgetc:
/* @2 */ 	stx         __result
/* @4 */ 	sty         __result+1
/* @5 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x00,0x00		// Save mask i:0 b:0 l:0 x:0 f:0 
/* @10 */ 	lda          #__i0			// stream
	ldx          #0
	jsr          __arg_value2
/* @12 */ 	ldx         #__i0
/* @14 */ 	jsr         __pushreg2
/* @16 */ 	ldx         #__i0
/* @17 */ 	ldy          #0
/* @18 */ 	jsr         getc
/* @20 */ 	ldx          #2
/* @22 */ 	jsr         __incsp
/* @24 */ 	ldx          #8
	jsr          __load_result
/* @25 */ 	lda         #__i0
/* @27 */ 	jsr         __result2
.fgetc_label_28:
/* @29 */ 	ldy          #10
	jsr          __leave
/* @30 */ 	rts         
.func_end_fgetc:
	.size fgetc, .func_end_fgetc-fgetc

	.global getchar
	.type getchar, @function

getchar:
/* @2 */ 	stx         __result
/* @4 */ 	sty         __result+1
/* @5 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x00,0x00		// Save mask i:0 b:0 l:0 x:0 f:0 
/* @11 */ 	lda         stdin+0
/* @12 */ 	sta         __i0
/* @14 */ 	lda         stdin+1
/* @15 */ 	sta         __i0+1
/* @16 */ 	ldx         #__i0
/* @18 */ 	jsr         __pushreg2
/* @20 */ 	ldx         #__i0
/* @21 */ 	ldy          #0
/* @22 */ 	jsr         getc
/* @24 */ 	ldx          #2
/* @26 */ 	jsr         __incsp
/* @28 */ 	ldx          #8
	jsr          __load_result
/* @29 */ 	lda         #__i0
/* @31 */ 	jsr         __result2
.getchar_label_32:
/* @33 */ 	ldy          #10
	jsr          __leave
/* @34 */ 	rts         
.func_end_getchar:
	.size getchar, .func_end_getchar-getchar

	.global ungetc
	.type ungetc, @function

ungetc:
/* @12 */ 	stx         __result
/* @14 */ 	sty         __result+1
/* @15 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @19 */ 	lda          #__i0			// ch
	ldx          #0
	jsr          __arg_value2
/* @23 */ 	lda          #__i1			// stream
	ldx          #2
	jsr          __arg_value2
/* @25 */ 	lda         __i0
/* @26 */ 	cmp          #255
/* @27 */ 	bne         .ungetc_label_44
/* @28 */ 	lda         __i0+1
/* @29 */ 	cmp          #255
/* @30 */ 	bne         .ungetc_label_44
.ungetc_label_24:
/* @34 */ 	lda          #255
/* @35 */ 	sta         __i2
/* @36 */ 	lda          #255
/* @37 */ 	sta         __i2+1
/* @38 */ 	lda         #__i2
/* @40 */ 	jsr         __result2
.ungetc_label_41:
/* @42 */ 	ldy          #8
	jsr          __leave_leaf
/* @43 */ 	rts         
.ungetc_label_44:
/* @48 */ 	ldy          #16
/* @49 */ 	lda         (__i1), Y
/* @50 */ 	sta         __b0
/* @53 */ 	lda         __b0
/* @54 */ 	cmp          #10
/* @55 */ 	bne         .ungetc_label_65
.ungetc_label_52:
/* @58 */ 	lda          #255
/* @59 */ 	sta         __i2
/* @60 */ 	lda          #255
/* @61 */ 	sta         __i2+1
/* @62 */ 	lda         #__i2
/* @63 */ 	jsr         __result2
/* @64 */ 	bra         .ungetc_label_41
.ungetc_label_65:
/* @67 */ 	lda          #0
/* @68 */ 	sta         __b0
/* @70 */ 	lda         __b0
/* @72 */ 	ldy          #17
/* @73 */ 	sta         (__i1), Y
/* @76 */ 	clc         
/* @77 */ 	lda         __i1
/* @78 */ 	adc          #19
/* @79 */ 	sta         __i2
/* @80 */ 	lda         __i1+1
/* @81 */ 	adc          #0
/* @82 */ 	sta         __i2+1
/* @85 */ 	lda         __i1
/* @86 */ 	sta         __i3
/* @87 */ 	lda         __i1+1
/* @88 */ 	sta         __i3+1
/* @92 */ 	clc         
/* @93 */ 	lda         __i3
/* @94 */ 	adc          #16
/* @95 */ 	sta         __b0
/* @96 */ 	lda         __i3+1
/* @97 */ 	adc          #0
/* @98 */ 	sta         __b0+1
/* @103 */ 	ldy          #16
/* @104 */ 	lda         (__i3), Y
/* @105 */ 	sta         __b1
/* @107 */ 	lda          #__b0
/* @109 */ 	jsr         __inc1
/* @113 */ 	lda         __b1
/* @114 */ 	sta         __i3
/* @116 */ 	and          #128
/* @118 */ 	beq         .ungetc_label_117
/* @119 */ 	lda          #255
.ungetc_label_117:
/* @120 */ 	sta         __i3+1
/* @126 */ 	clc         
/* @127 */ 	lda         __i2
/* @128 */ 	adc         __i3
/* @129 */ 	sta         __i4
/* @130 */ 	lda         __i2+1
/* @131 */ 	adc         __i3+1
/* @132 */ 	sta         __i4+1
/* @135 */ 	lda         __i0
/* @136 */ 	sta         __i2
/* @137 */ 	lda         __i0+1
/* @138 */ 	sta         __i2+1
/* @143 */ 	lda         __i2
/* @144 */ 	sta         (__i4)
/* @145 */ 	lda         #__i0
/* @146 */ 	jsr         __result2
/* @147 */ 	jmp         .ungetc_label_41
.func_end_ungetc:
	.size ungetc, .func_end_ungetc-ungetc

	.global fgets
	.type fgets, @function

fgets:
/* @10 */ 	stx         __result
/* @12 */ 	sty         __result+1
/* @13 */ 	ldx          #7
	jsr          __enter
	.byte        0x24,0x00,0x00		// Save mask i:4 b:1 l:0 x:0 f:0 
/* @18 */ 	lda          #__i4			// n
	ldx          #2
	jsr          __arg_value2
/* @25 */ 	lda          #__i6			// s
	ldx          #0
	jsr          __arg_value2
/* @31 */ 	lda          #__i7			// stream
	ldx          #4
	jsr          __arg_value2
/* @32 */ 	lda          #__i4
/* @34 */ 	jsr         __rdec21
/* @35 */ 	lda         __i6
/* @36 */ 	sta         __i5
/* @37 */ 	lda         __i6+1
/* @38 */ 	sta         __i5+1
.fgets_label_39:
/* @41 */ 	sec         
/* @42 */ 	lda          #0
/* @43 */ 	cmp         __i4
/* @44 */ 	lda          #0
/* @45 */ 	sbc         __i4+1
/* @46 */ 	bvc         .fgets_label_40
/* @48 */ 	eor          #128
.fgets_label_40:
/* @49 */ 	bpl         .fgets_label_130
/* @50 */ 	ldx         #__i7
/* @52 */ 	jsr         __pushreg2
/* @54 */ 	ldx         #__i0
/* @55 */ 	ldy          #0
/* @56 */ 	jsr         fgetc
/* @58 */ 	ldx          #2
/* @60 */ 	jsr         __incsp
/* @63 */ 	lda         __i0
/* @64 */ 	sta         __b2
/* @66 */ 	lda         __b2
/* @67 */ 	cmp          #255
/* @68 */ 	bne         .fgets_label_100
.fgets_label_65:
/* @71 */ 	lda          #1
/* @72 */ 	sta         __b0
/* @74 */ 	lda         __b0
/* @76 */ 	ldy          #17
/* @77 */ 	sta         (__i7), Y
/* @79 */ 	lda         __i5
/* @80 */ 	cmp         __i6
/* @81 */ 	bne         .fgets_label_98
/* @82 */ 	lda         __i5+1
/* @83 */ 	cmp         __i6+1
/* @84 */ 	bne         .fgets_label_98
.fgets_label_78:
/* @87 */ 	lda          #0
/* @88 */ 	sta         __i0
/* @89 */ 	lda          #0
/* @90 */ 	sta         __i0+1
/* @91 */ 	ldx          #8
	jsr          __load_result
/* @92 */ 	lda         #__i0
/* @94 */ 	jsr         __result2
.fgets_label_95:
/* @96 */ 	ldy          #10
	jsr          __leave
/* @97 */ 	rts         
.fgets_label_98:
/* @99 */ 	bra         .fgets_label_130
.fgets_label_100:
/* @103 */ 	lda         __i5
/* @104 */ 	sta         __i0
/* @105 */ 	lda         __i5+1
/* @106 */ 	sta         __i0+1
/* @107 */ 	lda          #__i5
/* @109 */ 	jsr         __rinc21
/* @112 */ 	lda         __b2
/* @113 */ 	sta         (__i0)
/* @115 */ 	lda         __b2
/* @116 */ 	cmp          #10
/* @117 */ 	bne         .fgets_label_126
.fgets_label_114:
/* @120 */ 	lda          #0
/* @121 */ 	sta         __b0
/* @123 */ 	lda         __b0
/* @124 */ 	sta         (__i5)
/* @125 */ 	bra         .fgets_label_130
.fgets_label_126:
/* @127 */ 	lda          #__i4
/* @128 */ 	jsr         __rdec21
/* @129 */ 	jmp         .fgets_label_39
.fgets_label_130:
/* @131 */ 	ldx          #8
	jsr          __load_result
/* @132 */ 	lda         #__i6
/* @133 */ 	jsr         __result2
/* @134 */ 	bra         .fgets_label_95
.func_end_fgets:
	.size fgets, .func_end_fgets-fgets

	.global gets
	.type gets, @function

gets:
/* @10 */ 	stx         __result
/* @12 */ 	sty         __result+1
/* @13 */ 	ldx          #7
	jsr          __enter
	.byte        0x22,0x00,0x00		// Save mask i:2 b:1 l:0 x:0 f:0 
/* @22 */ 	lda          #__i5			// str
	ldx          #0
	jsr          __arg_value2
/* @25 */ 	lda         __i5
/* @26 */ 	sta         __i4
/* @27 */ 	lda         __i5+1
/* @28 */ 	sta         __i4+1
.gets_label_29:
/* @32 */ 	lda         stdin+0
/* @33 */ 	sta         __i0
/* @34 */ 	lda         stdin+1
/* @35 */ 	sta         __i0+1
/* @36 */ 	ldx         #__i0
/* @38 */ 	jsr         __pushreg2
/* @40 */ 	ldx         #__i0
/* @41 */ 	ldy          #0
/* @42 */ 	jsr         fgetc
/* @44 */ 	ldx          #2
/* @46 */ 	jsr         __incsp
/* @49 */ 	lda         __i0
/* @50 */ 	sta         __b2
/* @52 */ 	lda         __b2
/* @53 */ 	cmp          #255
/* @54 */ 	bne         .gets_label_93
.gets_label_51:
/* @57 */ 	lda          #1
/* @58 */ 	sta         __b0
/* @61 */ 	lda         stdin+0
/* @62 */ 	sta         __i0
/* @63 */ 	lda         stdin+1
/* @64 */ 	sta         __i0+1
/* @67 */ 	lda         __b0
/* @69 */ 	ldy          #17
/* @70 */ 	sta         (__i0), Y
/* @72 */ 	lda         __i4
/* @73 */ 	cmp         __i5
/* @74 */ 	bne         .gets_label_91
/* @75 */ 	lda         __i4+1
/* @76 */ 	cmp         __i5+1
/* @77 */ 	bne         .gets_label_91
.gets_label_71:
/* @80 */ 	lda          #0
/* @81 */ 	sta         __i0
/* @82 */ 	lda          #0
/* @83 */ 	sta         __i0+1
/* @84 */ 	ldx          #8
	jsr          __load_result
/* @85 */ 	lda         #__i0
/* @87 */ 	jsr         __result2
.gets_label_88:
/* @89 */ 	ldy          #10
	jsr          __leave
/* @90 */ 	rts         
.gets_label_91:
/* @92 */ 	bra         .gets_label_123
.gets_label_93:
/* @96 */ 	lda         __i4
/* @97 */ 	sta         __i0
/* @98 */ 	lda         __i4+1
/* @99 */ 	sta         __i0+1
/* @100 */ 	lda          #__i4
/* @102 */ 	jsr         __rinc21
/* @105 */ 	lda         __b2
/* @106 */ 	sta         (__i0)
/* @108 */ 	lda         __b2
/* @109 */ 	cmp          #10
/* @110 */ 	bne         .gets_label_120
.gets_label_107:
/* @113 */ 	lda          #0
/* @114 */ 	sta         __b0
/* @116 */ 	lda         __b0
/* @117 */ 	ldy          #255
/* @118 */ 	sta         (__i4), Y
/* @119 */ 	bra         .gets_label_123
.gets_label_120:
.gets_label_121:
/* @122 */ 	jmp         .gets_label_29
.gets_label_123:
/* @124 */ 	ldx          #8
	jsr          __load_result
/* @125 */ 	lda         #__i5
/* @126 */ 	jsr         __result2
/* @127 */ 	bra         .gets_label_88
.func_end_gets:
	.size gets, .func_end_gets-gets

	.global fread
	.type fread, @function

fread:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #7
	jsr          __enter
	.byte        0x26,0x00,0x00		// Save mask i:6 b:1 l:0 x:0 f:0 
/* @15 */ 	lda          #__i4			// stream
	ldx          #6
	jsr          __arg_value2
/* @19 */ 	lda          #__i5			// size
	ldx          #2
	jsr          __arg_value2
/* @23 */ 	lda          #__i0			// n
	ldx          #4
	jsr          __arg_value2
/* @32 */ 	lda          #__i1			// ptr
	ldx          #0
	jsr          __arg_value2
/* @37 */ 	lda          #__i9
/* @38 */ 	ldx          #__i5
/* @39 */ 	ldy          #__i0
/* @41 */ 	jsr         __umul2
/* @43 */ 	lda          #0
/* @44 */ 	sta         __i6
/* @46 */ 	lda          #0
/* @47 */ 	sta         __i6+1
/* @48 */ 	lda         __i1
/* @49 */ 	sta         __i7
/* @50 */ 	lda         __i1+1
/* @51 */ 	sta         __i7+1
.fread_label_52:
/* @58 */ 	ldx          #1
/* @59 */ 	lda          #0
/* @60 */ 	cmp         __i9+1
/* @61 */ 	bcc         .fread_label_57
/* @62 */ 	bne         .fread_label_56
/* @63 */ 	lda          #0
/* @64 */ 	cmp         __i9
/* @65 */ 	bcc         .fread_label_57
.fread_label_56:
/* @66 */ 	dex         
.fread_label_57:
/* @67 */ 	stx         __b2
/* @69 */ 	lda         __b2
/* @70 */ 	beq         .fread_label_101
/* @71 */ 	ldx         #__i4
/* @73 */ 	jsr         __pushreg2
/* @75 */ 	ldx         #__i0
/* @76 */ 	ldy          #0
/* @77 */ 	jsr         fgetc
/* @79 */ 	ldx          #2
/* @81 */ 	jsr         __incsp
/* @84 */ 	lda         __i0
/* @85 */ 	sta         __i8
/* @86 */ 	lda         __i0+1
/* @87 */ 	sta         __i8+1
/* @92 */ 	ldx          #0
/* @93 */ 	lda         __i0
/* @94 */ 	cmp          #255
/* @95 */ 	bne         .fread_label_91
/* @96 */ 	lda         __i0+1
/* @97 */ 	cmp          #255
/* @98 */ 	beq         .fread_label_90
.fread_label_91:
/* @99 */ 	inx         
.fread_label_90:
/* @100 */ 	stx         __b2
.fread_label_101:
/* @103 */ 	lda         __b2
/* @104 */ 	beq         .fread_label_132
/* @107 */ 	lda         __i7
/* @108 */ 	sta         __i0
/* @109 */ 	lda         __i7+1
/* @110 */ 	sta         __i0+1
/* @111 */ 	lda          #__i7
/* @113 */ 	jsr         __rinc21
/* @116 */ 	lda         __i8
/* @117 */ 	sta         __i1
/* @118 */ 	lda         __i8+1
/* @119 */ 	sta         __i1+1
/* @124 */ 	lda         __i1
/* @125 */ 	sta         (__i0)
/* @126 */ 	lda          #__i6
/* @127 */ 	jsr         __rinc21
/* @128 */ 	lda          #__i9
/* @130 */ 	jsr         __rdec21
/* @131 */ 	bra         .fread_label_52
.fread_label_132:
/* @135 */ 	lda          #__i0
/* @136 */ 	ldx          #__i6
/* @137 */ 	ldy          #__i5
/* @139 */ 	jsr         __udiv2
/* @141 */ 	ldx          #8
	jsr          __load_result
/* @142 */ 	lda         #__i0
/* @144 */ 	jsr         __result2
.fread_label_145:
/* @146 */ 	ldy          #10
	jsr          __leave
/* @147 */ 	rts         
.func_end_fread:
	.size fread, .func_end_fread-fread

	.global feof
	.type feof, @function

feof:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x00,0x00,0x00		// Save mask i:0 b:0 l:0 x:0 f:0 
/* @11 */ 	lda          #__i0			// stream
	ldx          #0
	jsr          __arg_value2
/* @15 */ 	ldy          #17
/* @16 */ 	lda         (__i0), Y
/* @18 */ 	sta         __b0
/* @22 */ 	lda         __b0
/* @23 */ 	sta         __i0
/* @25 */ 	and          #128
/* @27 */ 	beq         .feof_label_26
/* @29 */ 	lda          #255
.feof_label_26:
/* @31 */ 	sta         __i0+1
/* @33 */ 	lda         #__i0
/* @35 */ 	jsr         __result2
.feof_label_36:
/* @37 */ 	ldy          #8
	jsr          __leave_leaf
/* @38 */ 	rts         
.func_end_feof:
	.size feof, .func_end_feof-feof

	.global ferror
	.type ferror, @function

ferror:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x00,0x00,0x00		// Save mask i:0 b:0 l:0 x:0 f:0 
/* @11 */ 	lda          #__i0			// stream
	ldx          #0
	jsr          __arg_value2
/* @15 */ 	ldy          #18
/* @16 */ 	lda         (__i0), Y
/* @18 */ 	sta         __b0
/* @22 */ 	lda         __b0
/* @23 */ 	sta         __i0
/* @25 */ 	and          #128
/* @27 */ 	beq         .ferror_label_26
/* @29 */ 	lda          #255
.ferror_label_26:
/* @31 */ 	sta         __i0+1
/* @33 */ 	lda         #__i0
/* @35 */ 	jsr         __result2
.ferror_label_36:
/* @37 */ 	ldy          #8
	jsr          __leave_leaf
/* @38 */ 	rts         
.func_end_ferror:
	.size ferror, .func_end_ferror-ferror

	.global clearerr
	.type clearerr, @function

clearerr:
/* @3 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x00,0x00,0x00		// Save mask i:0 b:0 l:0 x:0 f:0 
/* @7 */ 	lda          #__i0			// stream
	ldx          #0
	jsr          __arg_value2
/* @9 */ 	lda          #0
/* @10 */ 	sta         __b0
/* @12 */ 	lda         __b0
/* @14 */ 	ldy          #18
/* @15 */ 	sta         (__i0), Y
/* @16 */ 	ldy          #8
	jsr          __leave_leaf_void
/* @17 */ 	rts         
.func_end_clearerr:
	.size clearerr, .func_end_clearerr-clearerr

	.data
	.p2align  0
s_stdin:
	.type   s_stdin,@object
	.local  s_stdin
	.size   s_stdin,29
	.short   0
	.short   768
	.short   128
	.short   0
	.short   0
	.short   0
	.short   2
	.space  15

	.p2align  0
s_stdout:
	.type   s_stdout,@object
	.local  s_stdout
	.size   s_stdout,29
	.short   1
	.short   896
	.short   128
	.short   0
	.short   0
	.short   0
	.short   2
	.space  15

	.p2align  0
s_stderr:
	.type   s_stderr,@object
	.local  s_stderr
	.size   s_stderr,29
	.short   2
	.short   0
	.short   0
	.short   0
	.short   0
	.short   0
	.short   3
	.space  15

	.p2align  0
stdin:
	.type   stdin,@object
	.global stdin
	.size   stdin,2
	.global s_stdin
	.hword    s_stdin

	.p2align  0
stdout:
	.type   stdout,@object
	.global stdout
	.size   stdout,2
	.global s_stdout
	.hword    s_stdout

	.p2align  0
stderr:
	.type   stderr,@object
	.global stderr
	.size   stderr,2
	.global s_stderr
	.hword    s_stderr

	.section ".rodata", "aMS", @progbits
