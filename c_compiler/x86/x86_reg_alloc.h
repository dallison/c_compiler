//
//  x86_reg_alloc.h
//  c_compiler_library
//

#ifndef x86_reg_alloc_h
#define x86_reg_alloc_h

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "bitset.h"
#include "map.h"
#include "x86_machine.h"
#include "x86_profile.h"
#include "target_generator.h"

struct X86Generator;
struct X86Instruction;

typedef enum {
  kX86RegTypeInt,
  kX86RegTypeFloat,
} X86RegisterType;

typedef struct X86Register {
  TargetRegister base;
  X86RegisterType type;
} X86Register;

typedef struct {
  X86RegisterType type;
  int start;
  int end;
  const char* prefix;
  int base;
  bool temp;
} X86RegisterRange;

#define X86_MAX_ALLOCATION_DEPTH 4
#define X86_NUM_REGISTER_RANGES 11

typedef struct {
  struct X86Generator* rv;
  X86Register int_regs[X86_MAX_INT_REGS];
  X86Register float_regs[X86_MAX_FLOAT_REGS];
  X86RegisterRange register_ranges[X86_NUM_REGISTER_RANGES];
  int num_register_ranges;

  BitSet used_int_regs;
  BitSet used_float_regs;
  int current_spilled_region_size;
  int max_spilled_region_size;
  BitSet preserved_instructions;
  Map varreg_spills;
  bool spill_after_definition;
  unsigned pinned_int_phys;
  unsigned pinned_float_phys;
  TargetInstruction* allocating[X86_MAX_ALLOCATION_DEPTH];
  size_t allocating_depth;
} X86RegisterAllocator;

void X86RegisterAllocatorInit(X86RegisterAllocator* alloc,
                             struct X86Generator* rv);
X86RegisterAllocator* NewX86RegisterAllocator(struct X86Generator* pcode);
const char* X86RegisterNameFromNumImpl(int num, X86RegisterType type, char* buf,
                                       size_t len, const X86Profile* profile);
#define X86RegisterNameFromNum(num, type, buf, len) \
  X86RegisterNameFromNumImpl(num, type, buf, len, NULL)
void X86SetRegisterNameProfile(const X86Profile* profile);
const X86Profile* X86CurrentRegisterNameProfile(void);
void X86RegisterAllocatorDestruct(X86RegisterAllocator* alloc);
void X86RegisterAllocatorDelete(X86RegisterAllocator* alloc);
void X86AllocateRegisters(X86RegisterAllocator* emitter);
const char* X86RegisterName(X86Register* reg, char* buf, size_t len);

#endif /* x86_reg_alloc_h */
