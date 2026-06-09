//
//  g->_codegen.c
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "arm_codegen.h"
#include "arm_optimize.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "debug.h"

#include "target_basic_block.h"

static void LowerVariables(ARMGenerator* g, Generator* gen);

static void Trap() {}
static void TrapLower(String* name) {
  if (StringEqual(name, "")) {
    Trap();
  }
}

const char* ARMOpcodeName(int op) {
  switch (op) {
  case ARM_OP(save): return "save";
  case ARM_OP(restore): return "restore";

  case ARM_OP(symbol): return "symbol";   // Static symbol.
  case ARM_OP(literal): return "literal";  // String literal.
  case ARM_OP(tmp): return "tmp";

  // Constants.
  case ARM_OP(const8): return "const8";
  case ARM_OP(const16): return "const16";
  case ARM_OP(const32): return "const32";
  case ARM_OP(const64): return "const64";
  case ARM_OP(constf): return "constf";
  case ARM_OP(constd): return "constd";

  case ARM_OP(mv): return "mv";
  case ARM_OP(fmv_s): return "fmv_s";
  case ARM_OP(fmv_d): return "fmv_d";

  case ARM_OP(movc): return "movc";
  case ARM_OP(movfc): return "movfc";
  case ARM_OP(movdc): return "movdc";
  case ARM_OP(movxc): return "movxc";

  case ARM_OP(ret): return "ret";

  case ARM_OP(label): return "label";

  case ARM_OP(fp): return "fp";  // Frame pointer pseudo operation.
  case ARM_OP(sp): return "sp";  // Stack pointer pseudo operation.
  case ARM_OP(tp): return "tp";  // Thread pointer pseudo operation.

  // Function result registers.
  case ARM_OP(resulti): return "resulti";
  case ARM_OP(resultf): return "resultf";
  case ARM_OP(resultd): return "resultd";

  case ARM_OP(structreturn): return "structreturn";  // Struct return address.

  case ARM_OP(asm): return "asm";

  case ARM_OP(loc): return "loc";
  case ARM_OP(named_label): return "named_label";
  case ARM_OP(ivarreg): return "ivarreg";
  case ARM_OP(fvarreg): return "fvarreg";
  
  // End of TargetOpcode enumeration.

  // Now follow the actual ARMv8 64 instruction directly from the
  // specifications.

  case ARM_OP(adc): return "adc";
  case ARM_OP(add): return "add";
  case ARM_OP(adcs): return "adcs";
  case ARM_OP(adds): return "adds";
  case ARM_OP(adr): return "adr";
  case ARM_OP(adrp): return "adrp";
  case ARM_OP(cmn): return "cmn";
  case ARM_OP(cmp): return "cmp";
  case ARM_OP(madd): return "madd";
  case ARM_OP(mneg): return "mneg";
  case ARM_OP(msub): return "mls";
  case ARM_OP(mul): return "mul";
  case ARM_OP(neg): return "neg";
  case ARM_OP(ngc): return "ngc";
  case ARM_OP(sbc): return "sbc";
  case ARM_OP(negs): return "negs";
  case ARM_OP(ngcs): return "ngcs";
  case ARM_OP(sbcs): return "sbcs";
    case ARM_OP(sdiv): return "sdiv";
    case ARM_OP(smod): return "smod";
  case ARM_OP(smaddl): return "smaddl";
  case ARM_OP(smnegl): return "smnegl";
  case ARM_OP(smsubl): return "smsubl";
  case ARM_OP(smulh): return "smulh";
  case ARM_OP(smull): return "smull";
  case ARM_OP(sub): return "sub";
  case ARM_OP(subs): return "subs";
    case ARM_OP(udiv): return "udiv";
    case ARM_OP(umod): return "umod";
  case ARM_OP(umaddl): return "umaddl";
  case ARM_OP(umnegl): return "umnegl";
  case ARM_OP(umsubl): return "umsubl";
  case ARM_OP(umulh): return "umulh";
    case ARM_OP(umull): return "umull";
    case ARM_OP(eor): return "eor";

  case ARM_OP(bfi): return "bfi";
  case ARM_OP(bfxil): return "bfxil";
  case ARM_OP(cls): return "cls";
  case ARM_OP(clz): return "clz";
  case ARM_OP(extr): return "extr";
  case ARM_OP(rbit): return "rbit";
  case ARM_OP(rev): return "rev";
  case ARM_OP(rev16): return "rev16";
  case ARM_OP(rev32): return "rev32";
  case ARM_OP(sbfiz): return "sbfiz";
  case ARM_OP(ubfiz): return "ubfiz";
  case ARM_OP(sbfx): return "sbfx";
  case ARM_OP(ubfx): return "ubfx";
  case ARM_OP(sbxt): return "sbxt";
  case ARM_OP(sbxtb): return "sbxtb";
  case ARM_OP(sbxth): return "sbxth";
  case ARM_OP(ubxt): return "ubxt";
  case ARM_OP(ubxtb): return "ubxtb";
  case ARM_OP(ubxth): return "ubxth";
  case ARM_OP(sxtb): return "sxtb";
  case ARM_OP(sxth): return "sxth";
  case ARM_OP(sxtw): return "sxtw";

  case ARM_OP(and): return "and";
  case ARM_OP(ands): return "ands";
  case ARM_OP(asr): return "asr";
  case ARM_OP(bic): return "bic";
  case ARM_OP(bics): return "bics";
  case ARM_OP(eon): return "eon";
  case ARM_OP(eons): return "eons";
  case ARM_OP(lsl): return "lsl";
  case ARM_OP(lsr): return "lsr";
  case ARM_OP(mov): return "mov";
  case ARM_OP(movk): return "movk";
  case ARM_OP(movn): return "movn";
  case ARM_OP(movz): return "movz";
  case ARM_OP(movw): return "movw";
  case ARM_OP(movt): return "movt";
  case ARM_OP(mvn): return "mvn";
  case ARM_OP(orn): return "orn";
  case ARM_OP(orr): return "orr";
  case ARM_OP(orrs): return "orrs";
  case ARM_OP(ror): return "ror";
  case ARM_OP(tst): return "tst";

  case ARM_OP(b): return "b";
  case ARM_OP(bl): return "bl";
  case ARM_OP(blr): return "blr";
  case ARM_OP(br): return "br";
  case ARM_OP(cbnz): return "cbnz";
  case ARM_OP(cbz): return "cbz";
  case ARM_OP(tbnz): return "tbnz";
  case ARM_OP(tbz): return "tbz";
  
    case ARM_OP(eq): return "eq";
    case ARM_OP(ne): return "ne";
    case ARM_OP(cs): return "cs";
    case ARM_OP(hs): return "hs";
    case ARM_OP(cc): return "cc";
    case ARM_OP(lo): return "lo";
    case ARM_OP(mi): return "mi";
    case ARM_OP(pl): return "pl";
    case ARM_OP(vs): return "vs";
    case ARM_OP(vc): return "vc";
    case ARM_OP(hi): return "hi";
    case ARM_OP(ls): return "ls";
    case ARM_OP(ge): return "ge";
    case ARM_OP(lt): return "lt";
    case ARM_OP(gt): return "gt";
    case ARM_OP(le): return "le";
    case ARM_OP(al): return "al";
      
  case ARM_OP(ccmn): return "ccmn";
  case ARM_OP(ccmp): return "ccmp";
  case ARM_OP(cinc): return "cinc";
  case ARM_OP(cinv): return "cinv";
  case ARM_OP(cneg): return "cneg";
  case ARM_OP(csel): return "csel";
  case ARM_OP(cset): return "cset";
  case ARM_OP(csetm): return "csetm";
  case ARM_OP(csinc): return "csinc";
  case ARM_OP(csinv): return "csinv";
  case ARM_OP(csneg): return "csneg";
  
  case ARM_OP(ldp): return "ldp";
  case ARM_OP(ldpsw): return "ldpsw";
  case ARM_OP(ldr): return "ldr";
  case ARM_OP(ldur): return "ldur";
  case ARM_OP(ldrb): return "ldrb";
  case ARM_OP(ldrh): return "ldrh";
  case ARM_OP(ldurb): return "ldurb";
  case ARM_OP(ldurh): return "ldurh";
  case ARM_OP(ldrsb): return "ldrsb";
  case ARM_OP(ldrsh): return "ldrsh";
  case ARM_OP(ldursb): return "ldursb";
  case ARM_OP(ldursh): return "ldursh";
  case ARM_OP(ldursw): return "ldursw";
  case ARM_OP(prfm): return "prfm";
  case ARM_OP(stp): return "stp";
  case ARM_OP(str): return "str";
  case ARM_OP(stur): return "stur";
  case ARM_OP(strb): return "strb";
  case ARM_OP(strh): return "strh";
  case ARM_OP(sturb): return "sturb";
  case ARM_OP(sturh): return "sturh";

  case ARM_OP(fldr): return "fldr";
  case ARM_OP(fstr): return "fstr";
  case ARM_OP(fadd): return "fadd";
  case ARM_OP(fsub): return "fsub";
  case ARM_OP(fmul): return "fmul";
  case ARM_OP(fdiv): return "fdiv";
  case ARM_OP(fsqrt): return "fsqrt";
  case ARM_OP(fmin): return "fmin";
  case ARM_OP(fmax): return "fmax";
  case ARM_OP(fcvtns): return "fcvtns";
  case ARM_OP(fcvtnu): return "fcvtnu";
    case ARM_OP(fcvtsd): return "fcvtsd";
    case ARM_OP(fcvtds): return "fcvtds";
  case ARM_OP(fmov): return "fmov";
  case ARM_OP(fcmp): return "fcmp";
  case ARM_OP(scvtf): return "scvtf";
  case ARM_OP(ucvtf): return "ucvtf";
    case ARM_OP(fneg): return "fneg";
    case ARM_OP(fcvt): return "fcvt";

  case ARM_OP(xxx): return "xxx";
    case ARM_OP(not): return "not";
    case ARM_OP(nop): return "nop";

    case ARM_OP(oplsl): return "oplsl";

  case ARM_OP(nrvoval): return "nrvoval";
  
  // Argument Registers
  // int
  case ARM_OP(r0): return "r0";
  case ARM_OP(r1): return "r1";
  case ARM_OP(r2): return "r2";
  case ARM_OP(r3): return "r3";
  case ARM_OP(r4): return "r4";
  case ARM_OP(r5): return "r5";
  case ARM_OP(r6): return "r6";
    case ARM_OP(r7): return "r7";
    case ARM_OP(r8): return "r8";
    case ARM_OP(r9): return "r9";

  case ARM_OP(d0): return "d0";
  case ARM_OP(d1): return "d1";
  case ARM_OP(d2): return "d2";
  case ARM_OP(d3): return "d3";
  case ARM_OP(d4): return "d4";
  case ARM_OP(d5): return "d5";
  case ARM_OP(d6): return "d6";
  case ARM_OP(d7): return "d7";

  case ARM_OP(zr): return "zr";
  case ARM_OP(lr): return "lr";
  case ARM_OP(xr): return "xr";

  case ARM_OP(regarg): return "regarg";  // Holder for reg args.
  
  // Spill and reload.
  case ARM_OP(spill): return "spill";
  case ARM_OP(reload): return "reload";
    default: return "unknown ARM instruction";
  }
}

bool ARMIsExpression(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(label):
    case ARM_OP(asm):
    case ARM_OP(b):
    case ARM_OP(bl):
    case ARM_OP(blr):
    case ARM_OP(br):
    case ARM_OP(cbnz):
    case ARM_OP(cbz):
    case ARM_OP(tbnz):
    case ARM_OP(tbz):
    case ARM_OP(ret):
    case ARM_OP(save):
    case ARM_OP(restore):
    case ARM_OP(stp):
    case ARM_OP(str):
    case ARM_OP(stur):
    case ARM_OP(strb):
    case ARM_OP(strh):
    case ARM_OP(sturb):
    case ARM_OP(sturh):
    case ARM_OP(fstr):
    case ARM_OP(loc):
    case ARM_OP(named_label):
    case ARM_OP(regarg):
    case ARM_OP(nrvoval):
    case ARM_OP(symbol):
    case ARM_OP(spill):
      return false;
    default:
      return !ARMIsFixedRegister(inst) && !ARMIsConst(inst);
  }
}

bool ARMGeneratesOutput(TargetInstruction* inst) {
  if (ARMIsFixedRegister(inst)) {
    return false;
  }
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(ivarreg):
    case ARM_OP(fvarreg):
    case ARM_OP(resulti):
    case ARM_OP(resultf):
    case ARM_OP(resultd):
      return false;
    default:
      return ARMIsExpression(inst) && !ARMIsSymbol(inst) && !ARMIsConst(inst);
  }
}

bool ARMIsLoad(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(ldp):
    case ARM_OP(ldpsw):
    case ARM_OP(ldr):
    case ARM_OP(ldur):
    case ARM_OP(ldrb):
    case ARM_OP(ldrh):
    case ARM_OP(ldurb):
    case ARM_OP(ldurh):
    case ARM_OP(ldrsb):
    case ARM_OP(ldrsh):
    case ARM_OP(ldursb):
    case ARM_OP(ldursh):
    case ARM_OP(ldursw):
      return true;
    default:
      return false;
  }
}

bool ARMIsSignedLoad(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(ldp):
    case ARM_OP(ldpsw):
    case ARM_OP(ldr):
    case ARM_OP(ldur):
    case ARM_OP(ldrb):
    case ARM_OP(ldrh):
    case ARM_OP(ldrsb):
    case ARM_OP(ldrsh):
      return true;
    default:
      return false;
  }
}

bool ARMIsStore(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(stp):
    case ARM_OP(str):
    case ARM_OP(stur):
    case ARM_OP(strb):
    case ARM_OP(strh):
    case ARM_OP(sturb):
    case ARM_OP(sturh):
      return true;
    default:
      return false;
  }
}

bool ARMIsIntConst(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(const32):
    case ARM_OP(const8):
    case ARM_OP(const16):
    case ARM_OP(const64):
      return true;
    default:
      return false;
  }
}

bool ARMIsConst(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(const32):
    case ARM_OP(const8):
    case ARM_OP(const16):
    case ARM_OP(const64):
    case ARM_OP(constf):
    case ARM_OP(constd):
      return true;
    default:
      return false;
  }
}

bool ARMIsFloatingPoint(TargetInstruction* inst) {
  if ((ARMOpcode)inst->opcode < ARM_OP(fldr) || (ARMOpcode)inst->opcode > ARM_OP(ucvtf)) {
    return false;
  }
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(fvarreg):
    case ARM_OP(constf):
    case ARM_OP(constd):
    case ARM_OP(fmv_s):
    case ARM_OP(fmv_d):
       return true;
    default:
      return false;
  }
}

bool ARMIsSymbol(TargetInstruction* inst) {
  return (ARMOpcode)((int)inst->opcode == (int)ARM_OP(symbol));
}

bool ARMIsCall(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(bl):
    case ARM_OP(blr):
      return true;
    default:
      return false;
  }
}

bool ARMIsArgRegister(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(r0):
    case ARM_OP(r1):
    case ARM_OP(r2):
    case ARM_OP(r3):
    case ARM_OP(r4):
    case ARM_OP(r5):
    case ARM_OP(r6):
    case ARM_OP(r7):
      return true;
    default:
      return false;
  }
}
 
bool ARMIsVarRegister(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
  case ARM_OP(ivarreg):
  case ARM_OP(fvarreg):
      return true;
  default:
    return false;
  }
}

bool ARMIsFixedRegister(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(r0):
    case ARM_OP(r1):
    case ARM_OP(r2):
    case ARM_OP(r3):
    case ARM_OP(r4):
    case ARM_OP(r5):
    case ARM_OP(r6):
    case ARM_OP(r7):
    case ARM_OP(r9):  // Dedicated scratch/temp register (see Tmp()).
    case ARM_OP(fp):
    case ARM_OP(sp):
    case ARM_OP(lr):
    case ARM_OP(xr):
    case ARM_OP(zr):

      // Calls always return in z0 (or fa0?).
    case ARM_OP(bl):
    case ARM_OP(blr):
      return true;
    default:
      return false;
  }
}

bool ARMIsResult(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
     case ARM_OP(resulti):
      case ARM_OP(resultf):
      case ARM_OP(resultd):
      return true;
    default:
      return false;
  }
}

bool ARMIsBranch(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(b):
    case ARM_OP(br):
      return true;
    default:
      return false;
  }
}

bool ARMIsConditionalBranch(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(b):
      // AL condition means unconditional.
      return ((int)inst->operand[0]->opcode != (int)ARM_OP(al));
 
    case ARM_OP(br):
      return false;
      
    default:
      return false;
  }
}

bool ARMIsSpill(TargetInstruction* inst) {
  return (ARMOpcode)((int)inst->opcode == (int)ARM_OP(spill));
}

bool ARMIsLabel(TargetInstruction* inst) {
  return (ARMOpcode)((int)inst->opcode == (int)ARM_OP(label));
}

bool ARMIsReturn(TargetInstruction* inst) {
  return (ARMOpcode)((int)inst->opcode == (int)ARM_OP(ret));
}

int ARMIntValue(TargetInstruction* inst) {
  if (inst->opcode == (TargetOpcode)ARM_OP(r0)) {
    return 0;
  }
  return (int)((TargetConstant*)inst)->value.ivalue;
}

// Is the value small enough to be encoded in an immediate field?
// TODO: there are many sizes of immediate fields.  This is wrong->base.
bool ARMIsPossibleImmediate(int64_t value) {
  // Check for 12 bit signed immediate.
  if (value < 0) {
    return value >= -2048;
  }
  return value < 2048;
}

// The size of the data an instruction operates on is 32 or 64 bits.
int ARMGetRegisterSize(TargetInstruction* inst) {
  return (inst->flags >> 16) & 3;
}

static TargetInstruction* SetInstructionSize(TargetInstruction* inst, int size) {
  inst->flags = (inst->flags & ~(3<<16)) | (size << 16);
  return inst;
}

// Copy size from operand.
static TargetInstruction* CopyInstructionSize(TargetInstruction* inst, int op) {
  if (inst->operand[op] == NULL) {
    return inst;
  }
  inst->flags = (inst->flags & ~(3<<16)) | ((inst->operand[op]->flags & (3 << 16)));
  return inst;
}

// On the ARM target `long double` has the same 8-byte size and representation
// as `double`, so the backend must treat the two identically for register and
// instruction selection (size, fp load/store, register class).  TypeIsDouble
// only matches the `double` type bit, so use this wherever a "64-bit FP value"
// decision is made.
static bool ARMTypeIsDouble(TypeRecord* type) {
  return TypeIsDouble(type) || TypeIsLongDouble(type);
}

static TargetInstruction* CopyOrSetInstructionSize(IRNode* node, TargetInstruction* inst) {
  // Floating-point values are sized by their own type, never by their operands:
  // a load's operand is a 32-bit address, so copying from operand[0] would
  // wrongly size a `double` load (and every value derived from it) as 32 bits.
  if (node->type != NULL && ARMTypeIsDouble(node->type)) {
    SetInstructionSize(inst, kSize64Bit);
    return inst;
  }
  if (node->type != NULL && TypeIsFloat(node->type)) {
    SetInstructionSize(inst, kSize32Bit);
    return inst;
  }
  if (node->inputs.length == 0) {
    // Leaf node, set size based on node type.
    int size = kSize32Bit;
    if (TypeIsLongLong(node->type) || ARMTypeIsDouble(node->type)) {
      size = kSize64Bit;
    }
    SetInstructionSize(inst, size);
  } else {
    // Propagate the instruction size from the first operand.
    CopyInstructionSize(inst, 0);
  }
  return inst;
}

TargetInstruction* ARMGetBranchTarget(TargetInstruction* inst) {
  if (((int)inst->opcode == (int)ARM_OP(br))) {
    return inst->operand[0];
  }
  return inst->operand[1];
}

bool ARMIsJumpableEntry(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)ARM_OP(b) && ((int)inst->operand[0]->opcode == (int)ARM_OP(al));
}

static bool HasLoweredNode(IRNode* node) {
  return node->data.ptr != NULL;
}

static TargetVirtuals virtuals = {
  .opcode_name = ARMOpcodeName,
  .is_branch = ARMIsBranch,
  .is_call = ARMIsCall,
  .is_return = ARMIsReturn,
  .is_spill = ARMIsSpill,
  .is_label = ARMIsLabel,
  .is_floating_point = ARMIsFloatingPoint,
  .is_conditional_branch = ARMIsConditionalBranch,
  .is_fixed_register = ARMIsFixedRegister,
  .is_const = ARMIsConst,
  .is_symbol = ARMIsSymbol,
  .is_expression = ARMIsExpression,
  .is_table_entry = ARMIsJumpableEntry,
  .get_branch_target = ARMGetBranchTarget,
};

void ARMGeneratorInit(ARMGenerator* g, Generator* gen) {
  TargetGeneratorInit(&g->base, gen, &virtuals);

  g->num_int_arg_regs = 0;
  g->num_fp_arg_regs = 0;
  g->num_int_reg_vars = 0;
  g->num_fp_reg_vars = 0;
  g->struct_return_reg = -1;
  g->not_leaf = false;
  g->has_stack_args = false;
  g->uses_dynamic_stack = false;
  g->saved_arg_size = 0;
  g->zero = NULL;
  g->tmp = NULL;
  g->lsl = NULL;

  memset(g->int_argument_registers, 0, sizeof(g->int_argument_registers));
  memset(g->fp_argument_registers, 0, sizeof(g->fp_argument_registers));
  VectorInit(&g->var_regs);
  VectorInit(&g->saved_regs);
  VectorInit(&g->offsets);

  MapInitForInt64Keys(&g->conditions);
  ARMRegisterAllocatorInit(&g->register_allocator, g);
}

ARMGenerator* NewARMGenerator(Generator* gen) {
  ARMGenerator* g = malloc(sizeof(ARMGenerator));
  ARMGeneratorInit(g, gen);
  return g;
}

void ARMGeneratorDestruct(ARMGenerator* g) {
  TargetGeneratorDestruct(&g->base);
  VectorDestructWithContents(&g->var_regs, NULL, /*free_element=*/true);
  VectorDestructWithContents(&g->saved_regs, NULL, /*free_element=*/true);
  VectorDestructWithContents(&g->offsets, NULL, /*free_element=*/true);
  MapDestruct(&g->conditions);
  ARMRegisterAllocatorDestruct(&g->register_allocator);
}

void ARMGeneratorDelete(ARMGenerator* g) {
  ARMGeneratorDestruct(g);
  free(g);
}

static SavedArgumentRegister* NewSavedArgumentRegister(int reg_num,
                                                       int base_reg_num,
                                                       int offset,
                                                       bool is_fp) {
  SavedArgumentRegister* reg = malloc(sizeof(SavedArgumentRegister));
  reg->base_reg_num = base_reg_num;
  reg->reg_num = reg_num;
  reg->offset = offset;
  reg->is_fp = is_fp;
  reg->is_double = false;
  return reg;
}

static COMPILER_UNUSED void SavedArgumentRegisterDelete(SavedArgumentRegister* reg) {
  free(reg);
}

static int AllocateSavedArgumentHome(ARMGenerator* g, int size, int align) {
  if (align > 1) {
    g->saved_arg_size = (g->saved_arg_size + align - 1) & ~(align - 1);
  }
  int offset = -ARM_STACK_FRAME_HEADER_SIZE - g->saved_arg_size - size;
  g->saved_arg_size += size;
  return offset;
}

// Some static utility functions that map to generic target functions.
static TargetInstruction* NewInstruction1(ARMOpcode opcode,
                                          TargetInstruction* op1) {
  return TargetNewInstruction1((TargetOpcode)opcode, op1);
}

static TargetInstruction* NewInstruction2(ARMOpcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2) {
  return TargetNewInstruction2((TargetOpcode)opcode, op1, op2);
}

static COMPILER_UNUSED TargetInstruction* NewInstruction3(ARMOpcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2,
                                          TargetInstruction* op3) {
  return TargetNewInstruction3((TargetOpcode)opcode, op1, op2, op3);
}

