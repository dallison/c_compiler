//
//  setjmp.s
//  aarch64 support
//
//  Save the AAPCS64 non-volatile execution context.  jmp_buf reserves 32
//  64-bit slots; the layout used here is shared with longjmp.s:
//    0..9   x19-x28
//    10     x29 (frame pointer)
//    11     x30 (return address)
//    12     sp
//    13..20 d8-d15
//

.text

.global setjmp
.type setjmp, @function

// int setjmp(jmp_buf env);
// env is in x0.
setjmp:
	str x19, [x0, #0]
	str x20, [x0, #8]
	str x21, [x0, #16]
	str x22, [x0, #24]
	str x23, [x0, #32]
	str x24, [x0, #40]
	str x25, [x0, #48]
	str x26, [x0, #56]
	str x27, [x0, #64]
	str x28, [x0, #72]
	str x29, [x0, #80]
	str x30, [x0, #88]
	mov x2, sp
	str x2, [x0, #96]
	fstr d8, [x0, #104]
	fstr d9, [x0, #112]
	fstr d10, [x0, #120]
	fstr d11, [x0, #128]
	fstr d12, [x0, #136]
	fstr d13, [x0, #144]
	fstr d14, [x0, #152]
	fstr d15, [x0, #160]
	mov x0, #0
	ret
