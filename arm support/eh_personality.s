.text

.global __davecc_arm_personality
.type __davecc_arm_personality, @function
__davecc_arm_personality:
	mov r0, #8
	bx lr
