//
//  x86_reg_alloc.c
//  c_compiler_library
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "x86_reg_alloc.h"
#include <assert.h>
#include <string.h>
#include "x86_profile.h"
#include "x86_codegen.h"
#include "x86_machine.h"
#include "compiler.h"

static void InitializeRegister(X86Register* reg, int num, X86RegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void X86RegisterAllocatorInit(X86RegisterAllocator* allocator, X86Generator* rv) {
  allocator->rv = rv;

  for (int i = 0; i < X86_MAX_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kX86RegTypeInt);
  }

  for (int i = 0; i < X86_MAX_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kX86RegTypeFloat);
  }

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  allocator->current_spilled_region_size = 0;
  allocator->max_spilled_region_size = 0;
}

X86RegisterAllocator* NewX86RegisterAllocator(X86Generator* pcode) {
  X86RegisterAllocator* reg_alloc = malloc(sizeof(X86RegisterAllocator));
  X86RegisterAllocatorInit(reg_alloc, pcode);
  return reg_alloc;
}

void X86RegisterAllocatorDestruct(X86RegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
}

void X86RegisterAllocatorDelete(X86RegisterAllocator* alloc) {
  X86RegisterAllocatorDestruct(alloc);
  free(alloc);
}

// The logical register file has 32 slots but x86-64 only has 16 integer and 16
// xmm physical registers, so several slots alias the same physical register
// (e.g. logical slots 7, 18 and 22 all denote r12).  Map a logical slot to its
// canonical physical register number so the allocator can avoid handing the
// same physical register to two simultaneously-live values.
static int X86IntPhysical(X86RegisterAllocator* allocator, int slot) {
  if (!allocator->rv->profile->is_64bit) {
    return (slot >= 0 && slot < allocator->rv->profile->num_int_regs) ? slot : -1;
  }
  // Canonical x86-64 numbering: rax=0 rcx=1 rdx=2 rbx=3 rsp=4 rbp=5 rsi=6 rdi=7
  // r8=8 r9=9 r10=10 r11=11 r12=12 r13=13 r14=14 r15=15.  This mirrors the
  // kIntRegNames table used for printing.
  static const int kPhys[X86_MAX_INT_REGS] = {
      0,  0,  4,  11, 10, 10, 11, 12, 5,  3,  7,  6,  2,  1,  8,  9,
      10, 11, 12, 13, 14, 15, 12, 13, 14, 15, 10, 11, 8,  9,  10, 11,
  };
  return (slot >= 0 && slot < X86_MAX_INT_REGS) ? kPhys[slot] : -1;
}

static int X86FloatPhysical(X86RegisterAllocator* allocator, int slot) {
  if (!allocator->rv->profile->is_64bit) {
    return slot & 7;
  }
  // The xmm name table is xmm0..xmm15 repeated, so the physical register is the
  // slot modulo 16.
  return slot & 15;
}

int X86RegisterDwarfNumber(X86RegisterAllocator* allocator,
                           TargetRegister* r) {
  X86Register* reg = (X86Register*)r;
  if (reg == NULL) {
    return -1;
  }
  bool is_64bit = allocator->rv->profile->is_64bit;
  if (reg->type == kX86RegTypeFloat) {
    // xmm0 is 17 on x86-64 and 21 on i386.
    return (is_64bit ? 17 : 21) + X86FloatPhysical(allocator, reg->base.num);
  }
  int phys = X86IntPhysical(allocator, reg->base.num);
  if (phys < 0 || !is_64bit) {
    return phys;  // i386 numbers them as the hardware does.
  }
  // x86-64 numbers the first eight rax, rdx, rcx, rbx, rsi, rdi, rbp, rsp.
  static const int kDwarf[8] = {0, 2, 1, 3, 7, 6, 4, 5};
  return phys < 8 ? kDwarf[phys] : phys;
}


