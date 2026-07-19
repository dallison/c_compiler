.text

.global setjmp
.type setjmp, @function
setjmp:
	str r4, [r0, #0]
	str r5, [r0, #4]
	str r6, [r0, #8]
	str r7, [r0, #12]
	str r8, [r0, #16]
	str r9, [r0, #20]
	str r10, [r0, #24]
	str r11, [r0, #28]
	str sp, [r0, #32]
	str lr, [r0, #36]
	vstr d8, [r0, #40]
	vstr d9, [r0, #48]
	vstr d10, [r0, #56]
	vstr d11, [r0, #64]
	vstr d12, [r0, #72]
	vstr d13, [r0, #80]
	vstr d14, [r0, #88]
	vstr d15, [r0, #96]
	mov r0, #0
	bx lr
