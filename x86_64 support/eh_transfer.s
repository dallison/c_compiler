//
//  eh_transfer.s
//  x86_64 support
//

.data
.align 3
.weak _ZTVN10__cxxabiv117__class_type_infoE
_ZTVN10__cxxabiv117__class_type_infoE:
	.space 64
.weak _ZTVN10__cxxabiv120__si_class_type_infoE
_ZTVN10__cxxabiv120__si_class_type_infoE:
	.space 64
.weak _ZTVN10__cxxabiv121__vmi_class_type_infoE
_ZTVN10__cxxabiv121__vmi_class_type_infoE:
	.space 64
.global __davecc_itanium_vptr_class
__davecc_itanium_vptr_class:
	.8byte _ZTVN10__cxxabiv117__class_type_infoE+16
.global __davecc_itanium_vptr_si_class
__davecc_itanium_vptr_si_class:
	.8byte _ZTVN10__cxxabiv120__si_class_type_infoE+16
.global __davecc_itanium_vptr_vmi_class
__davecc_itanium_vptr_vmi_class:
	.8byte _ZTVN10__cxxabiv121__vmi_class_type_infoE+16

.text

.global g_davecc_unwind_transfer_regs

.global __davecc_capture_regs
.type __davecc_capture_regs, @function
__davecc_capture_regs:
	mov (%rsp), %rax
	mov %rax, 0(%rdi)
	lea 8(%rsp), %rax
	mov %rax, 8(%rdi)
	mov %rbp, 16(%rdi)
	mov %rbx, 48(%rdi)
	mov %r12, 120(%rdi)
	mov %r13, 128(%rdi)
	mov %r14, 136(%rdi)
	mov %r15, 144(%rdi)
	ret

.global __davecc_jump_to_landing_pad
.type __davecc_jump_to_landing_pad, @function
__davecc_jump_to_landing_pad:
	mov %rdx, %rbp
	mov %rsi, %rsp
	jmp *%rdi

.global __davecc_unwind_install_context
.type __davecc_unwind_install_context, @function
__davecc_unwind_install_context:
	mov %rdx, %rbp
	mov %rcx, %rax
	mov %r8, %rdx
	mov %rsi, %rsp
	lea g_davecc_unwind_transfer_regs(%rip), %r9
	mov 48(%r9), %rbx
	mov 120(%r9), %r12
	mov 128(%r9), %r13
	mov 136(%r9), %r14
	mov 144(%r9), %r15
	jmp *%rdi
