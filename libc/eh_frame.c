#include <eh_frame.h>

extern char __eh_frame_start[];
extern char __eh_frame_end[];

static uint32_t ReadU32(const uint8_t* p) {
  return *(const uint32_t*)p;
}

static uint64_t ReadU64(const uint8_t* p) {
  return *(const uint64_t*)p;
}

int DaveEHFrameGetRange(DaveEHFrameRange* range) {
  if (range == 0) {
    return 0;
  }
  range->start = (const uint8_t*)__eh_frame_start;
  range->end = (const uint8_t*)__eh_frame_end;
  return range->start < range->end;
}

int DaveEHFrameCountFDEs(void) {
  DaveEHFrameRange range;
  const uint8_t* entry;
  int count = 0;

  if (!DaveEHFrameGetRange(&range)) {
    return 0;
  }
  entry = range.start;
  while (entry + 8 <= range.end) {
    uint32_t cie_length = *(const unsigned int*)entry;
    if (cie_length == 0 || cie_length < 4) {
      break;
    }
    entry = entry + 4 + cie_length;
    if (entry + 8 > range.end) {
      break;
    }
    uint32_t fde_length = *(const unsigned int*)entry;
    if (fde_length == 0 || fde_length < 4) {
      break;
    }
    if (*(const unsigned int*)(entry + 4) != 0) {
      count++;
    }
    entry = entry + 4 + fde_length;
  }
  return count;
}

int DaveEHFrameNextFDE(uintptr_t* cursor,
                       uintptr_t end,
                       DaveEHFDE* out) {
  uintptr_t entry_addr = *cursor;
  const uint8_t* entry = (const uint8_t*)entry_addr;
  if (entry_addr >= end) {
    *cursor = end;
    return 0;
  }
  if (*(const unsigned int*)(entry + 4) == 0) {
    unsigned int cie_length = *(const unsigned int*)entry;
    if (cie_length == 0) {
      *cursor = end;
      return 0;
    }
    entry = entry + 4 + cie_length;
  }

  unsigned int fde_length = *(const unsigned int*)entry;
  const uint8_t* entry_body = entry + 4;
  const uint8_t* entry_end = entry_body + fde_length;
  if (fde_length == 0 || *(const unsigned int*)entry_body == 0) {
    *cursor = end;
    return 0;
  }

  const uint8_t* fde = entry_body + 4;
  uintptr_t pc_begin = (uintptr_t)(*(const uint64_t*)fde);
  uintptr_t pc_range = (uintptr_t)(*(const uint64_t*)(fde + 8));
  const uint8_t* instructions = fde + 17;

  out->pc_begin = pc_begin;
  out->pc_end = pc_begin + pc_range;
  out->fde_start = entry;
  out->instructions = instructions;
  out->instructions_end = entry_end;
  out->has_frame = instructions < entry_end;
  *cursor = (uintptr_t)entry_end;
  return 1;
}

int DaveEHFrameFindFDE(uintptr_t pc, DaveEHFDE* out) {
  DaveEHFrameRange range;
  uintptr_t cursor;
  DaveEHFDE fde;

  if (!DaveEHFrameGetRange(&range)) {
    return 0;
  }
  cursor = (uintptr_t)range.start;
  while (DaveEHFrameNextFDE(&cursor, (uintptr_t)range.end, &fde)) {
    if (pc >= fde.pc_begin && pc < fde.pc_end) {
      if (out != 0) {
        out->pc_begin = fde.pc_begin;
        out->pc_end = fde.pc_end;
        out->fde_start = fde.fde_start;
        out->instructions = fde.instructions;
        out->instructions_end = fde.instructions_end;
        out->has_frame = fde.has_frame;
      }
      return 1;
    }
  }
  return 0;
}

static void InitCFI(DaveEHFrameCFI* cfi) {
  cfi->cfa_reg = 7;  // rsp
  cfi->cfa_offset = 8;
  cfi->return_address_offset = -8;
  cfi->has_saved_rbp = 0;
  cfi->saved_rbp_offset = 0;
}

static uintptr_t RegisterValue(const DaveEHFrameRegisters* regs, int reg) {
  if (reg == 6) {
    return regs->rbp;
  }
  if (reg == 7) {
    return regs->rsp;
  }
  return 0;
}

int DaveEHFrameCFIAtPC(const DaveEHFDE* fde,
                       uintptr_t pc,
                       DaveEHFrameCFI* out) {
  if (fde == 0 || out == 0) {
    return 0;
  }

  InitCFI(out);
  if (!fde->has_frame || pc <= fde->pc_begin) {
    return 1;
  }
  out->cfa_offset = 16;
  out->has_saved_rbp = 1;
  out->saved_rbp_offset = -16;
  if (pc < fde->pc_begin + 7) {
    return 1;
  }
  out->cfa_reg = 6;  // rbp
  out->cfa_offset = 8;
  return 1;
}

int DaveEHFrameWalkFrame(const DaveEHFrameRegisters* regs,
                         DaveEHFrameWalkResult* out) {
  DaveEHFDE fde;
  DaveEHFrameCFI cfi;
  uintptr_t cfa_base;
  uintptr_t cfa;

  if (regs == 0 || out == 0) {
    return 0;
  }
  if (!DaveEHFrameFindFDE(regs->pc, &fde)) {
    return 0;
  }
  if (!DaveEHFrameCFIAtPC(&fde, regs->pc, &cfi)) {
    return 0;
  }
  cfa_base = RegisterValue(regs, cfi.cfa_reg);
  if (cfa_base == 0) {
    return 0;
  }
  cfa = cfa_base + (uintptr_t)cfi.cfa_offset;
  out->caller_rsp = cfa;
  out->caller_pc = *(uintptr_t*)(cfa + cfi.return_address_offset);
  out->caller_rbp = regs->rbp;
  if (cfi.has_saved_rbp) {
    out->caller_rbp = *(uintptr_t*)(cfa + cfi.saved_rbp_offset);
  }
  return out->caller_pc != 0;
}
