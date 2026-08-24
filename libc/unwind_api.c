#include <unwind.h>

#include <eh_frame.h>
#include <lsda.h>
#include <stdlib.h>
#include <string.h>

#if defined(__arm__)
#include <eh_arm.h>

void DaveUnwindSetInstalledCleanup(_Unwind_Context* ctx, int is_cleanup) {
  if (ctx != 0) {
    ctx->installed_cleanup = is_cleanup;
  }
}

uintptr_t _Unwind_GetIP(_Unwind_Context* ctx) {
  if (ctx == 0) {
    return 0;
  }
  return ctx->vrs[DAVE_ARM_R_PC];
}

void _Unwind_SetIP(_Unwind_Context* ctx, uintptr_t ip) {
  if (ctx != 0) {
    ctx->vrs[DAVE_ARM_R_PC] = (uint32_t)ip;
  }
}

uintptr_t _Unwind_GetIPInfo(_Unwind_Context* ctx, int* ip_before_insn) {
  if (ip_before_insn != 0) {
    *ip_before_insn = 0;
  }
  return _Unwind_GetIP(ctx);
}

_Unwind_Word _Unwind_GetGR(_Unwind_Context* ctx, int index) {
  if (ctx == 0 || index < 0 || index > 15) {
    return 0;
  }
  return ctx->vrs[index];
}

void _Unwind_SetGR(_Unwind_Context* ctx, int index, _Unwind_Word value) {
  if (ctx == 0 || index < 0 || index > 15) {
    return;
  }
  ctx->vrs[index] = (uint32_t)value;
}

uintptr_t _Unwind_GetCFA(_Unwind_Context* ctx) {
  if (ctx == 0) {
    return 0;
  }
  return ctx->vrs[DAVE_ARM_R_SP];
}

uintptr_t _Unwind_GetLanguageSpecificData(_Unwind_Context* ctx) {
  if (ctx == 0) {
    return 0;
  }
  return (uintptr_t)ctx->lsda;
}

uintptr_t _Unwind_GetRegionStart(_Unwind_Context* ctx) {
  if (ctx == 0) {
    return 0;
  }
  return ctx->fnstart;
}

void DaveUnwindSetContext(uintptr_t pc, uintptr_t rsp, uintptr_t rbp) {
  (void)pc;
  (void)rsp;
  (void)rbp;
}

void DaveUnwindSetRegisters(const DaveEHFrameRegisters* regs) {
  (void)regs;
}

void DaveUnwindAdvanceToCaller(void) {
}

int DaveUnwindEHExceptionReg(void) { return DAVE_EH_REG_EXCEPTION; }
int DaveUnwindEHSelectorReg(void) { return DAVE_EH_REG_SELECTOR; }

_Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Exception* exc) {
  return DaveARMRaiseException(exc);
}

void _Unwind_Resume(_Unwind_Exception* exc) {
  DaveARMResume(exc);
}

void _Unwind_DeleteException(_Unwind_Exception* exc) {
  if (exc != 0 && exc->exception_cleanup != 0) {
    exc->exception_cleanup(_URC_FOREIGN_EXCEPTION_CAUGHT, exc);
  }
}

typedef int (*PersonalityFn)(int, _Unwind_Exception*, _Unwind_Context*);
static const PersonalityFn kForcePersonalityLink = __aeabi_unwind_cpp_pr1;

#else /* !__arm__ */

struct _Unwind_Context {
  DaveEHFrameRegisters regs;
  DaveEHFDE fde;
  DaveEHFrameCFI cfi;
  int have_fde;
  int have_cfi;
  int installed_cleanup;
};

#if defined(__risc_v__)
#define DAVECC_UNWIND_TLS __thread
#else
#define DAVECC_UNWIND_TLS
#endif

static DAVECC_UNWIND_TLS struct _Unwind_Context g_unwind_context;
static DAVECC_UNWIND_TLS struct _Unwind_Context g_resume_context;
static DAVECC_UNWIND_TLS _Unwind_Exception* g_resume_exception;
DaveEHFrameRegisters g_davecc_unwind_transfer_regs;

