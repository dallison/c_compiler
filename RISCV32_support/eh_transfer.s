.data
.align 2
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
	.4byte _ZTVN10__cxxabiv117__class_type_infoE+16
.global __davecc_itanium_vptr_si_class
__davecc_itanium_vptr_si_class:
	.4byte _ZTVN10__cxxabiv120__si_class_type_infoE+16
.global __davecc_itanium_vptr_vmi_class
__davecc_itanium_vptr_vmi_class:
	.4byte _ZTVN10__cxxabiv121__vmi_class_type_infoE+16

.text

.global g_davecc_unwind_transfer_regs

.global __davecc_capture_regs
.type __davecc_capture_regs, @function
__davecc_capture_regs:
	sw ra, 0(a0)
	sw sp, 4(a0)
	sw s0, 8(a0)
	sw s1, 48(a0)
	sw s2, 84(a0)
	sw s3, 88(a0)
	sw s4, 92(a0)
	sw s5, 96(a0)
	sw s6, 100(a0)
	sw s7, 104(a0)
	sw s8, 108(a0)
	sw s9, 112(a0)
	sw s10, 116(a0)
	sw s11, 120(a0)
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
	lw s1, 48(t1)
	lw s2, 84(t1)
	lw s3, 88(t1)
	lw s4, 92(t1)
	lw s5, 96(t1)
	lw s6, 100(t1)
	lw s7, 104(t1)
	lw s8, 108(t1)
	lw s9, 112(t1)
	lw s10, 116(t1)
	lw s11, 120(t1)
	mv t0, a0
	mv sp, a1
	mv s0, a2
	mv a0, a3
	mv a1, a4
	jr t0
