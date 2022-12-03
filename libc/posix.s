	.file   "/Users/dallison/Google Drive/c_compiler/libc/posix.c"
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


	.global open
	.type open, @function

open:
/* @6 */ 	stx         __result
/* @8 */ 	sty         __result+1
/* @9 */ 	ldx          #9
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @18 */ 	ldx          #0
	jsr          __arg_value2_i0			// filename
/* @20 */ 	ldx          #5
	jsr          __var_addr_i1			// ap
/* @22 */ 	ldx          #2
	jsr          __arg_addr_i2			// flags
/* @26 */ 	clc         
/* @28 */ 	ldy          #0
/* @29 */ 	lda         __i2
/* @30 */ 	adc          #2
/* @31 */ 	sta         (__i1)
/* @33 */ 	iny         
/* @34 */ 	lda         __i2+1
/* @35 */ 	adc          #0
/* @36 */ 	sta         (__i1), Y
/* @40 */ 	lda         __i1
/* @42 */ 	sta         __mem_src
/* @43 */ 	lda         __i1+1
/* @45 */ 	sta         __mem_src+1
/* @48 */ 	lda         #__i1
/* @50 */ 	sta         __mem_dest
/* @52 */ 	stz         __mem_dest+1
/* @54 */ 	jsr         __builtin_va_arg2
/* @57 */ 	jsr         __pushi1
/* @59 */ 	ldx          #2
	jsr          __arg_value2_i1			// flags
/* @62 */ 	jsr         __pushi1
/* @63 */ 	jsr         __pushi0
/* @64 */ 	ldx          #2
/* @66 */ 	jsr         __pushxy0
/* @68 */ 	ldx         #__i4
/* @69 */ 	ldy          #0
/* @70 */ 	jsr         syscall
/* @72 */ 	jsr         __incsp8
/* @74 */ 	ldx          #10
	jsr          __load_result
/* @75 */ 	lda         #__i4
/* @77 */ 	jsr         __result2
/* @79 */ 	ldy          #12
	jmp          __leave
.func_end_open:
	.size open, .func_end_open-open

	.global close
	.type close, @function

close:
/* @5 */ 	stx         __result
/* @7 */ 	sty         __result+1
/* @8 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @13 */ 	ldx          #0
	jsr          __arg_value2_i0			// fd
/* @14 */ 	jsr         __pushi0
/* @16 */ 	ldx          #3
/* @18 */ 	jsr         __pushxy0
/* @21 */ 	ldx         #__i4
/* @22 */ 	ldy          #0
/* @23 */ 	jsr         syscall
/* @25 */ 	jsr         __incsp4
/* @27 */ 	ldx          #8
	jsr          __load_result
/* @28 */ 	lda         #__i4
/* @30 */ 	jsr         __result2
/* @32 */ 	ldy          #10
	jmp          __leave
.func_end_close:
	.size close, .func_end_close-close

	.global write
	.type write, @function

write:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @15 */ 	ldx          #4
	jsr          __arg_value2_i0			// len
/* @19 */ 	ldx          #2
	jsr          __arg_value2_i1			// buffer
/* @23 */ 	ldx          #0
	jsr          __arg_value2_i2			// fd
/* @24 */ 	jsr         __pushi0
/* @25 */ 	jsr         __pushi1
/* @26 */ 	jsr         __pushi2
/* @28 */ 	ldx          #4
/* @30 */ 	jsr         __pushxy0
/* @33 */ 	ldx         #__i4
/* @34 */ 	ldy          #0
/* @35 */ 	jsr         syscall
/* @37 */ 	jsr         __incsp8
/* @39 */ 	ldx          #8
	jsr          __load_result
/* @40 */ 	lda         #__i4
/* @42 */ 	jsr         __result2
/* @44 */ 	ldy          #10
	jmp          __leave
.func_end_write:
	.size write, .func_end_write-write

	.global read
	.type read, @function

