	.file   "/Users/dallison/Google Drive/c_compiler/libc/ctype.c"
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


	.global isalnum
	.type isalnum, @function

isalnum:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i1			// c
/* @22 */ 	lda         __i1
/* @23 */ 	sta         __i2
/* @26 */ 	stz         __i2+1
/* @30 */ 	sta         __i0
/* @31 */ 	lda         __i2+1
/* @32 */ 	sta         __i0+1
/* @35 */ 	lda         #%lo(char_traits)
/* @36 */ 	sta         __i2
/* @37 */ 	lda         #%hi(char_traits)
/* @38 */ 	sta         __i2+1
/* @42 */ 	clc         
/* @43 */ 	lda         __i2
/* @44 */ 	adc         __i0
/* @45 */ 	sta         __i3
/* @46 */ 	lda         __i2+1
/* @47 */ 	adc         __i0+1
/* @48 */ 	sta         __i3+1
/* @53 */ 	lda         (__i3)
/* @59 */ 	sta         __i2
/* @61 */ 	and          #128
/* @63 */ 	beq         .isalnum_label_62
/* @65 */ 	lda          #255
.isalnum_label_62:
/* @66 */ 	sta         __i2+1
/* @70 */ 	lda         __i2
/* @71 */ 	and          #3
/* @72 */ 	sta         __i3
/* @74 */ 	stz         __i3+1
/* @80 */ 	ldx          #1
/* @82 */ 	ora         __i3+1
/* @84 */ 	bne         .isalnum_label_79
/* @85 */ 	dex         
.isalnum_label_79:
/* @90 */ 	txa         
/* @91 */ 	sta         __i2
/* @93 */ 	stz         __i2+1
/* @95 */ 	lda         #__i2
/* @97 */ 	jsr         __result2
/* @99 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_isalnum:
	.size isalnum, .func_end_isalnum-isalnum

	.global isalpha
	.type isalpha, @function

isalpha:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @17 */ 	ldx          #0
	jsr          __arg_value2_i1			// c
/* @21 */ 	lda         __i1
/* @22 */ 	sta         __i2
/* @25 */ 	stz         __i2+1
/* @29 */ 	sta         __i0
/* @30 */ 	lda         __i2+1
/* @31 */ 	sta         __i0+1
/* @34 */ 	lda         #%lo(char_traits)
/* @35 */ 	sta         __i2
/* @36 */ 	lda         #%hi(char_traits)
/* @37 */ 	sta         __i2+1
/* @41 */ 	clc         
/* @42 */ 	lda         __i2
/* @43 */ 	adc         __i0
/* @44 */ 	sta         __i3
/* @45 */ 	lda         __i2+1
/* @46 */ 	adc         __i0+1
/* @47 */ 	sta         __i3+1
/* @52 */ 	lda         (__i3)
/* @58 */ 	sta         __i2
/* @60 */ 	and          #128
/* @62 */ 	beq         .isalpha_label_61
/* @64 */ 	lda          #255
.isalpha_label_61:
/* @65 */ 	sta         __i2+1
/* @69 */ 	lda         __i2
/* @70 */ 	and          #1
/* @71 */ 	sta         __i3
/* @73 */ 	stz         __i3+1
/* @79 */ 	ldx          #1
/* @81 */ 	ora         __i3+1
/* @83 */ 	bne         .isalpha_label_78
/* @84 */ 	dex         
.isalpha_label_78:
/* @89 */ 	txa         
/* @90 */ 	sta         __i2
/* @92 */ 	stz         __i2+1
/* @94 */ 	lda         #__i2
/* @96 */ 	jsr         __result2
/* @98 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_isalpha:
	.size isalpha, .func_end_isalpha-isalpha

	.global isblank
	.type isblank, @function

isblank:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @13 */ 	ldx          #0
	jsr          __arg_value2_i0			// c
