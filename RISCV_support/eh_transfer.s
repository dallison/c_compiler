.text

.global __davecc_capture_regs
.type __davecc_capture_regs, @function
__davecc_capture_regs:
	sd ra, 0(a0)
	sd sp, 8(a0)
	sd s0, 16(a0)
	ret

.global __davecc_jump_to_landing_pad
.type __davecc_jump_to_landing_pad, @function
__davecc_jump_to_landing_pad:
	mv sp, a1
	mv s0, a2
	jr a0

.global __davecc_unwind_install_context
.type __davecc_unwind_install_context, @function
__davecc_unwind_install_context:
	mv t0, a0
	mv sp, a1
	mv s0, a2
	mv a0, a3
	mv a1, a4
	jr t0
