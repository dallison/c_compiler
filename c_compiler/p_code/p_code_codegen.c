//
//  p_code.c
//  c_compiler
//
//  Created by David Allison on 12/22/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "p_code_codegen.h"
#include <assert.h>
#include <stdlib.h>
#include "compiler.h"
#include "member_pointer.h"
#include "p_code_optimize.h"

const char* PCodeOpcodeName(int op) {
  PCodeOpcode opcode = op;
  switch (opcode) {
    default:
      // Use the generic TargetOpcodeName for all non-p-code specific
      // opcodes.
      return TargetOpcodeName((TargetOpcode)opcode);

    case P_OP(ap):
      return "ap";
    case P_OP(decsp):
      return "decsp";
    case P_OP(incsp):
      return "incsp";

    case P_OP(push):
      return "push";
    case P_OP(pushf):
      return "pushf";
    case P_OP(pushd):
      return "pushd";
    case P_OP(pushx):
      return "pushx";
    case P_OP(pop):
      return "pop";
    case P_OP(popf):
      return "popf";
    case P_OP(popd):
      return "popd";
    case P_OP(popx):
      return "popx";

    case P_OP(ldw):
      return "ldw";
    case P_OP(ldh):
      return "ldh";
    case P_OP(lduw):
      return "lduw";
    case P_OP(ldub):
      return "ldub";
    case P_OP(lduh):
      return "lduh";
    case P_OP(lda):
      return "lda";
    case P_OP(ldf):
      return "ldf";
    case P_OP(ldd):
      return "ldd";
    case P_OP(ldb):
      return "ldb";
    case P_OP(ldx):
      return "ldx";

    case P_OP(stw):
      return "stw";
    case P_OP(sth):
      return "sth";
    case P_OP(stx):
      return "stx";
    case P_OP(stf):
      return "stf";
    case P_OP(std):
      return "std";
    case P_OP(stb):
      return "stb";

    // Add.
    case P_OP(add):
      return "add";
    case P_OP(addf):
      return "addf";
    case P_OP(addd):
      return "addd";
    case P_OP(addc):
      return "addc";

    // Subtract.
    case P_OP(sub):
      return "sub";
    case P_OP(subf):
      return "subf";
    case P_OP(subd):
      return "subd";

    // Multiply.
    case P_OP(mul):
      return "mul";
    case P_OP(mulf):
      return "mulf";
    case P_OP(muld):
      return "muld";

    // Divide.
    case P_OP(div):
      return "div";
    case P_OP(divu):
      return "divu";
    case P_OP(divf):
      return "divf";
    case P_OP(divd):
      return "divd";

    // Modulus.
    case P_OP(mod):
      return "mod";
    case P_OP(modu):
      return "modu";

    // Shifts.
    case P_OP(lsr):
      return "lsr";
    case P_OP(asr):
      return "asr";
    case P_OP(lsl):
      return "lsl";

    // Bitwise.
    case P_OP(or):
      return "or";
    case P_OP(and):
      return "and";
    case P_OP(xor):
      return "xor";

    case P_OP(not):
      return "not";
    case P_OP(inv):
      return "inv";
    case P_OP(neg):
      return "neg";
    case P_OP(negf):
      return "negf";
    case P_OP(negd):
      return "negd";

    // Compares.
    case P_OP(cmpeq):
      return "cmpeq";
    case P_OP(cmpne):
      return "cmpne";
    case P_OP(cmplt):
      return "cmplt";
    case P_OP(cmple):
      return "cmple";
    case P_OP(cmpgt):
      return "cmpgt";
    case P_OP(cmpge):
      return "cmpge";
    case P_OP(cmpltu):
      return "cmpltu";
    case P_OP(cmpleu):
      return "cmpleu";
    case P_OP(cmpgtu):
      return "cmpgtu";
    case P_OP(cmpgeu):
      return "cmpgeu";

    case P_OP(cmpeqf):
      return "cmpeqf";
    case P_OP(cmpnef):
      return "cmpnef";
    case P_OP(cmpltf):
      return "cmpltf";
    case P_OP(cmplef):
      return "cmplef";
    case P_OP(cmpgtf):
      return "cmpgtf";
    case P_OP(cmpgef):
      return "cmpgef";

    case P_OP(cmpeqd):
      return "cmpeqd";
    case P_OP(cmpned):
      return "cmpned";
    case P_OP(cmpltd):
      return "cmpltd";
    case P_OP(cmpled):
      return "cmpled";
    case P_OP(cmpgtd):
      return "cmpgtd";
    case P_OP(cmpged):
      return "cmpged";

    case P_OP(cmp3way):
      return "cmp3way";
    case P_OP(cmp3wayu):
      return "cmp3wayu";
    case P_OP(cmp3wayf):
      return "cmp3wayf";
    case P_OP(cmp3wayd):
      return "cmp3wayd";

    // Relative branches.
    case P_OP(bnz):
      return "bnz";
    case P_OP(bz):
      return "bz";
    case P_OP(bra):
      return "bra";
    case P_OP(cbra):
      return "cbra";  // Computed branch.

    case P_OP(i2f):
      return "i2f";
    case P_OP(i2d):
      return "i2d";
    case P_OP(ui2f):
      return "ui2f";
    case P_OP(ui2d):
      return "ui2d";
    case P_OP(f2d):
      return "f2d";
    case P_OP(d2f):
      return "d2f";
    case P_OP(f2i):
      return "f2i";
    case P_OP(d2i):
      return "d2i";
    case P_OP(f2ui):
      return "f2ui";
    case P_OP(d2ui):
      return "d2ui";

    // Absolute jump.
    case P_OP(jmp):
      return "jmp";
    case P_OP(cjmp):
      return "cjmp";

    case P_OP(adr):
      return "adr";
    case P_OP(adrs):
      return "adrs";
    case P_OP(adrtls):
      return "adrtls";

    // Call and return.
    case P_OP(call):
      return "call";
    case P_OP(callf):
      return "callf";
    case P_OP(calld):
      return "calld";
    case P_OP(rcall):
      return "rcall";
    case P_OP(rcallf):
      return "rcallf";
    case P_OP(rcalld):
      return "rcalld";
    case P_OP(ret):
      return "ret";
  }
}

static COMPILER_UNUSED TargetInstruction* ArgumentPointer(PCodeGenerator* pcode) {
  if (pcode->argument_pointer == NULL) {
    pcode->argument_pointer =
        TargetEmit(&pcode->base, TargetNewInstruction((TargetOpcode)P_OP(ap)));
  }
  return pcode->argument_pointer;
}

static bool PCodeIsBranch(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
  case P_OP(bra):
  case P_OP(bz):
  case P_OP(bnz):
  case P_OP(jmp):      // Jump to address.
  case P_OP(cjmp):     // Jump to contents of address.
    return true;
  default:
    return false;
  }
}

static bool PCodeIsCall(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
  case P_OP(call):
  case P_OP(callf):
  case P_OP(calld):
  case P_OP(rcall):
  case P_OP(rcallf):
  case P_OP(rcalld):
    return true;
  default:
    return false;
  }
}

static bool PCodeIsReturn(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)P_OP(ret);
}

static bool PCodeIsSpill(TargetInstruction* inst) {
  return false;
}

static bool PCodeIsLabel(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)P_OP(label);

}

int PCodeIntValue(TargetInstruction* inst) {
  return (int)((TargetConstant*)inst)->value.ivalue;
}

static bool PCodeIsFloatingPoint(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
  case  P_OP(constf):
  case  P_OP(constd):
  case P_OP(movf):
  case P_OP(movd):
//  case P_OP(rmovf):
//  case P_OP(rmovd):
  case P_OP(resultf):
  case P_OP(resultd):
  case P_OP(pushf):
  case P_OP(pushd):
  case P_OP(popf):
  case P_OP(popd):
  case P_OP(ldf):
  case P_OP(ldd):
  case P_OP(stf):
  case P_OP(std):
  case P_OP(addf):
  case P_OP(addd):
  case P_OP(subf):
  case P_OP(subd):
  case P_OP(mulf):
  case P_OP(muld):
  case P_OP(divf):
  case P_OP(divd):
  case P_OP(negf):
  case P_OP(negd):
    case P_OP(i2f):     // int to float.
    case  P_OP(i2d):     // int to double.
    case  P_OP(ui2f):     // unsigned int to float.
    case  P_OP(ui2d):     // unsigned int to double.
    case  P_OP(f2d):     // float to double.
    case  P_OP(d2f):      // double to float.
    case P_OP(callf):
    case P_OP(calld):
    case P_OP(rcallf):
    case P_OP(rcalld):
      return true;
  default:
    return false;
  }
}
static bool PCodeIsConditionalBranch(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
  case P_OP(bnz):
  case P_OP(bz):
    return true;
  default:
    return false;
  }
}

static bool PCodeIsFixedRegister(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
  case P_OP(call):
  case P_OP(callf):
  case P_OP(calld):
  case P_OP(rcall):
  case P_OP(rcallf):
  case P_OP(rcalld):
    return true;
  default:
    return false;
  }
}
static bool PCodeIsConst(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
    case P_OP(const8):
    case  P_OP(const16):
    case  P_OP(const32):
    case  P_OP(const64):
    case  P_OP(constf):
    case  P_OP(constd):

    return true;
  default:
    return false;
  }
}

static bool PCodeIsSymbol(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
  case P_OP(symbol):
    return true;
  default:
    return false;
  }
}

static bool PCodeIsJumpTableEntry(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)P_OP(bra);
}

static TargetInstruction* PCodeGetBranchTarget(TargetInstruction* inst) {
  return inst->operand[1];
}

static TargetVirtuals virtuals = {
  .opcode_name = PCodeOpcodeName,
  .is_branch = PCodeIsBranch,
  .is_call = PCodeIsCall,
  .is_return = PCodeIsReturn,
  .is_spill = PCodeIsSpill,
  .is_label = PCodeIsLabel,
  .is_floating_point = PCodeIsFloatingPoint,
  .is_conditional_branch = PCodeIsConditionalBranch,
  .is_fixed_register = PCodeIsFixedRegister,
  .is_const = PCodeIsConst,
  .is_symbol = PCodeIsSymbol,
  .is_expression = PCodeIsExpression,
  .is_table_entry = PCodeIsJumpTableEntry,
  .get_branch_target = PCodeGetBranchTarget,
};

void PCodeGeneratorInit(PCodeGenerator* pcode, Generator* gen) {
  TargetGeneratorInit(&pcode->base, gen, &virtuals);

  pcode->argument_pointer = NULL;
  pcode->source_pointer_size = gen->source_pointer_size;
  VectorInit(&pcode->exception_ranges);
  VectorInit(&pcode->exception_typeinfos);
  PCodeRegisterAllocatorInit(&pcode->register_allocator, pcode);
}

PCodeGenerator* NewPCodeGenerator(Generator* gen) {
  PCodeGenerator* pcode = malloc(sizeof(PCodeGenerator));
  PCodeGeneratorInit(pcode, gen);
  return pcode;
}

void PCodeGeneratorDestruct(PCodeGenerator* pcode) {
  TargetGeneratorDestruct(&pcode->base);
  VectorDestructWithContents(&pcode->exception_ranges, NULL,
                             /*free_element=*/true);
  VectorDestruct(&pcode->exception_typeinfos);
  PCodeRegisterAllocatorDestruct(&pcode->register_allocator);
}

static void ResolveExceptionRanges(PCodeGenerator* pcode, Generator* gen) {
  for (size_t i = 0; i < gen->exception_typeinfos.length; i++) {
    VectorAppend(&pcode->exception_typeinfos,
                 gen->exception_typeinfos.value.p[i]);
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
    catch_label->flags |=
        TARGET_INST_KEEP_UNREACHABLE | TARGET_INST_EXCEPTION_LANDING;
    TargetRecordExceptionEdge(&pcode->base, try_start, try_end, catch_label);
    PCodeExceptionRange* range = malloc(sizeof(PCodeExceptionRange));
    range->try_start = try_start;
    range->try_end = try_end;
    range->catch_label = catch_label;
    range->catch_typeinfo = ir_range->catch_typeinfo;
    range->is_cleanup = ir_range->is_cleanup;
    VectorAppend(&pcode->exception_ranges, range);
  }
  for (size_t i = 0; i < gen->exception_keep_labels.length; i++) {
    IRNode* label = gen->exception_keep_labels.value.p[i];
    TargetInstruction* target_label = label->data.ptr;
    if (target_label != NULL) {
      target_label->flags |= TARGET_INST_KEEP_UNREACHABLE;
    }
  }
}

