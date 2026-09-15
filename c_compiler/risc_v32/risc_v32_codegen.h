//
//  risc_v32_codegen.h
//  c_compiler_library
//
//  Created by David Allison on 2/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v32_codegen_h
#define risc_v32_codegen_h

#include "codegen.h"
#include "hashtable.h"
#include "list.h"
#include "risc_v32_reg_alloc.h"
#include "target_generator.h"

struct TargetBasicBlock;

// If this bit is set in the data.ivalue of a variable pool entry then
// the lower bits contain a register variable number.
#define RV32_REG_VAR 0x80000000

// Since we are using bit 31 for the REG_VAR flag we have to be careful
// with negative offsets.  If the offset it negative bit 30 of the value
// will also be set.  So to check for a register variable, AND with the
// RV32_REG_VAR_MASK and check that bit 30 is not set.
#define RV32_REG_VAR_MASK 0xc0000000
#define RV32_IS_REG_VAR(offset) ((offset & RV32_REG_VAR_MASK) == RV32_REG_VAR)

#define RV32_INST_EXTENDED_ASM (1 << 24)
#define RV32_MAX_ASM_OPERANDS 16

typedef struct {
  TargetInstruction base;
  AsmASTNode* asm_node;
  int num_operands;
  int reg_nums[RV32_MAX_ASM_OPERANDS];
  bool is_fp[RV32_MAX_ASM_OPERANDS];
  int64_t immediate_values[RV32_MAX_ASM_OPERANDS];
} RV32AsmInstruction;

#define RV32_OP(op) kRV32_##op

// RISC-V32 code generator opcodes.

