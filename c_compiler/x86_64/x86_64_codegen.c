//
//  x86_64_codegen.c
//  c_compiler_library
//
//  Created by David Allison on 2/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "x86_64_codegen.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"
#include "compiler.h"
#include "debug.h"
#include "member_pointer.h"

#include "x86_64_optimize.h"
#include "target_basic_block.h"

static void LowerVariables(X86_64Generator* rv, Generator* gen);
static TargetInstruction* LowerIRNode(X86_64Generator* rv, Generator* gen,
                                      IRNode* node);

static void Trap() {}
static void TrapLower(String* name) {
  if (StringEqual(name, "CollectActualArguments")) {
    Trap();
  }
}

const char* X86_64OpcodeName(int op) {
  if (op <= (int)TARGET_OP(fvarreg)) {
    return TargetOpcodeName(op);
  }
#define OPCODE(name) \
  case X86_64_OP(name): \
    return #name;
  switch ((X86_64Opcode)op) {
    OPCODE(loadb)
    OPCODE(loadb_z)
    OPCODE(loadw)
    OPCODE(loadw_z)
    OPCODE(loadl)
    OPCODE(loadl_z)
    OPCODE(loadq)
    OPCODE(storeb)
    OPCODE(storew)
    OPCODE(storel)
    OPCODE(storeq)
    OPCODE(loadss)
    OPCODE(loadsd)
    OPCODE(storess)
    OPCODE(storesd)
    OPCODE(add)
    OPCODE(addl)
    OPCODE(sub)
    OPCODE(subl)
    OPCODE(imul)
    OPCODE(imull)
    OPCODE(idiv)
    OPCODE(div)
    OPCODE(mod)
    OPCODE(and)
    OPCODE(or)
    OPCODE(xor)
    OPCODE(not)
    OPCODE(neg)
    OPCODE(shl)
    OPCODE(shr)
    OPCODE(sar)
    OPCODE(shll)
    OPCODE(shrl)
    OPCODE(sarl)
    OPCODE(cmp)
    OPCODE(test)
    OPCODE(setl)
    OPCODE(setb)
    OPCODE(setg)
    OPCODE(je)
    OPCODE(jne)
    OPCODE(jl)
    OPCODE(jge)
    OPCODE(jb)
    OPCODE(jae)
    OPCODE(jz)
    OPCODE(jnz)
    OPCODE(jmp)
    OPCODE(call)
    OPCODE(rcall)
    OPCODE(callf)
    OPCODE(rcallf)
    OPCODE(mov)
    OPCODE(movabs)
    OPCODE(lea)
    OPCODE(lea_rip)
    OPCODE(addss)
    OPCODE(addsd)
    OPCODE(subss)
    OPCODE(subsd)
    OPCODE(mulss)
    OPCODE(mulsd)
    OPCODE(divss)
    OPCODE(divsd)
    OPCODE(sqrtss)
    OPCODE(sqrtsd)
    OPCODE(ucomiss)
    OPCODE(ucomisd)
    OPCODE(cvtsi2ss)
    OPCODE(cvtsi2sd)
    OPCODE(cvttss2si)
    OPCODE(cvttsd2si)
    OPCODE(cvtss2sd)
    OPCODE(cvtsd2ss)
    OPCODE(movss)
    OPCODE(movsd)
    OPCODE(movd)
    OPCODE(movq_xmm)
    OPCODE(fneg_ss)
    OPCODE(fneg_sd)
    OPCODE(movxc)
    OPCODE(tp)
    OPCODE(nop)
    OPCODE(movslq)
    OPCODE(sete)
    OPCODE(setne)
    OPCODE(atomic_compare_exchange_bool)
    OPCODE(atomic_compare_exchange_val)
    OPCODE(atomic_compare_exchange_n)
    OPCODE(atomic_fetch_add_sub)
    OPCODE(a0)
    OPCODE(a1)
    OPCODE(a2)
    OPCODE(a3)
    OPCODE(a4)
    OPCODE(a5)
    OPCODE(a6)
    OPCODE(a7)
    OPCODE(fa0)
    OPCODE(fa1)
    OPCODE(fa2)
    OPCODE(fa3)
    OPCODE(fa4)
    OPCODE(fa5)
    OPCODE(fa6)
    OPCODE(fa7)
    OPCODE(nrvoval)
    OPCODE(x0)
    OPCODE(t0)
    OPCODE(regarg)
    OPCODE(spill)
    OPCODE(reload)
    default:
      return TargetOpcodeName(op);
  }
#undef OPCODE
}

bool X86_64IsExpression(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(label):
    case X86_64_OP(asm):
    case X86_64_OP(call):
    case X86_64_OP(rcall):
    case X86_64_OP(je):
    case X86_64_OP(jne):
    case X86_64_OP(jl):
    case X86_64_OP(jge):
    case X86_64_OP(jb):
    case X86_64_OP(jae):
    case X86_64_OP(jz):
    case X86_64_OP(jnz):
    case X86_64_OP(jmp):
    case X86_64_OP(callf):
    case X86_64_OP(rcallf):
    case X86_64_OP(ret):
    case X86_64_OP(save):
    case X86_64_OP(restore):
//    case X86_64_OP(rmov):
//    case X86_64_OP(rmovf):
//    case X86_64_OP(rmovd):
    case X86_64_OP(storeb):
    case X86_64_OP(storel):
    case X86_64_OP(storew):
    case X86_64_OP(storeq):
    case X86_64_OP(storess):
    case X86_64_OP(storesd):
    case X86_64_OP(loc):
    case X86_64_OP(named_label):
    case X86_64_OP(regarg):
    case X86_64_OP(nrvoval):
    case X86_64_OP(symbol):
    case X86_64_OP(spill):
      return false;
    default:
      return !X86_64IsFixedRegister(inst) && !X86_64IsConst(inst);
  }
}

bool X86_64GeneratesOutput(TargetInstruction* inst) {
  if (X86_64IsFixedRegister(inst)) {
    return false;
  }
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(ivarreg):
    case X86_64_OP(fvarreg):
    case X86_64_OP(resulti):
    case X86_64_OP(resultf):
    case X86_64_OP(resultd):
      return false;
    default:
      return X86_64IsExpression(inst) && !X86_64IsSymbol(inst) && !X86_64IsConst(inst);
  }
}

bool X86_64IsLoad(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(loadb):
    case X86_64_OP(loadl):
    case X86_64_OP(loadl_z):
    case X86_64_OP(loadq):
    case X86_64_OP(loadb_z):
    case X86_64_OP(loadw):
    case X86_64_OP(loadw_z):
    case X86_64_OP(loadss):
    case X86_64_OP(loadsd):
      return true;
    default:
      return false;
  }
}

bool X86_64IsSignedLoad(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(loadb):  // emitted as movsbq (sign-extends)
    case X86_64_OP(loadw):  // emitted as movswq (sign-extends)
    case X86_64_OP(loadl):  // emitted as movslq (sign-extends)
    case X86_64_OP(loadq):  // full 64-bit load, no extension needed
      return true;
    // NOTE: loadl_z is deliberately excluded.  It is emitted as a plain movl,
    // which zero-extends the upper 32 bits (correct for unsigned int).
    default:
      return false;
  }
}

bool X86_64IsStore(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(storeb):
    case X86_64_OP(storel):
    case X86_64_OP(storeq):
    case X86_64_OP(storew):
    case X86_64_OP(storess):
    case X86_64_OP(storesd):
      return true;
    default:
      return false;
  }
}

bool X86_64IsIntConst(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(const32):
    case X86_64_OP(const8):
    case X86_64_OP(const16):
    case X86_64_OP(const64):
      return true;
    default:
      return false;
  }
}

bool X86_64IsConst(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(const32):
    case X86_64_OP(const8):
    case X86_64_OP(const16):
    case X86_64_OP(const64):
    case X86_64_OP(constf):
    case X86_64_OP(constd):
      return true;
    default:
      return false;
  }
}

bool X86_64IsFloatingPoint(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(loadss):
    case X86_64_OP(loadsd):
    case X86_64_OP(storess):
    case X86_64_OP(storesd):
    case X86_64_OP(addss):
    case X86_64_OP(addsd):
    case X86_64_OP(subss):
    case X86_64_OP(subsd):
    case X86_64_OP(mulss):
    case X86_64_OP(mulsd):
    case X86_64_OP(divss):
    case X86_64_OP(divsd):
    case X86_64_OP(sqrtss):
    case X86_64_OP(sqrtsd):
    case X86_64_OP(ucomiss):
    case X86_64_OP(ucomisd):
    case X86_64_OP(cvtsi2ss):
    case X86_64_OP(cvtsi2sd):
    case X86_64_OP(cvttss2si):
    case X86_64_OP(cvttsd2si):
    case X86_64_OP(cvtss2sd):
    case X86_64_OP(cvtsd2ss):
    case X86_64_OP(movss):
    case X86_64_OP(movsd):
    case X86_64_OP(movd):
    case X86_64_OP(movq_xmm):
    case X86_64_OP(fneg_ss):
    case X86_64_OP(fneg_sd):
    case X86_64_OP(fmv_s):
    case X86_64_OP(fmv_d):
    case X86_64_OP(fa0):
    case X86_64_OP(fa1):
    case X86_64_OP(fa2):
    case X86_64_OP(fa3):
    case X86_64_OP(fa4):
    case X86_64_OP(fa5):
    case X86_64_OP(fa6):
    case X86_64_OP(fa7):
    case X86_64_OP(fvarreg):
    case X86_64_OP(constf):
    case X86_64_OP(constd):
    case X86_64_OP(resultf):
    case X86_64_OP(resultd):
      return true;
    default:
      return false;
  }
}

bool X86_64IsSymbol(TargetInstruction* inst) {
  return (X86_64Opcode)((int)inst->opcode == (int)X86_64_OP(symbol));
}

bool X86_64IsCall(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(call):
    case X86_64_OP(rcall):
    case X86_64_OP(callf):
    case X86_64_OP(rcallf):
       return true;
    default:
      return false;
  }
}

bool X86_64IsArgRegister(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(a0):
    case X86_64_OP(a1):
    case X86_64_OP(a2):
    case X86_64_OP(a3):
    case X86_64_OP(a4):
    case X86_64_OP(a5):
    case X86_64_OP(a6):
    case X86_64_OP(a7):
    case X86_64_OP(fa0):
    case X86_64_OP(fa1):
    case X86_64_OP(fa2):
    case X86_64_OP(fa3):
    case X86_64_OP(fa4):
    case X86_64_OP(fa5):
    case X86_64_OP(fa6):
    case X86_64_OP(fa7):
      return true;
    default:
      return false;
  }
}
 
bool X86_64IsVarRegister(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
  case X86_64_OP(ivarreg):
  case X86_64_OP(fvarreg):
      return true;
  default:
    return false;
  }
}

bool X86_64IsFixedRegister(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(a0):
    case X86_64_OP(a1):
    case X86_64_OP(a2):
    case X86_64_OP(a3):
    case X86_64_OP(a4):
    case X86_64_OP(a5):
    case X86_64_OP(a6):
    case X86_64_OP(a7):
    case X86_64_OP(fa0):
    case X86_64_OP(fa1):
    case X86_64_OP(fa2):
    case X86_64_OP(fa3):
    case X86_64_OP(fa4):
    case X86_64_OP(fa5):
    case X86_64_OP(fa6):
    case X86_64_OP(fa7):
    case X86_64_OP(x0):
    case X86_64_OP(t0):
    case X86_64_OP(fp):

      // Calls always return in a0 or fa0.
    case X86_64_OP(call):
    case X86_64_OP(rcall):
    case X86_64_OP(callf):
    case X86_64_OP(rcallf):
      return true;
    default:
      return false;
  }
}

bool X86_64IsResult(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
     case X86_64_OP(resulti):
      case X86_64_OP(resultf):
      case X86_64_OP(resultd):
      return true;
    default:
      return false;
  }
}

bool X86_64IsBranch(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(jmp):
    case X86_64_OP(call):
    case X86_64_OP(rcall):
    case X86_64_OP(je):
    case X86_64_OP(jne):
    case X86_64_OP(jl):
    case X86_64_OP(jge):
    case X86_64_OP(jb):
    case X86_64_OP(jae):
    case  X86_64_OP(jz):
    case  X86_64_OP(jnz):
      return true;
    default:
      return false;
  }
}

bool X86_64IsConditionalBranch(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(je):
    case X86_64_OP(jne):
    case X86_64_OP(jl):
    case X86_64_OP(jge):
    case X86_64_OP(jb):
    case X86_64_OP(jae):
    case  X86_64_OP(jz):
    case  X86_64_OP(jnz):
      return true;
    default:
      return false;
  }
}

bool X86_64IsSpill(TargetInstruction* inst) {
  return (X86_64Opcode)((int)inst->opcode == (int)X86_64_OP(spill));
}

bool X86_64IsLabel(TargetInstruction* inst) {
  return (X86_64Opcode)((int)inst->opcode == (int)X86_64_OP(label));
}

bool X86_64IsReturn(TargetInstruction* inst) {
  return (X86_64Opcode)((int)inst->opcode == (int)X86_64_OP(ret));
}

int X86_64IntValue(TargetInstruction* inst) {
  if (inst->opcode == (TargetOpcode)X86_64_OP(x0)) {
    return 0;
  }
  return (int)((TargetConstant*)inst)->value.ivalue;
}

// Is the value small enough to be encoded in an immediate field?
bool X86_64IsPossibleImmediate(int64_t value) {
  // Check for 12 bit signed immediate.
  if (value < 0) {
    return value >= -2048;
  }
  return value < 2048;
}

TargetInstruction* X86_64GetBranchTarget(TargetInstruction* inst) {
  X86_64Opcode opcode = (X86_64Opcode)inst->opcode;
  if (opcode == X86_64_OP(jmp)) {
    return inst->operand[0];
  }
  if (opcode == X86_64_OP(jnz) || opcode == X86_64_OP(jz)) {
    return inst->operand[1];
  }
  if (inst->operand[2] != NULL) {
    return inst->operand[2];
  }
  return inst->operand[0];
}

bool X86_64IsJumpableEntry(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)X86_64_OP(jmp);
}

static TargetVirtuals virtuals = {
  .opcode_name = X86_64OpcodeName,
  .is_branch = X86_64IsBranch,
  .is_call = X86_64IsCall,
  .is_return = X86_64IsReturn,
  .is_spill = X86_64IsSpill,
  .is_label = X86_64IsLabel,
  .is_floating_point = X86_64IsFloatingPoint,
  .is_conditional_branch = X86_64IsConditionalBranch,
  .is_fixed_register = X86_64IsFixedRegister,
  .is_const = X86_64IsConst,
  .is_symbol = X86_64IsSymbol,
  .is_expression = X86_64IsExpression,
  .is_table_entry = X86_64IsJumpableEntry,
  .get_branch_target = X86_64GetBranchTarget,
};

void X86_64GeneratorInit(X86_64Generator* rv, Generator* gen) {
  TargetGeneratorInit(&rv->base, gen, &virtuals);

  rv->num_int_arg_regs = 0;
  rv->num_fp_arg_regs = 0;
  rv->num_int_reg_vars = 0;
  rv->num_fp_reg_vars = 0;
  rv->struct_return_reg = -1;
  rv->struct_return_spill_offset = 0;
  rv->zero = NULL;
  rv->tmp = NULL;
  rv->not_leaf = false;
  rv->has_incoming_stack_args = false;
  memset(rv->int_argument_registers, 0, sizeof(rv->int_argument_registers));
  memset(rv->fp_argument_registers, 0, sizeof(rv->fp_argument_registers));
  VectorInit(&rv->var_regs);
  VectorInit(&rv->saved_regs);
  rv->saved_arg_area_size = 0;
  VectorInit(&rv->offsets);
  VectorInit(&rv->exception_ranges);
  VectorInit(&rv->exception_typeinfos);

  X86_64RegisterAllocatorInit(&rv->register_allocator, rv);
}

X86_64Generator* NewX86_64Generator(Generator* gen) {
  X86_64Generator* rv = malloc(sizeof(X86_64Generator));
  X86_64GeneratorInit(rv, gen);
  return rv;
}

void X86_64GeneratorDestruct(X86_64Generator* rv) {
  TargetGeneratorDestruct(&rv->base);
  VectorDestructWithContents(&rv->var_regs, NULL, /*free_element=*/true);
  VectorDestructWithContents(&rv->saved_regs, NULL, /*free_element=*/true);
  VectorDestructWithContents(&rv->offsets, NULL, /*free_element=*/true);
  VectorDestructWithContents(&rv->exception_ranges, NULL, /*free_element=*/true);
  VectorDestruct(&rv->exception_typeinfos);
  X86_64RegisterAllocatorDestruct(&rv->register_allocator);
  
}

static void ResolveExceptionRanges(X86_64Generator* rv, Generator* gen) {
  for (size_t i = 0; i < gen->exception_typeinfos.length; i++) {
    VectorAppend(&rv->exception_typeinfos, gen->exception_typeinfos.value.p[i]);
  }
  for (size_t i = 0; i < gen->exception_ranges.length; i++) {
    ExceptionHandlerRange* ir_range = gen->exception_ranges.value.p[i];
    TargetInstruction* try_start = ir_range->try_start->data.ptr;
    TargetInstruction* try_end = ir_range->try_end->data.ptr;
    TargetInstruction* catch_label = ir_range->catch_label->data.ptr;
    if (try_start == NULL || try_end == NULL || catch_label == NULL) {
      continue;
    }
    try_start->flags |= TARGET_INST_KEEP_UNREACHABLE;
    try_end->flags |= TARGET_INST_KEEP_UNREACHABLE;
    catch_label->flags |= TARGET_INST_KEEP_UNREACHABLE;
    catch_label->flags |= TARGET_INST_EXCEPTION_LANDING;
    X86_64ExceptionRange* range = malloc(sizeof(X86_64ExceptionRange));
    range->try_start = try_start;
    range->try_end = try_end;
    range->catch_label = catch_label;
    range->catch_typeinfo = ir_range->catch_typeinfo;
    range->is_cleanup = ir_range->is_cleanup;
    VectorAppend(&rv->exception_ranges, range);
  }
  for (size_t i = 0; i < gen->exception_keep_labels.length; i++) {
    IRNode* label = gen->exception_keep_labels.value.p[i];
    TargetInstruction* target_label = label->data.ptr;
    if (target_label != NULL) {
      target_label->flags |= TARGET_INST_KEEP_UNREACHABLE;
    }
  }
}

void X86_64GeneratorDelete(X86_64Generator* rv) {
  X86_64GeneratorDestruct(rv);
  free(rv);
}

static SavedArgumentRegister* NewSavedArgumentRegister(int reg_num,
                                                       int base_reg_num,
                                                       int offset,
                                                       bool is_fp,
                                                       int value_bytes) {
  SavedArgumentRegister* reg = malloc(sizeof(SavedArgumentRegister));
  reg->base_reg_num = base_reg_num;
  reg->reg_num = reg_num;
  reg->offset = offset;
  reg->is_fp = is_fp;
  reg->value_bytes = value_bytes;
  reg->copy_bytes = 0;
  return reg;
}

static COMPILER_UNUSED void SavedArgumentRegisterDelete(SavedArgumentRegister* reg) {
  free(reg);
}

// Some static utility functions that map to generic target functions.
static TargetInstruction* NewInstruction1(X86_64Opcode opcode,
                                          TargetInstruction* op1) {
  return TargetNewInstruction1((TargetOpcode)opcode, op1);
}

static TargetInstruction* NewInstruction2(X86_64Opcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2) {
  return TargetNewInstruction2((TargetOpcode)opcode, op1, op2);
}

static COMPILER_UNUSED TargetInstruction* NewInstruction3(X86_64Opcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2,
                                          TargetInstruction* op3) {
  return TargetNewInstruction3((TargetOpcode)opcode, op1, op2, op3);
}

static TargetInstruction* Emit(X86_64Generator* rv, TargetInstruction* inst) {
  return TargetEmit(&rv->base, inst);
}

static COMPILER_UNUSED TargetInstruction* EmitBefore(X86_64Generator* rv, TargetInstruction* inst,
                                     TargetInstruction* pos) {
  return TargetEmitBefore(&rv->base, inst, pos);
}

static COMPILER_UNUSED TargetInstruction* EmitAfter(X86_64Generator* rv, TargetInstruction* inst,
                                    TargetInstruction* pos) {
  return TargetEmitAfter(&rv->base, inst, pos);
}

static COMPILER_UNUSED TargetInstruction* EmitConstant(X86_64Generator* rv, TargetInstruction* c) {
  return TargetEmitConstant(&rv->base, c);
}

static COMPILER_UNUSED TargetInstruction* EmitSymbol(X86_64Generator* rv, TargetInstruction* c) {
  return TargetEmitSymbol(&rv->base, c);
}

static COMPILER_UNUSED TargetInstruction* FramePointer(X86_64Generator* rv) {
  return TargetFramePointer(&rv->base);
}

static COMPILER_UNUSED TargetInstruction* StackPointer(X86_64Generator* rv) {
  return TargetStackPointer(&rv->base);
}

static TargetInstruction* GetLoweredNode(IRNode* node) {
  return TargetGetLoweredNode(node);
}

static TargetInstruction* SetLoweredNode(IRNode* node,
                                         TargetInstruction* inst) {
  if (node->data.ptr != NULL) {
    return (TargetInstruction*)node->data.ptr;
  }
  return TargetSetLoweredNode(node, inst);
}

static TargetInstruction* GetDestInstruction(X86_64Generator* rv, Generator* gen,
                                             IRNode* node) {
  if (node->dest == NULL) {
    return NULL;
  }
  LowerIRNode(rv, gen, node->dest);
  return GetLoweredNode(node->dest);
}

static X86_64Opcode MoveOpcodeForLoad(X86_64Opcode load_opcode) {
  switch (load_opcode) {
    case X86_64_OP(loadss):
      return X86_64_OP(fmv_s);
    case X86_64_OP(loadsd):
      return X86_64_OP(fmv_d);
    default:
      return X86_64_OP(mv);
  }
}

// This backend has no x87 unit: it represents `long double` values with the
// same 64-bit SSE encoding as `double` (the wider 16-byte object layout only
// pads the storage; the guest libc reads a long double variadic argument as a
// `double`).  Every place that selects a double-width SSE operation over a
// single-precision one must therefore treat `long double` like `double`,
// otherwise a long double value is silently truncated to 32 bits (e.g. a
// `movss` where a `movsd` was required).  `TypeIsDouble()` alone does not cover
// long double, so use this predicate for the double-vs-float choice.
static bool X86_64FpIsDoubleWidth(TypeRecord* type) {
  return TypeIsDouble(type) || TypeIsLongDouble(type);
}

static TargetInstruction* GetIntConstant(X86_64Generator* rv, IRNode* node,
                                         TargetType type, int64_t value) {
  return TargetGetIntConstant(&rv->base, node, type, value);
}

static COMPILER_UNUSED TargetInstruction* GetFloatingPointConstant(X86_64Generator* rv,
                                                   IRNode* node,
                                                   TargetType type,
                                                   double value) {
  return TargetGetFloatingPointConstant(&rv->base, node, type, value);
}

static TargetInstruction* GetSymbol(X86_64Generator* rv, IRNode* node,
                                    Symbol* symbol) {
  return TargetGetSymbol(&rv->base, node, symbol);
}

static TargetInstruction* NewInstruction(X86_64Opcode opcode) {
  return TargetNewInstruction((TargetOpcode)opcode);
}

static TargetInstruction* Zero(X86_64Generator* rv) {
  if (rv->zero == NULL) {
    rv->zero = Emit(rv, NewInstruction(X86_64_OP(x0)));
  }
  return rv->zero;
}

static TargetInstruction* Tmp(X86_64Generator* rv) {
  if (rv->tmp == NULL) {
    rv->tmp = Emit(rv, NewInstruction(X86_64_OP(t0)));
  }
  return rv->tmp;
}

static TargetInstruction* IntArgumentRegister(X86_64Generator* rv, int argnum) {
  if (rv->int_argument_registers[argnum] == NULL) {
    // Allocate instruction for argument register and emit it.  The argument
    // registers have to be contiguous in value from a0..a7.
    rv->int_argument_registers[argnum] =
        EmitSymbol(rv, NewInstruction(X86_64_OP(a0) + argnum));
  }
  return rv->int_argument_registers[argnum];
}

static TargetInstruction* IncomingIntArgumentRegister(X86_64Generator* rv,
                                                      int argnum) {
  return EmitSymbol(rv, NewInstruction(X86_64_OP(a0) + argnum));
}


static TargetInstruction* FloatingPointArgumentRegister(X86_64Generator* rv,
                                                        int argnum) {
  if (rv->fp_argument_registers[argnum] == NULL) {
    // Allocate instruction for argument register and emit it.  The argument
    // registers have to be contiguous in value from a0..a7.
    rv->fp_argument_registers[argnum] =
        EmitSymbol(rv, NewInstruction(X86_64_OP(fa0) + argnum));
  }
  return rv->fp_argument_registers[argnum];
}

