//
//  eh_transfer.s
//  x86 support (i386)
//
//  cdecl: arguments on the stack.  DaveEHFrameRegisters is pc, rsp, rbp
//  followed by gr[] indexed by DWARF register number, 4 bytes each.
//

.text

.global __davecc_capture_regs
.type __davecc_capture_regs, @function
__davecc_capture_regs:
	movl 4(%esp), %eax
	movl (%esp), %ecx
	movl %ecx, 0(%eax)
	lea 4(%esp), %ecx
	movl %ecx, 4(%eax)
	movl %ebp, 8(%eax)
	movl %ebx, 24(%eax)
	movl %esi, 36(%eax)
	movl %edi, 40(%eax)
	ret

.global __davecc_jump_to_landing_pad
.type __davecc_jump_to_landing_pad, @function
__davecc_jump_to_landing_pad:
	movl 4(%esp), %ecx
	movl 8(%esp), %edx
	movl 12(%esp), %ebp
	movl %edx, %esp
	jmp *%ecx

.global __davecc_unwind_install_context
.type __davecc_unwind_install_context, @function
__davecc_unwind_install_context:
	movl 16(%esp), %eax
	movl 20(%esp), %edx
	movl 4(%esp), %ecx
	movl 12(%esp), %ebp
	movl 8(%esp), %esp
	jmp *%ecx
