//
//  longjmp.s
//  aarch64 support
//

.text

.global longjmp
.type longjmp, @function

// void longjmp(jmp_buf env, int value);
// env is in x0, value is in w1.  The saved x30 is the continuation directly
// after setjmp's original call.
longjmp:
	mov x2, x0
	// value is an int; the upper half of x1 is not part of the argument.
	cbnz w1, .Llongjmp_value_ready
	mov w1, #1
.Llongjmp_value_ready:
	ldr x19, [x2, #0]
	ldr x20, [x2, #8]
	ldr x21, [x2, #16]
	ldr x22, [x2, #24]
	ldr x23, [x2, #32]
	ldr x24, [x2, #40]
	ldr x25, [x2, #48]
	ldr x26, [x2, #56]
	ldr x27, [x2, #64]
	ldr x28, [x2, #72]
	ldr x29, [x2, #80]
	ldr x30, [x2, #88]
	ldr x3, [x2, #96]
	fldr d8, [x2, #104]
	fldr d9, [x2, #112]
	fldr d10, [x2, #120]
	fldr d11, [x2, #128]
	fldr d12, [x2, #136]
	fldr d13, [x2, #144]
	fldr d14, [x2, #152]
	fldr d15, [x2, #160]
	mov sp, x3
	mov w0, w1
	br x30
