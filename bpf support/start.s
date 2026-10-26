.text
.global _start
.type _start, @function
_start:
	call __davecc_program_init
	mov r1, 0
	mov r2, 0
	call main
	mov r1, r0
	call exit
	exit
.Lstart_end:
.size _start, .Lstart_end-_start
