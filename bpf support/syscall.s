.text
.global syscall
.type syscall, @function
syscall:
	// r1 is the guest service number and r2-r5 are its arguments.
	// 2147483647 is the interpreter's host-syscall helper, not a kernel id.
	call 2147483647
	exit
.Lsyscall_end:
.size syscall, .Lsyscall_end-syscall