void PCodeGeneratorDelete(PCodeGenerator* pcode) {
  PCodeGeneratorDestruct(pcode);
  free(pcode);
}

// Some static utility functions that map to generic target functions.
static TargetInstruction* NewInstruction1(PCodeOpcode opcode,
                                          TargetInstruction* op1) {
  return TargetNewInstruction1((TargetOpcode)opcode, op1);
}

static TargetInstruction* NewInstruction2(PCodeOpcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2) {
  return TargetNewInstruction2((TargetOpcode)opcode, op1, op2);
}

static COMPILER_UNUSED TargetInstruction* NewInstruction3(PCodeOpcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2,
                                          TargetInstruction* op3) {
  return TargetNewInstruction3((TargetOpcode)opcode, op1, op2, op3);
}

static TargetInstruction* Emit(PCodeGenerator* pcode, TargetInstruction* inst) {
  return TargetEmit(&pcode->base, inst);
}

static COMPILER_UNUSED TargetInstruction* EmitBefore(PCodeGenerator* pcode,
                                     TargetInstruction* inst,
                                     TargetInstruction* pos) {
  return TargetEmitBefore(&pcode->base, inst, pos);
}

static COMPILER_UNUSED TargetInstruction* EmitAfter(PCodeGenerator* pcode,
                                    TargetInstruction* inst,
                                    TargetInstruction* pos) {
  return TargetEmitAfter(&pcode->base, inst, pos);
}

static COMPILER_UNUSED TargetInstruction* EmitConstant(PCodeGenerator* pcode,
                                       TargetInstruction* c) {
  return TargetEmitConstant(&pcode->base, c);
}

static COMPILER_UNUSED TargetInstruction* EmitSymbol(PCodeGenerator* pcode,
                                     TargetInstruction* c) {
  return TargetEmitSymbol(&pcode->base, c);
}

static COMPILER_UNUSED TargetInstruction* FramePointer(PCodeGenerator* pcode) {
  return TargetFramePointer(&pcode->base);
}

static COMPILER_UNUSED TargetInstruction* StackPointer(PCodeGenerator* pcode) {
  return TargetStackPointer(&pcode->base);
}

static COMPILER_UNUSED TargetInstruction* ThreadPointer(PCodeGenerator* pcode) {
  return TargetThreadPointer(&pcode->base);
}

static TargetInstruction* GetLoweredNode(IRNode* node) {
  return TargetGetLoweredNode(node);
}

static TargetInstruction* SetLoweredNode(IRNode* node,
                                         TargetInstruction* inst) {
  return TargetSetLoweredNode(node, inst);
}

static TargetInstruction* GetIntConstant(PCodeGenerator* pcode, IRNode* node,
                                         TargetType type, int64_t value) {
  return TargetGetIntConstant(&pcode->base, node, type, value);
}

static COMPILER_UNUSED TargetInstruction* GetFloatingPointConstant(PCodeGenerator* pcode,
                                                   IRNode* node,
                                                   TargetType type,
                                                   double value) {
  return TargetGetFloatingPointConstant(&pcode->base, node, type, value);
}

static TargetInstruction* GetSymbol(PCodeGenerator* pcode, IRNode* node,
                                    Symbol* symbol) {
  return TargetGetSymbol(&pcode->base, node, symbol);
}

static TargetInstruction* NewInstruction(PCodeOpcode opcode) {
  return TargetNewInstruction((TargetOpcode)opcode);
}

static TargetInstruction* AddImmediate(PCodeGenerator* pcode, TargetInstruction* src, int64_t value) {
  return Emit(pcode, NewInstruction2(P_OP(addc), src,
                                      GetIntConstant(pcode, NULL, kTargetType32Bit,
                                                     value)));
}

// Static variables have an address calculated by the linker so at this
// point they are unknown.  We need to load their address into a register.  This
// is done using a movxc instruction.
static TargetInstruction* LoadStaticVariable(PCodeGenerator* pcode,
                                             IRNode* node) {
  if (!compiler->pic) {
    // Non-PIC, load address into reg.
    return Emit(pcode,NewInstruction1(P_OP(movxc),
                                      GetLoweredNode(node)));
  }
  // PIC, addr is address of GOT entry, load the address from it.
  TargetInstruction* addr =  Emit(pcode,
                                  NewInstruction1(P_OP(adr),
                                                  GetLoweredNode(node)));

  return Emit(pcode, NewInstruction2(P_OP(ldx), addr,
                                     GetIntConstant(pcode, NULL, kTargetType32Bit, 0)));
}

static bool PCodeFpIsDoubleWidth(TypeRecord* type) {
  return TypeUsesFloat64Representation(type);
}

static PCodeOpcode PCodeAddressLoadOpcode(PCodeGenerator* pcode) {
  return pcode->source_pointer_size <= 4 ? P_OP(lda) : P_OP(ldx);
}

static PCodeOpcode PCodeAddressStoreOpcode(PCodeGenerator* pcode) {
  return pcode->source_pointer_size <= 4 ? P_OP(stw) : P_OP(stx);
}

static struct {
  bool (*type_func)(TypeRecord*);
  PCodeOpcode load;
} load_opcodes[] = {
    {TypeIsInt, P_OP(ldw)},
    {TypeIsShort, P_OP(ldh)},
    {TypeIsChar8, P_OP(ldub)},
    {TypeIsChar, P_OP(ldb)},
    {TypeIsLongLong, P_OP(ldx)},
    {TypeIsUnsignedInt, P_OP(lduw)},
    {TypeIsUnsignedShort, P_OP(lduh)},
    {TypeIsUnsignedChar, P_OP(ldub)},
    {TypeUsesFloat32Representation, P_OP(ldf)},
    {PCodeFpIsDoubleWidth, P_OP(ldd)},
    {TypeIsBool, P_OP(ldb)},
    {TypeIsPointerOrArray, P_OP(ldx)},
    {TypeIsFunction, P_OP(ldx)},
    {NULL, 0},
};

static COMPILER_UNUSED TargetInstruction* LoadVariableValue(PCodeGenerator* pcode, IRNode* node,
                                            TargetInstruction* addr,
                                            TargetInstruction* offset) {
  PCodeOpcode opcode = P_OP(ldw);
  if (TypeIsLong(node->type)) {
    opcode = node->type->size > 4
                 ? P_OP(ldx)
                 : TypeIsUnsigned(node->type) ? P_OP(lduw) : P_OP(ldw);
  } else {
    for (size_t i = 0; load_opcodes[i].type_func != NULL; i++) {
      if (load_opcodes[i].type_func(node->type)) {
        opcode = load_opcodes[i].load;
        break;
      }
    }
  }
  assert(opcode != 0);
  return Emit(pcode, NewInstruction2(opcode, addr, offset));
}

// TODO: allow override of tls model per variable.
static COMPILER_UNUSED TargetInstruction* GetTlsVariableAddress(PCodeGenerator* pcode, IRNode* node) {
  switch (compiler->tls_model) {
    default:
      abort();
    case TLS(global_dynamic):
    case TLS(local_dynamic): {
      // These call __tls_get_addr to get the address of the a TLS variable
      // from the GOT.
      //
      // First load the address of the GOT entry, based on the symbol.
      TargetInstruction* addr =  Emit(pcode,
                                      NewInstruction1(P_OP(adr),
                                                      GetLoweredNode(node)));
      // Push onto stack.
      Emit(pcode, NewInstruction1(P_OP(push), addr));
      TargetInstruction* __tls_get_addr =
          GetSymbol(pcode, NULL, pcode->base.__tls_get_addr);
      TargetInstruction* result = Emit(pcode,
                                       NewInstruction1(P_OP(call), __tls_get_addr));
      Emit(pcode,
           NewInstruction1(P_OP(incsp),
                           GetIntConstant(pcode, NULL, kTargetType32Bit, 8)));
      return result;
    }
    case TLS(initial_exec): {
      // The address of the TLS variable is obtained from adding the offset
      // from the GOT to the thread pointer.
      TargetInstruction* addr =  Emit(pcode,
                                      NewInstruction1(P_OP(adrtls),
                                                      GetLoweredNode(node)));
      // Load the value as a 64-bit integer.
      TargetInstruction* zero = GetIntConstant(pcode, NULL, kTargetType64Bit, 0);
      TargetInstruction* value = Emit(pcode, NewInstruction2(P_OP(ldx), addr, zero));
      TargetInstruction* tp = ThreadPointer(pcode);
      return Emit(pcode, NewInstruction2(P_OP(addc), tp, value));
    }
    case TLS(local_exec): {
      // Address is constructed from thread pointer plus an offset
      // provided by the linker.
      TargetInstruction* addr = ThreadPointer(pcode);
      TargetInstruction* offset = Emit(pcode, NewInstruction1(P_OP(movxc),
                                                              GetLoweredNode(node)));
      return Emit(pcode, NewInstruction2(P_OP(add), addr, offset));
    }
  }
}


static COMPILER_UNUSED void GetTlsAddressAndOffset(PCodeGenerator* pcode, IRNode* addr_node,
                                                 TargetInstruction** addr,
                                                 TargetInstruction** offset) {
  switch (compiler->tls_model) {
    default:
      abort();
    case TLS(global_dynamic):
    case TLS(local_dynamic): {
      // These call __tls_get_addr to get the address of the a TLS variable
      // from the GOT.
      //
      // First load the address of the GOT entry, based on the symbol.
      TargetInstruction* tls_addr =  Emit(pcode,
                                      NewInstruction1(P_OP(adr),
                                                      GetLoweredNode(addr_node)));
      // Push onto stack.
      Emit(pcode, NewInstruction1(P_OP(push), tls_addr));
      TargetInstruction* __tls_get_addr = GetSymbol(pcode, NULL, pcode->base.__tls_get_addr);
      *addr = Emit(pcode, NewInstruction1(P_OP(call), __tls_get_addr));
      Emit(pcode,
                  NewInstruction1(P_OP(incsp),
                                  GetIntConstant(pcode, NULL, kTargetType32Bit, 8)));
      *offset = GetIntConstant(pcode, NULL, kTargetType64Bit, 0);
      break;
    }
    case TLS(initial_exec): {
      // The address of the TLS variable is obtained from adding the offset
      // from the GOT to the thread pointer.
      TargetInstruction* tls_addr =  Emit(pcode,
                                      NewInstruction1(P_OP(adrtls),
                                                      GetLoweredNode(addr_node)));
      // Load the value as a 64-bit integer.
      *offset = GetIntConstant(pcode, NULL, kTargetType64Bit, 0);
      tls_addr = Emit(pcode, NewInstruction2(P_OP(ldx), tls_addr, *offset));
      *addr = Emit(pcode, NewInstruction2(P_OP(add), ThreadPointer(pcode), tls_addr));
      break;
     }
    case TLS(local_exec): {
      // Address is thread pointer plus an offset obtained from the
      // linker.
      TargetInstruction* tp_offset =
          Emit(pcode, NewInstruction1(P_OP(movxc), GetLoweredNode(addr_node)));
      *addr = Emit(pcode, NewInstruction2(P_OP(add), ThreadPointer(pcode), tp_offset));
      *offset = GetIntConstant(pcode, NULL, kTargetType64Bit, 0);
      break;
    }
  }
}

