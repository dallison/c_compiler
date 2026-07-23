#ifndef DAVECC_EH_CXA_INTERNAL_H
#define DAVECC_EH_CXA_INTERNAL_H

#include <stdint.h>
#include <stddef.h>

#include "cxxabi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DAVECC_EH_EXCEPTION_CLASS 0x434C4E47432B2B00ULL /* "CLNGC++\0" */

typedef struct DaveCXXTypeInfoBase {
  const char* name;
  long offset;
} DaveCXXTypeInfoBase;

typedef struct DaveCXXTypeInfo {
  const char* name;
  long base_count;
  const DaveCXXTypeInfoBase* bases;
  long object_size;
  long object_is_class;
} DaveCXXTypeInfo;

struct __cxa_eh_globals* __davecc_eh_get_globals(void);

struct __cxa_exception* __davecc_eh_header_from_object(void* thrown_object);

void* __davecc_eh_object_from_header(struct __cxa_exception* header);

void __davecc_eh_install_active_exception(struct __cxa_exception* header,
                                          void* adjusted_ptr);

void __davecc_eh_clear_active_exception(void);

void __davecc_eh_unwind_current_exception(void);

void __davecc_eh_enter_catch_from_unwinder(long base_offset);

void __davecc_eh_sync_legacy_current_exception(void* object,
                                               const DaveCXXTypeInfo* typeinfo);

void __davecc_eh_read_legacy_current_exception(void** object,
                                               const DaveCXXTypeInfo** typeinfo);

void* __davecc_eh_current_adjusted_ptr(void);

const struct __cxa_exception* __davecc_eh_current_caught_header(void);

unsigned int __davecc_eh_uncaught_exceptions(void);

#ifdef __cplusplus
}
#endif

#endif /* DAVECC_EH_CXA_INTERNAL_H */
