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
	sd ra, 0(a0)
	sd sp, 8(a0)
	sd s0, 16(a0)
	sd s1, 96(a0)
	sd s2, 168(a0)
	sd s3, 176(a0)
	sd s4, 184(a0)
	sd s5, 192(a0)
	sd s6, 200(a0)
	sd s7, 208(a0)
	sd s8, 216(a0)
	sd s9, 224(a0)
	sd s10, 232(a0)
	sd s11, 240(a0)
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
	la t1, g_davecc_unwind_transfer_regs
	ld s1, 96(t1)
	ld s2, 168(t1)
	ld s3, 176(t1)
	ld s4, 184(t1)
	ld s5, 192(t1)
	ld s6, 200(t1)
	ld s7, 208(t1)
	ld s8, 216(t1)
	ld s9, 224(t1)
	ld s10, 232(t1)
	ld s11, 240(t1)
	mv t0, a0
	mv sp, a1
	mv s0, a2
	mv a0, a3
	mv a1, a4
	jr t0
