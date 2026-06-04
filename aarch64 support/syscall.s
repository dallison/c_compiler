//
//  syscall.s
//  aarch64 support
//
//  Guest syscalls for the aarch64 interpreter (see aarch64_syscalls.h).
//  Syscall number in x16; args in x0-x5.
//

.text

.global syscall
.type syscall, @function

// int syscall(int n, ...);
syscall:
	mov x16, x0
	mov x0, x1
	mov x1, x2
	mov x2, x3
	mov x3, x4
	mov x4, x5
	mov x5, x6
	.word 0xd4000001
	ret
