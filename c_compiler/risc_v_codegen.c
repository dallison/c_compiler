//
//  risc_v_codegen.c
//  c_compiler_library
//
//  Created by David Allison on 2/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v_codegen.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "debug.h"

#include "risc_v_optimize.h"

const char* RVOpcodeName(int op) {
  RVOpcode opcode = (RVOpcode)op;
  switch (opcode) {
    default:
      return TargetOpcodeName(opcode);

    case RV_OP(mv):
      return "mv";
    case RV_OP(fmv_s):
      return "fmv.s";
    case RV_OP(fmv_d):
      return "fmv.d";

    // RV32I instructions.
    case RV_OP(lui):
      return "lui";
    case RV_OP(auipc):
      return "auipc";
    case RV_OP(jal):
      return "jal";
    case RV_OP(jalr):
      return "jalr";
    case RV_OP(beq):
      return "beq";
    case RV_OP(bne):
      return "bne";
    case RV_OP(blt):
      return "blt";
    case RV_OP(bge):
      return "bge";
    case RV_OP(bltu):
      return "bltu";
    case RV_OP(bgeu):
      return "bgeu";
    case RV_OP(lb):
      return "lb";
    case RV_OP(lh):
      return "lh";
    case RV_OP(lw):
      return "lw";
    case RV_OP(lbu):
      return "lbu";
    case RV_OP(lhu):
      return "lhu";
    case RV_OP(sb):
      return "sb";
    case RV_OP(sh):
      return "sh";
    case RV_OP(sw):
      return "sw";
    case RV_OP(addi):
      return "addi";
    case RV_OP(slti):
      return "slti";
    case RV_OP(sltiu):
      return "sltiu";
    case RV_OP(xori):
      return "xori";
    case RV_OP(ori):
      return "ori";
    case RV_OP(andi):
      return "andi";
    case RV_OP(slli):
      return "slli";
    case RV_OP(srli):
      return "srli";
    case RV_OP(srai):
      return "srai";
    case RV_OP(add):
      return "add";
    case RV_OP(sub):
      return "sub";
    case RV_OP(sll):
      return "sll";
    case RV_OP(slt):
      return "slt";
    case RV_OP(sltu):
      return "sltu";
    case RV_OP(xor):
      return "xor";
    case RV_OP(srl):
      return "srl";
    case RV_OP(sra):
      return "sra";
    case RV_OP(or):
      return "or";
    case RV_OP(and):
      return "and";
    case RV_OP(fence):
      return "fence";
    case RV_OP(fence_i):
      return "fence.i";
    case RV_OP(ecall):
      return "ecall";
    case RV_OP(ebreak):
      return "ebreak";
    case RV_OP(csrrw):
      return "csrrw";
    case RV_OP(csrrs):
      return "csrrs";
    case RV_OP(csrrc):
      return "csrrc";
    case RV_OP(csrrwi):
      return "csrrwi";
    case RV_OP(csrrsi):
      return "csrrsi";
    case RV_OP(csrrci):
      return "csrrci";

    // RV64I instructions.
    case RV_OP(lwu):
      return "lwu";
    case RV_OP(ld):
      return "ld";
    case RV_OP(sd):
      return "sd";
    case RV_OP(addiw):
      return "addiw";
    case RV_OP(slliw):
      return "slliw";
    case RV_OP(srliw):
      return "srliw";
    case RV_OP(sraiw):
      return "sraiw";
    case RV_OP(addw):
      return "addw";
    case RV_OP(subw):
      return "subw";
    case RV_OP(sllw):
      return "sllw";
    case RV_OP(srlw):
      return "srlw";
    case RV_OP(sraw):
      return "sraw";

    // RV32M instructions.
    case RV_OP(mul):
      return "mul";
    case RV_OP(mulh):
      return "mulh";
    case RV_OP(mulhsu):
      return "mulhsu";
    case RV_OP(mulhu):
      return "mulhu";
    case RV_OP(div):
      return "div";
    case RV_OP(divu):
      return "divu";
    case RV_OP(rem):
      return "rem";
    case RV_OP(remu):
      return "remu";

    // RV64M instructions.
    case RV_OP(mulw):
      return "mulw";
    case RV_OP(divw):
      return "divw";
    case RV_OP(divuw):
      return "divuw";
    case RV_OP(remw):
      return "remw";
    case RV_OP(remuw):
      return "remuw";

    // RV32F instructions.
    case RV_OP(flw):
      return "flw";
    case RV_OP(fsw):
      return "fsw";
    case RV_OP(fmadd_s):
      return "fmadd.s";
    case RV_OP(fmsub_s):
      return "fmsub.s";
    case RV_OP(fnmsub_s):
      return "fnmsub.s";
    case RV_OP(fnmadd_s):
      return "fnmadd.s";
    case RV_OP(fadd_s):
      return "fadd.s";
    case RV_OP(fsub_s):
      return "fsub.s";
    case RV_OP(fmul_s):
      return "fmul.s";
    case RV_OP(fdiv_s):
      return "fdiv.s";
    case RV_OP(fsqrt_s):
      return "fsqrt.s";
    case RV_OP(fsgnj_s):
      return "fsgnj.s";
    case RV_OP(fsgnjn_s):
      return "fsgnjn.s";
    case RV_OP(fsgnjx_s):
      return "fsgnjx.s";
    case RV_OP(fmin_s):
      return "fmin.s";
    case RV_OP(fmax_s):
      return "fmax.s";
    case RV_OP(fcvt_w_s):
      return "fcvt.w.s";
    case RV_OP(fcvt_wu_s):
      return "fcvt.wu.s";
    case RV_OP(fmv_x_w):
      return "fmv.x.w";
    case RV_OP(feq_s):
      return "feq.s";
    case RV_OP(flt_s):
      return "flt.s";
    case RV_OP(fle_s):
      return "fle.s";
    case RV_OP(fclass_s):
      return "fclass.s";
    case RV_OP(fcvt_s_w):
      return "fcvt.s.w";
    case RV_OP(fcvt_s_wu):
      return "fcvt.s.wu";
    case RV_OP(fmv_w_x):
      return "fmv.w.x";

    // RV64F instructions.
    case RV_OP(fcvt_l_s):
      return "fcvt.l.s";
    case RV_OP(fcvt_lu_s):
      return "fcvt.lu.s";
    case RV_OP(fcvt_s_l):
      return "fcvt.s.l";
    case RV_OP(fcvt_s_lu):
      return "fcvt.s.lu";

    // RV32D instructions.
    case RV_OP(fld):
      return "fld";
    case RV_OP(fsd):
      return "fsd";
    case RV_OP(fmadd_d):
      return "fmadd.d";
    case RV_OP(fmsub_d):
      return "fmsub.d";
    case RV_OP(fnmsub_d):
      return "fnmsub.d";
    case RV_OP(fnmadd_d):
      return "fnmadd.d";
    case RV_OP(fadd_d):
      return "fadd.d";
    case RV_OP(fsub_d):
      return "fsub.d";
    case RV_OP(fmul_d):
      return "fmul.d";
    case RV_OP(fdiv_d):
      return "fdiv.d";
    case RV_OP(fsqrt_d):
      return "fsqrt.d";
    case RV_OP(fsgnj_d):
      return "fsgnj.d";
    case RV_OP(fsgnjn_d):
      return "fsgnjn.d";
    case RV_OP(fsgnjx_d):
      return "fsgnjx.d";
    case RV_OP(fmin_d):
      return "fmin.d";
    case RV_OP(fmax_d):
      return "fmax.d";
    case RV_OP(fcvt_s_d):
      return "fcvt.s.d";
    case RV_OP(fcvt_d_s):
      return "fcvt.d.s";
    case RV_OP(feq_d):
      return "feq.d";
    case RV_OP(flt_d):
      return "flt.d";
    case RV_OP(fle_d):
      return "fle.d";
    case RV_OP(fclass_d):
      return "fclass.d";
    case RV_OP(fcvt_w_d):
      return "fcvt.w.d";
    case RV_OP(fcvt_wu_d):
      return "fcvt.wu.d";
    case RV_OP(fcvt_d_w):
      return "fcvt.d.w";
    case RV_OP(fcvt_d_wu):
      return "fcvt.d.wu";

    // RV64D instructions.
    case RV_OP(fcvt_l_d):
      return "fcvt.l.d";
    case RV_OP(fcvt_lu_d):
      return "fcvt.lu.d";
    case RV_OP(fmv_x_d):
      return "fmv.x.d";
    case RV_OP(fcvt_d_l):
      return "fcvt.d.l";
    case RV_OP(fcvt_d_lu):
      return "fcvt.d.lu";
    case RV_OP(fmv_d_x):
      return "fmv.d.x";

    case RV_OP(nop):
      return "nop";
    case RV_OP(not):
      return "not";
    case RV_OP(neg):
      return "neg";
    case RV_OP(fneg_s):
      return "fneg.s";
    case RV_OP(fneg_d):
      return "fneg.d";
    case RV_OP(li):
      return "li";
    case RV_OP(seqz):
      return "seqz";
    case RV_OP(snez):
      return "snez";
    case RV_OP(sltz):
      return "sltz";
    case RV_OP(sgtz):
      return "sz";
    case RV_OP(beqz):
      return "beqz";
    case RV_OP(bnez):
      return "bnez";
    case RV_OP(j):
      return "j";
    case RV_OP(call):
      return "call";
    case RV_OP(rcall):
      return "rcall";

    case RV_OP(callf):
      return "callf";
    case RV_OP(rcallf):
      return "rcallf";

    case RV_OP(la):
      return "la";
    case RV_OP(lla):
        return "lla";
    case RV_OP(sext_w):
      return "sext.w";

      // Integer argument registers.
    case RV_OP(a0):
      return "a0";
    case RV_OP(a1):
      return "a1";
    case RV_OP(a2):
      return "a2";
    case RV_OP(a3):
      return "a3";
    case RV_OP(a4):
      return "a4";
    case RV_OP(a5):
      return "a5";
    case RV_OP(a6):
      return "a6";
    case RV_OP(a7):
      return "a7";

      // Floating point argument registers.
    case RV_OP(fa0):
      return "fa0";
    case RV_OP(fa1):
      return "fa1";
    case RV_OP(fa2):
      return "fa2";
    case RV_OP(fa3):
      return "fa3";
    case RV_OP(fa4):
      return "fa4";
    case RV_OP(fa5):
      return "fa5";
    case RV_OP(fa6):
      return "fa6";
    case RV_OP(fa7):
      return "fa7";

    // NOTE: these must match the number of int and fp reg vars.
    case RV_OP(v0):
      return "v0";
    case RV_OP(v1):
      return "v1";
    case RV_OP(v2):
      return "v2";
    case RV_OP(v3):
      return "v3";
    case RV_OP(v4):
      return "v4";
    case RV_OP(v5):
      return "v5";

    case RV_OP(fv0):
      return "fv0";
    case RV_OP(fv1):
      return "fv1";
    case RV_OP(fv2):
      return "fv2";
    case RV_OP(fv3):
      return "fv3";
    case RV_OP(fv4):
      return "fv4";
    case RV_OP(fv5):
      return "fv5";

    case RV_OP(regarg):
      return "regarg";

    case RV_OP(x0):
      return "x0";
  }
}