static TargetInstruction* NewInstruction4(ARMOpcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2,
                                          TargetInstruction* op3,
                                          TargetInstruction* op4) {
  return TargetNewInstruction4((TargetOpcode)opcode, op1, op2, op3, op4);
}

static TargetInstruction* Emit(ARMGenerator* g, TargetInstruction* inst) {
  return TargetEmit(&g->base, inst);
}

static COMPILER_UNUSED TargetInstruction* EmitBefore(ARMGenerator* g, TargetInstruction* inst,
                                     TargetInstruction* pos) {
  return TargetEmitBefore(&g->base, inst, pos);
}

static COMPILER_UNUSED TargetInstruction* EmitAfter(ARMGenerator* g, TargetInstruction* inst,
                                    TargetInstruction* pos) {
  return TargetEmitAfter(&g->base, inst, pos);
}

static COMPILER_UNUSED TargetInstruction* EmitConstant(ARMGenerator* g, TargetInstruction* c) {
  return TargetEmitConstant(&g->base, c);
}

static COMPILER_UNUSED TargetInstruction* EmitSymbol(ARMGenerator* g, TargetInstruction* c) {
  return TargetEmitSymbol(&g->base, c);
}

static COMPILER_UNUSED TargetInstruction* FramePointer(ARMGenerator* g) {
  return SetInstructionSize(TargetFramePointer(&g->base), kSize32Bit);
}

static COMPILER_UNUSED TargetInstruction* StackPointer(ARMGenerator* g) {
  return SetInstructionSize(TargetStackPointer(&g->base), kSize32Bit);
}

static TargetInstruction* GetLoweredNode(IRNode* node) {
  return TargetGetLoweredNode(node);
}

static TargetInstruction* SetLoweredNode(IRNode* node,
                                         TargetInstruction* inst) {
  return TargetSetLoweredNode(node, inst);
}

// ===== 64-bit integer (register-pair) support =====
// On 32-bit ARM a 64-bit integer (long long / unsigned long long) is held in a
// pair of 32-bit registers.  Each "wide" IR node is lowered into two
// instructions: the low half (stored normally via SetLoweredNode, i.e. in
// node->data.ptr) and the high half (stashed in the otherwise-unused
// node->data.lvalue).  Consumers that only need the low 32 bits (truncation to
// int, etc.) transparently get the low half through the normal lowering path.
static bool TypeIsWideInt(TypeRecord* t) {
  return t != NULL && TypeIsIntegral(t) && !TypeIsFloatingPoint(t) &&
         !TypeIsStructOrUnion(t) && !TypeIsArray(t) && !TypeIsPointer(t) &&
         t->size == 8;
}

static bool NodeIsWideInt(IRNode* node) {
  return node != NULL && TypeIsWideInt(node->type);
}

static void SetLoweredHi(IRNode* node, TargetInstruction* hi) {
  node->data.lvalue = (int64_t)(intptr_t)hi;
}

static TargetInstruction* GetLoweredHi(IRNode* node) {
  return (TargetInstruction*)(intptr_t)node->data.lvalue;
}

static TargetInstruction* GetIntConstant(ARMGenerator* g, IRNode* node,
                                         TargetType type, int64_t value) {
  return SetInstructionSize(TargetGetIntConstant(&g->base, node, type, value),
                            type == kTargetType64Bit ? kSize64Bit : kSize32Bit);
}

static COMPILER_UNUSED TargetInstruction* GetFloatingPointConstant(ARMGenerator* g,
                                                   IRNode* node,
                                                   TargetType type,
                                                   double value) {
  return TargetGetFloatingPointConstant(&g->base, node, type, value);
}

static TargetInstruction* GetSymbol(ARMGenerator* g, IRNode* node,
                                    Symbol* symbol) {
  return TargetGetSymbol(&g->base, node, symbol);
}

static TargetInstruction* NewInstruction(ARMOpcode opcode) {
  return TargetNewInstruction((TargetOpcode)opcode);
}

static TargetInstruction* Condition(ARMGenerator* g, ARMOpcode cond, int size);
static TargetInstruction* EmitBranch(ARMGenerator* g, ARMOpcode cond,
                                     IRNode* target_node) {
  TargetInstruction* bra = Emit(g, NewInstruction1(ARM_OP(b), Condition(g, cond, 0)));
  if (target_node->data.ptr == NULL) {
    VectorAppend(&g->base.fixups, NewBranchFixup(bra, target_node, 1));
  } else {
    bra->operand[1] = target_node->data.ptr;
    TargetAddUser(target_node->data.ptr, bra);
  }
  return bra;
}

static COMPILER_UNUSED TargetInstruction* EmitLabelReference(ARMGenerator* g,
                                             TargetInstruction* label,
                                             IRNode* target_node) {
  Emit(g, label);
  if (target_node->data.ptr == NULL) {
    VectorAppend(&g->base.fixups, NewBranchFixup(label, target_node, 1));
  } else {
    label->operand[1] = target_node->data.ptr;
    TargetAddUser(label, target_node->data.ptr);
  }
  return label;
}

static TargetInstruction* movi(ARMGenerator* g, int size,
                               TargetInstruction* src);

static TargetInstruction* ZeroReg(ARMGenerator* g) {
  // ARM (unlike AArch64) has no hardware zero register, so a register holding
  // zero must be materialized with `mov rd, #0`.  We must emit a *fresh*
  // materialization at every use rather than caching a single instruction:
  // a cached zero value would have one long live range spanning whole basic
  // blocks and `bl` calls.  Since the arg registers (r0-r3) it often lands in
  // are caller-saved, that range gets clobbered, and later uses would read a
  // stale (non-zero) register.  A short-lived per-use `mov #0` lets the
  // allocator place each one correctly.
  return movi(g, kSize32Bit, GetIntConstant(g, NULL, kTargetType32Bit, 0));
}

static TargetInstruction* ZeroImm(ARMGenerator* g) {
  return GetIntConstant(g, NULL, kTargetType32Bit, 0);
}

static TargetInstruction* Tmp(ARMGenerator* g) {
  if (g->tmp == NULL) {
    g->tmp = Emit(g, NewInstruction(ARM_OP(r9)));
  }
  return g->tmp;
}

static TargetInstruction* lsl(ARMGenerator* g) {
  if (g->lsl == NULL) {
    g->lsl = Emit(g, NewInstruction(ARM_OP(oplsl)));
  }
  return g->lsl;
}


static TargetInstruction* movi(ARMGenerator* g, int size, TargetInstruction* src) {
  TargetInstruction* inst = Emit(g, NewInstruction1(ARM_OP(mov), src));
  SetInstructionSize(inst, size);
  return inst;
}

static TargetInstruction* Condition(ARMGenerator* g, ARMOpcode cond, int size) {
  MapKeyType k = {.w = ((int64_t)cond << 1) | (size & 1)};
  TargetInstruction* inst = MapFind(&g->conditions, k);
  if (inst != NULL) {
    return inst;
  }
  inst =  EmitConstant(g, NewInstruction(cond));
  MapKeyValue kv = {.key = k, .value.p = inst};
  MapInsert(&g->conditions, kv);
  return inst;
}

static TargetInstruction* IntArgumentRegister(ARMGenerator* g, int argnum) {
  if (g->int_argument_registers[argnum] == NULL) {
    // Allocate instruction for argument register and emit it.  The argument
    // registers have to be contiguous in value from r0..r3.
    g->int_argument_registers[argnum] =
        EmitSymbol(g, NewInstruction(ARM_OP(r0) + argnum));
  }
  return g->int_argument_registers[argnum];
}


static TargetInstruction* FloatingPointArgumentRegister(ARMGenerator* g,
                                                        int argnum) {
  if (g->fp_argument_registers[argnum] == NULL) {
    // Allocate instruction for argument register and emit it.  The argument
    // registers have to be contiguous in value from a0..a7.
    g->fp_argument_registers[argnum] =
        EmitSymbol(g, NewInstruction(ARM_OP(d0) + argnum));
  }
  return g->fp_argument_registers[argnum];
}

