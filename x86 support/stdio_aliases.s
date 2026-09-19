//
//  stdio_aliases.s
//  x86 support (i386)
//
//  libc compiles printf-family functions as __printf / __fprintf / ...
//  User programs still call the unprefixed names.
//

.text

.global printf
.type printf, @function
printf:
	jmp __printf

.global fprintf
.type fprintf, @function
fprintf:
	jmp __fprintf

.global sprintf
.type sprintf, @function
sprintf:
	jmp __sprintf

.global snprintf
.type snprintf, @function
snprintf:
	jmp __snprintf