bool RVIsExpression(RVOpcode opcode) {
  switch (opcode) {
    case RV_OP(label):
    case RV_OP(asm):
    case RV_OP(jal):
    case RV_OP(jalr):
    case RV_OP(beq):
    case RV_OP(bne):
    case RV_OP(blt):
    case RV_OP(bge):
    case RV_OP(bltu):
    case RV_OP(bgeu):
    case RV_OP(beqz):
    case RV_OP(bnez):
    case RV_OP(j):
    case RV_OP(call):
    case RV_OP(rcall):
    case RV_OP(callf):
    case RV_OP(rcallf):
    case RV_OP(ret):
    case RV_OP(save):
    case RV_OP(restore):
    case RV_OP(rmov):
    case RV_OP(rmovf):
    case RV_OP(rmovd):
    case RV_OP(sb):
    case RV_OP(sw):
    case RV_OP(sh):
    case RV_OP(sd):
    case RV_OP(fsw):
    case RV_OP(fsd):
    case RV_OP(loc):
    case RV_OP(named_label):
      return false;
    default:
      return true;
  }
}

bool RVIsLoad(RVOpcode opcode) {
  switch (opcode) {
    case RV_OP(lb):
    case RV_OP(lw):
    case RV_OP(ld):
    case RV_OP(lbu):
    case RV_OP(lwu):
    case RV_OP(lh):
    case RV_OP(lhu):
    case RV_OP(flw):
    case RV_OP(fld):
      return true;
    default:
      return false;
  }
}

bool RVIsSignedLoad(RVOpcode opcode) {
  switch (opcode) {
    case RV_OP(lb):
    case RV_OP(lw):
    case RV_OP(lh):
    case RV_OP(ld):
      return true;
    default:
      return false;
  }
}

bool RVIsStore(RVOpcode opcode) {
  switch (opcode) {
    case RV_OP(sb):
    case RV_OP(sw):
    case RV_OP(sd):
    case RV_OP(lh):
    case RV_OP(fsw):
    case RV_OP(fsd):
      return true;
    default:
      return false;
  }
}

bool RVIsIntConst(RVOpcode opcode) {
  switch (opcode) {
    case RV_OP(constw):
    case RV_OP(constb):
    case RV_OP(consth):
    case RV_OP(constx):
      return true;
    default:
      return false;
  }
}

bool RVIsFixedRegister(RVOpcode opcode) {
  switch (opcode) {
    case RV_OP(a0):
    case RV_OP(a1):
    case RV_OP(a2):
    case RV_OP(a3):
    case RV_OP(a4):
    case RV_OP(a5):
    case RV_OP(a6):
    case RV_OP(a7):
    case RV_OP(fa0):
    case RV_OP(fa1):
    case RV_OP(fa2):
    case RV_OP(fa3):
    case RV_OP(fa4):
    case RV_OP(fa5):
    case RV_OP(fa6):
    case RV_OP(fa7):
    case RV_OP(v0):
    case RV_OP(v1):
    case RV_OP(v2):
    case RV_OP(v3):
    case RV_OP(fv0):
    case RV_OP(fv1):
    case RV_OP(fv2):
    case RV_OP(fv3):
    case RV_OP(x0):
    case RV_OP(fp):

      // Calls always return in a0 or fa0.
    case RV_OP(call):
    case RV_OP(rcall):
    case RV_OP(callf):
    case RV_OP(rcallf):
      return true;
    default:
      return false;
  }
}

int RVIntValue(TargetInstruction* inst) {
  if (inst->opcode == (TargetOpcode)RV_OP(x0)) {
    return 0;
  }
  return (int)((TargetConstant*)inst)->value.ivalue;
}

// Is the value small enough to be encoded in an immediate field?
bool RVIsPossibleImmediate(int64_t value) {
  // Check for 12 bit signed immediate.
  if (value < 0) {
    return value >= -2048;
  }
  return value < 2048;
}


void RVGeneratorInit(RVGenerator* rv, Generator* gen) {
  TargetGeneratorInit(&rv->base, gen);

  rv->num_int_arg_regs = 0;
  rv->num_fp_arg_regs = 0;
  rv->num_int_reg_vars = 0;
  rv->num_fp_reg_vars = 0;
  rv->struct_return_reg = -1;
  rv->use_reg_vars = false;
  rv->zero = NULL;
  memset(rv->int_argument_registers, 0, sizeof(rv->int_argument_registers));
  memset(rv->fp_argument_registers, 0, sizeof(rv->fp_argument_registers));
  memset(rv->int_variable_registers, 0, sizeof(rv->int_variable_registers));
  memset(rv->fp_variable_registers, 0, sizeof(rv->fp_variable_registers));
  VectorInit(&rv->saved_regs);
  VectorInit(&rv->register_loads);
  VectorInit(&rv->offsets);

  RVRegisterAllocatorInit(&rv->register_allocator, rv);
}

RVGenerator* NewRVGenerator(Generator* gen) {
  RVGenerator* rv = malloc(sizeof(RVGenerator));
  RVGeneratorInit(rv, gen);
  return rv;
}

void RVGeneratorDestruct(RVGenerator* rv) {
  TargetGeneratorDestruct(&rv->base);
  VectorDestructWithContents(&rv->saved_regs, NULL);
  VectorDestructWithContents(&rv->register_loads, NULL);
  VectorDestructWithContents(&rv->offsets, NULL);
  RVRegisterAllocatorDestruct(&rv->register_allocator);
}

void RVGeneratorDelete(RVGenerator* rv) {
  RVGeneratorDestruct(rv);
  free(rv);
}

static SavedArgumentRegister* NewSavedArgumentRegister(int reg_num,
                                                       int base_reg_num,
                                                       int offset) {
  SavedArgumentRegister* reg = malloc(sizeof(SavedArgumentRegister));
  reg->base_reg_num = base_reg_num;
  reg->reg_num = reg_num;
  reg->offset = offset;
  return reg;
}

static void SavedArgumentRegisterDelete(SavedArgumentRegister* reg) {
  free(reg);
}

// Some static utility functions that map to generic target functions.
static TargetInstruction* NewInstruction1(RVOpcode opcode,
                                          TargetInstruction* op1) {
  return TargetNewInstruction1((TargetOpcode)opcode, op1);
}

static TargetInstruction* NewInstruction2(RVOpcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2) {
  return TargetNewInstruction2((TargetOpcode)opcode, op1, op2);
}

static TargetInstruction* NewInstruction3(RVOpcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2,
                                          TargetInstruction* op3) {
  return TargetNewInstruction3((TargetOpcode)opcode, op1, op2, op3);
}

static TargetInstruction* Emit(RVGenerator* rv, TargetInstruction* inst) {
  return TargetEmit(&rv->base, inst);
}

static TargetInstruction* EmitBefore(RVGenerator* rv, TargetInstruction* inst,
                                     TargetInstruction* pos) {
  return TargetEmitBefore(&rv->base, inst, pos);
}

static TargetInstruction* EmitAfter(RVGenerator* rv, TargetInstruction* inst,
                                    TargetInstruction* pos) {
  return TargetEmitAfter(&rv->base, inst, pos);
}

static TargetInstruction* EmitConstant(RVGenerator* rv, TargetInstruction* c) {
  return TargetEmitConstant(&rv->base, c);
}

static TargetInstruction* EmitSymbol(RVGenerator* rv, TargetInstruction* c) {
  return TargetEmitSymbol(&rv->base, c);
}

static TargetInstruction* FramePointer(RVGenerator* rv) {
  return TargetFramePointer(&rv->base);
}

static TargetInstruction* StackPointer(RVGenerator* rv) {
  return TargetStackPointer(&rv->base);
}

static TargetInstruction* GetLoweredNode(IRNode* node) {
  return TargetGetLoweredNode(node);
}

static TargetInstruction* SetLoweredNode(IRNode* node,
                                         TargetInstruction* inst) {
  return TargetSetLoweredNode(node, inst);
}

static TargetInstruction* GetIntConstant(RVGenerator* rv, IRNode* node,
                                         TargetType type, int64_t value) {
  return TargetGetIntConstant(&rv->base, node, type, value);
}

static TargetInstruction* GetFloatingPointConstant(RVGenerator* rv,
                                                   IRNode* node,
                                                   TargetType type,
                                                   double value) {
  return TargetGetFloatingPointConstant(&rv->base, node, type, value);
}

static TargetInstruction* GetSymbol(RVGenerator* rv, IRNode* node,
                                    Symbol* symbol) {
  return TargetGetSymbol(&rv->base, node, symbol);
}

static TargetInstruction* NewInstruction(RVOpcode opcode) {
  return TargetNewInstruction((TargetOpcode)opcode);
}

static TargetInstruction* Zero(RVGenerator* rv) {
  if (rv->zero == NULL) {
    rv->zero = Emit(rv, NewInstruction(RV_OP(x0)));
  }
  return rv->zero;
}

static TargetInstruction* IntArgumentRegister(RVGenerator* rv, int argnum) {
  if (rv->int_argument_registers[argnum] == NULL) {
    // Allocate instruction for argument register and emit it.  The argument
    // registers have to be contiguous in value from a0..a7.
    rv->int_argument_registers[argnum] =
        Emit(rv, NewInstruction(RV_OP(a0) + argnum));
  }
  return rv->int_argument_registers[argnum];
}

static TargetInstruction* FloatingPointArgumentRegister(RVGenerator* rv,
                                                        int argnum) {
  if (rv->fp_argument_registers[argnum] == NULL) {
    // Allocate instruction for argument register and emit it.  The argument
    // registers have to be contiguous in value from a0..a7.
    rv->fp_argument_registers[argnum] =
        Emit(rv, NewInstruction(RV_OP(fa0) + argnum));
  }
  return rv->fp_argument_registers[argnum];
}

static TargetInstruction* IntVariableRegister(RVGenerator* rv, int reg_index) {
  if (rv->int_variable_registers[reg_index] == NULL) {
    // Allocate instruction for variable register and emit it.  The variable
    // registers have to be contiguous in value from v0.
    rv->int_variable_registers[reg_index] =
        Emit(rv, NewInstruction(RV_OP(v0) + reg_index));
  }
  return rv->int_variable_registers[reg_index];
}

static TargetInstruction* FloatingPointVariableRegister(RVGenerator* rv,
                                                        int reg_index) {
  if (rv->fp_variable_registers[reg_index] == NULL) {
    // Allocate instruction for variable register and emit it.  The variable
    // registers have to be contiguous in value from fv0.
    rv->fp_variable_registers[reg_index] =
        Emit(rv, NewInstruction(RV_OP(fv0) + reg_index));
  }
  return rv->fp_variable_registers[reg_index];
}


// Add immediate to the src.  If it fits in 12 bits we can use an addi
// instruction, otherwise load the immediate and use an add instruction.
static TargetInstruction* AddImmediate(RVGenerator* rv, TargetInstruction* src,
                                       int64_t immed) {
  int64_t imm = immed;
  if (immed < 0) {
    imm = -immed;
  }
  TargetInstruction* immed_inst =
      GetIntConstant(rv, NULL, kTargetTypeWord, immed);
  if (imm <= 0x7ff) {
    return Emit(rv, NewInstruction2(RV_OP(addi), src, immed_inst));
  }
  TargetInstruction* li = Emit(rv, NewInstruction1(RV_OP(li), immed_inst));
  return Emit(rv, NewInstruction2(RV_OP(add), src, li));
}

