//
//  risc_v_codegen.h
//  c_compiler_library
//
//  Created by David Allison on 2/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v_codegen_h
#define risc_v_codegen_h

#include "codegen.h"
#include "hashtable.h"
#include "list.h"
#include "risc_v_reg_alloc.h"
#include "target_generator.h"

// If this bit is set in the data.ivalue of a variable pool entry then
// the lower bits specify the absolute register number in which the
// variable is stored.
#define RV_REG_VAR 0x80000000

// Since we are using bit 31 for the REG_VAR flag we have to be careful
// with negative offsets.  If the offset it negative bit 30 of the value
// will also be set.  So to check for a register variable, AND with the
// RV_REG_VAR_MASK and check that bit 30 is not set.
#define RV_REG_VAR_MASK 0xc0000000
#define RV_IS_REG_VAR(offset) ((offset & RV_REG_VAR_MASK) == RV_REG_VAR)

#define RV_OP(op) kRVk##op

// RISC-V code generator opcodes.

typedef enum {
  // The initial sequence for these opcode must match the TargetOpcode
  // enumeration.  The names don't need to match but the positions and
  // operations must.  For example, TARGET_OP(mov) and RV_OP(mv) are the
  // same operation.
  RV_OP(save),
  RV_OP(restore),

  RV_OP(symbol),   // Static symbol.
  RV_OP(literal),  // String literal.
  RV_OP(tmp),

  // Constants.
  RV_OP(constb),
  RV_OP(consth),
  RV_OP(constw),
  RV_OP(constx),
  RV_OP(constf),
  RV_OP(constd),

  RV_OP(mv),
  RV_OP(fmv_s),
  RV_OP(fmv_d),

  RV_OP(movc),
  RV_OP(movfc),
  RV_OP(movdc),
  RV_OP(movxc),

  RV_OP(rmov),
  RV_OP(rmovf),
  RV_OP(rmovd),

  RV_OP(ret),

  RV_OP(label),

  RV_OP(fp),  // Frame pointer pseudo operation.
  RV_OP(sp),  // Stack pointer pseudo operation.

  // Function result registers.
  RV_OP(resultx),
  RV_OP(resultf),
  RV_OP(resultd),

  RV_OP(structreturn),  // Struct return address.

  RV_OP(asm),

  RV_OP(loc),

  // End of TargetOpcode enumeration.

  // Now follow the actual RISC-V instruction directly from the
  // specifications.

  // RV32I instructions.
  RV_OP(lui),
  RV_OP(auipc),
  RV_OP(jal),
  RV_OP(jalr),
  RV_OP(beq),
  RV_OP(bne),
  RV_OP(blt),
  RV_OP(bge),
  RV_OP(bltu),
  RV_OP(bgeu),
  RV_OP(lb),
  RV_OP(lh),
  RV_OP(lw),
  RV_OP(lbu),
  RV_OP(lhu),
  RV_OP(sb),
  RV_OP(sh),
  RV_OP(sw),
  RV_OP(addi),
  RV_OP(slti),
  RV_OP(sltiu),
  RV_OP(xori),
  RV_OP(ori),
  RV_OP(andi),
  RV_OP(slli),
  RV_OP(srli),
  RV_OP(srai),
  RV_OP(add),
  RV_OP(sub),
  RV_OP(sll),
  RV_OP(slt),
  RV_OP(sltu),
  RV_OP(xor),
  RV_OP(srl),
  RV_OP(sra),
  RV_OP(or),
  RV_OP(and),
  RV_OP(fence),
  RV_OP(fence_i),
  RV_OP(ecall),
  RV_OP(ebreak),
  RV_OP(csrrw),
  RV_OP(csrrs),
  RV_OP(csrrc),
  RV_OP(csrrwi),
  RV_OP(csrrsi),
  RV_OP(csrrci),

  // RV64I instructions.
  RV_OP(lwu),
  RV_OP(ld),
  RV_OP(sd),
  RV_OP(addiw),
  RV_OP(slliw),
  RV_OP(srliw),
  RV_OP(sraiw),
  RV_OP(addw),
  RV_OP(subw),
  RV_OP(sllw),
  RV_OP(srlw),
  RV_OP(sraw),

  // RV32M instructions.
  RV_OP(mul),
  RV_OP(mulh),
  RV_OP(mulhsu),
  RV_OP(mulhu),
  RV_OP(div),
  RV_OP(divu),
  RV_OP(rem),
  RV_OP(remu),

  // RV64M instructions.
  RV_OP(mulw),
  RV_OP(divw),
  RV_OP(divuw),
  RV_OP(remw),
  RV_OP(remuw),

  // RV32F instructions.
  RV_OP(flw),
  RV_OP(fsw),
  RV_OP(fmadd_s),
  RV_OP(fmsub_s),
  RV_OP(fnmsub_s),
  RV_OP(fnmadd_s),
  RV_OP(fadd_s),
  RV_OP(fsub_s),
  RV_OP(fmul_s),
  RV_OP(fdiv_s),
  RV_OP(fsqrt_s),
  RV_OP(fsgnj_s),
  RV_OP(fsgnjn_s),
  RV_OP(fsgnjx_s),
  RV_OP(fmin_s),
  RV_OP(fmax_s),
  RV_OP(fcvt_w_s),
  RV_OP(fcvt_wu_s),
  RV_OP(fmv_x_w),
  RV_OP(feq_s),
  RV_OP(flt_s),
  RV_OP(fle_s),
  RV_OP(fclass_s),
  RV_OP(fcvt_s_w),
  RV_OP(fcvt_s_wu),
  RV_OP(fmv_w_x),

  // RV64F instructions.
  RV_OP(fcvt_l_s),
  RV_OP(fcvt_lu_s),
  RV_OP(fcvt_s_l),
  RV_OP(fcvt_s_lu),

  // RV32D instructions.
  RV_OP(fld),
  RV_OP(fsd),
  RV_OP(fmadd_d),
  RV_OP(fmsub_d),
  RV_OP(fnmsub_d),
  RV_OP(fnmadd_d),
  RV_OP(fadd_d),
  RV_OP(fsub_d),
  RV_OP(fmul_d),
  RV_OP(fdiv_d),
  RV_OP(fsqrt_d),
  RV_OP(fsgnj_d),
  RV_OP(fsgnjn_d),
  RV_OP(fsgnjx_d),
  RV_OP(fmin_d),
  RV_OP(fmax_d),
  RV_OP(fcvt_s_d),
  RV_OP(fcvt_d_s),
  RV_OP(feq_d),
  RV_OP(flt_d),
  RV_OP(fle_d),
  RV_OP(fclass_d),
  RV_OP(fcvt_w_d),
  RV_OP(fcvt_wu_d),
  RV_OP(fcvt_d_w),
  RV_OP(fcvt_d_wu),

  // RV64D instructions.
  RV_OP(fcvt_l_d),
  RV_OP(fcvt_lu_d),
  RV_OP(fmv_x_d),
  RV_OP(fcvt_d_l),
  RV_OP(fcvt_d_lu),
  RV_OP(fmv_d_x),

  // Pseudo ops
  RV_OP(nop),
  RV_OP(not),
  RV_OP(neg),
  RV_OP(fneg_s),
  RV_OP(fneg_d),
  RV_OP(li),
  RV_OP(seqz),
  RV_OP(snez),
  RV_OP(sltz),
  RV_OP(sgtz),
  RV_OP(beqz),
  RV_OP(bnez),
  RV_OP(j),
  RV_OP(call),
  RV_OP(rcall),
  RV_OP(callf),
  RV_OP(rcallf),

  RV_OP(la),      // Load address.
  RV_OP(sext_w),  // Sign extend word.

  // Integer argument registers.
  RV_OP(a0),
  RV_OP(a1),
  RV_OP(a2),
  RV_OP(a3),
  RV_OP(a4),
  RV_OP(a5),
  RV_OP(a6),
  RV_OP(a7),

  // Floating point argument registers.
  RV_OP(fa0),
  RV_OP(fa1),
  RV_OP(fa2),
  RV_OP(fa3),
  RV_OP(fa4),
  RV_OP(fa5),
  RV_OP(fa6),
  RV_OP(fa7),

  // Integer variable registers.
  RV_OP(v0),
  RV_OP(v1),
  RV_OP(v2),
  RV_OP(v3),

  // Floating point variable registers.
  RV_OP(fv0),
  RV_OP(fv1),
  RV_OP(fv2),
  RV_OP(fv3),

  RV_OP(x0),  // Zero reg.

  RV_OP(regarg),  // Holder for reg args.
} RVOpcode;