/* @20 */ 	ldx          #1
/* @22 */ 	lda         __i0
/* @23 */ 	cmp          #9
/* @24 */ 	bne         .isblank_label_17
/* @25 */ 	lda         __i0+1
/* @27 */ 	beq         .isblank_label_18
.isblank_label_17:
/* @28 */ 	dex         
.isblank_label_18:
/* @29 */ 	stx         __b0
/* @31 */ 	txa         
/* @32 */ 	bne         .isblank_label_45
/* @36 */ 	ldx          #1
/* @37 */ 	lda         __i0
/* @38 */ 	cmp          #32
/* @39 */ 	bne         .isblank_label_34
/* @40 */ 	lda         __i0+1
/* @42 */ 	beq         .isblank_label_35
.isblank_label_34:
/* @43 */ 	dex         
.isblank_label_35:
/* @44 */ 	stx         __b0
.isblank_label_45:
/* @49 */ 	lda         __b0
/* @50 */ 	sta         __i1
/* @52 */ 	stz         __i1+1
/* @54 */ 	lda         #__i1
/* @56 */ 	jsr         __result2
/* @58 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_isblank:
	.size isblank, .func_end_isblank-isblank

	.global iscntrl
	.type iscntrl, @function

iscntrl:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i1			// c
/* @22 */ 	lda         __i1
/* @23 */ 	sta         __i2
/* @26 */ 	stz         __i2+1
/* @30 */ 	sta         __i0
/* @31 */ 	lda         __i2+1
/* @32 */ 	sta         __i0+1
/* @35 */ 	lda         #%lo(char_traits)
/* @36 */ 	sta         __i2
/* @37 */ 	lda         #%hi(char_traits)
/* @38 */ 	sta         __i2+1
/* @42 */ 	clc         
/* @43 */ 	lda         __i2
/* @44 */ 	adc         __i0
/* @45 */ 	sta         __i3
/* @46 */ 	lda         __i2+1
/* @47 */ 	adc         __i0+1
/* @48 */ 	sta         __i3+1
/* @53 */ 	lda         (__i3)
/* @59 */ 	sta         __i2
/* @61 */ 	and          #128
/* @63 */ 	beq         .iscntrl_label_62
/* @65 */ 	lda          #255
.iscntrl_label_62:
/* @66 */ 	sta         __i2+1
/* @70 */ 	lda         __i2
/* @71 */ 	and          #16
/* @72 */ 	sta         __i3
/* @74 */ 	stz         __i3+1
/* @80 */ 	ldx          #1
/* @82 */ 	ora         __i3+1
/* @84 */ 	bne         .iscntrl_label_79
/* @85 */ 	dex         
.iscntrl_label_79:
/* @90 */ 	txa         
/* @91 */ 	sta         __i2
/* @93 */ 	stz         __i2+1
/* @95 */ 	lda         #__i2
/* @97 */ 	jsr         __result2
/* @99 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_iscntrl:
	.size iscntrl, .func_end_iscntrl-iscntrl

	.global isdigit
	.type isdigit, @function

isdigit:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i1			// c
/* @22 */ 	lda         __i1
/* @23 */ 	sta         __i2
/* @26 */ 	stz         __i2+1
/* @30 */ 	sta         __i0
/* @31 */ 	lda         __i2+1
/* @32 */ 	sta         __i0+1
/* @35 */ 	lda         #%lo(char_traits)
/* @36 */ 	sta         __i2
/* @37 */ 	lda         #%hi(char_traits)
/* @38 */ 	sta         __i2+1
/* @42 */ 	clc         
/* @43 */ 	lda         __i2
/* @44 */ 	adc         __i0
/* @45 */ 	sta         __i3
/* @46 */ 	lda         __i2+1
/* @47 */ 	adc         __i0+1
/* @48 */ 	sta         __i3+1
/* @53 */ 	lda         (__i3)
/* @59 */ 	sta         __i2
/* @61 */ 	and          #128
/* @63 */ 	beq         .isdigit_label_62
/* @65 */ 	lda          #255
.isdigit_label_62:
/* @66 */ 	sta         __i2+1
/* @70 */ 	lda         __i2
/* @71 */ 	and          #2
/* @72 */ 	sta         __i3
/* @74 */ 	stz         __i3+1
/* @80 */ 	ldx          #1
/* @82 */ 	ora         __i3+1
/* @84 */ 	bne         .isdigit_label_79
/* @85 */ 	dex         
.isdigit_label_79:
/* @90 */ 	txa         
/* @91 */ 	sta         __i2
/* @93 */ 	stz         __i2+1
/* @95 */ 	lda         #__i2
/* @97 */ 	jsr         __result2
/* @99 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_isdigit:
	.size isdigit, .func_end_isdigit-isdigit

	.global isgraph
	.type isgraph, @function

isgraph:
/* @5 */ 	stx         __result
/* @7 */ 	sty         __result+1
/* @8 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @12 */ 	ldx          #0
	jsr          __arg_value2_i0			// c
