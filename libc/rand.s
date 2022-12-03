	.file   "/Users/dallison/Google Drive/c_compiler/libc/rand.c"
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


	.global rand
	.type rand, @function

rand:
/* @8 */ 	stx         __result
/* @10 */ 	sty         __result+1
/* @11 */ 	ldx          #5
	jsr          __enter_leaf
	.byte        0x00,0x02,0x00		// Save mask i:0 b:0 l:1 x:0 f:0 
/* @17 */ 	ldy          #3
/* @18 */ 	ldx          #3
.rand_label_19:
/* @21 */ 	lda         next+-1, Y
/* @22 */ 	sta         __l0, X
/* @23 */ 	dey         
/* @24 */ 	dex         
/* @25 */ 	bpl         .rand_label_19
/* @29 */ 	ldx          #3
.rand_label_30:
/* @31 */ 	lda         .lit.1, X
/* @32 */ 	sta         __l1, X
/* @33 */ 	dex         
/* @34 */ 	bpl         .rand_label_30
/* @39 */ 	lda          #__l2
/* @40 */ 	ldx          #__l0
/* @41 */ 	ldy          #__l1
/* @43 */ 	jsr         __umul4
/* @47 */ 	clc         
/* @50 */ 	ldy          #4
/* @52 */ 	ldx          #0
.rand_label_53:
/* @54 */ 	lda         __l2, X
/* @55 */ 	adc         .lit.2, X
/* @56 */ 	sta         __l0, X
/* @57 */ 	inx         
/* @58 */ 	dey         
/* @59 */ 	bne         .rand_label_53
/* @61 */ 	ldx          #3
.rand_label_62:
/* @63 */ 	lda         __l0, X
/* @64 */ 	sta         next+-1, Y
/* @65 */ 	dex         
/* @66 */ 	bpl         .rand_label_62
/* @70 */ 	lda         #%lo(next)
/* @71 */ 	sta         __i0
/* @73 */ 	lda         #%hi(next)
/* @74 */ 	sta         __i0+1
/* @78 */ 	ldy          #2
/* @79 */ 	lda         (__i0), Y
/* @80 */ 	sta         __l0
/* @81 */ 	iny         
/* @82 */ 	lda         (__i0), Y
/* @83 */ 	sta         __l0+1
/* @85 */ 	stz         __l0+2
/* @86 */ 	sta         __l0+3
/* @90 */ 	lda         __l0
/* @91 */ 	sta         __i0
/* @92 */ 	lda         __l0+1
/* @93 */ 	sta         __i0+1
/* @97 */ 	lda         __i0
/* @98 */ 	sta         __i1
/* @99 */ 	lda         __i0+1
/* @100 */ 	and          #127
/* @101 */ 	sta         __i1+1
/* @103 */ 	lda         #__i1
/* @105 */ 	jsr         __result2
/* @107 */ 	ldy          #8
	jmp          __leave_leaf
.func_end_rand:
	.size rand, .func_end_rand-rand

	.global srand
	.type srand, @function

srand:
/* @2 */ 	ldx          #5
	jsr          __enter_leaf_nomask
/* @7 */ 	ldx          #0
	jsr          __arg_value2_i0			// seed
/* @10 */ 	lda         __i0
/* @11 */ 	sta         __l0
/* @13 */ 	lda         __i0+1
/* @14 */ 	sta         __l0+1
/* @17 */ 	stz         __l0+2
/* @19 */ 	sta         __l0+3
/* @21 */ 	ldx          #3
.srand_label_22:
/* @24 */ 	lda         __l0, X
/* @25 */ 	sta         next+-1, Y
/* @26 */ 	dex         
/* @27 */ 	bpl         .srand_label_22
/* @28 */ 	ldy          #8
	jmp          __leave_leaf_void_nomask
.func_end_srand:
	.size srand, .func_end_srand-srand

	.data
	.p2align  0
next:
	.type   next,@object
	.local  next
	.size   next,4
	.word   1

	.section ".rodata", "aMS", @progbits
.lit.1:
	.byte 0x6d
	.byte 0x4e
	.byte 0xc6
	.byte 0x41
	.type .lit.1, @object
	.size .lit.1, 4

.lit.2:
	.byte 0x39
	.byte 0x30
	.byte 0x00
	.byte 0x00
	.type .lit.2, @object
	.size .lit.2, 4

