#include <stdint.h>

#if defined(__arm__)

#include <unwind.h>
#include <lsda.h>
#include <stddef.h>

#include "eh_cxa_internal.h"
#include <eh_arm.h>

extern void __davecc_eh_set_landing_pad_state(_Unwind_Exception* exc,
                                              long selector, long type_offset);

#if defined(__risc_v__)
#define DAVECC_ARM_PERSONALITY_TLS __thread
#else
#define DAVECC_ARM_PERSONALITY_TLS
#endif

static DAVECC_ARM_PERSONALITY_TLS uintptr_t resume_scope_start;
static DAVECC_ARM_PERSONALITY_TLS uintptr_t resume_scope_end;

static int IsCXXExceptionClass(uint64_t exception_class) {
  return exception_class == DAVECC_EH_EXCEPTION_CLASS_GNU ||
         exception_class == DAVECC_EH_EXCEPTION_CLASS_CLANG;
}

static const struct type_info* ThrownTypeInfo(uint64_t exception_class,
                                              _Unwind_Exception* exc) {
  if (exc == 0 || !IsCXXExceptionClass(exception_class)) {
    return 0;
  }
  struct __cxa_exception* header =
      (struct __cxa_exception*)((char*)exc - DAVECC_CXA_UNWIND_OFFSET);
  return header->exceptionType;
}

static int RunLSDAPersonality(int state, _Unwind_Exception* ucb,
                                              _Unwind_Context* context,
                                              int foreign_exception) {
  const uint8_t* lsda;
  uintptr_t func_start;
  uintptr_t pc;
  DaveLSDAAction action;
  int search_phase;
  int handler_frame;
  uintptr_t scope_start;
  uintptr_t scope_end;
  DaveLSDAQuery query;
  uint64_t exception_class;

  (void)foreign_exception;
  lsda = (const uint8_t*)context->lsda;
  func_start = DaveARMCanonicalGuestPC(context->fnstart);
  pc = DaveARMCanonicalGuestPC(context->vrs[DAVE_ARM_R_PC]);
  if (lsda == 0 || func_start == 0) {
    return _URC_CONTINUE_UNWIND;
  }

  search_phase = state == _US_VIRTUAL_UNWIND_FRAME;
  handler_frame = state == _US_UNWIND_FRAME_STARTING;
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

  exception_class = DaveARMExceptionClass(ucb);
  query.pc = pc;
  query.scope_start = scope_start;
  query.scope_end = scope_end;
  query.thrown = ThrownTypeInfo(exception_class, ucb);
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

  if (action.is_cleanup || (action.is_catch && handler_frame)) {
    void* adjusted = ucb;
    if (action.is_catch && IsCXXExceptionClass(exception_class)) {
      struct __cxa_exception* header =
          (struct __cxa_exception*)((char*)ucb - DAVECC_CXA_UNWIND_OFFSET);
      adjusted = header->adjustedPtr;
      if (adjusted != 0 && action.type_offset != 0) {
        adjusted = (char*)adjusted + action.type_offset;
      }
      header->adjustedPtr = adjusted;
    }
    context->vrs[0] = (uint32_t)(uintptr_t)ucb;
    context->vrs[1] = (uint32_t)action.selector;
    uintptr_t base = DaveARMCanonicalGuestPC(func_start);
    context->vrs[DAVE_ARM_R_PC] =
        (uint32_t)(base + (action.landing_pad - func_start));
    context->installed_cleanup = action.is_cleanup;
    if (action.is_cleanup) {
      resume_scope_start = action.try_start;
      resume_scope_end = action.try_end;
    } else {
      resume_scope_start = 0;
      resume_scope_end = 0;
    }
    __davecc_eh_set_landing_pad_state(ucb, action.selector, action.type_offset);
    return _URC_INSTALL_CONTEXT;
  }

  return _URC_CONTINUE_UNWIND;
}

int __aeabi_unwind_cpp_pr0(int state, _Unwind_Exception* ucb,
                           _Unwind_Context* context) {
  (void)ucb;
  (void)context;
  (void)state;
  return _URC_CONTINUE_UNWIND;
}

int __aeabi_unwind_cpp_pr1(int state, _Unwind_Exception* ucb,
                           _Unwind_Context* context) {
  return RunLSDAPersonality(state, ucb, context, 0);
}

int __aeabi_unwind_cpp_pr2(int state, _Unwind_Exception* ucb,
                           _Unwind_Context* context) {
  return RunLSDAPersonality(state, ucb, context, 1);
}

int __gxx_personality_v0(int state, _Unwind_Exception* ucb,
                         _Unwind_Context* context) {
  return RunLSDAPersonality(state, ucb, context, 0);
}

#endif /* __arm__ */
