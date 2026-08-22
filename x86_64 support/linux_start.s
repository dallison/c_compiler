.text

.global _start
.global __davecc_program_init
.global __davecc_linux_tls_init
.global main
.global exit
.type _start, @function

_start:
	mov %rsp, %r12
	mov $9, %rax
	mov $0, %rdi
	mov $10000, %rsi
	mov $3, %rdx
	mov $22, %r10
	mov $-1, %r8
	mov $0, %r9
	syscall
	mov %rax, %r13
	mov %r13, %rsi
	mov $1002, %rdi
	mov $9e, %rax
	syscall
	mov %r13, %rdi
	mov %r12, %rsi
	call __davecc_linux_tls_init
	call __davecc_program_init
	mov (%r12), %rdi
	lea 8(%r12), %rsi
	call main
	mov %rax, %rdi
	call exit
