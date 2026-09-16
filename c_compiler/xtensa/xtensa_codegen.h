//
//  xtensa_codegen.h
//  c_compiler_library
//
//  Created by David Allison on 2/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef xtensa_codegen_h
#define xtensa_codegen_h

#include "codegen.h"
#include "hashtable.h"
#include "list.h"
#include "target_generator.h"
#include "xtensa_reg_alloc.h"

struct TargetBasicBlock;

// If this bit is set in the data.ivalue of a variable pool entry then
// the lower bits contain a register variable number.
#define XTENSA_REG_VAR 0x80000000

// Since we are using bit 31 for the REG_VAR flag we have to be careful
// with negative offsets.  If the offset it negative bit 30 of the value
// will also be set.  So to check for a register variable, AND with the
// XTENSA_REG_VAR_MASK and check that bit 30 is not set.
#define XTENSA_REG_VAR_MASK 0xc0000000
#define XTENSA_IS_REG_VAR(offset) \
  ((offset & XTENSA_REG_VAR_MASK) == XTENSA_REG_VAR)

#define XTENSA_INST_EXTENDED_ASM (1 << 24)
#define XTENSA_MAX_ASM_OPERANDS 16

typedef struct {
  TargetInstruction base;
  AsmASTNode* asm_node;
  int num_operands;
  int reg_nums[XTENSA_MAX_ASM_OPERANDS];
  bool is_fp[XTENSA_MAX_ASM_OPERANDS];
  int64_t immediate_values[XTENSA_MAX_ASM_OPERANDS];
} XTENSAAsmInstruction;

#define XTENSA_OP(op) kXTENSA_##op

// RISC-V32 code generator opcodes.

