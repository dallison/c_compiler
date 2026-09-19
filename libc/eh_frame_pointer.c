#include <eh_frame.h>

#if !defined(__x86_64__) && !defined(__aarch64__) && !defined(__riscv) && \
    !defined(__risc_v__)

#if defined(__p_code__)
#define DAVE_PCODE_DREG_FP 61
#define DAVE_PCODE_DREG_SP 62
#define DAVE_PCODE_DREG_PC 63

int DaveEHFrameFindFDE(uintptr_t pc, DaveEHFDE* out) {
  (void)pc;
  (void)out;
  return 0;
}

int DaveEHFrameCFIAtPC(const DaveEHFDE* fde, uintptr_t pc,
                       DaveEHFrameCFI* out) {
  (void)fde;
  (void)pc;
  (void)out;
  return 0;
}

void DaveEHFrameInitRegisters(DaveEHFrameRegisters* regs) {
  size_t i;
  if (regs == 0) {
    return;
  }
  regs->pc = 0;
  regs->rsp = 0;
  regs->rbp = 0;
  for (i = 0; i < DAVE_EH_MAX_DREG; i++) {
    regs->gr[i] = 0;
  }
}

void DaveEHFrameSyncCanonical(DaveEHFrameRegisters* regs) {
  if (regs == 0) {
    return;
  }
  regs->gr[DAVE_PCODE_DREG_PC] = regs->pc;
  regs->gr[DAVE_PCODE_DREG_SP] = regs->rsp;
  regs->gr[DAVE_PCODE_DREG_FP] = regs->rbp;
}

uintptr_t DaveEHFrameGetReg(const DaveEHFrameRegisters* regs, int dwarf_reg) {
  if (regs == 0 || dwarf_reg < 0 || dwarf_reg >= DAVE_EH_MAX_DREG) {
    return 0;
  }
  if (dwarf_reg == DAVE_PCODE_DREG_PC) {
    return regs->pc;
  }
  if (dwarf_reg == DAVE_PCODE_DREG_SP) {
    return regs->rsp;
  }
  if (dwarf_reg == DAVE_PCODE_DREG_FP) {
    return regs->rbp;
  }
  return regs->gr[dwarf_reg];
}

void DaveEHFrameSetReg(DaveEHFrameRegisters* regs, int dwarf_reg,
                       uintptr_t value) {
  if (regs == 0 || dwarf_reg < 0 || dwarf_reg >= DAVE_EH_MAX_DREG) {
    return;
  }
  regs->gr[dwarf_reg] = value;
  if (dwarf_reg == DAVE_PCODE_DREG_PC) {
    regs->pc = value;
  } else if (dwarf_reg == DAVE_PCODE_DREG_SP) {
    regs->rsp = value;
  } else if (dwarf_reg == DAVE_PCODE_DREG_FP) {
    regs->rbp = value;
  }
}
#endif

// DaveCC's hardware ABIs maintain a frame-pointer chain in every generated
// non-leaf function. Exception unwinding on ARM and p-code does not decode
// DWARF CFI yet: the saved frame pointer and return address live at fixed
// offsets from the current frame pointer.
int DaveEHFrameWalkFrame(const DaveEHFrameRegisters* regs,
                         DaveEHFrameWalkResult* out) {
  if (regs == 0 || out == 0 || regs->rbp == 0) {
    return 0;
  }
#if defined(__arm__) || defined(__i386__)
  {
    uintptr_t* frame = (uintptr_t*)regs->rbp;
    out->caller = *regs;
    out->caller.rbp = frame[0];
    out->caller.pc = frame[1];
    out->caller.rsp = regs->rbp + 2 * sizeof(uintptr_t);
    DaveEHFrameSyncCanonical(&out->caller);
  }
#elif defined(__p_code__)
  {
    uintptr_t* frame = (uintptr_t*)regs->rbp;
    uintptr_t* argument_frame = (uintptr_t*)regs->rsp;
    out->caller = *regs;
    out->caller.rbp = frame[0];
    out->caller.pc = argument_frame[1];
    out->caller.rsp = argument_frame[0];
    DaveEHFrameSyncCanonical(&out->caller);
  }
#else
#error "frame-pointer exception unwinding is unsupported on this target"
#endif
  return out->caller.pc != 0;
}

#endif /* !LP64 CFI targets */
