.text

.global __davecc_capture_regs
.type __davecc_capture_regs, @function
__davecc_capture_regs:
	str x30, [x0, #0]
	mov x3, sp
	str x3, [x0, #8]
	str x29, [x0, #16]
	ret

.global __davecc_jump_to_landing_pad
.type __davecc_jump_to_landing_pad, @function
__davecc_jump_to_landing_pad:
	mov sp, x1
	mov x29, x2
	br x0

.global __davecc_unwind_install_context
.type __davecc_unwind_install_context, @function
__davecc_unwind_install_context:
	mov sp, x1
	mov x29, x2
	mov x5, x0
	mov x0, x3
	mov x1, x4
	br x5
