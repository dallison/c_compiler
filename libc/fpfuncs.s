	.file   "/Users/dallison/Google Drive/c_compiler/libc/fpfuncs.c"
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


	.global __IncrementHalf
	.type __IncrementHalf, @function

__IncrementHalf:
/* @2 */ 	ldx          #7
	jsr          __enter_nomask
/* @7 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @8 */ 	jsr         __pushi0
/* @9 */ 	jsr         __inc128
/* @11 */ 	jsr         __incsp2
/* @12 */ 	ldy          #10
	jmp          __leave_void_nomask
.func_end___IncrementHalf:
	.size __IncrementHalf, .func_end___IncrementHalf-__IncrementHalf

	.global __Increment
	.type __Increment, @function

__Increment:
/* @2 */ 	ldx          #7
	jsr          __enter_nomask
/* @7 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @8 */ 	jsr         __pushi0
/* @9 */ 	jsr         __inc256
/* @11 */ 	jsr         __incsp2
/* @12 */ 	ldy          #10
	jmp          __leave_void_nomask
.func_end___Increment:
	.size __Increment, .func_end___Increment-__Increment

	.global __Add
	.type __Add, @function

__Add:
/* @3 */ 	ldx          #7
	jsr          __enter_nomask
/* @8 */ 	ldx          #2
	jsr          __arg_value2_i0			// b
/* @12 */ 	ldx          #0
	jsr          __arg_value2_i1			// a
/* @13 */ 	jsr         __pushi0
/* @14 */ 	jsr         __pushi1
/* @15 */ 	jsr         __add256
/* @17 */ 	jsr         __incsp4
/* @18 */ 	ldy          #10
	jmp          __leave_void_nomask
.func_end___Add:
	.size __Add, .func_end___Add-__Add

	.global __AddHalf
	.type __AddHalf, @function

__AddHalf:
/* @3 */ 	ldx          #7
	jsr          __enter_nomask
/* @8 */ 	ldx          #2
	jsr          __arg_value2_i0			// b
/* @12 */ 	ldx          #0
	jsr          __arg_value2_i1			// a
/* @13 */ 	jsr         __pushi0
/* @14 */ 	jsr         __pushi1
/* @15 */ 	jsr         __add128
/* @17 */ 	jsr         __incsp4
/* @18 */ 	ldy          #10
	jmp          __leave_void_nomask
.func_end___AddHalf:
	.size __AddHalf, .func_end___AddHalf-__AddHalf

	.global __LShiftHalf
	.type __LShiftHalf, @function

__LShiftHalf:
/* @2 */ 	ldx          #7
	jsr          __enter_nomask
/* @7 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @8 */ 	jsr         __pushi0
/* @9 */ 	jsr         __lshift128
/* @11 */ 	jsr         __incsp2
/* @12 */ 	ldy          #10
	jmp          __leave_void_nomask
.func_end___LShiftHalf:
	.size __LShiftHalf, .func_end___LShiftHalf-__LShiftHalf

	.global __LShift
	.type __LShift, @function

__LShift:
/* @2 */ 	ldx          #7
	jsr          __enter_nomask
/* @7 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @8 */ 	jsr         __pushi0
/* @9 */ 	jsr         __lshift256
/* @11 */ 	jsr         __incsp2
/* @12 */ 	ldy          #10
	jmp          __leave_void_nomask
.func_end___LShift:
	.size __LShift, .func_end___LShift-__LShift

	.global __RShift
	.type __RShift, @function

__RShift:
/* @2 */ 	ldx          #7
	jsr          __enter_nomask
/* @7 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @8 */ 	jsr         __pushi0
/* @9 */ 	jsr         __rshift256
/* @11 */ 	jsr         __incsp2
/* @12 */ 	ldy          #10
	jmp          __leave_void_nomask
.func_end___RShift:
	.size __RShift, .func_end___RShift-__RShift

	.global __MultiplyBy10
	.type __MultiplyBy10, @function

__MultiplyBy10:
/* @9 */ 	ldx          #39
	jsr          __enter
	.byte        0x21,0x00,0x00		// Save mask i:1 b:1 l:0 x:0 f:0 
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i4			// a
/* @22 */ 	ldx          #35
	jsr          __var_addr_i0			// t
/* @26 */ 	lda          #32
/* @27 */ 	sta         __i1
/* @29 */ 	stz         __i1+1
/* @33 */ 	sta         __mem_size
/* @34 */ 	lda         __i1+1
/* @36 */ 	sta         __mem_size+1
/* @37 */ 	lda         __i4
/* @39 */ 	sta         __mem_src
/* @40 */ 	lda         __i4+1
/* @42 */ 	sta         __mem_src+1
/* @44 */ 	lda         __i0
/* @46 */ 	sta         __mem_dest
/* @47 */ 	lda         __i0+1
/* @49 */ 	sta         __mem_dest+1
/* @51 */ 	jsr         __builtin_memcpy
/* @52 */ 	stz         __b2
.__MultiplyBy10_label_53:
/* @55 */ 	lda         __b2
/* @56 */ 	sta         __i0
/* @58 */ 	stz         __i0+1
/* @61 */ 	lda         __i0+1
/* @62 */ 	cmp          #0
/* @63 */ 	bcc         .__MultiplyBy10_label_60
/* @64 */ 	bne         .__MultiplyBy10_label_82
/* @65 */ 	lda         __i0
/* @66 */ 	cmp          #3
/* @67 */ 	bcs         .__MultiplyBy10_label_82
.__MultiplyBy10_label_60:
/* @70 */ 	ldx          #35
	jsr          __var_addr_i0			// t
