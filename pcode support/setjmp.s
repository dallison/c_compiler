.text

.global setjmp
.type setjmp, @function
setjmp:
	// The interpreter reads env from [sp, #8] and snapshots the complete
	// virtual register file before this function changes any registers.
	esc #257
	movc r0, #0
	ret
