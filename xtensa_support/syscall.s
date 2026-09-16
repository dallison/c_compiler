	.text
	.align 2
	.global syscall
	.type syscall, @function
syscall:
	entry a1, 16
	// The C entry sequence presents n,args... in a2-a7.  The hosted trap
	// consumes the service number in a8 and arguments in a2-a5.
	mov a8, a2
	mov a2, a3
	mov a3, a4
	mov a4, a5
	mov a5, a6
	break 1, 0
	retw
.Lsyscall_end:
	.size syscall, .Lsyscall_end-syscall