static TargetInstruction* AddValue(RVGenerator* rv, TargetInstruction* src,
                                   TargetInstruction* value) {
  if (TargetIsConst(value)) {
    return AddImmediate(rv, src, TargetIntValue(value));
  }

  return Emit(rv, NewInstruction2(RV_OP(add), src, value));
}

// Calculate the offset from the frame pointer to a local variable in the stack.
static int LocalVariableOffset(RVGenerator* rv, int32_t var_offset) {
  return var_offset - rv->base.stack_frame_size -
      RV_STACK_FRAME_HEADER_SIZE;
}

static TargetInstruction* PagedOffsetFrom(RVGenerator* rv, TargetInstruction* src,
                                          int32_t offset, int32_t* page_offset) {
  // Offset is not in range.  Need to calculate an offset in a register.
  //
  // We calculate a page offset.  The addi instruction
  // has a 12 bit signed immediate that can be added to an offset
  // calculated from the src.
  int page;
  if (offset < 0) {
    page = -(-offset & ~0xfff);
  } else {
    page = offset & ~0xfff;
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
static TargetInstruction* OffsetFrom(RVGenerator* rv, TargetInstruction* src,
                                     int32_t offset) {
  bool offset_in_range = RVIsPossibleImmediate(offset);
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

static TargetInstruction* LoadImmediate(RVGenerator* rv, RVOpcode opcode,
                                          TargetInstruction* base, int32_t offset) {
  if (RVIsPossibleImmediate(offset)) {
    return Emit(rv, NewInstruction2(opcode, base,
                                    GetIntConstant(rv, NULL, kTargetTypeWord, offset)));
  }
  int32_t page_offset;
  TargetInstruction* page_inst = PagedOffsetFrom(rv, base, offset, &page_offset);
  return Emit(rv, NewInstruction2(opcode, page_inst,
                                  GetIntConstant(rv, NULL, kTargetTypeWord, page_offset)));

}

static TargetInstruction* StoreImmediate(RVGenerator* rv, RVOpcode opcode,
                                         TargetInstruction* value, TargetInstruction* base, int32_t offset) {
  if (RVIsPossibleImmediate(offset)) {
    return Emit(rv, NewInstruction3(opcode, value, base,
                                    GetIntConstant(rv, NULL, kTargetTypeWord, offset)));
  }
  int32_t page_offset;
  TargetInstruction* page_inst = PagedOffsetFrom(rv, base, offset, &page_offset);
  return Emit(rv, NewInstruction3(opcode, value, page_inst,
                                  GetIntConstant(rv, NULL, kTargetTypeWord, page_offset)));

}

static TargetInstruction* Memcpy(RVGenerator* rv, TargetInstruction* dest_addr,
                                 TargetInstruction* src_addr, int length,
                                 int src_offset, int dest_offset) {
  if (length <= 40) {
    // Length is short, copy using sequence of ld/sd and lb/sb instructions.
    int num_bytes = length & 7;
    int num_words = length >> 3;
    TargetInstruction* result = NULL;
    for (int i = 0; i < num_words; i++, src_offset += 8, dest_offset += 8) {
      TargetInstruction* load = LoadImmediate(rv, RV_OP(ld), src_addr, src_offset);
      result = StoreImmediate(rv, RV_OP(sd), load, dest_addr, dest_offset);
    }
    for (int i = 0; i < num_bytes; i++, src_offset += 1, dest_offset += 1) {
      TargetInstruction* load = LoadImmediate(rv, RV_OP(lb), src_addr, src_offset);
      result = StoreImmediate(rv, RV_OP(sb), load, dest_addr, dest_offset);
    }
    return result;
  }
  // TODO: generate a loop for intermediate lengths?

  // Length in a2.
  TargetInstruction* size = Emit(
      rv, NewInstruction1(RV_OP(li),
                          GetIntConstant(rv, NULL, kTargetTypeWord, length)));
  TargetInstruction* arg2 =
      Emit(rv, NewInstruction2(RV_OP(rmov), IntArgumentRegister(rv, 2), size));

  // Source in a1.
  if (src_offset != 0) {
    src_addr = OffsetFrom(rv, src_addr, src_offset);
  }
  TargetInstruction* arg1 = Emit(
      rv, NewInstruction2(RV_OP(rmov), IntArgumentRegister(rv, 1), src_addr));

  // Dest in a0.
  if (dest_offset != 0) {
    dest_addr = OffsetFrom(rv, dest_addr, dest_offset);
  }
  TargetInstruction* arg0 = Emit(
      rv, NewInstruction2(RV_OP(rmov), IntArgumentRegister(rv, 0), dest_addr));

  // We need to keep the arguments alive until the point of the call.  This
  // is done using a RV_OP(regarg) instruction sequence.  See BuildArgList for
  // details on the regarg instruction.
  TargetInstruction* regarg =
      Emit(rv, NewInstruction2(RV_OP(regarg), NULL, arg2));
  regarg = Emit(rv, NewInstruction2(RV_OP(regarg), regarg, arg1));
  regarg = Emit(rv, NewInstruction2(RV_OP(regarg), regarg, arg0));

  TargetInstruction* memcpy = GetSymbol(rv, NULL, rv->base.memcpy);
  return Emit(rv, NewInstruction2(RV_OP(call), memcpy, regarg));
}

static TargetInstruction* Memzero(RVGenerator* rv, TargetInstruction* dest_addr,
                                  int length, int offset) {
  if (length <= 40) {
    // Length is short, copy using sequence of ld/sd and lb/sb instructions.
    int num_bytes = length & 7;
    int num_words = length >> 3;
    TargetInstruction* result = NULL;
    for (int i = 0; i < num_words; i++, offset += 8) {
      result = StoreImmediate(rv, RV_OP(sd), Zero(rv), dest_addr, offset);
    }
    for (int i = 0; i < num_bytes; i++, offset += 1) {
      result = StoreImmediate(rv, RV_OP(sb), Zero(rv), dest_addr, offset);
    }
    return result;
  }

  // Third parameter to memset is the length.
  TargetInstruction* size = Emit(
      rv, NewInstruction1(RV_OP(li),
                          GetIntConstant(rv, NULL, kTargetTypeWord, length)));
  TargetInstruction* arg2 =
      Emit(rv, NewInstruction2(RV_OP(rmov), IntArgumentRegister(rv, 2), size));

  // Second arg is zero.
  TargetInstruction* arg1 = Emit(
      rv, NewInstruction2(RV_OP(rmov), IntArgumentRegister(rv, 1), Zero(rv)));

  // First arg is the address.
  TargetInstruction* arg0 = Emit(
      rv, NewInstruction2(RV_OP(rmov), IntArgumentRegister(rv, 0), dest_addr));

  // We need to keep the arguments alive until the point of the call.  This
  // is done using a RV_OP(regarg) instruction sequence.  See BuildArgList for
  // details on the regarg instruction.
  TargetInstruction* regarg =
      Emit(rv, NewInstruction2(RV_OP(regarg), NULL, arg2));
  regarg = Emit(rv, NewInstruction2(RV_OP(regarg), regarg, arg1));
  regarg = Emit(rv, NewInstruction2(RV_OP(regarg), regarg, arg0));

  TargetInstruction* memset = GetSymbol(rv, NULL, rv->base.memset);
  return Emit(rv, NewInstruction2(RV_OP(call), memset, regarg));
}

static RVOpcode IR2RV(IROpcode op) {
  switch (op) {
    case IR_OP(addi):
      return RV_OP(add);
    case IR_OP(addf):
      return RV_OP(fadd_s);
    case IR_OP(addd):
      return RV_OP(fadd_d);
    case IR_OP(adda):
      return RV_OP(add);

    case IR_OP(subi):
      return RV_OP(sub);
    case IR_OP(subf):
      return RV_OP(fsub_s);
    case IR_OP(subd):
      return RV_OP(fsub_d);
    case IR_OP(suba):
      return RV_OP(sub);

    case IR_OP(muli):
      return RV_OP(mul);
    case IR_OP(mulf):
      return RV_OP(fmul_s);
    case IR_OP(muld):
      return RV_OP(fmul_d);

    case IR_OP(divi):
      return RV_OP(div);
    case IR_OP(divf):
      return RV_OP(fdiv_s);
    case IR_OP(divd):
      return RV_OP(fdiv_d);

    case IR_OP(modi):
      return RV_OP(rem);

    case IR_OP(lsri):
      return RV_OP(srl);
    case IR_OP(asri):
      return RV_OP(sra);
    case IR_OP(lsli):
      return RV_OP(sll);

    case IR_OP(ori):
      return RV_OP(or);
    case IR_OP(andi):
      return RV_OP(and);
    case IR_OP(xori):
      return RV_OP(xor);

    case IR_OP(noti):
      return RV_OP(not);
    case IR_OP(nota):
      return RV_OP(not);
    case IR_OP(onescomp):
      return RV_OP(not);
    case IR_OP(negi):
      return RV_OP(neg);
    case IR_OP(negf):
      return RV_OP(fneg_s);
    case IR_OP(negd):
      return RV_OP(fneg_d);

    case IR_OP(i2f):
      return RV_OP(fcvt_s_l);
    case IR_OP(i2d):
      return RV_OP(fcvt_d_l);
    case IR_OP(f2d):
      return RV_OP(fcvt_d_s);
    case IR_OP(d2f):
      return RV_OP(fcvt_s_d);
    case IR_OP(f2i):
      return RV_OP(fcvt_l_s);
    case IR_OP(d2i):
      return RV_OP(fcvt_l_d);

    case IR_OP(movi):
      return RV_OP(mv);
    case IR_OP(movf):
      return RV_OP(fmv_s);
    case IR_OP(movd):
      return RV_OP(fmv_d);
    case IR_OP(mova):
      return RV_OP(mv);
    case IR_OP(rmovi):
      return RV_OP(rmov);
    case IR_OP(rmovf):
      return RV_OP(rmovf);
    case IR_OP(rmovd):
      return RV_OP(rmovd);
    case IR_OP(rmova):
      return RV_OP(rmov);
    case IR_OP(tmp):
      return RV_OP(tmp);
    default:
      assert(false);
  }
}

static bool UseRegisterForVariable(RVGenerator* rv, IRNode* var_node) {
  if (!compiler->optimize) {
    // When not optimizing, all variables are on the stack.
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

  if (TypeIsFloatingPoint(var_node->type)) {
    // Too many?
    if (rv->num_fp_reg_vars >= RV_MAX_FP_REG_VARS) {
      return false;
    }
    // printf("variable %s allocated to a register\n", var->symbol->name.value);
    return true;
  }

  // Variable might be able to go in integer register.
  if (rv->num_int_reg_vars >= RV_MAX_INT_REG_VARS) {
    return false;
  }
  // printf("variable %s allocated to a register\n", var->symbol->name.value);
  return true;
}

// Static varaibles have an address calculated by the linker so at this
// point they are unknown.  We need to load their address into a register.  This
// is done using a la or lla pseudo-instruction.
static TargetInstruction* LoadStaticVariableAddress(RVGenerator* rv,
                                                    IRNode* node) {
  IRVariable* var = (IRVariable*)node;
  // A local variable is loaded usng the lla instruction and globals
  // are loaded using la.  The difference is in PIC code lla will
  // not use the GOT for the relocation.
  RVOpcode opcode = var->symbol->flags.is_local ? RV_OP(lla) : RV_OP(la);
  return Emit(rv, NewInstruction1(opcode, GetLoweredNode(node)));
}

static struct {
  bool (*type_func)(TypeRecord*);
  RVOpcode load;
} load_opcodes[] = {
    {TypeIsInt, RV_OP(lw)},
    {TypeIsShort, RV_OP(lh)},
    {TypeIsChar, RV_OP(lb)},
    {TypeIsLong, RV_OP(ld)},
    {TypeIsLongLong, RV_OP(ld)},
    {TypeIsUnsignedInt, RV_OP(lwu)},
    {TypeIsUnsignedShort, RV_OP(lhu)},
    {TypeIsUnsignedChar, RV_OP(lbu)},
    {TypeIsFloat, RV_OP(flw)},
    {TypeIsDouble, RV_OP(fld)},
    {TypeIsBool, RV_OP(lb)},
    {TypeIsPointerOrArray, RV_OP(ld)},
    {TypeIsFunction, RV_OP(ld)},
    {NULL, 0},
};

static TargetInstruction* LoadVariableValue(RVGenerator* rv, IRNode* node,
                                            TargetInstruction* addr,
                                            TargetInstruction* offset) {
  RVOpcode opcode = RV_OP(ld);
  for (size_t i = 0; load_opcodes[i].type_func != NULL; i++) {
    if (load_opcodes[i].type_func(node->type)) {
      opcode = load_opcodes[i].load;
      break;
    }
  }
  assert(opcode != 0);
  return Emit(rv, NewInstruction2(opcode, addr, offset));
}

// Materialize a value into a register.  This loads a constant into a register
// or returns the instruction associated with the node if it's
// already in a register.
static TargetInstruction* Materialize(RVGenerator* rv, IRNode* node) {
  if (IRIsConst(node)) {
    switch (node->opcode) {
      case IR_OP(constb):
      case IR_OP(consts):
      case IR_OP(consti):
      case IR_OP(constl):
      case IR_OP(consta):
        if (IRIsZero(node)) {
          // RISC-V has an explicit zero register (x0).  If we are loading
          // the constant zero, just move it into the destination.
          return Emit(rv, NewInstruction1(RV_OP(mv), Zero(rv)));
        } else {
          return Emit(rv, NewInstruction1(RV_OP(li), GetLoweredNode(node)));
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
                           RV_OP(li),
                           GetIntConstant(rv, NULL, kTargetTypeWord, bits)));
        }
        return Emit(rv, NewInstruction1(RV_OP(fmv_w_x), c));
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
                       RV_OP(li),
                       GetIntConstant(rv, NULL, kTargetTypeExtended, bits)));
        }
        return Emit(rv, NewInstruction1(RV_OP(fmv_d_x), c));
      }
      default:
        assert(false);
    }
  }
  if (IRIsAutoVariable(node)) {
    int32_t var_offset = node->data.ivalue;
    if (RV_IS_REG_VAR(var_offset)) {
      // Variable is in a register.
      int reg_num = var_offset & ~RV_REG_VAR;
      if (TypeIsFloatingPoint(node->type)) {
        return FloatingPointVariableRegister(rv, reg_num);
      } else {
        return IntVariableRegister(rv, reg_num);
      }
    }

    // Auto variable is in the stack frame.  These are accessed through
    // the frame pointer with a negative offset.
    TargetInstruction* addr = FramePointer(rv);
    return OffsetFrom(rv, addr, LocalVariableOffset(rv, var_offset));
  } else if (IRIsArgument(node)) {
    // TODO: structs passed by reference.
    int32_t var_offset = node->data.ivalue;
    if (RV_IS_REG_VAR(var_offset)) {
      // Argument is in a register.
      int reg_num = var_offset & ~RV_REG_VAR;
      if (TypeIsFloatingPoint(node->type)) {
        return FloatingPointVariableRegister(rv, reg_num);
      } else {
        return IntVariableRegister(rv, reg_num);
      }
    } else {
      // Argument is on the stack.
      TargetInstruction* addr = FramePointer(rv);
      int32_t var_offset = node->data.ivalue;
      return OffsetFrom(rv, addr, var_offset);
    }
  } else if (IRIsStaticVariable(node)) {
    // The address of static variables need to be moved into a register.

    return LoadStaticVariableAddress(rv, node);
  }

  // Node is not a variable, is must be an already-lowered expression.
  return GetLoweredNode(node);
}

