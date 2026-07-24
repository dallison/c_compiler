#include <unwind.h>

#include <stddef.h>
#include <lsda.h>

#include "eh_cxa_internal.h"

extern void __davecc_eh_set_landing_pad_state(_Unwind_Exception* exc,
                                              long selector, long type_offset);

#if defined(__arm__) || defined(__risc_v__)
#define DAVECC_PERSONALITY_TLS __thread
#else
#define DAVECC_PERSONALITY_TLS
#endif

static DAVECC_PERSONALITY_TLS uintptr_t resume_scope_start;
static DAVECC_PERSONALITY_TLS uintptr_t resume_scope_end;

static const struct type_info* ThrownTypeInfo(_Unwind_Exception* exc) {
  if (exc == 0) {
    return 0;
  }
  struct __cxa_exception* header =
      (struct __cxa_exception*)((char*)exc - DAVECC_CXA_UNWIND_OFFSET);
  return header->exceptionType;
}

_Unwind_Reason_Code __gxx_personality_v0(int version, _Unwind_Action actions,
                                        _Unwind_Exception_Class exception_class,
                                        _Unwind_Exception* exc,
                                        _Unwind_Context* ctx) {
  const uint8_t* lsda;
  uintptr_t func_start;
  uintptr_t pc;
  DaveLSDAAction action;
  int search_phase;
  int handler_frame;
  uintptr_t scope_start;
  uintptr_t scope_end;
  DaveLSDAQuery query;

  (void)version;
  (void)exception_class;

  lsda = (const uint8_t*)_Unwind_GetLanguageSpecificData(ctx);
  func_start = _Unwind_GetRegionStart(ctx);
  pc = _Unwind_GetIP(ctx);
  if (lsda == 0 || func_start == 0) {
    return _URC_CONTINUE_UNWIND;
  }

  search_phase = (actions & _UA_SEARCH_PHASE) != 0;
  handler_frame = (actions & _UA_HANDLER_FRAME) != 0;
  if (pc != 0) {
    pc -= 1;
  }

  scope_start = pc;
  scope_end = pc;
  if (!search_phase && resume_scope_start != 0 &&
      pc >= resume_scope_start && pc <= resume_scope_end) {
    scope_start = resume_scope_start;
    scope_end = resume_scope_end;
  }
  query.pc = pc;
  query.scope_start = scope_start;
  query.scope_end = scope_end;
  query.thrown = ThrownTypeInfo(exc);
  query.search_phase = search_phase;
  query.handler_frame = handler_frame;
  if (!DaveLSDAFindAction(lsda, func_start, &query, &action)) {
    if (!search_phase) {
      resume_scope_start = 0;
      resume_scope_end = 0;
    }
    return _URC_CONTINUE_UNWIND;
  }

  if (search_phase) {
    if (action.is_catch) {
      return _URC_HANDLER_FOUND;
    }
    return _URC_CONTINUE_UNWIND;
  }

  if (actions & _UA_CLEANUP_PHASE) {
    if (action.is_cleanup || (action.is_catch && handler_frame)) {
      void* adjusted = exc;
      if (action.is_catch) {
        struct __cxa_exception* header =
            (struct __cxa_exception*)((char*)exc -
                                      DAVECC_CXA_UNWIND_OFFSET);
        adjusted = header->adjustedPtr;
        if (adjusted != 0 && action.type_offset != 0) {
          adjusted = (char*)adjusted + action.type_offset;
        }
        header->adjustedPtr = adjusted;
      }
      _Unwind_SetGR(ctx, 0, (_Unwind_Word)exc);
      _Unwind_SetGR(ctx, 1, (_Unwind_Word)action.selector);
      _Unwind_SetIP(ctx, action.landing_pad);
      DaveUnwindSetInstalledCleanup(ctx, action.is_cleanup);
      if (action.is_cleanup) {
        resume_scope_start = action.try_start;
        resume_scope_end = action.try_end;
      } else {
        resume_scope_start = 0;
        resume_scope_end = 0;
      }
      __davecc_eh_set_landing_pad_state(exc, action.selector,
                                          action.type_offset);
      return _URC_INSTALL_CONTEXT;
    }
  }

  return _URC_CONTINUE_UNWIND;
}