/* @73 */ 	jsr         __pushi0
/* @74 */ 	jsr         __LShift
/* @76 */ 	jsr         __incsp2
/* @78 */ 	lda          #__b2
/* @80 */ 	jsr         __rinc1
/* @81 */ 	bra         .__MultiplyBy10_label_53
.__MultiplyBy10_label_82:
/* @83 */ 	jsr         __pushi4
/* @84 */ 	jsr         __LShift
/* @85 */ 	jsr         __incsp2
/* @87 */ 	ldx          #35
	jsr          __var_addr_i0			// t
/* @90 */ 	jsr         __pushi0
/* @91 */ 	jsr         __pushi4
/* @92 */ 	jsr         __Add
/* @94 */ 	jsr         __incsp4
/* @95 */ 	ldy          #42
	jmp          __leave_void
.func_end___MultiplyBy10:
	.size __MultiplyBy10, .func_end___MultiplyBy10-__MultiplyBy10

	.global __MultiplyBy10Half
	.type __MultiplyBy10Half, @function

__MultiplyBy10Half:
/* @9 */ 	ldx          #23
	jsr          __enter
	.byte        0x21,0x00,0x00		// Save mask i:1 b:1 l:0 x:0 f:0 
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i4			// a
/* @22 */ 	ldx          #19
	jsr          __var_addr_i0			// t
/* @26 */ 	lda          #16
/* @27 */ 	sta         __i1
/* @29 */ 	stz         __i1+1
/* @33 */ 	sta         __mem_size
/* @34 */ 	lda         __i1+1
/* @36 */ 	sta         __mem_size+1
/* @37 */ 	lda         __i4
/* @39 */ 	sta         __mem_src
/* @40 */ 	lda         __i4+1
/* @42 */ 	sta         __mem_src+1
/* @44 */ 	lda         __i0
/* @46 */ 	sta         __mem_dest
/* @47 */ 	lda         __i0+1
/* @49 */ 	sta         __mem_dest+1
/* @51 */ 	jsr         __builtin_memcpy
/* @52 */ 	stz         __b2
.__MultiplyBy10Half_label_53:
/* @55 */ 	lda         __b2
/* @56 */ 	sta         __i0
/* @58 */ 	stz         __i0+1
/* @61 */ 	lda         __i0+1
/* @62 */ 	cmp          #0
/* @63 */ 	bcc         .__MultiplyBy10Half_label_60
/* @64 */ 	bne         .__MultiplyBy10Half_label_82
/* @65 */ 	lda         __i0
/* @66 */ 	cmp          #3
/* @67 */ 	bcs         .__MultiplyBy10Half_label_82
.__MultiplyBy10Half_label_60:
/* @70 */ 	ldx          #19
	jsr          __var_addr_i0			// t
/* @73 */ 	jsr         __pushi0
/* @74 */ 	jsr         __LShiftHalf
/* @76 */ 	jsr         __incsp2
/* @78 */ 	lda          #__b2
/* @80 */ 	jsr         __rinc1
/* @81 */ 	bra         .__MultiplyBy10Half_label_53
.__MultiplyBy10Half_label_82:
/* @83 */ 	jsr         __pushi4
/* @84 */ 	jsr         __LShiftHalf
/* @85 */ 	jsr         __incsp2
/* @87 */ 	ldx          #19
	jsr          __var_addr_i0			// t
/* @90 */ 	jsr         __pushi0
/* @91 */ 	jsr         __pushi4
/* @92 */ 	jsr         __AddHalf
/* @94 */ 	jsr         __incsp4
/* @95 */ 	ldy          #26
	jmp          __leave_void
.func_end___MultiplyBy10Half:
	.size __MultiplyBy10Half, .func_end___MultiplyBy10Half-__MultiplyBy10Half

	.global __DivMod64By10
	.type __DivMod64By10, @function

__DivMod64By10:
/* @4 */ 	stx         __result
/* @6 */ 	sty         __result+1
/* @7 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x00,0x40,0x00		// Save mask i:0 b:0 l:0 x:2 f:0 
/* @14 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @18 */ 	ldy          #7
/* @19 */ 	ldx          #7
.__DivMod64By10_label_20:
/* @22 */ 	lda         (__i0), Y
/* @23 */ 	sta         __x0, X
/* @24 */ 	dey         
/* @25 */ 	dex         
/* @26 */ 	bpl         .__DivMod64By10_label_20
/* @31 */ 	ldx          #7
.__DivMod64By10_label_32:
/* @33 */ 	lda         .lit.22, X
/* @34 */ 	sta         __x1, X
/* @35 */ 	dex         
/* @36 */ 	bpl         .__DivMod64By10_label_32
/* @41 */ 	lda          #__x2
/* @42 */ 	ldx          #__x0
/* @43 */ 	ldy          #__x1
/* @45 */ 	jsr         __umod8
/* @49 */ 	lda         __x2
/* @50 */ 	sta         __b0
/* @53 */ 	lda         __i0
/* @54 */ 	sta         __i1
/* @56 */ 	lda         __i0+1
/* @57 */ 	sta         __i1+1
/* @62 */ 	ldy          #7
/* @63 */ 	ldx          #7
.__DivMod64By10_label_64:
/* @65 */ 	lda         (__i1), Y
/* @66 */ 	sta         __x0, X
/* @67 */ 	dey         
/* @68 */ 	dex         
/* @69 */ 	bpl         .__DivMod64By10_label_64
/* @75 */ 	ldx          #7
.__DivMod64By10_label_76:
/* @77 */ 	lda         .lit.22, X
/* @78 */ 	sta         __x1, X
/* @79 */ 	dex         
/* @80 */ 	bpl         .__DivMod64By10_label_76
/* @85 */ 	lda          #__x2
/* @86 */ 	ldx          #__x0
/* @87 */ 	ldy          #__x1
/* @89 */ 	jsr         __udiv8
/* @94 */ 	ldy          #7
/* @95 */ 	ldx          #7
.__DivMod64By10_label_96:
/* @97 */ 	lda         __x2, X
/* @98 */ 	sta         (__i1), Y
/* @99 */ 	dey         
/* @100 */ 	dex         
/* @101 */ 	bpl         .__DivMod64By10_label_96
/* @102 */ 	lda         #__b0
/* @104 */ 	jsr         __result1
/* @106 */ 	ldy          #8
	jmp          __leave_leaf