// Materialize a value into a register.  This loads a constant into a register
// or returns the pcode instruction associated with the node if it's
// already in a register.
static TargetInstruction* Materialize(PCodeGenerator* pcode, IRNode* node) {
  if (IRIsConst(node)) {
    switch (node->opcode) {
      case IR_OP(const8):
      case IR_OP(const16):
      case IR_OP(const32):
        return Emit(pcode, NewInstruction1(P_OP(movc), GetLoweredNode(node)));
      case IR_OP(const64):
      case IR_OP(consta):
        return Emit(pcode, NewInstruction1(P_OP(movxc), GetLoweredNode(node)));
      case IR_OP(constf):
        return Emit(pcode, NewInstruction1(P_OP(movfc), GetLoweredNode(node)));
      case IR_OP(constd):
        return Emit(pcode, NewInstruction1(P_OP(movdc), GetLoweredNode(node)));
      default:
        assert(false);
    }
  }
  // Variables are materialized as their address.
  if ((node->flags & kIRNrvoMarker) != 0) {
    return Emit(
        pcode,
        NewInstruction2(P_OP(ldx), ArgumentPointer(pcode),
                        GetIntConstant(pcode, NULL, kTargetType32Bit, 16)));
  }
  if (IRIsAutoVariable(node)) {
    IRVariable* var = (IRVariable*)node;
    if (TypeIsVLA(node->type) && var->symbol != NULL &&
        var->symbol->value.other != NULL) {
      return GetLoweredNode(var->symbol->value.other);
    }
    // Auto variables are in the stack frame.  These are accessed through
    // the frame pointer with a negative offset.
    TargetInstruction* addr = FramePointer(pcode);
    int32_t var_offset = node->data.ivalue;
    TargetInstruction* offset = (TargetInstruction*)GetIntConstant(
        pcode, node, kTargetType32Bit,
        var_offset - pcode->base.stack_frame_size);
    return Emit(pcode, NewInstruction2(P_OP(addc), addr, offset));
  } else if (IRIsArgument(node)) {
    TargetInstruction* addr = ArgumentPointer(pcode);
    int32_t var_offset = node->data.ivalue;
    TargetInstruction* offset = (TargetInstruction*)GetIntConstant(
        pcode, node, kTargetType32Bit, var_offset);
    return Emit(pcode, NewInstruction2(P_OP(addc), addr, offset));
  } else if (IRIsThreadVariable(node)) {
    // Thread variables are relative to the thread pointer.  Access to
    // them depends on the TLS model being used.
    return GetTlsVariableAddress(pcode, node);
  } else if (IRIsStaticVariable(node)) {
    // The address of static variables need to be moved into a register.

    return LoadStaticVariable(pcode, node);
  }
  
  // Node is not a variable, is must be an already-lowered expression.
  return GetLoweredNode(node);
}

static TargetInstruction* LowerIRNode(PCodeGenerator* pcode, IRNode* node);

static TargetInstruction* GetDestInstruction(PCodeGenerator* pcode,
                                             IRNode* node) {
  if (node->dest == NULL) {
    return NULL;
  }
  LowerIRNode(pcode, node->dest);
  return GetLoweredNode(node->dest);
}

static TargetInstruction* ApplyDestInstruction(PCodeGenerator* pcode,
                                               IRNode* node,
                                               TargetInstruction* inst) {
  TargetInstruction* dest = GetDestInstruction(pcode, node);
  if (dest != NULL) {
    inst->dest = dest;
  }
  return inst;
}

static PCodeOpcode IR2PCode(IROpcode op, bool is_unsigned) {
  switch (op) {
    case IR_OP(addi):
      return P_OP(add);
    case IR_OP(addf):
      return P_OP(addf);
    case IR_OP(addd):
      return P_OP(addd);
    case IR_OP(adda):
      return P_OP(add);

    case IR_OP(subi):
      return P_OP(sub);
    case IR_OP(subf):
      return P_OP(subf);
    case IR_OP(subd):
      return P_OP(subd);
    case IR_OP(suba):
      return P_OP(sub);

    case IR_OP(muli):
      return P_OP(mul);
    case IR_OP(mulf):
      return P_OP(mulf);
    case IR_OP(muld):
      return P_OP(muld);

    case IR_OP(divi):
      return is_unsigned ? P_OP(divu) : P_OP(div);
    case IR_OP(divf):
      return P_OP(divf);
    case IR_OP(divd):
      return P_OP(divd);

    case IR_OP(modi):
      return is_unsigned ? P_OP(modu) : P_OP(mod);

    case IR_OP(lsri):
      return P_OP(lsr);
    case IR_OP(asri):
      return P_OP(asr);
    case IR_OP(lsli):
      return P_OP(lsl);

    case IR_OP(ori):
      return P_OP(or);
    case IR_OP(andi):
      return P_OP(and);
    case IR_OP(xori):
      return P_OP(xor);

    case IR_OP(noti):
      return P_OP(not);
    case IR_OP(nota):
      return P_OP(not);
    case IR_OP(onescomp):
      return P_OP(inv);
    case IR_OP(negi):
      return P_OP(neg);
    case IR_OP(negf):
      return P_OP(negf);
    case IR_OP(negd):
      return P_OP(negd);

    case IR_OP(cmpeqi):
      return P_OP(cmpeq);
    case IR_OP(cmpnei):
      return P_OP(cmpne);
    case IR_OP(cmplti):
      return is_unsigned ? P_OP(cmpltu) : P_OP(cmplt);
    case IR_OP(cmplei):
      return is_unsigned ? P_OP(cmpleu) : P_OP(cmple);
    case IR_OP(cmpgti):
      return is_unsigned ? P_OP(cmpgtu) : P_OP(cmpgt);
    case IR_OP(cmpgei):
      return is_unsigned ? P_OP(cmpgeu) : P_OP(cmpge);

    case IR_OP(cmpeqf):
      return P_OP(cmpeqf);
    case IR_OP(cmpnef):
      return P_OP(cmpnef);
    case IR_OP(cmpltf):
      return P_OP(cmpltf);
    case IR_OP(cmplef):
      return P_OP(cmplef);
    case IR_OP(cmpgtf):
      return P_OP(cmpgtf);
    case IR_OP(cmpgef):
      return P_OP(cmpgef);

    case IR_OP(cmpeqd):
      return P_OP(cmpeqd);
    case IR_OP(cmpned):
      return P_OP(cmpned);
    case IR_OP(cmpltd):
      return P_OP(cmpltd);
    case IR_OP(cmpled):
      return P_OP(cmpled);
    case IR_OP(cmpgtd):
      return P_OP(cmpgtd);
    case IR_OP(cmpged):
      return P_OP(cmpged);

    case IR_OP(cmpeqa):
      return P_OP(cmpeq);
    case IR_OP(cmpnea):
      return P_OP(cmpne);
    case IR_OP(cmplta):
      return P_OP(cmplt);
    case IR_OP(cmplea):
      return P_OP(cmple);
    case IR_OP(cmpgta):
      return P_OP(cmpgt);
    case IR_OP(cmpgea):
      return P_OP(cmpge);

    case IR_OP(cmp3wayi):
      return P_OP(cmp3way);
    case IR_OP(cmp3wayu):
      return P_OP(cmp3wayu);
    case IR_OP(cmp3waya):
      return P_OP(cmp3wayu);
    case IR_OP(cmp3wayf):
      return P_OP(cmp3wayf);
    case IR_OP(cmp3wayd):
      return P_OP(cmp3wayd);

    case IR_OP(i2f):
      return is_unsigned ? P_OP(ui2f) : P_OP(i2f);
    case IR_OP(i2d):
      return is_unsigned ? P_OP(ui2d) : P_OP(i2d);
    case IR_OP(f2d):
      return P_OP(f2d);
    case IR_OP(d2f):
      return P_OP(d2f);
    case IR_OP(f2i):
      return is_unsigned ? P_OP(f2ui) : P_OP(f2i);
    case IR_OP(d2i):
      return is_unsigned ? P_OP(d2ui) : P_OP(d2i);

    case IR_OP(movi):
      return P_OP(mov);
    case IR_OP(movf):
      return P_OP(movf);
    case IR_OP(movd):
      return P_OP(movd);
    case IR_OP(mova):
      return P_OP(mov);
//    case IR_OP(rmovi):
//      return P_OP(rmov);
//    case IR_OP(rmovf):
//      return P_OP(rmovf);
//    case IR_OP(rmovd):
//      return P_OP(rmovd);
//    case IR_OP(rmova):
//      return P_OP(rmov);
    case IR_OP(tmp):
      return P_OP(tmp);
    default:
      assert(false);
      return false;
  }
}

static void ApplyFixups(PCodeGenerator* pcode, IRNode* label_node) {
  TargetApplyFixups(&pcode->base, label_node);
}

// Do some strength reduction if we can.  Returns NULL or new instruciton
static TargetInstruction* ReduceExpressionStrength(PCodeGenerator* pcode,
                                        IRNode* node, PCodeOpcode opcode) {
  TargetInstruction* inst = NULL;
  switch (opcode) {
    default:
      // All others are handled below.
      break;
    case P_OP(add): {
      // We have an add with constant instruction.  Use it if we can.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      // Adds are commutative so we can have a const as first or
      // second operand.
      if (IRIsConst(op1) && IRIsConst(op2)) {
        // Both constants, fold.
        int64_t lhs = ((IRConstant*)op1)->value.ivalue;
        int64_t rhs = ((IRConstant*)op2)->value.ivalue;
        inst = (TargetInstruction*)NewInstruction(P_OP(movxc));
        inst->operand[0] =
        GetIntConstant(pcode, NULL, kTargetType32Bit, lhs + rhs);
        break;
      }
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 0) {
          // Adding zero is a move instruction.
          inst = (TargetInstruction*)NewInstruction(P_OP(mov));
          inst->operand[0] = Materialize(pcode, op1);
        } else {
          inst = (TargetInstruction*)NewInstruction(P_OP(addc));
          inst->operand[0] = Materialize(pcode, op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      } else if (IRIsConst(op1)) {
        int64_t c = ((IRConstant*)op1)->value.ivalue;
        if (c == 0) {
          // Adding zero is a move instruction.
          inst = (TargetInstruction*)NewInstruction(P_OP(mov));
          inst->operand[0] = Materialize(pcode, op1);
        } else {
          inst = (TargetInstruction*)NewInstruction(P_OP(addc));
          inst->operand[0] = Materialize(pcode, op2);
          inst->operand[1] = GetLoweredNode(op1);
        }
      }
      break;
    }
      
    case P_OP(sub): {
      // A sub with a constant can be converted to an addc with a negative
      // constant.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) && IRIsConst(op2)) {
        // Both constants, fold.
        int64_t lhs = ((IRConstant*)op1)->value.ivalue;
        int64_t rhs = ((IRConstant*)op2)->value.ivalue;
        inst = (TargetInstruction*)NewInstruction(P_OP(movxc));
        inst->operand[0] =
        GetIntConstant(pcode, NULL, kTargetType32Bit, lhs - rhs);
        break;
      }
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 0) {
          // Subtract zero is mov
          inst = (TargetInstruction*)NewInstruction(P_OP(mov));
          inst->operand[0] = Materialize(pcode, op1);
        } else {
          // Add the negative of the constant.
          inst = (TargetInstruction*)NewInstruction(P_OP(addc));
          inst->operand[0] = Materialize(pcode, op1);
          inst->operand[1] = GetIntConstant(pcode, NULL, kTargetType32Bit, -c);
        }
      }
      break;
    }
      
    case P_OP(mul): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) || IRIsConst(op2)) {
        if (IRIsConst(op1) && IRIsConst(op2)) {
          // Both constants, fold.
          int64_t lhs = ((IRConstant*)op1)->value.ivalue;
          int64_t rhs = ((IRConstant*)op2)->value.ivalue;
          inst = (TargetInstruction*)NewInstruction(P_OP(movxc));
          inst->operand[0] =
          GetIntConstant(pcode, NULL, kTargetType32Bit, lhs * rhs);
        } else {
          if (IRIsConst(op1)) {
            IRNode* tmp = op1;
            op1 = op2;
            op2 = tmp;
          }
          int64_t c = ((IRConstant*)op2)->value.ivalue;
          if (c == 0) {
            // Multiply by zero is zero.
            inst = GetIntConstant(pcode, NULL, kTargetType32Bit, 0);
          } else if (c == 1) {
            // Multiply by 1 is mov.
            inst = (TargetInstruction*)NewInstruction(P_OP(mov));
            inst->operand[0] = Materialize(pcode, op1);
          }
        }
      }
      break;
    }
    case P_OP(div): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) && IRIsConst(op2)) {
        // Both constants, fold.
        int64_t lhs = ((IRConstant*)op1)->value.ivalue;
        int64_t rhs = ((IRConstant*)op2)->value.ivalue;
        if (rhs != 0) {
          // Don't divide by zero.
          inst = (TargetInstruction*)NewInstruction(P_OP(movxc));
          inst->operand[0] =
          GetIntConstant(pcode, NULL, kTargetType32Bit, lhs / rhs);
          break;
        }
      }
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 1) {
          // Division by 1 is a mov
          inst = (TargetInstruction*)NewInstruction(P_OP(mov));
          inst->operand[0] = Materialize(pcode, op1);
        }
      }
      break;
    }
    case P_OP(divu): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) && IRIsConst(op2)) {
        // Both constants, fold.
        uint64_t lhs = ((IRConstant*)op1)->value.ivalue;
        uint64_t rhs = ((IRConstant*)op2)->value.ivalue;
        if (rhs != 0) {
          // Don't divide by zero.
          inst = (TargetInstruction*)NewInstruction(P_OP(movxc));
          inst->operand[0] =
          GetIntConstant(pcode, NULL, kTargetType32Bit, lhs / rhs);
          break;
        }
      }
      if (IRIsConst(op2)) {
        uint64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 1) {
          // Division by 1 is a mov
          inst = (TargetInstruction*)NewInstruction(P_OP(mov));
          inst->operand[0] = Materialize(pcode, op1);
        }
      }
      break;
    }
  }
  return inst;
}

