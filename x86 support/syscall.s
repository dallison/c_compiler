//
//  syscall.s
//  x86 support
//
//  Guest syscalls for ELF32 i386 programs running in the x86_64 interpreter.
//  The interpreter treats int $0x80 as a DaveCC guest syscall:
//  eax=number, ebx, ecx, edx, esi, edi, ebp = arguments.
//

.text

.global syscall
.type syscall, @function

// long syscall(int n, ...);
// cdecl: n at 4(%esp), a0 at 8(%esp), ...
syscall:
	pushl %ebx
	pushl %esi
	pushl %edi
	pushl %ebp
	movl 20(%esp), %eax
	movl 24(%esp), %ebx
	movl 28(%esp), %ecx
	movl 32(%esp), %edx
	movl 36(%esp), %esi
	movl 40(%esp), %edi
	movl 44(%esp), %ebp
	.byte 0xcd, 0x80
	popl %ebp
	popl %edi
	popl %esi
	popl %ebx
	ret