.func_end___DivMod64By10:
	.size __DivMod64By10, .func_end___DivMod64By10-__DivMod64By10

	.global __IsZeroUpper
	.type __IsZeroUpper, @function

__IsZeroUpper:
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
/* @33 */ 	lda         __i2
/* @34 */ 	sta         __i0
/* @35 */ 	lda         __i2+1
/* @36 */ 	sta         __i0+1
/* @37 */ 	stz         __b0
.__IsZeroUpper_label_38:
/* @40 */ 	lda         __b0
/* @41 */ 	sta         __i1
/* @43 */ 	stz         __i1+1
/* @46 */ 	lda         __i1+1
/* @47 */ 	cmp          #0
/* @48 */ 	bcc         .__IsZeroUpper_label_45
/* @49 */ 	bne         .__IsZeroUpper_label_110
/* @50 */ 	lda         __i1
/* @51 */ 	cmp          #2
/* @52 */ 	bcs         .__IsZeroUpper_label_110
.__IsZeroUpper_label_45:
/* @56 */ 	lda         __i0
/* @57 */ 	sta         __i1
/* @58 */ 	lda         __i0+1
/* @59 */ 	sta         __i1+1
/* @60 */ 	lda          #__i0
/* @62 */ 	ldx          #8
/* @64 */ 	jsr         __rinc2
/* @70 */ 	ldy          #7
/* @71 */ 	ldx          #7
.__IsZeroUpper_label_72:
/* @74 */ 	lda         (__i1), Y
/* @75 */ 	sta         __x0, X
/* @76 */ 	dey         
/* @77 */ 	dex         
/* @78 */ 	bpl         .__IsZeroUpper_label_72
/* @80 */ 	lda         __x0
/* @81 */ 	ora         __x0+1
/* @83 */ 	ora         __x0+2
/* @85 */ 	ora         __x0+3
/* @87 */ 	ora         __x0+4
/* @89 */ 	ora         __x0+5
/* @91 */ 	ora         __x0+6
/* @92 */ 	ora         __x0+7
/* @94 */ 	beq         .__IsZeroUpper_label_104
/* @97 */ 	stz         __b1
/* @98 */ 	lda         #__b1
/* @100 */ 	jsr         __result1
.__IsZeroUpper_label_101:
/* @102 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.__IsZeroUpper_label_104:
/* @106 */ 	lda          #__b0
/* @108 */ 	jsr         __rinc1
/* @109 */ 	bra         .__IsZeroUpper_label_38
.__IsZeroUpper_label_110:
/* @113 */ 	lda          #1
/* @114 */ 	sta         __b1
/* @115 */ 	lda         #__b1
/* @116 */ 	jsr         __result1
/* @117 */ 	bra         .__IsZeroUpper_label_101
.func_end___IsZeroUpper:
	.size __IsZeroUpper, .func_end___IsZeroUpper-__IsZeroUpper

	.global __IsOneHalf
	.type __IsOneHalf, @function

