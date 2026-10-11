	.file   "libc/tmpfile.c"
	.file 1 "libc/include/stdio.h"
	.file 2 "libc/include/stdarg.h"
	.file 3 "libc/tmpfile.c"
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

.PCbegin:
	.section ".text.MakeTemporaryName", "ax", @progbits
	.local  MakeTemporaryName
	.type MakeTemporaryName, @function

MakeTemporaryName:
	lda          #7
	jsr          __enter_res
	.byte        0x01,0x00,0x00		// Save mask i:1 b:0 l:0 x:0 f:0 
	ldx          #0
	jsr          __arg_value2_i0			// buffer
	.loc 4 6 47
	ldy          #3
	ldx          #3
.MakeTemporaryName_label_23:
	lda         temporary_file_counter+0, Y
	sta         __l1, X
	dey         
	dex         
	bpl         .MakeTemporaryName_label_23
	lda         #%lo(temporary_file_counter)
	sta         __i1
	lda         #%hi(temporary_file_counter)
	sta         __i1+1
	lda          #__i1
	jsr         __inc4
	.loc 4 7 1
	lda         __i0
	sta         __i4
	lda         __i0+1
	sta         __i4+1
	jsr         __pushl1
	ldx         #%lo(.str.1)
	ldy         #%hi(.str.1)
	jsr         __pushxy
	ldx          #16
	jsr         __pushxy0
	jsr         __pushi4
	ldx         #__i1
	ldy          #0
	jsr         __snprintf_long
	jsr         __incsp10
	.loc 4 8 1
	ldx          #8
	lda          #__i4
	jsr         __load_result_value2
	ldy          #12
	jmp          __leave
.func_end_MakeTemporaryName:
	.size MakeTemporaryName, .func_end_MakeTemporaryName-MakeTemporaryName

	.section ".text.tmpnam", "ax", @progbits
	.global tmpnam
	.type tmpnam, @function

tmpnam:
	lda          #7
	jsr          __enter_res_nomask
	ldx          #0
	jsr          __arg_value2_i0			// output
	.loc 4 13 27
	lda         __i0
	bne         .tmpnam_label_26
	lda         __i0+1
	bne         .tmpnam_label_26
	.loc 4 14 1
	lda         #%lo(.local.buffer.402)
	sta         __i0
	lda         #%hi(.local.buffer.402)
	sta         __i0+1
.tmpnam_label_26:
	.loc 4 16 1
	ldy          #10
	jsr          __leave_nomask
	ldx         __result
	ldy         __result+1
	jmp         MakeTemporaryName
.func_end_tmpnam:
	.size tmpnam, .func_end_tmpnam-tmpnam

	.section ".text.tmpfile", "ax", @progbits
	.global tmpfile
	.type tmpfile, @function

tmpfile:
	lda          #23
	jsr          __enter_res
	.byte        0x03,0x00,0x00		// Save mask i:3 b:0 l:0 x:0 f:0 
	.loc 4 23 48
.tmpfile_label_20:
	.loc 4 24 1
	ldx          #19
	lda          #__i5
	jsr          __var_addr_push_i5
	ldx         #__i0
	ldy          #0
	jsr         MakeTemporaryName
	.loc 4 25 1
	ldx         #%lo(.str.11)
	ldy         #%hi(.str.11)
	jsr         __pushxy
	jsr         __pushi5
	ldx         #__i6
	ldy          #0
	jsr         fopen
	lda         __i6
	sta         __i4
	lda         __i6+1
	sta         __i4+1
	.loc 4 26 27
	lda         __i6
	bne         .tmpfile_label_54
	lda         __i6+1
	beq         .tmpfile_label_98
.tmpfile_label_54:
	.loc 4 27 28
	jsr         __pushi5
	ldx         #__i0
	ldy          #0
	jsr         remove
	lda         __i0
	ora         __i0+1
	bne         .tmpfile_label_81
	.loc 4 28 1
	ldx          #24
	lda          #__i4
	jsr         __load_result_value2
	ldy          #26
	jmp          __leave
.tmpfile_label_81:
	.loc 4 30 1
	jsr         __pushi4
	ldx         #__i0
	ldy          #0
	jsr         fclose
	.loc 4 31 1
	jsr         __pushi5
	ldx         #__i0
	ldy          #0
	jsr         remove
.tmpfile_label