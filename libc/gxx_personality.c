#include <unwind.h>

#include <lsda.h>

typedef struct CXXTypeInfo CXXTypeInfo;

extern void DaveUnwindSetContext(uintptr_t pc, uintptr_t rsp, uintptr_t rbp);

static const CXXTypeInfo* DaveCurrentExceptionTypeInfo(void);

_Unwind_Reason_Code __gxx_personality_v0(int version, _Unwind_Action actions,
                                        uint64_t exception_class,
                                        _Unwind_Exception* exc,
                                        _Unwind_Context* ctx) {
  const uint8_t* lsda;
  uintptr_t func_start;
  uintptr_t pc;
  DaveLSDAAction action;

  (void)version;
  (void)exception_class;
  (void)exc;

  lsda = (const uint8_t*)_Unwind_GetLanguageSpecificData(ctx);
  func_start = _Unwind_GetRegionStart(ctx);
  pc = _Unwind_GetIP(ctx);
  if (lsda == 0 || func_start == 0) {
    return _URC_CONTINUE_UNWIND;
  }

  if (!DaveLSDAFindAction(lsda, func_start, pc, pc, pc,
                          DaveCurrentExceptionTypeInfo(), &action)) {
    return _URC_CONTINUE_UNWIND;
  }

  if (actions & _UA_SEARCH_PHASE) {
    if (action.is_catch) {
      return _URC_HANDLER_FOUND;
    }
    return _URC_CONTINUE_UNWIND;
  }

  if (actions & _UA_CLEANUP_PHASE) {
    if (action.is_catch || action.is_cleanup) {
      _Unwind_SetIP(ctx, action.landing_pad);
      return _URC_INSTALL_CONTEXT;
    }
  }

  return _URC_CONTINUE_UNWIND;
}
