	.file   "/Users/dallison/Google Drive/c_compiler/libc/memcmp.c"
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


	.global memcmp
	.type memcmp, @function

memcmp:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x02,0x00,0x00		// Save mask i:2 b:0 l:0 x:0 f:0 
/* @16 */ 	ldx          #0
	jsr          __arg_value2_i1			// s1
/* @22 */ 	ldx          #2
	jsr          __arg_value2_i3			// s2
/* @26 */ 	ldx          #4
	jsr          __arg_value2_i4			// n
/* @28 */ 	lda         __i1
/* @29 */ 	sta         __i0
/* @31 */ 	lda         __i1+1
/* @32 */ 	sta         __i0+1
/* @33 */ 	lda         __i3
/* @34 */ 	sta         __i2
/* @35 */ 	lda         __i3+1
/* @36 */ 	sta         __i2+1
.memcmp_label_37:
/* @40 */ 	lda         (__i0)
/* @46 */ 	sta         __i1
/* @48 */ 	stz         __i1+1
/* @51 */ 	lda         (__i2)
/* @57 */ 	sta         __i3
/* @59 */ 	stz         __i3+1
/* @63 */ 	lda         __i1
/* @64 */ 	cmp         __i3
/* @65 */ 	bne         .memcmp_label_84
/* @66 */ 	lda         __i1+1
/* @67 */ 	cmp         __i3+1
/* @68 */ 	bne         .memcmp_label_84
/* @70 */ 	lda          #__i4
/* @72 */ 	jsr         __rdec21
/* @73 */ 	lda         __i4
/* @74 */ 	ora         __i4+1
/* @76 */ 	beq         .memcmp_label_84
/* @78 */ 	lda          #__i0
/* @80 */ 	jsr         __rinc21
/* @81 */ 	lda          #__i2
/* @82 */ 	jsr         __rinc21
/* @83 */ 	bra         .memcmp_label_37
.memcmp_label_84:
/* @87 */ 	lda         (__i0)
/* @93 */ 	sta         __i1
/* @95 */ 	stz         __i1+1
/* @98 */ 	lda         (__i2)
/* @104 */ 	sta         __i3
/* @106 */ 	stz         __i3+1
/* @111 */ 	sec         
/* @112 */ 	lda         __i1
/* @113 */ 	sbc         __i3
/* @114 */ 	sta         __i5
/* @115 */ 	lda         __i1+1
/* @116 */ 	sbc         __i3+1
/* @117 */ 	sta         __i5+1
/* @119 */ 	lda         #__i5
/* @121 */ 	jsr         __result2
/* @123 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_memcmp:
	.size memcmp, .func_end_memcmp-memcmp

	.data
	.section ".rodata", "aMS", @progbits
