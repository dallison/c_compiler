	.file   "/Users/dallison/Google Drive/c_compiler/libc/syscall.c"
	.text
.PCbegin:
	.global syscall
	.type syscall, @function

syscall:

	// *** Basic block 0

/* @2 */ 	// Local vars at offset -16(s0)
	// End of stack frame
/* @4 */ 	mv t6, a0
ecall

	// *** Basic block 1

.syscall_label_5:
/* @6 */ 	// Restored registers.
/* @7 */ 	ret         
.func_end_syscall:
	.size syscall, .func_end_syscall-syscall

	.data
	.section ".rodata", "aMS", @progbits
