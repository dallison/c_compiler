.text

.global longjmp
.type longjmp, @function
longjmp:
	mov r3, r1
	cmp r3, #0
	bne .Llongjmp_value_ready
	mov r3, #1
.Llongjmp_value_ready:
	ldr r4, [r0, #0]
	ldr r5, [r0, #4]
	ldr r6, [r0, #8]
	ldr r7, [r0, #12]
	ldr r8, [r0, #16]
	ldr r9, [r0, #20]
	ldr r10, [r0, #24]
	ldr r11, [r0, #28]
	vldr d8, [r0, #40]
	vldr d9, [r0, #48]
	vldr d10, [r0, #56]
	vldr d11, [r0, #64]
	vldr d12, [r0, #72]
	vldr d13, [r0, #80]
	vldr d14, [r0, #88]
	vldr d15, [r0, #96]
	ldr sp, [r0, #32]
	ldr lr, [r0, #36]
	mov r0, r3
	bx lr
