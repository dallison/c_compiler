.text

.global __davecc_linux_syscall6
.type __davecc_linux_syscall6, @function

__davecc_linux_syscall6:
	mov %rdi, %rax
	mov %rsi, %rdi
	mov %rdx, %rsi
	mov %rcx, %rdx
	mov %r8, %r10
	mov %r9, %r8
	.byte 0x0f, 0x05
	ret
