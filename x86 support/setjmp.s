//
//  setjmp.s
//  x86 support (i386)
//
//  cdecl: jmp_buf pointer at 4(%esp).
//

.text

.global setjmp
.type setjmp, @function

setjmp:
	movl 4(%esp), %eax
	movl %ebx, 0(%eax)
	movl %esi, 4(%eax)
	movl %edi, 8(%eax)
	movl %ebp, 12(%eax)
	lea 4(%esp), %ecx
	movl %ecx, 16(%eax)
	movl (%esp), %ecx
	movl %ecx, 20(%eax)
	xorl %eax, %eax
	ret