/* @18 */ 	ldx          #0
/* @19 */ 	lda         __i0
/* @20 */ 	cmp          #32
/* @21 */ 	bne         .isgraph_label_16
/* @23 */ 	lda         __i0+1
/* @25 */ 	beq         .isgraph_label_15
.isgraph_label_16:
/* @26 */ 	inx         
.isgraph_label_15:
/* @31 */ 	txa         
/* @32 */ 	sta         __i1
/* @34 */ 	stz         __i1+1
/* @36 */ 	lda         #__i1
/* @38 */ 	jsr         __result2
/* @40 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_isgraph:
	.size isgraph, .func_end_isgraph-isgraph

	.global islower
	.type islower, @function

islower:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i1			// c
/* @22 */ 	lda         __i1
/* @23 */ 	sta         __i2
/* @26 */ 	stz         __i2+1
/* @30 */ 	sta         __i0
/* @31 */ 	lda         __i2+1
/* @32 */ 	sta         __i0+1
/* @35 */ 	lda         #%lo(char_traits)
/* @36 */ 	sta         __i2
/* @37 */ 	lda         #%hi(char_traits)
/* @38 */ 	sta         __i2+1
/* @42 */ 	clc         
/* @43 */ 	lda         __i2
/* @44 */ 	adc         __i0
/* @45 */ 	sta         __i3
/* @46 */ 	lda         __i2+1
/* @47 */ 	adc         __i0+1
/* @48 */ 	sta         __i3+1
/* @53 */ 	lda         (__i3)
/* @59 */ 	sta         __i2
/* @61 */ 	and          #128
/* @63 */ 	beq         .islower_label_62
/* @65 */ 	lda          #255
.islower_label_62:
/* @66 */ 	sta         __i2+1
/* @70 */ 	lda         __i2
/* @71 */ 	and          #32
/* @72 */ 	sta         __i3
/* @74 */ 	stz         __i3+1
/* @80 */ 	ldx          #1
/* @82 */ 	ora         __i3+1
/* @84 */ 	bne         .islower_label_79
/* @85 */ 	dex         
.islower_label_79:
/* @90 */ 	txa         
/* @91 */ 	sta         __i2
/* @93 */ 	stz         __i2+1
/* @95 */ 	lda         #__i2
/* @97 */ 	jsr         __result2
/* @99 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_islower:
	.size islower, .func_end_islower-islower

	.global isprint
	.type isprint, @function

isprint:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i1			// c
/* @22 */ 	lda         __i1
/* @23 */ 	sta         __i2
/* @26 */ 	stz         __i2+1
/* @30 */ 	sta         __i0
/* @31 */ 	lda         __i2+1
/* @32 */ 	sta         __i0+1
/* @35 */ 	lda         #%lo(char_traits)
/* @36 */ 	sta         __i2
/* @37 */ 	lda         #%hi(char_traits)
/* @38 */ 	sta         __i2+1
/* @42 */ 	clc         
/* @43 */ 	lda         __i2
/* @44 */ 	adc         __i0
/* @45 */ 	sta         __i3
/* @46 */ 	lda         __i2+1
/* @47 */ 	adc         __i0+1
/* @48 */ 	sta         __i3+1
/* @53 */ 	lda         (__i3)
/* @59 */ 	sta         __i2
/* @61 */ 	and          #128
/* @63 */ 	beq         .isprint_label_62
/* @65 */ 	lda          #255
.isprint_label_62:
/* @66 */ 	sta         __i2+1
/* @70 */ 	lda         __i2
/* @71 */ 	and          #15
/* @72 */ 	sta         __i3
/* @74 */ 	stz         __i3+1
/* @80 */ 	ldx          #1
/* @82 */ 	ora         __i3+1
/* @84 */ 	bne         .isprint_label_79
/* @85 */ 	dex         
.isprint_label_79:
/* @90 */ 	txa         
/* @91 */ 	sta         __i2
/* @93 */ 	stz         __i2+1
/* @95 */ 	lda         #__i2
/* @97 */ 	jsr         __result2
/* @99 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_isprint:
	.size isprint, .func_end_isprint-isprint

	.global ispunct
	.type ispunct, @function

ispunct:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @17 */ 	ldx          #0
	jsr          __arg_value2_i1			// c
