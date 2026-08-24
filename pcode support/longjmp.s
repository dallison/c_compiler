.text

.global longjmp
.type longjmp, @function
longjmp:
	// The interpreter reads env and value from this call frame, restores the
	// saved context, and resumes after the original call to setjmp.
	esc #258
