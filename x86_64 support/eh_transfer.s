//
//  eh_transfer.s
//  x86_64 support
//
//  Throw-path helpers for DaveCC zero-cost catch-all exceptions.
//

.text

.global __davecc_capture_regs
.type __davecc_capture_regs, @function

// void __davecc_capture_regs(DaveEHFrameRegisters* regs);
// SysV: regs in %rdi.
__davecc_capture_regs:
	mov (%rsp), %rax
	mov %rax, 0(%rdi)
	lea 8(%rsp), %rax
	mov %rax, 8(%rdi)
	mov %rbp, 16(%rdi)
	ret

.global __davecc_jump_to_landing_pad
.type __davecc_jump_to_landing_pad, @function

// void __davecc_jump_to_landing_pad(uintptr_t target, uintptr_t rsp,
//                                   uintptr_t rbp);
// SysV: target in %rdi, rsp in %rsi, rbp in %rdx.
__davecc_jump_to_landing_pad:
	mov %rdx, %rbp
	mov %rsi, %rsp
	jmp *%rdi
