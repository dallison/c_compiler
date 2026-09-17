	.text
	.align 2
	.global setjmp
	.type setjmp, @function
setjmp:
	entry a1, 16
	// a2 holds jmp_buf.  The hosted interpreter snapshots the call stack
	// on break 1,1; retw then returns 0 to the caller as usual.
	break 1, 1
	li a2, 0
	retw
.Lsetjmp_end:
	.size setjmp, .Lsetjmp_end-setjmp