static bool IntegerArithmeticNeedsNormalization(IROpcode opcode) {
  switch (opcode) {
    case IR_OP(addi):
    case IR_OP(subi):
    case IR_OP(muli):
    case IR_OP(divi):
    case IR_OP(modi):
    case IR_OP(lsri):
    case IR_OP(asri):
    case IR_OP(lsli):
    case IR_OP(ori):
    case IR_OP(andi):
    case IR_OP(xori):
    case IR_OP(noti):
    case IR_OP(onescomp):
    case IR_OP(negi):
      return true;
    default:
      return false;
  }
}

static TargetInstruction* NormalizeIntegerArithmeticResult(
    PCodeGenerator* pcode, IRNode* node, TargetInstruction* value) {
  int size = node->type == NULL ? 8 : node->type->size;
  if (size <= 0 || size >= 8) {
    return value;
  }

  int shift = 64 - size * 8;
  if (TypeIsUnsigned(node->type)) {
    uint64_t mask = (UINT64_C(1) << (size * 8)) - 1;
    TargetInstruction* mask_value = Emit(
        pcode, NewInstruction1(
                   P_OP(movxc),
                   GetIntConstant(pcode, NULL, kTargetType64Bit, mask)));
    return Emit(pcode, NewInstruction2(P_OP(and), value, mask_value));
  }

  TargetInstruction* shift_value =
      Emit(pcode,
           NewInstruction1(P_OP(movc),
                           GetIntConstant(pcode, NULL, kTargetType32Bit,
                                          shift)));
  TargetInstruction* shifted =
      Emit(pcode, NewInstruction2(P_OP(lsl), value, shift_value));
  return Emit(pcode, NewInstruction2(P_OP(asr), shifted, shift_value));
}

static TargetInstruction* LowerExpression(PCodeGenerator* pcode, IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  // For comparisons the result type is `bool` (which is unsigned), so the
  // signed/unsigned choice must come from the operand type instead, matching
  // how the other backends decide (e.g. x86_64's ComparisonIsUnsigned).
  bool is_unsigned = TypeIsUnsigned(node->type);
  if (IRIsComparison(node) && node->inputs.length > 0) {
    IRNode* operand = node->inputs.value.p[0];
    is_unsigned = operand != NULL && operand->type != NULL &&
                  TypeIsUnsigned(operand->type);
  }
  PCodeOpcode opcode = IR2PCode(node->opcode, is_unsigned);
  assert(node->inputs.length <= 2);
  TargetInstruction* inst = ReduceExpressionStrength(pcode, node, opcode);

  if (inst == NULL) {
    inst = (TargetInstruction*)NewInstruction(opcode);
    for (size_t i = 0; i < node->inputs.length; i++) {
      IRNode* input = node->inputs.value.p[i];
      inst->operand[i] = Materialize(pcode, input);
    }
  }
  TargetUpdateOperandUsers(inst);
  TargetInstruction* result = Emit(pcode, inst);
  if (IntegerArithmeticNeedsNormalization(node->opcode)) {
    result = NormalizeIntegerArithmeticResult(pcode, node, result);
  }
  ApplyDestInstruction(pcode, node, result);
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerConditionalBranch(PCodeGenerator* pcode,
                                                 IRNode* node) {
  PCodeOpcode opcode;
  switch (node->opcode) {
    case IR_OP(btrue):
      opcode = P_OP(bnz);
      break;
    case IR_OP(bfalse):
      opcode = P_OP(bz);
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }
  assert(node->inputs.length == 2);
  IRNode* expr = node->inputs.value.p[0];
  IRNode* target_node = node->inputs.value.p[1];

  TargetInstruction* inst = (TargetInstruction*)Emit(
      pcode, NewInstruction1(opcode, Materialize(pcode, expr)));

  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    // Forward branch, add fixup for target label.
    VectorAppend(&pcode->base.fixups, NewBranchFixup(inst, target_node, 1));
  } else {
    inst->operand[1] = target;
  }
  return inst;
}

static TargetInstruction* LowerBranch(PCodeGenerator* pcode, IRNode* node) {
  assert(node->inputs.length == 1);
  IRNode* target_node = node->inputs.value.p[0];

  TargetInstruction* inst =
      (TargetInstruction*)Emit(pcode, NewInstruction(P_OP(bra)));

  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    // Forward branch, add fixup for target label.
    VectorAppend(&pcode->base.fixups, NewBranchFixup(inst, target_node, 0));
  } else {
    inst->operand[0] = target;
  }
  return inst;
}

static TargetInstruction* LowerComputedBranch(PCodeGenerator* pcode,
                                              IRNode* node) {
  assert(node->inputs.length == 1);
  TargetInstruction* value = GetLoweredNode(node->inputs.value.p[0]);
  TargetInstruction* cbra = Emit(pcode, NewInstruction1(P_OP(cbra), value));
  SetLoweredNode(node, cbra);
  return cbra;
}

static TargetInstruction* LowerLabel(PCodeGenerator* pcode, IRNode* label) {
  TargetInstruction* inst = Emit(pcode, NewInstruction(P_OP(label)));
  label->data.ptr = inst;
  ApplyFixups(pcode, label);
  return inst;
}

static TargetInstruction* LowerNamedLabel(PCodeGenerator* rv, IRNode* label) {
  IRNamedLabel* n = (IRNamedLabel*)label;
  TargetInstruction* inst =  Emit(rv, TargetNewNamedLabel(n->name));
  label->data.ptr = inst;
  return inst;
}

static void GetAddressAndOffset(PCodeGenerator* pcode, IRNode* addr_node,
                            TargetInstruction** addr,
                            TargetInstruction** offset) {
  if ((addr_node->flags & kIRNrvoMarker) != 0) {
    *addr = Emit(pcode, NewInstruction2(
                            P_OP(ldx), ArgumentPointer(pcode),
                            GetIntConstant(pcode, NULL, kTargetType32Bit, 16)));
    *offset = GetIntConstant(pcode, NULL, kTargetType32Bit, 0);
    return;
  }
  if (IRIsAutoVariable(addr_node)) {
    // Auto variables are in the stack frame.  These are accessed through
    // the frame pointer with a negative offset.
    *addr = FramePointer(pcode);
    int32_t var_offset = addr_node->data.ivalue;
    *offset = (TargetInstruction*)GetIntConstant(
        pcode, addr_node, kTargetType32Bit,
        var_offset - pcode->base.stack_frame_size);
  } else if (IRIsArgument(addr_node)) {
    *addr = ArgumentPointer(pcode);
    int32_t var_offset = addr_node->data.ivalue;
    *offset = (TargetInstruction*)GetIntConstant(pcode, addr_node,
                                                 kTargetType32Bit, var_offset);
  } else if (IRIsThreadVariable(addr_node)) {
    GetTlsAddressAndOffset(pcode, addr_node, addr, offset);
  } else if (IRIsStaticVariable(addr_node)) {
    // The address of static variables need to be moved into a register.

    *addr = LoadStaticVariable(pcode, addr_node);
    *offset = GetIntConstant(pcode, NULL, kTargetType32Bit, 0);
  } else if (addr_node->opcode == IR_OP(structreturn)) {
    // Struct return value is an invisible first argument at ap + 16.
    *addr = Emit(pcode, NewInstruction2(
                            P_OP(ldx), ArgumentPointer(pcode),
                            GetIntConstant(pcode, NULL, kTargetType32Bit, 16)));
    *offset = GetIntConstant(pcode, NULL, kTargetType32Bit, 0);
  } else {
    // All others have a calculated address.
    *addr = GetLoweredNode(addr_node);
    *offset = GetIntConstant(pcode, NULL, kTargetType32Bit, 0);
    assert(addr != NULL);
  }
}

static TargetInstruction* Load(PCodeGenerator* pcode, IRNode* addr_node, PCodeOpcode opcode) {
  TargetInstruction* addr;
  TargetInstruction* offset;
  GetAddressAndOffset(pcode, addr_node, &addr, &offset);
  TargetInstruction* result =
      Emit(pcode, NewInstruction2(opcode, addr, offset));
  return result;
}

static TargetInstruction* LowerLoad(PCodeGenerator* pcode, IRNode* node) {
  PCodeOpcode opcode;
  assert(node->inputs.length == 1);
  IRNode* addr_node = node->inputs.value.p[0];

  switch (node->opcode) {
    case IR_OP(load32):
      opcode = P_OP(ldw);
      break;
    case IR_OP(load8):
      opcode = P_OP(ldb);
      break;
    case IR_OP(load64):
      opcode = P_OP(ldx);
      break;
    case IR_OP(load16):
      opcode = P_OP(ldh);
      break;
    case IR_OP(loadu32):
      opcode = P_OP(lduw);
      break;
    case IR_OP(loadu8):
      opcode = P_OP(ldub);
      break;
    case IR_OP(loadu16):
      opcode = P_OP(lduh);
      break;
    case IR_OP(loadf):
      opcode = P_OP(ldf);
      break;
    case IR_OP(loadd):
      opcode = P_OP(ldd);
      break;
    case IR_OP(loada):
      opcode = PCodeAddressLoadOpcode(pcode);
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }

  TargetInstruction* load = Load(pcode, addr_node, opcode);
  ApplyDestInstruction(pcode, node, load);
  return SetLoweredNode(node, load);
}

static TargetInstruction* Store(PCodeGenerator* pcode, IRNode* addr_node, TargetInstruction* src, PCodeOpcode opcode) {
  TargetInstruction* addr;
  TargetInstruction* offset;
  GetAddressAndOffset(pcode, addr_node, &addr, &offset);

  // NOTE: the first operand of the st instructions is the source register.
  TargetInstruction* result =
      Emit(pcode, NewInstruction3(opcode, src, addr, offset));
  return result;
}

static TargetInstruction* LowerStore(PCodeGenerator* pcode, IRNode* node) {
  PCodeOpcode opcode;
  assert(node->inputs.length == 2);

  // Address to store to is the first operand of the store IR node.
  // The value to store is the second operand.
  IRNode* addr_node = node->inputs.value.p[0];
  IRNode* src_node = node->inputs.value.p[1];

  // Get the src in a register.
  TargetInstruction* src = Materialize(pcode, src_node);

  switch (node->opcode) {
    case IR_OP(store32):
      opcode = P_OP(stw);
      break;
    case IR_OP(store8):
      opcode = P_OP(stb);
      break;
    case IR_OP(store64):
      opcode = P_OP(stx);
      break;
    case IR_OP(store16):
      opcode = P_OP(sth);
      break;
    case IR_OP(storef):
      opcode = P_OP(stf);
      break;
    case IR_OP(stored):
      opcode = P_OP(std);
      break;
    case IR_OP(storea):
      opcode = PCodeAddressStoreOpcode(pcode);
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }

  return SetLoweredNode(node, Store(pcode, addr_node, src, opcode));
}

