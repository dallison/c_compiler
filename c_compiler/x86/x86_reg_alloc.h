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
#include "target_ra_ir.h"

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
  struct X86Generator* rv;
  X86Register int_regs[X86_MAX_INT_REGS];
  X86Register float_regs[X86_MAX_FLOAT_REGS];

  BitSet used_int_regs;
  BitSet used_float_regs;
  int current_spilled_region_size;
  int max_spilled_region_size;
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

// True if the allocator gives |inst|'s result a floating point register.
bool X86HasFloatResult(TargetInstruction* inst);

// The DWARF number of |reg|, or -1 if it is none.
int X86RegisterDwarfNumber(X86RegisterAllocator* allocator,
                           TargetRegister* reg);

// How the shared allocator code reads x86-64 instructions.
const TargetRAInfo* X86RAInfo(void);

// Allocates the current function with the shared coloring allocator.
void X86AllocateRegistersByColoring(X86RegisterAllocator* allocator);

#endif /* x86_reg_alloc_h */