static TargetInstruction* IntVariableRegister(ARMGenerator* g, int varnum, Symbol* sym) {
  for (size_t i = 0; i < g->var_regs.length; i++) {
    RegisterVariable* var = g->var_regs.value.p[i];
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
  VectorAppend(&g->var_regs, var);
  return EmitSymbol(g, var->inst);
}


static TargetInstruction* FloatingPointVariableRegister(ARMGenerator* g, int varnum, Symbol* sym) {
  for (size_t i = 0; i < g->var_regs.length; i++) {
    RegisterVariable* var = g->var_regs.value.p[i];
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
  VectorAppend(&g->var_regs, var);
  return EmitSymbol(g, var->inst);
}

// Add immediate to the src.  If it fits in 12 bits we can use an addi
// instruction, otherwise load the immediate and use an add instruction.
static TargetInstruction* AddImmediate(ARMGenerator* g, TargetInstruction* src,
                                       int64_t immed) {
  int64_t imm = immed;
  if (immed < 0) {
    imm = -immed;
  }
  TargetInstruction* immed_inst =
      GetIntConstant(g, NULL, kTargetType32Bit, immed);
  if (imm <= 0x7ff) {
    return Emit(g, CopyInstructionSize(NewInstruction2(ARM_OP(add), src, immed_inst), 0));
  }
  TargetInstruction* movi = Emit(g, CopyInstructionSize(NewInstruction1(ARM_OP(mov), immed_inst), 0));
  return Emit(g, CopyInstructionSize(NewInstruction2(ARM_OP(add), src, movi), 0));
}

static TargetInstruction* SetDestOrMove(ARMGenerator* g,
                                        TargetInstruction* from,
                                        TargetInstruction* to,
                                        ARMOpcode mov_opcode) {
  bool can_set_dest = from->dest == NULL && ARMGeneratesOutput(from);

  if (can_set_dest) {
    TargetSetDest(from, to);
    return from;
  }
  TargetInstruction* move = Emit(g, CopyInstructionSize(NewInstruction1(mov_opcode, from), 0));
  move->dest = to;
  return to;
}

static TargetInstruction* SetDestOrMoveToArgReg(ARMGenerator* g,
                                                IRNode* from_node,
                                                TargetInstruction* from,
                                                TargetInstruction* to,
                                                ARMOpcode rmov_opcode) {
  // Look at all uses of from_node and make sure they are all in the
  // same basic block as from_node itself.
  bool candidate = true;
  for (size_t i = 0; i < from_node->outputs.length; i++) {
    IRNode* user = from_node->outputs.value.p[i];
    if (user->block != from_node->block) {
      candidate = false;
      break;
    }
  }
  // Only pin the producer's result directly into the (caller-saved) argument
  // register when that result is consumed solely by this argument.  IR basic
  // blocks are not split by calls, so a value with another use later in the
  // same IR block -- after an intervening call that clobbers the argument
  // registers -- would read the wrong value.  This happens when GVN merges a
  // value feeding a call argument with one feeding a later assignment (e.g. the
  // `(float)'a'` conversion shared by `floatfunc('a')` and `float f = 'a'`).
  // The argument node is usually a `pusharg` wrapper, so look through it to the
  // value actually producing the register.  With more than one user, keep the
  // value in its own register (which the allocator can preserve) and emit an
  // explicit move into the argument register instead.
  IRNode* value_node = from_node;
  if (value_node != NULL && (int)value_node->opcode == (int)IR_OP(pusharg) &&
      value_node->inputs.length > 0) {
    value_node = value_node->inputs.value.p[0];
  }
  if (value_node != NULL && value_node->outputs.length > 1) {
    candidate = false;
  }
  // Only redirect the producer's destination straight into the argument
  // register when the producer is the most recently emitted instruction.
  // Arguments are moved into their registers in reverse order, so a preceding
  // argument's value (e.g. a call result still living in r0) may already have
  // been moved out by an instruction emitted *after* this producer.  Giving an
  // older producer an argument register as its destination would place that
  // (clobbering) write before the move that consumes the register's previous
  // value; emit an explicit move at the current position instead.
  if (candidate && TargetNext(from) == NULL) {
    return SetDestOrMove(g, from, to, rmov_opcode);
  }
  // Emit an explicit register move at the current position.  Use the
  // destination-move form (NewInstruction1 + dest) rather than the two-operand
  // form: ARM's emitter prints a two-operand mov as a three-address
  // data-processing instruction ("mov rd, rn, rm"), which is wrong for a plain
  // register-to-register move.  The destination-move form prints correctly as
  // "mov <to>, <from>" (matching SetDestOrMove's own fallback).
  TargetInstruction* move = Emit(g, CopyInstructionSize(NewInstruction1(rmov_opcode, from), 0));
  move->dest = to;
  if (rmov_opcode == ARM_OP(mov)) {
    // Tag integer argument moves so the register allocator's parallel-move
    // resolver can re-sequence a contiguous run of them and break any cycles
    // (e.g. an r2<->r3 swap) through the scratch register.
    move->flags |= kARMArgMove;
  }
  return to;
}

static TargetInstruction* AddValue(ARMGenerator* g, TargetInstruction* src,
                                   TargetInstruction* value) {
  if (TargetIsConst(value)) {
    return AddImmediate(g, src, TargetIntValue(value));
  }

  if (((int)value->opcode == (int)ARM_OP(zr))) {
    return src;
  }
  return Emit(g, NewInstruction2(ARM_OP(add), src, value));
}

// Calculate the offset from the frame pointer to a local variable in the stack.
static int LocalVariableOffset(ARMGenerator* g, int32_t var_offset) {
  return var_offset - g->base.stack_frame_size -
      ARM_STACK_FRAME_HEADER_SIZE;
}

static TargetInstruction* PagedOffsetFrom(ARMGenerator* g, TargetInstruction* src,
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
  for (size_t i = 0; i < g->offsets.length; i++) {
    Offset* f = g->offsets.value.p[i];
    if (f->page_offset == page) {
      page_inst = f->inst;
      break;
    }
  }
  if (page_inst == NULL) {
    // No page offset calculated, need to calculate one.
    page_inst =
        AddImmediate(g, src, page);
    Offset* f = malloc(sizeof(Offset));
    f->inst = page_inst;
    f->page_offset = page;
    VectorAppend(&g->offsets, f);
  }
  *page_offset = offset - page;
  return page_inst;
}

// Returns either an integer constant or an instruction to calculate an
// offset from the src.
static TargetInstruction* OffsetFrom(ARMGenerator* g, TargetInstruction* src,
                                     int32_t offset) {
  bool offset_in_range = ARMIsPossibleImmediate(offset);
  if (offset_in_range) {
    return AddImmediate(g, src, offset);
  }
  int page_offset;
  TargetInstruction* page_inst = PagedOffsetFrom(g, src, offset, &page_offset);
  if (page_offset == 0) {
    return page_inst;
  }
  return AddImmediate(g, page_inst, page_offset);
}

static TargetInstruction* LoadImmediate(ARMGenerator* g, ARMOpcode opcode,
                                          TargetInstruction* base, int32_t offset) {
  if (ARMIsPossibleImmediate(offset)) {
    return Emit(g, NewInstruction2(opcode, base,
                                    GetIntConstant(g, NULL, kTargetType32Bit, offset)));
  }
  int32_t page_offset;
  TargetInstruction* page_inst = PagedOffsetFrom(g, base, offset, &page_offset);
  return Emit(g, NewInstruction2(opcode, page_inst,
                                  GetIntConstant(g, NULL, kTargetType32Bit, page_offset)));

}

static TargetInstruction* StoreImmediate(ARMGenerator* g, ARMOpcode opcode,
                                         TargetInstruction* value, TargetInstruction* base, int32_t offset) {
  if (ARMIsPossibleImmediate(offset)) {
    return Emit(g, NewInstruction3(opcode, value, base,
                                    GetIntConstant(g, NULL, kTargetType32Bit, offset)));
  }
  int32_t page_offset;
  TargetInstruction* page_inst = PagedOffsetFrom(g, base, offset, &page_offset);
  return Emit(g, NewInstruction3(opcode, value, page_inst,
                                  GetIntConstant(g, NULL, kTargetType32Bit, page_offset)));

}

static TargetInstruction* Memcpy(ARMGenerator* g, TargetInstruction* dest_addr,
                                 TargetInstruction* src_addr, int length,
                                 int src_offset, int dest_offset, bool count_as_call) {
  if (length <= 40) {
    // Length is short, copy using sequence of ld/sd and lb/sb instructions.
    int num_bytes = length & 3;
    int num_words = length >> 2;
    TargetInstruction* result = NULL;
    for (int i = 0; i < num_words; i++, src_offset += 4, dest_offset += 4) {
      TargetInstruction* load = LoadImmediate(g, ARM_OP(ldr), src_addr, src_offset);
      result = StoreImmediate(g, ARM_OP(str), load, dest_addr, dest_offset);
    }
    for (int i = 0; i < num_bytes; i++, src_offset += 1, dest_offset += 1) {
      TargetInstruction* load = LoadImmediate(g, ARM_OP(ldrb), src_addr, src_offset);
      result = StoreImmediate(g, ARM_OP(strb), load, dest_addr, dest_offset);
    }
    if (count_as_call) {
      g->base.num_calls--;
    }
    return result;
  }
  // TODO: generate a loop for intermediate lengths?

  // Length in a2.
  TargetInstruction* size = Emit(
      g, NewInstruction1(ARM_OP(mov),
                          GetIntConstant(g, NULL, kTargetType32Bit, length)));
  TargetInstruction* arg2 = SetDestOrMove(g, size, IntArgumentRegister(g, 2),
                                           ARM_OP(mov));
  // Source in a1.
  if (src_offset != 0) {
    src_addr = OffsetFrom(g, src_addr, src_offset);
  }
  TargetInstruction* arg1 = SetDestOrMove(g, src_addr, IntArgumentRegister(g, 1),
                                           ARM_OP(mov));

  // Dest in a0.
  if (dest_offset != 0) {
    dest_addr = OffsetFrom(g, dest_addr, dest_offset);
  }
  TargetInstruction* arg0 = SetDestOrMove(g, dest_addr, IntArgumentRegister(g, 0),
                                          ARM_OP(mov));

  // We need to keep the arguments alive until the point of the call.  This
  // is done using a ARM_OP(regarg) instruction sequence.  See BuildArgList for
  // details on the regarg instruction.
  TargetInstruction* regarg =
      Emit(g, NewInstruction2(ARM_OP(regarg), NULL, arg2));
  regarg = Emit(g, NewInstruction2(ARM_OP(regarg), regarg, arg1));
  regarg = Emit(g, NewInstruction2(ARM_OP(regarg), regarg, arg0));

  TargetInstruction* memcpy = GetSymbol(g, NULL, g->base.memcpy);
  return Emit(g, NewInstruction2(ARM_OP(bl), memcpy, regarg));
}

static TargetInstruction* Memzero(ARMGenerator* g, TargetInstruction* dest_addr,
                                  int length, int offset) {
  if (length <= 40) {
    // Length is short, zero it inline with a sequence of word (str) and byte
    // (strb) stores.  ARM is a 32-bit machine, so a word is 4 bytes: stride by
    // 4 and handle the trailing 0-3 bytes individually.  (A previous 8-byte
    // stride left every other word uninitialized.)
    int num_bytes = length & 3;
    int num_words = length >> 2;
    TargetInstruction* result = NULL;
    for (int i = 0; i < num_words; i++, offset += 4) {
      result = StoreImmediate(g, ARM_OP(str), ZeroReg(g), dest_addr, offset);
    }
    for (int i = 0; i < num_bytes; i++, offset += 1) {
      result = StoreImmediate(g, ARM_OP(strb), ZeroReg(g), dest_addr, offset);
    }
    g->base.num_calls--;
    return result;
  }

  // Third parameter to memset is the length.
  TargetInstruction* size = Emit(
      g, NewInstruction1(ARM_OP(mov),
                          GetIntConstant(g, NULL, kTargetType32Bit, length)));
  TargetInstruction* arg2 = SetDestOrMove(g, size, IntArgumentRegister(g, 2), ARM_OP(mov));

  // Second arg is zero.
  TargetInstruction* arg1 = Emit(
      g, NewInstruction1(ARM_OP(mov), ZeroReg(g)));
  arg1->dest = IntArgumentRegister(g, 1);

  // First arg is the address.  Apply the variable's frame-relative offset here:
  // the short (inline store) path above folds `offset` into each store, but on
  // this memset-call path it must be added to the base register, otherwise the
  // zero-fill targets the bare frame pointer instead of the variable's slot.
  TargetInstruction* base = dest_addr;
  if (offset != 0) {
    base = AddImmediate(g, dest_addr, offset);
  }
  TargetInstruction* arg0 = SetDestOrMove(g, base, IntArgumentRegister(g, 0), ARM_OP(mov));

  // We need to keep the arguments alive until the point of the call.  This
  // is done using a ARM_OP(regarg) instruction sequence.  See BuildArgList for
  // details on the regarg instruction.
  TargetInstruction* regarg =
      Emit(g, NewInstruction2(ARM_OP(regarg), NULL, arg2));
  regarg = Emit(g, NewInstruction2(ARM_OP(regarg), regarg, arg1));
  regarg = Emit(g, NewInstruction2(ARM_OP(regarg), regarg, arg0));

  TargetInstruction* memset = GetSymbol(g, NULL, g->base.memset);
  return Emit(g, NewInstruction2(ARM_OP(bl), memset, regarg));
}

static ARMOpcode IR2RV(IROpcode op, bool is_unsigned) {
  switch (op) {
    case IR_OP(addi):
      return ARM_OP(add);
    case IR_OP(addf):
      return ARM_OP(fadd);
    case IR_OP(addd):
      return ARM_OP(fadd);
    case IR_OP(adda):
      return ARM_OP(add);

    case IR_OP(subi):
      return ARM_OP(sub);
    case IR_OP(subf):
      return ARM_OP(fsub);
    case IR_OP(subd):
      return ARM_OP(fsub);
    case IR_OP(suba):
      return ARM_OP(sub);

    case IR_OP(muli):
      return ARM_OP(mul);
    case IR_OP(mulf):
      return ARM_OP(fmul);
    case IR_OP(muld):
      return ARM_OP(fmul);

    case IR_OP(divi):
      return is_unsigned ? ARM_OP(udiv) : ARM_OP(sdiv);
    case IR_OP(divf):
      return ARM_OP(fdiv);
    case IR_OP(divd):
      return ARM_OP(fdiv);

    case IR_OP(modi):
      return is_unsigned ? ARM_OP(umod) : ARM_OP(smod);

    case IR_OP(lsri):
      return ARM_OP(lsr);
    case IR_OP(asri):
      return ARM_OP(asr);
    case IR_OP(lsli):
      return ARM_OP(lsl);

    case IR_OP(ori):
      return ARM_OP(orr);
    case IR_OP(andi):
      return ARM_OP(and);
    case IR_OP(xori):
      return ARM_OP(eor);

    case IR_OP(noti):
      return ARM_OP(mvn);
    case IR_OP(nota):
      return ARM_OP(mvn);
    case IR_OP(onescomp):
      return ARM_OP(mvn);
    case IR_OP(negi):
      return ARM_OP(neg);
    case IR_OP(negf):
      return ARM_OP(fneg);
    case IR_OP(negd):
      return ARM_OP(fneg);

    case IR_OP(i2f):
      return ARM_OP(scvtf);
    case IR_OP(i2d):
      return ARM_OP(scvtf);
    case IR_OP(f2d):
      return ARM_OP(fcvtsd);
    case IR_OP(d2f):
      return ARM_OP(fcvtds);
    case IR_OP(f2i):
      return ARM_OP(fcvtnu);
    case IR_OP(d2i):
      return ARM_OP(fcvtnu);

    case IR_OP(movi):
      return ARM_OP(mov);
    case IR_OP(movf):
      return ARM_OP(fmov);
    case IR_OP(movd):
      return ARM_OP(fmov);
    case IR_OP(mova):
      return ARM_OP(mov);
    case IR_OP(tmp):
      return ARM_OP(tmp);
    default:
      assert(false);
      return 0;
  }
}

static bool UseRegisterForVariable(ARMGenerator* g, IRNode* var_node) {
  if (OptLevel0()) {
    // When not optimizing, all variables are on the stack.
    return false;
  }
  // A 64-bit integer needs a register pair; the simple single-register variable
  // machinery can't represent that, so keep wide integers on the stack and
  // access them with load64/store64.
  if (TypeIsWideInt(var_node->type)) {
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

// Static variables have an address calculated by the linker.  Load the
// address into a register using movw/movt (ARM32 has no adrp).
static TargetInstruction* EmitAddressOfSymbol(ARMGenerator* g,
                                              TargetInstruction* sym,
                                              int size) {
  TargetInstruction* movw =
      Emit(g, SetInstructionSize(NewInstruction1(ARM_OP(movw), sym), size));
  movw->flags |= ARM_LO_RELOC;
  TargetInstruction* movt =
      Emit(g, SetInstructionSize(NewInstruction2(ARM_OP(movt), movw, sym), size));
  movt->flags |= ARM_HI_RELOC;
  // movt modifies the high half of the register written by movw in place, so
  // both instructions must use the *same* physical register.  Route movw's
  // result into movt's register (the register allocator processes movw first
  // and will allocate movt, which inherits its register from whatever consumes
  // the address, e.g. an argument register).  Without this the pair can be
  // split across two registers and the materialized address is corrupted.
  movw->dest = movt;
  return movt;
}

static TargetInstruction* LoadStaticVariableAddress(ARMGenerator* g,
                                                    IRNode* node) {
  return EmitAddressOfSymbol(g, GetLoweredNode(node), kSize32Bit);
}

static struct {
  bool (*type_func)(TypeRecord*);
  ARMOpcode load;
} load_opcodes[] = {
    {TypeIsInt, ARM_OP(ldr)},
    {TypeIsShort, ARM_OP(ldrh)},
    {TypeIsChar, ARM_OP(ldrb)},
    {TypeIsLong, ARM_OP(ldr)},
    {TypeIsLongLong, ARM_OP(ldr)},
    {TypeIsUnsignedInt, ARM_OP(ldur)},
    {TypeIsUnsignedShort, ARM_OP(ldurh)},
    {TypeIsUnsignedChar, ARM_OP(ldurb)},
    {TypeIsFloat, ARM_OP(fldr)},
    {TypeIsDouble, ARM_OP(fldr)},
    {TypeIsLongDouble, ARM_OP(fldr)},
    {TypeIsBool, ARM_OP(ldrb)},
    {TypeIsPointerOrArray, ARM_OP(ldr)},
    {TypeIsFunction, ARM_OP(ldr)},
    {NULL, 0},
};

static COMPILER_UNUSED TargetInstruction* LoadVariableValue(ARMGenerator* g, IRNode* node,
                                            TargetInstruction* addr,
                                            TargetInstruction* offset) {
  ARMOpcode opcode = ARM_OP(ldr);
  for (size_t i = 0; load_opcodes[i].type_func != NULL; i++) {
    if (load_opcodes[i].type_func(node->type)) {
      opcode = load_opcodes[i].load;
      break;
    }
  }
  assert(opcode != 0);
  return Emit(g, NewInstruction2(opcode, addr, offset));
}

static TargetInstruction* MoveImmediate(ARMGenerator* g, TargetInstruction* src, int size) {
  uint64_t value = TargetIntValue(src);
  TargetType type = size == kSize64Bit ? kTargetType64Bit : kTargetType32Bit;
  return movi(g, size, GetIntConstant(g, NULL, type, value));
}

// Materialize a value into a register.  This loads a constant into a register
// or returns the instruction associated with the node if it's
// already in a register.
static TargetInstruction* Materialize1(ARMGenerator* g, IRNode* node) {
  if (IRIsConst(node)) {
    switch (node->opcode) {
      case IR_OP(const8):
      case IR_OP(const16):
      case IR_OP(const32):
         if (IRIsZero(node)) {
          return ZeroReg(g);
        } else {
          return MoveImmediate(g, GetLoweredNode(node), kSize32Bit);
        }
        
      case IR_OP(const64):
      case IR_OP(consta):
        if (IRIsZero(node)) {
         return ZeroReg(g);
       } else {
         return MoveImmediate(g, GetLoweredNode(node), kSize64Bit);
       }
        
      case IR_OP(constf): {
        // The constant's fvalue field is always in double precision.
        double dvalue = ((IRConstant*)node)->value.fvalue;

        // Convert to single precision.
        float fvalue = (float)dvalue;
        int32_t bits = *(int32_t*)(&fvalue);
        TargetInstruction* c;
        if (bits == 0) {
          c = ZeroReg(g);
        } else {
          c = MoveImmediate(g, GetIntConstant(g, NULL, kTargetType32Bit, bits), kSize32Bit);
        }
        return Emit(g, NewInstruction1(ARM_OP(fcvt), c));
      }
      case IR_OP(constd): {
        // A double has no single-register representation on AArch32.  Build the
        // two 32-bit halves in core registers and move them into a
        // double-precision VFP register with `vmov Dn, Rlo, Rhi` (the 2-operand
        // form of fcvt).
        double value = ((IRConstant*)node)->value.fvalue;
        uint64_t bits;
        memcpy(&bits, &value, sizeof(bits));
        int32_t lo_bits = (int32_t)(uint32_t)bits;
        int32_t hi_bits = (int32_t)(uint32_t)(bits >> 32);
        TargetInstruction* lo = MoveImmediate(
            g, GetIntConstant(g, NULL, kTargetType32Bit, lo_bits), kSize32Bit);
        TargetInstruction* hi = MoveImmediate(
            g, GetIntConstant(g, NULL, kTargetType32Bit, hi_bits), kSize32Bit);
        TargetInstruction* d = NewInstruction2(ARM_OP(fcvt), lo, hi);
        SetInstructionSize(d, kSize64Bit);
        return Emit(g, d);
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
      if (ARM_IS_REG_VAR(var_offset)) {
        // Variable is in a register.
        int var_num = var_offset & ~ARM_REG_VAR;
        if (TypeIsFloatingPoint(node->type)) {
          return FloatingPointVariableRegister(g, var_num, var->symbol);
        } else {
          return IntVariableRegister(g, var_num, var->symbol);
        }
      }

      // Auto variable is in the stack frame.  These are accessed through
      // the frame pointer with a negative offset.
      TargetInstruction* addr = FramePointer(g);
      return OffsetFrom(g, addr, LocalVariableOffset(g, var_offset));
    }
  } else if (IRIsArgument(node)) {
    // TODO: structs passed by reference.
    int32_t var_offset = node->data.ivalue;
    if (ARM_IS_REG_VAR(var_offset)) {
      // Argument is in a register.
      IRVariable* var = (IRVariable*)node;
      int var_num = var_offset & ~ARM_REG_VAR;
      if (TypeIsFloatingPoint(node->type)) {
        return FloatingPointVariableRegister(g, var_num, var->symbol);
      } else {
        return IntVariableRegister(g, var_num, var->symbol);
      }
    } else {
      // Argument is on the stack.
      TargetInstruction* addr = FramePointer(g);
      int32_t var_offset = node->data.ivalue;
      return OffsetFrom(g, addr, var_offset);
    }
  } else if (IRIsStaticVariable(node)) {
    // The address of static variables need to be moved into a register.

    return LoadStaticVariableAddress(g, node);
  }

  // Node is not a variable, is must be an already-lowered expression.
  return GetLoweredNode(node);
}

static TargetInstruction* Materialize(ARMGenerator* g, IRNode* node) {
  TargetInstruction* inst = Materialize1(g, node);
  return CopyOrSetInstructionSize(node, inst);
}

// Emit a 32-bit data-processing instruction (its result is its own freshly
// allocated register).  Used to build the individual halves of 64-bit ops.
static TargetInstruction* EmitWide32(ARMGenerator* g, TargetInstruction* inst) {
  return Emit(g, SetInstructionSize(inst, kSize32Bit));
}

static TargetInstruction* Const32(ARMGenerator* g, int32_t value) {
  if (value == 0) {
    return ZeroReg(g);
  }
  return MoveImmediate(g, GetIntConstant(g, NULL, kTargetType32Bit, value),
                       kSize32Bit);
}

// Produce the low and high 32-bit halves of a wide (64-bit) operand.  Constants
// are materialized on demand; other wide nodes have already been lowered to a
// (low, high) pair by LowerWideExpression and friends.
static void MaterializeWide(ARMGenerator* g, IRNode* node,
                            TargetInstruction** lo, TargetInstruction** hi) {
  if (IRIsConst(node)) {
    uint64_t v = (uint64_t)((IRConstant*)node)->value.ivalue;
    *lo = Const32(g, (int32_t)(uint32_t)v);
    *hi = Const32(g, (int32_t)(uint32_t)(v >> 32));
    return;
  }
  *lo = GetLoweredNode(node);
  *hi = GetLoweredHi(node);
  // A non-wide value used in a wide context (e.g. an int promoted implicitly)
  // has no high half; treat it as zero-extended.  This should be rare because
  // the front end inserts explicit extensions, but guard against a NULL.
  if (*hi == NULL) {
    *hi = ZeroReg(g);
  }
}

static TargetInstruction* WideMov(ARMGenerator* g, TargetInstruction* src) {
  return EmitWide32(g, NewInstruction1(ARM_OP(mov), src));
}

// Variable (runtime) 64-bit shift of the pair {ahi:alo} by the amount in
// register r (assumed 0..63).  Implemented with ARM register-controlled shifts,
// which produce 0 when the count is >= 32 (LSL/LSR) or the sign (ASR), so the
// cross-word terms can be OR-combined without conditional execution.
static void WideVariableShift(ARMGenerator* g, IROpcode op,
                              TargetInstruction* alo, TargetInstruction* ahi,
                              TargetInstruction* r, TargetInstruction** lo,
                              TargetInstruction** hi) {
  TargetInstruction* c32 = Const32(g, 32);
  TargetInstruction* n32 =
      EmitWide32(g, NewInstruction2(ARM_OP(sub), c32, r));  // 32 - r
  TargetInstruction* nm32 =
      EmitWide32(g, NewInstruction2(ARM_OP(sub), r, Const32(g, 32)));  // r - 32
  if (op == IR_OP(lsli)) {
    *lo = EmitWide32(g, NewInstruction2(ARM_OP(lsl), alo, r));
    TargetInstruction* t1 = EmitWide32(g, NewInstruction2(ARM_OP(lsl), ahi, r));
    TargetInstruction* t2 = EmitWide32(g, NewInstruction2(ARM_OP(lsr), alo, n32));
    TargetInstruction* t3 = EmitWide32(g, NewInstruction2(ARM_OP(lsl), alo, nm32));
    TargetInstruction* h = EmitWide32(g, NewInstruction2(ARM_OP(orr), t1, t2));
    *hi = EmitWide32(g, NewInstruction2(ARM_OP(orr), h, t3));
  } else if (op == IR_OP(lsri)) {
    *hi = EmitWide32(g, NewInstruction2(ARM_OP(lsr), ahi, r));
    TargetInstruction* t1 = EmitWide32(g, NewInstruction2(ARM_OP(lsr), alo, r));
    TargetInstruction* t2 = EmitWide32(g, NewInstruction2(ARM_OP(lsl), ahi, n32));
    TargetInstruction* t3 = EmitWide32(g, NewInstruction2(ARM_OP(lsr), ahi, nm32));
    TargetInstruction* l = EmitWide32(g, NewInstruction2(ARM_OP(orr), t1, t2));
    *lo = EmitWide32(g, NewInstruction2(ARM_OP(orr), l, t3));
  } else {  // asri (arithmetic right): the high->low cross term needs a select.
    *hi = EmitWide32(g, NewInstruction2(ARM_OP(asr), ahi, r));
    TargetInstruction* t1 = EmitWide32(g, NewInstruction2(ARM_OP(lsr), alo, r));
    TargetInstruction* t2 = EmitWide32(g, NewInstruction2(ARM_OP(lsl), ahi, n32));
    TargetInstruction* lo_lt =
        EmitWide32(g, NewInstruction2(ARM_OP(orr), t1, t2));  // valid r<32
    TargetInstruction* lo_ge =
        EmitWide32(g, NewInstruction2(ARM_OP(asr), ahi, nm32));  // valid r>=32
    // mask = 0xFFFFFFFF iff r >= 32, else 0.  (r-32) is negative for r<32, so
    // ~((r-32) >> 31) gives the desired mask.
    TargetInstruction* sign =
        EmitWide32(g, NewInstruction2(ARM_OP(asr), nm32, Const32(g, 31)));
    TargetInstruction* mask = EmitWide32(g, NewInstruction1(ARM_OP(mvn), sign));
    TargetInstruction* a = EmitWide32(g, NewInstruction2(ARM_OP(and), lo_ge, mask));
    TargetInstruction* b =
        EmitWide32(g, NewInstruction2(ARM_OP(bic), lo_lt, mask));  // lo_lt & ~mask
    *lo = EmitWide32(g, NewInstruction2(ARM_OP(orr), a, b));
  }
}

// 32-bit ARM has no hardware 64-bit multiply/divide/modulo, so these are
// implemented by calling the usual libgcc-style runtime helpers.  Look up (and
// cache) an external Symbol for the helper by name.
static Symbol* GetLibcallSymbol(const char* name) {
  static struct {
    const char* name;
    Symbol* sym;
  } cache[8];
  static int count = 0;
  for (int i = 0; i < count; i++) {
    if (strcmp(cache[i].name, name) == 0) {
      return cache[i].sym;
    }
  }
  TypeRecord* base = NewTypeRecord(kTypeInt, kQualPlain);
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.unknown_args = true;
  TypeRecordChain(func, base);
  Symbol* sym = NewSymbol(name, func, STO(extern));
  assert(count < (int)(sizeof(cache) / sizeof(cache[0])));
  cache[count].name = name;
  cache[count].sym = sym;
  count++;
  return sym;
}

// Emit a call to a runtime helper taking two 64-bit integer arguments and
// returning a 64-bit integer.  Per AAPCS the first 64-bit argument is passed in
// r0:r1 and the second in r2:r3, and the result comes back in r0:r1.
static TargetInstruction* WideLibCall(ARMGenerator* g, IRNode* node,
                                      const char* name) {
  IRNode* a = node->inputs.value.p[0];
  IRNode* b = node->inputs.value.p[1];
  TargetInstruction *alo, *ahi, *blo, *bhi;
  MaterializeWide(g, a, &alo, &ahi);
  MaterializeWide(g, b, &blo, &bhi);

  TargetInstruction* arg0 =
      SetDestOrMove(g, alo, IntArgumentRegister(g, 0), ARM_OP(mov));
  TargetInstruction* arg1 =
      SetDestOrMove(g, ahi, IntArgumentRegister(g, 1), ARM_OP(mov));
  TargetInstruction* arg2 =
      SetDestOrMove(g, blo, IntArgumentRegister(g, 2), ARM_OP(mov));
  TargetInstruction* arg3 =
      SetDestOrMove(g, bhi, IntArgumentRegister(g, 3), ARM_OP(mov));

  // Keep all four argument registers live up to the call.
  TargetInstruction* regarg =
      Emit(g, NewInstruction2(ARM_OP(regarg), NULL, arg3));
  regarg = Emit(g, NewInstruction2(ARM_OP(regarg), regarg, arg2));
  regarg = Emit(g, NewInstruction2(ARM_OP(regarg), regarg, arg1));
  regarg = Emit(g, NewInstruction2(ARM_OP(regarg), regarg, arg0));

  TargetInstruction* sym = GetSymbol(g, NULL, GetLibcallSymbol(name));
  TargetInstruction* call = Emit(g, NewInstruction2(ARM_OP(bl), sym, regarg));
  g->not_leaf = true;
  g->base.num_calls++;

  // Result low half is r0 (the call instruction itself); capture the high half
  // from r1 immediately while it is still live.
  TargetInstruction* r1 = Emit(g, NewInstruction(ARM_OP(r1)));
  TargetInstruction* hi = Emit(g, NewInstruction1(ARM_OP(mov), r1));
  SetLoweredHi(node, hi);
  return SetLoweredNode(node, call);
}

// Lower a 64-bit integer producing IR node into a (low, high) register pair.
static TargetInstruction* LowerWideExpression(ARMGenerator* g, IRNode* node) {
  TargetInstruction* lo = NULL;
  TargetInstruction* hi = NULL;

  switch (node->opcode) {
    case IR_OP(const64):
    case IR_OP(consta):
      MaterializeWide(g, node, &lo, &hi);
      break;

    case IR_OP(movi): {
      TargetInstruction *alo, *ahi;
      MaterializeWide(g, node->inputs.value.p[0], &alo, &ahi);
      lo = WideMov(g, alo);
      hi = WideMov(g, ahi);
      break;
    }

    case IR_OP(addi): {
      TargetInstruction *alo, *ahi, *blo, *bhi;
      MaterializeWide(g, node->inputs.value.p[0], &alo, &ahi);
      MaterializeWide(g, node->inputs.value.p[1], &blo, &bhi);
      lo = EmitWide32(g, NewInstruction2(ARM_OP(adds), alo, blo));
      hi = EmitWide32(g, NewInstruction2(ARM_OP(adc), ahi, bhi));
      break;
    }

    case IR_OP(subi): {
      TargetInstruction *alo, *ahi, *blo, *bhi;
      MaterializeWide(g, node->inputs.value.p[0], &alo, &ahi);
      MaterializeWide(g, node->inputs.value.p[1], &blo, &bhi);
      lo = EmitWide32(g, NewInstruction2(ARM_OP(subs), alo, blo));
      hi = EmitWide32(g, NewInstruction2(ARM_OP(sbc), ahi, bhi));
      break;
    }

    case IR_OP(andi):
    case IR_OP(ori):
    case IR_OP(xori): {
      ARMOpcode op = node->opcode == IR_OP(andi)   ? ARM_OP(and)
                     : node->opcode == IR_OP(ori)  ? ARM_OP(orr)
                                                   : ARM_OP(eor);
      TargetInstruction *alo, *ahi, *blo, *bhi;
      MaterializeWide(g, node->inputs.value.p[0], &alo, &ahi);
      MaterializeWide(g, node->inputs.value.p[1], &blo, &bhi);
      lo = EmitWide32(g, NewInstruction2(op, alo, blo));
      hi = EmitWide32(g, NewInstruction2(op, ahi, bhi));
      break;
    }

    case IR_OP(muli):
      return WideLibCall(g, node, "__muldi3");

    case IR_OP(divi):
      return WideLibCall(
          g, node, TypeIsUnsigned(node->type) ? "__udivdi3" : "__divdi3");

    case IR_OP(modi):
      return WideLibCall(
          g, node, TypeIsUnsigned(node->type) ? "__umoddi3" : "__moddi3");

    case IR_OP(onescomp): {
      TargetInstruction *alo, *ahi;
      MaterializeWide(g, node->inputs.value.p[0], &alo, &ahi);
      lo = EmitWide32(g, NewInstruction1(ARM_OP(mvn), alo));
      hi = EmitWide32(g, NewInstruction1(ARM_OP(mvn), ahi));
      break;
    }

    case IR_OP(negi): {
      // Two's complement: -x = ~x + 1 over the 64-bit pair.
      TargetInstruction *alo, *ahi;
      MaterializeWide(g, node->inputs.value.p[0], &alo, &ahi);
      TargetInstruction* nlo = EmitWide32(g, NewInstruction1(ARM_OP(mvn), alo));
      TargetInstruction* nhi = EmitWide32(g, NewInstruction1(ARM_OP(mvn), ahi));
      lo = EmitWide32(g, NewInstruction2(ARM_OP(adds), nlo,
                                         GetIntConstant(g, NULL, kTargetType32Bit, 1)));
      hi = EmitWide32(g, NewInstruction2(ARM_OP(adc), nhi,
                                         GetIntConstant(g, NULL, kTargetType32Bit, 0)));
      break;
    }

    case IR_OP(lsli):
    case IR_OP(lsri):
    case IR_OP(asri): {
      TargetInstruction *alo, *ahi;
      MaterializeWide(g, node->inputs.value.p[0], &alo, &ahi);
      IRNode* amount = node->inputs.value.p[1];
      if (!IRIsConst(amount)) {
        TargetInstruction* r = Materialize(g, amount);
        WideVariableShift(g, node->opcode, alo, ahi, r, &lo, &hi);
        break;
      }
      int n = (int)(((IRConstant*)amount)->value.ivalue) & 63;
      ARMOpcode op = node->opcode;
      if (n == 0) {
        lo = WideMov(g, alo);
        hi = WideMov(g, ahi);
      } else if (op == IR_OP(lsli)) {
        if (n < 32) {
          TargetInstruction* hi_hi = EmitWide32(g, NewInstruction2(ARM_OP(lsl), ahi,
                                       GetIntConstant(g, NULL, kTargetType32Bit, n)));
          TargetInstruction* hi_lo = EmitWide32(g, NewInstruction2(ARM_OP(lsr), alo,
                                       GetIntConstant(g, NULL, kTargetType32Bit, 32 - n)));
          hi = EmitWide32(g, NewInstruction2(ARM_OP(orr), hi_hi, hi_lo));
          lo = EmitWide32(g, NewInstruction2(ARM_OP(lsl), alo,
                                       GetIntConstant(g, NULL, kTargetType32Bit, n)));
        } else {
          hi = (n == 32) ? WideMov(g, alo)
                         : EmitWide32(g, NewInstruction2(ARM_OP(lsl), alo,
                               GetIntConstant(g, NULL, kTargetType32Bit, n - 32)));
          lo = ZeroReg(g);
        }
      } else {  // lsri / asri (right shifts)
        ARMOpcode high_op = (op == IR_OP(asri)) ? ARM_OP(asr) : ARM_OP(lsr);
        if (n < 32) {
          TargetInstruction* lo_lo = EmitWide32(g, NewInstruction2(ARM_OP(lsr), alo,
                                       GetIntConstant(g, NULL, kTargetType32Bit, n)));
          TargetInstruction* lo_hi = EmitWide32(g, NewInstruction2(ARM_OP(lsl), ahi,
                                       GetIntConstant(g, NULL, kTargetType32Bit, 32 - n)));
          lo = EmitWide32(g, NewInstruction2(ARM_OP(orr), lo_lo, lo_hi));
          hi = EmitWide32(g, NewInstruction2(high_op, ahi,
                                       GetIntConstant(g, NULL, kTargetType32Bit, n)));
        } else {
          if (op == IR_OP(asri)) {
            lo = (n == 32) ? WideMov(g, ahi)
                           : EmitWide32(g, NewInstruction2(ARM_OP(asr), ahi,
                                 GetIntConstant(g, NULL, kTargetType32Bit, n - 32)));
            hi = EmitWide32(g, NewInstruction2(ARM_OP(asr), ahi,
                                 GetIntConstant(g, NULL, kTargetType32Bit, 31)));
          } else {
            lo = (n == 32) ? WideMov(g, ahi)
                           : EmitWide32(g, NewInstruction2(ARM_OP(lsr), ahi,
                                 GetIntConstant(g, NULL, kTargetType32Bit, n - 32)));
            hi = ZeroReg(g);
          }
        }
      }
      break;
    }

    default:
      assert(false && "unhandled wide (64-bit) integer operation");
      break;
  }

  SetLoweredHi(node, hi);
  return SetLoweredNode(node, lo);
}

static void ApplyFixups(ARMGenerator* g, IRNode* label_node) {
  TargetApplyFixups(&g->base, label_node);
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
// Since this is a 64 bit processor, that's
// 8 cycles to multiply two 64 bit numbers.  Each 1-bit in the constant
// causes the emission of a shift instruction and these need to be added
// together.  Therefore the number of instructions for a n bits is
// n + (n - 1) = 2n-1.  However for bit 0 we don't do the shift but instead
// use the input value directly.
//
// Let's assume that both a slli and an add instruction take 1 cycle.
static TargetInstruction* MultiplyByConstant(ARMGenerator* g,
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
  
  // TODO: use ARM's shifted register instructions to make this more efficient.
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
           Emit(g, CopyInstructionSize(NewInstruction2(ARM_OP(lsl), input,
                           GetIntConstant(g, NULL, kTargetType32Bit, bitpos)), 0));
      }
      if (left == NULL) {
        left = inst;
      } else if (right == NULL) {
        right = inst;
      }
      if (left != NULL && right != NULL) {
        // Add left and right together.
        inst = Emit(g, CopyInstructionSize(NewInstruction2(ARM_OP(add), left, right), 0));
        
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

static TargetInstruction* LowerModulo(ARMGenerator* g, IRNode* node,
                                      ARMOpcode div_opcode) {
  IRNode* op1 = node->inputs.value.p[0];
  IRNode* op2 = node->inputs.value.p[1];
  TargetInstruction* lhs = Materialize(g, op1);
  TargetInstruction* rhs = Materialize(g, op2);
  TargetInstruction* quotient_tmp = Emit(g, NewInstruction(ARM_OP(tmp)));
  TargetInstruction* divide = Emit(
      g, CopyInstructionSize(NewInstruction2(div_opcode, lhs, rhs), 0));
  divide->dest = quotient_tmp;
  return NewInstruction3(ARM_OP(msub), quotient_tmp, rhs, lhs);
}


static TargetInstruction* GetDestInstruction(ARMGenerator* g, IRNode* node);

static TargetInstruction* LowerExpression(ARMGenerator* g, IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }

  // 64-bit integer operations are lowered into register pairs.
  if (NodeIsWideInt(node)) {
    switch (node->opcode) {
      case IR_OP(addi):
      case IR_OP(subi):
      case IR_OP(andi):
      case IR_OP(ori):
      case IR_OP(xori):
      case IR_OP(onescomp):
      case IR_OP(negi):
      case IR_OP(lsli):
      case IR_OP(lsri):
      case IR_OP(asri):
      case IR_OP(movi):
      case IR_OP(muli):
      case IR_OP(divi):
      case IR_OP(modi):
        return LowerWideExpression(g, node);
      default:
        break;
    }
  }

  // Logical NOT (!x).  This is distinct from one's-complement (~x, onescomp):
  // the result is 1 when the operand is zero and 0 otherwise.  Materialize it
  // as "cmp x, #0 ; cset rd, eq" rather than the bitwise mvn that the IR2RV
  // table maps noti/nota onto (mvn would wrongly yield ~x).
  if (node->opcode == IR_OP(noti) || node->opcode == IR_OP(nota)) {
    IRNode* op = node->inputs.value.p[0];
    int compare_size = (op->type->size > 4) ? kSize64Bit : kSize32Bit;
    int result_size = (node->type->size > 4) ? kSize64Bit : kSize32Bit;
    Emit(g, SetInstructionSize(
                NewInstruction2(ARM_OP(cmp), Materialize(g, op),
                                GetIntConstant(g, NULL, kTargetType32Bit, 0)),
                compare_size));
    TargetInstruction* result = SetInstructionSize(
        NewInstruction1(ARM_OP(cset), Condition(g, ARM_OP(eq), result_size)),
        result_size);
    result->flags |= kARMComparisonGenerated;
    Emit(g, result);
    TargetInstruction* dest = GetDestInstruction(g, node);
    if (dest != NULL) {
      result = SetDestOrMove(g, result, dest, ARM_OP(mov));
    }
    return SetLoweredNode(node, result);
  }

  ARMOpcode opcode = IR2RV(node->opcode, TypeIsUnsigned(node->type));
  assert(node->inputs.length <= 2);
  TargetInstruction* inst = NULL;
  bool ref_counts_ok =
      false;  // True if we don't need to update operand ref counts.
  
  // Do some strength reduction if we can.
  switch (opcode) {
    default:
      // All others are handled below.
      break;
    case ARM_OP(add): {
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
        if (ARMIsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(ARM_OP(add));
          inst->operand[0] = Materialize(g, op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      } else if (IRIsConst(op1)) {
        int64_t c = ((IRConstant*)op1)->value.ivalue;
        if (ARMIsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(ARM_OP(add));
          inst->operand[0] = Materialize(g, op2);
          inst->operand[1] = GetLoweredNode(op1);
        }
      }
      break;
    }

    case ARM_OP(sub): {
      // A subtract immediate can be done using an addi with the negative of the
      // immediate.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (ARMIsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(ARM_OP(sub));
          inst->operand[0] = Materialize(g, op1);
          inst->operand[1] = GetIntConstant(g, NULL, kTargetType32Bit, c);
        }
      }
    }
      break;
    case ARM_OP(lsl):
    case ARM_OP(lsr):
    case ARM_OP(asr): {
      // There are constant shift operations.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        // TODO: what about a shift out of range?
        if (c == 0) {
          // A shift of 0 is a mv.
          inst = NewInstruction(ARM_OP(mov));
          inst->operand[0] = Materialize(g, op1);
        } else {
          switch (opcode) {
            case ARM_OP(lsl):
              opcode = ARM_OP(lsl);
              break;
            case ARM_OP(lsr):
              opcode = ARM_OP(lsr);
              break;
            case ARM_OP(asr):
              opcode = ARM_OP(asr);
              break;
            default:
              break;
          }
          inst = (TargetInstruction*)NewInstruction(opcode);
          inst->operand[0] = Materialize(g, op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      }
      break;
    }
    case ARM_OP(mul): {
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
            inst = ZeroReg(g);
          } else if (c == 1) {
            // Multiply by 1 is a mv.
            inst = NewInstruction(ARM_OP(mov));
            inst->operand[0] = Materialize(g, op2);
          } else {
            inst = MultiplyByConstant(g, op2, (IRConstant*)op1);
            if (inst == NULL) {
              break;
            }
            ref_counts_ok = true;
          }
        } else {
          int64_t c = ((IRConstant*)op2)->value.ivalue;
          if (c == 0) {
            // Multiply by zero is zero
            inst = ZeroReg(g);
          } else if (c == 1) {
            // Multiply by 1 is a mv.
            inst = NewInstruction(ARM_OP(mov));
            inst->operand[0] = Materialize(g, op1);
          } else {
            inst = MultiplyByConstant(g, op1, (IRConstant*)op2);
            if (inst == NULL) {
              break;
            }
            ref_counts_ok = true;
          }
        }
      }
      break;
    }
    case ARM_OP(udiv):
    case ARM_OP(sdiv): {
      // If we are dividing by a constant power of 2 we can use a shift.
      // TODO: other constants can be done too.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        ARMOpcode opcode =
            TypeIsUnsigned(node->type) ? ARM_OP(lsr) : ARM_OP(asr);
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        // TODO: can we give an error on division by zero here?
        if (c == 1) {
          // Division by 1 is a mv.
          inst = NewInstruction(ARM_OP(mov));
          inst->operand[0] = Materialize(g, op1);
        } else {
          if (IsPowerOf2(c) && c < 64) {
            c = Log2(c);
            inst =
                NewInstruction2(opcode, Materialize(g, op1),
                                GetIntConstant(g, NULL, kTargetType32Bit, c));
            ref_counts_ok = true;
          }
        }
      }

      break;
    }

    case ARM_OP(umod):
    case ARM_OP(smod): {
      // If we are moding an unsigned value by a constant power of 2 we can use
      // an AND. Signed modulo needs truncation toward zero, so use div/msub.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        // TODO: can we give an error on division by zero here?
        if (opcode == ARM_OP(umod) && IsPowerOf2(c)) {
          int64_t mask = c - 1;
          inst =
                 NewInstruction2(ARM_OP(and), Materialize(g, op1),
                                 GetIntConstant(g, NULL, kTargetType32Bit, mask));
          ref_counts_ok = true;
        }
      }
      if (inst == NULL) {
        inst = LowerModulo(g, node, opcode == ARM_OP(umod) ? ARM_OP(udiv) : ARM_OP(sdiv));
        ref_counts_ok = true;
      }
      break;
    }
      
    case ARM_OP(and): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 0) {
          // Anding with zero is zero.
          inst = ZeroReg(g);
        } else if (ARMIsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(ARM_OP(and));
          inst->operand[0] = Materialize(g, op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      }
      break;
    }

    case ARM_OP(orr): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 0) {
          // ORing with zero is nop.
          inst = GetLoweredNode(op1);
        } else if (ARMIsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(ARM_OP(orr));
          inst->operand[0] = Materialize(g, op1);
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
      inst->operand[i] = Materialize(g, input);
    }
  }

  // A bare temporary (e.g. the merge slot of a ?: / && / || expression) is
  // type-agnostic and defaults to an integer register.  When it holds a
  // floating-point value mark it so the register allocator picks a float
  // register instead.
  if ((ARMOpcode)inst->opcode == ARM_OP(tmp) && node->type != NULL &&
      TypeIsFloatingPoint(node->type)) {
    inst->flags |= kARMFloatValue;
  }

  CopyOrSetInstructionSize(node, inst);
  if (!ref_counts_ok) {
    TargetUpdateOperandUsers(inst);
  }
  Emit(g, inst);
  // If this expression writes its result into a destination tmp (the "-> $n"
  // annotation used to merge the arms of && / || / ?: into one location), route
  // the result there so the merged value lands in the destination's register.
  // Without this the merge temp is never written and the consumer (a later use
  // or branch) reads an unrelated register.  The comparison and logical-NOT
  // paths handle their own routing; this covers everything else (e.g. a mova
  // that materializes an address into a ?: result).
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL) {
    inst = SetDestOrMove(g, inst, dest, ARM_OP(mov));
  }
  return SetLoweredNode(node, inst);
}

