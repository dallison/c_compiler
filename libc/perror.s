	.file   "/Users/dallison/Google Drive/c_compiler/libc/perror.c"
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


	.global perror
	.type perror, @function

perror:
/* @10 */ 	ldx          #7
	jsr          __enter
	.byte        0x22,0x00,0x00		// Save mask i:2 b:1 l:0 x:0 f:0 
/* @21 */ 	ldx          #0
	jsr          __arg_value2_i5			// s
/* @25 */ 	lda          #214
/* @26 */ 	sta         __i1
/* @28 */ 	lda          #3
/* @29 */ 	sta         __i1+1
/* @32 */ 	lda         (__i1)
/* @33 */ 	sta         __i0
/* @34 */ 	ldy          #1
/* @35 */ 	lda         (__i1), Y
/* @36 */ 	sta         __i0+1
/* @39 */ 	jsr         __pushi0
/* @40 */ 	ldx         #__i4
/* @41 */ 	ldy          #0
/* @42 */ 	jsr         strerror
/* @44 */ 	jsr         __incsp2
/* @50 */ 	ldx          #0
/* @51 */ 	lda         __i5
/* @53 */ 	bne         .perror_label_49
/* @54 */ 	lda         __i5+1
/* @56 */ 	beq         .perror_label_48
.perror_label_49:
/* @57 */ 	inx         
.perror_label_48:
/* @58 */ 	stx         __b2
/* @60 */ 	txa         
/* @61 */ 	cmp          #0
/* @62 */ 	beq         .perror_label_90
/* @65 */ 	lda         (__i5)
/* @71 */ 	sta         __i0
/* @73 */ 	and          #128
/* @75 */ 	beq         .perror_label_74
/* @77 */ 	lda          #255
.perror_label_74:
/* @78 */ 	sta         __i0+1
/* @83 */ 	ldx          #1
/* @84 */ 	lda         __i0
/* @85 */ 	ora         __i0+1
/* @87 */ 	bne         .perror_label_82
/* @88 */ 	dex         
.perror_label_82:
/* @89 */ 	stx         __b2
.perror_label_90:
/* @92 */ 	lda         __b2
/* @94 */ 	beq         .perror_label_116
/* @95 */ 	jsr         __pushi4
/* @96 */ 	jsr         __pushi5
/* @98 */ 	ldx         #%lo(.str.1)
	ldy         #%hi(.str.1)
/* @100 */ 	jsr         __pushxy
/* @103 */ 	lda         stderr+0
/* @104 */ 	sta         __i0
/* @105 */ 	lda         stderr+1
/* @106 */ 	sta         __i0+1
/* @108 */ 	jsr         __pushi0
/* @110 */ 	ldx         #__i0
/* @111 */ 	ldy          #0
/* @112 */ 	jsr         fprintf
/* @114 */ 	jsr         __incsp8
/* @115 */ 	bra         .perror_label_127
.perror_label_116:
/* @117 */ 	jsr         __pushi4
/* @119 */ 	ldx         #%lo(.str.2)
	ldy         #%hi(.str.2)
/* @120 */ 	jsr         __pushxy
/* @122 */ 	ldx         #__i0
/* @123 */ 	ldy          #0
/* @124 */ 	jsr         printf
/* @126 */ 	jsr         __incsp4
.perror_label_127:
/* @128 */ 	ldy          #10
	jmp          __leave_void
.func_end_perror:
	.size perror, .func_end_perror-perror

	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s: %s\n"
	.type .str.1, @object
	.size .str.1, 8

.str.2:
	.asciz "%s\n"
	.type .str.2, @object
	.size .str.2, 4