static TargetInstruction* IncomingFloatingPointArgumentRegister(
    X86_64Generator* rv, int argnum) {
  return EmitSymbol(rv, NewInstruction(X86_64_OP(fa0) + argnum));
}

static TargetInstruction* IntVariableRegister(X86_64Generator* rv, int varnum, Symbol* sym) {
  for (size_t i = 0; i < rv->var_regs.length; i++) {
    RegisterVariable* var = rv->var_regs.value.p[i];
    if (!var->is_fp && var->varnum == varnum) {
      return var->inst;
    }
  }
  RegisterVariable* var = malloc(sizeof(RegisterVariable));
  var->varnum = varnum;
  TargetSymbol* inst = malloc(sizeof(TargetSymbol));
  TargetInitInstruction(&inst->base, TARGET_OP(ivarreg));
  inst->symbol = sym;
  var->inst = &inst->base;
  var->is_fp = false;
  VectorAppend(&rv->var_regs, var);
  return EmitSymbol(rv, var->inst);
}


static TargetInstruction* FloatingPointVariableRegister(X86_64Generator* rv, int varnum, Symbol* sym) {
  for (size_t i = 0; i < rv->var_regs.length; i++) {
    RegisterVariable* var = rv->var_regs.value.p[i];
    if (var->is_fp && var->varnum == varnum) {
      return var->inst;
    }
  }
  RegisterVariable* var = malloc(sizeof(RegisterVariable));
  var->varnum = varnum;
  TargetSymbol* inst = malloc(sizeof(TargetSymbol));
  TargetInitInstruction(&inst->base, TARGET_OP(fvarreg));
  inst->symbol = sym;
  var->inst = &inst->base;
  var->is_fp = true;
  VectorAppend(&rv->var_regs, var);
  return EmitSymbol(rv, var->inst);
}

// Add immediate to the src.  If it fits in 12 bits we can use an addi
// instruction, otherwise load the immediate and use an add instruction.
static TargetInstruction* AddImmediate(X86_64Generator* rv, TargetInstruction* src,
                                       int64_t immed) {
  int64_t imm = immed;
  if (immed < 0) {
    imm = -immed;
  }
  TargetInstruction* immed_inst =
      GetIntConstant(rv, NULL, kTargetType32Bit, immed);
  if (imm <= 0x7ff) {
    return Emit(rv, NewInstruction2(X86_64_OP(add), src, immed_inst));
  }
  TargetInstruction* li = Emit(rv, NewInstruction1(X86_64_OP(mov), immed_inst));
  return Emit(rv, NewInstruction2(X86_64_OP(add), src, li));
}

static TargetInstruction* SetDestOrMove(X86_64Generator* rv,
                                        TargetInstruction* from,
                                        TargetInstruction* to,
                                        X86_64Opcode mov_opcode) {
  if (from == to) {
    return to;
  }
  X86_64Opcode from_opcode = (X86_64Opcode)from->opcode;
  bool fixed_pointer =
      from_opcode == X86_64_OP(sp) || from_opcode == X86_64_OP(fp);
  bool can_set_dest =
      !fixed_pointer && from->dest == NULL && X86_64GeneratesOutput(from);

  if (can_set_dest) {
    TargetSetDest(from, to);
    return from;
  }
  TargetInstruction* move = Emit(rv, NewInstruction1(mov_opcode, from));
  move->dest = to;
  return to;
}

static TargetInstruction* SetDestOrMoveToArgReg(X86_64Generator* rv,
                                                IRNode* from_node,
                                                TargetInstruction* from,
                                                TargetInstruction* to,
                                                X86_64Opcode rmov_opcode) {
  (void)from_node;
  if (from == to) {
    return to;
  }
  // Calls may follow other calls while preparing their arguments.  Do not
  // assign an argument register directly as an earlier expression's
  // destination: that value can be produced before an intervening call and
  // then be clobbered.  Materialize the final ABI-register move at the call
  // site instead.
  TargetInstruction* move = Emit(rv, NewInstruction1(rmov_opcode, from));
  move->dest = to;
  return to;
}

static TargetInstruction* AddValue(X86_64Generator* rv, TargetInstruction* src,
                                   TargetInstruction* value) {
  if (TargetIsConst(value)) {
    return AddImmediate(rv, src, TargetIntValue(value));
  }

  if (((int)value->opcode == (int)X86_64_OP(x0))) {
    return src;
  }
  return Emit(rv, NewInstruction2(X86_64_OP(add), src, value));
}

// Calculate the offset from the frame pointer to a local variable in the stack.
static int LocalVariableOffset(X86_64Generator* rv, int32_t var_offset) {
  return var_offset - rv->base.stack_frame_size -
      X86_64_STACK_FRAME_HEADER_SIZE;
}

static TargetInstruction* PagedOffsetFrom(X86_64Generator* rv, TargetInstruction* src,
                                          int32_t offset, int32_t* page_offset) {
  // Offset is not in range.  Need to calculate an offset in a register.
  //
  // We calculate a page offset.  The addi instruction
  // has a 12 bit signed immediate that can be added to an offset
  // calculated from the src.
  int page;
  if (offset < 0) {
    page = -(-offset & ~0x7ff);
  } else {
    page = offset & ~0x7ff;
  }
  TargetInstruction* page_inst = NULL;
  for (size_t i = 0; i < rv->offsets.length; i++) {
    Offset* f = rv->offsets.value.p[i];
    if (f->page_offset == page) {
      page_inst = f->inst;
      break;
    }
  }
  if (page_inst == NULL) {
    // No page offset calculated, need to calculate one.
    page_inst =
        AddImmediate(rv, src, page);
    Offset* f = malloc(sizeof(Offset));
    f->inst = page_inst;
    f->page_offset = page;
    VectorAppend(&rv->offsets, f);
  }
  *page_offset = offset - page;
  return page_inst;
}

// Returns either an integer constant or an instruction to calculate an
// offset from the src.
static TargetInstruction* OffsetFrom(X86_64Generator* rv, TargetInstruction* src,
                                     int32_t offset) {
  bool offset_in_range = X86_64IsPossibleImmediate(offset);
  if (offset_in_range) {
    return AddImmediate(rv, src, offset);
  }
  int page_offset;
  TargetInstruction* page_inst = PagedOffsetFrom(rv, src, offset, &page_offset);
  if (page_offset == 0) {
    return page_inst;
  }
  return AddImmediate(rv, page_inst, page_offset);
}

static TargetInstruction* LoadImmediate(X86_64Generator* rv, X86_64Opcode opcode,
                                          TargetInstruction* base, int32_t offset) {
  if (X86_64IsPossibleImmediate(offset)) {
    return Emit(rv, NewInstruction2(opcode, base,
                                    GetIntConstant(rv, NULL, kTargetType32Bit, offset)));
  }
  int32_t page_offset;
  TargetInstruction* page_inst = PagedOffsetFrom(rv, base, offset, &page_offset);
  return Emit(rv, NewInstruction2(opcode, page_inst,
                                  GetIntConstant(rv, NULL, kTargetType32Bit, page_offset)));

}

static TargetInstruction* StoreImmediate(X86_64Generator* rv, X86_64Opcode opcode,
                                         TargetInstruction* value, TargetInstruction* base, int32_t offset) {
  if (X86_64IsPossibleImmediate(offset)) {
    return Emit(rv, NewInstruction3(opcode, value, base,
                                    GetIntConstant(rv, NULL, kTargetType32Bit, offset)));
  }
  int32_t page_offset;
  TargetInstruction* page_inst = PagedOffsetFrom(rv, base, offset, &page_offset);
  return Emit(rv, NewInstruction3(opcode, value, page_inst,
                                  GetIntConstant(rv, NULL, kTargetType32Bit, page_offset)));

}

static TargetInstruction* Memcpy(X86_64Generator* rv, TargetInstruction* dest_addr,
                                 TargetInstruction* src_addr, int length,
                                 int src_offset, int dest_offset, bool count_as_call) {
  if (length <= 40) {
    // Length is short, copy using sequence of ld/sd and lb/sb instructions.
    int num_bytes = length & 7;
    int num_words = length >> 3;
    TargetInstruction* result = NULL;
    TargetInstruction* stable_src = Emit(rv, NewInstruction1(X86_64_OP(mv), src_addr));
    for (int i = 0; i < num_words; i++, src_offset += 8, dest_offset += 8) {
      TargetInstruction* chunk_src =
          Emit(rv, NewInstruction1(X86_64_OP(mv), stable_src));
      if (src_offset != 0) {
        chunk_src = AddImmediate(rv, chunk_src, src_offset);
      }
      TargetInstruction* load = LoadImmediate(rv, X86_64_OP(loadq), chunk_src, 0);
      result = StoreImmediate(rv, X86_64_OP(storeq), load, dest_addr, dest_offset);
    }
    for (int i = 0; i < num_bytes; i++, src_offset += 1, dest_offset += 1) {
      TargetInstruction* chunk_src =
          Emit(rv, NewInstruction1(X86_64_OP(mv), stable_src));
      if (src_offset != 0) {
        chunk_src = AddImmediate(rv, chunk_src, src_offset);
      }
      TargetInstruction* load = LoadImmediate(rv, X86_64_OP(loadb), chunk_src, 0);
      result = StoreImmediate(rv, X86_64_OP(storeb), load, dest_addr, dest_offset);
    }
    if (count_as_call) {
      rv->base.num_calls--;
    }
    return result;
  }
  // TODO: generate a loop for intermediate lengths?

  // Length in a2.
  TargetInstruction* size = Emit(
      rv, NewInstruction1(X86_64_OP(mov),
                          GetIntConstant(rv, NULL, kTargetType32Bit, length)));
  TargetInstruction* arg2 = SetDestOrMoveToArgReg(
      rv, NULL, size, IntArgumentRegister(rv, 2), X86_64_OP(mv));
  //TargetInstruction* arg2 =
  //    Emit(rv, NewInstruction2(X86_64_OP(rmov), IntArgumentRegister(rv, 2), size));

  // Source in a1.
  if (src_offset != 0) {
    src_addr = OffsetFrom(rv, src_addr, src_offset);
  }
  TargetInstruction* arg1 = SetDestOrMoveToArgReg(
      rv, NULL, src_addr, IntArgumentRegister(rv, 1), X86_64_OP(mv));
      //Emit(
      //rv, NewInstruction2(X86_64_OP(rmov), IntArgumentRegister(rv, 1), src_addr));

  // Dest in a0.
  if (dest_offset != 0) {
    dest_addr = OffsetFrom(rv, dest_addr, dest_offset);
  }
  TargetInstruction* arg0 = SetDestOrMoveToArgReg(
      rv, NULL, dest_addr, IntArgumentRegister(rv, 0), X86_64_OP(mv));
      //Emit(
      //rv, NewInstruction2(X86_64_OP(rmov), IntArgumentRegister(rv, 0), dest_addr));

  // We need to keep the arguments alive until the point of the call.  This
  // is done using a X86_64_OP(regarg) instruction sequence.  See BuildArgList for
  // details on the regarg instruction.
  TargetInstruction* regarg =
      Emit(rv, NewInstruction2(X86_64_OP(regarg), NULL, arg2));
  regarg = Emit(rv, NewInstruction2(X86_64_OP(regarg), regarg, arg1));
  regarg = Emit(rv, NewInstruction2(X86_64_OP(regarg), regarg, arg0));

  TargetInstruction* memcpy = GetSymbol(rv, NULL, rv->base.memcpy);
  return Emit(rv, NewInstruction2(X86_64_OP(call), memcpy, regarg));
}

static TargetInstruction* Memzero(X86_64Generator* rv, TargetInstruction* dest_addr,
                                  int length, int offset) {
  if (length <= 40) {
    // Length is short, copy using sequence of ld/sd and lb/sb instructions.
    int num_bytes = length & 7;
    int num_words = length >> 3;
    TargetInstruction* result = NULL;
    for (int i = 0; i < num_words; i++, offset += 8) {
      result = StoreImmediate(rv, X86_64_OP(storeq), Zero(rv), dest_addr, offset);
    }
    for (int i = 0; i < num_bytes; i++, offset += 1) {
      result = StoreImmediate(rv, X86_64_OP(storeb), Zero(rv), dest_addr, offset);
    }
    rv->base.num_calls--;
    return result;
  }

  // Third parameter to memset is the length.
  TargetInstruction* size = Emit(
      rv, NewInstruction1(X86_64_OP(mov),
                          GetIntConstant(rv, NULL, kTargetType32Bit, length)));
  TargetInstruction* arg2 = SetDestOrMoveToArgReg(
      rv, NULL, size, IntArgumentRegister(rv, 2), X86_64_OP(mv));
  //TargetInstruction* arg2 =
  //    Emit(rv, NewInstruction2(X86_64_OP(rmov), IntArgumentRegister(rv, 2), size));

  // Second arg is zero.
  TargetInstruction* arg1 = Emit(
      rv, NewInstruction1(X86_64_OP(mv), Zero(rv)));
  arg1->dest = IntArgumentRegister(rv, 1);

  // First arg is the address.  Apply the offset to the base address; the
  // short (in-line) path above folds the offset into each store, but for the
  // memset call we must materialize base + offset explicitly or we would zero
  // the wrong location (e.g. clobbering the return address at rbp+0).
  if (offset != 0) {
    dest_addr = OffsetFrom(rv, dest_addr, offset);
  }
  TargetInstruction* arg0 = SetDestOrMoveToArgReg(
      rv, NULL, dest_addr, IntArgumentRegister(rv, 0), X86_64_OP(mv));
  //TargetInstruction* arg0 = Emit(
  //    rv, NewInstruction2(X86_64_OP(rmov), IntArgumentRegister(rv, 0), dest_addr));

  // We need to keep the arguments alive until the point of the call.  This
  // is done using a X86_64_OP(regarg) instruction sequence.  See BuildArgList for
  // details on the regarg instruction.
  TargetInstruction* regarg =
      Emit(rv, NewInstruction2(X86_64_OP(regarg), NULL, arg2));
  regarg = Emit(rv, NewInstruction2(X86_64_OP(regarg), regarg, arg1));
  regarg = Emit(rv, NewInstruction2(X86_64_OP(regarg), regarg, arg0));

  TargetInstruction* memset = GetSymbol(rv, NULL, rv->base.memset);
  return Emit(rv, NewInstruction2(X86_64_OP(call), memset, regarg));
}

static X86_64Opcode IR2X86_64(IROpcode op) {
  switch (op) {
    case IR_OP(addi):
      return X86_64_OP(add);
    case IR_OP(addf):
      return X86_64_OP(addss);
    case IR_OP(addd):
      return X86_64_OP(addsd);
    case IR_OP(adda):
      return X86_64_OP(add);

    case IR_OP(subi):
      return X86_64_OP(sub);
    case IR_OP(subf):
      return X86_64_OP(subss);
    case IR_OP(subd):
      return X86_64_OP(subsd);
    case IR_OP(suba):
      return X86_64_OP(sub);

    case IR_OP(muli):
      return X86_64_OP(imul);
    case IR_OP(mulf):
      return X86_64_OP(mulss);
    case IR_OP(muld):
      return X86_64_OP(mulsd);

    case IR_OP(divi):
      return X86_64_OP(idiv);
    case IR_OP(divf):
      return X86_64_OP(divss);
    case IR_OP(divd):
      return X86_64_OP(divsd);

    case IR_OP(modi):
      return X86_64_OP(mod);

    case IR_OP(lsri):
      return X86_64_OP(shr);
    case IR_OP(asri):
      return X86_64_OP(sar);
    case IR_OP(lsli):
      return X86_64_OP(shl);
    case IR_OP(rotli):
      return X86_64_OP(rol);
    case IR_OP(rotri):
      return X86_64_OP(ror);
    case IR_OP(clzi):
      return X86_64_OP(bsr);
    case IR_OP(ctzi):
      return X86_64_OP(bsf);

    case IR_OP(ori):
      return X86_64_OP(or);
    case IR_OP(andi):
      return X86_64_OP(and);
    case IR_OP(xori):
      return X86_64_OP(xor);

    case IR_OP(noti):
      return X86_64_OP(not);
    case IR_OP(nota):
      return X86_64_OP(not);
    case IR_OP(onescomp):
      return X86_64_OP(not);
    case IR_OP(negi):
      return X86_64_OP(neg);
    case IR_OP(negf):
      return X86_64_OP(fneg_ss);
    case IR_OP(negd):
      return X86_64_OP(fneg_sd);

    case IR_OP(i2f):
      return X86_64_OP(cvtsi2ss);
    case IR_OP(i2d):
      return X86_64_OP(cvtsi2sd);
    case IR_OP(f2d):
      return X86_64_OP(cvtss2sd);
    case IR_OP(d2f):
      return X86_64_OP(cvtsd2ss);
    case IR_OP(f2i):
      return X86_64_OP(cvttss2si);
    case IR_OP(d2i):
      return X86_64_OP(cvttsd2si);

    case IR_OP(movi):
      return X86_64_OP(mv);
    case IR_OP(movf):
      return X86_64_OP(fmv_s);
    case IR_OP(movd):
      return X86_64_OP(fmv_d);
    case IR_OP(mova):
      return X86_64_OP(mv);
//    case IR_OP(rmovi):
//      return X86_64_OP(rmov);
//    case IR_OP(rmovf):
//      return X86_64_OP(rmovf);
//    case IR_OP(rmovd):
//      return X86_64_OP(rmovd);
//    case IR_OP(rmova):
//      return X86_64_OP(rmov);
    case IR_OP(tmp):
      return X86_64_OP(tmp);
    default:
      assert(false);
      return 0;
  }
}

// Number of integer variables that can be pinned to a distinct physical
// register.  Mirrors the usable fixed slots in X86_64VarRegSlot(): a non-leaf
// function uses the callee-saved r12-r15 (4 of them); a leaf function makes no
// calls and may also use the caller-saved temporaries r8-r10 (3 of them).
// Beyond this, additional variables must live on the stack -- otherwise they
// would be allocated to caller-saved temporaries and silently clobbered across
// the calls they span, or alias another register variable.
#define X86_64_MAX_LEAF_INT_REG_VARS 3
#define X86_64_MAX_NONLEAF_INT_REG_VARS 4

static bool X86_64HasFreeIntRegVar(X86_64Generator* rv) {
  bool is_leaf = rv->base.num_calls == 0 && compiler->optimize;
  int limit = is_leaf ? X86_64_MAX_LEAF_INT_REG_VARS
                      : X86_64_MAX_NONLEAF_INT_REG_VARS;
  return rv->num_int_reg_vars < limit;
}

static bool UseRegisterForVariable(X86_64Generator* rv, IRNode* var_node) {
  if (OptLevel0()) {
    // When not optimizing, all variables are on the stack.
    return false;
  }
  // Aggregates (struct/union) are always manipulated through their address in
  // this backend: member and array-element access lower to adda/addressof/load
  // against the object's storage, and a small struct argument arrives packed in
  // a register only as a value.  Such a value has no stable address, so binding
  // an aggregate to a register variable makes taking &member (e.g. `a.x` where
  // x is an array member) yield the raw register contents instead of a pointer.
  // The frontend does not mark these implicit member addresses as address-taken,
  // so guard here: an aggregate must live in memory.
  if (TypeIsStructOrUnion(var_node->type)) {
    return false;
  }

  // Can't use a register if its address has been taken.
  IRVariable* var = (IRVariable*)var_node;
  if (var->symbol->flags.address_taken) {
    return false;
  }

  // No references?  No point in putting it in a register.
  if (var->base.outputs.length == 0) {
    return false;
  }
  return true;
}

// TODO: allow override of tls model per variable.
static void DiagnoseUnsupportedTlsModel(X86_64Generator* rv, const char* model) {
  (void)rv;
  fprintf(stderr,
          "error: x86-64 TLS model '%s' is not supported; use -ftls-model=local-exec "
          "for static executables\n",
          model);
  abort();
}

static const char* TlsModelName(TlsModel model) {
  switch (model) {
    case TLS(global_dynamic):
      return "global-dynamic";
    case TLS(local_dynamic):
      return "local-dynamic";
    case TLS(initial_exec):
      return "initial-exec";
    case TLS(local_exec):
      return "local-exec";
    default:
      return "unknown";
  }
}

static TargetInstruction* ThreadPointer(X86_64Generator* rv) {
  // Unlike targets with a dedicated thread-pointer register, x86-64 must load
  // the current FS base value into a caller-saved general register.  Emit that
  // load at every use: caching one instruction for the whole function lets a
  // call clobber its allocated register before a later TLS access.
  return Emit(rv, NewInstruction(X86_64_OP(tp)));
}

static TargetInstruction* GetTlsVariableAddress(X86_64Generator* rv, IRNode* node) {
  switch (compiler->tls_model) {
    case TLS(global_dynamic):
    case TLS(local_dynamic):
    case TLS(initial_exec):
      DiagnoseUnsupportedTlsModel(rv, TlsModelName(compiler->tls_model));
      return NULL;
    case TLS(local_exec): {
      TargetInstruction* tp = ThreadPointer(rv);
      TargetInstruction* offset =
          Emit(rv, NewInstruction1(X86_64_OP(movxc), GetLoweredNode(node)));
      offset->flags |= X86_64_TLS_RELOC;
      return Emit(rv, NewInstruction2(X86_64_OP(add), tp, offset));
    }
    default:
      DiagnoseUnsupportedTlsModel(rv, TlsModelName(compiler->tls_model));
      return NULL;
  }
}

static void GetTlsAddressAndOffset(X86_64Generator* rv, IRNode* addr_node,
                                   TargetInstruction** addr,
                                   TargetInstruction** offset) {
  switch (compiler->tls_model) {
    case TLS(global_dynamic):
    case TLS(local_dynamic):
    case TLS(initial_exec):
      DiagnoseUnsupportedTlsModel(rv, TlsModelName(compiler->tls_model));
      break;
    case TLS(local_exec): {
      TargetInstruction* tp_offset =
          Emit(rv, NewInstruction1(X86_64_OP(movxc), GetLoweredNode(addr_node)));
      tp_offset->flags |= X86_64_TLS_RELOC;
      *addr = Emit(rv, NewInstruction2(X86_64_OP(add), ThreadPointer(rv), tp_offset));
      *offset = GetIntConstant(rv, NULL, kTargetType64Bit, 0);
      break;
    }
    default:
      DiagnoseUnsupportedTlsModel(rv, TlsModelName(compiler->tls_model));
      break;
  }
}

// Static varaibles have an address calculated by the linker so at this
// point they are unknown.  We need to load their address into a register.  This
// is done using a la or lla pseudo-instruction.
static TargetInstruction* LoadStaticVariableAddress(X86_64Generator* rv,
                                                    IRNode* node) {
  IRVariable* var = (IRVariable*)node;
  TargetInstruction* inst = Emit(
      rv, NewInstruction1(X86_64_OP(lea_rip), GetLoweredNode(node)));
  if (!var->symbol->flags.is_local && compiler->pic) {
    inst->flags |= X86_64_GOTPCREL_RELOC;
  }
  return inst;
}

// Materialize a value into a register.  This loads a constant into a register
// or returns the instruction associated with the node if it's
// already in a register.
static TargetInstruction* Materialize(X86_64Generator* rv, IRNode* node) {
  if (IRIsConst(node)) {
    switch (node->opcode) {
      case IR_OP(const8):
      case IR_OP(const16):
      case IR_OP(const32):
      case IR_OP(const64):
      case IR_OP(consta):
        if (IRIsZero(node)) {
          // x86-64 has an explicit zero register (x0).  If we are loading
          // the constant zero, just move it into the destination.
          //return Emit(rv, NewInstruction1(X86_64_OP(mv), Zero(rv)));
          return Zero(rv);
        } else {
          return Emit(rv, NewInstruction1(X86_64_OP(mov), GetLoweredNode(node)));
        }
      case IR_OP(constf): {
        // The constant's fvalue field is always in double precision.
        double dvalue = ((IRConstant*)node)->value.fvalue;

        // Convert to single precision.
        float fvalue = (float)dvalue;
        int32_t bits = *(int32_t*)(&fvalue);
        TargetInstruction* c;
        if (bits == 0) {
          c = Zero(rv);
        } else {
          c = Emit(rv, NewInstruction1(
                           X86_64_OP(mov),
                           GetIntConstant(rv, NULL, kTargetType32Bit, bits)));
        }
        return Emit(rv, NewInstruction1(X86_64_OP(movd), c));
      }
      case IR_OP(constd): {
        double value = ((IRConstant*)node)->value.fvalue;
        int64_t bits = *(int64_t*)(&value);
        TargetInstruction* c;
        if (bits == 0) {
          c = Zero(rv);
        } else {
          c = Emit(rv,
                   NewInstruction1(
                       X86_64_OP(mov),
                       GetIntConstant(rv, NULL, kTargetType64Bit, bits)));
        }
        return Emit(rv, NewInstruction1(X86_64_OP(movq_xmm), c));
      }
      default:
        assert(false);
    }
  }
  if (IRIsAutoVariable(node)) {
    IRVariable* var = (IRVariable*)node;
    if (TypeIsVLA(node->type)) {
      IRNode* addr = var->symbol->value.other;
      return GetLoweredNode(addr);
    } else {
      int32_t var_offset = node->data.ivalue;
      if (X86_64_IS_REG_VAR(var_offset)) {
        // Variable is in a register.
        int var_num = var_offset & ~X86_64_REG_VAR;
        if (TypeIsFloatingPoint(node->type)) {
          return FloatingPointVariableRegister(rv, var_num, var->symbol);
        } else {
          return IntVariableRegister(rv, var_num, var->symbol);
        }
      }

      // Auto variable is in the stack frame.  These are accessed through
      // the frame pointer with a negative offset.
      TargetInstruction* addr = FramePointer(rv);
      return OffsetFrom(rv, addr, LocalVariableOffset(rv, var_offset));
    }
  } else if (IRIsArgument(node)) {
    // TODO: structs passed by reference.
    int32_t var_offset = node->data.ivalue;
    if (X86_64_IS_REG_VAR(var_offset)) {
      // Argument is in a register.
      IRVariable* var = (IRVariable*)node;
      int var_num = var_offset & ~X86_64_REG_VAR;
      if (TypeIsFloatingPoint(node->type)) {
        return FloatingPointVariableRegister(rv, var_num, var->symbol);
      } else {
        return IntVariableRegister(rv, var_num, var->symbol);
      }
    } else {
      // Argument is on the stack.
      TargetInstruction* addr = FramePointer(rv);
      int32_t var_offset = node->data.ivalue;
      return OffsetFrom(rv, addr, var_offset);
    }
  } else if (IRIsThreadVariable(node)) {
    return GetTlsVariableAddress(rv, node);
  } else if (IRIsStaticVariable(node)) {
    // The address of static variables need to be moved into a register.

    return LoadStaticVariableAddress(rv, node);
  }

  // Node is not a variable, is must be an already-lowered expression.
  return GetLoweredNode(node);
}