// If the IR node writes its result into a destination tmp (the "-> $n"
// annotation, used to merge the results of && / || / ?: into one location),
// return the lowered instruction for that destination so the caller can route
// the result there.  Returns NULL when there is no destination.
static TargetInstruction* GetDestInstruction(ARMGenerator* g, IRNode* node) {
  if (node->dest == NULL) {
    return NULL;
  }
  if (node->dest->data.ptr == NULL) {
    LowerExpression(g, node->dest);
  }
  return GetLoweredNode(node->dest);
}

static ARMOpcode InvertCondCode(ARMOpcode c) {
  if (c == ARM_OP(eq)) return ARM_OP(ne);
  if (c == ARM_OP(ne)) return ARM_OP(eq);
  if (c == ARM_OP(lt)) return ARM_OP(ge);
  if (c == ARM_OP(ge)) return ARM_OP(lt);
  if (c == ARM_OP(lo)) return ARM_OP(hs);
  if (c == ARM_OP(hs)) return ARM_OP(lo);
  assert(false && "unexpected wide condition code");
  return c;
}

// Emit a 64-bit integer comparison of two wide operands, setting the condition
// flags.  Returns the ARM condition code to test afterwards (with cset or a
// conditional branch).  If `invert` is true the returned condition is negated
// (used for the bfalse / "branch when false" case).
static ARMOpcode WideCompareSetFlags(ARMGenerator* g, IRNode* lhs_node,
                                     IRNode* rhs_node, IROpcode opcode,
                                     bool is_unsigned, bool invert) {
  TargetInstruction *alo, *ahi, *blo, *bhi;
  MaterializeWide(g, lhs_node, &alo, &ahi);
  MaterializeWide(g, rhs_node, &blo, &bhi);

  // Categorize the comparison.
  enum { CEQ, CNE, CLT, CGE, CGT, CLE } cat;
  switch (opcode) {
    case IR_OP(cmpeqi): case IR_OP(cmpeqa): cat = CEQ; break;
    case IR_OP(cmpnei): case IR_OP(cmpnea): cat = CNE; break;
    case IR_OP(cmplti): case IR_OP(cmplta): cat = CLT; break;
    case IR_OP(cmpgei): case IR_OP(cmpgea): cat = CGE; break;
    case IR_OP(cmpgti): case IR_OP(cmpgta): cat = CGT; break;
    case IR_OP(cmplei): case IR_OP(cmplea): cat = CLE; break;
    default: assert(false && "non-integer wide comparison"); cat = CEQ; break;
  }

  ARMOpcode cond;
  if (cat == CEQ || cat == CNE) {
    TargetInstruction* t1 = EmitWide32(g, NewInstruction2(ARM_OP(eor), alo, blo));
    TargetInstruction* t2 = EmitWide32(g, NewInstruction2(ARM_OP(eor), ahi, bhi));
    EmitWide32(g, NewInstruction2(ARM_OP(orrs), t1, t2));
    cond = (cat == CEQ) ? ARM_OP(eq) : ARM_OP(ne);
  } else {
    // Ordering comparisons via subs/sbcs (64-bit subtract).  gt/le are turned
    // into swapped lt/ge so that only N/V (signed) or C (unsigned) -- which are
    // valid for the full 64-bit value after sbcs -- are needed.
    bool swap = (cat == CGT || cat == CLE);
    bool want_ge = (cat == CGE || cat == CLE);
    TargetInstruction *xlo, *xhi, *ylo, *yhi;
    if (!swap) {
      xlo = alo; xhi = ahi; ylo = blo; yhi = bhi;
    } else {
      xlo = blo; xhi = bhi; ylo = alo; yhi = ahi;
    }
    EmitWide32(g, NewInstruction2(ARM_OP(subs), xlo, ylo));
    EmitWide32(g, NewInstruction2(ARM_OP(sbcs), xhi, yhi));
    if (want_ge) {
      cond = is_unsigned ? ARM_OP(hs) : ARM_OP(ge);
    } else {
      cond = is_unsigned ? ARM_OP(lo) : ARM_OP(lt);
    }
  }
  return invert ? InvertCondCode(cond) : cond;
}

static TargetInstruction* LowerComparison(ARMGenerator* g, IRNode* node) {
  // Check if any the outputs of the node are not branches.  If all
  // the uses are branches we defer the generation of the comparison
  // to the branch.
  //
  bool is_expression = node->dest != NULL;
  for (size_t i = 0; !is_expression && i < node->outputs.length; i++) {
    IRNode* output = node->outputs.value.p[i];
    if (!IRIsConditionalBranch(output)) {
      is_expression = true;
      break;
    }
  }
  if (!is_expression) {
    return NULL;
  }
  // Size is the size of the inputs.  They will all be the same.
  IRNode* op1 = node->inputs.value.p[0];
  bool is_unsigned = TypeIsUnsigned(op1->type);

  IRNode* lhs = node->inputs.value.p[0];
  IRNode* rhs = node->inputs.value.p[1];
  int compare_size = kSize32Bit;
  if (op1->type->size > 4) {
    compare_size = kSize64Bit;
  }
  int result_size = kSize32Bit;
  if (node->type->size > 4) {
    result_size = kSize64Bit;
  }
  TargetInstruction* result = NULL;

  // 64-bit integer comparison: build the flags from the register pair.
  if (TypeIsWideInt(op1->type)) {
    ARMOpcode cond =
        WideCompareSetFlags(g, lhs, rhs, node->opcode, is_unsigned, false);
    result = SetInstructionSize(
        NewInstruction1(ARM_OP(cset), Condition(g, cond, result_size)),
        result_size);
    result->flags |= kARMComparisonGenerated;
    Emit(g, result);
    TargetInstruction* dest = GetDestInstruction(g, node);
    if (dest != NULL) {
      result = SetDestOrMove(g, result, dest, ARM_OP(mov));
    }
    return SetLoweredNode(node, result);
  }
#define CMP_SET(cond)                                                       \
  do {                                                                      \
    Emit(g, SetInstructionSize(                                             \
                NewInstruction2(TypeIsFloatingPoint(op1->type)              \
                                    ? ARM_OP(fcmp)                      \
                                    : ARM_OP(cmp),                      \
                                Materialize(g, lhs), Materialize(g, rhs)),  \
                compare_size));                                             \
    result = SetInstructionSize(                                            \
        NewInstruction1(ARM_OP(cset), Condition(g, cond, result_size)),  \
        result_size);                                                       \
  } while (0)
  switch (node->opcode) {
    case IR_OP(cmpeqi):
    case IR_OP(cmpeqa):
    case IR_OP(cmpeqf):
    case IR_OP(cmpeqd):
      CMP_SET(ARM_OP(eq));
      break;
    case IR_OP(cmpnei):
    case IR_OP(cmpnea):
    case IR_OP(cmpnef):
    case IR_OP(cmpned):
      CMP_SET(ARM_OP(ne));
      break;
    case IR_OP(cmplti):
    case IR_OP(cmplta):
      CMP_SET(is_unsigned ? ARM_OP(lo) : ARM_OP(lt));
      break;
    case IR_OP(cmpltf):
    case IR_OP(cmpltd):
      CMP_SET(ARM_OP(lt));
      break;
    case IR_OP(cmplei):
    case IR_OP(cmplea):
      CMP_SET(is_unsigned ? ARM_OP(ls) : ARM_OP(le));
      break;
    case IR_OP(cmplef):
    case IR_OP(cmpled):
      CMP_SET(ARM_OP(le));
      break;
    case IR_OP(cmpgti):
    case IR_OP(cmpgta):
      CMP_SET(is_unsigned ? ARM_OP(hi) : ARM_OP(gt));
      break;
    case IR_OP(cmpgtf):
    case IR_OP(cmpgtd):
      CMP_SET(ARM_OP(gt));
      break;
    case IR_OP(cmpgei):
    case IR_OP(cmpgea):
      CMP_SET(is_unsigned ? ARM_OP(hs) : ARM_OP(ge));
      break;
    case IR_OP(cmpgef):
    case IR_OP(cmpged):
      CMP_SET(ARM_OP(ge));
      break;
    default:
      abort();
  }
#undef CMP_SET
  // Mark result as having comparison generated.
  if (result != NULL) {
    result->flags |= kARMComparisonGenerated;
  }
  Emit(g, result);
  // If the comparison result is consumed via a destination tmp (e.g. it is an
  // operand of a && / || / ?: that is materialized into a value), route the
  // cset there so the merged result lands in the destination's register.  
  // Without this the merged value (read by a later branch or use) is never
  // written, and the consumer reads an unrelated register.
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL && result != NULL) {
    result = SetDestOrMove(g, result, dest, ARM_OP(mov));
  }
  return SetLoweredNode(node, result);
}

static void GetAddressAndOffsetFrom(ARMGenerator* g,
                                 TargetInstruction* addr,
                                 int offset,
                                 TargetInstruction** addr_inst,
                                 TargetInstruction** offset_inst) {
  if (ARMIsPossibleImmediate(offset)) {
    *addr_inst = addr;
    *offset_inst = GetIntConstant(g, NULL, kTargetType32Bit, offset);
    return;
  }
  int page_offset;
  TargetInstruction* page_inst = PagedOffsetFrom(g, addr, offset, &page_offset);
  *addr_inst = page_inst;
  *offset_inst = GetIntConstant(g, NULL, kTargetType32Bit, page_offset);
}

static bool GetRegAndOffset(ARMGenerator* g, IRNode* addr_node,
                            TargetInstruction** addr,
                            TargetInstruction** offset,
                            TargetInstruction** scale) {
  *scale = NULL;
  if (IRIsAutoVariable(addr_node)) {
    IRVariable* var = (IRVariable*)addr_node;
    if (TypeIsVLA(var->symbol->type)) {
      IRNode* vla_addr = var->symbol->value.other;
      *addr = GetLoweredNode(vla_addr);
      *offset = ZeroImm(g);
      return false;
    }
    if ((addr_node->flags & kIRNrvoMarker) != 0) {
      // NRVO variable.
      int reg = addr_node->data.ivalue & ~ARM_REG_VAR;
      *addr = IntVariableRegister(g, reg, var->symbol);
      *offset = ZeroImm(g);
     return true;
    }
    int32_t var_offset = addr_node->data.ivalue;
    if (ARM_IS_REG_VAR(var_offset)) {
      // Variable is in a register.
      int var_num = var_offset & ~ARM_REG_VAR;
      if (TypeIsFloatingPoint(addr_node->type)) {
        *addr = FloatingPointVariableRegister(g, var_num, var->symbol);
      } else {
        *addr = IntVariableRegister(g, var_num, var->symbol);
      }
      *offset = NULL;
      return false;
    }

    // Auto variable is in the stack frame.  These are accessed through
    // the frame pointer with a negative offset.
    GetAddressAndOffsetFrom(g, FramePointer(g), LocalVariableOffset(g, var_offset),
                            addr, offset);
  } else if (IRIsArgument(addr_node)) {
    int32_t var_offset = addr_node->data.ivalue;
    IRVariable* var = (IRVariable*)addr_node;
    if (ARM_IS_REG_VAR(var_offset)) {
      // Argument is in a register.
      int var_num = var_offset & ~ARM_REG_VAR;
      if (TypeIsFloatingPoint(addr_node->type)) {
        *addr = FloatingPointVariableRegister(g, var_num, var->symbol);
      } else {
        *addr = IntVariableRegister(g, var_num, var->symbol);
      }
      *offset = NULL;
      return false;
    } else {
      GetAddressAndOffsetFrom(g, FramePointer(g), var_offset,
                              addr, offset);
     }
  } else if (IRIsStaticVariable(addr_node)) {
    // The address of static variables need to be moved into a register.

    *addr = LoadStaticVariableAddress(g, addr_node);
    *offset = ZeroImm(g);
  } else {
    // All others have a calculated address.
    *addr = GetLoweredNode(addr_node);
    *offset = ZeroImm(g);
    assert(addr != NULL);
  }
  return true;
}

static TargetInstruction* Load(ARMGenerator* g, IRNode* addr_node, ARMOpcode opcode, int size) {
  TargetInstruction* addr;
  TargetInstruction* offset;
  TargetInstruction* scale;
  bool on_stack = GetRegAndOffset(g, addr_node, &addr, &offset, &scale);

  if (!on_stack) {
    return addr;
  }

  TargetInstruction* result = Emit(g, SetInstructionSize(NewInstruction2(opcode, addr, offset), size));

#if 0
  TargetInstruction* result = NULL;
  if ((ARMOpcode)((int)addr->opcode == (int)ARM_OP(add)) && TargetIsZero(offset)) {
    // If the address is calculated using an addi instruction we can
    // combine the immediate from the addi with the load.
    // The addi instruction will no longer be used and will be eliminated
    // during the optimization pass.
    TargetInstruction* src = addr->operand[0];
    TargetInstruction* immed = addr->operand[1];
    assert(src != NULL);
    assert(immed != NULL);
    assert(TargetIsConst(immed));
    result = Emit(g, SetInstructionSize(NewInstruction2(opcode, src, immed), size));
  }
  if (result == NULL) {
    result = Emit(g, SetInstructionSize(NewInstruction2(opcode, addr, offset), size));
  }
#endif
  return result;
}

// Load a 64-bit integer from memory into a (low, high) register pair using two
// 32-bit loads (avoids the alignment/consecutive-register constraints of ldrd).
static TargetInstruction* LowerWideLoad(ARMGenerator* g, IRNode* node) {
  IRNode* addr_node = node->inputs.value.p[0];
  TargetInstruction* addr;
  TargetInstruction* offset;
  TargetInstruction* scale;
  bool on_stack = GetRegAndOffset(g, addr_node, &addr, &offset, &scale);
  TargetInstruction* lo;
  TargetInstruction* hi;
  if (!on_stack) {
    // Value already sits in a register (shouldn't happen: wide variables are
    // forced to the stack).  Treat the register as the low half.
    lo = addr;
    hi = ZeroReg(g);
  } else {
    assert(TargetIsConst(offset) &&
           "wide load with non-constant offset not supported");
    int off = (int)TargetIntValue(offset);
    lo = Emit(g, SetInstructionSize(NewInstruction2(ARM_OP(ldr), addr, offset),
                                    kSize32Bit));
    TargetInstruction* offset_hi =
        GetIntConstant(g, NULL, kTargetType32Bit, off + 4);
    hi = Emit(g, SetInstructionSize(NewInstruction2(ARM_OP(ldr), addr, offset_hi),
                                    kSize32Bit));
  }
  SetLoweredHi(node, hi);
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL) {
    lo = SetDestOrMove(g, lo, dest, ARM_OP(mov));
  }
  return SetLoweredNode(node, lo);
}

