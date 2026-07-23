#include <unwind.h>

#include <eh_frame.h>
#include <lsda.h>
#include <stdlib.h>

typedef struct {
  DaveEHFrameRegisters regs;
  DaveEHFDE fde;
  int have_fde;
} DaveUnwindContext;

static DaveUnwindContext g_unwind_context;

uintptr_t _Unwind_GetIP(_Unwind_Context* ctx) {
  (void)ctx;
  return g_unwind_context.regs.pc;
}

void _Unwind_SetIP(_Unwind_Context* ctx, uintptr_t ip) {
  (void)ctx;
  g_unwind_context.regs.pc = ip;
}

uintptr_t _Unwind_GetLanguageSpecificData(_Unwind_Context* ctx) {
  (void)ctx;
  if (!g_unwind_context.have_fde || !g_unwind_context.fde.has_lsda) {
    return 0;
  }
  return (uintptr_t)g_unwind_context.fde.lsda;
}

uintptr_t _Unwind_GetRegionStart(_Unwind_Context* ctx) {
  (void)ctx;
  if (!g_unwind_context.have_fde) {
    return 0;
  }
  return g_unwind_context.fde.pc_begin;
}

void DaveUnwindSetContext(uintptr_t pc, uintptr_t rsp, uintptr_t rbp) {
  g_unwind_context.regs.pc = pc;
  g_unwind_context.regs.rsp = rsp;
  g_unwind_context.regs.rbp = rbp;
  g_unwind_context.have_fde = DaveEHFrameFindFDE(pc, &g_unwind_context.fde);
}

void DaveUnwindAdvanceToCaller(void) {
  DaveEHFrameWalkResult walk;
  if (!DaveEHFrameWalkFrame(&g_unwind_context.regs, &walk)) {
    g_unwind_context.have_fde = 0;
    return;
  }
  g_unwind_context.regs.pc = walk.caller_pc;
  g_unwind_context.regs.rsp = walk.caller_rsp;
  g_unwind_context.regs.rbp = walk.caller_rbp;
  g_unwind_context.have_fde =
      DaveEHFrameFindFDE(g_unwind_context.regs.pc, &g_unwind_context.fde);
}

_Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Exception* exc) {
  (void)exc;
  return _URC_FATAL_PHASE1_ERROR;
}

void _Unwind_Resume(_Unwind_Exception* exc) {
  (void)exc;
  abort();
}

typedef _Unwind_Reason_Code (*PersonalityFn)(int, _Unwind_Action, uint64_t,
                                             _Unwind_Exception*,
                                             _Unwind_Context*);
static const PersonalityFn kForcePersonalityLink = __gxx_personality_v0;