typedef enum {
  // The initial sequence for these opcode must match the TargetOpcode
  // enumeration.  The names don't need to match but the positions and
  // operations must.  For example, TARGET_OP(mov) and XTENSA_OP(mv) are the
  // same operation.
  XTENSA_OP(save),
  XTENSA_OP(restore),

  XTENSA_OP(symbol),   // Static symbol.
  XTENSA_OP(literal),  // String literal.
  XTENSA_OP(tmp),

  // Constants.
  XTENSA_OP(const8),
  XTENSA_OP(const16),
  XTENSA_OP(const32),
  XTENSA_OP(const64),
  XTENSA_OP(constf),
  XTENSA_OP(constd),

  XTENSA_OP(mv),
  XTENSA_OP(fmv_s),
  XTENSA_OP(fmv_d),

  XTENSA_OP(movc),
  XTENSA_OP(movfc),
  XTENSA_OP(movdc),
  XTENSA_OP(movxc),

  //  XTENSA_OP(rmov),
  //  XTENSA_OP(rmovf),
  //  XTENSA_OP(rmovd),

  XTENSA_OP(ret),

  XTENSA_OP(label),

  XTENSA_OP(fp),  // Frame pointer pseudo operation.
  XTENSA_OP(sp),  // Stack pointer pseudo operation.
  XTENSA_OP(tp),  // Thread pointer pseudo operation.

  // Function result registers.
  XTENSA_OP(resulti),
  XTENSA_OP(resultf),
  XTENSA_OP(resultd),

  XTENSA_OP(structreturn),  // Struct return address.

  XTENSA_OP(asm),

  XTENSA_OP(loc),
  XTENSA_OP(named_label),
  XTENSA_OP(ivarreg),
  XTENSA_OP(fvarreg),

  // End of TargetOpcode enumeration.

  // Now follow the actual RISC-V32 instruction directly from the
  // specifications.

  // XTENSAI instructions.
  XTENSA_OP(lui),
  XTENSA_OP(auipc),
  XTENSA_OP(jal),
  XTENSA_OP(jalr),
  XTENSA_OP(beq),
  XTENSA_OP(bne),
  XTENSA_OP(blt),
  XTENSA_OP(bge),
  XTENSA_OP(bltu),
  XTENSA_OP(bgeu),
  XTENSA_OP(lb),
  XTENSA_OP(lh),
  XTENSA_OP(lw),
  XTENSA_OP(lbu),
  XTENSA_OP(lhu),
  XTENSA_OP(sb),
  XTENSA_OP(sh),
  XTENSA_OP(sw),
  XTENSA_OP(addi),
  XTENSA_OP(slti),
  XTENSA_OP(sltiu),
  XTENSA_OP(xori),
  XTENSA_OP(ori),
  XTENSA_OP(andi),
  XTENSA_OP(slli),
  XTENSA_OP(srli),
  XTENSA_OP(srai),
  XTENSA_OP(add),
  XTENSA_OP(sub),
  XTENSA_OP(sll),
  XTENSA_OP(slt),
  XTENSA_OP(sltu),
  XTENSA_OP(xor),
  XTENSA_OP(srl),
  XTENSA_OP(sra),
  XTENSA_OP(or),
  XTENSA_OP(and),
  XTENSA_OP(fence),
  XTENSA_OP(fence_i),
  XTENSA_OP(ecall),
  XTENSA_OP(ebreak),
  XTENSA_OP(csrrw),
  XTENSA_OP(csrrs),
  XTENSA_OP(csrrc),
  XTENSA_OP(csrrwi),
  XTENSA_OP(csrrsi),
  XTENSA_OP(csrrci),

  // RV64I instructions.
  XTENSA_OP(lwu),
  XTENSA_OP(ld),
  XTENSA_OP(sd),
  XTENSA_OP(addiw),
  XTENSA_OP(slliw),
  XTENSA_OP(srliw),
  XTENSA_OP(sraiw),
  XTENSA_OP(addw),
  XTENSA_OP(subw),
  XTENSA_OP(sllw),
  XTENSA_OP(srlw),
  XTENSA_OP(sraw),

  // XTENSAM instructions.
  XTENSA_OP(mul),
  XTENSA_OP(mulh),
  XTENSA_OP(mulhsu),
  XTENSA_OP(mulhu),
  XTENSA_OP(div),
  XTENSA_OP(divu),
  XTENSA_OP(rem),
  XTENSA_OP(remu),

  // RV64M instructions.
  XTENSA_OP(mulw),
  XTENSA_OP(divw),
  XTENSA_OP(divuw),
  XTENSA_OP(remw),
  XTENSA_OP(remuw),

  // XTENSAF instructions.
  XTENSA_OP(flw),
  XTENSA_OP(fsw),
  XTENSA_OP(fmadd_s),
  XTENSA_OP(fmsub_s),
  XTENSA_OP(fnmsub_s),
  XTENSA_OP(fnmadd_s),
  XTENSA_OP(fadd_s),
  XTENSA_OP(fsub_s),
  XTENSA_OP(fmul_s),
  XTENSA_OP(fdiv_s),
  XTENSA_OP(fsqrt_s),
  XTENSA_OP(fsgnj_s),
  XTENSA_OP(fsgnjn_s),
  XTENSA_OP(fsgnjx_s),
  XTENSA_OP(fmin_s),
  XTENSA_OP(fmax_s),
  XTENSA_OP(fcvt_w_s),
  XTENSA_OP(fcvt_wu_s),
  XTENSA_OP(fmv_x_w),
  XTENSA_OP(feq_s),
  XTENSA_OP(flt_s),
  XTENSA_OP(fle_s),
  XTENSA_OP(fclass_s),
  XTENSA_OP(fcvt_s_w),
  XTENSA_OP(fcvt_s_wu),
  XTENSA_OP(fmv_w_x),

  // RV64F instructions.
  XTENSA_OP(fcvt_l_s),
  XTENSA_OP(fcvt_lu_s),
  XTENSA_OP(fcvt_s_l),
  XTENSA_OP(fcvt_s_lu),

  // XTENSAD instructions.
  XTENSA_OP(fld),
  XTENSA_OP(fsd),
  XTENSA_OP(fmadd_d),
  XTENSA_OP(fmsub_d),
  XTENSA_OP(fnmsub_d),
  XTENSA_OP(fnmadd_d),
  XTENSA_OP(fadd_d),
  XTENSA_OP(fsub_d),
  XTENSA_OP(fmul_d),
  XTENSA_OP(fdiv_d),
  XTENSA_OP(fsqrt_d),
  XTENSA_OP(fsgnj_d),
  XTENSA_OP(fsgnjn_d),
  XTENSA_OP(fsgnjx_d),
  XTENSA_OP(fmin_d),
  XTENSA_OP(fmax_d),
  XTENSA_OP(fcvt_s_d),
  XTENSA_OP(fcvt_d_s),
  XTENSA_OP(feq_d),
  XTENSA_OP(flt_d),
  XTENSA_OP(fle_d),
  XTENSA_OP(fclass_d),
  XTENSA_OP(fcvt_w_d),
  XTENSA_OP(fcvt_wu_d),
  XTENSA_OP(fcvt_d_w),
  XTENSA_OP(fcvt_d_wu),

  // RV64D instructions.
  XTENSA_OP(fcvt_l_d),
  XTENSA_OP(fcvt_lu_d),
  XTENSA_OP(fmv_x_d),
  XTENSA_OP(fcvt_d_l),
  XTENSA_OP(fcvt_d_lu),
  XTENSA_OP(fmv_d_x),

  // Pseudo ops
  XTENSA_OP(nop),
  XTENSA_OP(not),
  XTENSA_OP(neg),
  XTENSA_OP(fneg_s),
  XTENSA_OP(fneg_d),
  XTENSA_OP(li),
  XTENSA_OP(seqz),
  XTENSA_OP(snez),
  XTENSA_OP(sltz),
  XTENSA_OP(sgtz),
  XTENSA_OP(beqz),
  XTENSA_OP(bnez),
  XTENSA_OP(j),
  XTENSA_OP(jr),
  XTENSA_OP(call),
  XTENSA_OP(rcall),
  XTENSA_OP(callf),
  XTENSA_OP(rcallf),

  XTENSA_OP(la),      // Load address.
  XTENSA_OP(lla),     // Load local address.
  XTENSA_OP(tprel),   // Load local-exec TLS address relative to tp.
  XTENSA_OP(tlsgd),   // Materialize the global-dynamic TLS index GOT address.
  XTENSA_OP(sext_w),  // Sign extend word.

  // Integer argument registers.
  XTENSA_OP(a0),
  XTENSA_OP(a1),
  XTENSA_OP(a2),
  XTENSA_OP(a3),
  XTENSA_OP(a4),
  XTENSA_OP(a5),
  XTENSA_OP(a6),
  XTENSA_OP(a7),

  // Floating point argument registers.
  XTENSA_OP(fa0),
  XTENSA_OP(fa1),
  XTENSA_OP(fa2),
  XTENSA_OP(fa3),
  XTENSA_OP(fa4),
  XTENSA_OP(fa5),
  XTENSA_OP(fa6),
  XTENSA_OP(fa7),

  XTENSA_OP(nrvoval),

  XTENSA_OP(x0),  // Zero reg.
  XTENSA_OP(t0),  // Temp reg.
  XTENSA_OP(t1),  // Secondary temp reg.
  XTENSA_OP(t2),  // Tertiary temp reg.

  XTENSA_OP(regarg),  // Holder for reg args.

  XTENSA_OP(atomic_load),
  XTENSA_OP(atomic_store),
  XTENSA_OP(atomic_fetch_add),
  XTENSA_OP(atomic_fetch_sub),
  XTENSA_OP(atomic_add_fetch),
  XTENSA_OP(atomic_sub_fetch),
  XTENSA_OP(atomic_compare_exchange_bool),
  XTENSA_OP(atomic_compare_exchange_val),
  XTENSA_OP(atomic_compare_exchange_n),
  XTENSA_OP(atomic_fence),

  // Spill and reload.
  XTENSA_OP(spill),
  XTENSA_OP(reload),
} XTENSAOpcode;