static TargetInstruction* LowerLoad(ARMGenerator* g, IRNode* node) {
  ARMOpcode opcode;
  assert(node->inputs.length == 1);
  IRNode* addr_node = node->inputs.value.p[0];

  // load64 is always an 8-byte integer load (doubles use loadd); the value must
  // be materialized as a (lo, hi) register pair.
  if (node->opcode == IR_OP(load64)) {
    return LowerWideLoad(g, node);
  }

  int size = kSize32Bit;
  switch (node->opcode) {
    case IR_OP(load32):
      opcode = ARM_OP(ldr);
      break;
    case IR_OP(load8):
      opcode = ARM_OP(ldrb);
      break;
    case IR_OP(load64):
      opcode = ARM_OP(ldr);
      size = kSize64Bit;
      break;
    case IR_OP(load16):
      opcode = ARM_OP(ldrh);
      break;
    case IR_OP(loadu32):
      opcode = ARM_OP(ldur);
      break;
    case IR_OP(loadu8):
      opcode = ARM_OP(ldurb);
      break;
    case IR_OP(loadu16):
      opcode = ARM_OP(ldurh);
      break;
    case IR_OP(loadf):
      opcode = ARM_OP(fldr);
      break;
    case IR_OP(loadd):
      opcode = ARM_OP(fldr);
      size = kSize64Bit;
      break;
    case IR_OP(loada):
      opcode = ARM_OP(ldr);
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }

  TargetInstruction* inst = Load(g, addr_node, opcode, size);
  // If this load writes its result into a destination tmp (the "-> $n"
  // annotation used to merge the arms of && / || / ?: into one location),
  // route the loaded value there.  e.g. `a && b` where the right operand `b`
  // is a plain load: its result must land in the same register as the merge
  // temp, otherwise the consumer reads an unrelated register.
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL) {
    inst = SetDestOrMove(g, inst, dest, ARM_OP(mov));
  }
  return SetLoweredNode(node, inst);
}

static TargetInstruction* Store(ARMGenerator* g, IRNode* addr_node, TargetInstruction* src, ARMOpcode opcode, int size) {
  TargetInstruction* addr;
  TargetInstruction* offset;
  TargetInstruction* scale;
  bool on_stack = GetRegAndOffset(g, addr_node, &addr, &offset, &scale);
  // addr is a register.
  // if the variable is in memory offset will be an integer constant
  // containing the offset.  Otherwise it is NULL.

  // If we are not on the stack, move the src to the dest.
  if (!on_stack) {
    ARMOpcode opcode = TypeIsFloatingPoint(addr_node->type) ? ARM_OP(fmov) : ARM_OP(mov);
    TargetInstruction* result = SetDestOrMove(g, src, addr, opcode);
    return result;
  }

  return Emit(g, SetInstructionSize(NewInstruction4(opcode, src, addr, offset, scale), size));
}

static TargetInstruction* Compare(ARMGenerator* g, TargetInstruction* lhs, TargetInstruction* rhs,
                                  bool is_floating_point, int size) {
  return Emit(g, SetInstructionSize(NewInstruction2(is_floating_point ? ARM_OP(fcmp) : ARM_OP(cmp), lhs, rhs), size));
}

static TargetInstruction* CompareImmediate(ARMGenerator* g, TargetInstruction* lhs, TargetInstruction* rhs, int size) {
  return Emit(g, SetInstructionSize(NewInstruction2(ARM_OP(cmp), lhs, rhs), size));
}

static void CompareEqualZero(ARMGenerator* g, IRNode* value_node,
                             IRNode* target_node, int size) {
  CompareImmediate(g, Materialize(g, value_node), ZeroImm(g), size);
  EmitBranch(g, ARM_OP(eq), target_node);
}

static void CompareNotEqualZero(ARMGenerator* g, IRNode* value_node,
                             IRNode* target_node, int size) {
  CompareImmediate(g, Materialize(g, value_node), ZeroImm(g), size);
  EmitBranch(g, ARM_OP(ne), target_node);
}

enum ComparisonOp {
  kCompEqual,
  kCompNotEqual,
  kCompLess,
  kCompGreater,
  kCompLessEq,
  kCompGreaterEq,
};

struct Comparison {
  enum ComparisonOp comparison;
  ARMOpcode signed_forward;
  ARMOpcode signed_reverse;
  ARMOpcode unsigned_forward;
  ARMOpcode unsigned_reverse;
};

struct ToComparisonOp {
  IROpcode op;
  enum ComparisonOp forward_comparison;
  enum ComparisonOp inverted_comparison;
};

static struct Comparison comparisons[] = {
  {kCompEqual, ARM_OP(eq), ARM_OP(ne), ARM_OP(eq), ARM_OP(ne)},
  {kCompNotEqual, ARM_OP(ne), ARM_OP(eq), ARM_OP(ne), ARM_OP(eq)},
  {kCompLess, ARM_OP(lt), ARM_OP(ge), ARM_OP(lo), ARM_OP(hs)},
  {kCompGreater, ARM_OP(gt), ARM_OP(le), ARM_OP(hi), ARM_OP(ls)},
  {kCompLessEq, ARM_OP(le), ARM_OP(gt), ARM_OP(ls), ARM_OP(hi)},
  {kCompGreaterEq, ARM_OP(ge), ARM_OP(lt), ARM_OP(hs), ARM_OP(lo)},
};

#define NUM_COMPARISONS (sizeof(comparisons) / sizeof(comparisons[0]))

static ARMOpcode GetComparison(enum ComparisonOp op, bool is_unsigned, bool reverse) {
  for (int i = 0; i < NUM_COMPARISONS; i++) {
    struct Comparison* c = &comparisons[i];
    if (c->comparison == op) {
      if (is_unsigned) {
        if (reverse) {
          return c->unsigned_reverse;
        }
        return c->unsigned_forward;
      } else if (reverse) {
        return c->signed_reverse;
      }
      return c->signed_forward;
    }
  }
  abort();
}

static struct ToComparisonOp comparison_ops[] = {
  {IR_OP(cmpeqi), kCompEqual, kCompNotEqual},
  {IR_OP(cmpeqa), kCompEqual, kCompNotEqual},
  {IR_OP(cmpeqf), kCompEqual, kCompNotEqual},
  {IR_OP(cmpeqd), kCompEqual, kCompNotEqual},
  
  {IR_OP(cmpnei), kCompNotEqual, kCompEqual},
  {IR_OP(cmpnea), kCompNotEqual, kCompEqual},
  {IR_OP(cmpnef), kCompNotEqual, kCompEqual},
  {IR_OP(cmpned), kCompNotEqual, kCompEqual},

  {IR_OP(cmplti), kCompLess, kCompGreater},
  {IR_OP(cmplta), kCompLess, kCompGreater},
  {IR_OP(cmpltf), kCompLess, kCompGreater},
  {IR_OP(cmpltd), kCompLess, kCompGreater},
  
  {IR_OP(cmpgti), kCompGreater, kCompLess},
  {IR_OP(cmpgta), kCompGreater, kCompLess},
  {IR_OP(cmpgtf), kCompGreater, kCompLess},
  {IR_OP(cmpgtd), kCompGreater, kCompLess},

  {IR_OP(cmplei), kCompLessEq, kCompGreaterEq},
  {IR_OP(cmplea), kCompLessEq, kCompGreaterEq},
  {IR_OP(cmplef), kCompLessEq, kCompGreaterEq},
  {IR_OP(cmpled), kCompLessEq, kCompGreaterEq},
  
  {IR_OP(cmpgei), kCompGreaterEq, kCompLessEq},
  {IR_OP(cmpgea), kCompGreaterEq, kCompLessEq},
  {IR_OP(cmpgef), kCompGreaterEq, kCompLessEq},
  {IR_OP(cmpged), kCompGreaterEq, kCompLessEq},
};

#define NUM_COMPARISONS_OPS (sizeof(comparison_ops) / sizeof(comparison_ops[0]))

static enum ComparisonOp ToComparisonOp(IROpcode op, bool invert) {
  for (int i = 0; i < NUM_COMPARISONS_OPS; i++) {
    struct ToComparisonOp* c = &comparison_ops[i];
    if (c->op == op) {
      return invert ? c->inverted_comparison : c->forward_comparison;
    }
  }
  abort();
}

// Compares 2 nodes.  Branches to target.
static TargetInstruction* CompareAndBranch(ARMGenerator* g,
                                           IRNode* lhs_node,
                                           IRNode* rhs_node,
                                           IRNode* target_node,
                                           bool is_unsigned,
                                           bool reverse,
                                           IROpcode op) {
  bool is_floating_point = false;
  switch (op) {
    case IR_OP(cmpeqf):
    case IR_OP(cmpeqd):
    case IR_OP(cmpnef):
    case IR_OP(cmpned):
    case IR_OP(cmpltf):
    case IR_OP(cmpltd):
    case IR_OP(cmpgtf):
    case IR_OP(cmpgtd):
    case IR_OP(cmplef):
    case IR_OP(cmpled):
    case IR_OP(cmpgef):
    case IR_OP(cmpged):
      is_floating_point = true;
      break;
    default:
      break;
  }
  
  // 64-bit integer comparison: build the flags from the register pair and
  // branch on the resulting condition.
  if (!is_floating_point && TypeIsWideInt(lhs_node->type)) {
    ARMOpcode cond =
        WideCompareSetFlags(g, lhs_node, rhs_node, op, is_unsigned, reverse);
    return EmitBranch(g, cond, target_node);
  }

  int size = kSize32Bit;
  if (lhs_node->type->size > 4) {
    size = kSize64Bit;
  }
  TargetInstruction* lhs = NULL;
  TargetInstruction* rhs = NULL;
  bool constant_compare = false;
  bool inverted_comparison = false;

  // Floating point comparison doesn't have an immediate variant.
  if (!is_floating_point) {
    if (IRIsConstant(rhs_node)) {
      int64_t v = IRIntConstValue(rhs_node);
      constant_compare = ARMIsPossibleImmediate(v);
    } else if (IRIsConstant(lhs_node)) {
      int64_t v = IRIntConstValue(lhs_node);
      constant_compare = ARMIsPossibleImmediate(v);
      if (constant_compare) {
        // Comparison with constant on left. Invert comparison.
        inverted_comparison = true;
      }
    }
  }
  
  if (constant_compare) {
    if (inverted_comparison) {
      rhs = GetLoweredNode(lhs_node);
      lhs = Materialize(g, rhs_node);
    } else {
      lhs = Materialize(g, lhs_node);
      rhs = GetLoweredNode(rhs_node);
    }
    CompareImmediate(g, lhs, rhs, size);
  } else {
    lhs = Materialize(g, lhs_node);
    rhs = Materialize(g, rhs_node);
    Compare(g, lhs, rhs, is_floating_point, size);
  }
  
  enum ComparisonOp comp_op = ToComparisonOp(op, inverted_comparison);
  ARMOpcode cond = GetComparison(comp_op, is_unsigned, reverse);
  return EmitBranch(g, cond, target_node);
}

// Store a 64-bit integer (low, high) register pair to memory using two 32-bit
// stores.
static TargetInstruction* LowerWideStore(ARMGenerator* g, IRNode* node) {
  IRNode* addr_node = node->inputs.value.p[0];
  IRNode* src_node = node->inputs.value.p[1];
  TargetInstruction *src_lo, *src_hi;
  MaterializeWide(g, src_node, &src_lo, &src_hi);

  TargetInstruction* addr;
  TargetInstruction* offset;
  TargetInstruction* scale;
  bool on_stack = GetRegAndOffset(g, addr_node, &addr, &offset, &scale);
  assert(on_stack && "wide store target must be addressable");
  assert(TargetIsConst(offset) &&
         "wide store with non-constant offset not supported");
  int off = (int)TargetIntValue(offset);
  Emit(g, SetInstructionSize(
              NewInstruction4(ARM_OP(str), src_lo, addr, offset, NULL),
              kSize32Bit));
  TargetInstruction* offset_hi =
      GetIntConstant(g, NULL, kTargetType32Bit, off + 4);
  TargetInstruction* last = Emit(
      g, SetInstructionSize(
             NewInstruction4(ARM_OP(str), src_hi, addr, offset_hi, NULL),
             kSize32Bit));
  return SetLoweredNode(node, last);
}

static TargetInstruction* LowerStore(ARMGenerator* g, IRNode* node) {
  ARMOpcode opcode;
  assert(node->inputs.length == 2);

  // Address to store to is the first operand of the store IR node.
  IRNode* addr_node = node->inputs.value.p[0];

  // Value to store is in second input.
  IRNode* src_node = node->inputs.value.p[1];

  // store64 is always an 8-byte integer store (doubles use stored); store both
  // 32-bit halves.
  if (node->opcode == IR_OP(store64)) {
    return LowerWideStore(g, node);
  }

  int size = kSize32Bit;
  
  // Work out store opcode.
  switch (node->opcode) {
    case IR_OP(store32):
      opcode = ARM_OP(str);
      break;
    case IR_OP(store8):
      opcode = ARM_OP(strb);
      break;
    case IR_OP(store64):
      opcode = ARM_OP(str);
      size = kSize64Bit;
      break;
    case IR_OP(store16):
      opcode = ARM_OP(strh);
      break;
    case IR_OP(storef):
      opcode = ARM_OP(fstr);
      break;
    case IR_OP(stored):
      opcode = ARM_OP(fstr);
      size = kSize64Bit;
      break;
    case IR_OP(storea):
      opcode = ARM_OP(str);
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }
  TargetInstruction* src = Materialize(g, src_node);

  return SetLoweredNode(node, Store(g, addr_node, src, opcode, size));
}

// If the branch comes from a comparison node we combine the comparison
// with the branch.  If it comes from another node we generate an equality
// comparison of that node with zero.
static TargetInstruction* LowerConditionalBranch(ARMGenerator* g,
                                                 IRNode* node) {
  bool reverse = node->opcode == IR_OP(bfalse);

  assert(node->inputs.length == 2);
  IRNode* expr = node->inputs.value.p[0];
  IRNode* target_node = node->inputs.value.p[1];
  IRNode* input = node->inputs.value.p[0];

  // Check if the branch comes from a comparison.  It not, we compare with
  // zero.
  bool compare_with_zero = !IRIsComparison(input);
  if (!compare_with_zero && HasLoweredNode(input)) {
    TargetInstruction* comp = GetLoweredNode(input);
    compare_with_zero = (comp->flags & kARMComparisonGenerated) != 0;
  }
  if (compare_with_zero) {
    // The condition is an arbitrary value (e.g. the result of `||`/`&&` or a
    // plain load), not a comparison node, so it does not have two comparison
    // operands.  Compare it against zero instead.
    int size = kSize32Bit;
    if (expr->type->size > 4) {
      size = kSize64Bit;
    }
    if (IRIsConst(input)) {
      // Compare constant.
      // If constant is zero, BRA is comparing false
      // otherwise, BRA is comparing true.
      // The condition may also feed a merge destination (the "-> $n" used for
      // &&/||/?:).  For example `0 && f()` lowers the constant 0 as the branch
      // condition *and* the merge result; if we only fold the branch and drop
      // the dest, the merge slot keeps a stale value.  Materialize the constant
      // into its destination before taking the (folded) branch.
      if (input->dest != NULL) {
        TargetInstruction* dest = GetDestInstruction(g, input);
        if (dest != NULL) {
          TargetInstruction* cval_inst = Materialize(g, input);
          SetDestOrMove(g, cval_inst, dest, ARM_OP(mov));
        }
      }
      int64_t cval = IRIntConstValue(input);
      if (cval == 0) {
        if (node->opcode == IR_OP(bfalse)) {
          EmitBranch(g, ARM_OP(al), target_node);
        }
      } else {
        if (node->opcode == IR_OP(btrue)) {
          EmitBranch(g, ARM_OP(al), target_node);
        }
      }
   } else {
      // Generate equality comparison with zero.
      if (reverse) {
        CompareEqualZero(g, expr, target_node, size);
      } else {
        CompareNotEqualZero(g, expr, target_node, size);
      }
    }
    return NULL;
  }
  IRNode* lhs = input->inputs.value.p[0];
  IRNode* rhs = input->inputs.value.p[1];
  bool is_unsigned = TypeIsUnsigned(lhs->type);
  return CompareAndBranch(g, lhs, rhs, target_node, is_unsigned, reverse, expr->opcode);
}

static TargetInstruction* LowerBranch(ARMGenerator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  IRNode* target_node = node->inputs.value.p[0];

  // A return jump must branch to the shared epilogue: even a leaf function may
  // need to restore callee-saved registers and deallocate its frame there.
  // (Register usage is not known until after register allocation, so emitting a
  // bare `ret` here would skip a restore that turns out to be required.)

  // Normal branch or return branch.
  TargetInstruction* inst =
      (TargetInstruction*)Emit(g, NewInstruction1(ARM_OP(b), Condition(g, ARM_OP(al), 0)));

  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    // Forward branch, add fixup for target label.
    VectorAppend(&g->base.fixups, NewBranchFixup(inst, target_node, 1));
  } else {
    inst->operand[1] = target;
  }
  return inst;
}

static TargetInstruction* LowerLabel(ARMGenerator* g, IRNode* label) {
  TargetInstruction* inst =  Emit(g, NewInstruction(ARM_OP(label)));
  label->data.ptr = inst;
  ApplyFixups(g, label);
  return inst;
}

static TargetInstruction* LowerNamedLabel(ARMGenerator* g, IRNode* label) {
  IRNamedLabel* n = (IRNamedLabel*)label;
  TargetInstruction* inst =  Emit(g, TargetNewNamedLabel(n->name));
  label->data.ptr = inst;
  return inst;
}

static TargetInstruction* LowerResult(ARMGenerator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  ARMOpcode result_reg_opcode, opcode;
  switch (node->opcode) {
    case IR_OP(resulti):
    case IR_OP(resulta):
      result_reg_opcode = ARM_OP(resulti);
      opcode = ARM_OP(mov);
      break;
    case IR_OP(resultf):
      result_reg_opcode = ARM_OP(resultf);
      opcode = ARM_OP(fmov);
      break;
    case IR_OP(resultd):
      result_reg_opcode = ARM_OP(resultd);
      opcode = ARM_OP(fmov);
      break;
    default:
      assert(false);
      COMPILER_UNREACHABLE();
  }
#if 0
  TargetInstruction* result = Materialize(g, node->inputs.value.p[0]);
  TargetInstruction* result_reg = Emit(g, NewInstruction(result_reg_opcode));
  return Emit(g, NewInstruction2(opcode, result_reg, result));
#else
  if (node->opcode == IR_OP(resulti) && NodeIsWideInt(node)) {
    // 64-bit integer result: low half in r0 (resulti), high half in r1.
    TargetInstruction* lo;
    TargetInstruction* hi;
    MaterializeWide(g, node->inputs.value.p[0], &lo, &hi);
    TargetInstruction* r0 = EmitSymbol(g, NewInstruction(ARM_OP(resulti)));
    SetDestOrMove(g, lo, r0, ARM_OP(mov));
    TargetInstruction* r1 = EmitSymbol(g, NewInstruction(ARM_OP(r1)));
    TargetInstruction* res_hi = SetDestOrMove(g, hi, r1, ARM_OP(mov));
    return SetLoweredNode(node, res_hi);
  }
  TargetInstruction* result = Materialize(g, node->inputs.value.p[0]);
  TargetInstruction* result_reg = EmitSymbol(g, NewInstruction(result_reg_opcode));
  return SetLoweredNode(node, SetDestOrMove(g, result, result_reg, opcode));
#endif
}

static TargetInstruction* LowerAsm(ARMGenerator* g, IRNode* node) {
  // The first argument is a literal containing the assembly language.
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(g, TargetNewLiteral((int)id_node->value.ivalue));

  TargetInstruction* result = Emit(g, NewInstruction1(ARM_OP(asm), literal));

  SetLoweredNode(node, result);
  return result;
}

// A literal reference is an add of the literal offset (the first input
// to the literalref node) to the 'literal' with the given id.  This will
// be assembled as a reference to a symbol with the name .str.%d.
static TargetInstruction* LowerLiteralReference(ARMGenerator* g, IRNode* node) {
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(g, SetInstructionSize(TargetNewLiteral((int)id_node->value.ivalue), kSize32Bit));

  TargetInstruction* result = EmitAddressOfSymbol(g, literal, kSize32Bit);

  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerAddressOf(ARMGenerator* g, IRNode* node) {
  TargetInstruction* src = Materialize(g, node->inputs.value.p[0]);
  return SetLoweredNode(node, src);
}

static TargetInstruction* LowerZeroExtend(ARMGenerator* g, IRNode* node) {
  TargetInstruction* value = Materialize(g, node->inputs.value.p[0]);
  // The second IR operand is the bit-difference between the destination and
  // source widths (e.g. 24 for a char->int extension, 16 for a short->int
  // extension), NOT a usable AND mask.  Zero-extend by shifting the value left
  // to push the unwanted high bits out and then logically right, which clears
  // them.  (The previous code used the bit-difference directly as an AND mask,
  // emitting e.g. "and rX, rX, #24" instead of the correct "and rX, rX, #255".)
  IRConstant* diff_node = (IRConstant*)node->inputs.value.p[1];
  int64_t diff = diff_node->value.ivalue;
  if (diff <= 0 || diff >= 32) {
    return SetLoweredNode(node, value);
  }
  TargetInstruction* immed = GetIntConstant(g, NULL, kTargetType32Bit, diff);
  TargetInstruction* lsl =
      Emit(g, CopyInstructionSize(NewInstruction2(ARM_OP(lsl), value, immed), 0));
  TargetInstruction* lsr =
      Emit(g, CopyInstructionSize(NewInstruction2(ARM_OP(lsr), lsl, immed), 0));
  // Honor a "-> $n" merge destination (the temp used to merge the arms of
  // && / || / ?:).  A char/short operand of && is zero-extended here; without
  // routing the result to the merge temp the short-circuit path reads a stale
  // register (the extension lands in a fresh reg instead of the merge temp).
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL) {
    lsr = SetDestOrMove(g, lsr, dest, ARM_OP(mov));
  }
  return SetLoweredNode(node, lsr);
}

// A post-increment/decrement whose result value is used records the original
// (pre-update) value via a separate load node, attached as the inc/dec node's
// last input.  When the lvalue lives in a register (a register variable), that
// load aliases the variable's register, which the in-place update is about to
// overwrite.  Copy the old value into a fresh temporary and redirect the load's
// remaining uses to it so they observe the pre-update value.
static void PreserveInPlaceOldValue(ARMGenerator* g, IRNode* node, int size) {
  if (node->inputs.length <= 2) {
    return;
  }
  IRNode* old_load = node->inputs.value.p[2];
  if (old_load == NULL || !HasLoweredNode(old_load)) {
    return;
  }
  TargetInstruction* old = GetLoweredNode(old_load);
  if (old == NULL || !ARMIsVarRegister(old)) {
    return;
  }
  ARMOpcode copy_op =
      TypeIsFloatingPoint(node->type) ? ARM_OP(fmov) : ARM_OP(mov);
  TargetInstruction* saved =
      Emit(g, SetInstructionSize(NewInstruction1(copy_op, old), size));
  // SetLoweredNode is intentionally write-once; the load already has its
  // lowered value (the variable register).  Overwrite it directly so the load's
  // remaining consumers observe the preserved copy instead of the register that
  // the in-place update is about to clobber.
  old_load->data.ptr = saved;
}

