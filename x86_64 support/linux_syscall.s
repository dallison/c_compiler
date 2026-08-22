.text

.global __davecc_linux_syscall6
.type __davecc_linux_syscall6, @function

__davecc_linux_syscall6:
	mov %rdi, %rax
	mov 8(%rsp), %r11
	mov %rsi, %rdi
	mov %rdx, %rsi
	mov %rcx, %rdx
	mov %r8, %r10
	mov %r9, %r8
	mov %r11, %r9
	syscall
	ret
