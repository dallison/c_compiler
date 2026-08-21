.text

.global __davecc_linux_clone
.type __davecc_linux_clone, @function

__davecc_linux_clone:
	push {r4, r5, r6, r7, r8, r9, lr}
	sub r6, r0, #8
	str r2, [r6]
	str r3, [r6, #4]
	mov r8, r2
	mov r9, r3
	ldr r3, [sp, #28]
	ldr r2, [sp, #32]
	mov r4, r2
	mov r0, r1
	mov r1, r6
	mov r7, #120
	.word 0xef000000
	cmp r0, #0
	beq .Lclone_child
	pop {r4, r5, r6, r7, r8, r9, lr}
	bx lr

.Lclone_child:
	ldr r0, [sp]
	ldr r9, [sp, #4]
	blx r9
	mov r7, #1
	.word 0xef000000
