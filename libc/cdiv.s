	.file   "/Users/dallison/Google Drive/c_compiler/libc/cdiv.c"
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


	.global div
	.type div, @function

div:
/* @4 */ 	ldx          #7
	jsr          __enter_nomask
/* @9 */ 	ldx          #4
	jsr          __arg_value2_i0			// denom
/* @13 */ 	ldx          #2
	jsr          __arg_value2_i1			// numer
/* @14 */ 	lda          #__i2			// struct return address
	ldx          #0
	jsr          __arg_value2
/* @15 */ 	jsr         __pushi0
/* @16 */ 	jsr         __pushi1
/* @17 */ 	jsr         __pushi2
/* @18 */ 	jsr         __cdivmod2
/* @20 */ 	jsr         __pullxy
/* @22 */ 	stx         __i2
/* @24 */ 	sty         __i2+1
/* @26 */ 	jsr         __incsp4
/* @28 */ 	ldy          #10
	jmp          __leave_void_nomask
.func_end_div:
	.size div, .func_end_div-div

	.global ldiv
	.type ldiv, @function

ldiv:
/* @4 */ 	ldx          #7
	jsr          __enter_nomask
/* @9 */ 	ldx          #6
	jsr          __arg_value4_l0			// denom
/* @13 */ 	ldx          #2
	jsr          __arg_value4_l1			// numer
/* @14 */ 	lda          #__i0			// struct return address
	ldx          #0
	jsr          __arg_value2
/* @15 */ 	jsr         __pushl0
/* @16 */ 	jsr         __pushl1
/* @17 */ 	jsr         __pushi0
/* @18 */ 	jsr         __cdivmod4
/* @20 */ 	jsr         __pullxy
/* @22 */ 	stx         __i0
/* @24 */ 	sty         __i0+1
/* @26 */ 	jsr         __incsp8
/* @28 */ 	ldy          #10
	jmp          __leave_void_nomask
.func_end_ldiv:
	.size ldiv, .func_end_ldiv-ldiv

	.global lldiv
	.type lldiv, @function

lldiv:
/* @4 */ 	ldx          #7
	jsr          __enter
	.byte        0x00,0x20,0x00		// Save mask i:0 b:0 l:0 x:1 f:0 
/* @9 */ 	ldx          #10
	jsr          __arg_value8_x0			// denom
/* @12 */ 	lda          #__i0			// struct return address
	ldx          #0
	jsr          __arg_value2
/* @13 */ 	jsr         __pushx0
/* @15 */ 	ldx          #2
	jsr          __arg_value8_x1			// numer
/* @18 */ 	jsr         __pushx1
/* @19 */ 	jsr         __pushi0
/* @20 */ 	jsr         __cdivmod8
/* @22 */ 	jsr         __pullxy
/* @24 */ 	stx         __i0
/* @26 */ 	sty         __i0+1
/* @28 */ 	jsr         __incsp16
/* @30 */ 	ldy          #10
	jmp          __leave_void
.func_end_lldiv:
	.size lldiv, .func_end_lldiv-lldiv

	.data
	.section ".rodata", "aMS", @progbits