__IsOneHalf:
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
/* @37 */ 	ldx          #7
.__IsOneHalf_label_38:
/* @40 */ 	lda         (__i1), Y
/* @41 */ 	sta         __x0, X
/* @42 */ 	dey         
/* @43 */ 	dex         
/* @44 */ 	bpl         .__IsOneHalf_label_38
/* @47 */ 	lda         __x0
/* @48 */ 	cmp          #1
/* @49 */ 	bne         .__IsOneHalf_label_46
/* @50 */ 	lda         __x0+1
/* @52 */ 	bne         .__IsOneHalf_label_46
/* @54 */ 	lda         __x0+2
/* @56 */ 	bne         .__IsOneHalf_label_46
/* @58 */ 	lda         __x0+3
/* @60 */ 	bne         .__IsOneHalf_label_46
/* @62 */ 	lda         __x0+4
/* @63 */ 	cmp          #0
/* @64 */ 	bne         .__IsOneHalf_label_46
/* @66 */ 	lda         __x0+5
/* @68 */ 	bne         .__IsOneHalf_label_46
/* @70 */ 	lda         __x0+6
/* @72 */ 	bne         .__IsOneHalf_label_46
/* @73 */ 	lda         __x0+7
/* @75 */ 	beq         .__IsOneHalf_label_86
.__IsOneHalf_label_46:
/* @79 */ 	stz         __b1
/* @80 */ 	lda         #__b1
/* @82 */ 	jsr         __result1
.__IsOneHalf_label_83:
/* @84 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.__IsOneHalf_label_86:
/* @87 */ 	lda          #1
/* @88 */ 	sta         __b0
.__IsOneHalf_label_89:
/* @91 */ 	lda         __b0
/* @92 */ 	sta         __i1
/* @94 */ 	stz         __i1+1
/* @97 */ 	lda         __i1+1
/* @98 */ 	cmp          #0
/* @99 */ 	bcc         .__IsOneHalf_label_96
/* @100 */ 	bne         .__IsOneHalf_label_149
/* @101 */ 	lda         __i1
/* @102 */ 	cmp          #2
/* @103 */ 	bcs         .__IsOneHalf_label_149
.__IsOneHalf_label_96:
/* @107 */ 	lda         __i0
/* @108 */ 	sta         __i1
/* @109 */ 	lda         __i0+1
/* @110 */ 	sta         __i1+1
/* @111 */ 	lda          #__i0
/* @112 */ 	ldx          #8
/* @113 */ 	jsr         __rinc2
/* @118 */ 	ldy          #7
/* @119 */ 	ldx          #7
.__IsOneHalf_label_120:
/* @121 */ 	lda         (__i1), Y
/* @122 */ 	sta         __x0, X
/* @123 */ 	dey         
/* @124 */ 	dex         
/* @125 */ 	bpl         .__IsOneHalf_label_120
/* @127 */ 	lda         __x0
/* @128 */ 	ora         __x0+1
/* @129 */ 	ora         __x0+2
/* @130 */ 	ora         __x0+3
/* @131 */ 	ora         __x0+4
/* @132 */ 	ora         __x0+5
/* @133 */ 	ora         __x0+6
/* @134 */ 	ora         __x0+7
/* @136 */ 	beq         .__IsOneHalf_label_143
/* @139 */ 	stz         __b1
/* @140 */ 	lda         #__b1
/* @141 */ 	jsr         __result1
/* @142 */ 	bra         .__IsOneHalf_label_83
.__IsOneHalf_label_143:
/* @145 */ 	lda          #__b0
/* @147 */ 	jsr         __rinc1
/* @148 */ 	bra         .__IsOneHalf_label_89
.__IsOneHalf_label_149:
/* @152 */ 	lda          #1
/* @153 */ 	sta         __b1
/* @154 */ 	lda         #__b1
/* @155 */ 	jsr         __result1
/* @156 */ 	bra         .__IsOneHalf_label_83
.func_end___IsOneHalf:
	.size __IsOneHalf, .func_end___IsOneHalf-__IsOneHalf

	.global __DivModBy10Half
	.type __DivModBy10Half, @function

__DivModBy10Half:
/* @14 */ 	stx         __result
/* @16 */ 	sty         __result+1
/* @17 */ 	ldx          #23
	jsr          __enter
	.byte        0x44,0x20,0x00		// Save mask i:4 b:2 l:0 x:1 f:0 
/* @26 */ 	ldx          #0
	jsr          __arg_value2_i4			// a
/* @35 */ 	jsr         __pushi4
/* @37 */ 	ldx         #__b3
/* @38 */ 	ldy          #0
/* @39 */ 	jsr         __IsZeroUpper
/* @41 */ 	jsr         __incsp2
/* @43 */ 	lda         __b3
/* @45 */ 	beq         .__DivModBy10Half_label_60
/* @46 */ 	jsr         __pushi4
/* @48 */ 	ldx         #__b3
/* @49 */ 	ldy          #0
/* @50 */ 	jsr         __DivMod64By10
/* @51 */ 	jsr         __incsp2
/* @53 */ 	ldx          #24
	jsr          __load_result
/* @54 */ 	lda         #__b3
/* @56 */ 	jsr         __result1
.__DivModBy10Half_label_57:
/* @58 */ 	ldy          #26
	jmp          __leave
.__DivModBy10Half_label_60:
/* @62 */ 	lda          #16
/* @64 */ 	sta         __mem_size
/* @66 */ 	ldx          #19
	jsr          __var_addr_i0			// quotient
/* @68 */ 	lda         __i0
/* @70 */ 	sta         __mem_dest
/* @71 */ 	lda         __i0+1
/* @73 */ 	sta         __mem_dest+1
/* @75 */ 	jsr         __zeromem1
/* @76 */ 	stz         __b2
/* @77 */ 	lda          #1
/* @78 */ 	sta         __i5
/* @79 */ 	dec          A
/* @80 */ 	stz         __i5+1
/* @81 */ 	stz         __i6
/* @82 */ 	stz         __i6+1
.__DivModBy10Half_label_83:
/* @85 */ 	lda         __i6+1
/* @86 */ 	cmp          #0
/* @87 */ 	bcc         .__DivModBy10Half_label_84
/* @88 */ 	beq         .__DivModBy10Half_label_242
/* @243 */ 	jmp         .__DivModBy10Half_label_209
.__DivModBy10Half_label_242:
/* @89 */ 	lda         __i6
/* @90 */ 	cmp          #128
/* @91 */ 	bcc         .__DivModBy10Half_label_244
/* @245 */ 	jmp         .__DivModBy10Half_label_209
.__DivModBy10Half_label_244:
.__DivModBy10Half_label_84:
/* @95 */ 	lda         __b2
/* @96 */ 	asl          A
/* @101 */ 	sta         __b2
/* @105 */ 	ldy          #15
/* @107 */ 	ldx          #7
.__DivModBy10Half_label_108:
/* @110 */ 	lda         (__i4), Y
/* @111 */ 	sta         __x0, X
/* @112 */ 	dey         
/* @113 */ 	dex         
/* @114 */ 	bpl         .__DivModBy10Half_label_108
/* @119 */ 	stz         __x1
/* @121 */ 	stz         __x1+1
/* @124 */ 	stz         __x1+2
/* @127 */ 	stz         __x1+3
/* @130 */ 	stz         __x1+4
/* @133 */ 	stz         __x1+5
/* @136 */ 	stz         __x1+6
/* @137 */ 	lda         __x0+7
/* @138 */ 	and          #128
/* @139 */ 	sta         __x1+7
/* @141 */ 	lda         __x1
/* @142 */ 	ora         __x1+1
/* @143 */ 	ora         __x1+2
/* @144 */ 	ora         __x1+3
/* @145 */ 	ora         __x1+4
/* @146 */ 	ora         __x1+5
/* @147 */ 	ora         __x1+6
/* @148 */ 	ora         __x1+7
/* @150 */ 	beq         .__DivModBy10Half_label_160
/* @153 */ 	lda         __b2
/* @154 */ 	ora          #1
/* @159 */ 	sta         __b2
.__DivModBy10Half_label_160:
/* @161 */ 	jsr         __pushi4
/* @162 */ 	jsr         __LShiftHalf
/* @163 */ 	jsr         __incsp2
/* @165 */ 	ldx          #19
	jsr          __var_addr_i7			// quotient
