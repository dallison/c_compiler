//
//  cxx_rtti.c
//  c_compiler
//
//  C++ RTTI runtime: the dynamic_cast helper.  The type_info layout below must
//  match the objects emitted by the compiler (c_compiler/frontend/semantic/
//  rtti.c) and the <typeinfo> header.
//
//  Supported: single inheritance and non-virtual multiple inheritance downcasts
//  and sidecasts.  Virtual bases are not handled (their base_info entries are
//  omitted by the compiler).
//

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct __davecc_type_info __davecc_type_info;

// __davecc_base_info is defined complete before __davecc_type_info so that the
// `__bases` array stride is known where it is indexed (the compiler must see the
// element size, not an incomplete forward declaration).
typedef struct __davecc_base_info {
  const __davecc_type_info* __type;
  long __offset;
} __davecc_base_info;

struct __davecc_type_info {
  const char* __name;
  const __davecc_base_info* __bases;
  long __base_count;
};

// Walks the (non-virtual) base graph of `ti`, accumulating the byte offset from
// the most-derived object.  Records the offset of the first `dst` subobject
// found and counts how many distinct ones exist so the caller can reject
// ambiguous matches.
static void RttiSearch(const __davecc_type_info* ti,
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
    RttiSearch(ti->__bases[i].__type, dst, offset + ti->__bases[i].__offset,
               found_offset, count);
  }
}

// Pointer form of dynamic_cast.  Returns the adjusted pointer to the `dst`
// subobject of *p's most-derived object, or 0 on failure (including ambiguous
// base subobjects and a null input).
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
  RttiSearch(most, dst, 0, &found, &count);
  if (count == 1) {
    return top + found;
  }
  return 0;
}

#if defined(__x86_64__)
// EH is only wired up on x86_64; elsewhere a failed reference cast terminates.
// This DaveTypeInfo layout must match libc/eh_throw.c and the compiler's
// exception type_info emission.
typedef struct __davecc_eh_type_info {
  const char* name;
  long base_count;
  const void* bases;
} __davecc_eh_type_info;
void __davecc_throw(intptr_t exception_object,
                    const __davecc_eh_type_info* typeinfo);
static char __davecc_bad_cast_object;
// Must match the compiler's exception type name for std::bad_cast.
static const char __davecc_bad_cast_name[] = "struct bad_cast";
static const __davecc_eh_type_info __davecc_bad_cast_typeinfo = {
    __davecc_bad_cast_name, 0, 0};
#endif

// Reference form of dynamic_cast: like the pointer form but throws
// std::bad_cast (where exceptions are supported) instead of returning 0.
void* __davecc_dynamic_cast_ref(void* p, const __davecc_type_info* dst) {
  void* result = __davecc_dynamic_cast(p, dst);
  if (result == 0) {
#if defined(__x86_64__)
    __davecc_throw((intptr_t)&__davecc_bad_cast_object,
                   &__davecc_bad_cast_typeinfo);
#else
    abort();
#endif
  }
  return result;
}
