.text

.global _start
.global __davecc_program_init
.global __davecc_linux_tls_init
.global main
.global exit
.type _start, @function

_start:
	mov r8, sp
	mov r0, #0
	mov r1, #65536
	mov r2, #3
	mov r3, #34
	mvn r4, #0
	mov r5, #0
	mov r7, #192
	svc #0
	mov r9, r0
	mov r0, r9
	movw r7, #5
	movt r7, #15
	svc #0
	mov r0, r9
	mov r1, r8
	bl __davecc_linux_tls_init
	bl __davecc_program_init
	ldr r0, [r8]
	add r1, r8, #4
	bl main
	bl exit
