#ifndef DAVECC_UNWIND_H
#define DAVECC_UNWIND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Unwind_Context _Unwind_Context;
typedef struct _Unwind_Exception _Unwind_Exception;

typedef enum {
  _URC_NO_REASON = 0,
  _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
  _URC_FATAL_PHASE2_ERROR = 2,
  _URC_FATAL_PHASE1_ERROR = 3,
  _URC_NORMAL_STOP = 4,
  _URC_END_OF_STACK = 5,
  _URC_HANDLER_FOUND = 6,
  _URC_INSTALL_CONTEXT = 7,
  _URC_CONTINUE_UNWIND = 8
} _Unwind_Reason_Code;

typedef enum {
  _UA_SEARCH_PHASE = 1,
  _UA_CLEANUP_PHASE = 2,
  _UA_HANDLER_FRAME = 4,
  _UA_FORCE_UNWIND = 8
} _Unwind_Action;

typedef _Unwind_Reason_Code (*_Unwind_Personality_Fn)(int version,
                                                      _Unwind_Action actions,
                                                      uint64_t exception_class,
                                                      _Unwind_Exception* exc,
                                                      _Unwind_Context* ctx);

struct _Unwind_Exception {
  uint64_t exception_class;
  void (*exception_cleanup)(_Unwind_Reason_Code reason,
                            struct _Unwind_Exception* exc);
  uintptr_t private_1;
  uintptr_t private_2;
};

_Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Exception* exc);
void _Unwind_Resume(_Unwind_Exception* exc);
uintptr_t _Unwind_GetIP(_Unwind_Context* ctx);
void _Unwind_SetIP(_Unwind_Context* ctx, uintptr_t ip);
uintptr_t _Unwind_GetLanguageSpecificData(_Unwind_Context* ctx);
uintptr_t _Unwind_GetRegionStart(_Unwind_Context* ctx);

_Unwind_Reason_Code __gxx_personality_v0(int version, _Unwind_Action actions,
                                        uint64_t exception_class,
                                        _Unwind_Exception* exc,
                                        _Unwind_Context* ctx);

#ifdef __cplusplus
}
#endif

#endif /* DAVECC_UNWIND_H */
