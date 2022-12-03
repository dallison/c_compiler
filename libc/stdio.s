	.file   "/Users/dallison/Google Drive/c_compiler/libc/stdio.c"
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


	.global setbuf
	.type setbuf, @function

setbuf:
/* @9 */ 	ldx          #7
	jsr          __enter
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
/* @14 */ 	ldx          #2
	jsr          __arg_value2_i5			// buf
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i6			// stream
/* @20 */ 	ldx          #64
/* @22 */ 	jsr         __pushxy0
/* @24 */ 	lda         __i5
/* @26 */ 	ora         __i5+1
/* @28 */ 	beq         .setbuf_label_35
/* @30 */ 	lda          #1
/* @31 */ 	sta         __i4
/* @32 */ 	dec          A
/* @33 */ 	stz         __i4+1
/* @34 */ 	bra         .setbuf_label_41
.setbuf_label_35:
/* @37 */ 	lda          #3
/* @38 */ 	sta         __i4
/* @40 */ 	stz         __i4+1
.setbuf_label_41:
/* @45 */ 	lda         __i4+1
/* @47 */ 	and          #128
/* @48 */ 	ora         __i4
/* @54 */ 	jsr         __pusha
/* @55 */ 	jsr         __pushi5
/* @56 */ 	jsr         __pushi6
/* @58 */ 	ldx         #__i0
/* @59 */ 	ldy          #0
/* @60 */ 	jsr         setvbuf
/* @62 */ 	jsr         __incsp8
/* @63 */ 	ldy          #10
	jmp          __leave_void
.func_end_setbuf:
	.size setbuf, .func_end_setbuf-setbuf

	.global setvbuf
	.type setvbuf, @function

setvbuf:
/* @12 */ 	stx         __result
/* @14 */ 	sty         __result+1
/* @15 */ 	ldx          #7
	jsr          __enter
	.byte        0x84,0x00,0x00		// Save mask i:4 b:4 l:0 x:0 f:0 
/* @21 */ 	ldx          #4
	jsr          __arg_value1_b2			// mode
/* @25 */ 	ldx          #0
	jsr          __arg_value2_i4			// stream
/* @29 */ 	ldx          #2
	jsr          __arg_value2_i5			// buf
/* @33 */ 	ldx          #6
	jsr          __arg_value2_i6			// size
/* @39 */ 	lda         __b2
/* @40 */ 	sta         __i0
/* @42 */ 	and          #128
/* @44 */ 	beq         .setvbuf_label_43
/* @46 */ 	lda          #255
.setvbuf_label_43:
/* @47 */ 	sta         __i0+1
/* @52 */ 	ldx          #0
/* @53 */ 	lda         __i0
/* @54 */ 	cmp          #3
/* @55 */ 	bne         .setvbuf_label_51
/* @56 */ 	lda         __i0+1
/* @58 */ 	beq         .setvbuf_label_50
.setvbuf_label_51:
/* @59 */ 	inx         
.setvbuf_label_50:
/* @60 */ 	stx         __b3
/* @62 */ 	txa         
/* @63 */ 	cmp          #0
/* @64 */ 	beq         .setvbuf_label_87
/* @67 */ 	lda         __b2
/* @68 */ 	sta         __i0
/* @69 */ 	and          #128
/* @71 */ 	beq         .setvbuf_label_70
/* @72 */ 	lda          #255
.setvbuf_label_70:
/* @73 */ 	sta         __i0+1
/* @78 */ 	ldx          #0
/* @79 */ 	lda         __i0
/* @80 */ 	cmp          #2
/* @81 */ 	bne         .setvbuf_label_77
/* @82 */ 	lda         __i0+1
/* @84 */ 	beq         .setvbuf_label_76
.setvbuf_label_77:
/* @85 */ 	inx         
.setvbuf_label_76:
/* @86 */ 	stx         __b3
.setvbuf_label_87:
/* @89 */ 	lda         __b3
/* @91 */ 	beq         .setvbuf_label_114
/* @94 */ 	lda         __b2
/* @95 */ 	sta         __i0
/* @96 */ 	and          #128
/* @98 */ 	beq         .setvbuf_label_97
/* @99 */ 	lda          #255
.setvbuf_label_97:
/* @100 */ 	sta         __i0+1
/* @105 */ 	ldx          #0
/* @106 */ 	lda         __i0
/* @107 */ 	cmp          #1
/* @108 */ 	bne         .setvbuf_label_104
/* @109 */ 	lda         __i0+1
/* @111 */ 	beq         .setvbuf_label_103
.setvbuf_label_104:
/* @112 */ 	inx         
.setvbuf_label_103:
/* @113 */ 	stx         __b3
.setvbuf_label_114:
/* @116 */ 	lda         __b3
/* @118 */ 	beq         .setvbuf_label_132
/* @121 */ 	lda          #255
/* @122 */ 	sta         __i0
/* @124 */ 	sta         __i0+1
/* @125 */ 	ldx          #8
	jsr          __load_result