/* @168 */ 	jsr         __pushi7
/* @169 */ 	jsr         __LShiftHalf
/* @170 */ 	jsr         __incsp2
/* @172 */ 	lda         __b2
/* @173 */ 	sta         __i0
/* @175 */ 	stz         __i0+1
/* @178 */ 	lda         __i0+1
/* @179 */ 	cmp          #0
/* @180 */ 	bcc         .__DivModBy10Half_label_203
/* @181 */ 	bne         .__DivModBy10Half_label_177
/* @182 */ 	lda         __i0
/* @183 */ 	cmp          #10
/* @184 */ 	bcc         .__DivModBy10Half_label_203
.__DivModBy10Half_label_177:
/* @190 */ 	jsr         __pushi7
/* @191 */ 	jsr         __IncrementHalf
/* @192 */ 	jsr         __incsp2
/* @195 */ 	sec         
/* @196 */ 	lda         __b2
/* @197 */ 	sbc          #10
/* @202 */ 	sta         __b2
.__DivModBy10Half_label_203:
/* @205 */ 	lda          #__i6
/* @207 */ 	jsr         __rinc21
/* @208 */ 	jmp         .__DivModBy10Half_label_83
.__DivModBy10Half_label_209:
/* @211 */ 	ldx          #19
	jsr          __var_addr_i0			// quotient
/* @215 */ 	lda          #16
/* @216 */ 	sta         __i1
/* @218 */ 	stz         __i1+1
/* @221 */ 	sta         __mem_size
/* @222 */ 	lda         __i1+1
/* @224 */ 	sta         __mem_size+1
/* @226 */ 	lda         __i0
/* @228 */ 	sta         __mem_src
/* @229 */ 	lda         __i0+1
/* @231 */ 	sta         __mem_src+1
/* @232 */ 	lda         __i4
/* @233 */ 	sta         __mem_dest
/* @234 */ 	lda         __i4+1
/* @235 */ 	sta         __mem_dest+1
/* @237 */ 	jsr         __builtin_memcpy
/* @238 */ 	ldx          #24
	jsr          __load_result
/* @239 */ 	lda         #__b2
/* @240 */ 	jsr         __result1
/* @241 */ 	jmp         .__DivModBy10Half_label_57
.func_end___DivModBy10Half:
	.size __DivModBy10Half, .func_end___DivModBy10Half-__DivModBy10Half

	.global __DivideBy10
	.type __DivideBy10, @function

__DivideBy10:
/* @16 */ 	stx         __result
/* @18 */ 	sty         __result+1
/* @19 */ 	ldx          #39
	jsr          __enter
	.byte        0x44,0x20,0x00		// Save mask i:4 b:2 l:0 x:1 f:0 
/* @28 */ 	ldx          #0
	jsr          __arg_value2_i4			// a
/* @37 */ 	jsr         __pushi4
/* @39 */ 	ldx         #__b3
/* @40 */ 	ldy          #0
/* @41 */ 	jsr         __IsZeroUpper
/* @43 */ 	jsr         __incsp2
/* @45 */ 	lda         __b3
/* @47 */ 	beq         .__DivideBy10_label_62
/* @48 */ 	jsr         __pushi4
/* @50 */ 	ldx         #__b3
/* @51 */ 	ldy          #0
/* @52 */ 	jsr         __DivModBy10Half
/* @53 */ 	jsr         __incsp2
/* @55 */ 	ldx          #40
	jsr          __load_result
/* @56 */ 	lda         #__b3
/* @58 */ 	jsr         __result1
.__DivideBy10_label_59:
/* @60 */ 	ldy          #42
	jmp          __leave
.__DivideBy10_label_62:
/* @64 */ 	lda          #32
/* @66 */ 	sta         __mem_size
/* @68 */ 	ldx          #35
	jsr          __var_addr_i0			// quotient
