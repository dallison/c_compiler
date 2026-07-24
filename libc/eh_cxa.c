#include "eh_cxa_internal.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unwind.h>

#include <eh_frame.h>

extern void __davecc_capture_regs(DaveEHFrameRegisters* regs);
extern void DaveUnwindSetContext(uintptr_t pc, uintptr_t rsp, uintptr_t rbp);

#if defined(__arm__) || defined(__risc_v__)
#define DAVECC_EH_TLS __thread
#else
#define DAVECC_EH_TLS
#endif

static DAVECC_EH_TLS struct __cxa_eh_globals eh_thread_globals;
static DAVECC_EH_TLS int eh_globals_initialized;

static DAVECC_EH_TLS struct __cxa_exception* active_thrown_header;
static DAVECC_EH_TLS void* active_adjusted_ptr;
static DAVECC_EH_TLS const DaveCXXTypeInfo* active_legacy_typeinfo;
static DAVECC_EH_TLS _Unwind_Exception* landing_pad_exc;
static DAVECC_EH_TLS long landing_pad_selector;
static DAVECC_EH_TLS long landing_pad_type_offset;

static void InitGlobals(struct __cxa_eh_globals* globals) {
  globals->caughtExceptions = NULL;
  globals->uncaughtExceptions = 0;
  globals->nextPropagatingException = NULL;
  globals->unexpectedHandler = NULL;
  globals->terminateHandler = NULL;
}

static struct __cxa_eh_globals* GetGlobalsSlow(void) {
  if (!eh_globals_initialized) {
    InitGlobals(&eh_thread_globals);
    eh_globals_initialized = 1;
  }
  return &eh_thread_globals;
}

struct __cxa_eh_globals* __davecc_eh_get_globals(void) {
  return GetGlobalsSlow();
}

struct __cxa_eh_globals* __cxa_get_globals(void) {
  return GetGlobalsSlow();
}

struct __cxa_eh_globals* __cxa_get_globals_fast(void) {
  return &eh_thread_globals;
}

struct __cxa_exception* __davecc_eh_header_from_object(void* thrown_object) {
  if (thrown_object == NULL) {
    return NULL;
  }
  return (struct __cxa_exception*)((char*)thrown_object -
                                   sizeof(struct __cxa_exception));
}

void* __davecc_eh_object_from_header(struct __cxa_exception* header) {
  if (header == NULL) {
    return NULL;
  }
  return (char*)header + sizeof(struct __cxa_exception);
}

static void DestroyHeaderIfNeeded(struct __cxa_exception* header) {
  if (header == NULL) {
    return;
  }
  void* object = __davecc_eh_object_from_header(header);
  if (header->exceptionDestructor != NULL) {
    header->exceptionDestructor(object);
  }
  if (header->unwindHeader.exception_cleanup != NULL) {
    header->unwindHeader.exception_cleanup(_URC_FOREIGN_EXCEPTION_CAUGHT,
                                           &header->unwindHeader);
  }
  free(header);
}

static void PopCaughtException(struct __cxa_eh_globals* globals) {
  struct __cxa_exception* caught = globals->caughtExceptions;
  if (caught == NULL) {
    return;
  }
  globals->caughtExceptions = caught->nextException;
  caught->nextException = NULL;
  caught->handlerCount = 0;
}

void* __cxa_allocate_exception(size_t thrown_size) {
  size_t total = thrown_size + sizeof(struct __cxa_exception);
  struct __cxa_exception* header =
      (struct __cxa_exception*)calloc(1, total);
  if (header == NULL) {
    abort();
  }
  return __davecc_eh_object_from_header(header);
}

void __cxa_free_exception(void* thrown_exception) {
  if (thrown_exception == NULL) {
    return;
  }
  struct __cxa_exception* header =
      __davecc_eh_header_from_object(thrown_exception);
  free(header);
}

void __davecc_eh_install_active_exception(struct __cxa_exception* header,
                                          void* adjusted_ptr) {
  struct __cxa_eh_globals* globals = GetGlobalsSlow();
  active_thrown_header = header;
  active_adjusted_ptr = adjusted_ptr;
  active_legacy_typeinfo = NULL;
  globals->uncaughtExceptions++;
}

