//
//  syscall.s
//  x86_64 support
//
//  Guest syscalls for the x86_64 interpreter (see x86_64_syscalls.h).
//

.text

.global syscall
.type syscall, @function

// int syscall(int n, ...);
// Linux x86_64 syscall ABI: rax=n, rdi, rsi, rdx, r10, r8, r9
// C SysV on entry: rdi=n, rsi, rdx, rcx, r8, r9
syscall:
	mov %rdi, %rax
	mov %rsi, %rdi
	mov %rdx, %rsi
	mov %rcx, %rdx
	mov %r8, %r10
	mov %r9, %r8
	.byte 0x0f, 0x05
	ret
