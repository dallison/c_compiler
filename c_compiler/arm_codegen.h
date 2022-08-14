//
//  arm_codegen.h
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reseARMed.
//

#ifndef arm_codegen_h
#define arm_codegen_h

#include "codegen.h"
#include "hashtable.h"
#include "list.h"
#include "arm_reg_alloc.h"
#include "target_generator.h"
#include "arm_machine.h"

struct TargetBasicBlock;

// If this bit is set in the data.ivalue of a variable pool entry then
// the lower bits contain a register variable number.
#define ARM_REG_VAR 0x80000000

// Since we are using bit 31 for the REG_VAR flag we have to be careful
// with negative offsets.  If the offset it negative bit 30 of the value
// will also be set.  So to check for a register variable, AND with the
// ARM_REG_VAR_MASK and check that bit 30 is not set.
#define ARM_REG_VAR_MASK 0xc0000000
#define ARM_IS_REG_VAR(offset) ((offset & ARM_REG_VAR_MASK) == ARM_REG_VAR)

#define ARM_OP(op) kARM_##op

// ARM code generator opcodes.

typedef enum {
  // The initial sequence for these opcode must match the TargetOpcode
  // enumeration.  The names don't need to match but the positions and
  // operations must.  For example, TARGET_OP(mov) and ARM_OP(mv) are the
  // same operation.
  ARM_OP(save),
  ARM_OP(restore),

  ARM_OP(symbol),   // Static symbol.
  ARM_OP(literal),  // String literal.
  ARM_OP(tmp),

  // Constants.
  ARM_OP(const8),
  ARM_OP(const16),
  ARM_OP(const32),
  ARM_OP(const64),
  ARM_OP(constf),
  ARM_OP(constd),

  ARM_OP(mv),
  ARM_OP(fmv_s),
  ARM_OP(fmv_d),

  ARM_OP(movc),
  ARM_OP(movfc),
  ARM_OP(movdc),
  ARM_OP(movxc),

  ARM_OP(rmov),
  ARM_OP(rmovf),
  ARM_OP(rmovd),

  ARM_OP(ret),

  ARM_OP(label),

  ARM_OP(fp),  // Frame pointer pseudo operation.
  ARM_OP(sp),  // Stack pointer pseudo operation.
  ARM_OP(tp),  // Thread pointer pseudo operation.

  // Function result registers.
  ARM_OP(resulti),
  ARM_OP(resultf),
  ARM_OP(resultd),

  ARM_OP(structreturn),  // Struct return address.

  ARM_OP(asm),

  ARM_OP(loc),
  ARM_OP(named_label),
  ARM_OP(ivarreg),
  ARM_OP(fvarreg),
  
  // End of TargetOpcode enumeration.

  // Now follow the actual RISC-V instruction directly from the
  // specifications.

  // ARM32I instructions.
  ARM_OP(lui),
  ARM_OP(auipc),
  ARM_OP(jal),
  ARM_OP(jalr),
  ARM_OP(beq),
  ARM_OP(bne),
  ARM_OP(blt),
  ARM_OP(bge),
  ARM_OP(bltu),
  ARM_OP(bgeu),
  ARM_OP(lb),
  ARM_OP(lh),
  ARM_OP(lw),
  ARM_OP(lbu),
  ARM_OP(lhu),
  ARM_OP(sb),
  ARM_OP(sh),
  ARM_OP(sw),
  ARM_OP(addi),
  ARM_OP(slti),
  ARM_OP(sltiu),
  ARM_OP(xori),
  ARM_OP(ori),
  ARM_OP(andi),
  ARM_OP(slli),
  ARM_OP(srli),
  ARM_OP(srai),
  ARM_OP(add),
  ARM_OP(sub),
  ARM_OP(sll),
  ARM_OP(slt),
  ARM_OP(sltu),
  ARM_OP(xor),
  ARM_OP(srl),
  ARM_OP(sra),
  ARM_OP(or),
  ARM_OP(and),
  ARM_OP(fence),
  ARM_OP(fence_i),
  ARM_OP(ecall),
  ARM_OP(ebreak),
  ARM_OP(csrrw),
  ARM_OP(csrrs),
  ARM_OP(csrrc),
  ARM_OP(csrrwi),
  ARM_OP(csrrsi),
  ARM_OP(csrrci),

  // ARM64I instructions.
  ARM_OP(lwu),
  ARM_OP(ld),
  ARM_OP(sd),
  ARM_OP(addiw),
  ARM_OP(slliw),
  ARM_OP(srliw),
  ARM_OP(sraiw),
  ARM_OP(addw),
  ARM_OP(subw),
  ARM_OP(sllw),
  ARM_OP(srlw),
  ARM_OP(sraw),

  // ARM32M instructions.
  ARM_OP(mul),
  ARM_OP(mulh),
  ARM_OP(mulhsu),
  ARM_OP(mulhu),
  ARM_OP(div),
  ARM_OP(divu),
  ARM_OP(rem),
  ARM_OP(remu),

  // ARM64M instructions.
  ARM_OP(mulw),
  ARM_OP(divw),
  ARM_OP(divuw),
  ARM_OP(remw),
  ARM_OP(remuw),

  // ARM32F instructions.
  ARM_OP(flw),
  ARM_OP(fsw),
  ARM_OP(fmadd_s),
  ARM_OP(fmsub_s),
  ARM_OP(fnmsub_s),
  ARM_OP(fnmadd_s),
  ARM_OP(fadd_s),
  ARM_OP(fsub_s),
  ARM_OP(fmul_s),
  ARM_OP(fdiv_s),
  ARM_OP(fsqrt_s),
  ARM_OP(fsgnj_s),
  ARM_OP(fsgnjn_s),
  ARM_OP(fsgnjx_s),
  ARM_OP(fmin_s),
  ARM_OP(fmax_s),
  ARM_OP(fcvt_w_s),
  ARM_OP(fcvt_wu_s),
  ARM_OP(fmv_x_w),
  ARM_OP(feq_s),
  ARM_OP(flt_s),
  ARM_OP(fle_s),
  ARM_OP(fclass_s),
  ARM_OP(fcvt_s_w),
  ARM_OP(fcvt_s_wu),
  ARM_OP(fmv_w_x),

  // ARM64F instructions.
  ARM_OP(fcvt_l_s),
  ARM_OP(fcvt_lu_s),
  ARM_OP(fcvt_s_l),
  ARM_OP(fcvt_s_lu),

  // ARM32D instructions.
  ARM_OP(fld),
  ARM_OP(fsd),
  ARM_OP(fmadd_d),
  ARM_OP(fmsub_d),
  ARM_OP(fnmsub_d),
  ARM_OP(fnmadd_d),
  ARM_OP(fadd_d),
  ARM_OP(fsub_d),
  ARM_OP(fmul_d),
  ARM_OP(fdiv_d),
  ARM_OP(fsqrt_d),
  ARM_OP(fsgnj_d),
  ARM_OP(fsgnjn_d),
  ARM_OP(fsgnjx_d),
  ARM_OP(fmin_d),
  ARM_OP(fmax_d),
  ARM_OP(fcvt_s_d),
  ARM_OP(fcvt_d_s),
  ARM_OP(feq_d),
  ARM_OP(flt_d),
  ARM_OP(fle_d),
  ARM_OP(fclass_d),
  ARM_OP(fcvt_w_d),
  ARM_OP(fcvt_wu_d),
  ARM_OP(fcvt_d_w),
  ARM_OP(fcvt_d_wu),

  // ARM64D instructions.
  ARM_OP(fcvt_l_d),
  ARM_OP(fcvt_lu_d),
  ARM_OP(fmv_x_d),
  ARM_OP(fcvt_d_l),
  ARM_OP(fcvt_d_lu),
  ARM_OP(fmv_d_x),

  // Pseudo ops
  ARM_OP(nop),
  ARM_OP(not),
  ARM_OP(neg),
  ARM_OP(fneg_s),
  ARM_OP(fneg_d),
  ARM_OP(li),
  ARM_OP(seqz),
  ARM_OP(snez),
  ARM_OP(sltz),
  ARM_OP(sgtz),
  ARM_OP(beqz),
  ARM_OP(bnez),
  ARM_OP(j),
  ARM_OP(jr),
  ARM_OP(call),
  ARM_OP(rcall),
  ARM_OP(callf),
  ARM_OP(rcallf),

  ARM_OP(la),      // Load address.
  ARM_OP(lla),     // Load local address.
  ARM_OP(sext_w),  // Sign extend word.

  // Integer argument registers.
  ARM_OP(a0),
  ARM_OP(a1),
  ARM_OP(a2),
  ARM_OP(a3),
  ARM_OP(a4),
  ARM_OP(a5),
  ARM_OP(a6),
  ARM_OP(a7),

  // Floating point argument registers.
  ARM_OP(fa0),
  ARM_OP(fa1),
  ARM_OP(fa2),
  ARM_OP(fa3),
  ARM_OP(fa4),
  ARM_OP(fa5),
  ARM_OP(fa6),
  ARM_OP(fa7),

  ARM_OP(nrvoval),
  
  ARM_OP(x0),  // Zero reg.
  ARM_OP(t0),  // Temp reg.

  ARM_OP(regarg),  // Holder for reg args.
  
  // Spill and reload.
  ARM_OP(spill),
  ARM_OP(reload),
} ARMOpcode;

