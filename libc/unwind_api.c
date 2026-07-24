#include <unwind.h>

#include <eh_frame.h>
#include <lsda.h>
#include <stdlib.h>
#include <string.h>

struct _Unwind_Context {
  DaveEHFrameRegisters regs;
  DaveEHFDE fde;
  int have_fde;
  _Unwind_Word gr[2];
  int installed_cleanup;
};

#if defined(__arm__) || defined(__risc_v__)
#define DAVECC_UNWIND_TLS __thread
#else
#define DAVECC_UNWIND_TLS
#endif

static DAVECC_UNWIND_TLS struct _Unwind_Context g_unwind_context;
static DAVECC_UNWIND_TLS struct _Unwind_Context g_resume_context;
static DAVECC_UNWIND_TLS _Unwind_Exception* g_resume_exception;

extern void __davecc_capture_regs(DaveEHFrameRegisters* regs);
extern void __davecc_unwind_install_context(
    uintptr_t target, uintptr_t rsp, uintptr_t rbp,
    _Unwind_Exception* exception, uintptr_t selector);

static struct _Unwind_Context* ContextOrCurrent(_Unwind_Context* ctx) {
  return ctx != 0 ? ctx : &g_unwind_context;
}

uintptr_t _Unwind_GetIP(_Unwind_Context* ctx) {
  return ContextOrCurrent(ctx)->regs.pc;
}

void _Unwind_SetIP(_Unwind_Context* ctx, uintptr_t ip) {
  ContextOrCurrent(ctx)->regs.pc = ip;
}

_Unwind_Word _Unwind_GetGR(_Unwind_Context* ctx, int index) {
  struct _Unwind_Context* current = ContextOrCurrent(ctx);
  if (index < 0 || index >= 2) {
    return 0;
  }
  return current->gr[index];
}

void _Unwind_SetGR(_Unwind_Context* ctx, int index, _Unwind_Word value) {
  struct _Unwind_Context* current = ContextOrCurrent(ctx);
  if (index >= 0 && index < 2) {
    current->gr[index] = value;
  }
}

void DaveUnwindSetInstalledCleanup(_Unwind_Context* ctx, int is_cleanup) {
  ContextOrCurrent(ctx)->installed_cleanup = is_cleanup;
}

uintptr_t _Unwind_GetLanguageSpecificData(_Unwind_Context* ctx) {
  struct _Unwind_Context* current = ContextOrCurrent(ctx);
  if (!current->have_fde || !current->fde.has_lsda) {
    return 0;
  }
  return (uintptr_t)current->fde.lsda;
}

uintptr_t _Unwind_GetRegionStart(_Unwind_Context* ctx) {
  struct _Unwind_Context* current = ContextOrCurrent(ctx);
  if (!current->have_fde) {
    return 0;
  }
  return current->fde.pc_begin;
}

void DaveUnwindSetContext(uintptr_t pc, uintptr_t rsp, uintptr_t rbp) {
  g_unwind_context.regs.pc = pc;
  g_unwind_context.regs.rsp = rsp;
  g_unwind_context.regs.rbp = rbp;
  g_unwind_context.have_fde = DaveEHFrameFindFDE(pc, &g_unwind_context.fde);
}

static int AdvanceContext(struct _Unwind_Context* context) {
  DaveEHFrameWalkResult walk;
  if (!DaveEHFrameWalkFrame(&context->regs, &walk)) {
    context->have_fde = 0;
    return 0;
  }
  context->regs.pc = walk.caller_pc;
  context->regs.rsp = walk.caller_rsp;
  context->regs.rbp = walk.caller_rbp;
  context->have_fde =
      DaveEHFrameFindFDE(context->regs.pc, &context->fde);
  return context->have_fde;
}

void DaveUnwindAdvanceToCaller(void) {
  AdvanceContext(&g_unwind_context);
}

_Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Exception* exc) {
  struct _Unwind_Context search;
  struct _Unwind_Context cleanup;
  uintptr_t handler_pc = 0;
  _Unwind_Reason_Code reason;

  if (exc == 0) {
    return _URC_FATAL_PHASE1_ERROR;
  }

  memset(&search, 0, sizeof(search));
  if (g_unwind_context.regs.pc != 0) {
    search.regs = g_unwind_context.regs;
    search.have_fde = g_unwind_context.have_fde;
    search.fde = g_unwind_context.fde;
  } else {
    __davecc_capture_regs(&search.regs);
    search.have_fde = DaveEHFrameFindFDE(search.regs.pc, &search.fde);
  }
  struct _Unwind_Context throw_site = search;

  while (search.have_fde) {
    if (search.fde.has_lsda) {
      reason = __gxx_personality_v0(1, _UA_SEARCH_PHASE,
                                    exc->exception_class, exc, &search);
      if (reason == _URC_HANDLER_FOUND) {
        handler_pc = search.regs.pc;
        break;
      }
      if (reason != _URC_CONTINUE_UNWIND) {
        return _URC_FATAL_PHASE1_ERROR;
      }
    }
    if (!AdvanceContext(&search)) {
      break;
    }
  }
  if (handler_pc == 0) {
    return _URC_END_OF_STACK;
  }

  exc->private_1 = handler_pc;
  cleanup = throw_site;
  while (cleanup.have_fde) {
    _Unwind_Action actions = _UA_CLEANUP_PHASE;
    if (cleanup.regs.pc == handler_pc) {
      actions = (_Unwind_Action)(actions | _UA_HANDLER_FRAME);
    }
    if (cleanup.fde.has_lsda) {
      uintptr_t frame_pc = cleanup.regs.pc;
      reason = __gxx_personality_v0(1, actions, exc->exception_class, exc,
                                    &cleanup);
      if (reason == _URC_INSTALL_CONTEXT) {
        g_resume_context = cleanup;
        if (cleanup.installed_cleanup) {
          g_resume_context.regs.pc = frame_pc;
        } else {
          AdvanceContext(&g_resume_context);
        }
        g_resume_exception = exc;
        __davecc_unwind_install_context(
            cleanup.regs.pc, cleanup.regs.rsp, cleanup.regs.rbp, exc,
            (uintptr_t)cleanup.gr[1]);
        return _URC_FATAL_PHASE2_ERROR;
      }
      if (reason != _URC_CONTINUE_UNWIND) {
        return _URC_FATAL_PHASE2_ERROR;
      }
    }
    if (!AdvanceContext(&cleanup)) {
      break;
    }
  }
  return _URC_FATAL_PHASE2_ERROR;
}

void _Unwind_Resume(_Unwind_Exception* exc) {
  _Unwind_Reason_Code reason;
  struct _Unwind_Context context;
  if (exc == 0 || exc != g_resume_exception) {
    abort();
  }
  context = g_resume_context;
  while (context.have_fde) {
    if (context.fde.has_lsda) {
      uintptr_t frame_pc = context.regs.pc;
      _Unwind_Action actions = _UA_CLEANUP_PHASE;
      if (context.regs.pc == (uintptr_t)exc->private_1) {
        actions = (_Unwind_Action)(actions | _UA_HANDLER_FRAME);
      }
      reason = __gxx_personality_v0(1, actions, exc->exception_class, exc,
                                    &context);
      if (reason == _URC_INSTALL_CONTEXT) {
        g_resume_context = context;
        if (context.installed_cleanup) {
          g_resume_context.regs.pc = frame_pc;
        } else {
          AdvanceContext(&g_resume_context);
        }
        __davecc_unwind_install_context(
            context.regs.pc, context.regs.rsp, context.regs.rbp, exc,
            (uintptr_t)context.gr[1]);
      }
      if (reason != _URC_CONTINUE_UNWIND) {
        abort();
      }
    }
    if (!AdvanceContext(&context)) {
      break;
    }
  }
  abort();
}

typedef _Unwind_Reason_Code (*PersonalityFn)(
    int, _Unwind_Action, _Unwind_Exception_Class, _Unwind_Exception*,
    _Unwind_Context*);
static const PersonalityFn kForcePersonalityLink = __gxx_personality_v0;
