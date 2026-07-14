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