/* @21 */ 	lda         __i1
/* @22 */ 	sta         __i2
/* @25 */ 	stz         __i2+1
/* @29 */ 	sta         __i0
/* @30 */ 	lda         __i2+1
/* @31 */ 	sta         __i0+1
/* @34 */ 	lda         #%lo(char_traits)
/* @35 */ 	sta         __i2
/* @36 */ 	lda         #%hi(char_traits)
/* @37 */ 	sta         __i2+1
/* @41 */ 	clc         
/* @42 */ 	lda         __i2
/* @43 */ 	adc         __i0
/* @44 */ 	sta         __i3
/* @45 */ 	lda         __i2+1
/* @46 */ 	adc         __i0+1
/* @47 */ 	sta         __i3+1
/* @52 */ 	lda         (__i3)
/* @58 */ 	sta         __i2
/* @60 */ 	and          #128
/* @62 */ 	beq         .ispunct_label_61
/* @64 */ 	lda          #255
.ispunct_label_61:
/* @65 */ 	sta         __i2+1
/* @69 */ 	lda         __i2
/* @70 */ 	and          #8
/* @71 */ 	sta         __i3
/* @73 */ 	stz         __i3+1
/* @79 */ 	ldx          #1
/* @81 */ 	ora         __i3+1
/* @83 */ 	bne         .ispunct_label_78
/* @84 */ 	dex         
.ispunct_label_78:
/* @89 */ 	txa         
/* @90 */ 	sta         __i2
/* @92 */ 	stz         __i2+1
/* @94 */ 	lda         #__i2
/* @96 */ 	jsr         __result2
/* @98 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_ispunct:
	.size ispunct, .func_end_ispunct-ispunct

	.global isspace
	.type isspace, @function

isspace:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i1			// c
/* @22 */ 	lda         __i1
/* @23 */ 	sta         __i2
/* @26 */ 	stz         __i2+1
/* @30 */ 	sta         __i0
/* @31 */ 	lda         __i2+1
/* @32 */ 	sta         __i0+1
/* @35 */ 	lda         #%lo(char_traits)
/* @36 */ 	sta         __i2
/* @37 */ 	lda         #%hi(char_traits)
/* @38 */ 	sta         __i2+1
/* @42 */ 	clc         
/* @43 */ 	lda         __i2
/* @44 */ 	adc         __i0
/* @45 */ 	sta         __i3
/* @46 */ 	lda         __i2+1
/* @47 */ 	adc         __i0+1
/* @48 */ 	sta         __i3+1
/* @53 */ 	lda         (__i3)
/* @59 */ 	sta         __i2
/* @61 */ 	and          #128
/* @63 */ 	beq         .isspace_label_62
/* @65 */ 	lda          #255
.isspace_label_62:
/* @66 */ 	sta         __i2+1
/* @70 */ 	lda         __i2
/* @71 */ 	and          #4
/* @72 */ 	sta         __i3
/* @74 */ 	stz         __i3+1
/* @80 */ 	ldx          #1
/* @82 */ 	ora         __i3+1
/* @84 */ 	bne         .isspace_label_79
/* @85 */ 	dex         
.isspace_label_79:
/* @90 */ 	txa         
/* @91 */ 	sta         __i2
/* @93 */ 	stz         __i2+1
/* @95 */ 	lda         #__i2
/* @97 */ 	jsr         __result2
/* @99 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_isspace:
	.size isspace, .func_end_isspace-isspace

	.global isupper
	.type isupper, @function

isupper:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i1			// c
/* @22 */ 	lda         __i1
/* @23 */ 	sta         __i2
/* @26 */ 	stz         __i2+1
/* @30 */ 	sta         __i0
/* @31 */ 	lda         __i2+1
/* @32 */ 	sta         __i0+1
/* @35 */ 	lda         #%lo(char_traits)
/* @36 */ 	sta         __i2
/* @37 */ 	lda         #%hi(char_traits)
/* @38 */ 	sta         __i2+1
/* @42 */ 	clc         
/* @43 */ 	lda         __i2
/* @44 */ 	adc         __i0
/* @45 */ 	sta         __i3
/* @46 */ 	lda         __i2+1
/* @47 */ 	adc         __i0+1
/* @48 */ 	sta         __i3+1
/* @53 */ 	lda         (__i3)
/* @59 */ 	sta         __i2
/* @61 */ 	and          #128
/* @63 */ 	beq         .isupper_label_62
/* @65 */ 	lda          #255
.isupper_label_62:
/* @66 */ 	sta         __i2+1
/* @70 */ 	lda         __i2
/* @71 */ 	and          #64
/* @72 */ 	sta         __i3
/* @74 */ 	stz         __i3+1
/* @80 */ 	ldx          #1
/* @82 */ 	ora         __i3+1
/* @84 */ 	bne         .isupper_label_79
/* @85 */ 	dex         
.isupper_label_79:
/* @90 */ 	txa         
/* @91 */ 	sta         __i2
/* @93 */ 	stz         __i2+1
/* @95 */ 	lda         #__i2
/* @97 */ 	jsr         __result2
/* @99 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_isupper:
	.size isupper, .func_end_isupper-isupper

	.global isxdigit
	.type isxdigit, @function

isxdigit:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i1			// c
/* @22 */ 	lda         __i1
/* @23 */ 	sta         __i2
/* @26 */ 	stz         __i2+1
/* @30 */ 	sta         __i0
/* @31 */ 	lda         __i2+1
/* @32 */ 	sta         __i0+1
/* @35 */ 	lda         #%lo(char_traits)
/* @36 */ 	sta         __i2
/* @37 */ 	lda         #%hi(char_traits)
/* @38 */ 	sta         __i2+1
/* @42 */ 	clc         
/* @43 */ 	lda         __i2
/* @44 */ 	adc         __i0
/* @45 */ 	sta         __i3
/* @46 */ 	lda         __i2+1
/* @47 */ 	adc         __i0+1
/* @48 */ 	sta         __i3+1
/* @53 */ 	lda         (__i3)
/* @59 */ 	sta         __i2
/* @61 */ 	and          #128
/* @63 */ 	beq         .isxdigit_label_62
/* @65 */ 	lda          #255
.isxdigit_label_62:
/* @66 */ 	sta         __i2+1
/* @70 */ 	lda         __i2
/* @71 */ 	and          #128
/* @72 */ 	sta         __i3
/* @74 */ 	stz         __i3+1
/* @80 */ 	ldx          #1
/* @82 */ 	ora         __i3+1
/* @84 */ 	bne         .isxdigit_label_79
/* @85 */ 	dex         
.isxdigit_label_79:
/* @90 */ 	txa         
/* @91 */ 	sta         __i2
/* @93 */ 	stz         __i2+1
/* @95 */ 	lda         #__i2
/* @97 */ 	jsr         __result2
/* @99 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.func_end_isxdigit:
	.size isxdigit, .func_end_isxdigit-isxdigit

	.global tolower
	.type tolower, @function

tolower:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #7
	jsr          __enter_nomask
/* @15 */ 	ldx          #0
	jsr          __arg_value2_i0			// c
