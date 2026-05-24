//
//  setjmp.s
//  x86_64 support
//
//  Save the compiler's logical integer register file (see x86_64_reg_alloc.c)
//  plus the return address for setjmp/longjmp.
//

.text

.global setjmp
.type setjmp, @function

// int setjmp(jmp_buf buf);
// SysV: buf in %rdi
setjmp:
	mov %rax, 0(%rdi)
	mov %rax, 8(%rdi)
	mov %rsp, 16(%rdi)
	mov %r11, 24(%rdi)
	mov %r10, 32(%rdi)
	mov %r10, 40(%rdi)
	mov %r11, 48(%rdi)
	mov %r12, 56(%rdi)
	mov %rbp, 64(%rdi)
	mov %rbx, 72(%rdi)
	mov %rdi, 80(%rdi)
	mov %rsi, 88(%rdi)
	mov %rdx, 96(%rdi)
	mov %rcx, 104(%rdi)
	mov %r8, 112(%rdi)
	mov %r9, 120(%rdi)
	mov %r10, 128(%rdi)
	mov %r11, 136(%rdi)
	mov %r12, 144(%rdi)
	mov %r13, 152(%rdi)
	mov %r14, 160(%rdi)
	mov %r15, 168(%rdi)
	mov %r12, 176(%rdi)
	mov %r13, 184(%rdi)
	mov %r14, 192(%rdi)
	mov %r15, 200(%rdi)
	mov %r10, 208(%rdi)
	mov %r11, 216(%rdi)
	mov %r8, 224(%rdi)
	mov %r9, 232(%rdi)
	mov %r10, 240(%rdi)
	mov %r11, 248(%rdi)

	mov (%rsp), %rax
	mov %rax, 256(%rdi)

	xor %eax, %eax
	ret
