#ifndef DAVECC_EH_FRAME_H
#define DAVECC_EH_FRAME_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
  const uint8_t* start;
  const uint8_t* end;
} DaveEHFrameRange;

typedef struct {
  uintptr_t pc_begin;
  uintptr_t pc_end;
  const uint8_t* fde_start;
  const uint8_t* instructions;
  const uint8_t* instructions_end;
  int has_frame;
} DaveEHFDE;

typedef struct {
  int cfa_reg;
  intptr_t cfa_offset;
  intptr_t return_address_offset;
  int has_saved_rbp;
  intptr_t saved_rbp_offset;
} DaveEHFrameCFI;

typedef struct {
  uintptr_t pc;
  uintptr_t rsp;
  uintptr_t rbp;
} DaveEHFrameRegisters;

typedef struct {
  uintptr_t caller_pc;
  uintptr_t caller_rsp;
  uintptr_t caller_rbp;
} DaveEHFrameWalkResult;

int DaveEHFrameGetRange(DaveEHFrameRange* range);
int DaveEHFrameCountFDEs(void);
int DaveEHFrameNextFDE(uintptr_t* cursor,
                       uintptr_t end,
                       DaveEHFDE* out);
int DaveEHFrameFindFDE(uintptr_t pc, DaveEHFDE* out);
int DaveEHFrameCFIAtPC(const DaveEHFDE* fde,
                       uintptr_t pc,
                       DaveEHFrameCFI* out);
int DaveEHFrameWalkFrame(const DaveEHFrameRegisters* regs,
                         DaveEHFrameWalkResult* out);

#endif /* DAVECC_EH_FRAME_H */
