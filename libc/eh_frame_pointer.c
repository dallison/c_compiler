#include <eh_frame.h>

// DaveCC's AArch64 and RISC-V ABIs maintain a frame-pointer chain in every
// generated function. Exception unwinding on those targets therefore does not
// need to decode DWARF CFI: the saved frame pointer and return address live at
// fixed offsets from the current frame pointer.
int DaveEHFrameWalkFrame(const DaveEHFrameRegisters* regs,
                         DaveEHFrameWalkResult* out) {
  if (regs == 0 || out == 0 || regs->rbp == 0) {
    return 0;
  }
#if defined(__aarch64__)
  uintptr_t* frame = (uintptr_t*)regs->rbp;
  out->caller_rbp = frame[0];
  out->caller_pc = frame[1];
  out->caller_rsp = regs->rbp + 2 * sizeof(uintptr_t);
#elif defined(__risc_v__)
  uintptr_t* frame = (uintptr_t*)regs->rbp;
  out->caller_rbp = frame[-2];
  out->caller_pc = frame[-1];
  out->caller_rsp = regs->rbp;
#elif defined(__p_code__)
  uintptr_t* frame = (uintptr_t*)regs->rbp;
  uintptr_t* argument_frame = (uintptr_t*)regs->rsp;
  out->caller_rbp = frame[0];
  out->caller_pc = argument_frame[1];
  // P-code uses this field to carry the caller's argument pointer. Unlike the
  // hardware targets, its return-address position is defined relative to AP,
  // while the number of saved registers between AP and FP is variable.
  out->caller_rsp = argument_frame[0];
#else
#error "frame-pointer exception unwinding is unsupported on this target"
#endif
  return out->caller_pc != 0;
}
