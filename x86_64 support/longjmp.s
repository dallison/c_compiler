//
//  longjmp.s
//  x86_64 support
//

.text

.global longjmp
.type longjmp, @function

// void longjmp(jmp_buf buf, int value);
// SysV: buf in %rdi, value in %rsi
longjmp:
	mov %rsi, %rax
	test %rax, %rax
	jnz Lj_skip_one
	mov $1, %rax
Lj_skip_one:
	// Restore the saved register file.  Skip slots that map to %rax (0, 1),
	// because %rax holds the setjmp return value.  Do not restore %r11 here:
	// DaveCC reserves it as a scratch register, and we use it for the saved
	// return address.
	mov 240(%rdi), %r10
	mov 232(%rdi), %r9
	mov 224(%rdi), %r8
	mov 208(%rdi), %r10
	mov 200(%rdi), %r15
	mov 192(%rdi), %r14
	mov 184(%rdi), %r13
	mov 168(%rdi), %r15
	mov 160(%rdi), %r14
	mov 152(%rdi), %r13
	mov 144(%rdi), %r12
	mov 128(%rdi), %r10
	mov 120(%rdi), %r9
	mov 112(%rdi), %r8
	mov 104(%rdi), %rcx
	mov 96(%rdi), %rdx
	mov 88(%rdi), %rsi
	mov 72(%rdi), %rbx
	mov 64(%rdi), %rbp
	mov 40(%rdi), %r10
	mov 32(%rdi), %r10
	mov 16(%rdi), %rsp
	mov 256(%rdi), %r11
	mov 80(%rdi), %rdi

	jmp *%r11