/* @70 */ 	lda         __i0
/* @72 */ 	sta         __mem_dest
/* @73 */ 	lda         __i0+1
/* @75 */ 	sta         __mem_dest+1
/* @77 */ 	jsr         __zeromem1
/* @78 */ 	stz         __b2
/* @79 */ 	lda          #3
/* @80 */ 	sta         __i5
/* @82 */ 	stz         __i5+1
/* @83 */ 	stz         __i6
/* @84 */ 	stz         __i6+1
.__DivideBy10_label_85:
/* @87 */ 	lda         __i6+1
/* @88 */ 	cmp          #1
/* @89 */ 	bcc         .__DivideBy10_label_86
/* @90 */ 	beq         .__DivideBy10_label_244
/* @245 */ 	jmp         .__DivideBy10_label_211
.__DivideBy10_label_244:
/* @91 */ 	lda         __i6
/* @92 */ 	cmp          #0
/* @93 */ 	bcc         .__DivideBy10_label_246
/* @247 */ 	jmp         .__DivideBy10_label_211
.__DivideBy10_label_246:
.__DivideBy10_label_86:
/* @97 */ 	lda         __b2
/* @98 */ 	asl          A
/* @103 */ 	sta         __b2
/* @107 */ 	ldy          #31
/* @109 */ 	ldx          #7
.__DivideBy10_label_110:
/* @112 */ 	lda         (__i4), Y
/* @113 */ 	sta         __x0, X
/* @114 */ 	dey         
/* @115 */ 	dex         
/* @116 */ 	bpl         .__DivideBy10_label_110
/* @121 */ 	stz         __x1
/* @123 */ 	stz         __x1+1
/* @126 */ 	stz         __x1+2
/* @129 */ 	stz         __x1+3
/* @132 */ 	stz         __x1+4
/* @135 */ 	stz         __x1+5
/* @138 */ 	stz         __x1+6
/* @139 */ 	lda         __x0+7
/* @140 */ 	and          #128
/* @141 */ 	sta         __x1+7
/* @143 */ 	lda         __x1
/* @144 */ 	ora         __x1+1
/* @145 */ 	ora         __x1+2
/* @146 */ 	ora         __x1+3
/* @147 */ 	ora         __x1+4
/* @148 */ 	ora         __x1+5
/* @149 */ 	ora         __x1+6
/* @150 */ 	ora         __x1+7
/* @152 */ 	beq         .__DivideBy10_label_162
/* @155 */ 	lda         __b2
/* @156 */ 	ora          #1
/* @161 */ 	sta         __b2
.__DivideBy10_label_162:
/* @163 */ 	jsr         __pushi4
/* @164 */ 	jsr         __LShift
/* @165 */ 	jsr         __incsp2
/* @167 */ 	ldx          #35
	jsr          __var_addr_i7			// quotient
/* @170 */ 	jsr         __pushi7
/* @171 */ 	jsr         __LShift
/* @172 */ 	jsr         __incsp2
/* @174 */ 	lda         __b2
/* @175 */ 	sta         __i0
/* @177 */ 	stz         __i0+1
/* @180 */ 	lda         __i0+1
/* @181 */ 	cmp          #0
/* @182 */ 	bcc         .__DivideBy10_label_205
/* @183 */ 	bne         .__DivideBy10_label_179
/* @184 */ 	lda         __i0
/* @185 */ 	cmp          #10
/* @186 */ 	bcc         .__DivideBy10_label_205
.__DivideBy10_label_179:
/* @192 */ 	jsr         __pushi7
/* @193 */ 	jsr         __Increment
/* @194 */ 	jsr         __incsp2
/* @197 */ 	sec         
/* @198 */ 	lda         __b2
/* @199 */ 	sbc          #10
/* @204 */ 	sta         __b2
.__DivideBy10_label_205:
/* @207 */ 	lda          #__i6
/* @209 */ 	jsr         __rinc21
/* @210 */ 	jmp         .__DivideBy10_label_85
.__DivideBy10_label_211:
/* @213 */ 	ldx          #35
	jsr          __var_addr_i0			// quotient
/* @217 */ 	lda          #32
/* @218 */ 	sta         __i1
/* @220 */ 	stz         __i1+1
/* @223 */ 	sta         __mem_size
/* @224 */ 	lda         __i1+1
/* @226 */ 	sta         __mem_size+1
/* @228 */ 	lda         __i0
/* @230 */ 	sta         __mem_src
/* @231 */ 	lda         __i0+1
/* @233 */ 	sta         __mem_src+1
/* @234 */ 	lda         __i4
/* @235 */ 	sta         __mem_dest
/* @236 */ 	lda         __i4+1
/* @237 */ 	sta         __mem_dest+1
/* @239 */ 	jsr         __builtin_memcpy
/* @240 */ 	ldx          #40
	jsr          __load_result
/* @241 */ 	lda         #__b2
/* @242 */ 	jsr         __result1
/* @243 */ 	jmp         .__DivideBy10_label_59
.func_end___DivideBy10:
	.size __DivideBy10, .func_end___DivideBy10-__DivideBy10

	.global __Round
	.type __Round, @function

__Round:
/* @8 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x20,0x00		// Save mask i:1 b:0 l:0 x:1 f:0 
/* @13 */ 	ldx          #0
	jsr          __arg_value2_i4			// v
/* @17 */ 	ldy          #15
/* @19 */ 	ldx          #7
.__Round_label_20:
/* @22 */ 	lda         (__i4), Y
/* @23 */ 	sta         __x0, X
/* @24 */ 	dey         
/* @25 */ 	dex         
/* @26 */ 	bpl         .__Round_label_20
/* @32 */ 	stz         __x1
/* @35 */ 	stz         __x1+1
/* @38 */ 	stz         __x1+2
/* @41 */ 	stz         __x1+3
/* @44 */ 	stz         __x1+4
/* @47 */ 	stz         __x1+5
/* @50 */ 	stz         __x1+6
/* @51 */ 	lda         __x0+7
/* @52 */ 	and          #128
/* @53 */ 	sta         __x1+7
/* @55 */ 	lda         __x1
/* @56 */ 	ora         __x1+1
/* @57 */ 	ora         __x1+2
/* @58 */ 	ora         __x1+3
/* @59 */ 	ora         __x1+4
/* @60 */ 	ora         __x1+5
/* @61 */ 	ora         __x1+6
/* @62 */ 	ora         __x1+7
/* @64 */ 	beq         .__Round_label_80
/* @67 */ 	clc         
/* @68 */ 	lda         __i4
/* @69 */ 	adc          #16
/* @70 */ 	sta         __i0
/* @71 */ 	lda         __i4+1
/* @72 */ 	adc          #0
/* @73 */ 	sta         __i0+1
/* @76 */ 	jsr         __pushi0
/* @77 */ 	jsr         __IncrementHalf
/* @79 */ 	jsr         __incsp2
.__Round_label_80:
/* @81 */ 	ldy          #10
	jmp          __leave_void