static void ApplyFixups(X86_64Generator* rv, IRNode* label_node) {
  TargetApplyFixups(&rv->base, label_node);
}

static bool IsPowerOf2(int64_t v) {
  return v != 0 && (v & (v - 1)) == 0;
}

// Given a number that is a power of 2, what is the log (base 2) of it.
static int64_t Log2(int64_t v) {
  static const int MultiplyDeBruijnBitPosition2[32] =
  {
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
  };
  return MultiplyDeBruijnBitPosition2[(uint32_t)(v * 0x077CB531U) >> 27];
}

// Count the number of 1 bits in the integer up to maxbits in length.
static int PopulationCount(uint64_t x) {
  int c = 0;
  for (; x != 0; x &= x - 1) {
      c++;
  }
  return c;
}

// It's worth multiplying by a constant using shifts and adds
// if the number of shift and add instructions is less than the number of
// cycles it takes for the mul instruction.
//
// While each x86-64 processor is different, let's assume that the multiplier
// can do 8 bits at a time.  Since this is a 64 bit processor, that's
// 8 cycles to multiply two 64 bit numbers.  Each 1-bit in the constant
// causes the emission of a shift instruction and these need to be added
// together.  Therefore the number of instructions for a n bits is
// n + (n - 1) = 2n-1.  However for bit 0 we don't do the shift but instead
// use the input value directly.
//
// Let's assume that both a slli and an add instruction take 1 cycle.
static TargetInstruction* MultiplyByConstant(X86_64Generator* rv,
                               IRNode* variable,
                               IRConstant* constant) {
  int64_t value = constant->value.ivalue;
  int numbits = PopulationCount(value);
  int num_cycles = numbits * 2 - 1;
  if ((value & 1) == 1) {
    // If the bottom bit is 1 we can subtract one instruction.
    num_cycles--;
  }
  const int kMaxCycles = 8;
  if (num_cycles > kMaxCycles) {
    return NULL;
  }
 
  TargetInstruction* left = NULL;   // Current left instruction.
  TargetInstruction* right = NULL;  // Current right instruction.
  TargetInstruction* input = GetLoweredNode(variable);
  
  int bitpos = 0;
  while (value != 0) {
    if ((value & 1) == 1) {
      // Build a shift by the bitpos.
      TargetInstruction* inst;
      if (bitpos == 0) {
        inst = input;     // Bit 0, use input directly.
      } else {
        // Shift left by the bitpos.
       inst =
           Emit(rv, NewInstruction2(X86_64_OP(shl), input,
                           GetIntConstant(rv, NULL, kTargetType32Bit, bitpos)));
      }
      if (left == NULL) {
        left = inst;
      } else if (right == NULL) {
        right = inst;
      }
      if (left != NULL && right != NULL) {
        // Add left and right together.
        inst = Emit(rv, NewInstruction2(X86_64_OP(add), left, right));
        
        // Left is now the result of the add.  Right is empty.
        left = inst;
        right = NULL;
      }
    }
    bitpos++;
    value >>= 1;
  }
  return left;
}


static TargetInstruction* LowerExpression(X86_64Generator* rv, Generator* gen,
                                          IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  X86_64Opcode opcode = IR2X86_64(node->opcode);
  if (node->type != NULL && node->type->size <= 4) {
    if (node->opcode == IR_OP(addi)) opcode = X86_64_OP(addl);
    if (node->opcode == IR_OP(subi)) opcode = X86_64_OP(subl);
    if (node->opcode == IR_OP(muli)) opcode = X86_64_OP(imull);
    if (opcode == X86_64_OP(rol)) opcode = X86_64_OP(roll);
    if (opcode == X86_64_OP(ror)) opcode = X86_64_OP(rorl);
    if (opcode == X86_64_OP(bsf)) opcode = X86_64_OP(bsfl);
    if (opcode == X86_64_OP(bsr)) opcode = X86_64_OP(bsrl);
  }
  if (node->inputs.length >= 1) {
    IRNode* lhs = node->inputs.value.p[0];
    if (lhs != NULL && lhs->type != NULL && lhs->type->size <= 4) {
      if (node->opcode == IR_OP(muli)) opcode = X86_64_OP(imull);
      if (node->opcode == IR_OP(addi)) opcode = X86_64_OP(addl);
      if (node->opcode == IR_OP(subi)) opcode = X86_64_OP(subl);
    }
  }
  if (node->opcode == IR_OP(divi) && TypeIsUnsigned(node->type)) {
    opcode = X86_64_OP(div);
  }
  assert(node->inputs.length <= 2);
  TargetInstruction* inst = NULL;
  bool ref_counts_ok =
      false;  // True if we don't need to update operand ref counts.
                
  // Do some strength reduction if we can.
  switch (opcode) {
    default:
      // All others are handled below.
      break;
    case X86_64_OP(add): {
      // We have an add with constant instruction.  Use it if we can.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) && IRIsConst(op2)) {
        // Both constant, multiply don't replace.
        break;
      }
      // Adds are commutative so we can have a const as first or
      // second operand.
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (X86_64IsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(X86_64_OP(add));
          inst->operand[0] = Materialize(rv, op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      } else if (IRIsConst(op1)) {
        int64_t c = ((IRConstant*)op1)->value.ivalue;
        if (X86_64IsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(X86_64_OP(add));
          inst->operand[0] = Materialize(rv, op2);
          inst->operand[1] = GetLoweredNode(op1);
        }
      }
      break;
    }

    case X86_64_OP(sub): {
      // A subtract immediate can be done using an addi with the negative of the
      // immediate.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (X86_64IsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(X86_64_OP(add));
          inst->operand[0] = Materialize(rv, op1);
          inst->operand[1] = GetIntConstant(rv, NULL, kTargetType32Bit, -c);
        }
      }
    }
      break;
    case X86_64_OP(shl):
    case X86_64_OP(shr):
    case X86_64_OP(sar):
    case X86_64_OP(rol):
    case X86_64_OP(ror):
    case X86_64_OP(roll):
    case X86_64_OP(rorl): {
      // There are constant shift operations.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        // TODO: what about a shift out of range?
        if (c == 0) {
          // A shift of 0 is a mv.
          inst = NewInstruction(X86_64_OP(mv));
          inst->operand[0] = Materialize(rv, op1);
        } else {
          switch (opcode) {
            case X86_64_OP(shl):
              // TODO: slliw?
              opcode = X86_64_OP(shl);
              break;
            case X86_64_OP(shr):
              opcode = X86_64_OP(shr);
              break;
            case X86_64_OP(sar):
              opcode = X86_64_OP(sar);
              break;
            default:
              break;
          }
          inst = (TargetInstruction*)NewInstruction(opcode);
          inst->operand[0] = Materialize(rv, op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      }
      break;
    }
    case X86_64_OP(imul): {
      // If we are multiplying by a constant we can use shifts and
      // adds.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) || IRIsConst(op2)) {
        if (IRIsConst(op1) && IRIsConst(op2)) {
          break;
        }
        // One is constant, put it on the right of the slli instruction.
        if (IRIsConst(op1)) {
          int64_t c = ((IRConstant*)op1)->value.ivalue;
          if (c == 0) {
            // Multiply by zero is zero
            inst = Zero(rv);
          } else if (c == 1) {
            // Multiply by 1 is a mv.
            inst = NewInstruction(X86_64_OP(mv));
            inst->operand[0] = Materialize(rv, op2);
          } else {
            inst = MultiplyByConstant(rv, op2, (IRConstant*)op1);
            if (inst == NULL) {
              break;
            }
            ref_counts_ok = true;
          }
        } else {
          int64_t c = ((IRConstant*)op2)->value.ivalue;
          if (c == 0) {
            // Multiply by zero is zero
            inst = Zero(rv);
          } else if (c == 1) {
            // Multiply by 1 is a mv.
            inst = NewInstruction(X86_64_OP(mv));
            inst->operand[0] = Materialize(rv, op1);
          } else {
            inst = MultiplyByConstant(rv, op1, (IRConstant*)op2);
            if (inst == NULL) {
              break;
            }
            ref_counts_ok = true;
          }
        }
      }
      break;
    }
    case X86_64_OP(idiv): {
      // If we are dividing by a constant power of 2 we can use a shift for
      // unsigned values.  Signed division must keep idiv: a plain sar
      // truncates toward negative infinity, not zero as C requires.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 1) {
          inst = NewInstruction(X86_64_OP(mv));
          inst->operand[0] = Materialize(rv, op1);
        } else if (TypeIsUnsigned(node->type) && IsPowerOf2(c) && c < 64) {
          inst = NewInstruction2(
              X86_64_OP(shr), Materialize(rv, op1),
              GetIntConstant(rv, NULL, kTargetType32Bit, Log2(c)));
          ref_counts_ok = true;
        }
      }

      break;
    }

    case X86_64_OP(div): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 1) {
          inst = NewInstruction(X86_64_OP(mv));
          inst->operand[0] = Materialize(rv, op1);
        } else if (IsPowerOf2(c) && c < 64) {
          inst = NewInstruction2(
              X86_64_OP(shr), Materialize(rv, op1),
              GetIntConstant(rv, NULL, kTargetType32Bit, Log2(c)));
          ref_counts_ok = true;
        }
      }
      break;
    }

    case X86_64_OP(mod): {
      // If we are moding by a constant power of 2 we can use an AND (unsigned).
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        // TODO: can we give an error on division by zero here?
        if (TypeIsUnsigned(node->type) && IsPowerOf2(c) && c < 64) {
          int64_t mask = c - 1;
          if (X86_64IsPossibleImmediate(mask)) {
            inst = NewInstruction2(X86_64_OP(and), Materialize(rv, op1),
                                   GetIntConstant(rv, NULL, kTargetType32Bit,
                                                  mask));
            ref_counts_ok = true;
          } else {
            TargetInstruction* tmp = Emit(rv, NewInstruction(X86_64_OP(tmp)));
            TargetInstruction* mv = Emit(rv, NewInstruction1(
                                            X86_64_OP(mv),
                                            GetIntConstant(rv, NULL,
                                                           kTargetType32Bit,
                                                           mask)));
            mv->dest = tmp;

            inst = NewInstruction2(X86_64_OP(and), Materialize(rv, op1), tmp);
          }
        }
      }
      break;
    }
      
    case X86_64_OP(and): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 0) {
          // Anding with zero is zero.
          inst = Zero(rv);
        } else if (X86_64IsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(X86_64_OP(and));
          inst->operand[0] = Materialize(rv, op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      }
      break;
    }

    case X86_64_OP(or): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 0) {
          // ORing with zero is nop.
          inst = GetLoweredNode(op1);
        } else if (X86_64IsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(X86_64_OP(or));
          inst->operand[0] = Materialize(rv, op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      }
      break;
    }
  }

  if (inst == NULL) {
    inst = (TargetInstruction*)NewInstruction(opcode);
    for (size_t i = 0; i < node->inputs.length; i++) {
      IRNode* input = node->inputs.value.p[i];
      inst->operand[i] = Materialize(rv, input);
    }
    if (opcode == X86_64_OP(mod) && TypeIsUnsigned(node->type)) {
      inst->flags |= X86_64_UNSIGNED_MOD;
    }
  }
  if (!ref_counts_ok) {
    TargetUpdateOperandUsers(inst);
  }
  TargetInstruction* dest = GetDestInstruction(rv, gen, node);
  if (dest != NULL && inst != NULL) {
    X86_64Opcode mov_opcode = X86_64_OP(mv);
    if (TypeIsFloatingPoint(node->type)) {
      mov_opcode = node->type->size > 4 ? X86_64_OP(fmv_d) : X86_64_OP(fmv_s);
    }
    inst = SetDestOrMove(rv, inst, dest, mov_opcode);
    ref_counts_ok = true;
  }
  SetLoweredNode(node, inst);
  return Emit(rv, inst);
}

static COMPILER_UNUSED TargetInstruction* LowerRmov(X86_64Generator* rv, IRNode* node) {
  TargetInstruction* dest = GetLoweredNode(node->inputs.value.p[0]);
  TargetInstruction* src = Materialize(rv, node->inputs.value.p[1]);
  X86_64Opcode opcode = IR2X86_64(node->opcode);
  return SetLoweredNode(node, SetDestOrMove(rv, src, dest, opcode));
}

// There is no compare for equality instruction, it must be synthesized
// using a subtract and compare against zero.  This function
// performs the subtraction.  If it returns NULL there is no subtraction
// and the pseudo-instruction to compare against zero is used directly.
static TargetInstruction* SubtractForComparison(X86_64Generator* rv, IRNode* node,
                                                IRNode* op1, IRNode* op2) {
  if (!IRIsConst(op2)) {
    // Second operand isn't constant, compiled as sub.
    return Emit(rv, NewInstruction2(X86_64_OP(sub), Materialize(rv, op1),
                                    Materialize(rv, op2)));
  }

  // Second operand is constant.
  int64_t value = ((IRConstant*)op2)->value.ivalue;
  if (value == 0) {
    // Common case, compare with zero, no subtract.
    return NULL;
  }

  if (X86_64IsPossibleImmediate(value)) {
    // There is no subi instruction, so we have to use an addi
    // with the negative of the immediate.
    return AddImmediate(rv, Materialize(rv, op1), -value);
  }

  // Constant is too big for an immediate, materialize it into
  // a register and use a sub instruction.
  return Emit(rv, NewInstruction2(X86_64_OP(sub), Materialize(rv, op1),
                                  Materialize(rv, op2)));
}

// True for the unsigned integer comparison IR opcodes (the "a" suffixed forms
// behave as unsigned / "above-below").  Signed comparisons use setl, unsigned
// ones must use setb so the value-producing form matches the semantics.
static bool ComparisonIsUnsigned(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(cmplta):
    case IR_OP(cmplea):
    case IR_OP(cmpgta):
    case IR_OP(cmpgea):
      return true;
    case IR_OP(cmplti):
    case IR_OP(cmplei):
    case IR_OP(cmpgti):
    case IR_OP(cmpgei):
      if (node->inputs.length > 0) {
        IRNode* op = node->inputs.value.p[0];
        return TypeIsUnsigned(op->type);
      }
      return false;
    default:
      return false;
  }
}

static X86_64Opcode UnsignedBranchForComparison(X86_64Opcode branch) {
  switch (branch) {
    case X86_64_OP(jl):
      return X86_64_OP(jb);
    case X86_64_OP(jge):
      return X86_64_OP(jae);
    default:
      return branch;
  }
}

// Compare integers for less than (op1 < op2), producing a 0/1 value.  Uses
// setl for signed comparisons and setb for unsigned.  The set* lowering takes
// operand[0] as the left operand and operand[1] as the right (see
// PrintCompareAndSet); a single operand means "compare against zero".
static TargetInstruction* CompareLessThanInt(X86_64Generator* rv, IRNode* node,
                                             IRNode* op1, IRNode* op2) {
  X86_64Opcode setcc =
      ComparisonIsUnsigned(node) ? X86_64_OP(setb) : X86_64_OP(setl);
  if (!IRIsConst(op2)) {
    // Second operand isn't constant, compiled as slt.
    TargetInstruction* i1 = Materialize(rv, op1);
    TargetInstruction* i2 = Materialize(rv, op2);
    return Emit(rv, NewInstruction2(setcc, i1, i2));
  }

  // Second operand is constant.
  int64_t value = ((IRConstant*)op2)->value.ivalue;
  if (value == 0) {
    // Common case, compare with zero, use sltz.
    return Emit(rv, NewInstruction1(setcc, Materialize(rv, op1)));
  }

  if (X86_64IsPossibleImmediate(value)) {
    return Emit(
        rv, NewInstruction2(setcc, Materialize(rv, op1),
                            GetIntConstant(rv, NULL, kTargetType32Bit, value)));
  }

  // Constant is too big for an immediate, materialize it into
  // a register and use an slt instruction.
  return Emit(rv, NewInstruction2(setcc, Materialize(rv, op1),
                                  Materialize(rv, op2)));
}

// Logical NOT (!x).  This is distinct from one's-complement (~x): the result
// is 1 when the operand is zero and 0 otherwise.  Materialize it as
// "test x, x ; sete rd" rather than the bitwise notq used by onescomp.  The
// single-operand sete form emits "test op,op" before the set (see
// PrintCompareAndSet), and setcc results are zero-extended to the full
// register so the boolean is a clean 0/1.
// Route a freshly produced boolean (comparison or logical-not) result into the
// node's destination register if it has one.  Comparison/logical-not results
// are frequently merged into a value via && / || / ?: (the IR marks this with
// "-> $dest"); without this the result is left in a scratch register and the
// destination is never written.
static TargetInstruction* RouteResultToDest(X86_64Generator* rv, Generator* gen,
                                            IRNode* node,
                                            TargetInstruction* result) {
  TargetInstruction* dest = GetDestInstruction(rv, gen, node);
  if (dest != NULL && result != NULL) {
    result = SetDestOrMove(rv, result, dest, X86_64_OP(mv));
    SetLoweredNode(node, result);
  }
  return result;
}

static TargetInstruction* LowerLogicalNot(X86_64Generator* rv, Generator* gen,
                                          IRNode* node) {
  IRNode* op = node->inputs.value.p[0];
  // Floating-point logical-not is uncommon and the operand lives in an XMM
  // register, so fall back to the generic lowering for it.  Integer/pointer
  // logical-not is "test op,op ; sete rd" (the single-operand sete form emits
  // the test, and setcc results are zero-extended to a clean 0/1).
  if (TypeIsFloatingPoint(op->type)) {
    return LowerExpression(rv, gen, node);
  }
  TargetInstruction* result = Emit(
      rv, SetLoweredNode(node,
                         NewInstruction1(X86_64_OP(sete), Materialize(rv, op))));
  return RouteResultToDest(rv, gen, node, result);
}

// Comparisons set the result register to 1 or 0.  The result of integer
// comparisons is usually used as the input to a conditional branch.  The
// x86-64 has combined compare and branch instructions so we can generally
// eliminate the integer comparisons.  This will be done when we lower
// the conditional branch.  However we still need to generate the correct
// result because the result might not be used in a branch.
//
// ucomi sets CF for both less-than and unordered operands. Keep this primitive
// raw, then combine comparisons in both directions to distinguish NaNs without
// relying on PF surviving through the target IR.
static TargetInstruction* FloatCompareRawLess(X86_64Generator* rv, IRNode* a,
                                              IRNode* b, bool is_double) {
  TargetInstruction* inst =
      NewInstruction2(X86_64_OP(setb), Materialize(rv, a), Materialize(rv, b));
  inst->flags |= is_double ? X86_64_FCMP_SD : X86_64_FCMP_SS;
  return Emit(rv, inst);
}

static TargetInstruction* FloatCompareEqual(X86_64Generator* rv, IRNode* node,
                                            IRNode* a, IRNode* b,
                                            bool is_double, bool negate) {
  TargetInstruction* ab = FloatCompareRawLess(rv, a, b, is_double);
  TargetInstruction* ba = FloatCompareRawLess(rv, b, a, is_double);
  TargetInstruction* different_or_unordered =
      Emit(rv, NewInstruction2(X86_64_OP(or), ab, ba));
  TargetInstruction* result = different_or_unordered;
  if (!negate) {
    result = Emit(rv, NewInstruction2(
                          X86_64_OP(xor), result,
                          GetIntConstant(rv, NULL, kTargetType32Bit, 1)));
  }
  return SetLoweredNode(node, result);
}

static TargetInstruction* FloatCompareLess(X86_64Generator* rv, IRNode* node,
                                           IRNode* a, IRNode* b,
                                           bool is_double) {
  TargetInstruction* ab = FloatCompareRawLess(rv, a, b, is_double);
  TargetInstruction* ba = FloatCompareRawLess(rv, b, a, is_double);
  TargetInstruction* not_ba =
      Emit(rv, NewInstruction2(
                   X86_64_OP(xor), ba,
                   GetIntConstant(rv, NULL, kTargetType32Bit, 1)));
  return SetLoweredNode(
      node, Emit(rv, NewInstruction2(X86_64_OP(and), ab, not_ba)));
}

// a <= b is computed as !(b < a) and a >= b as !(a < b): produce (a < b) with
// setb then logically invert the boolean with "xor $1" (matching the integer
// <=/>= lowering, see cmplei/cmpgei above).
static TargetInstruction* FloatCompareInvert(X86_64Generator* rv, IRNode* node,
                                             IRNode* a, IRNode* b,
                                             bool is_double) {
  TargetInstruction* lt = FloatCompareRawLess(rv, a, b, is_double);
  return Emit(rv, SetLoweredNode(
                      node, NewInstruction2(
                                X86_64_OP(xor), lt,
                                GetIntConstant(rv, NULL, kTargetType32Bit, 1))));
}

// Floating point comparisons do exist but the only conditions are EQ/LT/LE.
// We need to reverse the operands to perform the other conditions.
static TargetInstruction* LowerComparison(X86_64Generator* rv, IRNode* node) {
  assert(node->inputs.length == 2);
  IRNode* op1 = node->inputs.value.p[0];
  IRNode* op2 = node->inputs.value.p[1];
  switch (node->opcode) {
    case IR_OP(cmpeqi):
    case IR_OP(cmpeqa): {
      // Subtract two ops and compare against zero.
      TargetInstruction* sub = SubtractForComparison(rv, node, op1, op2);
      if (sub == NULL) {
        return Emit(
            rv, SetLoweredNode(
                    node, NewInstruction1(X86_64_OP(sete), Materialize(rv, op1))));
      } else {
        return Emit(rv,
                    SetLoweredNode(node, NewInstruction1(X86_64_OP(sete), sub)));
      }
    }
    case IR_OP(cmpnei):
    case IR_OP(cmpnea): {
      // Subtract two ops and compare against zero.
      TargetInstruction* sub = SubtractForComparison(rv, node, op1, op2);
      if (sub == NULL) {
        return Emit(
            rv, SetLoweredNode(
                    node, NewInstruction1(X86_64_OP(setne), Materialize(rv, op1))));
      } else {
        return Emit(rv,
                    SetLoweredNode(node, NewInstruction1(X86_64_OP(setne), sub)));
      }
    }
    case IR_OP(cmplti):
    case IR_OP(cmplta):
      // Compare using slt/slti.
      return SetLoweredNode(node, CompareLessThanInt(rv, node, op1, op2));

    case IR_OP(cmplei):
    case IR_OP(cmplea): {
      // a <= b  ==  !(b < a).  Compute (b < a) then logically invert the 0/1
      // result with "xor $1" (bitwise notq would yield -1/-2, not a boolean).
      TargetInstruction* slt = CompareLessThanInt(rv, node, op2, op1);
      return Emit(rv, SetLoweredNode(
                          node, NewInstruction2(
                                    X86_64_OP(xor), slt,
                                    GetIntConstant(rv, NULL, kTargetType32Bit, 1))));
    }

    case IR_OP(cmpgti):
    case IR_OP(cmpgta):
      // Use op2 < op1.
      return SetLoweredNode(node, CompareLessThanInt(rv, node, op2, op1));

    case IR_OP(cmpgei):
    case IR_OP(cmpgea): {
      // a >= b  ==  !(a < b).  Compute (a < b) then logically invert the 0/1
      // result with "xor $1" (bitwise notq would yield -1/-2, not a boolean).
      TargetInstruction* slt = CompareLessThanInt(rv, node, op1, op2);
      return Emit(rv, SetLoweredNode(
                          node, NewInstruction2(
                                    X86_64_OP(xor), slt,
                                    GetIntConstant(rv, NULL, kTargetType32Bit, 1))));
    }

    case IR_OP(cmpeqf):
      return FloatCompareEqual(rv, node, op1, op2, false, false);
    case IR_OP(cmpnef):
      return FloatCompareEqual(rv, node, op1, op2, false, true);
    case IR_OP(cmpltf):
      return FloatCompareLess(rv, node, op1, op2, false);
    case IR_OP(cmpgtf):
      return FloatCompareLess(rv, node, op2, op1, false);
    case IR_OP(cmplef):
      // a <= b  ==  !(b < a): compute (b < a) then invert the 0/1 result.
      return FloatCompareInvert(rv, node, op2, op1, false);
    case IR_OP(cmpgef):
      // a >= b  ==  !(a < b): compute (a < b) then invert the 0/1 result.
      return FloatCompareInvert(rv, node, op1, op2, false);

    case IR_OP(cmpeqd):
      return FloatCompareEqual(rv, node, op1, op2, true, false);
    case IR_OP(cmpned):
      return FloatCompareEqual(rv, node, op1, op2, true, true);
    case IR_OP(cmpltd):
      return FloatCompareLess(rv, node, op1, op2, true);
    case IR_OP(cmpgtd):
      return FloatCompareLess(rv, node, op2, op1, true);
    case IR_OP(cmpled):
      return FloatCompareInvert(rv, node, op2, op1, true);
    case IR_OP(cmpged):
      return FloatCompareInvert(rv, node, op1, op2, true);
    default:
      assert(false);
  }
  return NULL;
}