// Flags for TargetInstruction.
#define ARM_HI_RELOC 0x1000
#define ARM_LO_RELOC 0x2000
#define ARM_PCREL_HI_RELOC 0x4000
#define ARM_PCREL_LO_RELOC 0x8000
#define ARM_EXPORTED_LABEL 0x10000

typedef struct {
  int reg_num;
  int base_reg_num;
  int offset;  // Negative offset from frame pointer (or zero).
} SavedArgumentRegister;

typedef struct {
  TargetInstruction* inst;      // Actually a TargetSymbol*.
  int varnum;
  bool is_fp;
} RegisterVariable;

typedef struct ARMGenerator {
  TargetGenerator base;

  int num_int_arg_regs;   // Number of args in int regs.
  int num_fp_arg_regs;    // Number of args in floating point regs.
  int num_int_reg_vars;   // Number of int regs used for variables.
  int num_fp_reg_vars;    // Number of floating point regs for vars.
  int struct_return_reg;
  bool not_leaf;          // Not a leaf procedure.
  
  Vector saved_regs;
  Vector offsets;         // Pointers to Offset.
  
  TargetInstruction* int_argument_registers[ARM_NUM_INT_ARGS];
  TargetInstruction* fp_argument_registers[ARM_NUM_FP_ARGS];
  Vector var_regs;

  
  // Register allocator.
  ARMRegisterAllocator register_allocator;
} ARMGenerator;