extern void __davecc_capture_regs(DaveEHFrameRegisters* regs);
extern void __davecc_unwind_install_context(
    uintptr_t target, uintptr_t rsp, uintptr_t rbp,
    _Unwind_Exception* exception, uintptr_t selector);

static struct _Unwind_Context* ContextOrCurrent(_Unwind_Context* ctx) {
  return ctx != 0 ? ctx : &g_unwind_context;
}

static void RefreshCFI(struct _Unwind_Context* context) {
  uintptr_t pc;
  context->have_cfi = 0;
  if (!context->have_fde) {
    return;
  }
  pc = context->regs.pc;
  if (pc >= context->fde.pc_end && pc > context->fde.pc_begin) {
    pc--;
  }
  if (DaveEHFrameCFIAtPC(&context->fde, pc, &context->cfi)) {
    context->have_cfi = 1;
  }
}

static void BindFDE(struct _Unwind_Context* context, uintptr_t pc) {
  context->have_fde = DaveEHFrameFindFDE(pc, &context->fde);
  RefreshCFI(context);
}

static _Unwind_Personality_Fn PersonalityForContext(
    const struct _Unwind_Context* context) {
  if (context->have_fde && context->fde.personality != 0) {
    return (_Unwind_Personality_Fn)context->fde.personality;
  }
  return 0;
}

uintptr_t _Unwind_GetIP(_Unwind_Context* ctx) {
  return ContextOrCurrent(ctx)->regs.pc;
}

void _Unwind_SetIP(_Unwind_Context* ctx, uintptr_t ip) {
  ContextOrCurrent(ctx)->regs.pc = ip;
}

uintptr_t _Unwind_GetIPInfo(_Unwind_Context* ctx, int* ip_before_insn) {
  struct _Unwind_Context* current = ContextOrCurrent(ctx);
  if (ip_before_insn != 0) {
    *ip_before_insn = 1;
  }
  return current->regs.pc;
}

_Unwind_Word _Unwind_GetGR(_Unwind_Context* ctx, int index) {
  struct _Unwind_Context* current = ContextOrCurrent(ctx);
  return (_Unwind_Word)DaveEHFrameGetReg(&current->regs, index);
}

void _Unwind_SetGR(_Unwind_Context* ctx, int index, _Unwind_Word value) {
  struct _Unwind_Context* current = ContextOrCurrent(ctx);
  DaveEHFrameSetReg(&current->regs, index, (uintptr_t)value);
}