typedef enum {
  // The initial sequence for these opcode must match the TargetOpcode
  // enumeration.  The names don't need to match but the positions and
  // operations must.  For example, TARGET_OP(mov) and RV32_OP(mv) are the
  // same operation.
  RV32_OP(save),
  RV32_OP(restore),

  RV32_OP(symbol),   // Static symbol.
  RV32_OP(literal),  // String literal.
  RV32_OP(tmp),

  // Constants.
  RV32_OP(const8),
  RV32_OP(const16),
  RV32_OP(const32),
  RV32_OP(const64),
  RV32_OP(constf),
  RV32_OP(constd),

  RV32_OP(mv),
  RV32_OP(fmv_s),
  RV32_OP(fmv_d),

  RV32_OP(movc),
  RV32_OP(movfc),
  RV32_OP(movdc),
  RV32_OP(movxc),

//  RV32_OP(rmov),
//  RV32_OP(rmovf),
//  RV32_OP(rmovd),

  RV32_OP(ret),

  RV32_OP(label),

  RV32_OP(fp),  // Frame pointer pseudo operation.
  RV32_OP(sp),  // Stack pointer pseudo operation.
  RV32_OP(tp),  // Thread pointer pseudo operation.

  // Function result registers.
  RV32_OP(resulti),
  RV32_OP(resultf),
  RV32_OP(resultd),

  RV32_OP(structreturn),  // Struct return address.

  RV32_OP(asm),

  RV32_OP(loc),
  RV32_OP(named_label),
  RV32_OP(ivarreg),
  RV32_OP(fvarreg),
  
  // End of TargetOpcode enumeration.

  // Now follow the actual RISC-V32 instruction directly from the
  // specifications.

  // RV32I instructions.
  RV32_OP(lui),
  RV32_OP(auipc),
  RV32_OP(jal),
  RV32_OP(jalr),
  RV32_OP(beq),
  RV32_OP(bne),
  RV32_OP(blt),
  RV32_OP(bge),
  RV32_OP(bltu),
  RV32_OP(bgeu),
  RV32_OP(lb),
  RV32_OP(lh),
  RV32_OP(lw),
  RV32_OP(lbu),
  RV32_OP(lhu),
  RV32_OP(sb),
  RV32_OP(sh),
  RV32_OP(sw),
  RV32_OP(addi),
  RV32_OP(slti),
  RV32_OP(sltiu),
  RV32_OP(xori),
  RV32_OP(ori),
  RV32_OP(andi),
  RV32_OP(slli),
  RV32_OP(srli),
  RV32_OP(srai),
  RV32_OP(add),
  RV32_OP(sub),
  RV32_OP(sll),
  RV32_OP(slt),
  RV32_OP(sltu),
  RV32_OP(xor),
  RV32_OP(srl),
  RV32_OP(sra),
  RV32_OP(or),
  RV32_OP(and),
  RV32_OP(fence),
  RV32_OP(fence_i),
  RV32_OP(ecall),
  RV32_OP(ebreak),
  RV32_OP(csrrw),
  RV32_OP(csrrs),
  RV32_OP(csrrc),
  RV32_OP(csrrwi),
  RV32_OP(csrrsi),
  RV32_OP(csrrci),

  // RV64I instructions.
  RV32_OP(lwu),
  RV32_OP(ld),
  RV32_OP(sd),
  RV32_OP(addiw),
  RV32_OP(slliw),
  RV32_OP(srliw),
  RV32_OP(sraiw),
  RV32_OP(addw),
  RV32_OP(subw),
  RV32_OP(sllw),
  RV32_OP(srlw),
  RV32_OP(sraw),

  // RV32M instructions.
  RV32_OP(mul),
  RV32_OP(mulh),
  RV32_OP(mulhsu),
  RV32_OP(mulhu),
  RV32_OP(div),
  RV32_OP(divu),
  RV32_OP(rem),
  RV32_OP(remu),

  // RV64M instructions.
  RV32_OP(mulw),
  RV32_OP(divw),
  RV32_OP(divuw),
  RV32_OP(remw),
  RV32_OP(remuw),

  // RV32F instructions.
  RV32_OP(flw),
  RV32_OP(fsw),
  RV32_OP(fmadd_s),
  RV32_OP(fmsub_s),
  RV32_OP(fnmsub_s),
  RV32_OP(fnmadd_s),
  RV32_OP(fadd_s),
  RV32_OP(fsub_s),
  RV32_OP(fmul_s),
  RV32_OP(fdiv_s),
  RV32_OP(fsqrt_s),
  RV32_OP(fsgnj_s),
  RV32_OP(fsgnjn_s),
  RV32_OP(fsgnjx_s),
  RV32_OP(fmin_s),
  RV32_OP(fmax_s),
  RV32_OP(fcvt_w_s),
  RV32_OP(fcvt_wu_s),
  RV32_OP(fmv_x_w),
  RV32_OP(feq_s),
  RV32_OP(flt_s),
  RV32_OP(fle_s),
  RV32_OP(fclass_s),
  RV32_OP(fcvt_s_w),
  RV32_OP(fcvt_s_wu),
  RV32_OP(fmv_w_x),

  // RV64F instructions.
  RV32_OP(fcvt_l_s),
  RV32_OP(fcvt_lu_s),
  RV32_OP(fcvt_s_l),
  RV32_OP(fcvt_s_lu),

  // RV32D instructions.
  RV32_OP(fld),
  RV32_OP(fsd),
  RV32_OP(fmadd_d),
  RV32_OP(fmsub_d),
  RV32_OP(fnmsub_d),
  RV32_OP(fnmadd_d),
  RV32_OP(fadd_d),
  RV32_OP(fsub_d),
  RV32_OP(fmul_d),
  RV32_OP(fdiv_d),
  RV32_OP(fsqrt_d),
  RV32_OP(fsgnj_d),
  RV32_OP(fsgnjn_d),
  RV32_OP(fsgnjx_d),
  RV32_OP(fmin_d),
  RV32_OP(fmax_d),
  RV32_OP(fcvt_s_d),
  RV32_OP(fcvt_d_s),
  RV32_OP(feq_d),
  RV32_OP(flt_d),
  RV32_OP(fle_d),
  RV32_OP(fclass_d),
  RV32_OP(fcvt_w_d),
  RV32_OP(fcvt_wu_d),
  RV32_OP(fcvt_d_w),
  RV32_OP(fcvt_d_wu),

  // RV64D instructions.
  RV32_OP(fcvt_l_d),
  RV32_OP(fcvt_lu_d),
  RV32_OP(fmv_x_d),
  RV32_OP(fcvt_d_l),
  RV32_OP(fcvt_d_lu),
  RV32_OP(fmv_d_x),

  // Pseudo ops
  RV32_OP(nop),
  RV32_OP(not),
  RV32_OP(neg),
  RV32_OP(fneg_s),
  RV32_OP(fneg_d),
  RV32_OP(li),
  RV32_OP(seqz),
  RV32_OP(snez),
  RV32_OP(sltz),
  RV32_OP(sgtz),
  RV32_OP(beqz),
  RV32_OP(bnez),
  RV32_OP(j),
  RV32_OP(jr),
  RV32_OP(call),
  RV32_OP(rcall),
  RV32_OP(callf),
  RV32_OP(rcallf),

  RV32_OP(la),      // Load address.
  RV32_OP(lla),     // Load local address.
  RV32_OP(tprel),   // Load local-exec TLS address relative to tp.
  RV32_OP(tlsgd),   // Materialize the global-dynamic TLS index GOT address.
  RV32_OP(sext_w),  // Sign extend word.

  // Integer argument registers.
  RV32_OP(a0),
  RV32_OP(a1),
  RV32_OP(a2),
  RV32_OP(a3),
  RV32_OP(a4),
  RV32_OP(a5),
  RV32_OP(a6),
  RV32_OP(a7),

  // Floating point argument registers.
  RV32_OP(fa0),
  RV32_OP(fa1),
  RV32_OP(fa2),
  RV32_OP(fa3),
  RV32_OP(fa4),
  RV32_OP(fa5),
  RV32_OP(fa6),
  RV32_OP(fa7),

  RV32_OP(nrvoval),
  
  RV32_OP(x0),  // Zero reg.
  RV32_OP(t0),  // Temp reg.
  RV32_OP(t1),  // Secondary temp reg.
  RV32_OP(t2),  // Tertiary temp reg.

  RV32_OP(regarg),  // Holder for reg args.

  RV32_OP(atomic_load),
  RV32_OP(atomic_store),
  RV32_OP(atomic_fetch_add),
  RV32_OP(atomic_fetch_sub),
  RV32_OP(atomic_add_fetch),
  RV32_OP(atomic_sub_fetch),
  RV32_OP(atomic_compare_exchange_bool),
  RV32_OP(atomic_compare_exchange_val),
  RV32_OP(atomic_compare_exchange_n),
  RV32_OP(atomic_fence),
  
  // Spill and reload.
  RV32_OP(spill),
  RV32_OP(reload),
} RV32Opcode;