/* @18 */ 	ldx         __i0
/* @20 */ 	ldy         __i0+1
/* @22 */ 	jsr         __builtin_isupper
/* @24 */ 	stz         __i1+1
/* @28 */ 	cmp          #0
/* @29 */ 	beq         .tolower_label_57
/* @32 */ 	sec         
/* @33 */ 	lda         __i0
/* @34 */ 	sbc          #65
/* @35 */ 	sta         __i1
/* @36 */ 	lda         __i0+1
/* @37 */ 	sbc          #0
/* @38 */ 	sta         __i1+1
/* @42 */ 	clc         
/* @43 */ 	lda         __i1
/* @44 */ 	adc          #97
/* @45 */ 	sta         __i2
/* @46 */ 	lda         __i1+1
/* @47 */ 	adc          #0
/* @48 */ 	sta         __i2+1
/* @50 */ 	ldx          #8
	jsr          __load_result
/* @51 */ 	lda         #__i2
/* @53 */ 	jsr         __result2
.tolower_label_54:
/* @55 */ 	ldy          #10
	jmp          __leave_nomask
.tolower_label_57:
/* @58 */ 	ldx          #8
	jsr          __load_result
/* @59 */ 	lda         #__i0
/* @60 */ 	jsr         __result2
/* @61 */ 	bra         .tolower_label_54
.func_end_tolower:
	.size tolower, .func_end_tolower-tolower

	.global toupper
	.type toupper, @function