// Flags for TargetInstruction.
#define RV_HI_RELOC 0x1000
#define RV_LO_RELOC 0x2000
#define RV_PCREL_HI_RELOC 0x4000
#define RV_PCREL_LO_RELOC 0x8000
#define RV_EXPORTED_LABEL 0x10000

typedef struct {
  int reg_num;
  int base_reg_num;
  int offset;  // Negative offset from frame pointer (or zero).
} SavedArgumentRegister;

// Record of loaded registers.  These are used to load
// arguments into a register variable.  The argument is either on
// the stack or already in a registers.
typedef struct {
  int dest_reg;       // Register varaible (reg num).
  bool on_stack;      // True if the source is on the stack (offset from fp).
  bool is_fp;         // True if the reg is floating point.
  bool address_only;  // Calculate address, not contents.
  union {
    int reg;     // Not on stack, source register.
    int offset;  // On stack, positive offset from fp.
  } src;
} RegisterLoad;

// A RISC-V Generator is derived from a TargetGenerator.  It has
// a '.base' field that is the TargetGenerator.
typedef struct RVGenerator {
  TargetGenerator base;

  int num_int_arg_regs;   // Number of args in int regs.
  int num_fp_arg_regs;    // Number of args in floating point regs.
  int num_int_reg_vars;   // Number of int regs used for variables.
  int num_fp_reg_vars;    // Number of floating point regs for vars.
  int struct_return_reg;

  Vector saved_regs;
  Vector register_loads;

  TargetInstruction* int_argument_registers[RV_NUM_INT_ARGS];
  TargetInstruction* fp_argument_registers[RV_NUM_FP_ARGS];
  TargetInstruction* int_variable_registers[RV_MAX_INT_REG_VARS];
  TargetInstruction* fp_variable_registers[RV_MAX_FP_REG_VARS];

  TargetInstruction* zero;

  // Register allocator.
  RVRegisterAllocator register_allocator;
} RVGenerator;

void RVGeneratorInit(RVGenerator* pcode, Generator* gen);
RVGenerator* NewRVGenerator(Generator* gen);

void RVGeneratorDestruct(RVGenerator* pcode);
void RVGeneratorDelete(RVGenerator* pcode);

// Lower the IR to RISC-V.
void RVLower(RVGenerator* pcode, Generator* gen);
void RVPrint(RVGenerator* pcode);

bool RVIsExpression(RVOpcode opcode);
bool RVIsLoad(RVOpcode opcode);
bool RVIsSignedLoad(RVOpcode opcode);
bool RVIsStore(RVOpcode opcode);
bool RVIsFixedRegister(RVOpcode opcode);
bool RVIsIntConst(RVOpcode opcode);
int RVIntValue(TargetInstruction* inst);
bool RVIsPossibleImmediate(int64_t value);

const char* RVOpcodeName(int op);

#endif /* risc_v_codegen_h */