// Flags for TargetInstruction.
#define XTENSA_HI_RELOC 0x1000
#define XTENSA_LO_RELOC 0x2000
#define XTENSA_PCREL_HI_RELOC 0x4000
#define XTENSA_PCREL_LO_RELOC 0x8000
#define XTENSA_EXPORTED_LABEL 0x10000

// A register-to-register move that copies a value into a physical argument
// register as part of a call's argument setup.  After register allocation
// these moves form a parallel copy; ResolveArgumentMoves reorders them (and
// breaks cycles with a scratch temporary) so they don't clobber each other.
#define XTENSA_INST_ARG_MOVE 0x20000

// Marks an XTENSA_OP(tmp) placeholder as holding a floating-point value, so the
// register allocator assigns it a float register.  The bare tmp opcode carries
// no type, so without this the allocator would default it to an integer
// register and a float move into it (fmv.d/fmv.s) would target the wrong
// register file.
#define XTENSA_INST_FLOAT_TMP 0x40000

// Atomic pseudo metadata in the target-instruction flag word.
#define XTENSA_ATOMIC_SIZE_SHIFT 18
#define XTENSA_ATOMIC_SIZE_MASK (3 << XTENSA_ATOMIC_SIZE_SHIFT)
#define XTENSA_ATOMIC_ORDER_SHIFT 20
#define XTENSA_ATOMIC_ORDER_MASK (7 << XTENSA_ATOMIC_ORDER_SHIFT)
#define XTENSA_ATOMIC_FAILURE_ORDER_SHIFT 25
#define XTENSA_ATOMIC_FAILURE_ORDER_MASK \
  (7 << XTENSA_ATOMIC_FAILURE_ORDER_SHIFT)