/* @126 */ 	lda         #__i0
/* @128 */ 	jsr         __result2
.setvbuf_label_129:
/* @130 */ 	ldy          #10
	jmp          __leave
.setvbuf_label_132:
/* @138 */ 	ldy          #2
/* @139 */ 	lda         (__i4), Y
/* @140 */ 	sta         __i0
/* @142 */ 	iny         
/* @143 */ 	lda         (__i4), Y
/* @144 */ 	sta         __i0+1
/* @149 */ 	ldx          #0
/* @150 */ 	lda         __i0
/* @152 */ 	bne         .setvbuf_label_148
/* @153 */ 	lda         __i0+1
/* @155 */ 	beq         .setvbuf_label_147
.setvbuf_label_148:
/* @156 */ 	inx         
.setvbuf_label_147:
/* @157 */ 	stx         __b4
/* @159 */ 	txa         
/* @160 */ 	cmp          #0
/* @161 */ 	beq         .setvbuf_label_167
/* @164 */ 	ldy          #14
/* @165 */ 	lda         (__i4), Y
/* @166 */ 	sta         __b4
.setvbuf_label_167:
/* @169 */ 	lda         __b4
/* @171 */ 	beq         .setvbuf_label_186
/* @174 */ 	ldy          #2
/* @175 */ 	lda         (__i4), Y
/* @176 */ 	sta         __i0
/* @177 */ 	iny         
/* @178 */ 	lda         (__i4), Y
/* @179 */ 	sta         __i0+1
/* @182 */ 	jsr         __pushi0
/* @183 */ 	jsr         free
/* @185 */ 	jsr         __incsp2
.setvbuf_label_186:
/* @192 */ 	ldx          #1
/* @193 */ 	lda         __i5
/* @195 */ 	bne         .setvbuf_label_190
/* @196 */ 	lda         __i5+1
/* @198 */ 	beq         .setvbuf_label_191
.setvbuf_label_190:
/* @199 */ 	dex         
.setvbuf_label_191:
/* @200 */ 	stx         __b5
/* @202 */ 	txa         
/* @203 */ 	cmp          #0
/* @204 */ 	beq         .setvbuf_label_227
/* @207 */ 	lda         __b2
/* @208 */ 	sta         __i0
/* @209 */ 	and          #128
/* @211 */ 	beq         .setvbuf_label_210
/* @212 */ 	lda          #255
.setvbuf_label_210:
/* @213 */ 	sta         __i0+1
/* @218 */ 	ldx          #0
/* @219 */ 	lda         __i0
/* @220 */ 	cmp          #3
/* @221 */ 	bne         .setvbuf_label_217
/* @222 */ 	lda         __i0+1
/* @224 */ 	beq         .setvbuf_label_216
.setvbuf_label_217:
/* @225 */ 	inx         
.setvbuf_label_216:
/* @226 */ 	stx         __b5
.setvbuf_label_227:
/* @229 */ 	lda         __b5
/* @231 */ 	beq         .setvbuf_label_250
/* @232 */ 	jsr         __pushi6
/* @234 */ 	ldx         #__i7
/* @235 */ 	ldy          #0
/* @236 */ 	jsr         malloc
/* @237 */ 	jsr         __incsp2
/* @240 */ 	lda         __i7
/* @241 */ 	ldy          #2
/* @242 */ 	sta         (__i4), Y
/* @243 */ 	lda         __i7+1
/* @244 */ 	iny         
/* @245 */ 	sta         (__i4), Y
/* @246 */ 	lda          #1
/* @247 */ 	ldy          #14
/* @248 */ 	sta         (__i4), Y
/* @249 */ 	bra         .setvbuf_label_257
.setvbuf_label_250:
/* @251 */ 	lda         __i5
/* @252 */ 	ldy          #2
/* @253 */ 	sta         (__i4), Y
/* @254 */ 	lda         __i5+1
/* @255 */ 	iny         
/* @256 */ 	sta         (__i4), Y
.setvbuf_label_257:
/* @258 */ 	lda         __i6
/* @260 */ 	ldy          #4
/* @261 */ 	sta         (__i4), Y
/* @262 */ 	lda         __i6+1
/* @264 */ 	iny         
/* @265 */ 	sta         (__i4), Y
/* @268 */ 	lda         __b2
/* @269 */ 	sta         __i0
/* @270 */ 	and          #128
/* @272 */ 	beq         .setvbuf_label_271
/* @273 */ 	lda          #255
.setvbuf_label_271:
/* @274 */ 	sta         __i0+1
/* @277 */ 	lda         __i0
/* @279 */ 	ldy          #12
/* @280 */ 	sta         (__i4), Y
/* @281 */ 	lda         __i0+1
/* @283 */ 	iny         
/* @284 */ 	sta         (__i4), Y
/* @287 */ 	stz         __i0
/* @288 */ 	stz         __i0+1
/* @289 */ 	ldx          #8
	jsr          __load_result