static void ApplyFixups(RVGenerator* rv, IRNode* label_node) {
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
// While each RISC-V processor is different, let's assume that the multiplier
// can do 8 bits at a time.  Since this is a 64 bit processor, that's
// 8 cycles to multiply two 64 bit numbers.  Each 1-bit in the constant
// causes the emission of a shift instruction and these need to be added
// together.  Therefore the number of instructions for a n bits is
// n + (n - 1) = 2n-1.  However for bit 0 we don't do the shift but instead
// use the input value directly.
//
// Let's assume that both a slli and an add instruction take 1 cycle.
static TargetInstruction* MultiplyByConstant(RVGenerator* rv,
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
           Emit(rv, NewInstruction2(RV_OP(slli), input,
                           GetIntConstant(rv, NULL, kTargetTypeWord, bitpos)));
      }
      if (left == NULL) {
        left = inst;
      } else if (right == NULL) {
        right = inst;
      }
      if (left != NULL && right != NULL) {
        // Add left and right together.
        inst = Emit(rv, NewInstruction2(RV_OP(add), left, right));
        
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


static TargetInstruction* LowerExpression(RVGenerator* rv, IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  RVOpcode opcode = IR2RV(node->opcode);
  assert(node->inputs.length <= 2);
  TargetInstruction* inst = NULL;
  bool ref_counts_ok =
      false;  // True if we don't need to update operand ref counts.
                
  // Do some strength reduction if we can.
  switch (opcode) {
    default:
      // All others are handled below.
      break;
    case RV_OP(add): {
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
        if (RVIsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(RV_OP(addi));
          inst->operand[0] = Materialize(rv, op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      } else if (IRIsConst(op1)) {
        int64_t c = ((IRConstant*)op1)->value.ivalue;
        if (RVIsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(RV_OP(addi));
          inst->operand[0] = Materialize(rv, op2);
          inst->operand[1] = GetLoweredNode(op1);
        }
      }
      break;
    }

    case RV_OP(sub): {
      // A subtract immediate can be done using an addi with the negative of the
      // immediate.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (RVIsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(RV_OP(addi));
          inst->operand[0] = Materialize(rv, op1);
          inst->operand[1] = GetIntConstant(rv, NULL, kTargetTypeWord, -c);
        }
      }
    } break;
      ;
    case RV_OP(sll):
    case RV_OP(srl):
    case RV_OP(sra): {
      // There are constant shift operations.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        // TODO: what about a shift out of range?
        if (c == 0) {
          // A shift of 0 is a mv.
          inst = NewInstruction(RV_OP(mv));
          inst->operand[0] = Materialize(rv, op1);
        } else {
          switch (opcode) {
            case RV_OP(sll):
              // TODO: slliw?
              opcode = RV_OP(slli);
              break;
            case RV_OP(srl):
              opcode = RV_OP(srli);
              break;
            case RV_OP(sra):
              opcode = RV_OP(srai);
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
    case RV_OP(mul): {
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
            inst = NewInstruction(RV_OP(mv));
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
            inst = NewInstruction(RV_OP(mv));
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
    case RV_OP(div): {
      // If we are dividing by a constant power of 2 we can use a shift.
      // TODO: other constants can be done too.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        RVOpcode opcode =
            TypeIsUnsigned(node->type) ? RV_OP(srli) : RV_OP(srai);
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        // TODO: can we give an error on division by zero here?
        if (c == 1) {
          // Division by 1 is a mv.
          inst = NewInstruction(RV_OP(mv));
          inst->operand[0] = Materialize(rv, op1);
        } else {
          if (IsPowerOf2(c) && c < 64) {
            c = Log2(c);
            inst =
                NewInstruction2(opcode, Materialize(rv, op1),
                                GetIntConstant(rv, NULL, kTargetTypeWord, c));
            ref_counts_ok = true;
          }
        }
      }

      break;
    }

    case RV_OP(and): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 0) {
          // Anding with zero is zero.
          inst = Zero(rv);
        } else if (RVIsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(RV_OP(andi));
          inst->operand[0] = Materialize(rv, op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      }
      break;
    }

    case RV_OP(or): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 0) {
          // ORing with zero is nop.
          inst = GetLoweredNode(op1);
        } else if (RVIsPossibleImmediate(c)) {
          inst = (TargetInstruction*)NewInstruction(RV_OP(ori));
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
  }
  if (!ref_counts_ok) {
    TargetUpdateRefCount(inst);
  }
  SetLoweredNode(node, inst);
  return Emit(rv, inst);
}

// There is no compare for equality instruction, it must be synthesized
// using a subtract and compare against zero.  This function
// performs the subtraction.  If it returns NULL there is no subtraction
// and the pseudo-instruction to compare against zero is used directly.
static TargetInstruction* SubtractForComparison(RVGenerator* rv, IRNode* node,
                                                IRNode* op1, IRNode* op2) {
  if (!IRIsConst(op2)) {
    // Second operand isn't constant, compiled as sub.
    return Emit(rv, NewInstruction2(RV_OP(sub), Materialize(rv, op1),
                                    Materialize(rv, op2)));
  }

  // Second operand is constant.
  int64_t value = ((IRConstant*)op2)->value.ivalue;
  if (value == 0) {
    // Common case, compare with zero, no subtract.
    return NULL;
  }

  if (RVIsPossibleImmediate(value)) {
    // There is no subi instruction, so we have to use an addi
    // with the negative of the immediate.
    return AddImmediate(rv, Materialize(rv, op1), -value);
  }

  // Constant is too big for an immediate, materialize it into
  // a register and use a sub instruction.
  return Emit(rv, NewInstruction2(RV_OP(sub), Materialize(rv, op1),
                                  Materialize(rv, op2)));
}

// Compare integers for less than.  This uses the slt/slti
// instructions.
static TargetInstruction* CompareLessThanInt(RVGenerator* rv, IRNode* node,
                                             IRNode* op1, IRNode* op2) {
  if (!IRIsConst(op2)) {
    // Second operand isn't constant, compiled as slt.
    return Emit(rv, NewInstruction2(RV_OP(slt), Materialize(rv, op1),
                                    Materialize(rv, op2)));
  }

  // Second operand is constant.
  int64_t value = ((IRConstant*)op2)->value.ivalue;
  if (value == 0) {
    // Common case, compare with zero, use sltz.
    return Emit(rv, NewInstruction1(RV_OP(sltz), Materialize(rv, op1)));
  }

  if (RVIsPossibleImmediate(value)) {
    // Immediate, use slti.
    return Emit(
        rv, NewInstruction2(RV_OP(slti), Materialize(rv, op1),
                            GetIntConstant(rv, NULL, kTargetTypeWord, value)));
  }

  // Constant is too big for an immediate, materialize it into
  // a register and use an slt instruction.
  return Emit(rv, NewInstruction2(RV_OP(slt), Materialize(rv, op1),
                                  Materialize(rv, op2)));
}

// Comparisons set the result register to 1 or 0.  The result of integer
// comparisons is usually used as the input to a conditional branch.  The
// RISC-V has combined compare and branch instructions so we can generally
// eliminate the integer comparisons.  This will be done when we lower
// the conditional branch.  However we still need to generate the correct
// result because the result might not be used in a branch.
//
// Floating point comparisons do exist but the only conditions are EQ/LT/LE.
// We need to reverse the operands to perform the other conditions.
static TargetInstruction* LowerComparison(RVGenerator* rv, IRNode* node) {
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
                    node, NewInstruction1(RV_OP(seqz), Materialize(rv, op1))));
      } else {
        return Emit(rv,
                    SetLoweredNode(node, NewInstruction1(RV_OP(seqz), sub)));
      }
    }
    case IR_OP(cmpnei):
    case IR_OP(cmpnea): {
      // Subtract two ops and compare against zero.
      TargetInstruction* sub = SubtractForComparison(rv, node, op1, op2);
      if (sub == NULL) {
        return Emit(
            rv, SetLoweredNode(
                    node, NewInstruction1(RV_OP(snez), Materialize(rv, op1))));
      } else {
        return Emit(rv,
                    SetLoweredNode(node, NewInstruction1(RV_OP(snez), sub)));
      }
    }
    case IR_OP(cmplti):
    case IR_OP(cmplta):
      // Compare using slt/slti.
      return SetLoweredNode(node, CompareLessThanInt(rv, node, op1, op2));

    case IR_OP(cmplei):
    case IR_OP(cmplea): {
      // Use op2 < op1 and invert
      TargetInstruction* slt = CompareLessThanInt(rv, node, op2, op1);
      return Emit(rv, SetLoweredNode(node, NewInstruction1(RV_OP(not), slt)));
    }

    case IR_OP(cmpgti):
    case IR_OP(cmpgta):
      // Use op2 < op1.
      return SetLoweredNode(node, CompareLessThanInt(rv, node, op2, op1));

    case IR_OP(cmpgei):
    case IR_OP(cmpgea): {
      // Compare less than and invert.
      TargetInstruction* slt = CompareLessThanInt(rv, node, op1, op2);
      return Emit(rv, SetLoweredNode(node, NewInstruction1(RV_OP(not), slt)));
    }

    case IR_OP(cmpeqf):
      // feq.s op1, op2
      return Emit(
          rv, SetLoweredNode(node,
                             NewInstruction2(RV_OP(feq_s), Materialize(rv, op1),
                                             Materialize(rv, op2))));

    case IR_OP(cmpnef): {
      // feq.s op1, op1
      // invert
      TargetInstruction* cmp =
          Emit(rv, NewInstruction2(RV_OP(feq_s), Materialize(rv, op1),
                                   Materialize(rv, op2)));
      return Emit(rv, SetLoweredNode(node, NewInstruction1(RV_OP(not), cmp)));
    }

    case IR_OP(cmpltf):
      // flt.s op1, op2
      return Emit(
          rv, SetLoweredNode(node,
                             NewInstruction2(RV_OP(flt_s), Materialize(rv, op1),
                                             Materialize(rv, op2))));
    case IR_OP(cmplef):
      // fle.s op1, op2
      return Emit(
          rv, SetLoweredNode(node,
                             NewInstruction2(RV_OP(fle_s), Materialize(rv, op1),
                                             Materialize(rv, op2))));
    case IR_OP(cmpgtf):
      // flt.s op2, op1
      return Emit(
          rv, SetLoweredNode(node,
                             NewInstruction2(RV_OP(flt_s), Materialize(rv, op2),
                                             Materialize(rv, op1))));
    case IR_OP(cmpgef):
      // fle.s op2, op1
      return Emit(
          rv, SetLoweredNode(node,
                             NewInstruction2(RV_OP(fle_s), Materialize(rv, op2),
                                             Materialize(rv, op1))));

    case IR_OP(cmpeqd):
      // feq.d op1, op2
      return Emit(
          rv, SetLoweredNode(node,
                             NewInstruction2(RV_OP(feq_d), Materialize(rv, op1),
                                             Materialize(rv, op2))));
    case IR_OP(cmpned): {
      // feq.d op1, op2
      // invert
      TargetInstruction* cmp =
          Emit(rv, NewInstruction2(RV_OP(feq_d), Materialize(rv, op1),
                                   Materialize(rv, op2)));
      return Emit(rv, SetLoweredNode(node, NewInstruction1(RV_OP(not), cmp)));
    }

    case IR_OP(cmpltd):
      // flt.d op1, op2
      return Emit(
          rv, SetLoweredNode(node,
                             NewInstruction2(RV_OP(flt_d), Materialize(rv, op1),
                                             Materialize(rv, op2))));
    case IR_OP(cmpled):
      // fle.d op1, op2
      return Emit(
          rv, SetLoweredNode(node,
                             NewInstruction2(RV_OP(fle_d), Materialize(rv, op1),
                                             Materialize(rv, op2))));
    case IR_OP(cmpgtd):
      // flt.d op2, op1
      return Emit(
          rv, SetLoweredNode(node,
                             NewInstruction2(RV_OP(flt_d), Materialize(rv, op2),
                                             Materialize(rv, op1))));
    case IR_OP(cmpged):
      // fle.d op2, op1
      return Emit(
          rv, SetLoweredNode(node,
                             NewInstruction2(RV_OP(fle_d), Materialize(rv, op2),
                                             Materialize(rv, op1))));
    default:
      assert(false);
  }
  return NULL;
}