// Lower a three-way comparison (operator<=> on scalars) to an integer result
// of -1/0/1 (and 2 = unordered for floating point).  For any operand pair we
// compute p = (a < b) and q = (b < a) as 0/1 booleans, then form
//   (q - p) + ((p & q) << 1).
// For totally-ordered operands p & q is always 0, so the result is the usual
// (a > b) - (a < b) == -1/0/1.  For floating point, ucomiSS/SD sets CF for both
// the less-than and (reversed) greater-than test when the operands are
// unordered (a NaN is involved), making p & q == 1 and contributing the +2.
static TargetInstruction* LowerThreeWay(X86_64Generator* rv, IRNode* node) {
  assert(node->inputs.length == 2);
  IRNode* a = node->inputs.value.p[0];
  IRNode* b = node->inputs.value.p[1];
  bool is_float = node->opcode == IR_OP(cmp3wayf);
  bool is_double = node->opcode == IR_OP(cmp3wayd);
  bool is_fp = is_float || is_double;
  // Signed integers use setl; unsigned ints, pointers and floats (via ucomi's
  // CF) all use setb.
  X86_64Opcode setcc =
      node->opcode == IR_OP(cmp3wayi) ? X86_64_OP(setl) : X86_64_OP(setb);
  TargetInstruction* ma = Materialize(rv, a);
  TargetInstruction* mb = Materialize(rv, b);
  TargetInstruction* lt = NewInstruction2(setcc, ma, mb);  // a < b
  TargetInstruction* gt = NewInstruction2(setcc, mb, ma);  // b < a
  if (is_fp) {
    lt->flags |= is_double ? X86_64_FCMP_SD : X86_64_FCMP_SS;
    gt->flags |= is_double ? X86_64_FCMP_SD : X86_64_FCMP_SS;
  }
  lt = Emit(rv, lt);
  gt = Emit(rv, gt);
  TargetInstruction* diff = Emit(rv, NewInstruction2(X86_64_OP(sub), gt, lt));
  if (!is_fp) {
    return SetLoweredNode(node, diff);
  }
  TargetInstruction* both = Emit(rv, NewInstruction2(X86_64_OP(and), lt, gt));
  TargetInstruction* twice =
      Emit(rv, NewInstruction2(X86_64_OP(add), both, both));
  return SetLoweredNode(node, Emit(rv, NewInstruction2(X86_64_OP(add), diff,
                                                       twice)));
}

static void GetAddressAndOffsetFrom(X86_64Generator* rv,
                                 TargetInstruction* addr,
                                 int offset,
                                 TargetInstruction** addr_inst,
                                 TargetInstruction** offset_inst) {
  if (X86_64IsPossibleImmediate(offset)) {
    *addr_inst = addr;
    *offset_inst = GetIntConstant(rv, NULL, kTargetType32Bit, offset);
    return;
  }
  int page_offset;
  TargetInstruction* page_inst = PagedOffsetFrom(rv, addr, offset, &page_offset);
  *addr_inst = page_inst;
  *offset_inst = GetIntConstant(rv, NULL, kTargetType32Bit, page_offset);
}

static bool GetRegAndOffset(X86_64Generator* rv, IRNode* addr_node,
                            TargetInstruction** addr,
                            TargetInstruction** offset) {
  if (IRIsAutoVariable(addr_node)) {
    IRVariable* var = (IRVariable*)addr_node;
    if (TypeIsVLA(var->symbol->type)) {
      IRNode* vla_addr = var->symbol->value.other;
      *addr = GetLoweredNode(vla_addr);
      *offset = Zero(rv);
      return false;
    }
    if ((addr_node->flags & kIRNrvoMarker) != 0) {
      // NRVO variable.
      int reg = addr_node->data.ivalue & ~X86_64_REG_VAR;
      *addr = IntVariableRegister(rv, reg, var->symbol);
      *offset = Zero(rv);
      return true;
    }
    int32_t var_offset = addr_node->data.ivalue;
    if (X86_64_IS_REG_VAR(var_offset)) {
      // Variable is in a register.
      int var_num = var_offset & ~X86_64_REG_VAR;
      if (TypeIsFloatingPoint(addr_node->type)) {
        *addr = FloatingPointVariableRegister(rv, var_num, var->symbol);
      } else {
        *addr = IntVariableRegister(rv, var_num, var->symbol);
      }
      *offset = NULL;
      return false;
    }

    // Auto variable is in the stack frame.  These are accessed through
    // the frame pointer with a negative offset.
    GetAddressAndOffsetFrom(rv, FramePointer(rv), LocalVariableOffset(rv, var_offset),
                            addr, offset);
  } else if (IRIsArgument(addr_node)) {
    int32_t var_offset = addr_node->data.ivalue;
    IRVariable* var = (IRVariable*)addr_node;
    if (X86_64_IS_REG_VAR(var_offset)) {
      // Argument is in a register.
      int var_num = var_offset & ~X86_64_REG_VAR;
      if (TypeIsFloatingPoint(addr_node->type)) {
        *addr = FloatingPointVariableRegister(rv, var_num, var->symbol);
      } else {
        *addr = IntVariableRegister(rv, var_num, var->symbol);
      }
      *offset = NULL;
      return false;
    } else {
      GetAddressAndOffsetFrom(rv, FramePointer(rv), var_offset,
                              addr, offset);
     }
  } else if (IRIsThreadVariable(addr_node)) {
    GetTlsAddressAndOffset(rv, addr_node, addr, offset);
    return true;
  } else if (IRIsStaticVariable(addr_node)) {
    // The address of static variables need to be moved into a register.

    *addr = LoadStaticVariableAddress(rv, addr_node);
    *offset = GetIntConstant(rv, NULL, kTargetType32Bit, 0);
  } else {
    // All others have a calculated address.
    *addr = GetLoweredNode(addr_node);
    *offset = Zero(rv);
    assert(addr != NULL);
  }
  return true;
}

static TargetInstruction* Load(X86_64Generator* rv, IRNode* addr_node, X86_64Opcode opcode) {
  TargetInstruction* addr;
  TargetInstruction* offset;
  bool on_stack = GetRegAndOffset(rv, addr_node, &addr, &offset);

  if (!on_stack) {
    return addr;
  }

  TargetInstruction* result = NULL;
  if ((X86_64Opcode)((int)addr->opcode == (int)X86_64_OP(add)) &&
      TargetIsZero(offset)) {
    // If the address is calculated using an addi instruction we can
    // combine the immediate from the addi with the load.
    // The addi instruction will no longer be used and will be eliminated
    // during the optimization pass.
    TargetInstruction* src = addr->operand[0];
    TargetInstruction* immed = addr->operand[1];
    assert(src != NULL);
    assert(immed != NULL);
    if ((X86_64Opcode)immed->opcode == X86_64_OP(movxc) &&
        (immed->flags & X86_64_TLS_RELOC) != 0) {
      result = Emit(rv, NewInstruction2(opcode, src, immed));
    } else {
      assert(TargetIsConst(immed));
      result = Emit(rv, NewInstruction2(opcode, src, immed));
    }
  }
  if (result == NULL) {
    result = Emit(rv, NewInstruction2(opcode, addr, offset));
  }
  return result;
}

static TargetInstruction* LowerLoad(X86_64Generator* rv, Generator* gen,
                                    IRNode* node) {
  X86_64Opcode opcode;
  assert(node->inputs.length == 1);
  IRNode* addr_node = node->inputs.value.p[0];
  
  switch (node->opcode) {
    case IR_OP(load32):
      opcode = X86_64_OP(loadl);
      break;
    case IR_OP(load8):
      opcode = X86_64_OP(loadb);
      break;
    case IR_OP(load64):
      opcode = X86_64_OP(loadq);
      break;
    case IR_OP(load16):
      opcode = X86_64_OP(loadw);
      break;
    case IR_OP(loadu32):
      opcode = X86_64_OP(loadl_z);
      break;
    case IR_OP(loadu8):
      opcode = X86_64_OP(loadb_z);
      break;
    case IR_OP(loadu16):
      opcode = X86_64_OP(loadw_z);
      break;
    case IR_OP(loadf):
      opcode = X86_64_OP(loadss);
      break;
    case IR_OP(loadd):
      opcode = X86_64_OP(loadsd);
      break;
    case IR_OP(loada):
      opcode = X86_64_OP(loadq);
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }

  TargetInstruction* result = Load(rv, addr_node, opcode);
  TargetInstruction* dest = GetDestInstruction(rv, gen, node);
  if (dest != NULL) {
    result = SetDestOrMove(rv, result, dest, MoveOpcodeForLoad(opcode));
  }
  return SetLoweredNode(node, result);
}

static TargetInstruction* Store(X86_64Generator* rv, IRNode* addr_node, TargetInstruction* src, X86_64Opcode opcode) {
  TargetInstruction* addr;
  TargetInstruction* offset;
  bool on_stack = GetRegAndOffset(rv, addr_node, &addr, &offset);
  // addr is a register.
  // if the variable is in memory offset will be an integer constant
  // containing the offset.  Otherwise it is NULL.

  // If we are not on the stack, move the src to the dest.
  if (!on_stack) {
    X86_64Opcode opcode = TypeIsFloatingPoint(addr_node->type) ? X86_64_OP(fmv_d) : X86_64_OP(mv);
    TargetInstruction* result = SetDestOrMove(rv, src, addr, opcode);
    return result;
  }

  return Emit(rv, NewInstruction3(opcode, src, addr, offset));
}

static TargetInstruction* LowerStore(X86_64Generator* rv, IRNode* node) {
  X86_64Opcode opcode;
  assert(node->inputs.length == 2);

  // Address to store to is the first operand of the store IR node.
  IRNode* addr_node = node->inputs.value.p[0];

  // Value to store is in second input.
  IRNode* src_node = node->inputs.value.p[1];

  // Work out store opcode.
  switch (node->opcode) {
    case IR_OP(store32):
      opcode = X86_64_OP(storel);
      break;
    case IR_OP(store8):
      opcode = X86_64_OP(storeb);
      break;
    case IR_OP(store64):
      opcode = X86_64_OP(storeq);
      break;
    case IR_OP(store16):
      opcode = X86_64_OP(storew);
      break;
    case IR_OP(storef):
      opcode = X86_64_OP(storess);
      break;
    case IR_OP(stored):
      opcode = X86_64_OP(storesd);
      break;
    case IR_OP(storea):
      opcode = X86_64_OP(storeq);
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }
  TargetInstruction* src = Materialize(rv, src_node);

  return SetLoweredNode(node, Store(rv, addr_node, src, opcode));
}

static X86_64Opcode AtomicLoadOpcode(TypeRecord* type) {
  if (TypeIsCharFamily(type)) {
    return TypeIsUnsigned(type) ? X86_64_OP(loadb_z) : X86_64_OP(loadb);
  }
  if (TypeIsShort(type)) {
    return TypeIsUnsigned(type) ? X86_64_OP(loadw_z) : X86_64_OP(loadw);
  }
  if (TypeIsLongLong(type) || TypeIsPointerOrArray(type)) {
    return X86_64_OP(loadq);
  }
  if (TypeIsFloat(type)) {
    return X86_64_OP(loadss);
  }
  if (X86_64FpIsDoubleWidth(type)) {
    return X86_64_OP(loadsd);
  }
  return X86_64_OP(loadl);
}

static X86_64Opcode AtomicStoreOpcode(TypeRecord* type) {
  if (TypeIsCharFamily(type)) {
    return X86_64_OP(storeb);
  }
  if (TypeIsShort(type)) {
    return X86_64_OP(storew);
  }
  if (TypeIsLongLong(type) || TypeIsPointerOrArray(type)) {
    return X86_64_OP(storeq);
  }
  if (TypeIsFloat(type)) {
    return X86_64_OP(storess);
  }
  if (X86_64FpIsDoubleWidth(type)) {
    return X86_64_OP(storesd);
  }
  return X86_64_OP(storel);
}

static int AtomicIRConstant(IRNode* node) {
  assert(node != NULL && IRIsConst(node));
  return (int)((IRConstant*)node)->value.ivalue;
}

static TargetInstruction* EmitAtomicFence(X86_64Generator* rv) {
  TargetInstruction* fence = NewInstruction(X86_64_OP(nop));
  fence->flags |= X86_64_MFENCE;
  return Emit(rv, fence);
}

static TargetInstruction* LowerAtomicLoad(X86_64Generator* rv, Generator* gen,
                                          IRNode* node) {
  TargetInstruction* result =
      Load(rv, node->inputs.value.p[0], AtomicLoadOpcode(node->type));
  TargetInstruction* dest = GetDestInstruction(rv, gen, node);
  if (dest != NULL) {
    result = SetDestOrMove(rv, result, dest,
                           MoveOpcodeForLoad(AtomicLoadOpcode(node->type)));
  }
  return SetLoweredNode(node, result);
}

static TargetInstruction* LowerAtomicStore(X86_64Generator* rv, IRNode* node) {
  IRNode* addr_node = node->inputs.value.p[0];
  IRNode* src_node = node->inputs.value.p[1];
  TargetInstruction* src = Materialize(rv, src_node);
  TargetInstruction* store =
      Store(rv, addr_node, src, AtomicStoreOpcode(src_node->type));
  if (AtomicIRConstant(node->inputs.value.p[2]) == 5) {
    EmitAtomicFence(rv);
  }
  return SetLoweredNode(node, store);
}

static TargetInstruction* LowerAtomicFetchAddSub(X86_64Generator* rv,
                                                 Generator* gen, IRNode* node,
                                                 bool add, bool return_new) {
  IRNode* addr_node = node->inputs.value.p[0];
  IRNode* value_node = node->inputs.value.p[1];
  TargetInstruction* addr = Materialize(rv, addr_node);
  TargetInstruction* value = Materialize(rv, value_node);
  TargetInstruction* delta =
      Emit(rv, NewInstruction1(add ? X86_64_OP(mv) : X86_64_OP(neg), value));
  TargetInstruction* atomic =
      NewInstruction2(X86_64_OP(atomic_fetch_add_sub), addr, delta);
  atomic->dest = delta;
  int size_log2 = node->type->size == 1 ? 0
                  : node->type->size == 2 ? 1
                  : node->type->size == 4 ? 2
                                          : 3;
  atomic->flags |= size_log2 << X86_64_ATOMIC_SIZE_SHIFT;
  Emit(rv, atomic);
  TargetInstruction* result = atomic;
  if (return_new) {
    result = Emit(
        rv, NewInstruction2((TypeIsLongLong(node->type) ||
                             TypeIsPointerOrArray(node->type))
                                ? (add ? X86_64_OP(add) : X86_64_OP(sub))
                                : (add ? X86_64_OP(addl) : X86_64_OP(subl)),
                            atomic, value));
  }
  TargetInstruction* dest = GetDestInstruction(rv, gen, node);
  if (dest != NULL) {
    result = SetDestOrMove(rv, result, dest,
                           MoveOpcodeForLoad(AtomicLoadOpcode(node->type)));
  }
  return SetLoweredNode(node, result);
}

static TargetInstruction* LowerAtomicCompareExchange(X86_64Generator* rv,
                                                     Generator* gen,
                                                     IRNode* node,
                                                     bool expected_is_pointer,
                                                     bool returns_bool) {
  IRNode* addr_node = node->inputs.value.p[0];
  TypeRecord* value_type = addr_node->type->next;
  assert(value_type != NULL &&
         (value_type->size == 1 || value_type->size == 2 ||
          value_type->size == 4 || value_type->size == 8));

  TargetInstruction* addr = Materialize(rv, addr_node);
  TargetInstruction* expected_ptr = NULL;
  TargetInstruction* expected;
  if (expected_is_pointer) {
    expected_ptr = Materialize(rv, node->inputs.value.p[1]);
    expected = Load(rv, node->inputs.value.p[1],
                    AtomicLoadOpcode(value_type));
  } else {
    expected = Materialize(rv, node->inputs.value.p[1]);
  }
  TargetInstruction* desired = Materialize(rv, node->inputs.value.p[2]);

  // CMPXCHG has an architectural accumulator operand.  Make the dependency
  // explicit so register allocation preserves/evicts RAX correctly.
  TargetInstruction* accumulator =
      EmitSymbol(rv, NewInstruction(X86_64_OP(resulti)));
  SetDestOrMove(rv, expected, accumulator, X86_64_OP(mv));

  X86_64Opcode opcode =
      expected_is_pointer
          ? X86_64_OP(atomic_compare_exchange_n)
          : (returns_bool ? X86_64_OP(atomic_compare_exchange_bool)
                          : X86_64_OP(atomic_compare_exchange_val));
  TargetInstruction* inst =
      NewInstruction3(opcode, addr, desired, expected_ptr);
  int size_log2 = value_type->size == 1 ? 0
                  : value_type->size == 2 ? 1
                  : value_type->size == 4 ? 2
                                          : 3;
  inst->flags |= size_log2 << X86_64_ATOMIC_SIZE_SHIFT;
  Emit(rv, inst);
  TargetInstruction* dest = GetDestInstruction(rv, gen, node);
  if (dest != NULL) {
    inst = SetDestOrMove(rv, inst, dest, X86_64_OP(mv));
  }
  return SetLoweredNode(node, inst);
}

static struct BranchInfo {
  IROpcode cmp;     // IR comparison opcode.
  bool btrue;       // IR branch was btrue.
  X86_64Opcode branch;  // Branch opcode.
  bool reverse;     // Reverse operands.
} branch_compare_ops[] = {
    // The backend only has jl/jge (signed) and jb/jae (unsigned) conditional
    // branches; <= and > are expressed by reversing the operands.  For
    // (a <= b): a <= b  <=>  b >= a, and !(a <= b) == (a > b)  <=>  (b < a),
    // both obtained by swapping the operands (reverse=true).
    {IR_OP(cmpeqi), true, X86_64_OP(je), false},
    {IR_OP(cmpnei), true, X86_64_OP(jne), false},
    {IR_OP(cmplti), true, X86_64_OP(jl), false},
    {IR_OP(cmplei), true, X86_64_OP(jge), true},
    {IR_OP(cmpgti), true, X86_64_OP(jl), true},
    {IR_OP(cmpgei), true, X86_64_OP(jge), false},

    {IR_OP(cmpeqa), true, X86_64_OP(je), false},
    {IR_OP(cmpnea), true, X86_64_OP(jne), false},
    {IR_OP(cmplta), true, X86_64_OP(jb), false},
    {IR_OP(cmplea), true, X86_64_OP(jae), true},
    {IR_OP(cmpgta), true, X86_64_OP(jb), true},
    {IR_OP(cmpgea), true, X86_64_OP(jae), false},

    {IR_OP(cmpeqi), false, X86_64_OP(jne), false},
    {IR_OP(cmpnei), false, X86_64_OP(je), false},
    {IR_OP(cmplti), false, X86_64_OP(jge), false},
    {IR_OP(cmplei), false, X86_64_OP(jl), true},
    {IR_OP(cmpgti), false, X86_64_OP(jge), true},
    {IR_OP(cmpgei), false, X86_64_OP(jl), false},

    {IR_OP(cmpeqa), false, X86_64_OP(jne), false},
    {IR_OP(cmpnea), false, X86_64_OP(je), false},
    {IR_OP(cmplta), false, X86_64_OP(jae), false},
    {IR_OP(cmplea), false, X86_64_OP(jb), true},
    {IR_OP(cmpgta), false, X86_64_OP(jae), true},
    {IR_OP(cmpgea), false, X86_64_OP(jb), false},
};

#define NUM_BRANCH_COMPARES \
  (sizeof(branch_compare_ops) / sizeof(branch_compare_ops[0]))

static TargetInstruction* LowerConditionalBranch(X86_64Generator* rv,
                                                 IRNode* node) {
  // CFG cleanup can detach the target from a branch in an unreachable block
  // while leaving the dead instruction in the linear IR list.  It has no
  // executable effect and must not be lowered as a live branch.
  if (node->inputs.length < 2) {
    return Emit(rv, NewInstruction(X86_64_OP(nop)));
  }
  IRNode* expr = node->inputs.value.p[0];
  IRNode* target_node = node->inputs.value.p[1];
  bool btrue = node->opcode == IR_OP(btrue);

  // The x86-64 has 3 operand integer compare and branch instructions.
  // For these we can remove the comparison instructions and combine
  // them with the branch.

  // A comparison that also materializes its boolean into a shared
  // short-circuit/conditional destination may overwrite one of the original
  // comparison operands before this branch executes.  Equality comparisons
  // are lowered through a subtraction followed by sete/setne; branch on that
  // subtraction result so it stays live across the destination write.
  if (expr->dest != NULL &&
      (expr->opcode == IR_OP(cmpeqi) || expr->opcode == IR_OP(cmpnei) ||
       expr->opcode == IR_OP(cmpeqa) || expr->opcode == IR_OP(cmpnea))) {
    TargetInstruction* comparison = GetLoweredNode(expr);
    if (comparison != NULL && comparison->operand[0] != NULL &&
        ((X86_64Opcode)comparison->opcode == X86_64_OP(sete) ||
         (X86_64Opcode)comparison->opcode == X86_64_OP(setne))) {
      bool true_when_zero =
          (X86_64Opcode)comparison->opcode == X86_64_OP(sete);
      X86_64Opcode branch =
          btrue == true_when_zero ? X86_64_OP(jz) : X86_64_OP(jnz);
      TargetInstruction* inst =
          Emit(rv, NewInstruction1(branch, comparison->operand[0]));
      TargetInstruction* target = target_node->data.ptr;
      if (target == NULL) {
        VectorAppend(&rv->base.fixups,
                     NewBranchFixup(inst, target_node, 1));
      } else {
        inst->operand[1] = target;
      }
      return inst;
    }
  }

  struct BranchInfo* branch_info = NULL;
  for (size_t i = 0; i < NUM_BRANCH_COMPARES; i++) {
    if (branch_compare_ops[i].cmp == expr->opcode &&
        branch_compare_ops[i].btrue == btrue) {
      branch_info = &branch_compare_ops[i];
      break;
    }
  }

  if (branch_info != NULL) {
    // Found an integer comparison as the expression for the
    // branch.  The comparison will be eliminated later if it is
    // only used in this branch.
    IRNode* op1 = expr->inputs.value.p[0];
    IRNode* op2 = expr->inputs.value.p[1];
    X86_64Opcode branch_opcode = branch_info->branch;
    if (ComparisonIsUnsigned(expr)) {
      branch_opcode = UnsignedBranchForComparison(branch_opcode);
    }

    // There are beqz and bnez pseudo-instructions for comparing against zero.
    // Use them if possible.
    if ((IRIsZero(op1) || IRIsZero(op2)) &&
        (branch_opcode == X86_64_OP(je) ||
         branch_opcode == X86_64_OP(jne))) {
      X86_64Opcode branch =
          branch_opcode == X86_64_OP(je) ? X86_64_OP(jz) : X86_64_OP(jnz);

      // Put the zero in operand 2.
      if (IRIsZero(op1)) {
        // Swap op1 and op2.
        IRNode* tmp = op1;
        op1 = op2;
        op2 = tmp;
      }
      TargetInstruction* inst =
          Emit(rv, NewInstruction1(branch, Materialize(rv, op1)));

      TargetInstruction* target = target_node->data.ptr;
      if (target == NULL) {
        // Forward branch, add fixup for target label.
        VectorAppend(&rv->base.fixups, NewBranchFixup(inst, target_node, 1));
      } else {
        inst->operand[1] = target;
      }

      return inst;
    }

    if (branch_info->reverse) {
      // Swap op1 and op2.
      IRNode* tmp = op1;
      op1 = op2;
      op2 = tmp;
    }

    TargetInstruction* lhs = Materialize(rv, op1);
    TargetInstruction* rhs = Materialize(rv, op2);
    if ((expr->opcode == IR_OP(cmpeqi) || expr->opcode == IR_OP(cmpnei)) &&
        op1->type != NULL && op1->type->size == 4 && IRIsConst(op2)) {
      int64_t c = IRIntConstValue(op2);
      if (c >= 0x80000000LL && c <= 0xffffffffLL) {
        TargetInstruction* mask = Emit(rv, NewInstruction(X86_64_OP(tmp)));
        TargetInstruction* mv =
            Emit(rv, NewInstruction1(
                         X86_64_OP(mv),
                         GetIntConstant(rv, NULL, kTargetType64Bit,
                                        0xffffffffLL)));
        mv->dest = mask;
        lhs = Emit(rv, NewInstruction2(X86_64_OP(and), lhs, mask));
      }
    }

    TargetInstruction* inst =
        Emit(rv, NewInstruction2(branch_opcode, lhs, rhs));

    TargetInstruction* target = target_node->data.ptr;
    if (target == NULL) {
      // Forward branch, add fixup for target label.
      VectorAppend(&rv->base.fixups, NewBranchFixup(inst, target_node, 2));
    } else {
      inst->operand[2] = target;
    }

    return inst;
  }

  X86_64Opcode opcode;
  switch (node->opcode) {
    case IR_OP(btrue):
      opcode = X86_64_OP(jnz);
      break;
    case IR_OP(bfalse):
      opcode = X86_64_OP(jz);
      break;
    default:
      assert(false);
  }

  int target_operand_num = 1;
  TargetInstruction* inst;

  if (IRIsConst(expr)) {
    int64_t value = ((IRConstant*)expr)->value.ivalue;
    if (value == 0) {
      // Expression is zero.  This becomes unconditional.
      if (opcode == X86_64_OP(jnz)) {
        // bnez with zero operand means branch will never be taken.
        return Emit(rv, NewInstruction(X86_64_OP(nop)));
      }
      // beqz with zero is unconditional.
      inst = Emit(rv, NewInstruction(X86_64_OP(jmp)));
      target_operand_num = 0;
    } else {
      if (opcode == X86_64_OP(jz)) {
         // beq with zero operand means branch will never be taken.
         return Emit(rv, NewInstruction(X86_64_OP(nop)));
       }
       // bnez with zero is unconditional.
       inst = Emit(rv, NewInstruction(X86_64_OP(jmp)));
       target_operand_num = 0;
    }
  } else {
    inst =
      Emit(rv, NewInstruction1(opcode, Materialize(rv, expr)));
  }
  
  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    // Forward branch, add fixup for target label.
    VectorAppend(&rv->base.fixups,
                 NewBranchFixup(inst, target_node, target_operand_num));
  } else {
    inst->operand[target_operand_num] = target;
  }
  return inst;
}

