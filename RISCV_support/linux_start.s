.text

.global _start
.global __davecc_program_init
.global __davecc_linux_tls_init
.global main
.global exit
.type _start, @function

_start:
	mv s0, sp
	li a0, 0
	li a1, 65536
	li a2, 3
	li a3, 34
	li a4, -1
	li a5, 0
	li a7, 222
	ecall
	mv s1, a0
	mv x4, s1
	mv a0, s1
	mv a1, s0
	call __davecc_linux_tls_init
	call __davecc_program_init
	ld a0, 0(s0)
	addi a1, s0, 8
	call main
	call exit
