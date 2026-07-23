.section ".ARM.extab", "a", @progbits
.align 2

.weak __davecc_arm_unwind_fp
.type __davecc_arm_unwind_fp, @object
__davecc_arm_unwind_fp:
	.word 0
	.byte 0x90, 0x0b
	.byte 0x90, 0x0f
	.align 2

.weak __davecc_arm_unwind_lr
.type __davecc_arm_unwind_lr, @object
__davecc_arm_unwind_lr:
	.word 0
	.byte 0x90, 0x0e
	.align 2

.text