static TargetInstruction* LowerBranch(X86_64Generator* rv, IRNode* node) {
  if (node->inputs.length == 0) {
    return NULL;
  }
  IRNode* target_node = node->inputs.value.p[0];

  // Return branches must go through the shared epilogue.  This lowering happens
  // before register allocation, and register allocation can later introduce a
  // real frame (saved registers, spills, forced frame pointer for stack args).
  // Emitting a bare ret here would then skip the restore path.
  
  // Normal branch or non-leaf return branch.
  TargetInstruction* inst =
      (TargetInstruction*)Emit(rv, NewInstruction(X86_64_OP(jmp)));

  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    // Forward branch, add fixup for target label.
    VectorAppend(&rv->base.fixups, NewBranchFixup(inst, target_node, 0));
  } else {
    inst->operand[0] = target;
  }
  return inst;
}

static TargetInstruction* LowerLabel(X86_64Generator* rv, IRNode* label) {
  TargetInstruction* inst =  Emit(rv, NewInstruction(X86_64_OP(label)));
  label->data.ptr = inst;
  ApplyFixups(rv, label);
  return inst;
}

static TargetInstruction* LowerNamedLabel(X86_64Generator* rv, IRNode* label) {
  IRNamedLabel* n = (IRNamedLabel*)label;
  TargetInstruction* inst =  Emit(rv, TargetNewNamedLabel(n->name));
  label->data.ptr = inst;
  return inst;
}

static TargetInstruction* LowerResult(X86_64Generator* rv, IRNode* node) {
  assert(node->inputs.length == 1);
  X86_64Opcode result_reg_opcode, opcode;
  switch (node->opcode) {
    case IR_OP(resulti):
    case IR_OP(resulta):
      result_reg_opcode = X86_64_OP(resulti);
      opcode = X86_64_OP(mv);
      break;
    case IR_OP(resultf):
      result_reg_opcode = X86_64_OP(resultf);
      opcode = X86_64_OP(fmv_s);
      break;
    case IR_OP(resultd):
      result_reg_opcode = X86_64_OP(resultd);
      opcode = X86_64_OP(fmv_d);
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }
  TargetInstruction* result = Materialize(rv, node->inputs.value.p[0]);
  TargetInstruction* result_reg = EmitSymbol(rv, NewInstruction(result_reg_opcode));
  return SetLoweredNode(node, SetDestOrMove(rv, result, result_reg, opcode));
}

static TargetInstruction* LowerAsm(X86_64Generator* rv, IRNode* node) {
  // The first argument is a literal containing the assembly language.
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(rv, TargetNewLiteral((int)id_node->value.ivalue));

  AsmASTNode* asm_node = node->aux;
  if (asm_node == NULL ||
      (asm_node->outputs.length == 0 && asm_node->inputs.length == 0 &&
       asm_node->clobbers.length == 0 && asm_node->labels.length == 0)) {
    TargetInstruction* result = Emit(rv, NewInstruction1(X86_64_OP(asm), literal));

    SetLoweredNode(node, result);
    return result;
  }

  assert(asm_node->outputs.length + asm_node->inputs.length <= X86_64_MAX_ASM_OPERANDS);
  X86_64AsmInstruction* asm_inst = malloc(sizeof(X86_64AsmInstruction));
  TargetInitInstruction(&asm_inst->base, (TargetOpcode)X86_64_OP(asm));
  asm_inst->base.flags |= X86_64_INST_EXTENDED_ASM;
  asm_inst->base.operand[0] = literal;
  asm_inst->asm_node = asm_node;
  asm_inst->num_operands = (int)(asm_node->outputs.length + asm_node->inputs.length);
  memset(asm_inst->immediate_values, 0, sizeof(asm_inst->immediate_values));

  size_t ir_index = 1;
  int operand_index = 0;
  TargetInstruction* output_regs[X86_64_MAX_ASM_OPERANDS] = {0};
  for (size_t i = 0; i < asm_node->outputs.length; i++, operand_index++) {
    AsmOperand* operand = asm_node->outputs.value.p[i];
    IRNode* addr_node = node->inputs.value.p[ir_index++];
    int arg_reg = 5 - (int)i;
    if (arg_reg < 0) {
      arg_reg = 5;
    }
    TargetInstruction* reg = IntArgumentRegister(rv, arg_reg);
    asm_inst->reg_nums[operand_index] = X86_64_INT_ARG_START + arg_reg;
    asm_inst->is_fp[operand_index] = false;
    asm_inst->sizes[operand_index] = operand->expr->type->size;
    output_regs[i] = reg;
    if (operand->is_readwrite) {
      TargetInstruction* loaded = Load(rv, addr_node, operand->expr->type->size <= 4
                                                    ? X86_64_OP(loadl)
                                                    : X86_64_OP(loadq));
      SetDestOrMove(rv, loaded, reg, X86_64_OP(mv));
    }
  }

  for (size_t i = 0; i < asm_node->inputs.length; i++, operand_index++) {
    AsmOperand* operand = asm_node->inputs.value.p[i];
    IRNode* input_node = node->inputs.value.p[ir_index++];
    asm_inst->is_fp[operand_index] = false;
    asm_inst->sizes[operand_index] = operand->expr->type->size;
    if (strchr(operand->constraint.value, 'i') != NULL && IRIsIntConst(input_node)) {
      asm_inst->reg_nums[operand_index] = INT_MIN;
      asm_inst->immediate_values[operand_index] = IRIntConstValue(input_node);
      continue;
    }
    int arg_reg = (int)i;
    TargetInstruction* reg = IntArgumentRegister(rv, arg_reg);
    asm_inst->reg_nums[operand_index] = X86_64_INT_ARG_START + arg_reg;
    SetDestOrMove(rv, Materialize(rv, input_node), reg, X86_64_OP(mv));
  }

  TargetInstruction* result = Emit(rv, &asm_inst->base);
  for (size_t i = 0; i < asm_node->outputs.length; i++) {
    AsmOperand* operand = asm_node->outputs.value.p[i];
    IRNode* addr_node = node->inputs.value.p[1 + i];
    Store(rv, addr_node, output_regs[i],
          operand->expr->type->size <= 4 ? X86_64_OP(storel) : X86_64_OP(storeq));
  }

  SetLoweredNode(node, result);
  return result;
}

// A literal reference is an add of the literal offset (the first input
// to the literalref node) to the 'literal' with the given id.  This will
// be assembled as a reference to a symbol with the name .str.%d.
static TargetInstruction* LowerLiteralReference(X86_64Generator* rv, Generator* gen, IRNode* node) {
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(rv, TargetNewLiteral((int)id_node->value.ivalue));

  TargetInstruction* result =
      Emit(rv, NewInstruction1(X86_64_OP(lea_rip), literal));

  // Route the result into a destination tmp when this literalref is a ?: / && /
  // || branch (the "-> $n" annotation); otherwise the merge tmp is never
  // written and the value is read uninitialized.
  TargetInstruction* dest = GetDestInstruction(rv, gen, node);
  if (dest != NULL) {
    result = SetDestOrMove(rv, result, dest, X86_64_OP(mv));
  }

  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerAddressOf(X86_64Generator* rv, IRNode* node) {
  IRNode* input = node->inputs.value.p[0];
  TargetInstruction* addr;
  TargetInstruction* offset;
  bool on_stack = GetRegAndOffset(rv, input, &addr, &offset);
  if (!on_stack || offset == NULL || TargetIsZero(offset)) {
    return SetLoweredNode(node, addr);
  }
  return SetLoweredNode(node, AddValue(rv, addr, offset));
}

static TargetInstruction* LowerZeroExtend(X86_64Generator* rv, Generator* gen,
                                          IRNode* node) {
  IRNode* src = node->inputs.value.p[0];
  TargetInstruction* value = Materialize(rv, src);
  // The second IR operand is the bit-difference (diff*8), not a usable mask, so
  // derive the mask from the operand types: a zero extension keeps the low
  // min(src,dest) bytes of value and clears the rest.  Using the constant
  // operand directly (e.g. 24 for a char->int extension) produces a wrong mask.
  int src_size = src->type != NULL ? src->type->size : node->type->size;
  int keep_bytes = src_size < node->type->size ? src_size : node->type->size;
  if (keep_bytes >= 8) {
    // No masking required; the value already occupies the full register.
    TargetInstruction* dest = GetDestInstruction(rv, gen, node);
    if (dest != NULL) {
      value = SetDestOrMove(rv, value, dest, X86_64_OP(mv));
    }
    return SetLoweredNode(node, value);
  }
  int64_t mask = (1LL << (keep_bytes * 8)) - 1;
  if (X86_64IsPossibleImmediate(mask)) {
    value = Emit(rv, NewInstruction2(
                         X86_64_OP(and), value,
                         GetIntConstant(rv, NULL, kTargetType64Bit, mask)));
  } else {
    TargetInstruction* mask_reg = Emit(
        rv, NewInstruction1(X86_64_OP(mov),
                            GetIntConstant(rv, NULL, kTargetType64Bit, mask)));
    value = Emit(rv, NewInstruction2(X86_64_OP(and), value, mask_reg));
  }
  TargetInstruction* dest = GetDestInstruction(rv, gen, node);
  if (dest != NULL) {
    value = SetDestOrMove(rv, value, dest, X86_64_OP(mv));
  }
  return SetLoweredNode(node, value);
}

// For a post-increment/decrement whose result value is consumed (such as
// `f(p++)`), the IR provides the previously-loaded old value as a third input.
// When the modified object is a register-allocated variable, that load aliases
// the live variable register; the in-place update below would clobber it before
// the consumer reads it.  Snapshot the old value into a fresh register and
// retarget the consumer's value node to the snapshot.  (Memory-resident
// variables already load into an independent temporary, so no snapshot is
// needed there.)
static void SnapshotPostIncOldValue(X86_64Generator* rv, IRNode* node,
                                    IRNode* addr_node) {
  if (node->inputs.length < 3) {
    return;
  }
  IRNode* value_node = node->inputs.value.p[2];
  if (value_node == NULL) {
    return;
  }
  TargetInstruction* old = GetLoweredNode(value_node);
  if (old == NULL || !X86_64IsVarRegister(old)) {
    return;
  }
  X86_64Opcode mv_opcode =
      TypeIsFloatingPoint(addr_node->type)
          ? (X86_64FpIsDoubleWidth(addr_node->type) ? X86_64_OP(fmv_d)
                                           : X86_64_OP(fmv_s))
          : X86_64_OP(mv);
  TargetInstruction* snapshot = Emit(rv, NewInstruction1(mv_opcode, old));
  // Redirect the value node's lowered result to the snapshot.  The node was
  // already lowered to the live variable register (so SetLoweredNode would be
  // a no-op); overwrite the cached lowering directly so the consumer reads the
  // snapshot instead.  Any user of `old` that is the inc/dec itself has already
  // been handled above.
  value_node->data.ptr = snapshot;
}

static TargetInstruction* LowerInc(X86_64Generator* rv, IRNode* node) {
  IRNode* addr_node = node->inputs.value.p[0];
  X86_64Opcode ld_opcode;
  X86_64Opcode st_opcode;
  switch (node->opcode) {
    case IR_OP(inc8):
      ld_opcode = X86_64_OP(loadb);
      st_opcode = X86_64_OP(storeb);
      break;
    case IR_OP(uinc8):
      ld_opcode = X86_64_OP(loadb_z);
      st_opcode = X86_64_OP(storeb);
      break;
    case IR_OP(inc16):
      ld_opcode = X86_64_OP(loadw);
      st_opcode = X86_64_OP(storew);
      break;
    case IR_OP(uinc16):
      ld_opcode = X86_64_OP(loadw_z);
      st_opcode = X86_64_OP(storew);
      break;
   case IR_OP(inc32):
      ld_opcode = X86_64_OP(loadl);
      st_opcode = X86_64_OP(storel);
      break;
    case IR_OP(uinc32):
       ld_opcode = X86_64_OP(loadl);
       st_opcode = X86_64_OP(storel);
       break;
    case IR_OP(inc64):
    case IR_OP(uinc64):
    case IR_OP(inca):
      ld_opcode = X86_64_OP(loadq);
      st_opcode = X86_64_OP(storeq);
     break;
    case IR_OP(incf):
      ld_opcode = X86_64_OP(loadss);
      st_opcode = X86_64_OP(storess);
     break;
    case IR_OP(incd):
      ld_opcode = X86_64_OP(loadsd);
      st_opcode = X86_64_OP(storesd);
     break;
    default:
      abort();
  }
  TargetInstruction* load = Load(rv, addr_node, ld_opcode);
  SnapshotPostIncOldValue(rv, node, addr_node);
  TargetInstruction* inc;
  IRNode* amount_node = node->inputs.value.p[1];
  TargetInstruction* amount = GetLoweredNode(amount_node);
  if (TypeIsFloatingPoint(node->type)) {
    inc =  Emit(rv, NewInstruction2(X86_64FpIsDoubleWidth(node->type) ? X86_64_OP(addsd) : X86_64_OP(addss), load, amount));
  } else  {
    inc =  AddImmediate(rv, load, X86_64IntValue(amount));
  }
  Store(rv, addr_node, inc, st_opcode);
  return SetLoweredNode(node, inc);
}


static TargetInstruction* LowerDec(X86_64Generator* rv, IRNode* node) {
  IRNode* addr_node = node->inputs.value.p[0];
  X86_64Opcode ld_opcode;
  X86_64Opcode st_opcode;
  switch (node->opcode) {
    case IR_OP(dec8):
      ld_opcode = X86_64_OP(loadb);
      st_opcode = X86_64_OP(storeb);
      break;
    case IR_OP(udec8):
      ld_opcode = X86_64_OP(loadb_z);
      st_opcode = X86_64_OP(storeb);
      break;
    case IR_OP(dec16):
      ld_opcode = X86_64_OP(loadw);
      st_opcode = X86_64_OP(storew);
      break;
    case IR_OP(udec16):
      ld_opcode = X86_64_OP(loadw_z);
      st_opcode = X86_64_OP(storew);
      break;
   case IR_OP(dec32):
      ld_opcode = X86_64_OP(loadl);
      st_opcode = X86_64_OP(storel);
      break;
    case IR_OP(udec32):
       ld_opcode = X86_64_OP(loadl);
       st_opcode = X86_64_OP(storel);
       break;
    case IR_OP(dec64):
    case IR_OP(udec64):
    case IR_OP(deca):
      ld_opcode = X86_64_OP(loadq);
      st_opcode = X86_64_OP(storeq);
     break;
    case IR_OP(decf):
      ld_opcode = X86_64_OP(loadss);
      st_opcode = X86_64_OP(storess);
     break;
    case IR_OP(decd):
      ld_opcode = X86_64_OP(loadsd);
      st_opcode = X86_64_OP(storesd);
     break;
    default:
      abort();
  }
  TargetInstruction* load = Load(rv, addr_node, ld_opcode);
  SnapshotPostIncOldValue(rv, node, addr_node);
  TargetInstruction* inc;
  IRNode* amount_node = node->inputs.value.p[1];
  TargetInstruction* amount = GetLoweredNode(amount_node);
  if (TypeIsFloatingPoint(node->type)) {
    inc =  Emit(rv, NewInstruction2(X86_64FpIsDoubleWidth(node->type) ? X86_64_OP(subsd) : X86_64_OP(subss), load, amount));
  } else  {
    inc =  AddImmediate(rv, load, -X86_64IntValue(amount));
  }
  Store(rv, addr_node, inc, st_opcode);
  return SetLoweredNode(node, inc);}

static TargetInstruction* LowerGetBitField(X86_64Generator* rv, IRNode* node) {
  TargetInstruction* value = Materialize(rv, node->inputs.value.p[0]);
  int bit_pos = (int)IRIntConstValue(node->inputs.value.p[1]);
  int bit_size = (int)IRIntConstValue(node->inputs.value.p[2]);
  if (TypeIsUnsigned(node->type)) {
    // Shift right by bit_pos
    // Mask with bit_size
    TargetInstruction* lsr = Emit(rv, NewInstruction2(X86_64_OP(sar), value, GetIntConstant(rv, NULL, kTargetType32Bit, bit_pos)));
    uint64_t mask = bit_size == 64 ? -1LL : (1 << bit_size) - 1;
    TargetInstruction* m = Emit(rv, NewInstruction2(X86_64_OP(and), lsr, GetIntConstant(rv, NULL, kTargetType32Bit, mask)));
    SetLoweredNode(node, m);
    return m;
  }
  // Shift left by 64 - (bit_pos + bit_size).  Top bit in bit 63.
  // Shift right by 64 - bit_size.
  TargetInstruction* lsl = Emit(rv, NewInstruction2(X86_64_OP(shl), value, GetIntConstant(rv, NULL, kTargetType32Bit, 64 - (bit_pos + bit_size))));
  TargetInstruction* asr = Emit(rv, NewInstruction2(X86_64_OP(sar), lsl, GetIntConstant(rv, NULL, kTargetType32Bit, 64 - bit_size)));

  SetLoweredNode(node, asr);
  return asr;
}

// Shift input left by bit_pos
// Mask input to bit_size bits (in correct position)
// Mask output by ~mask
// Or input into output.
static TargetInstruction* LowerSetBitField(X86_64Generator* rv, IRNode* node) {
  IRNode* output_node = node->inputs.value.p[0];
  IRNode* input_node = node->inputs.value.p[1];
  int bit_pos = (int)IRIntConstValue(node->inputs.value.p[2]);
  int bit_size = (int)IRIntConstValue(node->inputs.value.p[3]);
  uint64_t mask = bit_size == 64 ? -1LL : (1 << bit_size) - 1;
  mask <<= bit_pos;
  
  TargetInstruction* input = Materialize(rv, input_node);
  TargetInstruction* output = Materialize(rv, output_node);
  TargetInstruction* lsl = Emit(rv, NewInstruction2(X86_64_OP(shl), input, GetIntConstant(rv, NULL, kTargetType32Bit, bit_pos)));
  TargetInstruction* m1 = Emit(rv, NewInstruction2(X86_64_OP(and), lsl, GetIntConstant(rv, NULL, kTargetType32Bit, mask)));

  TargetInstruction* m2 = Emit(rv, NewInstruction2(X86_64_OP(and), output, GetIntConstant(rv, NULL, kTargetType32Bit, ~mask)));
  TargetInstruction* result = Emit(rv, NewInstruction2(X86_64_OP(or), m1, m2));
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerSignExtend(X86_64Generator* rv, Generator* gen,
                                          IRNode* node) {
  TargetInstruction* value = Materialize(rv, node->inputs.value.p[0]);
  bool route_stashed_result = (node->flags & kIRStashedCallResult) != 0;
  TargetInstruction* dest =
      route_stashed_result ? GetDestInstruction(rv, gen, node) : NULL;
  if (X86_64IsSignedLoad(value)) {
    if (dest != NULL) {
      value = SetDestOrMove(rv, value, dest, X86_64_OP(mv));
    }
    return SetLoweredNode(node, value);
  }
  IRConstant* diff_value = node->inputs.value.p[1];
  int64_t diff = diff_value->value.ivalue;
  if (diff <= 0) {
    // Narrowing is represented by the low destination-width view.  If the
    // narrowed signed value is widened again, that later conversion performs
    // the required extension.
    if (dest != NULL) {
      value = SetDestOrMove(rv, value, dest, X86_64_OP(mv));
    }
    return SetLoweredNode(node, value);
  }
  // Integer target instructions operate on 64-bit registers, so shift the
  // source sign bit to bit 63 rather than using merely the source/destination
  // width difference.  This also materializes scalar call results out of RAX
  // before a later call can overwrite them.
  IRNode* input = node->inputs.value.p[0];
  int source_bits =
      input->type != NULL ? (int)input->type->size * 8 : 32;
  int64_t shift = 64 - source_bits;
  TargetInstruction* immed =
      GetIntConstant(rv, NULL, kTargetType32Bit, shift);
  TargetInstruction* lsl = Emit(rv, NewInstruction2(X86_64_OP(shl), value, immed));
  TargetInstruction* asr = Emit(rv, NewInstruction2(X86_64_OP(sar), lsl, immed));
  if (dest != NULL) {
    asr = SetDestOrMove(rv, asr, dest, X86_64_OP(mv));
  }

  SetLoweredNode(node, asr);
  return asr;
}

static TargetInstruction* LowerAlign(X86_64Generator* rv, IRNode* node) {
  TargetInstruction* value = Materialize(rv, node->inputs.value.p[0]);
  IRConstant* align = node->inputs.value.p[1];

  TargetInstruction* immed = GetIntConstant(rv, NULL, kTargetType32Bit, align->value.ivalue - 1);
  TargetInstruction* inv_immed = GetIntConstant(rv, NULL, kTargetType32Bit, ~(align->value.ivalue - 1));
  TargetInstruction* add = Emit(rv, NewInstruction2(X86_64_OP(add), value, immed));
  TargetInstruction* and = Emit(rv, NewInstruction2(X86_64_OP(and), add, inv_immed));

  SetLoweredNode(node, and);
  return and;
}

static TargetInstruction* PushArg(X86_64Generator* rv, IRNode* node,
                                  TargetInstruction* inst, size_t offset) {
  if (node->type == NULL) {
    // No type, use sd instruction.
    return Emit(rv, NewInstruction3(
                        X86_64_OP(storeq), inst, StackPointer(rv),
                        GetIntConstant(rv, node, kTargetType64Bit, offset)));
  }
  X86_64Opcode opcode = X86_64_OP(storeq);
  if (TypeIsFloatingPoint(node->type)) {
    opcode = X86_64_OP(storesd);
  }
  return Emit(rv, NewInstruction3(
                      opcode, inst, StackPointer(rv),
                      GetIntConstant(rv, node, kTargetType64Bit, offset)));
}

static TargetInstruction* PopArg(X86_64Generator* rv, IRNode* node, size_t offset) {
  // Reading an incoming stack argument requires a frame pointer (these are
  // addressed rbp-relative), so make sure the prologue is not elided.
  rv->has_incoming_stack_args = true;
  // Incoming stack arguments live in the caller's frame just above our return
  // address.  rbp points at the return-address slot, so the first stack
  // argument is at rbp+8.  Address them frame-pointer relative (not via the
  // stack pointer, which has already been decremented by the frame size by the
  // time these loads execute in the prologue).  For varargs procedures the
  // prologue lowers rbp by space_above_frame_pointer, so add that back.
  int space_above_frame_pointer =
      compiler->current_function->info.function.varargs
          ? X86_64_VARARG_SAVE_AREA_SIZE
          : 0;
  int64_t fp_offset = (int64_t)offset + 8 + space_above_frame_pointer;
  if (node->type == NULL) {
    // No type, use ld instruction.
    return Emit(rv, NewInstruction2(
                        X86_64_OP(loadq), FramePointer(rv),
                        GetIntConstant(rv, node, kTargetType64Bit, fp_offset)));
  }
  X86_64Opcode opcode = X86_64_OP(loadq);
  if (TypeIsFloatingPoint(node->type)) {
    opcode = X86_64_OP(loadsd);
  }
  return Emit(rv, NewInstruction2(
                      opcode, FramePointer(rv),
                      GetIntConstant(rv, node, kTargetType64Bit, fp_offset)));
}


static TargetInstruction* LowerMemcpy(X86_64Generator* rv, IRNode* node) {
  // The memcpy IR node's inputs are the same as those for the memcpy
  // function.  However, there are no load nodes for the desination
  // or source addresses.  If we can do the copy without using a call
  // to memcpy we will do that.

  assert(node->inputs.length == 3);
  assert(IRIsConst(node->inputs.value.p[2]));

  // Source address.
  IRNode* src_node = node->inputs.value.p[1];
  TargetInstruction* src_addr;
  TargetInstruction* src_offset;
  int src_offset_value = 0;
  GetRegAndOffset(rv, src_node, &src_addr, &src_offset);
  if (src_offset != NULL) {
    if (!X86_64IsIntConst(src_offset)) {
      src_addr = AddValue(rv, src_addr, src_offset);
    } else {
      src_offset_value = (int)((TargetConstant*)src_offset)->value.ivalue;
    }
  }
  // Destination address.
  IRNode* dest_node = node->inputs.value.p[0];
  TargetInstruction* dest_addr;
  TargetInstruction* dest_offset;
  int dest_offset_value = 0;
  GetRegAndOffset(rv, dest_node, &dest_addr, &dest_offset);

  if (dest_offset != NULL) {
    if (!X86_64IsIntConst(dest_offset)) {
      // We have an address and register for the address, add them together.
      dest_addr = AddValue(rv, dest_addr, dest_offset);
    } else {
      dest_offset_value = (int)((TargetConstant*)dest_offset)->value.ivalue;
    }
  }

  int length = (int)((IRConstant*)node->inputs.value.p[2])->value.ivalue;
  TargetInstruction* result = Memcpy(rv, dest_addr, src_addr, length,
                                     src_offset_value, dest_offset_value, true);

  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerMemzero(X86_64Generator* rv, IRNode* node) {
  // The memzero IR node has one input: the variable to zero.  We
  // emit this is as a call to memset using the size of the symbol unless
  // we can do it more efficiently.
  assert(node->inputs.length == 1);
  IRNode* addr_node = node->inputs.value.p[0];
  IRVariable* var = (IRVariable*)addr_node;

  // Dest ddress.
  IRNode* dest_node = node->inputs.value.p[0];
  TargetInstruction* dest_addr;
  TargetInstruction* dest_offset;
  int offset_value = 0;
  GetRegAndOffset(rv, dest_node, &dest_addr, &dest_offset);

  if (dest_offset != NULL) {
    if (!X86_64IsIntConst(dest_offset)) {
      // We have an address and register for the address, add them together.
      dest_addr = AddValue(rv, dest_addr, dest_offset);
    } else {
      offset_value = (int)((TargetConstant*)dest_offset)->value.ivalue;
    }
  }
  // Prefer the backing symbol's size, but fall back to the memzero node's type
  // when the destination is a symbol-less slot (e.g. an sret return location).
  int64_t zero_size = (IRIsVariable(addr_node) && var->symbol != NULL)
                          ? var->symbol->type->size
                          : (node->type != NULL ? node->type->size : 0);
  TargetInstruction* result =
      Memzero(rv, dest_addr, zero_size, offset_value);

  SetLoweredNode(node, result);
  return result;
}

typedef enum {
  kArgLocationRegister,
  kArgLocationRegisterPair,
  kArgLocationPushed,
  kArgLocationPushedPair,
  kArgLocationPassedByReferenceInRegister,
  kArgLocationPassedByReferenceOnStack,
} ArgLocationType;

typedef struct {
  ArgLocationType type;
  union {
    TargetInstruction* reg;
    size_t offset;
  } location;
  TargetInstruction* second_reg;
  size_t second_offset;
  size_t reference_offset;
  size_t staging_offset;
  bool staged;
} ArgLocation;

static ArgLocation* NewArgLocationRegister(TargetInstruction* reg) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationRegister;
  loc->location.reg = reg;
  loc->reference_offset = 0;
  loc->second_reg = NULL;
  loc->second_offset = 0;
  loc->staging_offset = 0;
  loc->staged = false;
  return loc;
}

static ArgLocation* NewArgLocationPushed(ArgLocationType type, size_t offset) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = type;
  loc->location.offset = offset;
  loc->reference_offset = 0;
  loc->second_reg = NULL;
  loc->second_offset = 0;
  loc->staging_offset = 0;
  loc->staged = false;
  return loc;
}

static ArgLocation* NewArgLocationReferenceInRegister(TargetInstruction* reg,
                                                      size_t reference_offset) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationPassedByReferenceInRegister;
  loc->location.reg = reg;
  loc->reference_offset = reference_offset;
  loc->second_reg = NULL;
  loc->second_offset = 0;
  loc->staging_offset = 0;
  loc->staged = false;
  return loc;
}