// Flags for TargetInstruction.
#define RV32_HI_RELOC 0x1000
#define RV32_LO_RELOC 0x2000
#define RV32_PCREL_HI_RELOC 0x4000
#define RV32_PCREL_LO_RELOC 0x8000
#define RV32_EXPORTED_LABEL 0x10000

// A register-to-register move that copies a value into a physical argument
// register as part of a call's argument setup.  After register allocation
// these moves form a parallel copy; ResolveArgumentMoves reorders them (and
// breaks cycles with a scratch temporary) so they don't clobber each other.
#define RV32_INST_ARG_MOVE 0x20000

// Marks an RV32_OP(tmp) placeholder as holding a floating-point value, so the
// register allocator assigns it a float register.  The bare tmp opcode carries
// no type, so without this the allocator would default it to an integer
// register and a float move into it (fmv.d/fmv.s) would target the wrong
// register file.
#define RV32_INST_FLOAT_TMP 0x40000

// Atomic pseudo metadata in the target-instruction flag word.
#define RV32_ATOMIC_SIZE_SHIFT 18
#define RV32_ATOMIC_SIZE_MASK (3 << RV32_ATOMIC_SIZE_SHIFT)
#define RV32_ATOMIC_ORDER_SHIFT 20
#define RV32_ATOMIC_ORDER_MASK (7 << RV32_ATOMIC_ORDER_SHIFT)
#define RV32_ATOMIC_FAILURE_ORDER_SHIFT 25
#define RV32_ATOMIC_FAILURE_ORDER_MASK (7 << RV32_ATOMIC_FAILURE_ORDER_SHIFT)
#define RV32_ATOMIC_WEAK (1 << 28)