static X86RegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(constf):
    case X86_OP(constd):
    case X86_OP(fmv_s):
    case X86_OP(fmv_d):
    case X86_OP(fa0):
    case X86_OP(fa1):
    case X86_OP(fa2):
    case X86_OP(fa3):
    case X86_OP(fa4):
    case X86_OP(fa5):
    case X86_OP(fa6):
    case X86_OP(fa7):
    case X86_OP(fvarreg):
    case X86_OP(loadss):
    case X86_OP(loadsd):
    case X86_OP(loadv):
    case X86_OP(storess):
    case X86_OP(storesd):
    case X86_OP(storev):
    case X86_OP(paddb):
    case X86_OP(paddw):
    case X86_OP(paddd):
    case X86_OP(paddq):
    case X86_OP(psubb):
    case X86_OP(psubw):
    case X86_OP(psubd):
    case X86_OP(psubq):
    case X86_OP(pand):
    case X86_OP(por):
    case X86_OP(pxor):
    case X86_OP(pcmpeqb):
    case X86_OP(pcmpeqw):
    case X86_OP(pcmpeqd):
    case X86_OP(pcmpgtb):
    case X86_OP(pcmpgtw):
    case X86_OP(pcmpgtd):
    case X86_OP(addps):
    case X86_OP(addpd):
    case X86_OP(subps):
    case X86_OP(subpd):
    case X86_OP(mulps):
    case X86_OP(mulpd):
    case X86_OP(divps):
    case X86_OP(divpd):
    case X86_OP(addss):
    case X86_OP(addsd):
    case X86_OP(subss):
    case X86_OP(subsd):
    case X86_OP(mulss):
    case X86_OP(mulsd):
    case X86_OP(divss):
    case X86_OP(divsd):
    case X86_OP(sqrtss):
    case X86_OP(sqrtsd):
    case X86_OP(ucomiss):
    case X86_OP(ucomisd):
    case X86_OP(cvtsi2ss):
    case X86_OP(cvtsi2sd):
    case X86_OP(cvtss2sd):
    case X86_OP(cvtsd2ss):
    case X86_OP(movss):
    case X86_OP(movsd):
    case X86_OP(movd):
    case X86_OP(fneg_ss):
    case X86_OP(fneg_sd):
    case X86_OP(callf):
    case X86_OP(rcallf):
    case X86_OP(resultf):
    case X86_OP(resultd):
    case X86_OP(resultv):
      return kX86RegTypeFloat;

    case X86_OP(cvttsd2si):
    case X86_OP(cvttss2si):
    case X86_OP(movq_xmm):
      return kX86RegTypeInt;

    default:
      return kX86RegTypeInt;
  }
}

bool X86HasFloatResult(TargetInstruction* inst) {
  return RegisterTypeFromInstruction(inst) == kX86RegTypeFloat;
}

void X86AllocateRegisters(X86RegisterAllocator* allocator) {
  X86SetRegisterNameProfile(X86_P(allocator->rv));
  X86AllocateRegistersByColoring(allocator);
}

static const X86Profile* g_x86_reg_name_profile;

void X86SetRegisterNameProfile(const X86Profile* profile) {
  g_x86_reg_name_profile = profile;
}

const X86Profile* X86CurrentRegisterNameProfile(void) {
  return g_x86_reg_name_profile != NULL ? g_x86_reg_name_profile
                                        : &kX86ProfileAMD64;
}

const char* X86RegisterName(X86Register* reg, char* buf, size_t len) {
  const X86Profile* profile = g_x86_reg_name_profile != NULL ? g_x86_reg_name_profile
                                                             : &kX86ProfileAMD64;
  return X86RegisterNameFromNumImpl(reg->base.num, reg->type, buf, len, profile);
}

const char* X86RegisterNameFromNumImpl(int num, X86RegisterType type, char* buf,
                                       size_t len, const X86Profile* profile) {
  if (profile == NULL) {
    profile = g_x86_reg_name_profile != NULL ? g_x86_reg_name_profile
                                             : &kX86ProfileAMD64;
  }

  static const char* kAmd64IntRegNames[X86_MAX_INT_REGS] = {
      "rax",  "rax",  "rsp",  "r11",  "r10",  "r10",  "r11",  "r12",
      "rbp",  "rbx",  "rdi",  "rsi",  "rdx",  "rcx",  "r8",   "r9",
      "r10",  "r11",  "r12",  "r13",  "r14",  "r15",  "r12",  "r13",
      "r14",  "r15",  "r10",  "r11",  "r8",   "r9",   "r10",  "r11",
  };
  static const char* kAmd64FloatRegNames[X86_MAX_FLOAT_REGS] = {
      "xmm0",  "xmm1",  "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",
      "xmm8",  "xmm9",  "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
      "xmm0",  "xmm1",  "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",
      "xmm8",  "xmm9",  "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
  };
  static const char* kI386IntRegNames[8] = {
      "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
  };
  static const char* kI386FloatRegNames[8] = {
      "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
  };

  const char* const* int_names =
      profile->is_64bit ? kAmd64IntRegNames : kI386IntRegNames;
  const char* const* float_names =
      profile->is_64bit ? kAmd64FloatRegNames : kI386FloatRegNames;
  int num_int_regs = profile->num_int_regs;
  int num_float_regs = profile->num_float_regs;

  if (type == kX86RegTypeInt) {
    if (num >= 0 && num < num_int_regs) {
      snprintf(buf, len, "%s", int_names[num]);
      return buf;
    }
    snprintf(buf, len, profile->is_64bit ? "r%d" : "eax", num);
    return buf;
  }

  if (num >= 0 && num < num_float_regs) {
    snprintf(buf, len, "%s", float_names[num]);
    return buf;
  }
  snprintf(buf, len, "xmm%d", num);
  return buf;
}

