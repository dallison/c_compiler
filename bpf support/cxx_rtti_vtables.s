//
//  cxx_rtti_vtables.s
//  bpf support
//
//  Itanium ABI vtables for std::type_info class kinds.  Emitted type_info
//  objects point at the address point (offset 16).  Function slots are unused
//  until typeid/dynamic_cast grow real virtuals; the symbols still have to
//  exist so class vtables can link.
//

.data
.p2align 3

.weak _ZTVN10__cxxabiv117__class_type_infoE
.type _ZTVN10__cxxabiv117__class_type_infoE, @object
_ZTVN10__cxxabiv117__class_type_infoE:
	.space 64
.size _ZTVN10__cxxabiv117__class_type_infoE, 64

.weak _ZTVN10__cxxabiv120__si_class_type_infoE
.type _ZTVN10__cxxabiv120__si_class_type_infoE, @object
_ZTVN10__cxxabiv120__si_class_type_infoE:
	.space 64
.size _ZTVN10__cxxabiv120__si_class_type_infoE, 64

.weak _ZTVN10__cxxabiv121__vmi_class_type_infoE
.type _ZTVN10__cxxabiv121__vmi_class_type_infoE, @object
_ZTVN10__cxxabiv121__vmi_class_type_infoE:
	.space 64
.size _ZTVN10__cxxabiv121__vmi_class_type_infoE, 64

.global __davecc_itanium_vptr_class
.type __davecc_itanium_vptr_class, @object
__davecc_itanium_vptr_class:
	.8byte _ZTVN10__cxxabiv117__class_type_infoE+16
.size __davecc_itanium_vptr_class, 8

.global __davecc_itanium_vptr_si_class
.type __davecc_itanium_vptr_si_class, @object
__davecc_itanium_vptr_si_class:
	.8byte _ZTVN10__cxxabiv120__si_class_type_infoE+16
.size __davecc_itanium_vptr_si_class, 8

.global __davecc_itanium_vptr_vmi_class
.type __davecc_itanium_vptr_vmi_class, @object
__davecc_itanium_vptr_vmi_class:
	.8byte _ZTVN10__cxxabiv121__vmi_class_type_infoE+16
.size __davecc_itanium_vptr_vmi_class, 8