.func_end___Round:
	.size __Round, .func_end___Round-__Round

	.global __IsZeroHalf
	.type __IsZeroHalf, @function

__IsZeroHalf:
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
/* @25 */ 	stz         __b0
.__IsZeroHalf_label_26:
/* @28 */ 	lda         __b0
/* @29 */ 	sta         __i1
/* @31 */ 	stz         __i1+1
/* @34 */ 	lda         __i1+1
/* @35 */ 	cmp          #0
/* @36 */ 	bcc         .__IsZeroHalf_label_33
/* @37 */ 	bne         .__IsZeroHalf_label_98
/* @38 */ 	lda         __i1
/* @39 */ 	cmp          #2
/* @40 */ 	bcs         .__IsZeroHalf_label_98
.__IsZeroHalf_label_33:
/* @44 */ 	lda         __i0
/* @45 */ 	sta         __i1
/* @46 */ 	lda         __i0+1
/* @47 */ 	sta         __i1+1
/* @48 */ 	lda          #__i0
/* @50 */ 	ldx          #8
/* @52 */ 	jsr         __rinc2
/* @58 */ 	ldy          #7
/* @59 */ 	ldx          #7
.__IsZeroHalf_label_60:
/* @62 */ 	lda         (__i1), Y
/* @63 */ 	sta         __x0, X
/* @64 */ 	dey         
/* @65 */ 	dex         
/* @66 */ 	bpl         .__IsZeroHalf_label_60
/* @68 */ 	lda         __x0
/* @69 */ 	ora         __x0+1
/* @71 */ 	ora         __x0+2
/* @73 */ 	ora         __x0+3
/* @75 */ 	ora         __x0+4
/* @77 */ 	ora         __x0+5
/* @79 */ 	ora         __x0+6
/* @80 */ 	ora         __x0+7
/* @82 */ 	beq         .__IsZeroHalf_label_92
/* @85 */ 	stz         __b1
/* @86 */ 	lda         #__b1
/* @88 */ 	jsr         __result1
.__IsZeroHalf_label_89:
/* @90 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.__IsZeroHalf_label_92:
/* @94 */ 	lda          #__b0
/* @96 */ 	jsr         __rinc1
/* @97 */ 	bra         .__IsZeroHalf_label_26
.__IsZeroHalf_label_98:
/* @101 */ 	lda          #1
/* @102 */ 	sta         __b1
/* @103 */ 	lda         #__b1
/* @104 */ 	jsr         __result1
/* @105 */ 	bra         .__IsZeroHalf_label_89
.func_end___IsZeroHalf:
	.size __IsZeroHalf, .func_end___IsZeroHalf-__IsZeroHalf

	.global __IsZero
	.type __IsZero, @function

__IsZero:
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
/* @25 */ 	stz         __b0
.__IsZero_label_26:
/* @28 */ 	lda         __b0
/* @29 */ 	sta         __i1
/* @31 */ 	stz         __i1+1
/* @34 */ 	lda         __i1+1
/* @35 */ 	cmp          #0
/* @36 */ 	bcc         .__IsZero_label_33
/* @37 */ 	bne         .__IsZero_label_98
/* @38 */ 	lda         __i1
/* @39 */ 	cmp          #4
/* @40 */ 	bcs         .__IsZero_label_98
.__IsZero_label_33:
/* @44 */ 	lda         __i0
/* @45 */ 	sta         __i1
/* @46 */ 	lda         __i0+1
/* @47 */ 	sta         __i1+1
/* @48 */ 	lda          #__i0
/* @50 */ 	ldx          #8
/* @52 */ 	jsr         __rinc2
/* @58 */ 	ldy          #7
/* @59 */ 	ldx          #7
.__IsZero_label_60:
/* @62 */ 	lda         (__i1), Y
/* @63 */ 	sta         __x0, X
/* @64 */ 	dey         
/* @65 */ 	dex         
/* @66 */ 	bpl         .__IsZero_label_60
/* @68 */ 	lda         __x0
/* @69 */ 	ora         __x0+1
/* @71 */ 	ora         __x0+2
/* @73 */ 	ora         __x0+3
/* @75 */ 	ora         __x0+4
/* @77 */ 	ora         __x0+5
/* @79 */ 	ora         __x0+6
/* @80 */ 	ora         __x0+7
/* @82 */ 	beq         .__IsZero_label_92
/* @85 */ 	stz         __b1
/* @86 */ 	lda         #__b1
/* @88 */ 	jsr         __result1
.__IsZero_label_89:
/* @90 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.__IsZero_label_92:
/* @94 */ 	lda          #__b0
/* @96 */ 	jsr         __rinc1
/* @97 */ 	bra         .__IsZero_label_26
.__IsZero_label_98:
/* @101 */ 	lda          #1
/* @102 */ 	sta         __b1
/* @103 */ 	lda         #__b1
/* @104 */ 	jsr         __result1
/* @105 */ 	bra         .__IsZero_label_89
.func_end___IsZero:
	.size __IsZero, .func_end___IsZero-__IsZero

	.global __IsLessThan10
	.type __IsLessThan10, @function