static void GetAddressAndOffsetFrom(RVGenerator* rv,
                                 TargetInstruction* addr,
                                 int offset,
                                 TargetInstruction** addr_inst,
                                 TargetInstruction** offset_inst) {
  if (RVIsPossibleImmediate(offset)) {
    *addr_inst = addr;
    *offset_inst = GetIntConstant(rv, NULL, kTargetTypeWord, offset);
    return;
  }
  int page_offset;
  TargetInstruction* page_inst = PagedOffsetFrom(rv, addr, offset, &page_offset);
  *addr_inst = page_inst;
  *offset_inst = GetIntConstant(rv, NULL, kTargetTypeWord, page_offset);
}

static bool GetRegAndOffset(RVGenerator* rv, IRNode* addr_node,
                            TargetInstruction** addr,
                            TargetInstruction** offset) {
  if (IRIsAutoVariable(addr_node)) {
    int32_t var_offset = addr_node->data.ivalue;
    if (RV_IS_REG_VAR(var_offset)) {
      // Variable is in a register.
      int reg_num = var_offset & ~RV_REG_VAR;
      if (TypeIsFloatingPoint(addr_node->type)) {
        *addr = FloatingPointVariableRegister(rv, reg_num);
      } else {
        *addr = IntVariableRegister(rv, reg_num);
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
    if (RV_IS_REG_VAR(var_offset)) {
      // Argument is in a register.
      int reg_num = var_offset & ~RV_REG_VAR;
      if (TypeIsFloatingPoint(addr_node->type)) {
        *addr = FloatingPointVariableRegister(rv, reg_num);
      } else {
        *addr = IntVariableRegister(rv, reg_num);
      }
      *offset = NULL;
      return false;
    } else {
      GetAddressAndOffsetFrom(rv, FramePointer(rv), var_offset,
                              addr, offset);
     }
  } else if (IRIsStaticVariable(addr_node)) {
    // The address of static variables need to be moved into a register.

    *addr = LoadStaticVariableAddress(rv, addr_node);
    *offset = Zero(rv);
  } else {
    // All others have a calculated address.
    *addr = GetLoweredNode(addr_node);
    *offset = Zero(rv);
    assert(addr != NULL);
  }
  return true;
}

static TargetInstruction* LowerLoad(RVGenerator* rv, IRNode* node) {
  RVOpcode opcode;
  assert(node->inputs.length == 1);
  IRNode* addr_node = node->inputs.value.p[0];
  TargetInstruction* addr;
  TargetInstruction* offset;
  bool on_stack = GetRegAndOffset(rv, addr_node, &addr, &offset);

  if (!on_stack) {
    SetLoweredNode(node, addr);
    return addr;
  }

  switch (node->opcode) {
    case IR_OP(loadi):
      opcode = RV_OP(lw);
      break;
    case IR_OP(loadb):
      opcode = RV_OP(lb);
      break;
    case IR_OP(loadl):
      opcode = RV_OP(ld);
      break;
    case IR_OP(loads):
      opcode = RV_OP(lh);
      break;
    case IR_OP(loadui):
      opcode = RV_OP(lwu);
      break;
    case IR_OP(loadub):
      opcode = RV_OP(lbu);
      break;
    case IR_OP(loadus):
      opcode = RV_OP(lhu);
      break;
    case IR_OP(loadf):
      opcode = RV_OP(flw);
      break;
    case IR_OP(loadd):
      opcode = RV_OP(fld);
      break;
    case IR_OP(loada):
      opcode = RV_OP(ld);
      break;
    default:
      assert(false);
  }

  TargetInstruction* result = NULL;
  if ((RVOpcode)addr->opcode == RV_OP(addi) && TargetIsZero(offset)) {
    // If the address is calculated using an addi instruction we can
    // combine the immediate from the addi with the load.
    // The addi instruction will no longer be used and will be eliminated
    // during the optimization pass.
    TargetInstruction* src = addr->operand[0];
    TargetInstruction* immed = addr->operand[1];
    assert(src != NULL);
    assert(immed != NULL);
    assert(TargetIsConst(immed));
    result = Emit(rv, NewInstruction2(opcode, src, immed));
  }
  if (result == NULL) {
    result = Emit(rv, NewInstruction2(opcode, addr, offset));
  }
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerStore(RVGenerator* rv, IRNode* node) {
  RVOpcode opcode;
  assert(node->inputs.length == 2);

  // Address to store to is the first operand of the store IR node.
  IRNode* addr_node = node->inputs.value.p[0];
  TargetInstruction* addr;
  TargetInstruction* offset;
  bool on_stack = GetRegAndOffset(rv, addr_node, &addr, &offset);
  // addr is a register.
  // if the variable is in memory offset will be an integer constant
  // containing the offset.  Otherwise it is NULL.

  // Value to store is in second input.
  IRNode* src_node = node->inputs.value.p[1];

  // If we are not on the stack, move the src to the dest.
  if (!on_stack) {
    TargetInstruction* src = Materialize(rv, src_node);
    TargetInstruction* result = NULL;
    if (TypeIsFloatingPoint(addr_node->type)) {
      result = Emit(rv, NewInstruction2(RV_OP(rmovf), addr, src));
    } else {
      result = Emit(rv, NewInstruction2(RV_OP(rmov), addr, src));
    }
    SetLoweredNode(node, result);
    return result;
  }

  // Work out store opcode.
  switch (node->opcode) {
    case IR_OP(storei):
      opcode = RV_OP(sw);
      break;
    case IR_OP(storeb):
      opcode = RV_OP(sb);
      break;
    case IR_OP(storel):
      opcode = RV_OP(sd);
      break;
    case IR_OP(stores):
      opcode = RV_OP(sh);
      break;
    case IR_OP(storef):
      opcode = RV_OP(fsw);
      break;
    case IR_OP(stored):
      opcode = RV_OP(fsd);
      break;
    case IR_OP(storea):
      opcode = RV_OP(sd);
      break;
    default:
      assert(false);
  }

  TargetInstruction* src = Materialize(rv, src_node);
  TargetInstruction* result = NULL;
  result = Emit(rv, NewInstruction3(opcode, src, addr, offset));

  
  SetLoweredNode(node, result);
  return result;
}

static struct BranchInfo {
  IROpcode cmp;     // IR comparison opcode.
  bool btrue;       // IR branch was btrue.
  RVOpcode branch;  // Branch opcode.
  bool reverse;     // Reverse operands.
} branch_compare_ops[] = {
    {IR_OP(cmpeqi), true, RV_OP(beq), false},
    {IR_OP(cmpnei), true, RV_OP(bne), false},
    {IR_OP(cmplti), true, RV_OP(blt), false},
    {IR_OP(cmplei), false, RV_OP(bge), false},
    {IR_OP(cmpgti), true, RV_OP(blt), true},
    {IR_OP(cmpgei), true, RV_OP(bge), false},

    {IR_OP(cmpeqa), true, RV_OP(beq), false},
    {IR_OP(cmpnea), true, RV_OP(bne), false},
    {IR_OP(cmplta), true, RV_OP(bltu), false},
    {IR_OP(cmplea), false, RV_OP(bgeu), false},
    {IR_OP(cmpgta), true, RV_OP(bltu), true},
    {IR_OP(cmpgea), true, RV_OP(bgeu), false},

    {IR_OP(cmpeqi), false, RV_OP(bne), false},
    {IR_OP(cmpnei), false, RV_OP(beq), false},
    {IR_OP(cmplti), false, RV_OP(bge), false},
    {IR_OP(cmplei), true, RV_OP(blt), false},
    {IR_OP(cmpgti), false, RV_OP(bge), true},
    {IR_OP(cmpgei), false, RV_OP(blt), false},

    {IR_OP(cmpeqa), false, RV_OP(bne), false},
    {IR_OP(cmpnea), false, RV_OP(beq), false},
    {IR_OP(cmplta), false, RV_OP(bgeu), false},
    {IR_OP(cmplea), true, RV_OP(bltu), false},
    {IR_OP(cmpgta), false, RV_OP(bgeu), true},
    {IR_OP(cmpgea), false, RV_OP(bltu), false},
};

#define NUM_BRANCH_COMPARES \
  (sizeof(branch_compare_ops) / sizeof(branch_compare_ops[0]))

static TargetInstruction* LowerConditionalBranch(RVGenerator* rv,
                                                 IRNode* node) {
  assert(node->inputs.length == 2);
  IRNode* expr = node->inputs.value.p[0];
  IRNode* target_node = node->inputs.value.p[1];

  // The RISC-V has 3 operand integer compare and branch instructions.
  // For these we can remove the comparison instructions and combine
  // them with the branch.
  bool btrue = node->opcode == IR_OP(btrue);

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

    // There are beqz and bnez pseudo-instructions for comparing against zero.
    // Use them if possible.
    if ((IRIsZero(op1) || IRIsZero(op2)) &&
        (branch_info->branch == RV_OP(beq) ||
         branch_info->branch == RV_OP(bne))) {
      RVOpcode branch =
          branch_info->branch == RV_OP(beq) ? RV_OP(beqz) : RV_OP(bnez);

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

    TargetInstruction* inst =
        Emit(rv, NewInstruction2(branch_info->branch, Materialize(rv, op1),
                                 Materialize(rv, op2)));

    TargetInstruction* target = target_node->data.ptr;
    if (target == NULL) {
      // Forward branch, add fixup for target label.
      VectorAppend(&rv->base.fixups, NewBranchFixup(inst, target_node, 2));
    } else {
      inst->operand[2] = target;
    }

    return inst;
  }

  RVOpcode opcode;
  switch (node->opcode) {
    case IR_OP(btrue):
      opcode = RV_OP(bnez);
      break;
    case IR_OP(bfalse):
      opcode = RV_OP(beqz);
      break;
    default:
      assert(false);
  }

  TargetInstruction* inst =
      Emit(rv, NewInstruction1(opcode, GetLoweredNode(expr)));

  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    // Forward branch, add fixup for target label.
    VectorAppend(&rv->base.fixups, NewBranchFixup(inst, target_node, 1));
  } else {
    inst->operand[1] = target;
  }
  return inst;
}

static TargetInstruction* LowerBranch(RVGenerator* rv, IRNode* node) {
  assert(node->inputs.length == 1);
  IRNode* target_node = node->inputs.value.p[0];

  TargetInstruction* inst =
      (TargetInstruction*)Emit(rv, NewInstruction(RV_OP(j)));

  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    // Forward branch, add fixup for target label.
    VectorAppend(&rv->base.fixups, NewBranchFixup(inst, target_node, 0));
  } else {
    inst->operand[0] = target;
  }
  return inst;
}

static TargetInstruction* LowerLabel(RVGenerator* rv, IRNode* label) {
  TargetInstruction* inst =  Emit(rv, NewInstruction(RV_OP(label)));
  label->data.ptr = inst;
  ApplyFixups(rv, label);
  return inst;
}

static TargetInstruction* LowerNamedLabel(RVGenerator* rv, IRNode* label) {
  IRNamedLabel* n = (IRNamedLabel*)label;
  TargetInstruction* inst =  Emit(rv, TargetNewNamedLabel(n->name));
  label->data.ptr = inst;
  return inst;
}

static TargetInstruction* LowerResult(RVGenerator* rv, IRNode* node) {
  assert(node->inputs.length == 1);
  RVOpcode result_reg_opcode, opcode;
  switch (node->opcode) {
    case IR_OP(resulti):
    case IR_OP(resulta):
      result_reg_opcode = RV_OP(resultx);
      opcode = RV_OP(rmov);
      break;
    case IR_OP(resultf):
      result_reg_opcode = RV_OP(resultf);
      opcode = RV_OP(rmovf);
      break;
    case IR_OP(resultd):
      result_reg_opcode = RV_OP(resultd);
      opcode = RV_OP(rmovd);
      break;
    default:
      assert(false);
  }
  TargetInstruction* result = Materialize(rv, node->inputs.value.p[0]);
  TargetInstruction* result_reg = Emit(rv, NewInstruction(result_reg_opcode));
  return Emit(rv, NewInstruction2(opcode, result_reg, result));
}

static TargetInstruction* LowerAsm(RVGenerator* rv, IRNode* node) {
  // The first argument is a literal containing the assembly language.
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(rv, TargetNewLiteral((int)id_node->value.ivalue));

  TargetInstruction* result = Emit(rv, NewInstruction1(RV_OP(asm), literal));

  SetLoweredNode(node, result);
  return result;
}

// A literal reference is an add of the literal offset (the first input
// to the literalref node) to the 'literal' with the given id.  This will
// be assembled as a reference to a symbol with the name .str.%d.
static TargetInstruction* LowerLiteralReference(RVGenerator* rv, IRNode* node) {
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(rv, TargetNewLiteral((int)id_node->value.ivalue));

  TargetInstruction* result = Emit(rv, NewInstruction1(RV_OP(lla), literal));

  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerAddressOf(RVGenerator* rv, IRNode* node) {
  return SetLoweredNode(node, Materialize(rv, node->inputs.value.p[0]));
}

static TargetInstruction* LowerMask(RVGenerator* rv, IRNode* node) {
  TargetInstruction* value = Materialize(rv, node->inputs.value.p[0]);
  IRConstant* mask_node = (IRConstant*)node->inputs.value.p[1];
  int64_t mask = mask_node->value.ivalue;
  if (RVIsPossibleImmediate(mask)) {
    value = Emit(rv, NewInstruction2(
                         RV_OP(andi), value,
                         GetIntConstant(rv, NULL, kTargetTypeExtended, mask)));
  } else {
    value = Emit(rv, NewInstruction2(RV_OP(and), value,
                                     Materialize(rv, node->inputs.value.p[1])));
  }
  SetLoweredNode(node, value);
  return value;
}

static TargetInstruction* LowerSignExtend(RVGenerator* rv, IRNode* node) {
  TargetInstruction* value = Materialize(rv, node->inputs.value.p[0]);
  if (RVIsSignedLoad((RVOpcode)value->opcode)) {
    return SetLoweredNode(node, value);
  }
  IRConstant* diff_value = node->inputs.value.p[1];
  int64_t diff = diff_value->value.ivalue;
  if (diff == 32) {
    // There is a word signextension instruction sext.w
    return SetLoweredNode(node,
                          Emit(rv, NewInstruction1(RV_OP(sext_w), value)));
  }
  TargetInstruction* immed = GetIntConstant(rv, NULL, kTargetTypeWord, diff);
  TargetInstruction* lsl = Emit(rv, NewInstruction2(RV_OP(slli), value, immed));
  TargetInstruction* asr = Emit(rv, NewInstruction2(RV_OP(srai), lsl, immed));

  SetLoweredNode(node, asr);
  return asr;
}

static TargetInstruction* PushArg(RVGenerator* rv, IRNode* node,
                                  TargetInstruction* inst, size_t offset) {
  if (node->type == NULL) {
    // No type, use sd instruction.
    return Emit(rv, NewInstruction3(
                        RV_OP(sd), inst, StackPointer(rv),
                        GetIntConstant(rv, node, kTargetTypeExtended, offset)));
  }
  RVOpcode opcode = RV_OP(sd);
  if (TypeIsFloatingPoint(node->type)) {
    opcode = RV_OP(fsd);
  }
  return Emit(rv, NewInstruction3(
                      opcode, inst, StackPointer(rv),
                      GetIntConstant(rv, node, kTargetTypeExtended, offset)));
}

static TargetInstruction* LowerMemcpy(RVGenerator* rv, IRNode* node) {
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
    if (!RVIsIntConst((RVOpcode)src_offset->opcode)) {
      src_addr = AddValue(rv, src_addr, src_offset);
    } else {
      src_offset_value = (int)((TargetConstant*)src_offset)->value.ivalue;
    }
  }
  src_node->data.ptr = src_addr;

  // Destination address.
  IRNode* dest_node = node->inputs.value.p[0];
  TargetInstruction* dest_addr;
  TargetInstruction* dest_offset;
  int dest_offset_value = 0;
  GetRegAndOffset(rv, dest_node, &dest_addr, &dest_offset);

  if (dest_offset != NULL) {
    if (!RVIsIntConst((RVOpcode)dest_offset->opcode)) {
      // We have an address and register for the address, add them together.
      dest_addr = AddValue(rv, dest_addr, dest_offset);
    } else {
      dest_offset_value = (int)((TargetConstant*)dest_offset)->value.ivalue;
    }
  }

  int length = (int)((IRConstant*)node->inputs.value.p[2])->value.ivalue;
  TargetInstruction* result = Memcpy(rv, dest_addr, src_addr, length,
                                     src_offset_value, dest_offset_value);

  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerMemzero(RVGenerator* rv, IRNode* node) {
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
    if (!RVIsIntConst((RVOpcode)dest_offset->opcode)) {
      // We have an address and register for the address, add them together.
      dest_addr = AddValue(rv, dest_addr, dest_offset);
    } else {
      offset_value = (int)((TargetConstant*)dest_offset)->value.ivalue;
    }
  }
  dest_node->data.ptr = dest_addr;
  TargetInstruction* result =
      Memzero(rv, dest_addr, var->symbol->type->size, offset_value);

  SetLoweredNode(node, result);
  return result;
}

typedef enum {
  kArgLocationRegister,
  kArgLocationPushed,
  kArgLocationPassedByReferenceInRegister,
  kArgLocationPassedByReferenceOnStack,
} ArgLocationType;

typedef struct {
  ArgLocationType type;
  union {
    TargetInstruction* reg;
    size_t offset;
  } location;
  size_t reference_offset;
} ArgLocation;

static ArgLocation* NewArgLocationRegister(TargetInstruction* reg) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationRegister;
  loc->location.reg = reg;
  loc->reference_offset = 0;
  return loc;
}

