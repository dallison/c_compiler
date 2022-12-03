	.file   "/Users/dallison/Google Drive/c_compiler/libc/exit.c"
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


	.global atexit
	.type atexit, @function

atexit:
/* @9 */ 	stx         __result
/* @11 */ 	sty         __result+1
/* @12 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @21 */ 	ldx          #0
	jsr          __arg_value2_i0			// p
/* @24 */ 	lda         numfuncs+0
/* @25 */ 	sta         __i1
/* @27 */ 	stz         __i1+1
/* @30 */ 	lda          #0
/* @31 */ 	cmp         __i1+1
/* @32 */ 	bcc         .atexit_label_29
/* @33 */ 	bne         .atexit_label_50
/* @34 */ 	lda          #31
/* @35 */ 	cmp         __i1
/* @36 */ 	bcs         .atexit_label_50
.atexit_label_29:
/* @40 */ 	lda          #255
/* @41 */ 	sta         __i1
/* @43 */ 	sta         __i1+1
/* @44 */ 	lda         #__i1
/* @46 */ 	jsr         __result2
.atexit_label_47:
/* @48 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.atexit_label_50:
/* @53 */ 	lda         numfuncs+0
/* @54 */ 	sta         __b1
/* @57 */ 	lda         #%lo(numfuncs)
/* @58 */ 	sta         __i1
/* @59 */ 	lda         #%hi(numfuncs)
/* @60 */ 	sta         __i1+1
/* @62 */ 	lda          #__i1
/* @64 */ 	jsr         __inc1
/* @67 */ 	lda         __b1
/* @72 */ 	sta         __i1
/* @74 */ 	and          #128
/* @76 */ 	beq         .atexit_label_75
/* @78 */ 	lda          #255
.atexit_label_75:
/* @79 */ 	sta         __i1+1
/* @83 */ 	lda         __i1
/* @84 */ 	asl          A
/* @85 */ 	sta         __i2
/* @86 */ 	lda         __i1+1
/* @87 */ 	rol          A
/* @88 */ 	sta         __i2+1
/* @91 */ 	lda         #%lo(atexit_funcs)
/* @92 */ 	sta         __i1
/* @93 */ 	lda         #%hi(atexit_funcs)
/* @94 */ 	sta         __i1+1
/* @99 */ 	clc         
/* @100 */ 	lda         __i1
/* @101 */ 	adc         __i2
/* @102 */ 	sta         __i3
/* @103 */ 	lda         __i1+1
/* @104 */ 	adc         __i2+1
/* @105 */ 	sta         __i3+1
/* @108 */ 	lda         __i0
/* @109 */ 	sta         (__i3)
/* @110 */ 	lda         __i0+1
/* @111 */ 	ldy          #1
/* @112 */ 	sta         (__i3), Y
/* @115 */ 	stz         __i1
/* @116 */ 	stz         __i1+1
/* @117 */ 	lda         #__i1
/* @118 */ 	jsr         __result2
/* @119 */ 	bra         .atexit_label_47
.func_end_atexit:
	.size atexit, .func_end_atexit-atexit

	.global exit
	.type exit, @function

exit:
/* @8 */ 	ldx          #71
	jsr          __enter
	.byte        0x21,0x00,0x00		// Save mask i:1 b:1 l:0 x:0 f:0 
/* @20 */ 	ldx          #0
	jsr          __arg_value2_i4			// status
.exit_label_21:
/* @23 */ 	lda         numfuncs+0
/* @26 */ 	stz         __i0+1
/* @31 */ 	bne         .exit_label_150
/* @151 */ 	jmp         .exit_label_143
.exit_label_150:
/* @33 */ 	ldx          #67
	jsr          __var_addr_i0			// funcs
/* @37 */ 	lda          #64
/* @38 */ 	sta         __i1
/* @40 */ 	stz         __i1+1
/* @44 */ 	sta         __mem_size
/* @45 */ 	lda         __i1+1
/* @47 */ 	sta         __mem_size+1
/* @50 */ 	lda         #%lo(atexit_funcs)
/* @51 */ 	sta         __i1
/* @52 */ 	lda         #%hi(atexit_funcs)
/* @53 */ 	sta         __i1+1
/* @55 */ 	lda         __i1
/* @57 */ 	sta         __mem_src
/* @58 */ 	lda         __i1+1
/* @60 */ 	sta         __mem_src+1
/* @62 */ 	lda         __i0
/* @64 */ 	sta         __mem_dest
/* @65 */ 	lda         __i0+1
/* @67 */ 	sta         __mem_dest+1
/* @69 */ 	jsr         __builtin_memcpy
/* @70 */ 	lda         numfuncs+0
/* @71 */ 	sta         __b2
/* @72 */ 	stz         numfuncs+0
.exit_label_73:
/* @75 */ 	lda         __b2
/* @78 */ 	stz         __i0+1
/* @83 */ 	beq         .exit_label_141
/* @85 */ 	lda         __b2
/* @86 */ 	sta         __i0
/* @88 */ 	stz         __i0+1
/* @92 */ 	sec         
/* @94 */ 	sbc          #1
/* @95 */ 	sta         __i1
/* @96 */ 	lda         __i0+1
/* @97 */ 	sbc          #0
/* @98 */ 	sta         __i1+1
/* @102 */ 	lda         __i1
/* @103 */ 	asl          A
/* @104 */ 	sta         __i0
/* @105 */ 	lda         __i1+1
/* @106 */ 	rol          A
/* @107 */ 	sta         __i0+1
/* @109 */ 	ldx          #67
	jsr          __var_addr_i1			// funcs
/* @114 */ 	clc         
/* @115 */ 	lda         __i1
/* @116 */ 	adc         __i0
/* @117 */ 	sta         __i2
/* @118 */ 	lda         __i1+1
/* @119 */ 	adc         __i0+1
/* @120 */ 	sta         __i2+1
/* @125 */ 	lda         (__i2)
/* @126 */ 	sta         __i0
/* @127 */ 	ldy          #1
/* @128 */ 	lda         (__i2), Y
/* @129 */ 	sta         __i0+1
/* @134 */ 	jsr         .exit_label_132
/* @135 */ 	bra         .exit_label_133
.exit_label_132:
/* @136 */ 	jmp         (__i0)
.exit_label_133:
/* @137 */ 	lda          #__b2
/* @139 */ 	jsr         __rdec1
/* @140 */ 	bra         .exit_label_73
.exit_label_141:
/* @142 */ 	jmp         .exit_label_21
.exit_label_143:
/* @144 */ 	jsr         __pushi4
/* @145 */ 	jsr         _Exit
/* @147 */ 	jsr         __incsp2
/* @148 */ 	ldy          #74
	jmp          __leave_void
.func_end_exit:
	.size exit, .func_end_exit-exit

	.data
	.type   atexit_funcs,@object
	.local  atexit_funcs
	.comm   atexit_funcs,64,1

	.type   numfuncs,@object
	.local  numfuncs
	.comm   numfuncs,1,1

	.section ".rodata", "aMS", @progbits