uintptr_t _Unwind_GetCFA(_Unwind_Context* ctx) {
  struct _Unwind_Context* current = ContextOrCurrent(ctx);
  uintptr_t base;
  if (!current->have_cfi) {
    return 0;
  }
  base = DaveEHFrameGetReg(&current->regs, current->cfi.cfa_reg);
  return (uintptr_t)((intptr_t)base + current->cfi.cfa_offset);
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

void DaveUnwindSetRegisters(const DaveEHFrameRegisters* regs) {
  if (regs == 0) {
    return;
  }
  g_unwind_context.regs = *regs;
  DaveEHFrameSyncCanonical(&g_unwind_context.regs);
  BindFDE(&g_unwind_context, g_unwind_context.regs.pc);
}

void DaveUnwindSetContext(uintptr_t pc, uintptr_t rsp, uintptr_t rbp) {
  DaveEHFrameRegisters regs;
  DaveEHFrameInitRegisters(&regs);
  regs.pc = pc;
  regs.rsp = rsp;
  regs.rbp = rbp;
  DaveEHFrameSyncCanonical(&regs);
  DaveUnwindSetRegisters(&regs);
}

static int AdvanceContext(struct _Unwind_Context* context) {
  DaveEHFrameWalkResult walk;
  if (!DaveEHFrameWalkFrame(&context->regs, &walk)) {
    context->have_fde = 0;
    context->have_cfi = 0;
    return 0;
  }
  context->regs = walk.caller;
  DaveEHFrameSyncCanonical(&context->regs);
  BindFDE(context, context->regs.pc > 0 ? context->regs.pc - 1 : 0);
  return 1;
}

void DaveUnwindAdvanceToCaller(void) {
  AdvanceContext(&g_unwind_context);
}

static void InstallLandingContext(struct _Unwind_Context* cleanup,
                                  _Unwind_Exception* exc) {
  uintptr_t selector;
  g_resume_exception = exc;
  g_davecc_unwind_transfer_regs = cleanup->regs;
  DaveEHFrameSyncCanonical(&g_davecc_unwind_transfer_regs);
  selector = DaveEHFrameGetReg(&cleanup->regs, DAVE_EH_REG_SELECTOR);
  __davecc_unwind_install_context(
      cleanup->regs.pc, cleanup->regs.rsp, cleanup->regs.rbp, exc, selector);
}

static int IsHandlerFrame(const struct _Unwind_Context* context,
                          _Unwind_Exception* exc) {
  if (exc == 0) {
    return 0;
  }
  return context->regs.pc == (uintptr_t)exc->private_1;
}

static int InvokePersonality(struct _Unwind_Context* context,
                             _Unwind_Exception* exc, _Unwind_Action actions,
                             _Unwind_Reason_Code* out_reason) {
  _Unwind_Personality_Fn personality = PersonalityForContext(context);
  if (personality == 0 || !context->have_fde || !context->fde.has_lsda) {
    return 0;
  }
  *out_reason = personality(1, actions, exc->exception_class, exc, context);
  return 1;
}

_Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Exception* exc) {
  struct _Unwind_Context search;
  struct _Unwind_Context cleanup;
  uintptr_t handler_pc = 0;
  uintptr_t handler_region = 0;
  _Unwind_Reason_Code reason;

  if (exc == 0) {
    return _URC_FATAL_PHASE1_ERROR;
  }

  memset(&search, 0, sizeof(search));
  if (g_unwind_context.regs.pc != 0) {
    search = g_unwind_context;
  } else {
    __davecc_capture_regs(&search.regs);
    DaveEHFrameSyncCanonical(&search.regs);
    BindFDE(&search, search.regs.pc);
  }
  {
    struct _Unwind_Context throw_site = search;

    while (1) {
      if (InvokePersonality(&search, exc, _UA_SEARCH_PHASE, &reason)) {
        if (reason == _URC_HANDLER_FOUND) {
          handler_pc = search.regs.pc;
          handler_region = search.fde.pc_begin;
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
    exc->private_2 = handler_region;
    cleanup = throw_site;
    while (1) {
      _Unwind_Action actions = _UA_CLEANUP_PHASE;
      uintptr_t frame_pc = cleanup.regs.pc;
      if (IsHandlerFrame(&cleanup, exc)) {
        actions = (_Unwind_Action)(actions | _UA_HANDLER_FRAME);
      }
      if (InvokePersonality(&cleanup, exc, actions, &reason)) {
        if (reason == _URC_INSTALL_CONTEXT) {
          g_resume_context = cleanup;
          if (cleanup.installed_cleanup) {
            g_resume_context.regs.pc = frame_pc;
          } else {
            AdvanceContext(&g_resume_context);
          }
          InstallLandingContext(&cleanup, exc);
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
  while (1) {
    _Unwind_Action actions = _UA_CLEANUP_PHASE;
    uintptr_t frame_pc = context.regs.pc;
    if (IsHandlerFrame(&context, exc)) {
      actions = (_Unwind_Action)(actions | _UA_HANDLER_FRAME);
    }
    if (InvokePersonality(&context, exc, actions, &reason)) {
      if (reason == _URC_INSTALL_CONTEXT) {
        g_resume_context = context;
        if (context.installed_cleanup) {
          g_resume_context.regs.pc = frame_pc;
        } else {
          AdvanceContext(&g_resume_context);
        }
        InstallLandingContext(&context, exc);
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

void _Unwind_DeleteException(_Unwind_Exception* exc) {
  if (exc != 0 && exc->exception_cleanup != 0) {
    exc->exception_cleanup(_URC_FOREIGN_EXCEPTION_CAUGHT, exc);
  }
}

typedef _Unwind_Reason_Code (*PersonalityFn)(
    int, _Unwind_Action, _Unwind_Exception_Class, _Unwind_Exception*,
    _Unwind_Context*);
static const PersonalityFn kForcePersonalityLink = __gxx_personality_v0;

#endif /* __arm__ */
