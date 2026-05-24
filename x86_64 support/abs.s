//
//  abs.s
//  x86_64 support
//

.text

.global abs
.global labs
.global llabs

.type abs, @function
.type labs, @function
.type llabs, @function

// int abs(int j);
abs:
	cmp $0, %edi
	jge abs_pos
	neg %edi
abs_pos:
	mov %edi, %eax
	ret

// long int labs(long int j);
labs:
	cmp $0, %rdi
	jge labs_pos
	neg %rdi
labs_pos:
	mov %rdi, %rax
	ret

// long long int llabs(long long int j);
llabs:
	cmp $0, %rdi
	jge llabs_pos
	neg %rdi
llabs_pos:
	mov %rdi, %rax
	ret