static TargetInstruction* LowerInc(ARMGenerator* g, IRNode* node) {
  IRNode* addr_node = node->inputs.value.p[0];
  ARMOpcode ld_opcode;
  ARMOpcode st_opcode;
  int size = kSize32Bit;
  switch (node->opcode) {
    case IR_OP(inc8):
      ld_opcode = ARM_OP(ldrb);
      st_opcode = ARM_OP(strb);
      break;
    case IR_OP(uinc8):
      ld_opcode = ARM_OP(ldurb);
      st_opcode = ARM_OP(strb);
      break;
    case IR_OP(inc16):
      ld_opcode = ARM_OP(ldrh);
      st_opcode = ARM_OP(strh);
      break;
    case IR_OP(uinc16):
      ld_opcode = ARM_OP(ldurh);
      st_opcode = ARM_OP(strh);
      break;
   case IR_OP(inc32):
      ld_opcode = ARM_OP(ldr);
      st_opcode = ARM_OP(str);
      break;
    case IR_OP(uinc32):
       ld_opcode = ARM_OP(ldur);
       st_opcode = ARM_OP(str);
       break;
    case IR_OP(inc64):
    case IR_OP(uinc64):
      ld_opcode = ARM_OP(ldr);
      st_opcode = ARM_OP(str);
      size = kSize64Bit;
     break;
    case IR_OP(inca):
      ld_opcode = ARM_OP(ldr);
      st_opcode = ARM_OP(str);
     break;
    case IR_OP(incf):
      ld_opcode = ARM_OP(fldr);
      st_opcode = ARM_OP(fstr);
     break;
    case IR_OP(incd):
      ld_opcode = ARM_OP(fldr);
      st_opcode = ARM_OP(fstr);
      size = kSize64Bit;
     break;
    default:
      abort();
  }
  TargetInstruction* load = Load(g, addr_node, ld_opcode, size);
  PreserveInPlaceOldValue(g, node, size);
  TargetInstruction* inc;
  IRNode* amount_node = node->inputs.value.p[1];
  TargetInstruction* amount = GetLoweredNode(amount_node);
  if (TypeIsFloatingPoint(node->type)) {
    amount = Materialize(g, amount_node);
    inc =  Emit(g, SetInstructionSize(NewInstruction2(ARM_OP(fadd), load, amount), size));
  } else  {
    inc =  AddImmediate(g, load, ARMIntValue(amount));
  }
  Store(g, addr_node, inc, st_opcode, size);
  return SetLoweredNode(node, inc);
}


static TargetInstruction* LowerDec(ARMGenerator* g, IRNode* node) {
  IRNode* addr_node = node->inputs.value.p[0];
  ARMOpcode ld_opcode;
  ARMOpcode st_opcode;
  int size = kSize32Bit;

  switch (node->opcode) {
    case IR_OP(dec8):
      ld_opcode = ARM_OP(ldrb);
      st_opcode = ARM_OP(strb);
      break;
    case IR_OP(udec8):
      ld_opcode = ARM_OP(ldurb);
      st_opcode = ARM_OP(strb);
      break;
    case IR_OP(dec16):
      ld_opcode = ARM_OP(ldrh);
      st_opcode = ARM_OP(strh);
      break;
    case IR_OP(udec16):
      ld_opcode = ARM_OP(ldurh);
      st_opcode = ARM_OP(strh);
      break;
   case IR_OP(dec32):
      ld_opcode = ARM_OP(ldr);
      st_opcode = ARM_OP(str);
      break;
    case IR_OP(udec32):
       ld_opcode = ARM_OP(ldur);
       st_opcode = ARM_OP(str);
       break;
    case IR_OP(dec64):
    case IR_OP(udec64):
      ld_opcode = ARM_OP(ldr);
      st_opcode = ARM_OP(str);
      size = kSize64Bit;
     break;
    case IR_OP(deca):
      ld_opcode = ARM_OP(ldr);
      st_opcode = ARM_OP(str);
     break;
    case IR_OP(decf):
      ld_opcode = ARM_OP(fldr);
      st_opcode = ARM_OP(fstr);
     break;
    case IR_OP(decd):
      ld_opcode = ARM_OP(fldr);
      st_opcode = ARM_OP(fstr);
      size = kSize64Bit;
     break;
    default:
      abort();
  }
  TargetInstruction* load = Load(g, addr_node, ld_opcode, size);
  PreserveInPlaceOldValue(g, node, size);
  TargetInstruction* inc;
  IRNode* amount_node = node->inputs.value.p[1];
  TargetInstruction* amount = GetLoweredNode(amount_node);
  if (TypeIsFloatingPoint(node->type)) {
    amount = Materialize(g, amount_node);
    inc =  Emit(g, SetInstructionSize(NewInstruction2(ARM_OP(fsub), load, amount), size));
  } else  {
    inc =  AddImmediate(g, load, -ARMIntValue(amount));
  }
  Store(g, addr_node, inc, st_opcode, size);
  return SetLoweredNode(node, inc);
}

// TODO: ARM has bitfield instructions,
static TargetInstruction* LowerGetBitField(ARMGenerator* g, IRNode* node) {
  TargetInstruction* value = Materialize(g, node->inputs.value.p[0]);
  int bit_pos = (int)IRIntConstValue(node->inputs.value.p[1]);
  int bit_size = (int)IRIntConstValue(node->inputs.value.p[2]);
  if (TypeIsUnsigned(node->type)) {
    // Shift right by bit_pos
    // Mask with bit_size
    TargetInstruction* lsr = Emit(g, NewInstruction2(ARM_OP(asr), value, GetIntConstant(g, NULL, kTargetType32Bit, bit_pos)));
    uint64_t mask = bit_size == 64 ? -1LL : (1 << bit_size) - 1;
    TargetInstruction* m = Emit(g, NewInstruction2(ARM_OP(and), lsr, GetIntConstant(g, NULL, kTargetType32Bit, mask)));
    SetLoweredNode(node, m);
    return m;
  }
  // Shift left by 64 - (bit_pos + bit_size).  Top bit in bit 63.
  // Shift right by 64 - bit_size.
  TargetInstruction* lsl = Emit(g, NewInstruction2(ARM_OP(lsl), value, GetIntConstant(g, NULL, kTargetType32Bit, 64 - (bit_pos + bit_size))));
  TargetInstruction* asr = Emit(g, NewInstruction2(ARM_OP(asr), lsl, GetIntConstant(g, NULL, kTargetType32Bit, 64 - bit_size)));

  SetLoweredNode(node, asr);
  return asr;
}

// Shift input left by bit_pos
// Mask input to bit_size bits (in correct position)
// Mask output by ~mask
// Or input into output.
static TargetInstruction* LowerSetBitField(ARMGenerator* g, IRNode* node) {
  IRNode* output_node = node->inputs.value.p[0];
  IRNode* input_node = node->inputs.value.p[1];
  int bit_pos = (int)IRIntConstValue(node->inputs.value.p[2]);
  int bit_size = (int)IRIntConstValue(node->inputs.value.p[3]);
  uint64_t mask = bit_size == 64 ? -1LL : (1 << bit_size) - 1;
  mask <<= bit_pos;
  
  TargetInstruction* input = Materialize(g, input_node);
  TargetInstruction* output = Materialize(g, output_node);
  TargetInstruction* lsl = Emit(g, NewInstruction2(ARM_OP(lsl), input, GetIntConstant(g, NULL, kTargetType32Bit, bit_pos)));
  TargetInstruction* m1 = Emit(g, NewInstruction2(ARM_OP(and), lsl, GetIntConstant(g, NULL, kTargetType32Bit, mask)));

  TargetInstruction* m2 = Emit(g, NewInstruction2(ARM_OP(and), output, GetIntConstant(g, NULL, kTargetType32Bit, ~mask)));
  TargetInstruction* result = Emit(g, NewInstruction2(ARM_OP(orr), m1, m2));
  SetLoweredNode(node, result);
  return result;
}

// Finish a sign extension: `lo` holds the 32-bit sign-extended value.  If the
// result type is 64-bit, fill the high word with copies of the sign bit
// (asr lo, #31) so the value is correctly sign-extended across the register
// pair.
static TargetInstruction* FinishSignExtend(ARMGenerator* g, IRNode* node,
                                           TargetInstruction* lo) {
  if (NodeIsWideInt(node)) {
    TargetInstruction* hi = EmitWide32(
        g, NewInstruction2(ARM_OP(asr), lo,
                           GetIntConstant(g, NULL, kTargetType32Bit, 31)));
    SetLoweredHi(node, hi);
    return SetLoweredNode(node, lo);
  }
  // Honor a "-> $n" merge destination (see LowerZeroExtend): a sign-extended
  // char/short operand of && / || / ?: must write the merge temp.
  TargetInstruction* dest = GetDestInstruction(g, node);
  if (dest != NULL) {
    lo = SetDestOrMove(g, lo, dest, ARM_OP(mov));
  }
  return SetLoweredNode(node, lo);
}

static TargetInstruction* LowerSignExtend(ARMGenerator* g, IRNode* node) {
  TargetInstruction* value = Materialize(g, node->inputs.value.p[0]);
  if (ARMIsSignedLoad(value)) {
    return FinishSignExtend(g, node, value);
  }
  IRConstant* diff_value = node->inputs.value.p[1];
  int64_t diff = diff_value->value.ivalue;
  if (diff > 0) {
    return FinishSignExtend(g, node, value);
  }
  diff = -diff;
  if (diff == 32 || diff == 0) {
    // The low word already holds the full source value (e.g. int -> long long);
    // only the high word (handled by FinishSignExtend) needs filling.
    return FinishSignExtend(g, node,
                            Emit(g, NewInstruction1(ARM_OP(mov), value)));
  }
  TargetInstruction* immed = GetIntConstant(g, NULL, kTargetType32Bit, diff);
  TargetInstruction* lsl = Emit(g, CopyInstructionSize(NewInstruction2(ARM_OP(lsl), value, immed), 0));
  TargetInstruction* asr = Emit(g, CopyInstructionSize(NewInstruction2(ARM_OP(asr), lsl, immed), 0));

  return FinishSignExtend(g, node, asr);
}

static TargetInstruction* LowerAlign(ARMGenerator* g, IRNode* node) {
  TargetInstruction* value = Materialize(g, node->inputs.value.p[0]);
  IRConstant* align = node->inputs.value.p[1];

  TargetInstruction* immed = GetIntConstant(g, NULL, kTargetType32Bit, align->value.ivalue - 1);
  TargetInstruction* inv_immed = GetIntConstant(g, NULL, kTargetType32Bit, ~(align->value.ivalue - 1));
  TargetInstruction* add = Emit(g, CopyInstructionSize(NewInstruction2(ARM_OP(add), value, immed), 0));
  TargetInstruction* and = Emit(g, CopyInstructionSize(NewInstruction2(ARM_OP(and), add, inv_immed), 0));

  SetLoweredNode(node, and);
  return and;
}

static TargetInstruction* PushArg(ARMGenerator* g, IRNode* node,
                                  TargetInstruction* inst, size_t offset) {
  if (node->type == NULL) {
    // No type, use str instruction.
    return Emit(g, CopyInstructionSize(NewInstruction3(
                        ARM_OP(str), inst, StackPointer(g),
                        GetIntConstant(g, NULL, kTargetType64Bit, offset)), 0));
  }
  ARMOpcode opcode = ARM_OP(str);
  if (TypeIsFloatingPoint(node->type)) {
    opcode = ARM_OP(fstr);
  }
  return Emit(g, CopyInstructionSize(NewInstruction3(
                      opcode, inst, StackPointer(g),
                      GetIntConstant(g, NULL, kTargetType64Bit, offset)), 0));
}

static TargetInstruction* PopArg(ARMGenerator* g, IRNode* node, size_t offset) {
  if (node->type == NULL) {
    // No type, use ldr instruction.
    return Emit(g, CopyInstructionSize(NewInstruction2(
                        ARM_OP(ldr), StackPointer(g),
                        GetIntConstant(g, NULL, kTargetType64Bit, offset)), 0));
  }
  ARMOpcode opcode = ARM_OP(ldr);
  if (TypeIsFloatingPoint(node->type)) {
    opcode = ARM_OP(fldr);
  }
  return Emit(g, CopyInstructionSize(NewInstruction2(
                      opcode, StackPointer(g),
                      GetIntConstant(g, NULL, kTargetType64Bit, offset)), 0));
}


static TargetInstruction* LowerMemcpy(ARMGenerator* g, IRNode* node) {
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
  TargetInstruction* src_scale;
  int src_offset_value = 0;
  GetRegAndOffset(g, src_node, &src_addr, &src_offset, &src_scale);
  if (src_offset != NULL) {
    if (!ARMIsIntConst(src_offset)) {
      src_addr = AddValue(g, src_addr, src_offset);
      if (src_scale != NULL) {
        // TODO: reduce this to shifts if possible.
        src_addr = Emit(g, NewInstruction2(ARM_OP(mul), src_addr, src_offset));
      }
    } else {
      src_offset_value = (int)((TargetConstant*)src_offset)->value.ivalue;
      if (src_scale != NULL) {
        src_offset_value *= TargetIntValue(src_scale);
      }
    }
  }
  src_node->data.ptr = src_addr;

  // Destination address.
  IRNode* dest_node = node->inputs.value.p[0];
  TargetInstruction* dest_addr;
  TargetInstruction* dest_offset;
  TargetInstruction* dest_scale;
  int dest_offset_value = 0;
  GetRegAndOffset(g, dest_node, &dest_addr, &dest_offset, &dest_scale);

  if (dest_offset != NULL) {
    if (!ARMIsIntConst(dest_offset)) {
      // We have an address and register for the address, add them together.
      dest_addr = AddValue(g, dest_addr, dest_offset);
      if (dest_scale != NULL) {
        // TODO: reduce this to shifts if possible.
        dest_addr = Emit(g, NewInstruction2(ARM_OP(mul), dest_addr, dest_offset));
      }
   } else {
      dest_offset_value = (int)((TargetConstant*)dest_offset)->value.ivalue;
     if (dest_scale != NULL) {
       dest_offset_value *= TargetIntValue(dest_scale);
     }
   }
  }

  int length = (int)((IRConstant*)node->inputs.value.p[2])->value.ivalue;
  TargetInstruction* result = Memcpy(g, dest_addr, src_addr, length,
                                     src_offset_value, dest_offset_value, true);

  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerMemzero(ARMGenerator* g, IRNode* node) {
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
  TargetInstruction* scale;
  int offset_value = 0;
  GetRegAndOffset(g, dest_node, &dest_addr, &dest_offset, &scale);

  if (dest_offset != NULL) {
    if (!ARMIsIntConst(dest_offset)) {
      // We have an address and register for the address, add them together.
      dest_addr = AddValue(g, dest_addr, dest_offset);
    } else {
      offset_value = (int)((TargetConstant*)dest_offset)->value.ivalue;
    }
  }
  dest_node->data.ptr = dest_addr;
  TargetInstruction* result =
      Memzero(g, dest_addr, var->symbol->type->size, offset_value);

  SetLoweredNode(node, result);
  return result;
}

typedef enum {
  kArgLocationRegister,
  kArgLocationPushed,
  kArgLocationPassedByReferenceInRegister,
  kArgLocationPassedByReferenceOnStack,
  // A variadic double passed (per AAPCS) in a pair of core integer registers.
  kArgLocationFpIntPair,
  // A 64-bit integer (long long) passed in an even-aligned core register pair.
  kArgLocationIntPair,
  // A 64-bit integer passed on the stack (both halves, 8-byte aligned).
  kArgLocationPushedWide,
} ArgLocationType;

typedef struct {
  ArgLocationType type;
  union {
    TargetInstruction* reg;
    size_t offset;
  } location;
  TargetInstruction* reg2;  // High half register for kArgLocationFpIntPair.
  size_t reference_offset;
} ArgLocation;

static ArgLocation* NewArgLocationRegister(TargetInstruction* reg) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationRegister;
  loc->location.reg = reg;
  loc->reg2 = NULL;
  loc->reference_offset = 0;
  return loc;
}

// A variadic double passed in two consecutive core registers (lo, hi).
static ArgLocation* NewArgLocationFpIntPair(TargetInstruction* lo,
                                            TargetInstruction* hi) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationFpIntPair;
  loc->location.reg = lo;
  loc->reg2 = hi;
  loc->reference_offset = 0;
  return loc;
}

// A 64-bit integer passed in an even-aligned core register pair (lo, hi).
static ArgLocation* NewArgLocationIntPair(TargetInstruction* lo,
                                          TargetInstruction* hi) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationIntPair;
  loc->location.reg = lo;
  loc->reg2 = hi;
  loc->reference_offset = 0;
  return loc;
}

static ArgLocation* NewArgLocationPushed(ArgLocationType type, size_t offset) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = type;
  loc->location.offset = offset;
  loc->reg2 = NULL;
  loc->reference_offset = 0;
  return loc;
}

static ArgLocation* NewArgLocationReferenceInRegister(TargetInstruction* reg,
                                                      size_t reference_offset) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationPassedByReferenceInRegister;
  loc->location.reg = reg;
  loc->reg2 = NULL;
  loc->reference_offset = reference_offset;
  return loc;
}

static ArgLocation* NewArgLocationReferenceOnStack(size_t offset,
                                                   size_t reference_offset) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationPassedByReferenceOnStack;
  loc->location.offset = offset;
  loc->reg2 = NULL;
  loc->reference_offset = reference_offset;
  return loc;
}

// Build a list of ARM_OP(regarg) instructions to hold the
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
static TargetInstruction* BuildArgList(ARMGenerator* g, Vector* arg_locations) {
  TargetInstruction* result = NULL;
  // Find next -based argument and add it to the regargs instruction list.
  for (size_t i = 0; i < arg_locations->length; i++) {
    ArgLocation* loc = arg_locations->value.p[i];
    if (loc->type == kArgLocationRegister) {
      result =
          Emit(g, NewInstruction2(ARM_OP(regarg), result, loc->location.reg));
    } else if (loc->type == kArgLocationFpIntPair ||
               loc->type == kArgLocationIntPair) {
      // Both core registers of a 64-bit argument must stay live to the call.
      result =
          Emit(g, NewInstruction2(ARM_OP(regarg), result, loc->location.reg));
      result = Emit(g, NewInstruction2(ARM_OP(regarg), result, loc->reg2));
    }
  }
  return result;
}

// The RISC-V calling convention is very complex.  There are 8 integer and 8
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
// Determine how many leading arguments of the callee are *named* (declared in
// the prototype).  Returns true and sets *named_count if the callee is a
// variadic function; arguments at or beyond *named_count are variadic and, on
// AAPCS, must be passed in core registers / on the stack rather than VFP.
static bool CalleeVariadicNamedCount(IRNode* node, int* named_count) {
  IRNode* target = node->inputs.value.p[0];
  TypeRecord* t = target->type;
  while (t != NULL && t->declarator == kDeclPointer) {
    t = t->next;
  }
  if (t == NULL || t->declarator != kDeclFunction) {
    return false;
  }
  if (!t->info.function.varargs) {
    return false;
  }
  *named_count = (int)t->info.function.prototype.length;
  return true;
}

// AAPCS stack-slot layout for arguments passed on the stack.  Scalars of 4
// bytes or less occupy a 4-byte slot; 8-byte scalars (long long / double) are
// 8-byte aligned and occupy 8 bytes.  Structs/unions keep an 8-byte slot
// (matching the by-value / by-reference handling in LowerCall).  This sizing
// must be identical on the caller (LowerCall), the named-argument reader
// (ArgumentLocation) and va_arg so that overflow arguments line up.
static size_t ArgStackAlignment(TypeRecord* type) {
  if (type == NULL) {
    return 4;
  }
  if (TypeIsStructOrUnion(type)) {
    // A by-value struct keeps its natural alignment, clamped to the 4/8-byte
    // argument-slot granularity (AAPCS aligns aggregate arguments to at most a
    // double-word).
    size_t a = (size_t)TypeRecordAlignment(type);
    return a <= 4 ? 4 : 8;
  }
  if (TypeIsPointerOrArray(type)) {
    // Pointer and (decayed) array arguments are passed as a 4-byte pointer, not
    // as the pointee/array size.  Using type->size here would over-reserve the
    // slot (e.g. a string literal of type char[5]) and desync va_arg, which
    // advances pointers by 4 bytes.
    return 4;
  }
  if (type->size > 4) {
    return 8;
  }
  return 4;
}

static size_t ArgStackSize(TypeRecord* type) {
  if (type == NULL) {
    return 4;
  }
  if (TypeIsStructOrUnion(type)) {
    // A by-value struct occupies its whole size on the stack, rounded up to a
    // whole number of 4-byte words.
    return ((size_t)type->size + 3) & ~(size_t)3;
  }
  if (TypeIsPointerOrArray(type)) {
    return 4;  // Passed as a 4-byte pointer (see ArgStackAlignment).
  }
  if (type->size > 4) {
    return 8;
  }
  return 4;
}

// Align *offset for an argument of the given type, return its slot offset and
// advance *offset past it.
static size_t ArgStackSlot(TypeRecord* type, size_t* offset) {
  size_t align = ArgStackAlignment(type);
  *offset = (*offset + align - 1) & ~(align - 1);
  size_t slot = *offset;
  *offset += ArgStackSize(type);
  return slot;
}