/* @290 */ 	lda         #__i0
/* @291 */ 	jsr         __result2
/* @292 */ 	jmp         .setvbuf_label_129
.func_end_setvbuf:
	.size setvbuf, .func_end_setvbuf-setvbuf

	.global feof
	.type feof, @function

feof:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @11 */ 	ldx          #0
	jsr          __arg_value2_i0			// stream
/* @15 */ 	ldy          #16
/* @16 */ 	lda         (__i0), Y
/* @23 */ 	sta         __i1
/* @25 */ 	and          #128
/* @27 */ 	beq         .feof_label_26
/* @29 */ 	lda          #255
.feof_label_26:
/* @31 */ 	sta         __i1+1
/* @33 */ 	lda         #__i1
/* @35 */ 	jsr         __result2
/* @37 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_feof:
	.size feof, .func_end_feof-feof

	.global ferror
	.type ferror, @function

ferror:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @11 */ 	ldx          #0
	jsr          __arg_value2_i0			// stream
/* @15 */ 	ldy          #17
/* @16 */ 	lda         (__i0), Y
/* @23 */ 	sta         __i1
/* @25 */ 	and          #128
/* @27 */ 	beq         .ferror_label_26
/* @29 */ 	lda          #255
.ferror_label_26:
/* @31 */ 	sta         __i1+1
/* @33 */ 	lda         #__i1
/* @35 */ 	jsr         __result2
/* @37 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_ferror:
	.size ferror, .func_end_ferror-ferror

	.global clearerr
	.type clearerr, @function

clearerr:
/* @3 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @7 */ 	ldx          #0
	jsr          __arg_value2_i0			// stream
/* @8 */ 	lda          #0
/* @10 */ 	ldy          #17
/* @11 */ 	sta         (__i0), Y
/* @12 */ 	ldy          #8
	jmp          __leave_leaf_void_nomask
.func_end_clearerr:
	.size clearerr, .func_end_clearerr-clearerr

	.data
	.p2align  0
s_stdin:
	.type   s_stdin,@object
	.global s_stdin
	.size   s_stdin,28
	.short   0
	.global s_stdin_buf
	.hword    s_stdin_buf
	.short   64
	.short   0
	.short   0
	.short   0
	.short   2
	.byte   0
	.space  7

	.p2align  0
s_stdout:
	.type   s_stdout,@object
	.global s_stdout
	.size   s_stdout,28
	.short   1
	.global s_stdout_buf
	.hword    s_stdout_buf
	.short   64
	.short   0
	.short   0
	.short   0
	.short   2
	.space  8

	.p2align  0
s_stderr:
	.type   s_stderr,@object
	.global s_stderr
	.size   s_stderr,28
	.short   2
	.short   0
	.short   0
	.short   0
	.short   0
	.short   0
	.short   3
	.space  14

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

	.type   s_stdin_buf,@object
	.global s_stdin_buf
	.comm   s_stdin_buf,64,1

	.type   s_stdout_buf,@object
	.global s_stdout_buf
	.comm   s_stdout_buf,64,1

	.section ".rodata", "aMS", @progbits
