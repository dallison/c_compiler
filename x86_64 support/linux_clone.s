.text

.global __davecc_linux_clone
.type __davecc_linux_clone, @function

__davecc_linux_clone:
	push %r12
	push %r13
	sub $16, %rdi
	mov %rdx, 0(%rdi)
	mov %rcx, 8(%rdi)
	mov %rdi, %rax
	mov %rsi, %rdi
	mov %rax, %rsi
	mov %r9, %rdx
	mov %r9, %r10
	mov $38, %rax
	syscall
	test %rax, %rax
	jz .Lclone_child
	pop %r13
	pop %r12
	ret

.Lclone_child:
	mov 0(%rsp), %rdi
	mov 8(%rsp), %r13
	call *%r13
	mov %rax, %rdi
	mov $3c, %rax
	syscall
