.data
.align 3
.weak _ZTVN10__cxxabiv117__class_type_infoE
_ZTVN10__cxxabiv117__class_type_infoE:
	.space 64
.weak _ZTVN10__cxxabiv120__si_class_type_infoE
_ZTVN10__cxxabiv120__si_class_type_infoE:
	.space 64
.weak _ZTVN10__cxxabiv121__vmi_class_type_infoE
_ZTVN10__cxxabiv121__vmi_class_type_infoE:
	.space 64
.global __davecc_itanium_vptr_class
__davecc_itanium_vptr_class:
	.8byte _ZTVN10__cxxabiv117__class_type_infoE+16
.global __davecc_itanium_vptr_si_class
__davecc_itanium_vptr_si_class:
	.8byte _ZTVN10__cxxabiv120__si_class_type_infoE+16
.global __davecc_itanium_vptr_vmi_class
__davecc_itanium_vptr_vmi_class:
	.8byte _ZTVN10__cxxabiv121__vmi_class_type_infoE+16

.text

.global g_davecc_unwind_transfer_regs

.global __davecc_capture_regs
.type __davecc_capture_regs, @function
__davecc_capture_regs:
	str x30, [x0, #0]
	mov x3, sp
	str x3, [x0, #8]
	str x29, [x0, #16]
	stp x19, x20, [x0, #176]
	stp x21, x22, [x0, #192]
	stp x23, x24, [x0, #208]
	stp x25, x26, [x0, #224]
	stp x27, x28, [x0, #240]
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
	adrp x9, g_davecc_unwind_transfer_regs
	add x9, x9, :lo12:g_davecc_unwind_transfer_regs
	ldp x19, x20, [x9, #176]
	ldp x21, x22, [x9, #192]
	ldp x23, x24, [x9, #208]
	ldp x25, x26, [x9, #224]
	ldp x27, x28, [x9, #240]
	mov x0, x3
	mov x1, x4
	br x5