static ArgLocation* NewArgLocationReferenceOnStack(size_t offset,
                                                   size_t reference_offset) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationPassedByReferenceOnStack;
  loc->location.offset = offset;
  loc->reference_offset = reference_offset;
  loc->second_reg = NULL;
  loc->second_offset = 0;
  loc->staging_offset = 0;
  loc->staged = false;
  return loc;
}

static ArgLocation* NewArgLocationRegisterPair(TargetInstruction* first,
                                               TargetInstruction* second) {
  ArgLocation* loc = NewArgLocationRegister(first);
  loc->type = kArgLocationRegisterPair;
  loc->second_reg = second;
  return loc;
}

static ArgLocation* NewArgLocationPushedPair(size_t first_offset) {
  ArgLocation* loc =
      NewArgLocationPushed(kArgLocationPushedPair, first_offset);
  loc->second_offset = first_offset + 8;
  return loc;
}

// Build a list of X86_64_OP(regarg) instructions to hold the
// argument registers and allow their liveness to extend
// to the point of call.  Each instruction uses two operands:
// 0: the next regarg instruction.
// 1: the rmov instruction that assigns to the
//    argument register.
// This forms a linked list whose head is the second operand to the
// call instruction.
//
// The purpose of this list is to allow the register allocator to know
// that the registers used as arguments (a0..a7, fa0..fa7) are allocated
// until the call instruction executes, than can be freed.
static TargetInstruction* BuildArgList(X86_64Generator* rv, Vector* arg_locations) {
  TargetInstruction* result = NULL;
  // Find next -based argument and add it to the regargs instruction list.
  for (size_t i = 0; i < arg_locations->length; i++) {
    ArgLocation* loc = arg_locations->value.p[i];
    if (loc->type == kArgLocationRegister) {
      result =
          Emit(rv, NewInstruction2(X86_64_OP(regarg), result, loc->location.reg));
    } else if (loc->type == kArgLocationRegisterPair) {
      result =
          Emit(rv, NewInstruction2(X86_64_OP(regarg), result, loc->location.reg));
      result =
          Emit(rv, NewInstruction2(X86_64_OP(regarg), result, loc->second_reg));
    }
  }
  return result;
}

// The x86-64 calling convention is very complex.  There are 8 integer and 8
// floating pointer registers that can be used to pass arguments.  Structs are
// particularly complex and how they are passed depends on their size and
// contents.
//
// This code does not follow the ABI defined in:
// https://github.com/riscv/riscv-elf-psabi-doc/blob/master/riscv-elf.md
// exactly.
// In particular it:
// 1. passes structs longer than XLEN (8) bytes by reference but first
//    copies them to the stack so that they are passed by value.
//    NOTE: I think it's incorrect for the ABI to pass long structs by
//    reference and allow them to be modified by the callee.  This means
//    that there is a major difference in behavior between small and large
//    structs and simply adding another field to a struct will make programs
//    stop working (TODO: check the C standard for this).
// 2. Doesn't do the 2XXLEN stuff where 16 byte structs are passed in a
//    register pair.
static TargetInstruction* LowerCall(X86_64Generator* rv, Generator* gen,
                                    IRNode* node) {
  assert(node->inputs.length >= 1);
  size_t struct_area_size = 0;
  int next_int_arg_reg = 0;
  int next_fp_arg_reg = 0;
  size_t next_pushed_arg_offset = 0;
  Vector arg_locations;
  VectorInit(&arg_locations);

  // Phase 1:
  // Work out the locations for all arguments.  The first 8 go in argument
  // registers, split into integer and floating point sets.
  for (size_t i = 1; i < node->inputs.length; i++) {
    IRNode* arg_node = node->inputs.value.p[i];
    if (TypeIsMemberPointerAggregate(arg_node->type)) {
      if (next_int_arg_reg + 1 < X86_64_NUM_INT_ARGS) {
        TargetInstruction* first =
            IntArgumentRegister(rv, next_int_arg_reg++);
        TargetInstruction* second =
            IntArgumentRegister(rv, next_int_arg_reg++);
        VectorAppend(&arg_locations,
                     NewArgLocationRegisterPair(first, second));
      } else {
        VectorAppend(&arg_locations,
                     NewArgLocationPushedPair(next_pushed_arg_offset));
        next_pushed_arg_offset += 16;
      }
    } else if (TypeIsStructOrUnion(arg_node->type)) {
      if (i == 1 && arg_node->opcode == IR_OP(structreturn)) {
        // RVO (Return Value Optimization), passing structreturn as arg.
        TargetInstruction* arg_reg =
             IntArgumentRegister(rv, next_int_arg_reg++);
         VectorAppend(&arg_locations, NewArgLocationRegister(arg_reg));
        continue;
      }
      // Struct or union that fit in a register are passed in a register.  If
      // they are bigger than 8 bytes they are passed by reference (first making
      // a copy on the stack).
      size_t struct_size = arg_node->type->size;
      if (struct_size <= 8) {
        if (next_int_arg_reg < X86_64_NUM_INT_ARGS) {
          // Argument goes in an argument register.
          TargetInstruction* arg_reg =
              IntArgumentRegister(rv, next_int_arg_reg++);
          ArgLocation* loc = NewArgLocationRegister(arg_reg);
          loc->reference_offset = struct_area_size;
          VectorAppend(&arg_locations, loc);
          struct_area_size += 8;
        } else {
          // Need to push argument on to the stack.  But we do that in reverse
          // order so for now, we record that the arg location is on the stack.
          VectorAppend(
              &arg_locations,
              NewArgLocationPushed(kArgLocationPushed, next_pushed_arg_offset));
          next_pushed_arg_offset += 8;
        }
      } else {
        // The struct needs to be copied onto the stack and then its address
        // passed either in a register or on the stack.
        if (next_int_arg_reg < X86_64_NUM_INT_ARGS) {
          // Argument goes in an argument register.
          TargetInstruction* arg_reg =
              IntArgumentRegister(rv, next_int_arg_reg++);
          VectorAppend(&arg_locations, NewArgLocationReferenceInRegister(
                                           arg_reg, struct_area_size));
        } else {
          VectorAppend(&arg_locations,
                       NewArgLocationReferenceOnStack(next_pushed_arg_offset,
                                                      struct_area_size));
          next_pushed_arg_offset += 8;
        }
        struct_area_size += struct_size;
      }
    } else if (TypeIsFloatingPoint(arg_node->type)) {
      if (next_fp_arg_reg < X86_64_NUM_FP_ARGS) {
        // Argument goes in an argument register.
        TargetInstruction* arg_reg =
            FloatingPointArgumentRegister(rv, next_fp_arg_reg++);
        VectorAppend(&arg_locations, NewArgLocationRegister(arg_reg));
      } else {
        // Need to push argument on to the stack.  But we do that in reverse
        // order so for now, we record that the arg location is on the stack.
        VectorAppend(
            &arg_locations,
            NewArgLocationPushed(kArgLocationPushed, next_pushed_arg_offset));
        next_pushed_arg_offset += 8;
      }
    } else {
      if (next_int_arg_reg < X86_64_NUM_INT_ARGS) {
        // Argument goes in an argument register.
        TargetInstruction* arg_reg =
            IntArgumentRegister(rv, next_int_arg_reg++);
        VectorAppend(&arg_locations, NewArgLocationRegister(arg_reg));
      } else {
        // Need to push argument on to the stack.  But we do that in reverse
        // order so for now, we record that the arg location is on the stack.
        VectorAppend(
            &arg_locations,
            NewArgLocationPushed(kArgLocationPushed, next_pushed_arg_offset));
        next_pushed_arg_offset += 8;
      }
    }
  }

  // Phase 2:
  // Decrement the stack pointer to make space for the stack args
  // Stage scalar register arguments in memory before assigning any ABI
  // argument register. Straight-line moves cannot preserve cycles where
  // argument values occupy each other's destination registers.
  size_t total_stack_size = struct_area_size + next_pushed_arg_offset;
  for (size_t i = 1; i < node->inputs.length; i++) {
    ArgLocation* loc = arg_locations.value.p[i - 1];
    IRNode* arg_node = node->inputs.value.p[i];
    if (loc->type == kArgLocationRegister &&
        !TypeIsStructOrUnion(arg_node->type)) {
      loc->staged = true;
      loc->staging_offset = total_stack_size;
      total_stack_size += 8;
    }
  }
  if (total_stack_size > 0) {
    TargetInstruction* newsp =
        AddImmediate(rv, StackPointer(rv), -total_stack_size);
    TargetSetDest(newsp, StackPointer(rv));
    //Emit(rv, NewInstruction2(X86_64_OP(rmov), StackPointer(rv), newsp));
  }

  // Phase 3:
  // Struct and unions that are bigger than 8 bytes are copied onto the
  // stack and passed by reference.  We need to copy all of these onto the
  // stack before we push all other arguments
  for (size_t i = 1; i < node->inputs.length; i++) {
    ArgLocation* arg_location = arg_locations.value.p[i - 1];
    IRNode* arg_node = node->inputs.value.p[i];
    size_t size = arg_node->type->size;
    switch (arg_location->type) {
      case kArgLocationPassedByReferenceInRegister:
      case kArgLocationPassedByReferenceOnStack: {
        // Copy struct onto stack.
        // NOTE: the ABI says that these are passed by reference, but that
        // means they are not copied and the callee can modify the original.
        // This seems wrong to me.
        TargetInstruction* arg = Materialize(rv, arg_node);
        Memcpy(rv, StackPointer(rv), arg, (int)size, 0,
               (int)(arg_location->reference_offset + next_pushed_arg_offset), false);
        break;
      }
      case kArgLocationRegister: {
        if (TypeIsStructOrUnion(arg_node->type) && size <= 8) {
          TargetInstruction* arg = Materialize(rv, arg_node);
          Memcpy(rv, StackPointer(rv), arg, (int)size, 0,
                 (int)(arg_location->reference_offset + next_pushed_arg_offset),
                 false);
        } else if (arg_location->staged) {
          PushArg(rv, arg_node, Materialize(rv, arg_node),
                  arg_location->staging_offset);
        }
        break;
      }
      default:
        break;
    }
  }

  // Phase 4:
  // Pass through all args, in reverse order, pushing those not passed in
  // registers and moving the register arguments into their argument
  // registers.
  //
  // TODO: figure out if we can just set the dest to the reg.
  for (size_t i = node->inputs.length - 1; i >= 1; i--) {
    IRNode* arg_node = node->inputs.value.p[i];
    ArgLocation* arg_location = arg_locations.value.p[i - 1];
    switch (arg_location->type) {
      case kArgLocationRegisterPair: {
        TargetInstruction* address = Materialize(rv, arg_node);
        TargetInstruction* first = Emit(rv, NewInstruction2(
            X86_64_OP(loadq), address,
            GetIntConstant(rv, NULL, kTargetType32Bit, 0)));
        TargetInstruction* second = Emit(rv, NewInstruction2(
            X86_64_OP(loadq), address,
            GetIntConstant(rv, NULL, kTargetType32Bit, 8)));
        TargetInstruction* first_move =
            Emit(rv, NewInstruction1(X86_64_OP(mv), first));
        first_move->dest = arg_location->location.reg;
        TargetInstruction* second_move =
            Emit(rv, NewInstruction1(X86_64_OP(mv), second));
        second_move->dest = arg_location->second_reg;
        break;
      }
      case kArgLocationPushedPair: {
        TargetInstruction* address = Materialize(rv, arg_node);
        TargetInstruction* first = Emit(rv, NewInstruction2(
            X86_64_OP(loadq), address,
            GetIntConstant(rv, NULL, kTargetType32Bit, 0)));
        TargetInstruction* second = Emit(rv, NewInstruction2(
            X86_64_OP(loadq), address,
            GetIntConstant(rv, NULL, kTargetType32Bit, 8)));
        PushArg(rv, arg_node, first, arg_location->location.offset);
        PushArg(rv, arg_node, second, arg_location->second_offset);
        break;
      }
      case kArgLocationPassedByReferenceInRegister: {
        // Struct passed by reference in a register.  The reference_offset
        // contains the offset from the to of the pushed args to the copied
        // struct.
        TargetInstruction* arg = AddImmediate(
            rv, StackPointer(rv),
            arg_location->reference_offset + next_pushed_arg_offset);
        SetDestOrMoveToArgReg(rv, arg_node, arg, arg_location->location.reg, X86_64_OP(mv));
        // Emit(rv, NewInstruction2(X86_64_OP(rmov), arg_location->location.reg, arg));
        break;
      }
      case kArgLocationPassedByReferenceOnStack: {
        // Struct passed by reference on the stack.
        TargetInstruction* arg = AddImmediate(
            rv, StackPointer(rv),
            arg_location->reference_offset + next_pushed_arg_offset);
        PushArg(rv, arg_node, arg, arg_location->location.offset);
        break;
      }
      case kArgLocationPushed: {
        TargetInstruction* arg = Materialize(rv, arg_node);
        if (TypeIsStructOrUnion(arg_node->type)) {
          size_t size = arg_node->type->size;
          if (size <= 8) {
            // A struct less than 8 bytes is passed directly on stack.  The
            // Materialize call will result in the address of the struct.  We
            // need to load it.
            TargetInstruction* addr =
                IRIsStaticVariable(arg_node) ? GetLoweredNode(arg_node) : arg;
            arg = Emit(rv, NewInstruction2(
                               X86_64_OP(loadq), addr,
                               GetIntConstant(rv, NULL, kTargetType32Bit, 0)));
          }
        }
        PushArg(rv, arg_node, arg, arg_location->location.offset);
        break;
      }
      case kArgLocationRegister: {
        // Argument is in a register.
        TargetInstruction* arg;
        if (arg_location->staged) {
          X86_64Opcode load_opcode = X86_64_OP(loadq);
          if (TypeIsFloat(arg_node->type)) {
            load_opcode = X86_64_OP(loadss);
          } else if (TypeIsDouble(arg_node->type) ||
                     TypeIsLongDouble(arg_node->type)) {
            load_opcode = X86_64_OP(loadsd);
          }
          arg = Emit(rv, NewInstruction2(
                             load_opcode, StackPointer(rv),
                             GetIntConstant(rv, arg_node, kTargetType64Bit,
                                            arg_location->staging_offset)));
        } else {
          arg = Materialize(rv, arg_node);
        }
        if (arg_node->opcode == IR_OP(pusharg) && arg_node->inputs.length > 0) {
          IRNode* pushed = arg_node->inputs.value.p[0];
          if (pushed != NULL && pushed->opcode == IR_OP(structreturn)) {
            TargetInstruction* struct_return = GetLoweredNode(pushed);
            if (struct_return == NULL) {
              struct_return = Materialize(rv, pushed);
            }
            TargetInstruction* move =
                Emit(rv, NewInstruction1(X86_64_OP(mv), struct_return));
            move->dest = arg_location->location.reg;
            break;
          }
        }
        if (TypeIsStructOrUnion(arg_node->type)) {
          size_t size = arg_node->type->size;
          if (size <= 8) {
            // A struct less than 8 bytes is passed in a register.  The
            // original value has already been copied into the outgoing stack
            // scratch area so address temporaries used for other arguments
            // cannot clobber it before this load.
            TargetInstruction* addr =
                AddImmediate(rv, StackPointer(rv),
                             arg_location->reference_offset +
                                 next_pushed_arg_offset);
            arg = Emit(rv, NewInstruction2(
                               X86_64_OP(loadq), addr,
                               GetIntConstant(rv, NULL, kTargetType32Bit, 0)));
          }
        }
        X86_64Opcode mov_opcode = X86_64_OP(mv);
        if (TypeIsFloatingPoint(arg_node->type)) {
          if (X86_64FpIsDoubleWidth(arg_node->type)) {
            mov_opcode = X86_64_OP(fmv_d);
          } else {
            mov_opcode = X86_64_OP(fmv_s);
          }
        }
        // Emit(rv, NewInstruction2(mov_opcode, arg_location->location.reg, arg));
        SetDestOrMoveToArgReg(rv, arg_node, arg, arg_location->location.reg, mov_opcode);
        break;
      }
    }
  }

  // Finally emit the call instruction containing the address
  // to call, as its first operand and a linked list of regarg
  // pseudo-instructions as its second operand.  This list makes
  // the lifetime of the registers allocated for argument passing
  // extend to the call site, thus enabling the register allocator
  // to keep them from being used before the call.
  TargetInstruction* addr = GetLoweredNode(node->inputs.value.p[0]);
  X86_64Opcode opcode;
  TargetInstruction* call;
  
  // If the node has been identified as a tail call by the IR
  // optimizer we might be able to convert it to a jump.
  //
  // Possible optimization: if the arguments don't contain an address in
  // the current stack frame we can allow tail calls when we have space
  // allocated on the stack.  I don't know how to detect that though.
  bool can_be_tail_call = (node->flags & kIRTailCall) != 0 &&
      total_stack_size == 0 && rv->base.stack_frame_size == 0;

  if (can_be_tail_call) {
    // Tail call.
    // Add a restore instruction and replace the call with a jump.
    if (X86_64IsExpression(addr)) {
      // The address is calculated inside the function body.  It needs
      // to survive a restore operation so we need to put it in
      // a temp register.  All 't' regs should not be allocated now
      // since we are leaving the function.
      BuildArgList(rv, &arg_locations);
      TargetInstruction* mv = Emit(rv, NewInstruction1(X86_64_OP(mv), addr));
      mv->dest = Tmp(rv);
      Emit(rv, NewInstruction(X86_64_OP(restore)));
      call = Emit(rv, NewInstruction1(X86_64_OP(jmp), Tmp(rv)));
    } else {
      Emit(rv, NewInstruction(X86_64_OP(restore)));
      BuildArgList(rv, &arg_locations);
      call = Emit(rv, NewInstruction1(X86_64_OP(jmp), addr));
    }
    rv->base.num_calls--;
  } else {
    if (((int)addr->opcode == (int)X86_64_OP(symbol))) {
      // Calling a symbol, use a regular 'call' instruction.
      opcode = TypeIsFloatingPoint(node->type) ? X86_64_OP(callf) : X86_64_OP(call);
    } else {
      // Calling through a register, rcall.
      opcode = TypeIsFloatingPoint(node->type) ? X86_64_OP(rcallf) : X86_64_OP(rcall);
    }
    call =
        Emit(rv, NewInstruction2(opcode, addr, BuildArgList(rv, &arg_locations)));

    // Increment the stack pointer again to remove pushed args.
    if (total_stack_size > 0) {
      TargetInstruction* newsp =
          AddImmediate(rv, StackPointer(rv), total_stack_size);
      TargetSetDest(newsp, StackPointer(rv));
      // Emit(rv, NewInstruction2(X86_64_OP(rmov), StackPointer(rv), newsp));
    }
  }
  SetLoweredNode(node, call);

  VectorDestructWithContents(&arg_locations, NULL, /*free_element=*/true);

  // Honor an explicit result destination (node->dest, written "-> $N" in the
  // IR).  The call leaves its result in the return register (rax / xmm0); the
  // emitter does not move it anywhere on its own.  When the IR requests the
  // result be placed in a particular value (e.g. the shared temporary produced
  // by a short-circuit && / || or a ?: expression, where two different calls
  // must converge on the same destination), emit an explicit move.  Without
  // this the destination register keeps its previous (garbage) contents.
  if (node->dest != NULL) {
    TargetInstruction* dest = GetDestInstruction(rv, gen, node);
    if (dest != NULL && dest != call) {
      X86_64Opcode mov_opcode = X86_64_OP(mv);
      if (TypeIsFloatingPoint(node->type)) {
        mov_opcode =
            X86_64FpIsDoubleWidth(node->type) ? X86_64_OP(fmv_d) : X86_64_OP(fmv_s);
      }
      TargetInstruction* move = Emit(rv, NewInstruction1(mov_opcode, call));
      move->dest = dest;
      return SetLoweredNode(node, move);
    }
  }
  return call;
}