void ARMGeneratorInit(ARMGenerator* pcode, Generator* gen);
ARMGenerator* NewARMGenerator(Generator* gen);

void ARMGeneratorDestruct(ARMGenerator* pcode);
void ARMGeneratorDelete(ARMGenerator* pcode);

// Lower the IR to ARM.
void ARMLower(ARMGenerator* pcode, Generator* gen);
void ARMPrint(ARMGenerator* pcode, FILE* fp);

bool ARMIsExpression(TargetInstruction* inst);
bool ARMIsFloatingPoint(TargetInstruction* inst);
bool ARMIsLoad(TargetInstruction* inst);
bool ARMIsSignedLoad(TargetInstruction* inst);
bool ARMIsStore(TargetInstruction* inst);
bool ARMIsFixedRegister(TargetInstruction* inst);
bool ARMIsConst(TargetInstruction* inst);
bool ARMIsSymbol(TargetInstruction* inst);
bool ARMIsIntConst(TargetInstruction* inst);
int ARMIntValue(TargetInstruction* inst);
bool ARMIsPossibleImmediate(int64_t value);
bool ARMIsBranch(TargetInstruction* inst);
bool ARMIsConditionalBranch(TargetInstruction* inst);
bool ARMIsReturn(TargetInstruction* inst);
bool ARMIsCall(TargetInstruction* inst);
bool ARMGeneratesOutput(TargetInstruction* inst);
bool ARMIsResult(TargetInstruction* inst);
bool ARMIsSpill(TargetInstruction* inst);
bool ARMIsLabel(TargetInstruction* inst);
bool ARMIsArgRegister(TargetInstruction* inst);
bool ARMIsVarRegister(TargetInstruction* inst);
bool ARMIsJumpTableEntry(TargetInstruction* inst);

TargetInstruction* ARMGetBranchTarget(TargetInstruction* inst);

const char* ARMOpcodeName(int op);
#endif /* arm_codegen_h */
