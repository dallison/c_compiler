#ifndef bpf_reg_alloc_h
#define bpf_reg_alloc_h

#include <stdbool.h>
#include "bitset.h"
#include "bpf_machine.h"
#include "target_generator.h"

struct BPFGenerator;

typedef struct BPFRegister {
  TargetRegister base;
} BPFRegister;

typedef struct {
  struct BPFGenerator* bpf;
  BPFRegister regs[BPF_NUM_REGS];
  BitSet used_saved_regs;
  int current_spill_size;
  int max_spill_size;
} BPFRegisterAllocator;

void BPFRegisterAllocatorInit(BPFRegisterAllocator* alloc,
                              struct BPFGenerator* bpf);
void BPFRegisterAllocatorDestruct(BPFRegisterAllocator* alloc);
void BPFAllocateRegisters(BPFRegisterAllocator* alloc);
const char* BPFRegisterNameFromNum(int num, char* buf, size_t len);

#endif /* bpf_reg_alloc_h */