void __davecc_eh_clear_active_exception(void) {
  active_thrown_header = NULL;
  active_adjusted_ptr = NULL;
  active_legacy_typeinfo = NULL;
  landing_pad_exc = NULL;
  landing_pad_selector = 0;
  landing_pad_type_offset = 0;
}

void __davecc_eh_set_landing_pad_state(_Unwind_Exception* exc, long selector,
                                       long type_offset) {
  landing_pad_exc = exc;
  landing_pad_selector = selector;
  landing_pad_type_offset = type_offset;
}

void* __davecc_eh_landing_pad_exception_object(void) {
  struct __cxa_exception* header;
  void* object;
  if (landing_pad_exc == NULL) {
    return NULL;
  }
  header = (struct __cxa_exception*)((char*)landing_pad_exc -
                                     DAVECC_CXA_UNWIND_OFFSET);
  object = __davecc_eh_object_from_header(header);
  if (object != NULL && landing_pad_type_offset != 0) {
    object = (char*)object + landing_pad_type_offset;
  }
  return object;
}

_Unwind_Exception* __davecc_eh_landing_pad_unwind_header(void) {
  return landing_pad_exc;
}

long __davecc_eh_landing_pad_selector(void) {
  return landing_pad_selector;
}


void __davecc_eh_sync_legacy_current_exception(void* object,
                                               const DaveCXXTypeInfo* typeinfo) {
  active_adjusted_ptr = object;
  active_legacy_typeinfo = typeinfo;
}

void __davecc_eh_read_legacy_current_exception(void** object,
                                               const DaveCXXTypeInfo** typeinfo) {
  if (object != NULL) {
    *object = active_adjusted_ptr;
  }
  if (typeinfo != NULL) {
    *typeinfo = active_legacy_typeinfo;
  }
}

extern void __davecc_eh_unwind_from_throw(void);

static void DaveCCTerminateFromThrow(void) {
  struct __cxa_eh_globals* globals = GetGlobalsSlow();
  if (globals->terminateHandler != NULL) {
    globals->terminateHandler();
  }
  abort();
}

static void BeginUnwindFromThrowSite(struct __cxa_exception* header) {
  _Unwind_Reason_Code reason;
  DaveEHFrameRegisters regs;

  if (header == NULL) {
    abort();
  }
  __davecc_capture_regs(&regs);
  DaveUnwindSetContext(regs.pc, regs.rsp, regs.rbp);
  reason = _Unwind_RaiseException(&header->unwindHeader);
  if (reason == _URC_END_OF_STACK) {
    DaveCCTerminateFromThrow();
  }
  abort();
}

void __davecc_eh_unwind_from_throw(void) {
  BeginUnwindFromThrowSite(active_thrown_header);
}

void __cxa_throw(void* thrown_exception, struct type_info* tinfo,
                 void (*dest)(void*)) {
  _Unwind_Reason_Code reason;
  DaveEHFrameRegisters throw_site_regs;
  __davecc_capture_regs(&throw_site_regs);

  struct __cxa_exception* header =
      __davecc_eh_header_from_object(thrown_exception);
  header->exceptionType = tinfo;
  header->exceptionDestructor = dest;
  header->adjustedPtr = thrown_exception;
  header->handlerCount = 0;
  header->nextException = NULL;
  header->unwindHeader.exception_class = DAVECC_EH_EXCEPTION_CLASS;
  header->unwindHeader.exception_cleanup = NULL;
  header->unwindHeader.private_1 = 0;
  header->unwindHeader.private_2 = 0;

  struct __cxa_eh_globals* globals = GetGlobalsSlow();
  header->unexpectedHandler = globals->unexpectedHandler;
  header->terminateHandler = globals->terminateHandler;

  __davecc_eh_install_active_exception(header, thrown_exception);
  DaveUnwindSetContext(throw_site_regs.pc, throw_site_regs.rsp,
                       throw_site_regs.rbp);
  reason = _Unwind_RaiseException(&header->unwindHeader);
  if (reason == _URC_END_OF_STACK) {
    DaveCCTerminateFromThrow();
  }
  abort();
}