// A computed branch is used to branch to a dense switch table consisting
// of a sequence of 'j' instructions to the case labels.  Each instruction
// is 4 bytes long.  The instruction sequence for the computed branch is:
//
// entry: t0 = index into table.
// slli t1, t0, 2      - byte offset into table
// auipc t2, 0        - high 20 bits of pc at this instruction
// addi t2, t2, t1     - address of jump instruction
// jalr x0, t2, 12    - jump to jump instruction + 12
//
// The offset in the jalr instruction is because the auipc instruction
// is 12 bytes before the branch table start.  The value of t2 is the
// pc of the auipc instruction plus the offset into the table.

static TargetInstruction* LowerComputedBranch(X86_64Generator* rv, IRNode* node) {
  assert(node->inputs.length == 1);
  TargetInstruction* value = GetLoweredNode(node->inputs.value.p[0]);
  TargetInstruction* slli =
      Emit(rv, NewInstruction2(X86_64_OP(shl), value,
                               GetIntConstant(rv, NULL, kTargetType32Bit, 2)));
  TargetInstruction* auipc =
      Emit(rv, NewInstruction1(X86_64_OP(lea_rip),
                               GetIntConstant(rv, NULL, kTargetType32Bit, 0)));
  TargetInstruction* add = Emit(rv, NewInstruction2(X86_64_OP(add), auipc, slli));
  TargetInstruction* jalr =
      Emit(rv, NewInstruction3(X86_64_OP(rcall), Zero(rv), add,
                               GetIntConstant(rv, NULL, kTargetType32Bit, 12)));
  jalr->flags |= TARGET_INST_TABLE_JUMP;
  SetLoweredNode(node, jalr);
  return jalr;
}

// SysV AMD64 va_list helpers.  The structure layout is:
//   0: gp_offset (4 bytes)
//   4: fp_offset (4 bytes)
//   8: overflow_arg_area (pointer)
//  16: reg_save_area (pointer)
static void StoreApField(X86_64Generator* rv, IRNode* ap_node, int field_offset,
                         TargetInstruction* value, X86_64Opcode store) {
  if (X86_64IsConst(value)) {
    value = Emit(rv, NewInstruction1(X86_64_OP(mov), value));
  }
  TargetInstruction* ap_addr;
  TargetInstruction* ap_offset;
  GetRegAndOffset(rv, ap_node, &ap_addr, &ap_offset);
  TargetInstruction* off =
      AddImmediate(rv, ap_offset, field_offset);
  Emit(rv, NewInstruction3(store, value, ap_addr, off));
}

static TargetInstruction* LoadApField(X86_64Generator* rv, IRNode* ap_node,
                                    int field_offset, X86_64Opcode load) {
  TargetInstruction* ap_addr;
  TargetInstruction* ap_offset;
  GetRegAndOffset(rv, ap_node, &ap_addr, &ap_offset);
  TargetInstruction* off =
      AddImmediate(rv, ap_offset, field_offset);
  return Emit(rv, NewInstruction2(load, ap_addr, off));
}

// The first input is the address of the 'ap' variable.  The second is the
// address of the last named function argument.
static TargetInstruction* LowerBuiltinVaStart(X86_64Generator* rv, IRNode* node) {
  TargetInstruction* gp_offset = GetIntConstant(
      rv, NULL, kTargetType32Bit, (int64_t)rv->num_int_arg_regs * 8);
  StoreApField(rv, node->inputs.value.p[0], 0, gp_offset, X86_64_OP(storel));

  TargetInstruction* fp_offset = GetIntConstant(
      rv, NULL, kTargetType32Bit,
      X86_64_VARARG_FP_SAVE_OFFSET + (int64_t)rv->num_fp_arg_regs * 16);
  StoreApField(rv, node->inputs.value.p[0], 4, fp_offset, X86_64_OP(storel));

  // overflow_arg_area must point at the first variadic argument that was passed
  // on the stack.  The "&last_named_arg + 8" trick only works when the last
  // named argument itself lives on the stack; for the common case (e.g. printf,
  // whose single named argument "format" arrives in a register) it would point
  // into the register-save area instead.  Compute the address directly from the
  // frame pointer using the same incoming-stack-argument layout as PopArg:
  // rbp points at the return-address slot, the first stack slot is at rbp+8, a
  // varargs prologue lowers rbp by space_above_frame_pointer, and any named
  // arguments that spilled to the stack come before the variadic ones.
  int space_above_frame_pointer = X86_64_VARARG_SAVE_AREA_SIZE;
  int named_stack_bytes = 0;
  {
    bool is_struct_return = TypeIsStructOrUnion(
        compiler->current_function->info.function.symbol->type->next);
    int int_reg = X86_64_INT_ARG_START + (is_struct_return ? 1 : 0);
    int fp_reg = X86_64_FP_ARG_START;
    Vector* proto = &compiler->current_function->info.function.prototype;
    for (size_t i = 0; i < proto->length; i++) {
      Symbol* arg_symbol = proto->value.p[i];
      if (TypeIsFloatingPoint(arg_symbol->type)) {
        if (fp_reg <= X86_64_FP_ARG_END) {
          fp_reg++;
        } else {
          named_stack_bytes += 8;
        }
      } else {
        if (int_reg <= X86_64_INT_ARG_END) {
          int_reg++;
        } else {
          named_stack_bytes += 8;
        }
      }
    }
  }
  TargetInstruction* frame_ptr = Emit(rv, NewInstruction(X86_64_OP(fp)));
  TargetInstruction* overflow_addr = AddImmediate(
      rv, frame_ptr, 8 + space_above_frame_pointer + named_stack_bytes);
  StoreApField(rv, node->inputs.value.p[0], 8, overflow_addr, X86_64_OP(storeq));

  TargetInstruction* reg_save = Emit(rv, NewInstruction(X86_64_OP(fp)));
  StoreApField(rv, node->inputs.value.p[0], 16, reg_save, X86_64_OP(storeq));

  return SetLoweredNode(node, reg_save);
}

static TargetInstruction* LowerBuiltinVaArg(X86_64Generator* rv, IRNode* node) {
  IRNode* ap_node = node->inputs.value.p[0];
  int64_t arg_size = ((IRConstant*)node->inputs.value.p[1])->value.ivalue;
  int64_t aligned_size = (arg_size + 7) & ~7;

  // Integer arguments are fetched from the integer portion of the register
  // save area indexed by gp_offset (va_list field 0, capped at 48 == 6*8).
  // Floating-point arguments live in the vector portion indexed by fp_offset
  // (field 4, starting at 48 and stepping 16 bytes per register, capped at the
  // end of the save area).  Select the right field/cap/stride up front.
  bool va_is_fp = TypeIsFloatingPoint(node->type);
  bool va_is_small_aggregate =
      TypeIsStructOrUnion(node->type) && arg_size <= 8;
  int offset_field = va_is_fp ? 4 : 0;
  int offset_cap = va_is_fp
                       ? (X86_64_VARARG_FP_SAVE_OFFSET + X86_64_NUM_FP_ARGS * 16)
                       : (X86_64_NUM_INT_ARGS * 8);
  int offset_step = va_is_fp ? 16 : 8;

  TargetInstruction* gp_offset =
      LoadApField(rv, ap_node, offset_field, X86_64_OP(loadl));
  TargetInstruction* reg_save =
      LoadApField(rv, ap_node, 16, X86_64_OP(loadq));

  X86_64Opcode load_opcode = X86_64_OP(loadq);
  if (arg_size == 4) {
    load_opcode = X86_64_OP(loadl);
  } else if (arg_size == 1) {
    load_opcode = TypeIsUnsigned(node->type) ? X86_64_OP(loadb_z)
                                             : X86_64_OP(loadb);
  } else if (arg_size == 2) {
    load_opcode = TypeIsUnsigned(node->type) ? X86_64_OP(loadw_z)
                                             : X86_64_OP(loadw);
  } else if (TypeIsFloatingPoint(node->type)) {
    load_opcode =
        X86_64FpIsDoubleWidth(node->type) ? X86_64_OP(loadsd) : X86_64_OP(loadss);
  }

  TargetInstruction* overflow_label =
      TargetNewInstruction((TargetOpcode)X86_64_OP(label));
  TargetInstruction* done_label =
      TargetNewInstruction((TargetOpcode)X86_64_OP(label));

  // Merge the *address* of the argument (which is always a pointer, regardless
  // of the argument's own type) across the reg-save / overflow diamond, and
  // perform the load in the join block below.  Merging the loaded value here
  // instead would be unsound: a value defined only inside the two arms is
  // invisible to the join block's live-in set, which the dominator-tree
  // liveness derives solely from the join's immediate dominator's live-out
  // set.  The allocator would then treat the merged register as free in the
  // join and reuse it, clobbering the result before it is consumed (e.g. an
  // integer va_arg fed straight into a following call would come back as some
  // other argument's value).  The merge target `addr` is therefore created in
  // this pre-branch block so it is live through the join, and the reg-save arm
  // builds on it (addr += gp_offset) so the initial definition is a real use
  // rather than dead code.
  TargetInstruction* addr = Emit(rv, NewInstruction1(X86_64_OP(mv), reg_save));

  Emit(rv, NewInstruction2(X86_64_OP(cmp), gp_offset,
                           GetIntConstant(rv, NULL, kTargetType32Bit, offset_cap)));
  Emit(rv, NewInstruction1(X86_64_OP(jge), overflow_label));

  // Register-save arm: addr = reg_save + gp_offset; advance gp_offset.
  TargetInstruction* reg_addr =
      NewInstruction2(X86_64_OP(add), addr, gp_offset);
  reg_addr->dest = addr;
  Emit(rv, reg_addr);
  StoreApField(rv, ap_node, offset_field,
               AddImmediate(rv, gp_offset, offset_step),
               X86_64_OP(storel));
  Emit(rv, NewInstruction1(X86_64_OP(jmp), done_label));

  // Overflow arm: addr = overflow_area; advance overflow_arg_area.
  Emit(rv, overflow_label);
  TargetInstruction* overflow_area =
      LoadApField(rv, ap_node, 8, X86_64_OP(loadq));
  TargetInstruction* mv_stack = NewInstruction1(X86_64_OP(mv), overflow_area);
  mv_stack->dest = addr;
  Emit(rv, mv_stack);
  StoreApField(rv, ap_node, 8,
               AddImmediate(rv, overflow_area, aligned_size),
               X86_64_OP(storeq));

  Emit(rv, done_label);

  // Materialize the result in the join block from the merged address.  For a
  // small aggregate the value *is* the address (the caller copies from it).
  TargetInstruction* result =
      va_is_small_aggregate
          ? addr
          : Emit(rv, NewInstruction2(
                         load_opcode, addr,
                         GetIntConstant(rv, NULL, kTargetType32Bit, 0)));
  return SetLoweredNode(node, result);
}

static TargetInstruction* LowerBuiltinVaEnd(X86_64Generator* rv, IRNode* node) {
  (void)rv;
  (void)node;
  return NULL;
}

static TargetInstruction* LowerBuiltinVaCopy(X86_64Generator* rv, IRNode* node) {
  IRNode* dest = node->inputs.value.p[0];
  IRNode* src = node->inputs.value.p[1];
  StoreApField(rv, dest, 0, LoadApField(rv, src, 0, X86_64_OP(loadl)),
               X86_64_OP(storel));
  StoreApField(rv, dest, 4, LoadApField(rv, src, 4, X86_64_OP(loadl)),
               X86_64_OP(storel));
  StoreApField(rv, dest, 8, LoadApField(rv, src, 8, X86_64_OP(loadq)),
               X86_64_OP(storeq));
  StoreApField(rv, dest, 16, LoadApField(rv, src, 16, X86_64_OP(loadq)),
               X86_64_OP(storeq));
  return SetLoweredNode(node, NULL);
}

static TargetInstruction* LowerLocation(X86_64Generator* rv, IRNode* node) {
  IRLocation* loc = (IRLocation*)node;
  return SetLoweredNode(node, Emit(rv, TargetNewLocation(loc)));
}

static TargetInstruction* LowerStackPointerOps(X86_64Generator* rv, IRNode* node) {
  switch (node->opcode) {
    case IR_OP(decsp): {
      TargetInstruction* size = Materialize(rv, node->inputs.value.p[0]);
      TargetInstruction* new_sp;
      if (TargetIsConst(size)) {
        int64_t s = X86_64IntValue(size);
        new_sp = AddImmediate(rv, StackPointer(rv), -s);
      } else {
        new_sp = Emit(rv, NewInstruction2(X86_64_OP(sub), StackPointer(rv), size));
      }
      new_sp->dest = StackPointer(rv);
      return new_sp;
    }
    case IR_OP(savesp): {
      // One operand, a temp to hold stack pointer.
      TargetInstruction* tmp = Materialize(rv, node->inputs.value.p[0]);
      TargetInstruction* mv = NewInstruction1(X86_64_OP(mv), StackPointer(rv));
      mv->dest = tmp;
      return tmp;
    }
    case IR_OP(restoresp): {
      TargetInstruction* tmp = Materialize(rv, node->inputs.value.p[0]);
      TargetInstruction* mv = NewInstruction1(X86_64_OP(mv), tmp);
      mv->dest = StackPointer(rv);
      return mv->dest;
    }
    default:
      assert(false);
      return NULL;
  }
}

