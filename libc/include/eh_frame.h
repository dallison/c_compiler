#ifndef DAVECC_EH_FRAME_H
#define DAVECC_EH_FRAME_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DAVE_EH_MAX_DREG 64

typedef struct {
  const uint8_t* start;
  const uint8_t* end;
} DaveEHFrameRange;

typedef struct {
  uintptr_t pc_begin;
  uintptr_t pc_end;
  const uint8_t* fde_start;
  const uint8_t* fde_end;
  const uint8_t* instructions;
  const uint8_t* instructions_end;
  const uint8_t* lsda;
  const uint8_t* cie_start;
  void* personality;
  int has_frame;
  int has_lsda;
} DaveEHFDE;

typedef enum {
  DAVE_CFI_REG_UNDEFINED = 0,
  DAVE_CFI_REG_SAME,
  DAVE_CFI_REG_OFFSET,
  DAVE_CFI_REG_VAL_OFFSET,
  DAVE_CFI_REG_REGISTER,
  DAVE_CFI_REG_UNSUPPORTED
} DaveCFIRegRule;

typedef struct {
  DaveCFIRegRule rule;
  intptr_t offset;
  int reg;
} DaveCFIRegState;

typedef struct {
  int cfa_reg;
  intptr_t cfa_offset;
  int ra_reg;
  DaveCFIRegState regs[DAVE_EH_MAX_DREG];
} DaveEHFrameCFI;

typedef struct {
  uintptr_t pc;
  uintptr_t rsp;
  uintptr_t rbp;
  uintptr_t gr[DAVE_EH_MAX_DREG];
} DaveEHFrameRegisters;

typedef struct {
  DaveEHFrameRegisters caller;
} DaveEHFrameWalkResult;

int DaveEHFrameGetRange(DaveEHFrameRange* range);
int DaveEHFrameCountFDEs(void);
int DaveEHFrameNextFDE(uintptr_t* cursor, uintptr_t end, DaveEHFDE* out);
int DaveEHFrameFindFDE(uintptr_t pc, DaveEHFDE* out);
int DaveEHFrameCFIAtPC(const DaveEHFDE* fde, uintptr_t pc,
                       DaveEHFrameCFI* out);
int DaveEHFrameWalkFrame(const DaveEHFrameRegisters* regs,
                         DaveEHFrameWalkResult* out);

void DaveEHFrameInitRegisters(DaveEHFrameRegisters* regs);
void DaveEHFrameSyncCanonical(DaveEHFrameRegisters* regs);
uintptr_t DaveEHFrameGetReg(const DaveEHFrameRegisters* regs, int dwarf_reg);
void DaveEHFrameSetReg(DaveEHFrameRegisters* regs, int dwarf_reg,
                       uintptr_t value);

extern DaveEHFrameRegisters g_davecc_unwind_transfer_regs;

void __register_frame(const void* begin);
void __deregister_frame(const void* begin);

#ifdef __cplusplus
}
#endif

#endif /* DAVECC_EH_FRAME_H */
