.text

.global __davecc_linux_syscall6
.type __davecc_linux_syscall6, @function

__davecc_linux_syscall6:
	mv a7, a0
	mv a0, a1
	mv a1, a2
	mv a2, a3
	mv a3, a4
	mv a4, a5
	mv a5, a6
	ecall
	ret

.global __tls_get_addr
.type __tls_get_addr, @function

__tls_get_addr:
	lw a1, 4(a0)
	addi a0, x4, 16
	add a0, a0, a1
	ret