static PCodeOpcode AtomicLoadOpcode(PCodeGenerator* pcode, TypeRecord* type) {
  if (TypeIsCharFamily(type)) {
    return TypeIsUnsigned(type) ? P_OP(ldub) : P_OP(ldb);
  }
  if (TypeIsShort(type)) {
    return TypeIsUnsigned(type) ? P_OP(lduh) : P_OP(ldh);
  }
  if (TypeIsPointerOrArray(type)) {
    return PCodeAddressLoadOpcode(pcode);
  }
  if (TypeIsLongLong(type) || (TypeIsLong(type) && type->size > 4)) {
    return P_OP(ldx);
  }
  if (TypeUsesFloat32Representation(type)) {
    return P_OP(ldf);
  }
  if (PCodeFpIsDoubleWidth(type)) {
    return P_OP(ldd);
  }
  return TypeIsUnsigned(type) ? P_OP(lduw) : P_OP(ldw);
}

static PCodeOpcode AtomicStoreOpcode(PCodeGenerator* pcode, TypeRecord* type) {
  if (TypeIsCharFamily(type)) {
    return P_OP(stb);
  }
  if (TypeIsShort(type)) {
    return P_OP(sth);
  }
  if (TypeIsPointerOrArray(type)) {
    return PCodeAddressStoreOpcode(pcode);
  }
  if (TypeIsLongLong(type) || (TypeIsLong(type) && type->size > 4)) {
    return P_OP(stx);
  }
  if (TypeUsesFloat32Representation(type)) {
    return P_OP(stf);
  }
  if (PCodeFpIsDoubleWidth(type)) {
    return P_OP(std);
  }
  return P_OP(stw);
}

static TargetInstruction* LowerAtomicLoad(PCodeGenerator* pcode, IRNode* node) {
  IRNode* addr_node = node->inputs.value.p[0];
  TargetInstruction* load =
      Load(pcode, addr_node, AtomicLoadOpcode(pcode, node->type));
  ApplyDestInstruction(pcode, node, load);
  return SetLoweredNode(node, load);
}

static TargetInstruction* LowerAtomicStore(PCodeGenerator* pcode, IRNode* node) {
  IRNode* addr_node = node->inputs.value.p[0];
  IRNode* src_node = node->inputs.value.p[1];
  TargetInstruction* src = Materialize(pcode, src_node);
  return SetLoweredNode(
      node,
      Store(pcode, addr_node, src, AtomicStoreOpcode(pcode, src_node->type)));
}

static TargetInstruction* LowerAtomicFetchAddSub(PCodeGenerator* pcode,
                                                 IRNode* node, bool add,
                                                 bool return_new) {
  IRNode* addr_node = node->inputs.value.p[0];
  IRNode* value_node = node->inputs.value.p[1];
  TargetInstruction* old_value =
      Load(pcode, addr_node, AtomicLoadOpcode(pcode, node->type));
  TargetInstruction* value = Materialize(pcode, value_node);
  TargetInstruction* new_value =
      Emit(pcode, NewInstruction2(add ? P_OP(add) : P_OP(sub), old_value, value));
  Store(pcode, addr_node, new_value, AtomicStoreOpcode(pcode, node->type));
  TargetInstruction* result = return_new ? new_value : old_value;
  ApplyDestInstruction(pcode, node, result);
  return SetLoweredNode(node, result);
}

static TargetInstruction* LowerAtomicCompareExchange(PCodeGenerator* pcode,
                                                     IRNode* node,
                                                     bool expected_is_pointer,
                                                     bool returns_bool) {
  IRNode* addr_node = node->inputs.value.p[0];
  TypeRecord* value_type = addr_node->type->next;
  PCodeOpcode load_opcode = AtomicLoadOpcode(pcode, value_type);
  PCodeOpcode store_opcode = AtomicStoreOpcode(pcode, value_type);
  TargetInstruction* old_value = Load(pcode, addr_node, load_opcode);

  TargetInstruction* expected_ptr = NULL;
  TargetInstruction* expected;
  if (expected_is_pointer) {
    expected_ptr = Materialize(pcode, node->inputs.value.p[1]);
    expected = Load(pcode, node->inputs.value.p[1], load_opcode);
  } else {
    expected = Materialize(pcode, node->inputs.value.p[1]);
  }
  TargetInstruction* desired = Materialize(pcode, node->inputs.value.p[2]);
  TargetInstruction* matches =
      Emit(pcode, NewInstruction2(P_OP(cmpeq), old_value, expected));

  TargetInstruction* failure = NewInstruction(P_OP(label));
  TargetInstruction* done = NewInstruction(P_OP(label));
  TargetInstruction* branch_failure =
      Emit(pcode, NewInstruction1(P_OP(bz), matches));
  branch_failure->operand[1] = failure;

  Store(pcode, addr_node, desired, store_opcode);
  Emit(pcode, NewInstruction1(P_OP(bra), done));

  Emit(pcode, failure);
  if (expected_is_pointer) {
    // The expected argument is an address value, so store directly through
    // that register rather than asking Store() to reinterpret the IR node.
    Emit(pcode, NewInstruction3(store_opcode, old_value, expected_ptr,
                                GetIntConstant(pcode, NULL,
                                               kTargetType32Bit, 0)));
  }
  Emit(pcode, done);

  TargetInstruction* result = returns_bool ? matches : old_value;
  ApplyDestInstruction(pcode, node, result);
  return SetLoweredNode(node, result);
}

static struct {
  bool (*type_func)(TypeRecord*);
  PCodeOpcode opcode;
  size_t size;
} push_map[] = {
    {TypeIsInt, P_OP(push), 4},
    {TypeIsBool, P_OP(push), 4},
    {TypeIsShort, P_OP(push), 4},
    {TypeIsCharFamily, P_OP(push), 4},
    {TypeIsLongLong, P_OP(pushx), 8},
    {TypeUsesFloat32Representation, P_OP(pushf), 4},
    {PCodeFpIsDoubleWidth, P_OP(pushd), 8},
    {TypeIsPointerOrArray, P_OP(pushx), 8},
    {TypeIsNullPointer, P_OP(pushx), 8},
    {TypeIsFunction, P_OP(pushx), 8},
    {TypeIsReference, P_OP(pushx), 8},
    {TypeIsStructOrUnion, P_OP(pushx), 8},
    {TypeIsMemberPointer, P_OP(pushx), 8},
    {NULL, P_OP(push), 0},
};

static void PushArg(PCodeGenerator* pcode, IRNode* node,
                                  TargetInstruction* inst, size_t* size) {
  if (node->type == NULL) {
    Emit(pcode, NewInstruction1(P_OP(push), inst));
  }
  if (TypeIsBitInt(node->type)) {
    bool wide = node->type->size > 4;
    if (size != NULL) {
      *size += wide ? 8 : 4;
    }
    Emit(pcode, NewInstruction1(wide ? P_OP(pushx) : P_OP(push), inst));
    return;
  }
  if (TypeIsLong(node->type)) {
    bool wide = node->type->size > 4;
    if (size != NULL) {
      *size += wide ? 8 : 4;
    }
    Emit(pcode, NewInstruction1(wide ? P_OP(pushx) : P_OP(push), inst));
    return;
  }
  for (size_t i = 0; push_map[i].type_func != NULL; i++) {
    if (push_map[i].type_func(node->type)) {
      if (size != NULL) {
        *size += push_map[i].size;
      }
      Emit(pcode, NewInstruction1(push_map[i].opcode, inst));
      return;
    }
  }
  assert(false);
}

// Passing a struct or union to a function needs to copy
// the memory from the address to the stack.
static COMPILER_UNUSED void PushStructArg(PCodeGenerator* pcode, IRNode* node, size_t *args_size) {
  size_t struct_size = node->type->size;
  *args_size += struct_size;
  
  // First make space on the stack.
  TargetInstruction* size =
      GetIntConstant(pcode, NULL, kTargetType32Bit, struct_size);
  Emit(pcode, NewInstruction1(P_OP(decsp), size));
  
  TargetInstruction* dest_addr =
      Emit(pcode, NewInstruction1(P_OP(mov), StackPointer(pcode)));
  
  // Push size for memcpy.
  TargetInstruction* size_mov =
  Emit(pcode, NewInstruction1(P_OP(movc), size));
  Emit(pcode, NewInstruction1(P_OP(pushx), size_mov));
  
  // Push source address.
  TargetInstruction* src_addr;
  TargetInstruction* src_offset;
  GetAddressAndOffset(pcode, node, &src_addr, &src_offset);
  
  // We have an address and register for the address, add them together.
  src_addr = Emit(pcode, NewInstruction2(P_OP(addc), src_addr, src_offset));
  Emit(pcode, NewInstruction1(P_OP(pushx), src_addr));
  
  // Push dest address.
  Emit(pcode, NewInstruction1(P_OP(pushx), dest_addr));
  
  // Call memcpy.
  TargetInstruction* memcpy = GetSymbol(pcode, NULL, pcode->base.memcpy);
  Emit(pcode, NewInstruction1(P_OP(call), memcpy));
  Emit(pcode,
       NewInstruction1(P_OP(incsp),
                       GetIntConstant(pcode, NULL, kTargetType32Bit, 24)));

}

static TargetInstruction* LowerCall(PCodeGenerator* pcode, IRNode* node) {
  assert(node->inputs.length >= 1);
  size_t args_size = 0;
  for (size_t i = node->inputs.length - 1; i >= 1; i--) {
    IRNode* arg_node = node->inputs.value.p[i];
    if (TypeIsStructOrUnion(arg_node->type) ||
        TypeUsesLongDoubleRepresentation(arg_node->type) ||
        TypeIsMemberPointerAggregate(arg_node->type)) {
      PushStructArg(pcode, arg_node, &args_size);
    } else {
      TargetInstruction* arg = Materialize(pcode, arg_node);
      PushArg(pcode, arg_node, arg, &args_size);
    }
  }
  TargetInstruction* addr = Materialize(pcode, node->inputs.value.p[0]);
  PCodeOpcode opcode;
  if (((int)addr->opcode == (int)P_OP(symbol))) {
    if (TypeUsesFloat32Representation(node->type)) {
      opcode = P_OP(callf);
    } else if (PCodeFpIsDoubleWidth(node->type)) {
      opcode = P_OP(calld);
    } else {
      opcode = P_OP(call);
    }
  } else {
    if (TypeUsesFloat32Representation(node->type)) {
      opcode = P_OP(rcallf);
    } else if (PCodeFpIsDoubleWidth(node->type)) {
      opcode = P_OP(rcalld);
    } else {
      opcode = P_OP(rcall);
    }
  }
  TargetInstruction* call = Emit(pcode, NewInstruction1(opcode, addr));
  if (args_size > 0) {
    Emit(pcode,
       NewInstruction1(P_OP(incsp), GetIntConstant(pcode, NULL, kTargetType32Bit,
                                                  args_size)));
  }
  if (node->type != NULL && !TypeIsVoid(node->type)) {
    PCodeOpcode move_opcode = P_OP(mov);
    if (TypeUsesFloat32Representation(node->type)) {
      move_opcode = P_OP(movf);
    } else if (PCodeFpIsDoubleWidth(node->type)) {
      move_opcode = P_OP(movd);
    }
    TargetInstruction* dest = GetDestInstruction(pcode, node);
    if (dest != NULL) {
      TargetInstruction* result = Emit(pcode, NewInstruction1(move_opcode, call));
      result->dest = dest;
      SetLoweredNode(node, result);
      return result;
    }
    TargetInstruction* result = Emit(pcode, NewInstruction1(move_opcode, call));
    SetLoweredNode(node, result);
    return result;
  }
  SetLoweredNode(node, call);
  return call;
}

static TargetInstruction* LowerResult(PCodeGenerator* pcode, IRNode* node) {
  assert(node->inputs.length == 1);
  PCodeOpcode result_reg_opcode, opcode;
  switch (node->opcode) {
    case IR_OP(resulti):
    case IR_OP(resulta):
      result_reg_opcode = P_OP(resulti);
      opcode = P_OP(mov);
      break;
    case IR_OP(resultf):
      result_reg_opcode = P_OP(resultf);
      opcode = P_OP(movf);
      break;
    case IR_OP(resultd):
      result_reg_opcode = P_OP(resultd);
      opcode = P_OP(movd);
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }
  TargetInstruction* result = Materialize(pcode, node->inputs.value.p[0]);
  TargetInstruction* result_reg =
      Emit(pcode, NewInstruction(result_reg_opcode));
  TargetInstruction* mov = Emit(pcode, NewInstruction1(opcode, result));
  mov->dest = result_reg;
  return mov;
}

