.text

.global __davecc_capture_regs
.type __davecc_capture_regs, @function
__davecc_capture_regs:
	str lr, [r0, #0]
	str sp, [r0, #4]
	str fp, [r0, #8]
	bx lr

.global __davecc_jump_to_landing_pad
.type __davecc_jump_to_landing_pad, @function
__davecc_jump_to_landing_pad:
	mov sp, r1
	mov fp, r2
	bx r0

.global __davecc_unwind_install_context
.type __davecc_unwind_install_context, @function
__davecc_unwind_install_context:
	ldr ip, [sp]
	mov fp, r2
	mov sp, r1
	mov r1, ip
	mov ip, r0
	mov r0, r3
	bx ip
