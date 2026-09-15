.text

.global __davecc_linux_syscall6
.type __davecc_linux_syscall6, @function

__davecc_linux_syscall6:
	push {r4, r5, r7}
	mov r7, r0
	mov r0, r1
	mov r1, r2
	mov r2, r3
	ldr r3, [sp, #12]
	ldr r4, [sp, #16]
	ldr r5, [sp, #20]
	svc #0
	pop {r4, r5, r7}
	bx lr

.global __tls_get_addr
.type __tls_get_addr, @function

__tls_get_addr:
	mrc p15, 0, r2, c13, c0, 3
	add r2, r2, #8
	ldr r1, [r0, #4]
	add r0, r2, r1
	bx lr
