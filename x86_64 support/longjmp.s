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
	mov %rsi, %r12
	test %r12, %r12
	jnz Lj_skip_one
	mov $1, %r12
Lj_skip_one:
	// Restore the saved register file.  Skip slots that map to %rax (0, 1),
	// %rdi (10, the buf pointer), and %r12 (7, 18, 22) which holds the
	// return value until the end.
	mov 248(%rdi), %r11
	mov 240(%rdi), %r10
	mov 232(%rdi), %r9
	mov 224(%rdi), %r8
	mov 216(%rdi), %r11
	mov 208(%rdi), %r10
	mov 200(%rdi), %r15
	mov 192(%rdi), %r14
	mov 184(%rdi), %r13
	mov 168(%rdi), %r15
	mov 160(%rdi), %r14
	mov 152(%rdi), %r13
	mov 136(%rdi), %r11
	mov 128(%rdi), %r10
	mov 120(%rdi), %r9
	mov 112(%rdi), %r8
	mov 104(%rdi), %rcx
	mov 96(%rdi), %rdx
	mov 88(%rdi), %rsi
	mov 72(%rdi), %rbx
	mov 64(%rdi), %rbp
	mov 48(%rdi), %r11
	mov 40(%rdi), %r10
	mov 32(%rdi), %r10
	mov 24(%rdi), %r11
	mov 16(%rdi), %rsp

	mov %r12, %rax
	mov 256(%rdi), %r11
	pushq %r11
	ret