static TargetInstruction* LowerIRNode(X86_64Generator* rv, Generator* gen,
                                      IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  switch (node->opcode) {
    case IR_OP(popcounti):
      assert(false && "popcount must be software-expanded before x86-64 lowering");
      return NULL;

    case IR_OP(localvar):
    case IR_OP(argument):
    case IR_OP(tempvar):
    case IR_OP(staticvar):
    case IR_OP(externvar):
      // These are handled before we get here.
      return NULL;

    case IR_OP(nop):
    case last_ir_opcode:
      return NULL;

    case IR_OP(nrvoval):
      return Emit(rv, NewInstruction1(X86_64_OP(nrvoval), Materialize(rv, node->inputs.value.p[0])));
      
    case IR_OP(ssavar):
    case IR_OP(phi):
      // We should never see these as we've moved out of SSA form
      // before here.
      break;

    case IR_OP(structreturn): {
      // Always allocate a saved register for the struct return value.
      // rv->struct_return_reg = rv->num_int_reg_vars++;
      TargetInstruction* result =
          SetLoweredNode(node, EmitSymbol(rv, NewInstruction(X86_64_OP(structreturn))));
      TargetInstruction* mv = Emit(rv, NewInstruction1(X86_64_OP(mv),
                  IncomingIntArgumentRegister(rv, 0)));
      mv->dest = result;
      return result;
    }

    case IR_OP(literalref):
      return LowerLiteralReference(rv, gen, node);

    case IR_OP(addressof):
      return LowerAddressOf(rv, node);

    case IR_OP(const32):
    case IR_OP(consta):
      return GetIntConstant(rv, node, kTargetType32Bit,
                            ((IRConstant*)node)->value.ivalue);
    case IR_OP(const8):
      return GetIntConstant(rv, node, kTargetType8Bit,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(const16):
      return GetIntConstant(rv, node, kTargetType16Bit,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(const64):
      return GetIntConstant(rv, node, kTargetType64Bit,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(constf):
      return GetFloatingPointConstant(rv, node, kTargetTypeFloat,
                                      ((IRConstant*)node)->value.fvalue);

    case IR_OP(constd):
      return GetFloatingPointConstant(rv, node, kTargetTypeDouble,
                                      ((IRConstant*)node)->value.fvalue);

    case IR_OP(enter):
      // The prologue (save) is emitted at the start of X86_64Lower, before
      // variable lowering, so that argument spills land after the frame setup.
      return NULL;

    case IR_OP(leave):
      Emit(rv, NewInstruction(X86_64_OP(restore)));
      return NULL;

    case IR_OP(ret):
      return Emit(rv, NewInstruction(X86_64_OP(ret)));
      
    case IR_OP(load32):
    case IR_OP(load8):
    case IR_OP(load64):
    case IR_OP(load16):
    case IR_OP(loadu32):
    case IR_OP(loadu8):
    case IR_OP(loadu16):
    case IR_OP(loadf):
    case IR_OP(loadd):
    case IR_OP(loada):
      return LowerLoad(rv, gen, node);

      // stores.
    case IR_OP(store32):
    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store64):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
      return LowerStore(rv, node);

    case IR_OP(inc8):
    case IR_OP(inc16):
    case IR_OP(inc32):
    case IR_OP(inc64):
    case IR_OP(uinc8):
    case IR_OP(uinc16):
    case IR_OP(uinc32):
    case IR_OP(uinc64):
   case IR_OP(inca):
    case IR_OP(incf):
    case IR_OP(incd):
      return LowerInc(rv, node);
    case IR_OP(dec8):
     case IR_OP(dec16):
     case IR_OP(dec32):
     case IR_OP(dec64):
    case IR_OP(udec8):
     case IR_OP(udec16):
     case IR_OP(udec32):
     case IR_OP(udec64):
    case IR_OP(deca):
     case IR_OP(decf):
     case IR_OP(decd):
    return LowerDec(rv, node);
      
    case IR_OP(getbit):
      return LowerGetBitField(rv, node);
      
    case IR_OP(setbit):
      return LowerSetBitField(rv, node);

    case IR_OP(addi):
    case IR_OP(addf):
    case IR_OP(addd):
    case IR_OP(adda):

    case IR_OP(subi):
    case IR_OP(subf):
    case IR_OP(subd):
    case IR_OP(suba):

    case IR_OP(muli):
    case IR_OP(mulf):
    case IR_OP(muld):

    case IR_OP(divi):
    case IR_OP(divf):
    case IR_OP(divd):

    case IR_OP(modi):

    case IR_OP(lsri):
    case IR_OP(asri):
    case IR_OP(lsli):
    case IR_OP(rotli):
    case IR_OP(rotri):
    case IR_OP(clzi):
    case IR_OP(ctzi):

    case IR_OP(ori):
    case IR_OP(andi):
    case IR_OP(xori):

    case IR_OP(onescomp):
    case IR_OP(negi):
    case IR_OP(negf):
    case IR_OP(negd):

    case IR_OP(i2f):
    case IR_OP(i2d):
    case IR_OP(f2d):
    case IR_OP(d2f):
    case IR_OP(f2i):
    case IR_OP(d2i):

    case IR_OP(movi):
    case IR_OP(movf):
    case IR_OP(movd):
    case IR_OP(mova):
    case IR_OP(tmp):
      return LowerExpression(rv, gen, node);

    case IR_OP(noti):
    case IR_OP(nota):
      return LowerLogicalNot(rv, gen, node);

//    case IR_OP(rmovi):
//    case IR_OP(rmovf):
//    case IR_OP(rmovd):
//    case IR_OP(rmova):
//      return LowerRmov(rv, node);
      
    case IR_OP(cmpeqi):
    case IR_OP(cmpnei):
    case IR_OP(cmplti):
    case IR_OP(cmplei):
    case IR_OP(cmpgti):
    case IR_OP(cmpgei):

    case IR_OP(cmpeqa):
    case IR_OP(cmpnea):
    case IR_OP(cmplta):
    case IR_OP(cmplea):
    case IR_OP(cmpgta):
    case IR_OP(cmpgea):
      // Integer/pointer comparisons produce a 0/1 value (sete/setne/...).  If
      // the comparison feeds a merged value (&&/||/?:), route the result into
      // its destination register so the merge variable is actually written.
      return RouteResultToDest(rv, gen, node, LowerComparison(rv, node));

    case IR_OP(cmpeqf):
    case IR_OP(cmpnef):
    case IR_OP(cmpltf):
    case IR_OP(cmplef):
    case IR_OP(cmpgtf):
    case IR_OP(cmpgef):

    case IR_OP(cmpeqd):
    case IR_OP(cmpned):
    case IR_OP(cmpltd):
    case IR_OP(cmpled):
    case IR_OP(cmpgtd):
    case IR_OP(cmpged):
      // Floating comparisons also participate in && / || / ?: merges. Route
      // their 0/1 result into an explicit IR destination just like integer
      // comparisons; otherwise the merge temporary retains a stale register.
      return RouteResultToDest(rv, gen, node, LowerComparison(rv, node));

    case IR_OP(cmp3wayi):
    case IR_OP(cmp3wayu):
    case IR_OP(cmp3waya):
    case IR_OP(cmp3wayf):
    case IR_OP(cmp3wayd):
      return RouteResultToDest(rv, gen, node, LowerThreeWay(rv, node));

    case IR_OP(btrue):
    case IR_OP(bfalse):
      return LowerConditionalBranch(rv, node);

    case IR_OP(bra):
      return LowerBranch(rv, node);

    case IR_OP(cbra):
      return LowerComputedBranch(rv, node);

    case IR_OP(label):
      return LowerLabel(rv, node);

    case IR_OP(named_label):
      return LowerNamedLabel(rv, node);

    case IR_OP(pusharg):
      return SetLoweredNode(node, Materialize(rv, node->inputs.value.p[0]));
      
    case IR_OP(calla):
      return LowerCall(rv, gen, node);

    case IR_OP(structarg):
      // Same as its input.
      return SetLoweredNode(node, Materialize(rv, node->inputs.value.p[0]));
      
    case IR_OP(resulti):
    case IR_OP(resultf):
    case IR_OP(resultd):
    case IR_OP(resulta):
      return LowerResult(rv, node);

    case IR_OP(memzero):
      return LowerMemzero(rv, node);

    case IR_OP(memcpy):
      return LowerMemcpy(rv, node);

    case IR_OP(cast): {
      // A cast is otherwise a no-op pass-through of its input value, but it may
      // carry a destination (the "-> $n" annotation) when it is a ?:/&&/||
      // merge target or, crucially, the stashed result of a call evaluated
      // alongside a sibling call (kIRStashedCallResult).  In that case the
      // value must actually be moved into the destination register, exactly as
      // LowerExpression routes its conversions; otherwise the stash register is
      // never written and is read uninitialized (the value collapses to 0).
      TargetInstruction* inst = Materialize(rv, node->inputs.value.p[0]);
      TargetInstruction* dest = GetDestInstruction(rv, gen, node);
      if (dest != NULL && inst != NULL) {
        X86_64Opcode mov_opcode = X86_64_OP(mv);
        if (TypeIsFloatingPoint(node->type)) {
          mov_opcode =
              node->type->size > 4 ? X86_64_OP(fmv_d) : X86_64_OP(fmv_s);
        }
        inst = SetDestOrMove(rv, inst, dest, mov_opcode);
      }
      return SetLoweredNode(node, inst);
    }

    case IR_OP(zeroextendi):
      return LowerZeroExtend(rv, gen, node);

    case IR_OP(signextendi):
      return LowerSignExtend(rv, gen, node);

    case IR_OP(aligni):
       return LowerAlign(rv, node);

    case IR_OP(asm):;
      return LowerAsm(rv, node);

    case IR_OP(loc):
      return LowerLocation(rv, node);

    case IR_OP(builtin_va_start):
      return LowerBuiltinVaStart(rv, node);

    case IR_OP(builtin_va_arg):
      return LowerBuiltinVaArg(rv, node);

    case IR_OP(builtin_va_end):
      return LowerBuiltinVaEnd(rv, node);

    case IR_OP(builtin_va_copy):
      return LowerBuiltinVaCopy(rv, node);

    case IR_OP(atomic_load):
      return LowerAtomicLoad(rv, gen, node);

    case IR_OP(atomic_store):
      return LowerAtomicStore(rv, node);

    case IR_OP(atomic_fetch_add):
      return LowerAtomicFetchAddSub(rv, gen, node, true, false);

    case IR_OP(atomic_fetch_sub):
      return LowerAtomicFetchAddSub(rv, gen, node, false, false);

    case IR_OP(atomic_add_fetch):
      return LowerAtomicFetchAddSub(rv, gen, node, true, true);

    case IR_OP(atomic_sub_fetch):
      return LowerAtomicFetchAddSub(rv, gen, node, false, true);

    case IR_OP(atomic_compare_exchange_bool):
      return LowerAtomicCompareExchange(rv, gen, node, false, true);

    case IR_OP(atomic_compare_exchange_val):
      return LowerAtomicCompareExchange(rv, gen, node, false, false);

    case IR_OP(atomic_compare_exchange_n):
      return LowerAtomicCompareExchange(rv, gen, node, true, true);

    case IR_OP(atomic_fence):
      if (AtomicIRConstant(node->inputs.value.p[0]) == 5) {
        return SetLoweredNode(node, EmitAtomicFence(rv));
      }
      return SetLoweredNode(node,
                            GetIntConstant(rv, node, kTargetType32Bit, 0));
      
    case IR_OP(decsp):
    case IR_OP(savesp):
    case IR_OP(restoresp):
      return LowerStackPointerOps(rv, node);
  }

  // If we get here we've failed to handle the IR node.
  assert(false);
  return NULL;
}

// Calculate the size of an argument based on its type.
static int64_t CalculateArgumentSize(IRNode* arg) {
  if (TypeIsFloatingPoint(arg->type)) {
    return 8;
  }
  if (TypeIsPointerOrArray(arg->type)) {
    return 8;
  }
  if (TypeIsStructOrUnion(arg->type)) {
    return arg->type->info.struct_info->size;
  }
  return arg->type->size < 4 ? 4 : arg->type->size;
}

static COMPILER_UNUSED int CompareRegisterVar(const void* a, const void* b) {
  const PoolEntry* var1 = *(const PoolEntry**)a;
  const PoolEntry* var2 = *(const PoolEntry**)b;

  Symbol* sym1 = var1->value.symbol;
  Symbol* sym2 = var2->value.symbol;
  int weight1 = sym1->usage_info.reads * (sym1->usage_info.used_in_loop + 1);
  int weight2 = sym2->usage_info.reads * (sym2->usage_info.used_in_loop + 1);

  return weight2 - weight1;
  
  // Sorted in reverse order, highest first.
  //return (int)(var2->pooled->outputs.length - var1->pooled->outputs.length);
}

// Work out where an argument is located.  It will either be in a register
// or on the stack.  If it's in a register, floating points arguments are in the
// fp regs.  All other types are in integer registers.
// Returns the argument location in an ArgLocation struct.  Type type field
// says where it is (in reg or stack) and the location.offset field is either
// the register number or stack offset (from s0 - the frame pointer).
static ArgLocation ArgumentLocation(PoolEntry* arg, Vector* args,
                                    TypeRecord* func) {
  IRVariable* var = (IRVariable*)arg->pooled;
  size_t arg_num = var->symbol->value.arg_number;
  bool is_struct_return =
      func != NULL && TypeIsStructOrUnion(func->next);
  int int_reg = X86_64_INT_ARG_START;
  if (is_struct_return) {
    int_reg +=
        1;  // For struct returns, the first arg is the address of the struct.
  }
  int fp_reg = X86_64_FP_ARG_START;
  int stack_offset = 0;
  for (size_t i = 0; i < args->length; i++) {
    if (i == arg_num) {
      ArgLocation location = {0};
      if (TypeIsMemberPointerAggregate(arg->pooled->type)) {
        if (int_reg + 1 <= X86_64_INT_ARG_END) {
          location.type = kArgLocationRegisterPair;
          location.location.offset = int_reg;
          location.second_offset = int_reg + 1;
        } else {
          location.type = kArgLocationPushedPair;
          location.location.offset = stack_offset;
          location.second_offset = stack_offset + 8;
        }
      } else if (TypeIsFloatingPoint(arg->pooled->type)) {
        if (fp_reg <= X86_64_FP_ARG_END) {
          // Arg is in a floating point register.
          location.type = kArgLocationRegister;
          location.location.offset = fp_reg;
        } else {
          location.type = kArgLocationPushed;
          location.location.offset = stack_offset;
        }
      } else {
        if (int_reg <= X86_64_INT_ARG_END) {
          // Arg is in an integer register.
          location.type = kArgLocationRegister;
          location.location.offset = int_reg;
        } else {
          location.type = kArgLocationPushed;
          location.location.offset = stack_offset;
        }
      }
      return location;
    }

    // Account for the space consumed by this preceding argument.  The
    // register/stack split here must match both the target check above and the
    // caller-side pushing logic in LowerCall (which uses X86_64_NUM_INT_ARGS /
    // X86_64_NUM_FP_ARGS argument registers and always reserves 8-byte stack
    // slots).
    Symbol* arg_symbol = args->value.p[i];
    if (TypeIsMemberPointerAggregate(arg_symbol->type)) {
      if (int_reg + 1 <= X86_64_INT_ARG_END) {
        int_reg += 2;
      } else {
        stack_offset += 16;
      }
    } else if (TypeIsFloatingPoint(arg_symbol->type)) {
      if (fp_reg <= X86_64_FP_ARG_END) {
        fp_reg++;
      } else {
        stack_offset += 8;
      }
    } else {
      if (int_reg <= X86_64_INT_ARG_END) {
        int_reg++;
      } else {
        stack_offset += 8;
      }
    }
  }
  // Can't find argument.
  assert(false);
  ArgLocation error = {0};
  return error;
}

static void AlignOffset(PoolEntry* entry, int* offset) {
  int alignment = TypeRecordAlignment(entry->pooled->type);
  *offset = (*offset + (alignment - 1)) & ~(alignment - 1);
}

static void SetDebugRegisterLocation(PoolEntry* entry, int reg) {
  VariableDIESetRegister(entry->value.symbol->die, reg);
}

static void SetDebugStackLocation(PoolEntry* entry, int offset) {
  VariableDIESetStackOffset(entry->value.symbol->die, offset);
}

static void SetDebugSymbolLocation(PoolEntry* entry) {
  VariableDIESetStatic(entry->value.symbol->die,
                       entry->value.symbol->name.value);
}

static TargetInstruction* LoadFpArgumentIntoRegisterVariable(X86_64Generator* rv,
                                                             int reg_var,
                                             ArgLocation arg_loc,
                                             IRNode* symbol) {
  switch (arg_loc.type) {
  case kArgLocationRegister:
    case kArgLocationPassedByReferenceInRegister: {
      IRVariable* sym = (IRVariable*)symbol;
      TargetInstruction* var = FloatingPointVariableRegister(rv, reg_var, sym->symbol);
      X86_64Opcode move_op = X86_64FpIsDoubleWidth(symbol->type) ? X86_64_OP(fmv_d) : X86_64_OP(fmv_s);
      TargetInstruction* mv = Emit(
          rv, NewInstruction1(
                  move_op,
                  IncomingFloatingPointArgumentRegister(
                      rv,
                      (int)arg_loc.location.offset - X86_64_FP_ARG_START)));
      mv->dest = var;
      return var;
    }
    case kArgLocationPushed:
    case kArgLocationPassedByReferenceOnStack: {
      // Stack-passed argument bound to a register variable: load it from the
      // incoming stack slot and move it into the variable register so the
      // register actually holds the value (and the allocator keeps it live).
      IRVariable* sym = (IRVariable*)symbol;
      TargetInstruction* var =
          FloatingPointVariableRegister(rv, reg_var, sym->symbol);
      TargetInstruction* loaded = PopArg(rv, symbol, arg_loc.location.offset);
      X86_64Opcode move_op =
          X86_64FpIsDoubleWidth(symbol->type) ? X86_64_OP(fmv_d) : X86_64_OP(fmv_s);
      TargetInstruction* mv = Emit(rv, NewInstruction1(move_op, loaded));
      mv->dest = var;
      return var;
    }
    case kArgLocationRegisterPair:
    case kArgLocationPushedPair:
      assert(false);
      return NULL;
  }
  return NULL;
}

static TargetInstruction* LoadIntArgumentIntoRegisterVariable(X86_64Generator* rv,
                                                              int reg_var,
                                             ArgLocation arg_loc,
                                             IRNode* symbol) {
  switch (arg_loc.type) {
  case kArgLocationRegister:
    case kArgLocationPassedByReferenceInRegister: {
      IRVariable* sym = (IRVariable*)symbol;
      TargetInstruction* var = IntVariableRegister(rv, reg_var, sym->symbol);
      TargetInstruction* mv = Emit(rv, NewInstruction1(X86_64_OP(mv),
                 IncomingIntArgumentRegister(rv,
                          (int)arg_loc.location.offset - X86_64_INT_ARG_START)));
      mv->dest = var;
      return var;
    }
    case kArgLocationPassedByReferenceOnStack:
    case kArgLocationPushed: {
      // Stack-passed argument bound to a register variable: load it from the
      // incoming stack slot and move it into the variable register so the
      // register actually holds the value (and the allocator keeps it live).
      IRVariable* sym = (IRVariable*)symbol;
      TargetInstruction* var = IntVariableRegister(rv, reg_var, sym->symbol);
      TargetInstruction* loaded = PopArg(rv, symbol, arg_loc.location.offset);
      TargetInstruction* mv = Emit(rv, NewInstruction1(X86_64_OP(mv), loaded));
      mv->dest = var;
      return var;
    }
    case kArgLocationRegisterPair:
    case kArgLocationPushedPair:
      assert(false);
      return NULL;
  }
  return NULL;
}

// Assign a register to a variable or argument if possible.  The
// var_offset is below the stack frame.
static void AssignRegisterOrOffset(X86_64Generator* rv, PoolEntry* entry,
                                   Vector* args, TypeRecord* func,
                                   int* var_offset) {
  // Variable length arrays are not given offsets until their block
  // is entered.
  if (TypeIsVLA(entry->pooled->type)) {
    return;
  }
  switch (entry->pooled->opcode) {
    case IR_OP(argument):
    case IR_OP(localvar):
    case IR_OP(tempvar):
    case IR_OP(staticvar):
    case IR_OP(externvar): {
      IRVariable* variable = (IRVariable*)entry->pooled;
      if (variable->symbol != NULL && variable->symbol->type != NULL) {
        TypeRecordCalculateSize(variable->symbol->type);
        IRSetType(entry->pooled, variable->symbol->type);
      }
      break;
    }
    default:
      break;
  }
  bool is_arg = entry->pooled->opcode == IR_OP(argument);
  TypeRecordCalculateSize(entry->pooled->type);
  int64_t size =
      is_arg ? CalculateArgumentSize(entry->pooled) : entry->pooled->type->size;
  assert(size != 0);

  // printf("var %s\n", ((IRVariable*)entry->pooled)->symbol->name.value);
  if (TypeIsFloatingPoint(entry->pooled->type)) {
    if (UseRegisterForVariable(rv, entry->pooled)) {
      int reg = rv->num_fp_reg_vars++;
      entry->pooled->data.ivalue = X86_64_REG_VAR | reg;
      SetDebugRegisterLocation(entry, reg);
      if (is_arg) {
        ArgLocation location = ArgumentLocation(entry, args, func);
        if (location.type == kArgLocationRegister) {
          rv->num_fp_arg_regs++;
        }
        LoadFpArgumentIntoRegisterVariable(rv, reg, location, entry->pooled);
      }
    } else {
      if (is_arg) {
        ArgLocation location = ArgumentLocation(entry, args, func);
        if (location.type == kArgLocationRegister) {
          // Address-taken FP parameters need a stack home just like integer
          // parameters.  Defer the xmmN -> stack store to the prologue so rbp is
          // established before we address the local slot.
          int offset = -16 - rv->saved_arg_area_size - 8;
          rv->saved_arg_area_size += 8;
          entry->pooled->data.ivalue = offset;
          SavedArgumentRegister* saved = NewSavedArgumentRegister(
              (int)location.location.offset, X86_64_FP_REG, offset,
              /*is_fp=*/true, X86_64FpIsDoubleWidth(entry->pooled->type) ? 8 : 4);
          VectorAppend(&rv->saved_regs, saved);
          rv->num_fp_arg_regs++;
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        } else {
          // Stack-passed FP argument that remains in the caller-provided stack
          // slot.  Since it is addressed rbp-relative, force a frame.
          int space_above_frame_pointer =
              compiler->current_function->info.function.varargs
                  ? X86_64_VARARG_SAVE_AREA_SIZE
                  : 0;
          entry->pooled->data.ivalue =
              (int)location.location.offset + 8 + space_above_frame_pointer;
          rv->has_incoming_stack_args = true;
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        }
      } else {
        AlignOffset(entry, var_offset);
        entry->pooled->data.ivalue = *var_offset;
        SetDebugStackLocation(entry, *var_offset);
        *var_offset += size;
      }
    }
  } else if (TypeIsMemberPointerAggregate(entry->pooled->type)) {
    AlignOffset(entry, var_offset);
    if (is_arg) {
      ArgLocation location = ArgumentLocation(entry, args, func);
      if (location.type == kArgLocationRegisterPair) {
        int offset = -16 - rv->saved_arg_area_size - 16;
        rv->saved_arg_area_size += 16;
        entry->pooled->data.ivalue = offset;
        SetDebugStackLocation(entry, offset);
        VectorAppend(&rv->saved_regs,
                     NewSavedArgumentRegister(
                         (int)location.location.offset, X86_64_FP_REG, offset,
                         /*is_fp=*/false, 8));
        VectorAppend(&rv->saved_regs,
                     NewSavedArgumentRegister(
                         (int)location.second_offset, X86_64_FP_REG, offset + 8,
                         /*is_fp=*/false, 8));
        rv->num_int_arg_regs += 2;
      } else {
        entry->pooled->data.ivalue =
            (int)location.location.offset + 8;
        rv->has_incoming_stack_args = true;
        SetDebugStackLocation(entry, entry->pooled->data.ivalue);
      }
    } else {
      entry->pooled->data.ivalue = *var_offset;
      SetDebugStackLocation(entry, *var_offset);
      *var_offset += size;
    }
  } else if (TypeIsStructOrUnion(entry->pooled->type)) {
    if (is_arg) {
      if (size <= 8) {
        // Less than a pointer, passed in reg
        if (UseRegisterForVariable(rv, entry->pooled)) {
          // TODO: if this is a leaf procedure we can keep them in the arg regs.
          int reg = rv->num_int_reg_vars++;
          entry->pooled->data.ivalue = X86_64_REG_VAR | reg;
          SetDebugRegisterLocation(entry, reg);
          ArgLocation location = ArgumentLocation(entry, args, func);
          if (location.type == kArgLocationRegister) {
            // Named small-struct argument that consumed an integer argument
            // register; count it for va_start's gp_offset.
            rv->num_int_arg_regs++;
          }
          LoadIntArgumentIntoRegisterVariable(rv, reg, location, entry->pooled);
        } else {
          ArgLocation location = ArgumentLocation(entry, args, func);
          if (location.type == kArgLocationRegister) {
            int offset = -16 - rv->saved_arg_area_size - 8;
            rv->saved_arg_area_size += 8;
            entry->pooled->data.ivalue = offset;
            SetDebugStackLocation(entry, offset);
            rv->num_int_arg_regs++;
            VectorAppend(&rv->saved_regs,
                         NewSavedArgumentRegister(
                             (int)location.location.offset, X86_64_FP_REG,
                             offset, /*is_fp=*/false, 8));
          } else {
            AlignOffset(entry, var_offset);
            entry->pooled->data.ivalue = *var_offset;
            SetDebugStackLocation(entry, *var_offset);
            *var_offset += size;
          }
        }
      } else {
        // Passed by reference: the caller copied the struct onto its stack and
        // passed the address of that copy in an argument register (or on the
        // stack).  Because the address of this parameter can be taken (and the
        // ABI nominally forbids mutating the caller's copy), make our own copy
        // in a fixed slot of the saved-argument area and address the parameter
        // there.  Fixed (frame-pointer relative, stack_frame_size-independent)
        // offsets let us emit the copy here, before the frame size is known.
        int copy_size = (int)((size + 7) & ~7);
        int offset = -16 - rv->saved_arg_area_size - copy_size;
        rv->saved_arg_area_size += copy_size;
        entry->pooled->data.ivalue = offset;
        SetDebugStackLocation(entry, offset);

        ArgLocation location = ArgumentLocation(entry, args, func);
        if (location.type == kArgLocationRegister) {
          // The pointer to the caller's copy arrived in an argument register.
          // Defer the copy to the prologue (after the frame pointer is set up)
          // by recording it in saved_regs; emitting it inline here would run
          // before `enter`/push rbp and use a stale frame pointer.
          rv->num_int_arg_regs++;
          SavedArgumentRegister* saved = NewSavedArgumentRegister(
              (int)location.location.offset, X86_64_FP_REG, offset,
              /*is_fp=*/false, 8);
          saved->copy_bytes = (int)size;
          VectorAppend(&rv->saved_regs, saved);
        } else {
          // The pointer was passed on the stack just above the frame; copy now
          // (these reads/writes go through the stack/frame pointer which the
          // emitted prologue will have established for stack-relative access).
          TargetInstruction* src_ptr =
              PopArg(rv, entry->pooled, location.location.offset);
          Memcpy(rv, FramePointer(rv), src_ptr, (int)size, 0, offset,
                 /*count_as_call=*/false);
        }
      }
    } else {
      // Not an argument.
      // TODO: it is possible to put small structs in registers.
      if ((entry->pooled->flags & kIRNrvoMarker) != 0) {
        // Named RVO symbol.  This assigned the same register as the
        // structreturn.
        entry->pooled->data.ivalue = X86_64_REG_VAR | rv->struct_return_reg;
        TargetInstruction* var = IntVariableRegister(rv, rv->struct_return_reg, entry->value.symbol);
        Emit(rv, NewInstruction2(X86_64_OP(mv), var,
                    IncomingIntArgumentRegister(rv, 0)));
        SetDebugRegisterLocation(entry, rv->struct_return_reg);
      } else {
        AlignOffset(entry, var_offset);
        entry->pooled->data.ivalue = *var_offset;
        SetDebugStackLocation(entry, *var_offset);
        *var_offset += size;
      }
    }
  } else if (TypeIsVLA(entry->pooled->type)) {
    // No stack spac allocated for it at entry.  It's allocated
    // by the generated code.
  } else if (TypeIsArray(entry->pooled->type) &&
             !is_arg) {
    // Array local variable, always on the stack.
    AlignOffset(entry, var_offset);
    entry->pooled->data.ivalue = *var_offset;
    SetDebugStackLocation(entry, *var_offset);
    *var_offset += size;
  } else if (TypeIsFunction(entry->pooled->type)) {
    IRVariable* var = (IRVariable*)entry->pooled;
    TargetInstruction* inst = GetSymbol(rv, NULL, var->symbol);
    entry->pooled->data.ptr = inst;
    SetDebugSymbolLocation(entry);
  } else {
    // Integer or pointer.
    if (UseRegisterForVariable(rv, entry->pooled) &&
        X86_64HasFreeIntRegVar(rv)) {
      int reg = rv->num_int_reg_vars++;
      entry->pooled->data.ivalue = X86_64_REG_VAR | reg;
      SetDebugRegisterLocation(entry, reg);
      if (is_arg) {
        // Argument, load it into a register.
        ArgLocation location = ArgumentLocation(entry, args, func);
        if (location.type == kArgLocationRegister) {
          // This named argument consumed an integer argument register; count it
          // so va_start's gp_offset skips past all named register arguments.
          rv->num_int_arg_regs++;
        }
        LoadIntArgumentIntoRegisterVariable(rv,
                                            reg, location, entry->pooled);
      }
    } else {
      if (is_arg) {
        // The argument is not going to be placed in a register.  We need
        // to make sure it's on the stack.  It is already on the stack
        // if it is not passed in a0..a7.  But if is in an arg reg
        // we need to save it to the stack frame.
        ArgLocation location = ArgumentLocation(entry, args, func);
        if (location.type == kArgLocationRegister) {
          // Argument is in a register so we need to save it to the stack. These
          // are stored immediately below the saved frame pointer (24 bytes
          // below the previous stack pointer).
          int offset = -16 - rv->saved_arg_area_size - 8;
          rv->saved_arg_area_size += 8;
          SavedArgumentRegister* saved = NewSavedArgumentRegister(
              (int)location.location.offset, X86_64_FP_REG, offset,
              /*is_fp=*/false, 8);
          entry->pooled->data.ivalue = offset;
          VectorAppend(&rv->saved_regs, saved);
          rv->num_int_arg_regs++;  // Argument was passed in a register.
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        } else {
          // Stack-passed argument that stays on the stack (no register
          // assigned).  It lives in the caller's frame just above our return
          // address.  rbp points at the return-address slot, so the first
          // stack argument is at rbp+8.  For varargs procedures the prologue
          // lowers rbp by space_above_frame_pointer, so add that back here.
          int space_above_frame_pointer =
              compiler->current_function->info.function.varargs
                  ? X86_64_VARARG_SAVE_AREA_SIZE
                  : 0;
          entry->pooled->data.ivalue =
              (int)location.location.offset + 8 + space_above_frame_pointer;
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        }
      } else {
        AlignOffset(entry, var_offset);
        entry->pooled->data.ivalue = *var_offset;
        SetDebugStackLocation(entry, *var_offset);
        *var_offset += size;
      }
    }
  }
}

static void AssignRegisterVars(X86_64Generator* rv, Vector* vars, Vector* args,
                               TypeRecord* func) {
  // Variables are allocated below the frame, arguments are above or in
  // registers.
  // If the argument is in a register, the top bit of the data.ivalue is
  // set and the low order bits are the register number.

  // Sort the pooled local variables in reverse order of usage.  Those
  // with the largest number of references will be at the start of the
  // vector.
  // qsort(vars->value.p, vars->length, sizeof(IRNode*), CompareRegisterVar);

  int32_t var_offset = 0;

  for (size_t i = 0; i < vars->length; i++) {
    PoolEntry* entry = vars->value.p[i];
    AssignRegisterOrOffset(rv, entry, args, func, &var_offset);
  }
  
  // We now know the stack frame size.  This includes the length of the saved
  // registers.
  rv->base.stack_frame_size =
      (int32_t)var_offset + rv->saved_arg_area_size;
  // Align to 16 byte boundary.
  rv->base.stack_frame_size = (rv->base.stack_frame_size + 15) & ~15;
}

static void LowerVariables(X86_64Generator* rv, Generator* gen) {
  Vector local_vars = {0};
  
  // Collect all local variables so that we can assign some of them
  // to registers.
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = (PoolEntry*)gen->variable_pool.value.p[i];
    switch (entry->pooled->opcode) {
      case IR_OP(localvar):
      case IR_OP(tempvar):
        VectorAppend(&local_vars, entry);
        break;
      case IR_OP(argument):
        VectorAppend(&local_vars, entry);
        break;
      default: {
        // Static variables are referenced by a symbol instruction.
        IRVariable* var = (IRVariable*)entry->pooled;
        TargetInstruction* inst = GetSymbol(rv, NULL, var->symbol);
        entry->pooled->data.ptr = inst;
        break;
      }
    }
  }

  AssignRegisterVars(rv, &local_vars, &gen->func->info.function.prototype,
                     gen->func);
  VectorDestruct(&local_vars);
}

void X86_64Lower(X86_64Generator* rv, Generator* gen) {
  TrapLower(&gen->func->info.function.symbol->name);
  
  // If the function returns a struct, allocate the struct result
  // register now.
  if (TypeIsStructOrUnion(gen->func->next)) {
    rv->struct_return_reg = rv->num_int_reg_vars++;
    rv->struct_return_spill_offset =
        -16 - rv->saved_arg_area_size - 8;
    rv->saved_arg_area_size += 8;
    VectorAppend(&rv->saved_regs,
                 NewSavedArgumentRegister(X86_64_INT_ARG_START,
                                          X86_64_FP_REG,
                                          rv->struct_return_spill_offset,
                                          /*is_fp=*/false, 8));
  }

  // Emit the prologue (save) BEFORE lowering variables.  LowerVariables emits
  // the argument-register moves and may also queue spills of those values; if
  // the prologue (push rbp / frame setup) comes after them, those spill stores
  // use the caller's stale rbp and a stale first_spill_offset (computed when the
  // prologue prints), which is wrong.  Emitting the prologue first guarantees
  // the frame pointer and spill region are established before any rbp-relative
  // store.  The IR enter node (handled below) becomes a no-op in that case.
  bool has_enter = false;
  for (IRNode* scan = GeneratorFirstInstruction(gen); scan != NULL;
       scan = IRNext(scan)) {
    if (scan->opcode == IR_OP(enter)) {
      has_enter = true;
      break;
    }
  }
  if (has_enter) {
    Emit(rv, NewInstruction(X86_64_OP(save)));
  }

  LowerVariables(rv, gen);

  IRNode* node = GeneratorFirstInstruction(gen);
  while (node != NULL) {
    LowerIRNode(rv, gen, node);
    node = IRNext(node);
  }
  ResolveExceptionRanges(rv, gen);

  if (compiler->print_back_end|| compiler->ir_output_file != stdout) {
    X86_64Print(rv, compiler->ir_output_file);
  }
  
  // Build basic blocks for.
  TargetBuildBasicBlocks(&rv->base);
  
  if (compiler->print_back_end|| compiler->ir_output_file != stdout) {
    TargetPrintBasicBlocks(&rv->base, compiler->ir_output_file);
  }
  
  if (OptLevel2()) {
    // Optimize the code sequence for -O2 and above.
    X86_64Optimize(rv);
  
    if (compiler->print_back_end|| compiler->ir_output_file != stdout) {
      fprintf(compiler->ir_output_file, "\n After x86-64 optimization\n");
      TargetPrintBasicBlocks(&rv->base, compiler->ir_output_file);
    }
  }
  // Allocate registers to the instructions.
  X86_64AllocateRegisters(&rv->register_allocator);
}

void X86_64Print(X86_64Generator* rv, FILE* fp) {
  TargetInstruction* inst = TargetFirstInstruction(&rv->base);
  while (inst != NULL) {
    TargetPrintInstruction(inst, X86_64OpcodeName, fp);
    inst = TargetNext(inst);
  }
}
