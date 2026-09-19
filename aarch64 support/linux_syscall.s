// Linux AArch64 syscall ABI.
//

.text

.global __davecc_linux_syscall6
.type __davecc_linux_syscall6, @function

__davecc_linux_syscall6:
	mov x8, x0
	mov x0, x1
	mov x1, x2
	mov x2, x3
	mov x3, x4
	mov x4, x5
	mov x5, x6
	svc #0
	ret

.global __tls_get_addr
.type __tls_get_addr, @function

// x0 = &tls_index { module, offset }.  Single-module: TP + TCB + offset.
__tls_get_addr:
	mrs x1, tpidr_el0
	ldr x2, [x0, #8]
	add x0, x1, #16
	add x0, x0, x2
	ret
