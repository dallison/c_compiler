//
//  cxx_rtti.c
//  c_compiler
//
//  C++ RTTI runtime: Itanium __dynamic_cast on LP64-class targets and the
//  legacy DaveCC dynamic_cast helper on size-constrained targets.
//

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "eh_cxa_internal.h"

#if defined(__DAVECC_LEGACY_RTTI__)

typedef struct __davecc_type_info __davecc_type_info;

typedef struct __davecc_base_info {
  const __davecc_type_info* __type;
  long __offset;
} __davecc_base_info;

struct __davecc_type_info {
  const char* __name;
  const __davecc_base_info* __bases;
  long __base_count;
};

static void LegacyRttiSearch(const __davecc_type_info* ti,
                             const __davecc_type_info* dst, long offset,
                             long* found_offset, int* count) {
  if (ti == 0) {
    return;
  }
  if (ti == dst) {
    if (*count == 0) {
      *found_offset = offset;
    }
    (*count)++;
    return;
  }
  for (long i = 0; i < ti->__base_count; i++) {
    LegacyRttiSearch(ti->__bases[i].__type, dst,
                     offset + ti->__bases[i].__offset, found_offset, count);
  }
}

void* __davecc_dynamic_cast(void* p, const __davecc_type_info* dst) {
  if (p == 0 || dst == 0) {
    return 0;
  }
  void** vptr = *(void***)p;
  long offset_to_top = (long)(intptr_t)vptr[-2];
  const __davecc_type_info* most = (const __davecc_type_info*)vptr[-1];
  char* top = (char*)p + offset_to_top;
  long found = 0;
  int count = 0;
  LegacyRttiSearch(most, dst, 0, &found, &count);
  if (count == 1) {
    return top + found;
  }
  return 0;
}

#else

typedef struct __class_type_info __class_type_info;

typedef struct __si_class_type_info {
  const void* vptr;
  const char* __type_name;
  const __class_type_info* __base_type;
} __si_class_type_info;

typedef struct __base_class_type_info {
  const __class_type_info* __base_type;
  long __offset_flags;
} __base_class_type_info;

typedef struct __vmi_class_type_info {
  const void* vptr;
  const char* __type_name;
  unsigned int __flags;
  unsigned int __base_count;
} __vmi_class_type_info;

struct __class_type_info {
  const void* vptr;
  const char* __type_name;
};

extern const void* __davecc_itanium_vptr_class;
extern const void* __davecc_itanium_vptr_si_class;
extern const void* __davecc_itanium_vptr_vmi_class;

static const void* ItaniumResolveVptr(const void* vptr) {
  if (vptr == (const void*)&__davecc_itanium_vptr_class) {
    return __davecc_itanium_vptr_class;
  }
  if (vptr == (const void*)&__davecc_itanium_vptr_si_class) {
    return __davecc_itanium_vptr_si_class;
  }
  if (vptr == (const void*)&__davecc_itanium_vptr_vmi_class) {
    return __davecc_itanium_vptr_vmi_class;
  }
  return vptr;
}

static int ItaniumTypeInfoEqual(const __class_type_info* left,
                                const __class_type_info* right) {
  if (left == right) {
    return 1;
  }
  if (left == 0 || right == 0 || left->__type_name == 0 ||
      right->__type_name == 0) {
    return 0;
  }
  return strcmp(left->__type_name, right->__type_name) == 0;
}

static void ItaniumRttiSearch(const __class_type_info* ti,
                              const __class_type_info* dst, long offset,
                              long* found_offset, int* count) {
  if (ti == 0 || dst == 0) {
    return;
  }
  if (ItaniumTypeInfoEqual(ti, dst)) {
    if (*count == 0) {
      *found_offset = offset;
    }
    (*count)++;
    return;
  }

  const void* kind = ItaniumResolveVptr(ti->vptr);
  if (kind == __davecc_itanium_vptr_si_class) {
    const __si_class_type_info* si = (const __si_class_type_info*)ti;
    ItaniumRttiSearch(si->__base_type, dst, offset, found_offset, count);
    return;
  }

  if (kind == __davecc_itanium_vptr_vmi_class) {
    const __vmi_class_type_info* vmi = (const __vmi_class_type_info*)ti;
    const __base_class_type_info* bases =
        (const __base_class_type_info*)((const char*)vmi + 2 * sizeof(void*) +
                                        2 * sizeof(unsigned int));
    for (unsigned int i = 0; i < vmi->__base_count; i++) {
      if (bases[i].__offset_flags & 0x1) {
        continue;
      }
      if (!(bases[i].__offset_flags & 0x2)) {
        continue;
      }
      long base_offset = bases[i].__offset_flags >> 8;
      ItaniumRttiSearch(bases[i].__base_type, dst, offset + base_offset,
                        found_offset, count);
    }
  }
}

void* __dynamic_cast(const void* sub, const __class_type_info* src,
                     const __class_type_info* dst, ptrdiff_t src2dst) {
  (void)src;
  (void)src2dst;
  if (sub == 0 || dst == 0) {
    return 0;
  }

  const void* const* vtable = *(const void* const* const*)sub;
  if (vtable == 0) {
    return 0;
  }

  ptrdiff_t offset_to_top = (ptrdiff_t)(intptr_t)vtable[-2];
  const char* top = (const char*)sub + offset_to_top;
  const __class_type_info* most = (const __class_type_info*)vtable[-1];
  long found = 0;
  int count = 0;
  ItaniumRttiSearch(most, dst, 0, &found, &count);
  if (count == 1) {
    return (void*)(top + found);
  }
  return 0;
}

void* __davecc_dynamic_cast(void* p, const __class_type_info* dst) {
  return __dynamic_cast(p, 0, dst, -1);
}

#endif

extern void __davecc_raise_bad_cast(void);

static void RaiseBadCast(void) {
#if defined(__x86_64__) || defined(__aarch64__) || defined(__arm__) || \
    defined(__risc_v__)
  __davecc_raise_bad_cast();
#else
  abort();
#endif
}

void* __davecc_dynamic_cast_ref(void* p, const void* dst) {
#if !defined(__DAVECC_LEGACY_RTTI__)
  void* result = __dynamic_cast(p, 0, (const __class_type_info*)dst, -1);
#else
  void* result = __davecc_dynamic_cast(p, (const __davecc_type_info*)dst);
#endif
  if (result == 0) {
    RaiseBadCast();
  }
  return result;
}

#if !defined(__DAVECC_LEGACY_RTTI__)
void* __dynamic_cast_ref(const void* sub, const __class_type_info* src,
                         const __class_type_info* dst, ptrdiff_t src2dst) {
  void* result = __dynamic_cast(sub, src, dst, src2dst);
  if (result == 0) {
    RaiseBadCast();
  }
  return result;
}
#endif
