	.section ".text.start", "ax", @progbits
	.align 2
	.global _start
	.type _start, @function
_start:
	// The hosted loader initializes a1.  CALL8 presents arguments in a10-a15;
	// main(void) does not consume any, but zero argc/argv for main signatures.
	li a10, 0
	li a11, 0
	call8 main
	mov a2, a10
	// Private hosted-runtime EXIT service, with status in a2.
	li a8, 12
	break 1, 0
.Lstart_end:
	.size _start, .Lstart_end-_start