void* __cxa_begin_catch(void* exceptionObject) {
  struct __cxa_eh_globals* globals = GetGlobalsSlow();
  struct __cxa_exception* header;
  if (exceptionObject == NULL) {
    return exceptionObject;
  }
  // The Itanium landing-pad ABI passes the embedded _Unwind_Exception, not
  // the user object, to __cxa_begin_catch.
  header = (struct __cxa_exception*)((char*)exceptionObject -
                                     DAVECC_CXA_UNWIND_OFFSET);

  if (header->handlerCount == 0) {
    if (globals->uncaughtExceptions > 0) {
      globals->uncaughtExceptions--;
    }
    header->nextException = globals->caughtExceptions;
    globals->caughtExceptions = header;
  }
  header->handlerCount++;
  if (header->adjustedPtr == NULL) {
    header->adjustedPtr = __davecc_eh_object_from_header(header);
  }
  if (active_thrown_header == header) {
    active_thrown_header = NULL;
  }
  active_adjusted_ptr = header->adjustedPtr;
  return header->adjustedPtr;
}

void __cxa_end_catch(void) {
  struct __cxa_eh_globals* globals = GetGlobalsSlow();
  struct __cxa_exception* header = globals->caughtExceptions;
  if (header == NULL) {
    return;
  }

  header->handlerCount--;
  if (header->handlerCount > 0) {
    return;
  }

  PopCaughtException(globals);
  if (landing_pad_exc == &header->unwindHeader) {
    landing_pad_exc = NULL;
    landing_pad_selector = 0;
    landing_pad_type_offset = 0;
  }
  DestroyHeaderIfNeeded(header);
  if (globals->caughtExceptions != NULL) {
    active_adjusted_ptr =
        __davecc_eh_object_from_header(globals->caughtExceptions);
  } else {
    active_adjusted_ptr = NULL;
  }
}

void __cxa_rethrow(void) {
  _Unwind_Reason_Code reason;
  DaveEHFrameRegisters throw_site_regs;
  struct __cxa_eh_globals* globals = GetGlobalsSlow();
  struct __cxa_exception* header = globals->caughtExceptions;
  if (header == NULL) {
    abort();
  }

  __davecc_capture_regs(&throw_site_regs);
  void* object = __davecc_eh_object_from_header(header);
  header->handlerCount--;
  if (header->handlerCount == 0) {
    PopCaughtException(globals);
  }

  __davecc_eh_install_active_exception(header, object);
  DaveUnwindSetContext(throw_site_regs.pc, throw_site_regs.rsp,
                       throw_site_regs.rbp);
  reason = _Unwind_RaiseException(&header->unwindHeader);
  if (reason == _URC_END_OF_STACK) {
    DaveCCTerminateFromThrow();
  }
  abort();
}

void _Unwind_DeleteException(_Unwind_Exception* exc) {
  if (exc == NULL) {
    return;
  }
  struct __cxa_exception* header =
      (struct __cxa_exception*)((char*)exc - DAVECC_CXA_UNWIND_OFFSET);
  DestroyHeaderIfNeeded(header);
}

void __davecc_eh_enter_catch_from_unwinder(long base_offset) {
  struct __cxa_eh_globals* globals = GetGlobalsSlow();
  if (active_thrown_header != NULL) {
    void* object = active_adjusted_ptr;
    if (object != NULL && base_offset != 0) {
      object = (char*)object + base_offset;
      active_adjusted_ptr = object;
      active_thrown_header->adjustedPtr = object;
    }
    if (active_thrown_header->handlerCount == 0) {
      if (globals->uncaughtExceptions > 0) {
        globals->uncaughtExceptions--;
      }
      active_thrown_header->nextException = globals->caughtExceptions;
      globals->caughtExceptions = active_thrown_header;
    }
    active_thrown_header->handlerCount++;
    active_thrown_header = NULL;
    return;
  }

  if (active_adjusted_ptr != NULL && base_offset != 0) {
    active_adjusted_ptr = (char*)active_adjusted_ptr + base_offset;
  }
}

void* __davecc_eh_current_adjusted_ptr(void) {
  return active_adjusted_ptr;
}

const struct __cxa_exception* __davecc_eh_current_caught_header(void) {
  struct __cxa_eh_globals* globals = GetGlobalsSlow();
  return globals->caughtExceptions;
}

unsigned int __davecc_eh_uncaught_exceptions(void) {
  struct __cxa_eh_globals* globals = GetGlobalsSlow();
  return globals->uncaughtExceptions;
}
