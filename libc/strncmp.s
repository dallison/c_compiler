	.file   "/Users/dallison/Google Drive/c_compiler/libc/strncmp.c"
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


	.global strncmp
	.type strncmp, @function

strncmp:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @14 */ 	ldx          #0
	jsr          __arg_value2_i0			// a
/* @18 */ 	ldx          #2
	jsr          __arg_value2_i1			// b
/* @22 */ 	ldx          #4
	jsr          __arg_value2_i2			// n
.strncmp_label_23:
/* @26 */ 	lda         (__i0)
/* @32 */ 	sta         __i3
/* @34 */ 	and          #128
/* @36 */ 	beq         .strncmp_label_35
/* @38 */ 	lda          #255
.strncmp_label_35:
/* @40 */ 	sta         __i3+1
/* @42 */ 	lda         __i3
/* @43 */ 	ora         __i3+1
/* @45 */ 	beq         .strncmp_label_99
/* @48 */ 	lda         (__i0)
/* @54 */ 	sta         __i3
/* @55 */ 	and          #128
/* @57 */ 	beq         .strncmp_label_56
/* @58 */ 	lda          #255
.strncmp_label_56:
/* @59 */ 	sta         __i3+1
/* @62 */ 	lda         (__i1)
/* @68 */ 	sta         __i4
/* @69 */ 	and          #128
/* @71 */ 	beq         .strncmp_label_70
/* @72 */ 	lda          #255
.strncmp_label_70:
/* @73 */ 	sta         __i4+1
/* @77 */ 	lda         __i3
/* @78 */ 	cmp         __i4
/* @79 */ 	bne         .strncmp_label_99
/* @80 */ 	lda         __i3+1
/* @81 */ 	cmp         __i4+1
/* @82 */ 	bne         .strncmp_label_99
/* @85 */ 	lda          #__i2
/* @87 */ 	jsr         __rdec21
/* @88 */ 	lda         __i2
/* @89 */ 	ora         __i2+1
/* @91 */ 	beq         .strncmp_label_99
/* @93 */ 	lda          #__i0
/* @95 */ 	jsr         __rinc21
/* @96 */ 	lda          #__i1
/* @97 */ 	jsr         __rinc21
/* @98 */ 	bra         .strncmp_label_23
.strncmp_label_99:
/* @102 */ 	lda         (__i0)
/* @108 */ 	sta         __i3
/* @109 */ 	and          #128
/* @111 */ 	beq         .strncmp_label_110
/* @112 */ 	lda          #255
.strncmp_label_110:
/* @113 */ 	sta         __i3+1
/* @116 */ 	lda         (__i1)
/* @122 */ 	sta         __i4
/* @123 */ 	and          #128
/* @125 */ 	beq         .strncmp_label_124
/* @126 */ 	lda          #255
.strncmp_label_124:
/* @127 */ 	sta         __i4+1
/* @132 */ 	sec         
/* @133 */ 	lda         __i3
/* @134 */ 	sbc         __i4
/* @135 */ 	sta         __i5
/* @136 */ 	lda         __i3+1
/* @137 */ 	sbc         __i4+1
/* @138 */ 	sta         __i5+1
/* @140 */ 	lda         #__i5
/* @142 */ 	jsr         __result2
/* @144 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_strncmp:
	.size strncmp, .func_end_strncmp-strncmp

	.data
	.section ".rodata", "aMS", @progbits
