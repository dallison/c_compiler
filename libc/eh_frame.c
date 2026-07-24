#include <eh_frame.h>
#if defined(__arm__)
#include <eh_arm.h>
#endif

extern char __eh_frame_start[];
extern char __eh_frame_end[];

#define DAVECC_EH_MAX_MODULES 32

typedef struct {
  uintptr_t eh_frame_start;
  uintptr_t eh_frame_end;
  uintptr_t gcc_except_table_start;
  uintptr_t gcc_except_table_end;
  uintptr_t arm_exidx_start;
  uintptr_t arm_exidx_end;
} DaveEHModuleRange;

DaveEHModuleRange __davecc_eh_modules[DAVECC_EH_MAX_MODULES];
size_t __davecc_eh_module_count;

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
  uintptr_t cursor;
  DaveEHFDE fde;
  int count = 0;

  if (!DaveEHFrameGetRange(&range)) {
    return 0;
  }
  cursor = (uintptr_t)range.start;
  while (DaveEHFrameNextFDE(&cursor, (uintptr_t)range.end, &fde)) {
    count++;
  }
  return count;
}

static const uint8_t* DecodeLsdaPointer(const uint8_t* aug_ptr) {
  int32_t offset = *(const int32_t*)aug_ptr;
  return aug_ptr + offset;
}

int DaveEHFrameNextFDE(uintptr_t* cursor,
                       uintptr_t end,
                       DaveEHFDE* out) {
  uintptr_t entry_addr = *cursor;
  const uint8_t* entry = (const uint8_t*)entry_addr;
  if (cursor == 0 || out == 0) {
    return 0;
  }
  // Input .eh_frame fragments may be padded to their section alignment.
  // Skip padding byte-by-byte; treating it as a 32-bit length can combine a
  // trailing zero with the next CIE length and manufacture an enormous entry.
  while (entry_addr + 8 <= end && *(const uint8_t*)entry_addr == 0) {
    entry_addr++;
  }
  entry = (const uint8_t*)entry_addr;
  if (entry_addr + 8 > end) {
    *cursor = end;
    return 0;
  }
  if (*(const unsigned int*)(entry + 4) == 0) {
    unsigned int cie_length = *(const unsigned int*)entry;
    if (cie_length < 4 || entry_addr + 4 + cie_length > end) {
      *cursor = end;
      return 0;
    }
    entry = entry + 4 + cie_length;
    entry_addr = (uintptr_t)entry;
    if (entry_addr + 8 > end) {
      *cursor = end;
      return 0;
    }
  }

  unsigned int fde_length = *(const unsigned int*)entry;
  const uint8_t* entry_body = entry + 4;
  const uint8_t* entry_end = entry_body + fde_length;
  if (fde_length < 20 || (uintptr_t)entry_end > end ||
      *(const unsigned int*)entry_body == 0) {
    *cursor = end;
    return 0;
  }

  const uint8_t* fde = entry_body + 4;
  uintptr_t pc_begin = (uintptr_t)(*(const uint64_t*)fde);
  uintptr_t pc_range = (uintptr_t)(*(const uint64_t*)(fde + 8));
  const uint8_t* cursor_in_fde = fde + 16;
  const uint8_t* instructions = cursor_in_fde + 1;
  const uint8_t* lsda = 0;
  int has_lsda = 0;

  if (*cursor_in_fde == 4) {
    lsda = DecodeLsdaPointer(cursor_in_fde + 1);
    has_lsda = 1;
    instructions = cursor_in_fde + 5;
  }

  out->pc_begin = pc_begin;
  out->pc_end = pc_begin + pc_range;
  out->fde_start = entry;
  out->instructions = instructions;
  out->instructions_end = entry_end;
  out->lsda = lsda;
  out->has_frame = instructions < entry_end;
  out->has_lsda = has_lsda;
  *cursor = (uintptr_t)entry_end;
  return 1;
}

static int FindFDEInRange(uintptr_t pc, const uint8_t* start,
                          const uint8_t* end, DaveEHFDE* out) {
  uintptr_t cursor = (uintptr_t)start;
  DaveEHFDE fde;
  if (start == 0 || end <= start) {
    return 0;
  }
  while (DaveEHFrameNextFDE(&cursor, (uintptr_t)end, &fde)) {
    if (pc >= fde.pc_begin && pc < fde.pc_end) {
      if (out != 0) {
        *out = fde;
      }
      return 1;
    }
  }
  return 0;
}

int DaveEHFrameFindFDE(uintptr_t pc, DaveEHFDE* out) {
#if defined(__arm__)
  uintptr_t pc_begin;
  uintptr_t pc_end;
  const uint8_t* lsda;
  int found = DaveARMFindUnwindInfo(pc, &pc_begin, &pc_end, &lsda);
  size_t count = __davecc_eh_module_count;
  if (count > DAVECC_EH_MAX_MODULES) {
    count = DAVECC_EH_MAX_MODULES;
  }
  for (size_t i = 0; !found && i < count; i++) {
    DaveEHModuleRange* module = &__davecc_eh_modules[i];
    found = DaveARMFindUnwindInfoInRange(
        pc, (const uint8_t*)module->arm_exidx_start,
        (const uint8_t*)module->arm_exidx_end, &pc_begin, &pc_end, &lsda);
  }
  if (!found) {
    return 0;
  }
  if (out != 0) {
    out->pc_begin = pc_begin;
    out->pc_end = pc_end;
    out->fde_start = 0;
    out->instructions = 0;
    out->instructions_end = 0;
    out->lsda = lsda;
    out->has_frame = 1;
    out->has_lsda = lsda != 0;
  }
  return 1;
#else
  DaveEHFrameRange range;
  if (DaveEHFrameGetRange(&range) &&
      FindFDEInRange(pc, range.start, range.end, out)) {
    return 1;
  }
  size_t count = __davecc_eh_module_count;
  if (count > DAVECC_EH_MAX_MODULES) {
    count = DAVECC_EH_MAX_MODULES;
  }
  for (size_t i = 0; i < count; i++) {
    DaveEHModuleRange* module = &__davecc_eh_modules[i];
    if (FindFDEInRange(pc, (const uint8_t*)module->eh_frame_start,
                       (const uint8_t*)module->eh_frame_end, out)) {
      return 1;
    }
  }
  return 0;
#endif
}

static void InitCFI(DaveEHFrameCFI* cfi) {
  cfi->cfa_reg = 7;
  cfi->cfa_offset = 8;
  cfi->return_address_offset = -8;
  cfi->has_saved_rbp = 0;
  cfi->saved_rbp_offset = 0;
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
  out->cfa_reg = 6;
  out->cfa_offset = 8;
  return 1;
}

#if defined(__x86_64__)

static uintptr_t RegisterValue(const DaveEHFrameRegisters* regs, int reg) {
  if (reg == 6) {
    return regs->rbp;
  }
  if (reg == 7) {
    return regs->rsp;
  }
  return 0;
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

#endif /* __x86_64__ */
