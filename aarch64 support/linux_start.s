//
// Minimal Linux process entry for statically linked AArch64 programs.
//

.text

.global _start
.global __davecc_program_init
.global __davecc_linux_tls_init
.global main
.global exit
.type _start, @function

_start:
	mov x19, sp
	mov x0, #0
	mov x1, #65536
	mov x2, #3
	mov x3, #34
	movn x4, #0
	mov x5, #0
	mov x8, #222
	svc #0
	mov x20, x0
	// msr tpidr_el0, x0
	.word 0xd51bd040
	mov x0, x20
	mov x1, x19
	bl __davecc_linux_tls_init
	bl __davecc_program_init
	ldr x0, [x19]
	add x1, x19, #8
	bl main
	bl exit
