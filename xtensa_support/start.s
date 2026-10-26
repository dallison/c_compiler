	.section ".text.start", "ax", @progbits
	.align 2
	.global _start
	.type _start, @function
_start:
	// The hosted loader initializes a1.  CALL8 presents arguments in a10-a15;
	// main(void) does not consume any, but zero argc/argv for main signatures.
	call8 __davecc_program_init
	li a10, 0
	li a11, 0
	call8 main
	// main's result is already in a10, which is exit's first argument.
	// exit runs atexit handlers and flushes stdio before the host trap.
	call8 exit
.Lstart_end:
	.size _start, .Lstart_end-_start
