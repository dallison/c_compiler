.section ".ARM.extab", "a", @progbits
.align 2

.weak __davecc_arm_unwind_fp
.type __davecc_arm_unwind_fp, @object
__davecc_arm_unwind_fp:
	.word 0
	.word 0x019b8480
	.word 0x0000b000
	.align 2

.weak __davecc_arm_unwind_lr
.type __davecc_arm_unwind_lr, @object
__davecc_arm_unwind_lr:
	.word 0
	.word 0x019eb000
	.align 2

.text