__IsLessThan10:
/* @11 */ 	stx         __result
/* @13 */ 	sty         __result+1
/* @14 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i0			// v
/* @25 */ 	ldy          #7
/* @26 */ 	ldx          #7
.__IsLessThan10_label_27:
/* @29 */ 	lda         (__i0), Y
/* @30 */ 	sta         __x0, X
/* @31 */ 	dey         
/* @32 */ 	dex         
/* @33 */ 	bpl         .__IsLessThan10_label_27
/* @36 */ 	lda         __x0+7
/* @37 */ 	cmp          #0
/* @38 */ 	bcc         .__IsLessThan10_label_82
/* @39 */ 	bne         .__IsLessThan10_label_35
/* @41 */ 	lda         __x0+6
/* @42 */ 	cmp          #0
/* @43 */ 	bcc         .__IsLessThan10_label_82
/* @44 */ 	bne         .__IsLessThan10_label_35
/* @46 */ 	lda         __x0+5
/* @47 */ 	cmp          #0
/* @48 */ 	bcc         .__IsLessThan10_label_82
/* @49 */ 	bne         .__IsLessThan10_label_35
/* @51 */ 	lda         __x0+4
/* @52 */ 	cmp          #0
/* @53 */ 	bcc         .__IsLessThan10_label_82
/* @54 */ 	bne         .__IsLessThan10_label_35
/* @56 */ 	lda         __x0+3
/* @57 */ 	cmp          #0
/* @58 */ 	bcc         .__IsLessThan10_label_82
/* @59 */ 	bne         .__IsLessThan10_label_35
/* @61 */ 	lda         __x0+2
/* @62 */ 	cmp          #0
/* @63 */ 	bcc         .__IsLessThan10_label_82
/* @64 */ 	bne         .__IsLessThan10_label_35
/* @65 */ 	lda         __x0+1
/* @66 */ 	cmp          #0
/* @67 */ 	bcc         .__IsLessThan10_label_82
/* @68 */ 	bne         .__IsLessThan10_label_35
/* @69 */ 	lda         __x0
/* @70 */ 	cmp          #10
/* @71 */ 	bcc         .__IsLessThan10_label_82
.__IsLessThan10_label_35:
/* @75 */ 	stz         __b0
/* @76 */ 	lda         #__b0
/* @78 */ 	jsr         __result1
.__IsLessThan10_label_79:
/* @80 */ 	ldy          #8
	jmp          __leave_leaf_nomask
.__IsLessThan10_label_82:
/* @83 */ 	lda          #1
/* @84 */ 	sta         __i1
/* @85 */ 	dec          A
/* @86 */ 	stz         __i1+1
.__IsLessThan10_label_87:
/* @89 */ 	lda         __i1
/* @90 */ 	cmp          #2
/* @91 */ 	lda         __i1+1
/* @92 */ 	sbc          #0
/* @93 */ 	bvc         .__IsLessThan10_label_88
/* @95 */ 	eor          #128
.__IsLessThan10_label_88:
/* @96 */ 	bpl         .__IsLessThan10_label_156
/* @99 */ 	lda         __i1
/* @100 */ 	asl          A
/* @101 */ 	sta         __i2
/* @102 */ 	lda         __i1+1
/* @103 */ 	rol          A
/* @104 */ 	sta         __i2+1
/* @105 */ 	ldx          #2
.__IsLessThan10_label_106:
/* @107 */ 	asl         __i2
/* @108 */ 	rol         __i2+1
/* @109 */ 	dex         
/* @110 */ 	bne         .__IsLessThan10_label_106
/* @114 */ 	clc         
/* @115 */ 	lda         __i0
/* @116 */ 	adc         __i2
/* @117 */ 	sta         __i3
/* @118 */ 	lda         __i0+1
/* @119 */ 	adc         __i2+1
/* @120 */ 	sta         __i3+1
/* @125 */ 	ldy          #7
/* @126 */ 	ldx          #7
.__IsLessThan10_label_127:
/* @128 */ 	lda         (__i3), Y
/* @129 */ 	sta         __x0, X
/* @130 */ 	dey         
/* @131 */ 	dex         
/* @132 */ 	bpl         .__IsLessThan10_label_127
/* @134 */ 	lda         __x0
/* @135 */ 	ora         __x0+1
/* @136 */ 	ora         __x0+2
/* @137 */ 	ora         __x0+3
/* @138 */ 	ora         __x0+4
/* @139 */ 	ora         __x0+5
/* @140 */ 	ora         __x0+6
/* @141 */ 	ora         __x0+7
/* @143 */ 	beq         .__IsLessThan10_label_150
/* @146 */ 	stz         __b0
/* @147 */ 	lda         #__b0
/* @148 */ 	jsr         __result1
/* @149 */ 	bra         .__IsLessThan10_label_79
.__IsLessThan10_label_150:
/* @152 */ 	lda          #__i1
/* @154 */ 	jsr         __rinc21
/* @155 */ 	bra         .__IsLessThan10_label_87
.__IsLessThan10_label_156:
/* @159 */ 	lda          #1
/* @160 */ 	sta         __b0
/* @161 */ 	lda         #__b0
/* @162 */ 	jsr         __result1
/* @163 */ 	bra         .__IsLessThan10_label_79
.func_end___IsLessThan10:
	.size __IsLessThan10, .func_end___IsLessThan10-__IsLessThan10

	.data
	.section ".rodata", "aMS", @progbits
.lit.22:
	.byte 0x0a
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.byte 0x00
	.type .lit.22, @object
	.size .lit.22, 8

