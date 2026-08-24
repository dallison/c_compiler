.data
.align 2
.global __davecc_itanium_vptr_class
__davecc_itanium_vptr_class:
	.4byte _ZTVN10__cxxabiv117__class_type_infoE_u2b16
.global __davecc_itanium_vptr_si_class
__davecc_itanium_vptr_si_class:
	.4byte _ZTVN10__cxxabiv120__si_class_type_infoE_u2b16
.global __davecc_itanium_vptr_vmi_class
__davecc_itanium_vptr_vmi_class:
	.4byte _ZTVN10__cxxabiv121__vmi_class_type_infoE_u2b16

.text

.global g_davecc_arm_transfer_vrs
.type g_davecc_arm_transfer_vrs, @object
g_davecc_arm_transfer_vrs:
	.space 64

.global __davecc_capture_regs
.type __davecc_capture_regs, @function
__davecc_capture_regs:
	mov r12, r0
	str r1, [r12, #4]
	str r2, [r12, #8]
	str r3, [r12, #12]
	str r4, [r12, #16]
	str r5, [r12, #20]
	str r6, [r12, #24]
	str r7, [r12, #28]
	str r8, [r12, #32]
	str r9, [r12, #36]
	str r10, [r12, #40]
	str fp, [r12, #44]
	str lr, [r12, #56]
	str sp, [r12, #52]
	str lr, [r12, #60]
	mov r0, #0
	str r0, [r12, #0]
	bx lr

.global __davecc_jump_to_landing_pad
.type __davecc_jump_to_landing_pad, @function
__davecc_jump_to_landing_pad:
	mov sp, r1
	mov fp, r2
	bx r0

.global __davecc_arm_install_from_vrs
.type __davecc_arm_install_from_vrs, @function
__davecc_arm_install_from_vrs:
	mov r12, r0
	ldr r4, [r12, #16]
	ldr r5, [r12, #20]
	ldr r6, [r12, #24]
	ldr r7, [r12, #28]
	ldr r8, [r12, #32]
	ldr r9, [r12, #36]
	ldr r10, [r12, #40]
	ldr r11, [r12, #44]
	mov sp, r1
	ldr lr, [r12, #56]
	ldr r0, [r12, #0]
	ldr r1, [r12, #4]
	ldr r2, [r12, #8]
	ldr r3, [r12, #12]
	ldr pc, [r12, #60]

.global __davecc_unwind_install_context
.type __davecc_unwind_install_context, @function
__davecc_unwind_install_context:
	b __davecc_jump_to_landing_pad
