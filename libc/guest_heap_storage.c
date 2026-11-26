//
//  guest_heap_storage.c
//  libc
//
//  Default guest heap arena for HAS_HEAP_LOCK / p-code targets.
//  Kept out of malloc.c so InitializeHeap's references bind through the
//  global symbol (a same-TU weak def would ignore a larger strong override
//  from e.g. DaveOS kernel/heap.cc).
//

#include <syscall.h>

#if defined(__DAVECC_HAS_HEAP_LOCK__) || defined(__p_code__)

// Weak so the DaveOS kernel can provide a larger arena (heap.cc).  Lives in
// the same DSO/image as malloc so PIC GOT slots stay RELATIVE to a mapped
// .bss/COMMON range.
__attribute__((weak)) unsigned long long
    __davecc_guest_heap_storage[((1024 * 1024) + 15) /
                                sizeof(unsigned long long)];
__attribute__((weak)) const unsigned long
    __davecc_guest_heap_storage_size =
        sizeof(__davecc_guest_heap_storage);

#endif
