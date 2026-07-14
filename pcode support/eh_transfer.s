.text

.global __davecc_capture_regs
.type __davecc_capture_regs, @function
__davecc_capture_regs:
	ldx r1, [sp, #8]
	ldx r2, [sp, #0]
	stx r2, [r1, #0]
	mov r2, ap
	stx r2, [r1, #8]
	mov r2, fp
	stx r2, [r1, #16]
	ret

.global __davecc_jump_to_landing_pad
.type __davecc_jump_to_landing_pad, @function
__davecc_jump_to_landing_pad:
	ldx r1, [sp, #8]
	ldx r2, [sp, #16]
	ldx r3, [sp, #24]
	mov ap, r2
	mov fp, r3
	rcall r1