typedef struct {
  int reg_num;
  int base_reg_num;
  int offset;  // Negative offset from frame pointer (or zero).
  int size;
  bool is_fp;
} SavedArgumentRegister;


typedef struct {
  TargetInstruction* inst;      // Actually a TargetSymbol*.
  int varnum;
  bool is_fp;
} RegisterVariable;

typedef struct {
  TargetInstruction* try_start;
  TargetInstruction* try_end;
  TargetInstruction* catch_label;
  EHTypeInfo* catch_typeinfo;
  bool is_cleanup;
} RV32ExceptionRange;

// A RISC-V32 Generator is derived from a TargetGenerator.  It has
// a '.base' field that is the TargetGenerator.
typedef struct RV32Generator {
  TargetGenerator base;

  int num_int_arg_regs;   // Number of args in int regs.
  int num_fp_arg_regs;    // Number of args in floating point regs.
  int num_int_reg_vars;   // Number of int regs used for variables.
  int num_fp_reg_vars;    // Number of floating point regs for vars.
  int struct_return_reg;
  bool not_leaf;          // Not a leaf procedure.
  
  Vector saved_regs;
  
  TargetInstruction* int_argument_registers[RV32_NUM_INT_ARGS];
  TargetInstruction* fp_argument_registers[RV32_NUM_FP_ARGS];
  Vector var_regs;
  Vector exception_ranges;
  Vector exception_typeinfos;

  TargetInstruction* zero;  // Explicit zero (register x0).
  TargetInstruction* tmp;   // Temp reg for tail calls vi jr instruction.
  TargetInstruction* tmp2;  // Secondary temp reg.
  
  // Register allocator.
  RV32RegisterAllocator register_allocator;
} RV32Generator;

void RV32GeneratorInit(RV32Generator* pcode, Generator* gen);
RV32Generator* NewRV32Generator(Generator* gen);

void RV32GeneratorDestruct(RV32Generator* pcode);
void RV32GeneratorDelete(RV32Generator* pcode);

// Lower the IR to RISC-V32.
void RV32Lower(RV32Generator* pcode, Generator* gen);
void RV32Print(RV32Generator* pcode, FILE* fp);

bool RV32IsExpression(TargetInstruction* inst);
bool RV32IsFloatingPoint(TargetInstruction* inst);
bool RV32IsLoad(TargetInstruction* inst);
bool RV32IsSignedLoad(TargetInstruction* inst);
bool RV32IsStore(TargetInstruction* inst);
bool RV32IsFixedRegister(TargetInstruction* inst);
bool RV32IsConst(TargetInstruction* inst);
bool RV32IsSymbol(TargetInstruction* inst);
bool RV32IsIntConst(TargetInstruction* inst);
int64_t RV32IntValue(TargetInstruction* inst);
bool RV32IsPossibleImmediate(int64_t value);
bool RV32IsBranch(TargetInstruction* inst);
bool RV32IsConditionalBranch(TargetInstruction* inst);
bool RV32IsReturn(TargetInstruction* inst);
bool RV32IsCall(TargetInstruction* inst);
bool RV32GeneratesOutput(TargetInstruction* inst);
bool RV32IsResult(TargetInstruction* inst);
bool RV32IsSpill(TargetInstruction* inst);
bool RV32IsLabel(TargetInstruction* inst);
bool RV32IsArgRegister(TargetInstruction* inst);
bool RV32IsVarRegister(TargetInstruction* inst);
bool RV32IsJumpTableEntry(TargetInstruction* inst);

TargetInstruction* RV32GetBranchTarget(TargetInstruction* inst);

const char* RV32OpcodeName(int op);

#endif /* risc_v32_codegen_h */