read:
/* @7 */ 	stx         __result
/* @9 */ 	sty         __result+1
/* @10 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @15 */ 	ldx          #4
	jsr          __arg_value2_i0			// len
/* @19 */ 	ldx          #2
	jsr          __arg_value2_i1			// buffer
/* @23 */ 	ldx          #0
	jsr          __arg_value2_i2			// fd
/* @24 */ 	jsr         __pushi0
/* @25 */ 	jsr         __pushi1
/* @26 */ 	jsr         __pushi2
/* @28 */ 	ldx          #5
/* @30 */ 	jsr         __pushxy0
/* @33 */ 	ldx         #__i4
/* @34 */ 	ldy          #0
/* @35 */ 	jsr         syscall
/* @37 */ 	jsr         __incsp8
/* @39 */ 	ldx          #8
	jsr          __load_result
/* @40 */ 	lda         #__i4
/* @42 */ 	jsr         __result2
/* @44 */ 	ldy          #10
	jmp          __leave
.func_end_read:
	.size read, .func_end_read-read

	.global lseek
	.type lseek, @function

lseek:
/* @8 */ 	stx         __result
/* @10 */ 	sty         __result+1
/* @11 */ 	ldx          #7
	jsr          __enter
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
/* @16 */ 	ldx          #6
	jsr          __arg_value2_i0			// whence
/* @20 */ 	ldx          #2
	jsr          __arg_value4_l0			// pos
/* @24 */ 	ldx          #0
	jsr          __arg_value2_i1			// fd
/* @25 */ 	jsr         __pushi0
/* @26 */ 	jsr         __pushl0
/* @27 */ 	jsr         __pushi1
/* @29 */ 	ldx          #7
/* @31 */ 	jsr         __pushxy0
/* @34 */ 	ldx         #__i4
/* @35 */ 	ldy          #0
/* @36 */ 	jsr         syscall
/* @38 */ 	jsr         __incsp10
/* @42 */ 	lda         __i4
/* @43 */ 	sta         __l0
/* @45 */ 	lda         __i4+1
/* @46 */ 	sta         __l0+1
/* @48 */ 	and          #128
/* @50 */ 	beq         .lseek_label_49
/* @52 */ 	lda          #255
.lseek_label_49:
/* @54 */ 	sta         __l0+2
/* @56 */ 	sta         __l0+3
/* @58 */ 	ldx          #8
	jsr          __load_result
/* @59 */ 	lda         #__l0
/* @61 */ 	jsr         __result4
/* @63 */ 	ldy          #10
	jmp          __leave
.func_end_lseek:
	.size lseek, .func_end_lseek-lseek

	.global abort
	.type abort, @function

abort:
/* @3 */ 	ldx          #7
	jsr          __enter_nomask
/* @6 */ 	ldx          #8
/* @8 */ 	jsr         __pushxy0
/* @11 */ 	ldx         #__i0
/* @12 */ 	ldy          #0
/* @13 */ 	jsr         syscall
/* @15 */ 	jsr         __incsp2
/* @16 */ 	ldy          #10
	jmp          __leave_void_nomask
.func_end_abort:
	.size abort, .func_end_abort-abort

	.global _Exit
	.type _Exit, @function

_Exit:
/* @3 */ 	ldx          #7
	jsr          __enter_nomask
/* @8 */ 	ldx          #0
	jsr          __arg_value2_i0			// status
/* @9 */ 	jsr         __pushi0
/* @11 */ 	ldx          #1
/* @13 */ 	jsr         __pushxy0
/* @16 */ 	ldx         #__i1
/* @17 */ 	ldy          #0
/* @18 */ 	jsr         syscall
/* @20 */ 	jsr         __incsp4
/* @21 */ 	ldy          #10
	jmp          __leave_void_nomask
.func_end__Exit:
	.size _Exit, .func_end__Exit-_Exit

	.data
	.section ".rodata", "aMS", @progbits