static ArgLocation* NewArgLocationPushed(ArgLocationType type, size_t offset) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = type;
  loc->location.offset = offset;
  loc->reference_offset = 0;
  return loc;
}

static ArgLocation* NewArgLocationReferenceInRegister(TargetInstruction* reg,
                                                      size_t reference_offset) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationPassedByReferenceInRegister;
  loc->location.reg = reg;
  loc->reference_offset = reference_offset;
  return loc;
}

static ArgLocation* NewArgLocationReferenceOnStack(size_t offset,
                                                   size_t reference_offset) {
  ArgLocation* loc = malloc(sizeof(ArgLocation));
  loc->type = kArgLocationPassedByReferenceOnStack;
  loc->location.offset = offset;
  loc->reference_offset = reference_offset;
  return loc;
}

// Build a list of RV_OP(regarg) instructions to hold the
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
static TargetInstruction* BuildArgList(RVGenerator* rv, Vector* arg_locations) {
  TargetInstruction* result = NULL;
  // Find next -based argument and add it to the regargs instruction list.
  for (size_t i = 0; i < arg_locations->length; i++) {
    ArgLocation* loc = arg_locations->value.p[i];
    if (loc->type == kArgLocationRegister) {
      result =
          Emit(rv, NewInstruction2(RV_OP(regarg), result, loc->location.reg));
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
//    stop working (TODO: check the C standard for this).s
// 2. Doesn't do the 2XXLEN stuff where 16 byte structs are passed in a
//    register pair.
static TargetInstruction* LowerCall(RVGenerator* rv, IRNode* node) {
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
    if (TypeIsStructOrUnion(arg_node->type)) {
      // Struct or union that fit in a register are passed in a register.  If
      // they are bigger than 8 bytes they are passed by reference (first making
      // a copy on the stack).
      size_t struct_size = arg_node->type->size;
      if (struct_size <= 8) {
        if (next_int_arg_reg < RV_NUM_INT_ARGS) {
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
      } else {
        // The struct needs to be copied onto the stack and then its address
        // passed either in a register or on the stack.
        if (next_int_arg_reg < RV_NUM_INT_ARGS) {
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
      if (next_fp_arg_reg < RV_NUM_FP_ARGS) {
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
      if (next_int_arg_reg < RV_NUM_INT_ARGS) {
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
  size_t total_stack_size = struct_area_size + next_pushed_arg_offset;
  if (total_stack_size > 0) {
    TargetInstruction* newsp =
        AddImmediate(rv, StackPointer(rv), -total_stack_size);
    Emit(rv, NewInstruction2(RV_OP(rmov), StackPointer(rv), newsp));
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
               (int)(arg_location->reference_offset + next_pushed_arg_offset));
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
  for (size_t i = node->inputs.length - 1; i >= 1; i--) {
    IRNode* arg_node = node->inputs.value.p[i];
    ArgLocation* arg_location = arg_locations.value.p[i - 1];
    switch (arg_location->type) {
      case kArgLocationPassedByReferenceInRegister: {
        // Struct passed by reference in a register.  The reference_offset
        // contains the offset from the to of the pushed args to the copied
        // struct.
        TargetInstruction* arg = AddImmediate(
            rv, StackPointer(rv),
            arg_location->reference_offset + next_pushed_arg_offset);
        Emit(rv, NewInstruction2(RV_OP(rmov), arg_location->location.reg, arg));
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
            arg = Emit(rv, NewInstruction2(
                               RV_OP(ld), arg,
                               GetIntConstant(rv, NULL, kTargetTypeWord, 0)));
          }
        }
        PushArg(rv, arg_node, arg, arg_location->location.offset);
        break;
      }
      case kArgLocationRegister: {
        // Argument is in a register.
        TargetInstruction* arg = Materialize(rv, arg_node);
        if (TypeIsStructOrUnion(arg_node->type)) {
          size_t size = arg_node->type->size;
          if (size <= 8) {
            // A struct less than 8 bytes is passed in a register.  The
            // Materialize call will result in the address of the struct.  We
            // need to load it.
            arg = Emit(rv, NewInstruction2(
                               RV_OP(ld), arg,
                               GetIntConstant(rv, NULL, kTargetTypeWord, 0)));
          }
        }
        RVOpcode mov_opcode = RV_OP(rmov);
        if (TypeIsFloatingPoint(arg_node->type)) {
          if (TypeIsDouble(arg_node->type)) {
            mov_opcode = RV_OP(rmovd);
          } else {
            mov_opcode = RV_OP(rmovf);
          }
        }
        Emit(rv, NewInstruction2(mov_opcode, arg_location->location.reg, arg));
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
  RVOpcode opcode;
  if (addr->opcode == RV_OP(symbol)) {
    // Calling a symbol, use a regular 'call' instruction.
    opcode = TypeIsFloatingPoint(node->type) ? RV_OP(callf) : RV_OP(call);
  } else {
    // Calling through a register, rcall.
    opcode = TypeIsFloatingPoint(node->type) ? RV_OP(rcallf) : RV_OP(rcall);
  }
  TargetInstruction* call =
      Emit(rv, NewInstruction2(opcode, addr, BuildArgList(rv, &arg_locations)));

  // Increment the stack pointer again to remove pushed args.
  if (total_stack_size > 0) {
    TargetInstruction* newsp =
        AddImmediate(rv, StackPointer(rv), total_stack_size);
    Emit(rv, NewInstruction2(RV_OP(rmov), StackPointer(rv), newsp));
  }
  SetLoweredNode(node, call);

  VectorDestructWithContents(&arg_locations, NULL);
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

static TargetInstruction* LowerComputedBranch(RVGenerator* rv, IRNode* node) {
  assert(node->inputs.length == 1);
  TargetInstruction* value = GetLoweredNode(node->inputs.value.p[0]);
  TargetInstruction* slli =
      Emit(rv, NewInstruction2(RV_OP(slli), value,
                               GetIntConstant(rv, NULL, kTargetTypeWord, 2)));
  TargetInstruction* auipc =
      Emit(rv, NewInstruction1(RV_OP(auipc),
                               GetIntConstant(rv, NULL, kTargetTypeWord, 0)));
  TargetInstruction* add = Emit(rv, NewInstruction2(RV_OP(add), auipc, slli));
  TargetInstruction* jalr =
      Emit(rv, NewInstruction3(RV_OP(jalr), Zero(rv), add,
                               GetIntConstant(rv, NULL, kTargetTypeWord, 12)));
  SetLoweredNode(node, jalr);
  return jalr;
}

// The first input is the address of the 'ap' variable.  The second is the
// address of the last function argument (ignored in RISC-V).  This
// simply stores the value of the frame pointer in the address passed
// in the first input.
static TargetInstruction* LowerBuiltinVaStart(RVGenerator* rv, IRNode* node) {
  TargetInstruction* s0 = Emit(rv, NewInstruction(RV_OP(fp)));
  TargetInstruction* addr;
  TargetInstruction* offset;
  bool on_stack = GetRegAndOffset(rv, node->inputs.value.p[0], &addr, &offset);
  if (!on_stack) {
    return SetLoweredNode(node,
                          Emit(rv, NewInstruction2(RV_OP(rmov), addr, s0)));
  }
  return SetLoweredNode(node,
                        Emit(rv, NewInstruction3(RV_OP(sd), s0, addr, offset)));
}

// The first input is &ap.  The 'ap' variable contains the address of the
// current argument (starts at s0).  The code is:
// ld t0, 0(ap)  // Address of current arg.
// ld a0, 0(t1)      // Load current arg.
// addi t0, t0, 8    // Next arg
// sd t0, 0(a0)      // Update

static TargetInstruction* LowerBuiltinVaArg(RVGenerator* rv, IRNode* node) {
  TargetInstruction* ap_addr;
  TargetInstruction* ap_offset;
  bool on_stack =
      GetRegAndOffset(rv, node->inputs.value.p[0], &ap_addr, &ap_offset);
  TargetInstruction* ap_load;
  if (!on_stack) {
    ap_load = ap_addr;
  } else {
    ap_load = Emit(rv, NewInstruction2(RV_OP(ld), ap_addr, ap_offset));
  }
  TargetInstruction* result =
      Emit(rv, NewInstruction2(RV_OP(ld), ap_load,
                               GetIntConstant(rv, NULL, kTargetTypeWord, 0)));
  TargetInstruction* addi =
      Emit(rv, NewInstruction2(RV_OP(addi), ap_load,
                               GetIntConstant(rv, NULL, kTargetTypeWord, 8)));
  if (on_stack) {
    Emit(rv, NewInstruction3(RV_OP(sd), addi, ap_addr, ap_offset));
  } else {
    Emit(rv, NewInstruction2(RV_OP(rmov), ap_load, addi));
  }
  return SetLoweredNode(node, result);
}

static TargetInstruction* LowerBuiltinVaEnd(RVGenerator* rv, IRNode* node) {
  // Nothing to do for va_end.
  return NULL;
}

static TargetInstruction* LowerBuiltinVaCopy(RVGenerator* rv, IRNode* node) {
  return NULL;  // TODO
}

static RegisterLoad* NewRegisterLoad(int dest_reg, bool on_stack,
                                     int offset_or_src_reg, bool address_only,
                                     IRNode* symbol) {
  RegisterLoad* load = malloc(sizeof(RegisterLoad));
  load->dest_reg = dest_reg;
  load->on_stack = on_stack;
  if (on_stack) {
    load->src.offset = offset_or_src_reg;
  } else {
    load->src.reg = offset_or_src_reg;
  }
  load->address_only = address_only;
  load->is_fp = false;
  load->symbol = (IRVariable*)symbol;
  return load;
}

static TargetInstruction* LowerLocation(RVGenerator* rv, IRNode* node) {
  IRLocation* loc = (IRLocation*)node;
  return SetLoweredNode(node, Emit(rv, TargetNewLocation(loc)));
}

static TargetInstruction* LowerIRNode(RVGenerator* rv, IRNode* node) {
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

    case IR_OP(ssavar):
    case IR_OP(phi):
      // We should never see these as we've moved out of SSA form
      // before here.
      break;

    case IR_OP(structreturn): {
      // Always allocate a saved register for the struct return value.
      rv->struct_return_reg = rv->num_int_reg_vars++;
      TargetInstruction* result =
          SetLoweredNode(node, Emit(rv, NewInstruction(RV_OP(structreturn))));
      return result;
    }

    case IR_OP(literalref):
      return LowerLiteralReference(rv, node);

    case IR_OP(addressof):
      return LowerAddressOf(rv, node);

    case IR_OP(consti):
    case IR_OP(consta):
      return GetIntConstant(rv, node, kTargetTypeWord,
                            ((IRConstant*)node)->value.ivalue);
    case IR_OP(constb):
      return GetIntConstant(rv, node, kTargetTypeByte,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(consts):
      return GetIntConstant(rv, node, kTargetTypeHalf,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(constl):
      return GetIntConstant(rv, node, kTargetTypeExtended,
                            ((IRConstant*)node)->value.ivalue);

    case IR_OP(constf):
      return GetFloatingPointConstant(rv, node, kTargetTypeFloat,
                                      ((IRConstant*)node)->value.fvalue);

    case IR_OP(constd):
      return GetFloatingPointConstant(rv, node, kTargetTypeDouble,
                                      ((IRConstant*)node)->value.fvalue);

    case IR_OP(enter):
      Emit(rv, NewInstruction(RV_OP(save)));
      return NULL;

    case IR_OP(leave):
      Emit(rv, NewInstruction(RV_OP(restore)));
      return NULL;

    case IR_OP(ret):
      return Emit(rv, NewInstruction(RV_OP(ret)));

    case IR_OP(loadi):
    case IR_OP(loadb):
    case IR_OP(loadl):
    case IR_OP(loads):
    case IR_OP(loadui):
    case IR_OP(loadub):
    case IR_OP(loadus):
    case IR_OP(loadf):
    case IR_OP(loadd):
    case IR_OP(loada):
      return LowerLoad(rv, node);

      // stores.
    case IR_OP(storei):
    case IR_OP(storeb):
    case IR_OP(stores):
    case IR_OP(storel):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
      return LowerStore(rv, node);

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
    case IR_OP(rmovi):
    case IR_OP(rmovf):
    case IR_OP(rmovd):
    case IR_OP(rmova):
    case IR_OP(tmp):
      return LowerExpression(rv, node);

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
      return LowerComparison(rv, node);

      break;
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

    case IR_OP(calla):
      return LowerCall(rv, node);

    case IR_OP(resulti):
    case IR_OP(resultf):
    case IR_OP(resultd):
    case IR_OP(resulta):
      return LowerResult(rv, node);

    case IR_OP(memzero):
      return LowerMemzero(rv, node);

    case IR_OP(memcpy):
      return LowerMemcpy(rv, node);

    case IR_OP(maski):
      return LowerMask(rv, node);

    case IR_OP(signextendi):
      return LowerSignExtend(rv, node);

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
  }

  // If we get here we've failed to handle the IR node.
  assert(false);
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

static int CompareRegisterVar(const void* a, const void* b) {
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
  int int_reg = RV_INT_ARG_START;
  if (is_struct_return) {
    int_reg +=
        1;  // For struct returns, the first arg is the address of the struct.
  }
  int fp_reg = RV_FP_ARG_START;
  int stack_offset = 0;
  int current_stack_offset = 0;
  for (size_t i = 0; i < args->length; i++) {
    if (i == arg_num) {
      ArgLocation location;
      if (TypeIsFloatingPoint(arg->pooled->type)) {
        if (fp_reg <= RV_FP_ARG_END) {
          // Arg is in a floating point register.
          location.type = kArgLocationRegister;
          location.location.offset = fp_reg;
        } else {
          location.type = kArgLocationPushed;
          location.location.offset = current_stack_offset;
        }
      } else {
        if (int_reg <= RV_INT_ARG_END) {
          // Arg is in an integer register.
          location.type = kArgLocationRegister;
          location.location.offset = int_reg;
        } else {
          location.type = kArgLocationPushed;
          location.location.offset = current_stack_offset;
        }
      }
      return location;
    }

    Symbol* arg_symbol = args->value.p[i];
    if (TypeIsFloatingPoint(arg_symbol->type)) {
      if (fp_reg <= RV_LAST_FP_REG_VAR) {
        fp_reg++;
      } else {
        current_stack_offset = stack_offset;
        stack_offset += arg_symbol->type->size;
      }
    } else {
      if (int_reg <= RV_LAST_INT_REG_VAR) {
        int_reg++;
      } else {
        current_stack_offset = stack_offset;
        if (TypeIsStructOrUnion(arg_symbol->type)) {
          // Struct and unions are passed by reference - 8 bytes.
          stack_offset += 8;
        } else {
          int size = arg_symbol->type->size;
          if (size < 8) {
            stack_offset += 4;
          } else {
            stack_offset += 8;
          }
        }
      }
    }
  }
  // Can't find argument.
  assert(false);
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

// Assign a register to a variable or argument if possible.  The
// var_offset is below the stack frame.
static void AssignRegisterOrOffset(RVGenerator* rv, PoolEntry* entry,
                                   Vector* args, int* var_offset) {
  bool is_arg = entry->pooled->opcode == IR_OP(argument);
  int64_t size =
      is_arg ? CalculateArgumentSize(entry->pooled) : entry->pooled->type->size;
  assert(size != 0);

  // printf("var %s\n", ((IRVariable*)entry->pooled)->symbol->name.value);
  if (TypeIsFloatingPoint(entry->pooled->type)) {
    if (UseRegisterForVariable(rv, entry->pooled)) {
      int reg = rv->num_fp_reg_vars++;
      entry->pooled->data.ivalue = RV_REG_VAR | reg;
      SetDebugRegisterLocation(entry, reg);
      if (is_arg) {
        ArgLocation location = ArgumentLocation(entry, args);
        RegisterLoad* load =
            NewRegisterLoad(reg, location.type != kArgLocationRegister,
                            (int)location.location.offset, false, entry->pooled);
        load->is_fp = true;
        VectorAppend(&rv->register_loads, load);
      }
    } else {
      AlignOffset(entry, var_offset);
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
          entry->pooled->data.ivalue = RV_REG_VAR | reg;
          SetDebugRegisterLocation(entry, reg);
        } else {
          AlignOffset(entry, var_offset);
          entry->pooled->data.ivalue = *var_offset;
          SetDebugStackLocation(entry, *var_offset);
          *var_offset += size;
        }
      } else {
        // Passed by reference.  This means it is pushed onto the stack
        // and the address of the copy is passed in an argument register
        // or on the stack.
        // TODO:
      }
    } else {
      // Not an argument.
      // TODO: it is possible to put small structs in registers.
      AlignOffset(entry, var_offset);
      entry->pooled->data.ivalue = *var_offset;
      SetDebugStackLocation(entry, *var_offset);
      *var_offset += size;
    }

  } else if (TypeIsArray(entry->pooled->type) && !is_arg) {
    // An array local variable.  This is on the stack.  If it is in a
    // register we need to load its address into the register.
    AlignOffset(entry, var_offset);
    if (UseRegisterForVariable(rv, entry->pooled)) {
      int reg = rv->num_int_reg_vars++;
      entry->pooled->data.ivalue = RV_REG_VAR | reg;
      SetDebugRegisterLocation(entry, entry->pooled->data.ivalue);
      RegisterLoad* load = NewRegisterLoad(reg, true, (int)*var_offset, true, entry->pooled);
      VectorAppend(&rv->register_loads, load);
    } else {
      entry->pooled->data.ivalue = *var_offset;
      SetDebugStackLocation(entry, *var_offset);
    }
    *var_offset += size;
  } else if (TypeIsFunction(entry->pooled->type)) {
    IRVariable* var = (IRVariable*)entry->pooled;
    TargetInstruction* inst = GetSymbol(rv, NULL, var->symbol);
    entry->pooled->data.ptr = inst;
    SetDebugSymbolLocation(entry);
  } else {
    // Integer or pointer argument.
    if (UseRegisterForVariable(rv, entry->pooled)) {
      int reg = rv->num_int_reg_vars++;
      entry->pooled->data.ivalue = RV_REG_VAR | reg;
      SetDebugRegisterLocation(entry, reg);
      if (is_arg) {
        ArgLocation location = ArgumentLocation(entry, args);
        RegisterLoad* load =
            NewRegisterLoad(reg, location.type != kArgLocationRegister,
                            (int)location.location.offset, false, entry->pooled);
        VectorAppend(&rv->register_loads, load);
        if (location.type == kArgLocationRegister) {
          rv->num_int_arg_regs++;  // Argument was passed in a register.
        }
      }
    } else {
      if (is_arg) {
        // The argument is not going to be placed in a register.  We need
        // to make sure it's on the stack.  It is already on the stack
        // if it is not passed in a0..a7.  But if is in an arg reg
        // we need to save it to the stack frame.
        ArgLocation location = ArgumentLocation(entry, args);
        if (location.type == kArgLocationRegister) {
          // Argument is in a register so we need to save it to the stack. These
          // are stored immediately below the saved frame pointer (24 bytes
          // below the previous stack pointer).
          int offset = -24 - (int)rv->saved_regs.length * 8;
          SavedArgumentRegister* saved = NewSavedArgumentRegister(
              (int)location.location.offset, RV_FP_REG, offset);
          entry->pooled->data.ivalue = offset;
          VectorAppend(&rv->saved_regs, saved);
          rv->num_int_arg_regs++;  // Argument was passed in a register.
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

static void AssignRegisterVars(RVGenerator* rv, Vector* vars, Vector* args) {
  // Variables are allocated below the frame, arguments are above or in
  // registers.
  // If the argument is in a register, the top bit of the data.ivalue is
  // set and the low order bits are the register number.

  // Sort the pooled local variables in reverse order of usage.  Those
  // with the largest number of references will be at the start of the
  // vector.
  qsort(vars->value.p, vars->length, sizeof(IRNode*), CompareRegisterVar);

  int32_t var_offset = 0;

  for (size_t i = 0; i < vars->length; i++) {
    PoolEntry* entry = vars->value.p[i];
    AssignRegisterOrOffset(rv, entry, args, &var_offset);
  }

  // If we are a leaf procedure and we have more than 3 variables,
  // force the register allocator to use registers for the variables.
  rv->use_reg_vars = rv->num_int_reg_vars >= 3;
  
  // We now know the stack frame size.  This includes the length of the saved
  // registers.
  rv->base.stack_frame_size =
      (int32_t)var_offset + (int)rv->saved_regs.length * 8;
  // Align to 8 byte boundary.
  rv->base.stack_frame_size = (rv->base.stack_frame_size + 7) & ~7;
}

void RVLower(RVGenerator* rv, Generator* gen) {
  Vector local_vars;
  VectorInit(&local_vars);

  // Collect all local variables so that we can assign some of them
  // to registers.
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = (PoolEntry*)gen->variable_pool.value.p[i];
    switch (entry->pooled->opcode) {
      case IR_OP(localvar):
      case IR_OP(tempvar):
        if (!compiler->optimize || entry->value.symbol->flags.used) {
          VectorAppend(&local_vars, entry);
        }
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

  AssignRegisterVars(rv, &local_vars,
                     &compiler->current_function->info.function.prototype);
  VectorDestruct(&local_vars);

  IRNode* node = GeneratorFirstInstruction(gen);
  while (node != NULL) {
    LowerIRNode(rv, node);
    node = IRNext(node);
  }

  if (compiler->print_back_end) {
    RVPrint(rv);
  }
  
  // Optimize the code sequence.
  RVOptimize(rv);

  // RVPrint(rv);
  // Allocate registers to the instructions.
  RVAllocateRegisters(&rv->register_allocator);
}

void RVPrint(RVGenerator* rv) {
  TargetInstruction* inst = TargetFirstInstruction(&rv->base);
  while (inst != NULL) {
    TargetPrintInstruction(inst, RVOpcodeName, stdout);
    inst = TargetNext(inst);
  }
}
