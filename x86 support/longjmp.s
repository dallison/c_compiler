//
//  longjmp.s
//  x86 support (i386)
//
//  cdecl: jmp_buf at 4(%esp), value at 8(%esp).
//

.text

.global longjmp
.type longjmp, @function

longjmp:
	movl 4(%esp), %edx
	movl 8(%esp), %eax
	testl %eax, %eax
	jnz .Llongjmp_value_ready
	movl $1, %eax
.Llongjmp_value_ready:
	movl 20(%edx), %ecx
	movl 0(%edx), %ebx
	movl 4(%edx), %esi
	movl 8(%edx), %edi
	movl 12(%edx), %ebp
	movl 16(%edx), %esp
	jmp *%ecx