// A literal reference is a move of the literal offset (the first input
// to the literalref node) to the 'literal' with the given id.  This will
// be assembled as a reference to a symbol with the name .str.%d.
static TargetInstruction* LowerLiteralReference(PCodeGenerator* pcode,
                                                IRNode* node) {
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(pcode, TargetNewLiteral((int)id_node->value.ivalue));

  TargetInstruction* result =
      Emit(pcode, NewInstruction1(P_OP(adrs), literal));

  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerAddressOf(PCodeGenerator* pcode, IRNode* node) {
  TargetInstruction* address = Materialize(pcode, node->inputs.value.p[0]);
  TargetInstruction* dest = GetDestInstruction(pcode, node);
  if (dest != NULL) {
    TargetInstruction* result = NewInstruction1(P_OP(mov), address);
    result->dest = dest;
    TargetUpdateOperandUsers(result);
    address = Emit(pcode, result);
  }
  return SetLoweredNode(node, address);
}

static TargetInstruction* LowerCast(PCodeGenerator* pcode, IRNode* node) {
  TargetInstruction* value = Materialize(pcode, node->inputs.value.p[0]);
  TargetInstruction* dest = GetDestInstruction(pcode, node);
  if (dest == NULL) {
    return SetLoweredNode(node, value);
  }

  PCodeOpcode opcode = P_OP(mov);
  if (TypeUsesFloat32Representation(node->type)) {
    opcode = P_OP(movf);
  } else if (PCodeFpIsDoubleWidth(node->type)) {
    opcode = P_OP(movd);
  }
  TargetInstruction* result = NewInstruction1(opcode, value);
  result->dest = dest;
  return SetLoweredNode(node, Emit(pcode, result));
}

static TargetInstruction* LowerMemcpy(PCodeGenerator* pcode, IRNode* node) {
  // The memcpy IR node's inputs are the same as those for the memcpy
  // function.  However, there are no load nodes for the desination
  // or source addresses.
  assert(node->inputs.length == 3);
  IRNode* length_node = node->inputs.value.p[2];
  if (IRIsIntConst(length_node)) {
    IRNode* src_node = node->inputs.value.p[1];
    TargetInstruction* src_addr;
    TargetInstruction* src_offset;
    GetAddressAndOffset(pcode, src_node, &src_addr, &src_offset);
    src_addr = Emit(pcode, NewInstruction2(P_OP(addc), src_addr, src_offset));

    IRNode* dest_node = node->inputs.value.p[0];
    TargetInstruction* dest_addr;
    TargetInstruction* dest_offset;
    GetAddressAndOffset(pcode, dest_node, &dest_addr, &dest_offset);
    dest_addr = Emit(pcode, NewInstruction2(P_OP(addc), dest_addr, dest_offset));

    TargetInstruction* last = dest_addr;
    int64_t length_value = IRIntConstValue(length_node);
    for (int64_t i = 0; i < length_value; i++) {
      TargetInstruction* offset =
          GetIntConstant(pcode, NULL, kTargetType32Bit, i);
      TargetInstruction* byte =
          Emit(pcode, NewInstruction2(P_OP(ldub), src_addr, offset));
      last = Emit(pcode, NewInstruction3(P_OP(stb), byte, dest_addr, offset));
    }
    SetLoweredNode(node, last);
    return last;
  }

  // First push the constant for the length.
  TargetInstruction* length =
      Emit(pcode, Materialize(pcode, length_node));
  Emit(pcode, NewInstruction1(P_OP(pushx), length));

  // Now push src.
  IRNode* src_node = node->inputs.value.p[1];
  TargetInstruction* src_addr;
  TargetInstruction* src_offset;
  GetAddressAndOffset(pcode, src_node, &src_addr, &src_offset);

  // We have an address and register for the address, add them together.
  src_addr = Emit(pcode, NewInstruction2(P_OP(addc), src_addr, src_offset));
  src_node->data.ptr = src_addr;
  PushArg(pcode, src_node, src_addr, NULL);

  // And now push dest.
  IRNode* dest_node = node->inputs.value.p[0];
  TargetInstruction* dest_addr;
  TargetInstruction* dest_offset;
  GetAddressAndOffset(pcode, dest_node, &dest_addr, &dest_offset);

  // We have an address and register for the address, add them together.
  dest_addr = Emit(pcode, NewInstruction2(P_OP(addc), dest_addr, dest_offset));
  dest_node->data.ptr = dest_addr;

  PushArg(pcode, dest_node, dest_addr, NULL);

  TargetInstruction* memcpy = GetSymbol(pcode, NULL, pcode->base.memcpy);
  TargetInstruction* call = Emit(pcode, NewInstruction1(P_OP(call), memcpy));
  Emit(pcode,
       NewInstruction1(P_OP(incsp),
                       GetIntConstant(pcode, NULL, kTargetType32Bit, 24)));
  SetLoweredNode(node, call);

  return call;
}

static TargetInstruction* LowerMemzero(PCodeGenerator* pcode, IRNode* node) {
  // The memzero IR node has one input: the variable to zero.  Keep this
  // self-contained for pcode execution by emitting direct byte stores instead
  // of relying on an external memset implementation.
  assert(node->inputs.length == 1);
  IRNode* addr_node = node->inputs.value.p[0];

  TargetInstruction* addr;
  TargetInstruction* offset;
  GetAddressAndOffset(pcode, addr_node, &addr, &offset);

  // We have an address and register for the address, add them together
  // to produce the address to zero.
  addr = Emit(pcode, NewInstruction2(P_OP(addc), addr, offset));
  TargetInstruction* zero = Emit(
      pcode, NewInstruction1(P_OP(movc),
                             GetIntConstant(pcode, NULL, kTargetType32Bit, 0)));
  TargetInstruction* last = zero;
  // Prefer the backing symbol's size, but fall back to the memzero node's type
  // when the destination is a symbol-less slot (e.g. an sret return location).
  Symbol* symbol = IRGetVariableSymbol(addr_node);
  int64_t size = (symbol != NULL && symbol->type != NULL)
                     ? symbol->type->size
                     : (node->type != NULL ? node->type->size : 0);
  for (int64_t i = 0; i < size; i++) {
    last = Emit(pcode, NewInstruction3(P_OP(stb), zero, addr,
                                       GetIntConstant(pcode, NULL,
                                                      kTargetType32Bit, i)));
  }
  SetLoweredNode(node, last);
  return last;
}

static TargetInstruction* LowerZeroExtend(PCodeGenerator* pcode, IRNode* node) {
  IRNode* src = node->inputs.value.p[0];
  int src_size = src->type != NULL ? src->type->size : node->type->size;
  int keep_bytes = src_size < node->type->size ? src_size : node->type->size;
  TargetInstruction* value = Materialize(pcode, node->inputs.value.p[0]);
  if (keep_bytes < 8) {
    uint64_t mask = (UINT64_C(1) << (keep_bytes * 8)) - 1;
    value = Emit(pcode,
                 NewInstruction2(
                     P_OP(and), value,
                     Emit(pcode, NewInstruction1(
                                     P_OP(movxc),
                                     GetIntConstant(pcode, NULL,
                                                    kTargetType64Bit, mask)))));
  }
  SetLoweredNode(node, value);
  return value;
}

static TargetInstruction* LowerSignExtend(PCodeGenerator* pcode, IRNode* node) {
  // If the source instruction is a signed load then there is no need to
  // perform the sign extension since the load instructions already do
  // that.
  TargetInstruction* value = Materialize(pcode, node->inputs.value.p[0]);
  if (PCodeIsSignedLoad(value)) {
    return SetLoweredNode(node, value);
  }
  IRConstant* diff_value = node->inputs.value.p[1];
  int64_t diff = diff_value->value.ivalue;
  TargetInstruction* diff_inst =
      Emit(pcode,
           NewInstruction1(P_OP(movc),
                           GetIntConstant(pcode, NULL, kTargetType32Bit, diff)));
  TargetInstruction* lsl =
      Emit(pcode, NewInstruction2(P_OP(lsl), value, diff_inst));
  TargetInstruction* asr =
      Emit(pcode, NewInstruction2(P_OP(asr), lsl, diff_inst));

  SetLoweredNode(node, asr);
  return asr;
}

static TargetInstruction* LowerAlign(PCodeGenerator* pcode, IRNode* node) {
  TargetInstruction* value = Materialize(pcode, node->inputs.value.p[0]);
  IRConstant* align = node->inputs.value.p[1];

  TargetInstruction* immed = GetIntConstant(pcode, NULL, kTargetType32Bit, align->value.ivalue - 1);
  TargetInstruction* inv_immed = GetIntConstant(pcode, NULL, kTargetType32Bit, ~(align->value.ivalue - 1));
  TargetInstruction* add =
      Emit(pcode, NewInstruction2(P_OP(addc), value, immed));
  TargetInstruction* mask =
      Emit(pcode, NewInstruction1(P_OP(movxc), inv_immed));
  TargetInstruction* and =
      Emit(pcode, NewInstruction2(P_OP(and), add, mask));

  SetLoweredNode(node, and);
  return and;
}

static TargetInstruction* LowerGetBitField(PCodeGenerator* pcode, IRNode* node) {
  TargetInstruction* value = Materialize(pcode, node->inputs.value.p[0]);
  int bit_pos = (int)IRIntConstValue(node->inputs.value.p[1]);
  int bit_size = (int)IRIntConstValue(node->inputs.value.p[2]);
  if (TypeIsUnsigned(node->type)) {
    // Shift right by bit_pos
    // Mask with bit_size
    TargetInstruction* lsr = Emit(pcode, NewInstruction2(P_OP(asr), value, GetIntConstant(pcode, NULL, kTargetType32Bit, bit_pos)));
    uint64_t mask = bit_size == 64 ? -1LL : (1 << bit_size) - 1;
    TargetInstruction* m = Emit(pcode, NewInstruction2(P_OP(and), lsr, GetIntConstant(pcode, NULL, kTargetType32Bit, mask)));
    SetLoweredNode(node, m);
    return m;
  }
  // Shift left by 64 - (bit_pos + bit_size).  Top bit in bit 63.
  // Shift right by 64 - bit_size.
  TargetInstruction* lsl = Emit(pcode, NewInstruction2(P_OP(lsl), value, GetIntConstant(pcode, NULL, kTargetType32Bit, 64 - (bit_pos + bit_size))));
  TargetInstruction* asr = Emit(pcode, NewInstruction2(P_OP(asr), lsl, GetIntConstant(pcode, NULL, kTargetType32Bit, 64 - bit_size)));

  SetLoweredNode(node, asr);
  return asr;
}

static TargetInstruction* LowerSetBitField(PCodeGenerator* pcode, IRNode* node) {
  IRNode* output_node = node->inputs.value.p[0];
  IRNode* input_node = node->inputs.value.p[1];
  int bit_pos = (int)IRIntConstValue(node->inputs.value.p[2]);
  int bit_size = (int)IRIntConstValue(node->inputs.value.p[3]);
  uint64_t mask = bit_size == 64 ? -1LL : (1 << bit_size) - 1;
  mask <<= bit_pos;
  
  TargetInstruction* input = Materialize(pcode, input_node);
  TargetInstruction* output = Materialize(pcode, output_node);
  TargetInstruction* lsl = Emit(pcode, NewInstruction2(P_OP(lsl), input, GetIntConstant(pcode, NULL, kTargetType32Bit, bit_pos)));
  TargetInstruction* m1 = Emit(pcode, NewInstruction2(P_OP(and), lsl, GetIntConstant(pcode, NULL, kTargetType32Bit, mask)));

  TargetInstruction* m2 = Emit(pcode, NewInstruction2(P_OP(and), output, GetIntConstant(pcode, NULL, kTargetType32Bit, ~mask)));
  TargetInstruction* result = Emit(pcode, NewInstruction2(P_OP(or), m1, m2));
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerAsm(PCodeGenerator* pcode, IRNode* node) {
  // The first argument is a literal containing the assembly language.
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(pcode, TargetNewLiteral((int)id_node->value.ivalue));

  TargetInstruction* result = Emit(pcode, NewInstruction1(P_OP(asm), literal));

  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerInc(PCodeGenerator* pcode, IRNode* node) {
  IRNode* addr_node = node->inputs.value.p[0];
  PCodeOpcode ld_opcode;
  PCodeOpcode st_opcode;
  switch (node->opcode) {
    case IR_OP(inc8):
      ld_opcode = P_OP(ldb);
      st_opcode = P_OP(stb);
      break;
    case IR_OP(uinc8):
      ld_opcode = P_OP(ldub);
      st_opcode = P_OP(stb);
      break;
    case IR_OP(inc16):
      ld_opcode = P_OP(ldh);
      st_opcode = P_OP(sth);
      break;
    case IR_OP(uinc16):
      ld_opcode = P_OP(lduh);
      st_opcode = P_OP(sth);
      break;
   case IR_OP(inc32):
      ld_opcode = P_OP(ldw);
      st_opcode = P_OP(stw);
      break;
    case IR_OP(uinc32):
       ld_opcode = P_OP(lduw);
       st_opcode = P_OP(stw);
       break;
    case IR_OP(inc64):
    case IR_OP(uinc64):
    case IR_OP(inca):
      ld_opcode = P_OP(ldx);
      st_opcode = P_OP(stx);
     break;
    case IR_OP(incf):
      ld_opcode = P_OP(ldf);
      st_opcode = P_OP(stf);
     break;
    case IR_OP(incd):
      ld_opcode = P_OP(ldd);
      st_opcode = P_OP(std);
     break;
    default:
      abort();
  }
  TargetInstruction* load = Load(pcode, addr_node, ld_opcode);
  TargetInstruction* inc;
  if (TypeUsesHardwareFloatRegister(node->type)) {
    TargetInstruction* amount = GetLoweredNode(node->inputs.value.p[1]);
    inc = Emit(pcode,
               NewInstruction2(PCodeFpIsDoubleWidth(node->type) ? P_OP(addd)
                                                                : P_OP(addf),
                               load, amount));
  } else  {
    inc =  AddImmediate(pcode, load, PCodeIntValue(GetLoweredNode(node->inputs.value.p[1])));
  }
  Store(pcode, addr_node, inc, st_opcode);
  return SetLoweredNode(node, inc);
}


static TargetInstruction* LowerDec(PCodeGenerator* pcode, IRNode* node) {
  IRNode* addr_node = node->inputs.value.p[0];
  PCodeOpcode ld_opcode;
  PCodeOpcode st_opcode;
  switch (node->opcode) {
    case IR_OP(dec8):
      ld_opcode = P_OP(ldb);
      st_opcode = P_OP(stb);
      break;
    case IR_OP(udec8):
      ld_opcode = P_OP(ldub);
      st_opcode = P_OP(stb);
      break;
    case IR_OP(dec16):
      ld_opcode = P_OP(ldh);
      st_opcode = P_OP(sth);
      break;
    case IR_OP(udec16):
      ld_opcode = P_OP(lduh);
      st_opcode = P_OP(sth);
      break;
   case IR_OP(dec32):
      ld_opcode = P_OP(ldw);
      st_opcode = P_OP(stw);
      break;
    case IR_OP(udec32):
       ld_opcode = P_OP(lduw);
       st_opcode = P_OP(stw);
       break;
    case IR_OP(dec64):
    case IR_OP(udec64):
    case IR_OP(deca):
      ld_opcode = P_OP(ldx);
      st_opcode = P_OP(stx);
     break;
    case IR_OP(decf):
      ld_opcode = P_OP(ldf);
      st_opcode = P_OP(stf);
     break;
    case IR_OP(decd):
      ld_opcode = P_OP(ldd);
      st_opcode = P_OP(std);
     break;
    default:
      abort();
  }
  TargetInstruction* load = Load(pcode, addr_node, ld_opcode);
  TargetInstruction* inc;
  if (TypeUsesHardwareFloatRegister(node->type)) {
    TargetInstruction* amount = GetLoweredNode(node->inputs.value.p[1]);
    inc = Emit(pcode,
               NewInstruction2(PCodeFpIsDoubleWidth(node->type) ? P_OP(subd)
                                                                : P_OP(subf),
                               load, amount));
  } else  {
    inc =  AddImmediate(pcode, load, -PCodeIntValue(GetLoweredNode(node->inputs.value.p[1])));
  }
  Store(pcode, addr_node, inc, st_opcode);
  return SetLoweredNode(node, inc);
}


static TargetInstruction* LowerStackPointerOps(PCodeGenerator* pcode, IRNode* node) {
  switch (node->opcode) {
    case IR_OP(decsp): {
      TargetInstruction* size = Materialize(pcode, node->inputs.value.p[0]);
      TargetInstruction* new_sp;
      if (TargetIsConst(size)) {
        int64_t s = PCodeIntValue(size);
        new_sp = AddImmediate(pcode, StackPointer(pcode), -s);
      } else {
        new_sp = Emit(pcode, NewInstruction2(P_OP(sub), StackPointer(pcode), size));
      }
      new_sp->dest = StackPointer(pcode);
      return new_sp;
    }
    case IR_OP(savesp): {
      // One operand, a temp to hold stack pointer.
      TargetInstruction* mov =  Emit(pcode, NewInstruction1(P_OP(mov), StackPointer(pcode)));
      mov->dest = Materialize(pcode, node->inputs.value.p[0]);
      return mov;
    }
    case IR_OP(restoresp): {
      TargetInstruction* tmp = Materialize(pcode, node->inputs.value.p[0]);
      TargetInstruction* mov =  Emit(pcode, NewInstruction1(P_OP(mov), tmp));
      mov->dest = StackPointer(pcode);
      return mov;
    }
    default:
      assert(false);
      return NULL;
  }
}

static TargetInstruction* LowerLocation(PCodeGenerator* pcode, IRNode* node) {
  IRLocation* loc = (IRLocation*)node;
  return SetLoweredNode(node, Emit(pcode, TargetNewLocation(loc)));
}

static int64_t CalculateTypeArgumentSize(TypeRecord* type) {
  if (type == NULL) {
    return 0;
  }
  if (TypeUsesHardwareFloatRegister(type)) {
    return PCodeFpIsDoubleWidth(type) ? 8 : 4;
  }
  if (TypeIsPointerOrArray(type) || TypeIsReference(type)) {
    return 8;
  }
  if (TypeIsStructOrUnion(type) || TypeUsesLongDoubleRepresentation(type)) {
    return type->size;
  }
  return type->size < 4 ? 4 : type->size;
}

// The first input is the address of the 'ap' variable.  The second is the
// address of the last function argument.  The ap variable is set to the
// address immediately after the last argument.
static TargetInstruction* LowerBuiltinVaStart(PCodeGenerator* pcode,
                                              IRNode* node) {
  TargetInstruction* arg_addr;
  TargetInstruction* arg_offset;
  GetAddressAndOffset(pcode, node->inputs.value.p[1], &arg_addr, &arg_offset);
  TargetInstruction* arg =
      Emit(pcode, NewInstruction2(P_OP(addc), arg_addr, arg_offset));
  int64_t argument_size =
      CalculateTypeArgumentSize(((IRNode*)node->inputs.value.p[1])->type);
  TargetInstruction* add = Emit(
      pcode, NewInstruction2(P_OP(addc), arg,
                             GetIntConstant(pcode, NULL, kTargetType32Bit,
                                            argument_size)));

  TargetInstruction* ap_addr;
  TargetInstruction* ap_offset;
  GetAddressAndOffset(pcode, node->inputs.value.p[0], &ap_addr, &ap_offset);
  TargetInstruction* store =
      Emit(pcode, NewInstruction3(P_OP(stx), add, ap_addr, ap_offset));
  return SetLoweredNode(node, store);
}

static TargetInstruction* LowerBuiltinVaArg(PCodeGenerator* pcode,
                                            IRNode* node) {
  TargetInstruction* ap_addr;
  TargetInstruction* ap_offset;
  GetAddressAndOffset(pcode, node->inputs.value.p[0], &ap_addr, &ap_offset);
  TargetInstruction* ap_load =
      Emit(pcode, NewInstruction2(P_OP(ldx), ap_addr, ap_offset));

  PCodeOpcode load_opcode = P_OP(ldx);
  if (TypeUsesFloat32Representation(node->type)) {
    load_opcode = P_OP(ldf);
  } else if (PCodeFpIsDoubleWidth(node->type)) {
    load_opcode = P_OP(ldd);
  } else {
    switch (node->type->size) {
      case 1:
        load_opcode = TypeIsUnsigned(node->type) ? P_OP(ldub) : P_OP(ldb);
        break;
      case 2:
        load_opcode = TypeIsUnsigned(node->type) ? P_OP(lduh) : P_OP(ldh);
        break;
      case 4:
        load_opcode = TypeIsUnsigned(node->type) ? P_OP(lduw) : P_OP(ldw);
        break;
    }
  }

  TargetInstruction* result = Emit(
      pcode, NewInstruction2(load_opcode, ap_load,
                             GetIntConstant(pcode, NULL, kTargetType32Bit, 0)));

  TargetInstruction* size =
      GetIntConstant(pcode, NULL, kTargetType32Bit, node->type->size);
  TargetInstruction* addc =
      Emit(pcode, NewInstruction2(P_OP(addc), ap_load, size));
  Emit(pcode, NewInstruction3(P_OP(stx), addc, ap_addr, ap_offset));
  return SetLoweredNode(node, result);
}

static TargetInstruction* LowerBuiltinVaEnd(PCodeGenerator* pcode,
                                            IRNode* node) {
  // Nothing to do for va_end.
  return NULL;
}

static TargetInstruction* LowerBuiltinVaCopy(PCodeGenerator* pcode,
                                             IRNode* node) {
  return NULL;  // TODO
}

static TargetInstruction* LowerIRNode(PCodeGenerator* pcode, IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  switch (node->opcode) {
    case IR_OP(rotli):
    case IR_OP(rotri):
    case IR_OP(clzi):
    case IR_OP(ctzi):
    case IR_OP(popcounti):
      assert(false && "bit operation must be software-expanded before p-code lowering");
      return NULL;

    case IR_OP(vadd):
    case IR_OP(vsub):
    case IR_OP(vmul):
    case IR_OP(vdiv):
    case IR_OP(vmod):
    case IR_OP(vlsl):
    case IR_OP(vlsr):
    case IR_OP(vasr):
    case IR_OP(vand):
    case IR_OP(vor):
    case IR_OP(vxor):
    case IR_OP(vneg):
    case IR_OP(vonescomp):
    case IR_OP(vcmpeq):
    case IR_OP(vcmpne):
    case IR_OP(vcmplt):
    case IR_OP(vcmple):
    case IR_OP(vcmpgt):
    case IR_OP(vcmpge):
    case IR_OP(vcmpltu):
    case IR_OP(vcmpleu):
    case IR_OP(vcmpgtu):
    case IR_OP(vcmpgeu):
    case IR_OP(vectorarg):
    case IR_OP(resultv):
    case IR_OP(capturev):
      assert(false && "vector operation must be software-expanded before p-code lowering");
      return NULL;

    case IR_OP(localvar):
    case IR_OP(argument):
    case IR_OP(tempvar):
    case IR_OP(staticvar):
    case IR_OP(externvar):
      // These are handled before we get here.
      return NULL;

    case IR_OP(observable_checkpoint): {
      TargetInstruction* checkpoint =
          Emit(pcode, NewInstruction(P_OP(loc)));
      checkpoint->observable_checkpoint = true;
      return checkpoint;
    }
    case IR_OP(nop):
    case last_ir_opcode:
      return NULL;

    case IR_OP(ssavar):
    case IR_OP(phi):
      // We should never see these as we've moved out of SSA form
      // before here.
      break;

    case IR_OP(structreturn): {
      TargetInstruction* result = Emit(
          pcode,
          NewInstruction2(P_OP(ldx), ArgumentPointer(pcode),
                          GetIntConstant(pcode, NULL, kTargetType32Bit, 16)));
      SetLoweredNode(node, result);
      return result;
    }
      
    case IR_OP(nrvoval):
      abort();
      
    case IR_OP(literalref):
      return LowerLiteralReference(pcode, node);

    case IR_OP(addressof):
      return LowerAddressOf(pcode, node);

    case IR_OP(const32):
      return GetIntConstant(pcode, node, kTargetType32Bit,
                            ((IRConstant*)node)->value.ivalue);
    case IR_OP(const8):
      return GetIntConstant(pcode, node, kTargetType8Bit,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(const16):
      return GetIntConstant(pcode, node, kTargetType16Bit,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(consta):
    case IR_OP(const64):
     return GetIntConstant(pcode, node, kTargetType64Bit,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(constf):
      return GetFloatingPointConstant(pcode, node, kTargetTypeFloat,
                                      ((IRConstant*)node)->value.fvalue);

    case IR_OP(constd):
      return GetFloatingPointConstant(pcode, node, kTargetTypeDouble,
                                      ((IRConstant*)node)->value.fvalue);

    case IR_OP(enter): {
      // Entry sequence:
      // pushx ap
      // mov ap, sp
      // save (registers)
      // pushx fp
      // mov fp, sp
      // decsp #frame_size
      Emit(pcode, NewInstruction1(P_OP(pushx), ArgumentPointer(pcode)));
      TargetInstruction* mov = Emit(pcode, NewInstruction1(P_OP(mov),
                                  StackPointer(pcode)));
      mov->dest = ArgumentPointer(pcode);
      Emit(pcode, NewInstruction(P_OP(save)));
      Emit(pcode, NewInstruction1(P_OP(pushx), FramePointer(pcode)));
      TargetInstruction* result =
          Emit(pcode, NewInstruction1(P_OP(mov),
                                      StackPointer(pcode)));
      result->dest = FramePointer(pcode);
      if (pcode->base.stack_frame_size > 0) {
        return Emit(pcode, NewInstruction1(
                               P_OP(decsp),
                               GetIntConstant(pcode, NULL, kTargetType32Bit,
                                              pcode->base.stack_frame_size)));
      }
      return result;
    }
    case IR_OP(leave): {
      // Exit sequence:
      // mov sp, fp
      // popx fp
      // restore (registers)
      // popx ap
      // Reconstructing sp from fp also discards any VLA or over-aligned
      // allocations made below the fixed frame.
      TargetInstruction* restore_sp =
          Emit(pcode, NewInstruction1(P_OP(mov), FramePointer(pcode)));
      restore_sp->dest = StackPointer(pcode);
      Emit(pcode, NewInstruction1(P_OP(popx), FramePointer(pcode)));
      Emit(pcode, NewInstruction(P_OP(restore)));
      return Emit(pcode, NewInstruction1(P_OP(popx), ArgumentPointer(pcode)));
    }

    case IR_OP(ret):
      return Emit(pcode, NewInstruction(P_OP(ret)));

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
      return LowerLoad(pcode, node);

    // stores.
    case IR_OP(store32):
    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store64):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
      return LowerStore(pcode, node);

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
      return LowerInc(pcode, node);
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
    return LowerDec(pcode, node);

    case IR_OP(getbit):
      return LowerGetBitField(pcode, node);
      
    case IR_OP(setbit):
      return LowerSetBitField(pcode, node);

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

    case IR_OP(ori):
    case IR_OP(andi):
    case IR_OP(xori):

    case IR_OP(noti):
    case IR_OP(nota):
    case IR_OP(onescomp):
    case IR_OP(negi):
    case IR_OP(negf):
    case IR_OP(negd):

    case IR_OP(cmpeqi):
    case IR_OP(cmpnei):
    case IR_OP(cmplti):
    case IR_OP(cmplei):
    case IR_OP(cmpgti):
    case IR_OP(cmpgei):

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

    case IR_OP(cmpeqa):
    case IR_OP(cmpnea):
    case IR_OP(cmplta):
    case IR_OP(cmplea):
    case IR_OP(cmpgta):
    case IR_OP(cmpgea):

    case IR_OP(cmp3wayi):
    case IR_OP(cmp3wayu):
    case IR_OP(cmp3wayf):
    case IR_OP(cmp3wayd):
    case IR_OP(cmp3waya):

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
//    case IR_OP(rmovi):
//    case IR_OP(rmovf):
//    case IR_OP(rmovd):
//    case IR_OP(rmova):
    case IR_OP(tmp):
      return LowerExpression(pcode, node);

    case IR_OP(btrue):
    case IR_OP(bfalse):
      return LowerConditionalBranch(pcode, node);

    case IR_OP(bra):
      return LowerBranch(pcode, node);

    case IR_OP(cbra):
      return LowerComputedBranch(pcode, node);

    case IR_OP(label):
      return LowerLabel(pcode, node);
      
    case IR_OP(named_label):
        return LowerNamedLabel(pcode, node);

    case IR_OP(pusharg):
      return SetLoweredNode(node, Materialize(pcode, node->inputs.value.p[0]));

    case IR_OP(calla):
      return LowerCall(pcode, node);

    case IR_OP(structarg):
      // Same as its input.
      return SetLoweredNode(node, Materialize(pcode, node->inputs.value.p[0]));

    case IR_OP(resulti):
    case IR_OP(resultf):
    case IR_OP(resultd):
    case IR_OP(resulta):
      return LowerResult(pcode, node);

    case IR_OP(memzero):
      return LowerMemzero(pcode, node);

    case IR_OP(memcpy):
      return LowerMemcpy(pcode, node);
      
    case IR_OP(cast):
      return LowerCast(pcode, node);

    case IR_OP(zeroextendi):
      return LowerZeroExtend(pcode, node);

    case IR_OP(signextendi):
      return LowerSignExtend(pcode, node);
      
    case IR_OP(aligni):
      return LowerAlign(pcode, node);
      
    case IR_OP(asm):
      return LowerAsm(pcode, node);

    case IR_OP(loc):
      return LowerLocation(pcode, node);

    case IR_OP(builtin_va_start):
      return LowerBuiltinVaStart(pcode, node);

    case IR_OP(builtin_va_arg):
      return LowerBuiltinVaArg(pcode, node);

    case IR_OP(builtin_va_end):
      return LowerBuiltinVaEnd(pcode, node);

    case IR_OP(builtin_va_copy):
      return LowerBuiltinVaCopy(pcode, node);

    case IR_OP(atomic_load):
      return LowerAtomicLoad(pcode, node);

    case IR_OP(atomic_store):
      return LowerAtomicStore(pcode, node);

    case IR_OP(atomic_fetch_add):
      return LowerAtomicFetchAddSub(pcode, node, true, false);

    case IR_OP(atomic_fetch_sub):
      return LowerAtomicFetchAddSub(pcode, node, false, false);

    case IR_OP(atomic_add_fetch):
      return LowerAtomicFetchAddSub(pcode, node, true, true);

    case IR_OP(atomic_sub_fetch):
      return LowerAtomicFetchAddSub(pcode, node, false, true);

    case IR_OP(atomic_compare_exchange_bool):
      return LowerAtomicCompareExchange(pcode, node, false, true);

    case IR_OP(atomic_compare_exchange_val):
      return LowerAtomicCompareExchange(pcode, node, false, false);

    case IR_OP(atomic_compare_exchange_n):
      return LowerAtomicCompareExchange(pcode, node, true, true);

    case IR_OP(atomic_fence):
      return SetLoweredNode(node, GetIntConstant(pcode, node, kTargetType32Bit, 0));
      
    case IR_OP(decsp):
    case IR_OP(savesp):
    case IR_OP(restoresp):
      return LowerStackPointerOps(pcode, node);

    default:
      break;
  }
  // If we get here we've failed to handle the IR node.
  assert(false);
  return NULL;
}

// Calculate the size of an argument based on its type.
static int64_t CalculateArgumentSize(Symbol* arg) {
  if (arg == NULL || arg->type == NULL) {
    return 0;
  }
  return CalculateTypeArgumentSize(arg->type);
}

void PCodeLower(PCodeGenerator* pcode, Generator* gen) {
  // Variables are allocated below the frame, arguments are above.
  int32_t var_offset = 0;
  int32_t arg_offset = 16;  // Leave room for return address and saved ap.
  if (TypeReturnedThroughHiddenPointer(compiler->current_function->next)) {
    // Memory returns use an invisible first argument at ap + 16.
    arg_offset += 8;
  }
  
  // Calculate the offset on the stack for all the arguments.  Store this offset
  // in the stack_offset field of the Symbol.
  Vector* prototype = &compiler->current_function->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* arg = prototype->value.p[i];
    if (arg == NULL || arg->type == NULL) {
      continue;
    }
    arg->stack_offset = arg_offset;
    int64_t size = CalculateArgumentSize(arg);
    arg_offset += size;
  }

  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = (PoolEntry*)gen->variable_pool.value.p[i];
    if (entry->pooled->opcode == IR_OP(localvar) ||
        entry->pooled->opcode == IR_OP(tempvar)) {
      if ((entry->pooled->flags & kIRNrvoMarker) != 0) {
        continue;
      }
      TypeRecord* type = entry->value.symbol->type;
      if (TypeIsVLA(type)) {
        continue;
      }
      if (type->size == 0) {
        TypeRecordCalculateSize(type);
      }
      int32_t size = type->size;
      int32_t alignment = PoolEntryStackAlignment(entry);
      assert(size > 0);
      assert(alignment > 0 && (alignment & (alignment - 1)) == 0);
      var_offset = (var_offset + alignment - 1) & ~(alignment - 1);
      entry->pooled->data.ivalue = var_offset;
      var_offset += size;
    } else if (entry->pooled->opcode == IR_OP(argument)) {
      // Get the stack offset we calculated above.
      entry->pooled->data.ivalue = entry->value.symbol->stack_offset;
    } else if (entry->pooled->opcode == IR_OP(staticvar) ||
               entry->pooled->opcode == IR_OP(externvar)) {
      // Static variables are referenced by a symbol instruction.
      IRVariable* var = (IRVariable*)entry->pooled;
      TargetInstruction* inst = GetSymbol(pcode, NULL, var->symbol);
      entry->pooled->data.ptr = inst;
    }
  }

  // We now know the stack frame size.
  pcode->base.stack_frame_size = (var_offset + 7) & ~7;

  IRNode* node = GeneratorFirstInstruction(gen);
  while (node != NULL) {
    LowerIRNode(pcode, node);
    node = IRNext(node);
  }
  ResolveExceptionRanges(pcode, gen);

  PCodeOptimize(pcode);

  if (compiler->print_back_end) {
    PCodePrint(pcode);
  }
  
  // Allocate registers to the instructions.
  PCodeAllocateRegisters(&pcode->register_allocator);
}

void PCodePrint(PCodeGenerator* pcode) {
  TargetInstruction* inst = TargetFirstInstruction(&pcode->base);
  while (inst != NULL) {
    TargetPrintInstruction(inst, PCodeOpcodeName, stdout);
    inst = TargetNext(inst);
  }
}

bool PCodeIsExpression(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
    case P_OP(mov):
    case P_OP(movf):
    case P_OP(movd):
      return inst->dest == NULL;
    case P_OP(save):
    case P_OP(restore):
    case P_OP(label):
    case P_OP(asm):
    case P_OP(decsp):
    case P_OP(incsp):
    case P_OP(push):
    case P_OP(pushf):
    case P_OP(pushd):
    case P_OP(pushx):
    case P_OP(pop):
    case P_OP(popf):
    case P_OP(popd):
    case P_OP(popx):
    case P_OP(stw):
    case P_OP(sth):
    case P_OP(stx):
    case P_OP(stf):
    case P_OP(std):
    case P_OP(stb):
    case P_OP(bnz):
    case P_OP(bz):
    case P_OP(bra):
    case P_OP(cbra):
    case P_OP(jmp):
    case P_OP(cjmp):
    case P_OP(call):
    case P_OP(callf):
    case P_OP(calld):
    case P_OP(rcall):
    case P_OP(rcallf):
    case P_OP(rcalld):
    case P_OP(ret):
//    case P_OP(rmov):
//    case P_OP(rmovf):
//    case P_OP(rmovd):
    case P_OP(loc):
    case P_OP(named_label):
      return false;

    default:
      return true;
  }
}

bool PCodeIsSignedLoad(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
    case P_OP(ldb):
    case P_OP(ldh):
    case P_OP(ldw):
      return true;
    default:
      return false;
  }
}