#define XTENSA_ATOMIC_WEAK (1 << 28)

typedef struct {
  int reg_num;
  int base_reg_num;
  int offset;  // Negative offset from frame pointer (or zero).
  int size;
  bool is_fp;
} SavedArgumentRegister;

typedef struct {
  TargetInstruction* inst;  // Actually a TargetSymbol*.
  int varnum;
  bool is_fp;
} RegisterVariable;

typedef struct {
  TargetInstruction* try_start;
  TargetInstruction* try_end;
  TargetInstruction* catch_label;
  EHTypeInfo* catch_typeinfo;
  bool is_cleanup;
} XTENSAExceptionRange;

// A RISC-V32 Generator is derived from a TargetGenerator.  It has
// a '.base' field that is the TargetGenerator.
typedef struct XTENSAGenerator {
  TargetGenerator base;

  int num_int_arg_regs;  // Number of args in int regs.
  int num_fp_arg_regs;   // Number of args in floating point regs.
  int num_int_reg_vars;  // Number of int regs used for variables.
  int num_fp_reg_vars;   // Number of floating point regs for vars.
  int struct_return_reg;
  bool not_leaf;  // Not a leaf procedure.

  Vector saved_regs;

  TargetInstruction* int_argument_registers[XTENSA_NUM_INT_ARGS];
  TargetInstruction* fp_argument_registers[XTENSA_NUM_FP_ARGS];
  Vector var_regs;
  Vector exception_ranges;
  Vector exception_typeinfos;

  TargetInstruction* zero;  // Explicit zero (register x0).
  TargetInstruction* tmp;   // Temp reg for tail calls vi jr instruction.
  TargetInstruction* tmp2;  // Secondary temp reg.

  // Register allocator.
  XTENSARegisterAllocator register_allocator;
} XTENSAGenerator;

void XTENSAGeneratorInit(XTENSAGenerator* pcode, Generator* gen);
XTENSAGenerator* NewXTENSAGenerator(Generator* gen);

void XTENSAGeneratorDestruct(XTENSAGenerator* pcode);
void XTENSAGeneratorDelete(XTENSAGenerator* pcode);

// Lower the IR to RISC-V32.
void XTENSALower(XTENSAGenerator* pcode, Generator* gen);
void XTENSAPrint(XTENSAGenerator* pcode, FILE* fp);

bool XTENSAIsExpression(TargetInstruction* inst);
bool XTENSAIsFloatingPoint(TargetInstruction* inst);
bool XTENSAIsLoad(TargetInstruction* inst);
bool XTENSAIsSignedLoad(TargetInstruction* inst);
bool XTENSAIsStore(TargetInstruction* inst);
bool XTENSAIsFixedRegister(TargetInstruction* inst);
bool XTENSAIsConst(TargetInstruction* inst);
bool XTENSAIsSymbol(TargetInstruction* inst);
bool XTENSAIsIntConst(TargetInstruction* inst);
int64_t XTENSAIntValue(TargetInstruction* inst);
bool XTENSAIsPossibleImmediate(int64_t value);
bool XTENSAIsBranch(TargetInstruction* inst);
bool XTENSAIsConditionalBranch(TargetInstruction* inst);
bool XTENSAIsReturn(TargetInstruction* inst);
bool XTENSAIsCall(TargetInstruction* inst);
bool XTENSAGeneratesOutput(TargetInstruction* inst);
bool XTENSAIsResult(TargetInstruction* inst);
bool XTENSAIsSpill(TargetInstruction* inst);
bool XTENSAIsLabel(TargetInstruction* inst);
bool XTENSAIsArgRegister(TargetInstruction* inst);
bool XTENSAIsVarRegister(TargetInstruction* inst);
bool XTENSAIsJumpTableEntry(TargetInstruction* inst);

TargetInstruction* XTENSAGetBranchTarget(TargetInstruction* inst);

const char* XTENSAOpcodeName(int op);

#endif /* xtensa_codegen_h */
