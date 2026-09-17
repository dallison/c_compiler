	.text
	.align 2
	.global longjmp
	.type longjmp, @function
longjmp:
	entry a1, 16
	// a2 = jmp_buf, a3 = value.  The hosted interpreter restores the
	// setjmp frame on break 1,2 and does not return here.
	break 1, 2
	retw
.Llongjmp_end:
	.size longjmp, .Llongjmp_end-longjmp
