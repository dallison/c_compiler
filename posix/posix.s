	.file   "Google Drive/c_compiler/posix/posix.c"
	.text
	.global syscall
	.type syscall, @function

syscall:
	li t6, n
ecall

.syscall_label_5:
	ret         
.func_end_syscall:
	.size syscall, .func_end_syscall-syscall

	.global open
	.type open, @function

open:
	.global syscall
	addi sp, sp, -32
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	sd s2, 8(sp)
	sd s3, 0(sp)
	mv   s2, a0
	mv   s3, a1
	mv      a2, s3
	mv      a1, s2
	li          a0, 2
	call    syscall
.open_label_19:
	ld s2, 8(sp)
	ld s3, 0(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_open:
	.size open, .func_end_open-open

	.global close
	.type close, @function

close:
	.global syscall
	addi sp, sp, -32
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	sd s2, 8(sp)
	mv   s2, a0
	mv      a1, s2
	li          a0, 3
	call    syscall
.close_label_15:
	ld s2, 8(sp)
	ld ra, 24(sp)
	ld s0, 16(sp)
	addi sp, sp, 32
	ret         
.func_end_close:
	.size close, .func_end_close-close

	.global write
	.type write, @function

write:
	.global syscall
	addi sp, sp, -48
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	fmv.d   fs2, fa0
	mv   s3, a1
	mv   s4, a2
	mv      a3, s4
	mv      a2, s3
	mv      a1, s2
	li          a0, 4
	call    syscall
.write_label_23:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.func_end_write:
	.size write, .func_end_write-write

	.global read
	.type read, @function

read:
	.global syscall
	addi sp, sp, -48
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	mv   s2, a0
	mv   s3, a1
	mv   s4, a2
	mv      a3, s4
	mv      a2, s3
	mv      a1, s2
	li          a0, 5
	call    syscall
.read_label_23:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.func_end_read:
	.size read, .func_end_read-read

	.global lseek
	.type lseek, @function

lseek:
	.global syscall
	addi sp, sp, -48
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	sd s2, 24(sp)
	sd s3, 16(sp)
	sd s4, 8(sp)
	mv   s2, a0
	mv   s3, a1
	mv   s4, a2
	mv      a3, s4
	mv      a2, s3
	mv      a1, s2
	li          a0, 7
	call    syscall
.lseek_label_23:
	ld s2, 24(sp)
	ld s3, 16(sp)
	ld s4, 8(sp)
	ld ra, 40(sp)
	ld s0, 32(sp)
	addi sp, sp, 48
	ret         
.func_end_lseek:
	.size lseek, .func_end_lseek-lseek

	.data
	.section ".rodata", "aMS", @progbits