toupper:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #7
	jsr          __enter_nomask
/* @15 */ 	ldx          #0
	jsr          __arg_value2_i0			// c
/* @18 */ 	ldx         __i0
/* @20 */ 	ldy         __i0+1
/* @22 */ 	jsr         __builtin_islower
/* @24 */ 	stz         __i1+1
/* @28 */ 	cmp          #0
/* @29 */ 	beq         .toupper_label_57
/* @32 */ 	sec         
/* @33 */ 	lda         __i0
/* @34 */ 	sbc          #97
/* @35 */ 	sta         __i1
/* @36 */ 	lda         __i0+1
/* @37 */ 	sbc          #0
/* @38 */ 	sta         __i1+1
/* @42 */ 	clc         
/* @43 */ 	lda         __i1
/* @44 */ 	adc          #65
/* @45 */ 	sta         __i2
/* @46 */ 	lda         __i1+1
/* @47 */ 	adc          #0
/* @48 */ 	sta         __i2+1
/* @50 */ 	ldx          #8
	jsr          __load_result
/* @51 */ 	lda         #__i2
/* @53 */ 	jsr         __result2
.toupper_label_54:
/* @55 */ 	ldy          #10
	jmp          __leave_nomask
.toupper_label_57:
/* @58 */ 	ldx          #8
	jsr          __load_result
/* @59 */ 	lda         #__i0
/* @60 */ 	jsr         __result2
/* @61 */ 	bra         .toupper_label_54
.func_end_toupper:
	.size toupper, .func_end_toupper-toupper

	.data
	.p2align  0
char_traits:
	.type   char_traits,@object
	.local  char_traits
	.size   char_traits,256
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   20
	.byte   20
	.byte   20
	.byte   20
	.byte   20
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   16
	.byte   4
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   130
	.byte   130
	.byte   130
	.byte   130
	.byte   130
	.byte   130
	.byte   130
	.byte   130
	.byte   130
	.byte   130
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   193
	.byte   193
	.byte   193
	.byte   193
	.byte   193
	.byte   193
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   65
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   161
	.byte   161
	.byte   161
	.byte   161
	.byte   161
	.byte   161
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   33
	.byte   8
	.byte   8
	.byte   8
	.byte   8
	.byte   16
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0
	.byte   0

	.section ".rodata", "aMS", @progbits
