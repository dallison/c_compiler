//
//  syscall.s
//  arm support
//
//  Guest syscalls for the ARM interpreter (see arm_interpreter.h).
//  The interpreter expects the syscall number in r7 and the arguments in
//  r1-r5 (the first C argument, the syscall number, is in r0).  The result is
//  returned in r0.
//

.text

.global syscall
.type syscall, @function

// int syscall(int n, ...);
syscall:
	push {r4, r5, r7}
	mov r7, r0
	ldr r4, [sp, #12]
	ldr r5, [sp, #16]
	.word 0xef000000	// swi #0
	pop {r4, r5, r7}
	bx lr
