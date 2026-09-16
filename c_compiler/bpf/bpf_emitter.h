#ifndef bpf_emitter_h
#define bpf_emitter_h

#include <stdio.h>
#include "bpf_codegen.h"

typedef struct {
  BPFGenerator* bpf;
  BPFRegisterAllocator* regs;
  int saved_reg_bytes;
  int spill_bytes;
} BPFEmitter;

void BPFEmitterInit(BPFEmitter* emitter, BPFGenerator* bpf);
void BPFEmitterDestruct(BPFEmitter* emitter);
void BPFPrintFunction(BPFEmitter* emitter, FILE* fp);
void BPFPrintCXXAdjustorThunks(FILE* fp);

#endif /* bpf_emitter_h */