static TargetInstruction* LowerCall(ARMGenerator* g, IRNode* node) {
  assert(node->inputs.length >= 1);
  size_t struct_area_size = 0;
  int next_int_arg_reg = 0;
  int next_fp_arg_reg = 0;
  size_t next_pushed_arg_offset = 0;
  Vector arg_locations;
  VectorInit(&arg_locations);

  // Variadic floating-point arguments follow the integer ABI on ARM.
  int named_count = 0;
  bool callee_varargs = CalleeVariadicNamedCount(node, &named_count);
  // Argument ordinal among the real (source) arguments, ignoring a leading
  // hidden struct-return pointer.
  int arg_ordinal = 0;

  // Phase 1:
  // Work out the locations for all arguments.  The first 8 go in argument
  // registers, split into integer and floating point sets.
  for (size_t i = 1; i < node->inputs.length; i++) {
    IRNode* arg_node = node->inputs.value.p[i];
    if (TypeIsStructOrUnion(arg_node->type)) {
      if (i == 1 && arg_node->opcode == IR_OP(structreturn)) {
        // RVO (Return Value Optimization), passing structreturn as arg->base.
        TargetInstruction* arg_reg =
             IntArgumentRegister(g, next_int_arg_reg++);
         VectorAppend(&arg_locations, NewArgLocationRegister(arg_reg));
        continue;
      }
      bool variadic = callee_varargs && arg_ordinal >= named_count;
      // Named struct/union arguments are passed *by value* in the stacked
      // argument area, laid out consecutively (see ArgStackSlot).  The callee
      // reads them directly at fp + slot (ArgumentLocation / the argument case
      // in AssignRegisterOrOffset).  This keeps multi-struct calls and structs
      // of any size correct without the broken "fits in one register" path.
      // Variadic struct arguments are passed *by reference*: a single 4-byte
      // pointer to a caller-made copy, occupying one word in the argument area
      // (a core register, then the stack).  Keeping every variadic struct one
      // word wide lets va_arg(struct) walk the save area uniformly -- it loads
      // the pointer and treats it as the struct's address (see
      // LowerBuiltinVaArg).
      size_t struct_size = arg_node->type->size;
      if (!variadic) {
        size_t slot = ArgStackSlot(arg_node->type, &next_pushed_arg_offset);
        VectorAppend(&arg_locations,
                     NewArgLocationPushed(kArgLocationPushed, slot));
      } else {
        if (next_int_arg_reg < ARM_NUM_INT_ARGS) {
          // Pointer to the copy passed in an argument register.
          TargetInstruction* arg_reg =
              IntArgumentRegister(g, next_int_arg_reg++);
          VectorAppend(&arg_locations, NewArgLocationReferenceInRegister(
                                           arg_reg, struct_area_size));
        } else {
          // Pointer to the copy passed in a single 4-byte stack slot.
          size_t slot = next_pushed_arg_offset;
          next_pushed_arg_offset += 4;
          VectorAppend(&arg_locations,
                       NewArgLocationReferenceOnStack(slot, struct_area_size));
        }
        // Reserve an 8-byte-aligned region for the caller's copy so that
        // 8-byte members (HFA doubles / long double) stay aligned.
        struct_area_size += (struct_size + 7) & ~(size_t)7;
      }
    } else if (TypeIsFloatingPoint(arg_node->type)) {
      bool variadic = callee_varargs && arg_ordinal >= named_count;
      if (variadic) {
        // AAPCS: a variadic double is passed like a 64-bit integer -- in a pair
        // of core registers (8-byte aligned, so starting at an even register)
        // or on the stack.  (Variadic floats are promoted to double by the
        // front end, so every variadic FP argument is 8 bytes.)
        if (next_int_arg_reg % 2 != 0) {
          next_int_arg_reg++;  // 8-byte alignment: skip to an even register.
        }
        if (next_int_arg_reg + 1 < ARM_NUM_INT_ARGS) {
          TargetInstruction* lo = IntArgumentRegister(g, next_int_arg_reg);
          TargetInstruction* hi = IntArgumentRegister(g, next_int_arg_reg + 1);
          next_int_arg_reg += 2;
          VectorAppend(&arg_locations, NewArgLocationFpIntPair(lo, hi));
        } else {
          // No room for a register pair: pass the whole double on the stack.
          next_int_arg_reg = ARM_NUM_INT_ARGS;
          size_t slot = ArgStackSlot(arg_node->type, &next_pushed_arg_offset);
          VectorAppend(&arg_locations,
                       NewArgLocationPushed(kArgLocationPushed, slot));
        }
      } else if (next_fp_arg_reg < ARM_NUM_FP_ARGS) {
        // Argument goes in an argument register.
        TargetInstruction* arg_reg =
            FloatingPointArgumentRegister(g, next_fp_arg_reg++);
        VectorAppend(&arg_locations, NewArgLocationRegister(arg_reg));
      } else {
        // Need to push argument on to the stack.  But we do that in reverse
        // order so for now, we record that the arg location is on the stack.
        size_t slot = ArgStackSlot(arg_node->type, &next_pushed_arg_offset);
        VectorAppend(&arg_locations,
                     NewArgLocationPushed(kArgLocationPushed, slot));
      }
    } else if (TypeIsWideInt(arg_node->type)) {
      // AAPCS: a 64-bit integer is passed in an even-aligned pair of core
      // registers (r0:r1 or r2:r3) or, once those run out, 8-byte aligned on
      // the stack.
      if (next_int_arg_reg % 2 != 0) {
        next_int_arg_reg++;  // 8-byte alignment: skip to an even register.
      }
      if (next_int_arg_reg + 1 < ARM_NUM_INT_ARGS) {
        TargetInstruction* lo = IntArgumentRegister(g, next_int_arg_reg);
        TargetInstruction* hi = IntArgumentRegister(g, next_int_arg_reg + 1);
        next_int_arg_reg += 2;
        VectorAppend(&arg_locations, NewArgLocationIntPair(lo, hi));
      } else {
        next_int_arg_reg = ARM_NUM_INT_ARGS;
        size_t slot = ArgStackSlot(arg_node->type, &next_pushed_arg_offset);
        VectorAppend(&arg_locations,
                     NewArgLocationPushed(kArgLocationPushedWide, slot));
      }
    } else {
      if (next_int_arg_reg < ARM_NUM_INT_ARGS) {
        // Argument goes in an argument register.
        TargetInstruction* arg_reg =
            IntArgumentRegister(g, next_int_arg_reg++);
        VectorAppend(&arg_locations, NewArgLocationRegister(arg_reg));
      } else {
        // Need to push argument on to the stack.  But we do that in reverse
        // order so for now, we record that the arg location is on the stack.
        size_t slot = ArgStackSlot(arg_node->type, &next_pushed_arg_offset);
        VectorAppend(&arg_locations,
                     NewArgLocationPushed(kArgLocationPushed, slot));
      }
    }
    arg_ordinal++;
  }

  // Phase 2:
  // Decrement the stack pointer to make space for the stack args.  Keep the
  // stack pointer 8-byte aligned at the call site (AAPCS), even though
  // individual word arguments only occupy 4-byte slots.
  size_t total_stack_size = struct_area_size + next_pushed_arg_offset;
  total_stack_size = (total_stack_size + 7) & ~(size_t)7;
  if (total_stack_size > 0) {
    TargetInstruction* newsp =
        AddImmediate(g, StackPointer(g), -total_stack_size);
    TargetSetDest(newsp, StackPointer(g));
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
      case kArgLocationPushed:
        if (TypeIsStructOrUnion(arg_node->type)) {
          // Named struct passed by value on the stack: copy the whole struct
          // into its stacked slot now, before the argument registers are set
          // up (the copy may materialize the source address into scratch
          // registers).  Phase 4 then leaves this argument alone.
          TargetInstruction* arg = Materialize(g, arg_node);
          Memcpy(g, StackPointer(g), arg, (int)size, 0,
                 (int)arg_location->location.offset, false);
        }
        break;
      case kArgLocationPassedByReferenceInRegister:
      case kArgLocationPassedByReferenceOnStack: {
        TargetInstruction* arg = Materialize(g, arg_node);
        Memcpy(g, StackPointer(g), arg, (int)size, 0,
               (int)(arg_location->reference_offset + next_pushed_arg_offset), false);
        break;
      }
      default:
        break;
    }
  }

  // Phase 3.5:
  // For an indirect call (the target is computed into a register rather than
  // being a link-time symbol) stage the target into its own register *before*
  // the argument registers are set up below.  The blr below references this
  // staged value, so the register allocator keeps it live across the argument
  // moves and won't reuse its register for an argument (e.g. r0).  Without
  // this, the call target and the first argument can land in the same
  // register and the argument move clobbers the target.
  bool will_tail_call = (node->flags & kIRTailCall) != 0 &&
      total_stack_size == 0 && g->base.stack_frame_size == 0;
  TargetInstruction* staged_target = NULL;
  if (!will_tail_call) {
    IRNode* target_node = node->inputs.value.p[0];
    TargetInstruction* a = GetLoweredNode(target_node);
    if ((int)a->opcode == (int)ARM_OP(bl) ||
        (int)a->opcode == (int)ARM_OP(blr) ||
        (((int)a->opcode != (int)ARM_OP(symbol)) && ARMGeneratesOutput(a))) {
      // Indirect (or chained) call: the target value is computed into a
      // register.  ARM has only r0-r3 for arguments, so the register allocator
      // readily places the target in an argument register that the arg setup
      // below then clobbers.  Copy the target into a fresh allocatable
      // temporary and call through it.  Using a fresh temp (rather than the
      // single dedicated scratch r9) lets the allocator preserve it across a
      // nested call inside the argument list -- e.g. an indirect call whose
      // argument is itself an indirect call would otherwise have both targets
      // collide in r9.
      TargetInstruction* tmp = Emit(g, NewInstruction(ARM_OP(tmp)));
      TargetInstruction* mv =
          Emit(g, CopyInstructionSize(NewInstruction1(ARM_OP(mov), a), 0));
      mv->dest = tmp;
      staged_target = mv;
    }
  }

  // Phase 4:
  // Pass through all args, in reverse order, pushing those not passed in
  // registers and moving the register arguments into their argument
  // registers.
  //
  // TODO: figure out if we can just set the dest to the reg->base.
  for (size_t i = node->inputs.length - 1; i >= 1; i--) {
    IRNode* arg_node = node->inputs.value.p[i];
    ArgLocation* arg_location = arg_locations.value.p[i - 1];
    switch (arg_location->type) {
      case kArgLocationPassedByReferenceInRegister: {
        // Struct passed by reference in a register.  The reference_offset
        // contains the offset from the to of the pushed args to the copied
        // struct.
        TargetInstruction* arg = AddImmediate(
            g, StackPointer(g),
            arg_location->reference_offset + next_pushed_arg_offset);
        SetDestOrMoveToArgReg(g, arg_node, arg, arg_location->location.reg, ARM_OP(mov));
        break;
      }
      case kArgLocationPassedByReferenceOnStack: {
        // Struct passed by reference on the stack.
        TargetInstruction* arg = AddImmediate(
            g, StackPointer(g),
            arg_location->reference_offset + next_pushed_arg_offset);
        PushArg(g, arg_node, arg, arg_location->location.offset);
        break;
      }
      case kArgLocationPushed: {
        if (TypeIsStructOrUnion(arg_node->type)) {
          // Named struct passed by value on the stack was already copied into
          // its slot in phase 3.
          break;
        }
        TargetInstruction* arg = Materialize(g, arg_node);
        PushArg(g, arg_node, arg, arg_location->location.offset);
        break;
      }
      case kArgLocationRegister: {
        // Argument is in a register.
        TargetInstruction* arg = Materialize(g, arg_node);
        if (TypeIsStructOrUnion(arg_node->type)) {
          size_t size = arg_node->type->size;
          if (size <= 8) {
            // A struct less than 8 bytes is passed in a register.  The
            // Materialize call will result in the address of the struct.  We
            // need to load it.
            arg = Emit(g, CopyInstructionSize(NewInstruction2(
                               ARM_OP(ldr), arg,
                               GetIntConstant(g, NULL, kTargetType32Bit, 0)), 0));
          }
        }
        ARMOpcode mov_opcode = ARM_OP(mov);
        if (TypeIsFloatingPoint(arg_node->type)) {
          if (TypeIsDouble(arg_node->type)) {
            mov_opcode = ARM_OP(fmov);
          } else {
            mov_opcode = ARM_OP(fmov);
          }
        }
        // Emit(g, NewInstruction2(mov_opcode, arg_location->location.reg, arg));
        SetDestOrMoveToArgReg(g, arg_node, arg, arg_location->location.reg, mov_opcode);
        break;
      }
      case kArgLocationFpIntPair: {
        // Variadic double: move its 64-bit value into a pair of core registers
        // with `vmov rLo, rHi, dX` (the from-double doubleword transfer, which
        // shares the fcvt opcode with the to-double form used by constd).  The
        // instruction's destination is the low register; the high register is
        // written by the same instruction and kept live via the regarg list.
        TargetInstruction* arg = Materialize(g, arg_node);
        TargetInstruction* mov = Emit(
            g, SetInstructionSize(
                   NewInstruction2(ARM_OP(fcvt), arg_location->reg2, arg),
                   kSize64Bit));
        mov->dest = arg_location->location.reg;
        break;
      }
      case kArgLocationIntPair: {
        // 64-bit integer: move the low half into the even register and the high
        // half into the odd register.  Both are kept live to the call via the
        // regarg list (BuildArgList).  Tag these as argument moves so the
        // parallel-move resolver schedules them together with the other integer
        // argument moves; otherwise a following argument materialized into one
        // of this pair's destination registers (r2/r3) would be clobbered
        // before its own move reads it.
        TargetInstruction* lo;
        TargetInstruction* hi;
        MaterializeWide(g, arg_node, &lo, &hi);
        TargetInstruction* mlo = Emit(g, NewInstruction1(ARM_OP(mov), lo));
        mlo->dest = arg_location->location.reg;
        mlo->flags |= kARMArgMove;
        TargetInstruction* mhi = Emit(g, NewInstruction1(ARM_OP(mov), hi));
        mhi->dest = arg_location->reg2;
        mhi->flags |= kARMArgMove;
        break;
      }
      case kArgLocationPushedWide: {
        // 64-bit integer passed on the stack: store both halves.
        TargetInstruction* lo;
        TargetInstruction* hi;
        MaterializeWide(g, arg_node, &lo, &hi);
        size_t off = arg_location->location.offset;
        Emit(g, SetInstructionSize(
                    NewInstruction3(ARM_OP(str), lo, StackPointer(g),
                                    GetIntConstant(g, NULL, kTargetType32Bit,
                                                   (int)off)),
                    kSize32Bit));
        Emit(g, SetInstructionSize(
                    NewInstruction3(ARM_OP(str), hi, StackPointer(g),
                                    GetIntConstant(g, NULL, kTargetType32Bit,
                                                   (int)off + 4)),
                    kSize32Bit));
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
  ARMOpcode opcode;
  TargetInstruction* call;
  
  // If the node has been identified as a tail call by the IR
  // optimizer we might be able to convert it to a jump.
  //
  // Possible optimization: if the arguments don't contain an address in
  // the current stack frame we can allow tail calls when we have space
  // allocated on the stack.  I don't know how to detect that though.
  bool can_be_tail_call = (node->flags & kIRTailCall) != 0 &&
      total_stack_size == 0 && g->base.stack_frame_size == 0;

  if (can_be_tail_call) {
    // Tail call.
    // Add a restore instruction and replace the call with a jump.
    if (ARMIsExpression(addr)) {
      // The address is calculated inside the function body.  It needs
      // to survive a restore operation so we need to put it in
      // a temp register.  All 't' regs should not be allocated now
      // since we are leaving the function.
      BuildArgList(g, &arg_locations);
      TargetInstruction* mv = Emit(g, CopyInstructionSize(NewInstruction1(ARM_OP(mov), addr), 0));
      mv->dest = Tmp(g);
      Emit(g, NewInstruction(ARM_OP(restore)));
      call = Emit(g, NewInstruction1(ARM_OP(br), Tmp(g)));
    } else {
      Emit(g, NewInstruction(ARM_OP(restore)));
      BuildArgList(g, &arg_locations);
      call = Emit(g, NewInstruction2(ARM_OP(b), Condition(g, ARM_OP(al), 0), addr));
    }
    g->base.num_calls--;
  } else {
    TargetInstruction* call_target = addr;
    if (staged_target != NULL) {
      // Indirect call: target was staged into its own register above.
      opcode = ARM_OP(blr);
      call_target = staged_target;
    } else if (((int)addr->opcode == (int)ARM_OP(symbol))) {
      // Calling a symbol, use a regular 'call' instruction.
      opcode = TypeIsFloatingPoint(node->type) ? ARM_OP(bl) : ARM_OP(bl);
    } else {
      // Calling through a register, rcall.
      opcode = TypeIsFloatingPoint(node->type) ? ARM_OP(blr) : ARM_OP(blr);
    }
    call =
        Emit(g, NewInstruction2(opcode, call_target, BuildArgList(g, &arg_locations)));
    // A call's result is returned in a caller-saved register (r0, r0:r1, or
    // d0).  If that value is consumed later it may have to survive another
    // call (e.g. f(g(), h()) evaluates both calls before either result is
    // used).  The result is hard-pinned to the return register and cannot be
    // moved to a callee-saved register by the allocator, so copy it into a
    // fresh register immediately while it is still live.  The allocator's
    // preserved-instruction logic can then keep that copy across later calls.
    bool result_used = node->outputs.length > 0;
    if (TypeIsFloatingPoint(node->type)) {
      // The result comes back in the floating-point return register.
      call->flags |= kARMFpReturn;
      SetInstructionSize(call, ARMTypeIsDouble(node->type) ? kSize64Bit : kSize32Bit);
      if (result_used) {
        call = Emit(g, CopyInstructionSize(NewInstruction1(ARM_OP(fmov), call), 0));
      }
    } else if (NodeIsWideInt(node)) {
      // A 64-bit integer is returned in r0:r1.  The call instruction itself
      // owns r0 (the low half); capture r1 (the high half) into a fresh
      // register immediately, while it is still live.
      TargetInstruction* r1 = Emit(g, NewInstruction(ARM_OP(r1)));
      TargetInstruction* hi = Emit(g, NewInstruction1(ARM_OP(mov), r1));
      SetLoweredHi(node, hi);
      if (result_used) {
        call = Emit(g, NewInstruction1(ARM_OP(mov), call));
      }
    } else if (result_used && node->type != NULL &&
               !TypeIsStructOrUnion(node->type)) {
      // Scalar (integer/pointer) result in r0.
      call = Emit(g, NewInstruction1(ARM_OP(mov), call));
    }

    // Increment the stack pointer again to remove pushed args.
    if (total_stack_size > 0) {
      TargetInstruction* newsp =
          AddImmediate(g, StackPointer(g), total_stack_size);
      TargetSetDest(newsp, StackPointer(g));
    }
  }
  // If the call result feeds a merge destination (the "-> $n" annotation used
  // to funnel the two arms of &&/||/?: into one location), route the result
  // there.  Without this the call's return value (in r0) is dropped and the
  // merge slot keeps the other arm's stale value, e.g. `x || f()` yields x
  // instead of f()'s result.  bl/blr are not expressions, so SetDestOrMove
  // emits an explicit move rather than redirecting the call's r0 output.
  if (node->dest != NULL) {
    TargetInstruction* dest = GetDestInstruction(g, node);
    if (dest != NULL) {
      call = SetDestOrMove(g, call, dest, ARM_OP(mov));
    }
  }
  SetLoweredNode(node, call);

  VectorDestructWithContents(&arg_locations, NULL, /*free_element=*/true);
  return call;
}

// A computed branch is used to branch to a dense switch table consisting
// of a sequence of 'b' instructions to the case labels.  Each instruction
// is 4 bytes long->base.  The instruction sequence for the computed branch is:
//
// entry: t0 = index into table.
// adr t1, 12                  - pc at start of table
// add t1, t1, t0, lsl #2      - address of jump instruction
// br t1
// // Table is here.
// b l1
// b l2
// ...
// b lx

static TargetInstruction* LowerComputedBranch(ARMGenerator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  TargetInstruction* value = GetLoweredNode(node->inputs.value.p[0]);

  TargetInstruction* adr =
      Emit(g, SetInstructionSize(NewInstruction1(ARM_OP(adr),
                               GetIntConstant(g, NULL, kTargetType32Bit, 4)), kSize32Bit));
  TargetInstruction* add = Emit(g, CopyInstructionSize(NewInstruction4(ARM_OP(add), adr, value, lsl(g),
                                                     GetIntConstant(g, NULL, kTargetType32Bit, 2)), 0));
  TargetInstruction* br =
      Emit(g, NewInstruction1(ARM_OP(br), add));
  br->flags |= TARGET_INST_TABLE_JUMP;
  SetLoweredNode(node, br);
  return br;

}

// The first input is the address of the 'ap' variable.  The second is the
// address of the last function argument (ignored in RISC-V).  This
// simply stores the value of the frame pointer in the address passed
// in the first input.
static TargetInstruction* LowerBuiltinVaStart(ARMGenerator* g, IRNode* node) {
  TargetInstruction* s0 = Emit(g, NewInstruction(ARM_OP(fp)));
  // fp points at the r0 slot of the variadic register save area (see the
  // prologue in arm_emitter.c).  Skip the named integer-register arguments so
  // ap points at the first variadic slot.  Subsequent 8-byte arguments are then
  // 8-byte aligned relative to the (8-byte aligned) fp, which va_arg relies on.
  TargetInstruction* ap_value = s0;
  int named_offset = g->num_int_arg_regs * 4;
  if (named_offset != 0) {
    ap_value = AddImmediate(g, s0, named_offset);
  }
  TargetInstruction* addr;
  TargetInstruction* offset;
  TargetInstruction* scale;
  bool on_stack = GetRegAndOffset(g, node->inputs.value.p[0], &addr, &offset, &scale);
  if (!on_stack) {
    TargetInstruction* mv = Emit(g, CopyInstructionSize(NewInstruction1(ARM_OP(mov), ap_value), 0));
    mv->dest = addr;
    return SetLoweredNode(node, addr);
  }
  return SetLoweredNode(node,
                        Emit(g, CopyInstructionSize(NewInstruction3(ARM_OP(str), ap_value, addr, offset), 0)));
}

// The first input is &ap.  The 'ap' variable contains the address of the
// current argument (starts at s0).  The code is:
// ld t0, 0(ap)  // Address of current arg->base.
// ld a0, 0(t1)      // Load current arg->base.
// addi t0, t0, 8    // Next arg
// sd t0, 0(a0)      // Update

static TargetInstruction* LowerBuiltinVaArg(ARMGenerator* g, IRNode* node) {
  TargetInstruction* ap_addr;
  TargetInstruction* ap_offset;
  TargetInstruction* ap_scale;
  bool on_stack =
      GetRegAndOffset(g, node->inputs.value.p[0], &ap_addr, &ap_offset, &ap_scale);
  TargetInstruction* ap_load;
  if (!on_stack) {
    ap_load = ap_addr;
  } else {
    ap_load = Emit(g, NewInstruction2(ARM_OP(ldr), ap_addr, ap_offset));
  }

  // Variadic structs are passed by reference (see LowerCall): the save-area
  // slot holds a 4-byte pointer to the caller's copy.  Load that pointer -- it
  // is the struct address va_arg yields -- and advance ap past the one-word
  // pointer slot.
  if (node->type != NULL && TypeIsStructOrUnion(node->type)) {
    TargetInstruction* ptr = Emit(
        g, SetInstructionSize(
               NewInstruction2(ARM_OP(ldr), ap_load,
                               GetIntConstant(g, NULL, kTargetType32Bit, 0)),
               kSize32Bit));
    TargetInstruction* next =
        Emit(g, NewInstruction2(ARM_OP(add), ap_load,
                                GetIntConstant(g, NULL, kTargetType32Bit, 4)));
    if (on_stack) {
      Emit(g, NewInstruction3(ARM_OP(str), next, ap_addr, ap_offset));
    } else {
      TargetInstruction* mv = Emit(g, NewInstruction1(ARM_OP(mov), next));
      mv->dest = ap_addr;
    }
    return SetLoweredNode(node, ptr);
  }

  // 8-byte scalars (double, long long) are 8-byte aligned in the variadic area.
  bool is_eight = node->type != NULL && node->type->size > 4;
  if (is_eight) {
    TargetInstruction* aligned =
        Emit(g, NewInstruction2(ARM_OP(add), ap_load,
                                GetIntConstant(g, NULL, kTargetType32Bit, 7)));
    aligned = Emit(g, NewInstruction2(ARM_OP(bic), aligned,
                                GetIntConstant(g, NULL, kTargetType32Bit, 7)));
    ap_load = aligned;
  }

  int data_size = is_eight ? kSize64Bit : kSize32Bit;
  TargetInstruction* result;
  if (TypeIsFloatingPoint(node->type)) {
    result = Emit(g, SetInstructionSize(
                         NewInstruction2(ARM_OP(fldr), ap_load,
                                         GetIntConstant(g, NULL, kTargetType32Bit, 0)),
                         data_size));
  } else if (NodeIsWideInt(node)) {
    // 64-bit integer variadic argument: load the two 32-bit halves into a
    // register pair (the single 64-bit ldr only loads 32 bits).
    result = Emit(g, SetInstructionSize(
                         NewInstruction2(ARM_OP(ldr), ap_load,
                                         GetIntConstant(g, NULL, kTargetType32Bit, 0)),
                         kSize32Bit));
    TargetInstruction* hi = Emit(g, SetInstructionSize(
                         NewInstruction2(ARM_OP(ldr), ap_load,
                                         GetIntConstant(g, NULL, kTargetType32Bit, 4)),
                         kSize32Bit));
    SetLoweredHi(node, hi);
  } else {
    result = Emit(g, SetInstructionSize(
                         NewInstruction2(ARM_OP(ldr), ap_load,
                                         GetIntConstant(g, NULL, kTargetType32Bit, 0)),
                         data_size));
  }
  TargetInstruction* addi =
      Emit(g, NewInstruction2(ARM_OP(add), ap_load,
                               GetIntConstant(g, NULL, kTargetType32Bit,
                                              is_eight ? 8 : 4)));
  if (on_stack) {
    Emit(g, NewInstruction3(ARM_OP(str), addi, ap_addr, ap_offset));
  } else {
    TargetInstruction* mv = Emit(g, NewInstruction1(ARM_OP(mov), addi));
    mv->dest = ap_addr;
  }
  return SetLoweredNode(node, result);
}

static TargetInstruction* LowerBuiltinVaEnd(ARMGenerator* g, IRNode* node) {
  // Nothing to do for va_end.
  return NULL;
}

static TargetInstruction* LowerBuiltinVaCopy(ARMGenerator* g, IRNode* node) {
  return NULL;  // TODO
}

static TargetInstruction* LowerLocation(ARMGenerator* g, IRNode* node) {
  IRLocation* loc = (IRLocation*)node;
  return SetLoweredNode(node, Emit(g, TargetNewLocation(loc)));
}

static TargetInstruction* LowerStackPointerOps(ARMGenerator* g, IRNode* node) {
  switch (node->opcode) {
    case IR_OP(decsp): {
      // Dynamically lowering sp (VLA/alloca): the epilogue can no longer assume
      // a fixed distance between sp and the saved-register area, so it must
      // restore sp from the frame pointer.
      g->uses_dynamic_stack = true;
      TargetInstruction* size = Materialize(g, node->inputs.value.p[0]);
      TargetInstruction* new_sp;
      if (TargetIsConst(size)) {
        int64_t s = ARMIntValue(size);
        new_sp = AddImmediate(g, StackPointer(g), -s);
      } else {
        new_sp = Emit(g, CopyInstructionSize(NewInstruction2(ARM_OP(sub), StackPointer(g), size), 0));
      }
      new_sp->dest = StackPointer(g);
      return new_sp;
    }
    case IR_OP(savesp): {
      // One operand, a temp to hold stack pointer.
      TargetInstruction* tmp = Materialize(g, node->inputs.value.p[0]);
      TargetInstruction* mv = CopyInstructionSize(NewInstruction1(ARM_OP(mov), StackPointer(g)), 0);
      mv->dest = tmp;
      return tmp;
    }
    case IR_OP(restoresp): {
      TargetInstruction* tmp = Materialize(g, node->inputs.value.p[0]);
      TargetInstruction* mv = CopyInstructionSize(NewInstruction1(ARM_OP(mov), tmp), 0);
      mv->dest = StackPointer(g);
      return mv->dest;
    }
    default:
      assert(false);
      return NULL;
  }
}

static TargetInstruction* LowerIRNode(ARMGenerator* g, Generator* gen,
                                      IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  switch (node->opcode) {
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
      return Emit(g, NewInstruction1(ARM_OP(nrvoval), Materialize(g, node->inputs.value.p[0])));
      
    case IR_OP(ssavar):
    case IR_OP(phi):
      // We should never see these as we've moved out of SSA form
      // before here.
      break;

    case IR_OP(structreturn): {
      // Always allocate a saved register for the struct return value.
      // g->struct_return_reg = g->num_int_reg_vars++;
      TargetInstruction* result =
          SetLoweredNode(node, EmitSymbol(g, NewInstruction(ARM_OP(structreturn))));
      TargetInstruction* mv = Emit(g, NewInstruction1(ARM_OP(mov),
                  IntArgumentRegister(g, 0)));
      mv->dest = result;
      return result;
    }

    case IR_OP(literalref):
      return LowerLiteralReference(g, node);

    case IR_OP(addressof):
      return LowerAddressOf(g, node);

    case IR_OP(const32):
    case IR_OP(consta):
      return GetIntConstant(g, node, kTargetType32Bit,
                            ((IRConstant*)node)->value.ivalue);
    case IR_OP(const8):
      return GetIntConstant(g, node, kTargetType8Bit,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(const16):
      return GetIntConstant(g, node, kTargetType16Bit,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(const64):
      return GetIntConstant(g, node, kTargetType64Bit,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(constf):
      return GetFloatingPointConstant(g, node, kTargetTypeFloat,
                                      ((IRConstant*)node)->value.fvalue);

    case IR_OP(constd):
      return GetFloatingPointConstant(g, node, kTargetTypeDouble,
                                      ((IRConstant*)node)->value.fvalue);

    case IR_OP(enter):
      Emit(g, NewInstruction(ARM_OP(save)));
      LowerVariables(g, gen);
      return NULL;

    case IR_OP(leave):
      Emit(g, NewInstruction(ARM_OP(restore)));
      return NULL;

    case IR_OP(ret):
      return Emit(g, NewInstruction(ARM_OP(ret)));
      
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
      return LowerLoad(g, node);

      // stores.
    case IR_OP(store32):
    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store64):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
      return LowerStore(g, node);

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
      return LowerInc(g, node);
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
    return LowerDec(g, node);
      
    case IR_OP(getbit):
      return LowerGetBitField(g, node);
      
    case IR_OP(setbit):
      return LowerSetBitField(g, node);

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
      return LowerExpression(g, node);
      
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
      return LowerComparison(g, node);

    case IR_OP(btrue):
    case IR_OP(bfalse):
      return LowerConditionalBranch(g, node);

    case IR_OP(bra):
      return LowerBranch(g, node);

    case IR_OP(cbra):
      return LowerComputedBranch(g, node);

    case IR_OP(label):
      return LowerLabel(g, node);

    case IR_OP(named_label):
      return LowerNamedLabel(g, node);

    case IR_OP(pusharg):
      if (NodeIsWideInt(node)) {
        // Propagate both halves of a 64-bit integer argument so the call's
        // register-pair lowering (MaterializeWide) can find the high half.
        TargetInstruction* lo;
        TargetInstruction* hi;
        MaterializeWide(g, node->inputs.value.p[0], &lo, &hi);
        SetLoweredHi(node, hi);
        return SetLoweredNode(node, lo);
      }
      return SetLoweredNode(node, Materialize(g, node->inputs.value.p[0]));
      
    case IR_OP(calla):
      return LowerCall(g, node);

    case IR_OP(structarg):
      // Same as its input.
      return SetLoweredNode(node, Materialize(g, node->inputs.value.p[0]));
      
    case IR_OP(resulti):
    case IR_OP(resultf):
    case IR_OP(resultd):
    case IR_OP(resulta):
      return LowerResult(g, node);

    case IR_OP(memzero):
      return LowerMemzero(g, node);

    case IR_OP(memcpy):
      return LowerMemcpy(g, node);

    case IR_OP(cast): {
      TargetInstruction* inst = Materialize(g, node->inputs.value.p[0]);
      // Honor a "-> $n" merge destination (the temp used to merge the arms of
      // && / || / ?:).  Without this, a cast appearing in one arm (e.g.
      // `cond ? NULL : (char*)s`) never writes the merge temp, so the consumer
      // reads a stale register.
      TargetInstruction* dest = GetDestInstruction(g, node);
      if (dest != NULL) {
        inst = SetDestOrMove(g, inst, dest, ARM_OP(mov));
      }
      return SetLoweredNode(node, inst);
    }

    case IR_OP(zeroextendi):
      return LowerZeroExtend(g, node);

    case IR_OP(signextendi):
      return LowerSignExtend(g, node);

    case IR_OP(aligni):
       return LowerAlign(g, node);

    case IR_OP(asm):;
      return LowerAsm(g, node);

    case IR_OP(loc):
      return LowerLocation(g, node);

    case IR_OP(builtin_va_start):
      return LowerBuiltinVaStart(g, node);

    case IR_OP(builtin_va_arg):
      return LowerBuiltinVaArg(g, node);

    case IR_OP(builtin_va_end):
      return LowerBuiltinVaEnd(g, node);

    case IR_OP(builtin_va_copy):
      return LowerBuiltinVaCopy(g, node);
      
    case IR_OP(decsp):
    case IR_OP(savesp):
    case IR_OP(restoresp):
      return LowerStackPointerOps(g, node);
  }

  // If we get here we've failed to handle the IR node.
  printf("Unhandled IR OP %s\n", IROpcodeName(node->opcode));
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
static ArgLocation ArgumentLocation(PoolEntry* arg, Vector* args) {
  IRVariable* var = (IRVariable*)arg->pooled;
  size_t arg_num = var->symbol->value.arg_number;
  bool is_struct_return = TypeIsStructOrUnion(
      compiler->current_function->info.function.symbol->type->next);
  int int_reg = ARM_INT_ARG_START;
  if (is_struct_return) {
    int_reg +=
        1;  // For struct returns, the first arg is the address of the struct.
  }
  int fp_reg = ARM_FP_ARG_START;
  // Stack-passed arguments follow AAPCS packing (4-byte words; 8-byte-aligned
  // 8-byte scalars), matching the caller (LowerCall via ArgStackSlot) and
  // va_arg.  stack_offset tracks the next free byte; the requested argument's
  // slot is computed by aligning for its own type.
  size_t stack_offset = 0;
  for (size_t i = 0; i < args->length; i++) {
    if (i == arg_num) {
      ArgLocation location;
      if (TypeIsFloatingPoint(arg->pooled->type)) {
        // Each floating-point argument (float or double) consumes one full
        // double-precision argument register.  fp_reg counts d-register indices
        // (d0..d3), matching the caller side in LowerCall.
        if (fp_reg < ARM_NUM_FP_ARGS) {
          location.type = kArgLocationRegister;
          location.location.offset = fp_reg;
        } else {
          location.type = kArgLocationPushed;
          location.location.offset =
              (stack_offset + ArgStackAlignment(arg->pooled->type) - 1) &
              ~(ArgStackAlignment(arg->pooled->type) - 1);
        }
      } else if (TypeIsWideInt(arg->pooled->type)) {
        // 64-bit integer: even-aligned register pair (r0:r1 / r2:r3) or stack.
        if ((int_reg - ARM_INT_ARG_START) % 2 != 0) {
          int_reg++;
        }
        if (int_reg + 1 <= ARM_INT_ARG_END) {
          location.type = kArgLocationIntPair;
          location.location.offset = int_reg;  // low reg; high = int_reg + 1.
        } else {
          location.type = kArgLocationPushed;
          location.location.offset =
              (stack_offset + ArgStackAlignment(arg->pooled->type) - 1) &
              ~(ArgStackAlignment(arg->pooled->type) - 1);
        }
      } else if (TypeIsStructOrUnion(arg->pooled->type)) {
        // Named struct/union: always passed by value in the stacked-argument
        // area (matching the caller in LowerCall).  fp + offset addresses it.
        location.type = kArgLocationPushed;
        location.location.offset =
            (stack_offset + ArgStackAlignment(arg->pooled->type) - 1) &
            ~(ArgStackAlignment(arg->pooled->type) - 1);
      } else {
        if (int_reg <= ARM_INT_ARG_END) {
          // Arg is in an integer register.
          location.type = kArgLocationRegister;
          location.location.offset = int_reg;
        } else {
          location.type = kArgLocationPushed;
          location.location.offset =
              (stack_offset + ArgStackAlignment(arg->pooled->type) - 1) &
              ~(ArgStackAlignment(arg->pooled->type) - 1);
        }
      }
      return location;
    }

    // Advance the per-type register counter (or the stack offset once the
    // argument registers are exhausted) for each preceding argument.  This must
    // use the *argument*-register limits (r0..r3 / s0..s3) -- the same bounds
    // the per-argument decision above uses.  Using the wider callee-saved range
    // (ARM_LAST_INT_REG_VAR) would keep incrementing the register counter past
    // r3 and never advance stack_offset, so every stacked argument would be read
    // from the same (first) stack slot.
    Symbol* arg_symbol = args->value.p[i];
    if (TypeIsFloatingPoint(arg_symbol->type)) {
      if (fp_reg < ARM_NUM_FP_ARGS) {
        fp_reg++;
      } else {
        ArgStackSlot(arg_symbol->type, &stack_offset);
      }
    } else if (TypeIsWideInt(arg_symbol->type)) {
      if ((int_reg - ARM_INT_ARG_START) % 2 != 0) {
        int_reg++;
      }
      if (int_reg + 1 <= ARM_INT_ARG_END) {
        int_reg += 2;
      } else {
        int_reg = ARM_INT_ARG_END + 1;
        ArgStackSlot(arg_symbol->type, &stack_offset);
      }
    } else if (TypeIsStructOrUnion(arg_symbol->type)) {
      // Named struct/union: always consumes a stacked-argument slot (it is
      // never placed in a core register), matching the caller.
      ArgStackSlot(arg_symbol->type, &stack_offset);
    } else {
      if (int_reg <= ARM_INT_ARG_END) {
        int_reg++;
      } else {
        ArgStackSlot(arg_symbol->type, &stack_offset);
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

static TargetInstruction* LoadFpArgumentIntoRegisterVariable(ARMGenerator* g,
                                                             int reg_var,
                                             ArgLocation arg_loc,
                                             IRNode* symbol) {
  switch (arg_loc.type) {
  case kArgLocationRegister:
    case kArgLocationPassedByReferenceInRegister: {
      IRVariable* sym = (IRVariable*)symbol;
      TargetInstruction* var = FloatingPointVariableRegister(g, reg_var, sym->symbol);
      // The move's destination must be the variable register (not an operand):
      // the register allocator only binds a variable register when it is the
      // `dest` of a defining move (see AllocateRegister / AllocateForRmov),
      // mirroring the integer argument path below.
      TargetInstruction* mv = Emit(g, NewInstruction1(ARM_OP(fmov),
                FloatingPointArgumentRegister(g,
                    (int)arg_loc.location.offset - ARM_FP_ARG_START)));
      SetInstructionSize(mv,
                         TypeIsDouble(symbol->type) ? kSize64Bit : kSize32Bit);
      mv->dest = var;
      return var;
    }
    case kArgLocationPushed:
    case kArgLocationPassedByReferenceOnStack:
      return PopArg(g, symbol, arg_loc.location.offset);
  }
}

static TargetInstruction* LoadIntArgumentIntoRegisterVariable(ARMGenerator* g,
                                                              int reg_var,
                                             ArgLocation arg_loc,
                                             IRNode* symbol) {
  switch (arg_loc.type) {
  case kArgLocationRegister:
    case kArgLocationPassedByReferenceInRegister: {
      IRVariable* sym = (IRVariable*)symbol;
      TargetInstruction* var = IntVariableRegister(g, reg_var, sym->symbol);
      TargetInstruction* mv = Emit(g, NewInstruction1(ARM_OP(mov),
                 IntArgumentRegister(g,
                          (int)arg_loc.location.offset - ARM_INT_ARG_START)));
      mv->dest = var;
      return var;
    }
    case kArgLocationPassedByReferenceOnStack:
    case kArgLocationPushed:
      return PopArg(g, symbol, arg_loc.location.offset);
  }
}

// Assign a register to a variable or argument if possible.  The
// var_offset is below the stack frame.
static void AssignRegisterOrOffset(ARMGenerator* g, PoolEntry* entry,
                                   Vector* args, int* var_offset) {
  // Variable length arrays are not given offsets until their block
  // is entered.
  if (TypeIsVLA(entry->pooled->type)) {
    return;
  }
  bool is_arg = entry->pooled->opcode == IR_OP(argument);
  int64_t size =
      is_arg ? CalculateArgumentSize(entry->pooled) : entry->pooled->type->size;
  assert(size != 0);

  // An argument passed on the stack is addressed relative to the frame pointer,
  // so the function must set one up (see EmptyStackFrame / OmitFramePointer).
  if (is_arg) {
    ArgLocation loc = ArgumentLocation(entry, args);
    if (loc.type == kArgLocationPushed ||
        loc.type == kArgLocationPassedByReferenceOnStack) {
      g->has_stack_args = true;
    }
  }

  // printf("var %s\n", ((IRVariable*)entry->pooled)->symbol->name.value);
  if (TypeIsFloatingPoint(entry->pooled->type)) {
    // See the integer case below: a stack-passed argument has no entry load that
    // assigns its register, so it must not be promoted to a register variable.
    bool arg_on_stack = false;
    if (is_arg) {
      ArgLocation location = ArgumentLocation(entry, args);
      arg_on_stack = location.type == kArgLocationPushed;
    }
    if (UseRegisterForVariable(g, entry->pooled) && !arg_on_stack) {
      int reg = g->num_fp_reg_vars++;
      entry->pooled->data.ivalue = ARM_REG_VAR | reg;
      SetDebugRegisterLocation(entry, reg);
      if (is_arg) {
        ArgLocation location = ArgumentLocation(entry, args);
        LoadFpArgumentIntoRegisterVariable(g, reg, location, entry->pooled);
      }
    } else {
      if (is_arg) {
        ArgLocation location = ArgumentLocation(entry, args);
        if (location.type == kArgLocationRegister) {
          // Saved argument registers live immediately below the frame pointer.
          // The base must equal -(ARM_STACK_FRAME_HEADER_SIZE + 8) so that this
          // region tiles exactly with the local-variable region (which is placed
          // at var_offset - stack_frame_size - ARM_STACK_FRAME_HEADER_SIZE). With
          // an 8-byte header that base is -16; using -24 (the aarch64 value, for
          // a 16-byte header) makes locals overlap the saved-arg slots.
          int saved_size = ARMTypeIsDouble(entry->pooled->type) ? 8 : 4;
          int offset = AllocateSavedArgumentHome(g, saved_size, saved_size);
          // location.offset is a d-register index; the physical register file is
          // numbered in s-register units, so the even low half is index*2.
          SavedArgumentRegister* saved = NewSavedArgumentRegister(
              (int)location.location.offset * 2, ARM_FP_REG, offset, true);
          saved->is_double = ARMTypeIsDouble(entry->pooled->type);
          entry->pooled->data.ivalue = offset;
          VectorAppend(&g->saved_regs, saved);
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        } else {
          // Floating-point argument passed on the stack (above the frame
          // pointer); fp points directly at the first incoming stack argument.
          entry->pooled->data.ivalue = (int)location.location.offset;
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        }
      } else {
        AlignOffset(entry, var_offset);
        entry->pooled->data.ivalue = *var_offset;
        SetDebugStackLocation(entry, *var_offset);
        *var_offset += size;
      }
    }
  } else if (TypeIsStructOrUnion(entry->pooled->type)) {
    if (is_arg) {
      // Named struct/union arguments are passed by value in the stacked
      // argument area (see LowerCall / ArgumentLocation).  The argument lives
      // at fp + offset; record that positive offset directly (the argument
      // addressing path in LowerExpression treats it as fp-relative).
      ArgLocation location = ArgumentLocation(entry, args);
      entry->pooled->data.ivalue = (int)location.location.offset;
      SetDebugStackLocation(entry, entry->pooled->data.ivalue);
    } else {
      // Not an argument.
      // TODO: it is possible to put small structs in registers.
      if ((entry->pooled->flags & kIRNrvoMarker) != 0) {
        // Named RVO symbol.  This assigned the same register as the
        // structreturn.
        entry->pooled->data.ivalue = ARM_REG_VAR | g->struct_return_reg;
        TargetInstruction* var = IntVariableRegister(g, g->struct_return_reg, entry->value.symbol);
        Emit(g, NewInstruction2(ARM_OP(mov), var,
                    IntArgumentRegister(g, 0)));
        SetDebugRegisterLocation(entry, g->struct_return_reg);
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
    TargetInstruction* inst = GetSymbol(g, NULL, var->symbol);
    entry->pooled->data.ptr = inst;
    SetDebugSymbolLocation(entry);
  } else {
    // Integer or pointer.
    // A stack-passed argument cannot be promoted to a register variable here:
    // there is no entry load that assigns the variable's register (only the
    // register-passed path emits a move whose dest is the variable), so it would
    // reach emission with no register.  Leave such arguments on the stack.
    bool arg_on_stack = false;
    if (is_arg) {
      ArgLocation location = ArgumentLocation(entry, args);
      arg_on_stack = location.type == kArgLocationPushed;
    }
    if (UseRegisterForVariable(g, entry->pooled) && !arg_on_stack) {
      int reg = g->num_int_reg_vars++;
      entry->pooled->data.ivalue = ARM_REG_VAR | reg;
      SetDebugRegisterLocation(entry, reg);
      if (is_arg) {
        // Argument, load it into a register.
        ArgLocation location = ArgumentLocation(entry, args);
        LoadIntArgumentIntoRegisterVariable(g,
                                            reg, location, entry->pooled);
      }
    } else {
      if (is_arg) {
        // The argument is not going to be placed in a register.  We need
        // to make sure it's on the stack.  It is already on the stack
        // if it is not passed in x0..x7.  But if is in an arg reg
        // we need to save it to the stack frame.
        ArgLocation location = ArgumentLocation(entry, args);
        if (location.type == kArgLocationIntPair) {
          // 64-bit integer passed in an even-aligned register pair.  Save both
          // halves into a single 8-byte slot below the frame pointer (low at
          // offset, high at offset+4) so load64 can read [fp+off]/[fp+off+4].
          int offset = AllocateSavedArgumentHome(g, 8, 8);
          int lo_reg = (int)location.location.offset;
          VectorAppend(&g->saved_regs,
                       NewSavedArgumentRegister(lo_reg, ARM_FP_REG, offset, false));
          VectorAppend(&g->saved_regs,
                       NewSavedArgumentRegister(lo_reg + 1, ARM_FP_REG, offset + 4,
                                                false));
          entry->pooled->data.ivalue = offset;
          g->num_int_arg_regs += 2;
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        } else if (location.type == kArgLocationRegister) {
          // Argument is in a register so we need to save it to the stack. These
          // are stored immediately below the saved frame pointer. The base must
          // be -(ARM_STACK_FRAME_HEADER_SIZE + 8) = -16 so the saved-arg region
          // tiles with the local-variable region without overlapping (locals are
          // placed at var_offset - stack_frame_size - ARM_STACK_FRAME_HEADER_SIZE).
          int offset = AllocateSavedArgumentHome(g, 4, 4);
          SavedArgumentRegister* saved = NewSavedArgumentRegister(
              (int)location.location.offset, ARM_FP_REG, offset, false);
          entry->pooled->data.ivalue = offset;
          VectorAppend(&g->saved_regs, saved);
          g->num_int_arg_regs++;  // Argument was passed in a register.
          SetDebugStackLocation(entry, entry->pooled->data.ivalue);
        } else {
          // Argument was passed on the stack (above the frame pointer).  The
          // prologue does "add fp, sp, #ARM_STACK_FRAME_HEADER_SIZE", so fp
          // points directly at the first incoming stack argument; its location
          // offset (0, 8, 16, ...) is the fp-relative offset to use.
          entry->pooled->data.ivalue = (int)location.location.offset;
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

static void AssignRegisterVars(ARMGenerator* g, Vector* vars, Vector* args) {
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
    AssignRegisterOrOffset(g, entry, args, &var_offset);
  }
  
  // We now know the stack frame size.  This includes the length of the saved
  // registers.
  g->base.stack_frame_size = (int32_t)var_offset + g->saved_arg_size;
  // Align to 8 byte boundary (AAPCS).
  g->base.stack_frame_size = (g->base.stack_frame_size + 7) & ~7;
}

static void LowerVariables(ARMGenerator* g, Generator* gen) {
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
        TargetInstruction* inst = GetSymbol(g, NULL, var->symbol);
        entry->pooled->data.ptr = inst;
        break;
      }
    }
  }

  AssignRegisterVars(g, &local_vars,
                     &compiler->current_function->info.function.prototype);
  VectorDestruct(&local_vars);
}

void ARMLower(ARMGenerator* g, Generator* gen) {
  TrapLower(&gen->func->info.function.symbol->name);
  
  // If the function returns a struct, allocate the struct result
  // register now.
  if (TypeIsStructOrUnion(gen->func->next)) {
    g->struct_return_reg = g->num_int_reg_vars++;
  }
  
  IRNode* node = GeneratorFirstInstruction(gen);
  while (node != NULL) {
    LowerIRNode(g, gen, node);
    node = IRNext(node);
  }

  if (compiler->print_back_end|| compiler->ir_output_file != stdout) {
    ARMPrint(g, compiler->ir_output_file);
  }
  
  // Build basic blocks for.
  TargetBuildBasicBlocks(&g->base);
  
  if (compiler->print_back_end|| compiler->ir_output_file != stdout) {
    TargetPrintBasicBlocks(&g->base, compiler->ir_output_file);
  }
  
  if (OptLevel2()) {
    // Optimize the code sequence for -O2 and above.
    ARMOptimize(g);
  
    if (compiler->print_back_end|| compiler->ir_output_file != stdout) {
      fprintf(compiler->ir_output_file, "\n After ARM optimization\n");
      TargetPrintBasicBlocks(&g->base, compiler->ir_output_file);
    }
  }
  // Allocate registers to the instructions.
  ARMAllocateRegisters(&g->register_allocator);
}

void ARMPrint(ARMGenerator* g, FILE* fp) {
  TargetInstruction* inst = TargetFirstInstruction(&g->base);
  while (inst != NULL) {
    TargetPrintInstruction(inst, ARMOpcodeName, fp);
    inst = TargetNext(inst);
  }
}
