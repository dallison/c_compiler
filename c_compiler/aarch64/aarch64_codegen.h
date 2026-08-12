//
//  aarch64_codegen.h
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reseAARCH64ed.
//

#ifndef aarch64_codegen_h
#define aarch64_codegen_h

#include "codegen.h"
#include "hashtable.h"
#include "list.h"
#include "aarch64_reg_alloc.h"
#include "target_generator.h"
#include "aarch64_machine.h"

struct TargetBasicBlock;

// If this bit is set in the data.ivalue of a variable pool entry then
// the lower bits contain a register variable number.
#define AARCH64_REG_VAR 0x80000000

// Since we are using bit 31 for the REG_VAR flag we have to be careful
// with negative offsets.  If the offset it negative bit 30 of the value
// will also be set.  So to check for a register variable, AND with the
// AARCH64_REG_VAR_MASK and check that bit 30 is not set.
#define AARCH64_REG_VAR_MASK 0xc0000000
#define AARCH64_IS_REG_VAR(offset) ((offset & AARCH64_REG_VAR_MASK) == AARCH64_REG_VAR)

// Instruction size bits are bist 16:17 of flags (2 bits).  Not 0 and 1 so we
// can tell if the size is not set correctly.
#define kAARCH64InstructionSize (1 << 16)
#define kSize32Bit 1
#define kSize64Bit 2

// AARCH64 addressing modes. The value is stored in bit 21:18
//  of the TargetInstruction's flags member.
typedef enum {
  kAddrModeUnknown = 0,
  kAddrModeImplied = 1,
  kAddrModeRelative,
  kAddrModeInvalid,
} AddressingMode;

// This comparison was generated.  Used to in conditional branch.
#define kAARCH64ComparisonGenerated (1 << 22)

// Atomic pseudo metadata in the target-instruction flag word.
#define AARCH64_ATOMIC_SIZE_SHIFT 18
#define AARCH64_ATOMIC_SIZE_MASK (3 << AARCH64_ATOMIC_SIZE_SHIFT)
#define AARCH64_ATOMIC_ORDER_SHIFT 23
#define AARCH64_ATOMIC_ORDER_MASK (7 << AARCH64_ATOMIC_ORDER_SHIFT)
#define AARCH64_ATOMIC_FAILURE_ORDER_SHIFT 26
#define AARCH64_ATOMIC_FAILURE_ORDER_MASK \
  (7 << AARCH64_ATOMIC_FAILURE_ORDER_SHIFT)
#define AARCH64_ATOMIC_WEAK (1 << 29)

#define AARCH64_OP(op) kAARCH64_##op

// AARCH64 code generator opcodes.

typedef enum {
  // The initial sequence for these opcode must match the TargetOpcode
  // enumeration.  The names don't need to match but the positions and
  // operations must.  For example, TARGET_OP(mov) and AARCH64_OP(mv) are the
  // same operation.
  AARCH64_OP(save),
  AARCH64_OP(restore),

  AARCH64_OP(symbol),   // Static symbol.
  AARCH64_OP(literal),  // String literal.
  AARCH64_OP(tmp),

  // Constants.
  AARCH64_OP(const8),
  AARCH64_OP(const16),
  AARCH64_OP(const32),
  AARCH64_OP(const64),
  AARCH64_OP(constf),
  AARCH64_OP(constd),

  AARCH64_OP(mv),
  AARCH64_OP(fmv_s),
  AARCH64_OP(fmv_d),

  AARCH64_OP(movc),
  AARCH64_OP(movfc),
  AARCH64_OP(movdc),
  AARCH64_OP(movxc),

  AARCH64_OP(ret),

  AARCH64_OP(label),

  AARCH64_OP(fp),  // Frame pointer pseudo operation.
  AARCH64_OP(sp),  // Stack pointer pseudo operation.
  AARCH64_OP(tp),  // Thread pointer pseudo operation.

  // Function result registers.
  AARCH64_OP(resulti),
  AARCH64_OP(resultf),
  AARCH64_OP(resultd),

  AARCH64_OP(structreturn),  // Struct return address.

  AARCH64_OP(asm),

  AARCH64_OP(loc),
  AARCH64_OP(named_label),
  AARCH64_OP(ivarreg),
  AARCH64_OP(fvarreg),
  
  // End of TargetOpcode enumeration.

  // Now follow the actual AARCH64v8 64 instruction directly from the
  // specifications.

  AARCH64_OP(adc),
  AARCH64_OP(add),
  AARCH64_OP(adcs),
  AARCH64_OP(adds),
  AARCH64_OP(adr),
  AARCH64_OP(adrp),
  AARCH64_OP(cmn),
  AARCH64_OP(cmp),
  AARCH64_OP(madd),
  AARCH64_OP(mneg),
  AARCH64_OP(msub),
  AARCH64_OP(mul),
  AARCH64_OP(neg),
  AARCH64_OP(ngc),
  AARCH64_OP(sbc),
  AARCH64_OP(negs),
  AARCH64_OP(ngcs),
  AARCH64_OP(sbcs),
  AARCH64_OP(sdiv),
  AARCH64_OP(smod),     // Pseudo instruction, emitter produces 2 instructions.
  AARCH64_OP(smaddl),
  AARCH64_OP(smnegl),
  AARCH64_OP(smsubl),
  AARCH64_OP(smulh),
  AARCH64_OP(smull),
  AARCH64_OP(sub),
  AARCH64_OP(subs),
  AARCH64_OP(udiv),
  AARCH64_OP(umod),     // Pseudo instruction.
  AARCH64_OP(umaddl),
  AARCH64_OP(umnegl),
  AARCH64_OP(umsubl),
  AARCH64_OP(umulh),
  AARCH64_OP(umull),
  
  AARCH64_OP(bfi),
  AARCH64_OP(bfxil),
  AARCH64_OP(cls),
  AARCH64_OP(clz),
  AARCH64_OP(extr),
  AARCH64_OP(rbit),
  AARCH64_OP(rev),
  AARCH64_OP(rev16),
  AARCH64_OP(rev32),
  AARCH64_OP(sbfiz),
  AARCH64_OP(ubfiz),
  AARCH64_OP(sbfx),
  AARCH64_OP(ubfx),
  AARCH64_OP(sbxt),
  AARCH64_OP(sbxtb),
  AARCH64_OP(sbxth),
  AARCH64_OP(ubxt),
  AARCH64_OP(ubxtb),
  AARCH64_OP(ubxth),
  AARCH64_OP(sxtb),
  AARCH64_OP(sxth),
  AARCH64_OP(sxtw),

  AARCH64_OP(and),
  AARCH64_OP(ands),
  AARCH64_OP(asr),
  AARCH64_OP(asrv),
  AARCH64_OP(bic),
  AARCH64_OP(bics),
  AARCH64_OP(eon),
  AARCH64_OP(eons),
  AARCH64_OP(lsl),
  AARCH64_OP(lslv),
  AARCH64_OP(lsr),
  AARCH64_OP(lsrv),
  AARCH64_OP(mov),
  AARCH64_OP(movk),
  AARCH64_OP(movn),
  AARCH64_OP(movz),
  AARCH64_OP(mvn),
  AARCH64_OP(orn),
  AARCH64_OP(orr),
  AARCH64_OP(ror),
  AARCH64_OP(rorv),
  AARCH64_OP(tst),
  AARCH64_OP(eor),

  AARCH64_OP(b),
  AARCH64_OP(bl),
  AARCH64_OP(blr),
  AARCH64_OP(br),
  AARCH64_OP(cbnz),
  AARCH64_OP(cbz),
  AARCH64_OP(tbnz),
  AARCH64_OP(tbz),
  
  // Condition codes for csel instruction.
  AARCH64_OP(eq),
  AARCH64_OP(ne),
  AARCH64_OP(cs),
  AARCH64_OP(hs),
  AARCH64_OP(cc),
  AARCH64_OP(lo),
  AARCH64_OP(mi),
  AARCH64_OP(pl),
  AARCH64_OP(vs),
  AARCH64_OP(vc),
  AARCH64_OP(hi),
  AARCH64_OP(ls),
  AARCH64_OP(ge),
  AARCH64_OP(lt),
  AARCH64_OP(gt),
  AARCH64_OP(le),
  AARCH64_OP(al),
 

  AARCH64_OP(ccmn),
  AARCH64_OP(ccmp),
  AARCH64_OP(cinc),
  AARCH64_OP(cinv),
  AARCH64_OP(cneg),
  AARCH64_OP(csel),
  AARCH64_OP(cset),
  AARCH64_OP(csetm),
  AARCH64_OP(csinc),
  AARCH64_OP(csinv),
  AARCH64_OP(csneg),
  
  AARCH64_OP(ldp),
  AARCH64_OP(ldpsw),
  AARCH64_OP(ldr),
  AARCH64_OP(ldur),
  AARCH64_OP(ldrb),
  AARCH64_OP(ldrh),
  AARCH64_OP(ldurb),
  AARCH64_OP(ldurh),
  AARCH64_OP(ldrsb),
  AARCH64_OP(ldrsh),
  AARCH64_OP(ldursb),
  AARCH64_OP(ldursh),
  AARCH64_OP(ldursw),
  AARCH64_OP(prfm),
  AARCH64_OP(stp),
  AARCH64_OP(str),
  AARCH64_OP(stur),
  AARCH64_OP(strb),
  AARCH64_OP(strh),
  AARCH64_OP(sturb),
  AARCH64_OP(sturh),

  // ARMv8.0 acquire/release, exclusive, and barrier instructions.
  AARCH64_OP(ldxr),
  AARCH64_OP(ldaxr),
  AARCH64_OP(stxr),
  AARCH64_OP(stlxr),
  AARCH64_OP(ldar),
  AARCH64_OP(stlr),
  AARCH64_OP(dmb),
  AARCH64_OP(clrex),

  // Atomic operation pseudos.  The emitter expands these to ARMv8.0
  // load-exclusive/store-exclusive loops after register allocation.
  AARCH64_OP(atomic_load),
  AARCH64_OP(atomic_store),
  AARCH64_OP(atomic_fetch_add),
  AARCH64_OP(atomic_fetch_sub),
  AARCH64_OP(atomic_add_fetch),
  AARCH64_OP(atomic_sub_fetch),
  AARCH64_OP(atomic_compare_exchange_bool),
  AARCH64_OP(atomic_compare_exchange_val),
  AARCH64_OP(atomic_compare_exchange_n),
  AARCH64_OP(atomic_fence),

  AARCH64_OP(fldr),
  AARCH64_OP(fstr),
  AARCH64_OP(fadd),
  AARCH64_OP(fsub),
  AARCH64_OP(fmul),
  AARCH64_OP(fdiv),
  AARCH64_OP(fsqrt),
  AARCH64_OP(fmin),
  AARCH64_OP(fmax),
  AARCH64_OP(fcvtsd),     // Single to double
  AARCH64_OP(fcvtds),     // Double to single.
  AARCH64_OP(fcvtns),
  AARCH64_OP(fcvtnu),
  AARCH64_OP(fcvtzs),
  AARCH64_OP(fcvtzu),
  AARCH64_OP(fcvt),     // Copy from int reg (no conversion)
  AARCH64_OP(fmov),
  AARCH64_OP(fcmp),
  AARCH64_OP(scvtf),
  AARCH64_OP(ucvtf),
  AARCH64_OP(fneg),
   
  AARCH64_OP(xxx),
  AARCH64_OP(not),
  AARCH64_OP(nop),

  AARCH64_OP(nrvoval),
  
  // Argument Registers
  // int
  AARCH64_OP(r0),
  AARCH64_OP(r1),
  AARCH64_OP(r2),
  AARCH64_OP(r3),
  AARCH64_OP(r4),
  AARCH64_OP(r5),
  AARCH64_OP(r6),
  AARCH64_OP(r7),
  AARCH64_OP(r8),
  AARCH64_OP(r9),


  // Floating point args.
  AARCH64_OP(d0),
  AARCH64_OP(d1),
  AARCH64_OP(d2),
  AARCH64_OP(d3),
  AARCH64_OP(d4),
  AARCH64_OP(d5),
  AARCH64_OP(d6),
  AARCH64_OP(d7),

  AARCH64_OP(zr),
  AARCH64_OP(lr),
  AARCH64_OP(xr),

  AARCH64_OP(oplsl),    // Operand LSL

  AARCH64_OP(regarg),  // Holder for reg args.
  
  // Spill and reload.
  AARCH64_OP(spill),
  AARCH64_OP(reload),
} AARCH64Opcode;

// Flags for TargetInstruction.
#define AARCH64_HI_RELOC 0x1000
#define AARCH64_LO_RELOC 0x2000
#define AARCH64_PCREL_HI_RELOC 0x4000
#define AARCH64_PCREL_LO_RELOC 0x8000
#define AARCH64_EXPORTED_LABEL 0x10000

// Marks a call (bl/blr) whose value is returned in the floating-point return
// register (d0) rather than the integer return register (x0).  Set when the
// callee's return type is floating point so the register allocator binds the
// call result to d0.  Uses a bit above the instruction-size field (bits 16-17).
#define AARCH64_INST_FP_RETURN 0x40000

// Marks a value-carrying instruction whose result is a floating-point value but
// whose opcode does not otherwise imply a register class (e.g. a `tmp` merge
// slot used for ?: / && / ||).  The register allocator uses this to place the
// value in an FP register instead of defaulting to a general register.
#define AARCH64_INST_FP (1 << 23)
#define AARCH64_INST_EXTENDED_ASM (1 << 24)
#define AARCH64_TPREL_HI_RELOC (1 << 25)
#define AARCH64_TPREL_LO_RELOC (1 << 26)

#define AARCH64_MAX_ASM_OPERANDS 16

typedef struct {
  TargetInstruction base;
  AsmASTNode* asm_node;
  int num_operands;
  int reg_nums[AARCH64_MAX_ASM_OPERANDS];
  bool is_fp[AARCH64_MAX_ASM_OPERANDS];
  int sizes[AARCH64_MAX_ASM_OPERANDS];
  TargetInstruction* immediates[AARCH64_MAX_ASM_OPERANDS];
  int64_t immediate_values[AARCH64_MAX_ASM_OPERANDS];
} AARCH64AsmInstruction;

typedef struct {
  int reg_num;
  int base_reg_num;
  int offset;  // Negative offset from frame pointer (or zero).
  bool is_fp;
} SavedArgumentRegister;

// For large offsets that don't fit into an immediate field
// of load and store instructions we divide the offsets up into
// pages, each of which is 11 bits long.  Ww store the address of this
// page in an instruction (an add instruction) and use that as the
// base for the load and store.
//
// The offsets for load and store are 12 bit signed offsets, giving us
// a range of -2048...2047.
typedef struct {
  TargetInstruction* inst;    // Page calculation instruction.
  int page_offset;            // Offset for page.
} Offset;

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
} AARCH64ExceptionRange;

typedef struct AARCH64Generator {
  TargetGenerator base;

  int num_int_arg_regs;   // Number of args in int regs.
  int num_fp_arg_regs;    // Number of args in floating point regs.
  int num_int_reg_vars;   // Number of int regs used for variables.
  int num_fp_reg_vars;    // Number of floating point regs for vars.
  int struct_return_reg;
  int struct_return_spill_offset;
  bool not_leaf;          // Not a leaf procedure.
  
  Vector saved_regs;
  Vector offsets;         // Pointers to Offset.
  
  TargetInstruction* int_argument_registers[AARCH64_NUM_INT_ARGS];
  TargetInstruction* fp_argument_registers[AARCH64_NUM_FP_ARGS];
  Vector var_regs;
  Vector exception_ranges;
  Vector exception_typeinfos;

  TargetInstruction* zero;  // Explicit zero (register xzr).
  TargetInstruction* tmp;  // Temp reg.
  TargetInstruction* lsl;   // Left shift for register.
  TargetInstruction* struct_return_argument_register;  // AAPCS64 x8.
  
  Map conditions;       // Map of AARCH64Opcode vs TargetInstruction* for conds.
  
  // Register allocator.
  AARCH64RegisterAllocator register_allocator;
} AARCH64Generator;

void AARCH64GeneratorInit(AARCH64Generator* pcode, Generator* gen);
AARCH64Generator* NewAARCH64Generator(Generator* gen);

void AARCH64GeneratorDestruct(AARCH64Generator* pcode);
void AARCH64GeneratorDelete(AARCH64Generator* pcode);

// Lower the IR to AARCH64.
void AARCH64Lower(AARCH64Generator* pcode, Generator* gen);
void AARCH64Print(AARCH64Generator* pcode, FILE* fp);

bool AARCH64IsExpression(TargetInstruction* inst);
bool AARCH64IsFloatingPoint(TargetInstruction* inst);
bool AARCH64IsLoad(TargetInstruction* inst);
bool AARCH64IsSignedLoad(TargetInstruction* inst);
bool AARCH64IsStore(TargetInstruction* inst);
bool AARCH64IsFixedRegister(TargetInstruction* inst);
bool AARCH64IsConst(TargetInstruction* inst);
bool AARCH64IsSymbol(TargetInstruction* inst);
bool AARCH64IsIntConst(TargetInstruction* inst);
int AARCH64IntValue(TargetInstruction* inst);
bool AARCH64IsPossibleImmediate(int64_t value);
bool AARCH64IsBranch(TargetInstruction* inst);
bool AARCH64IsConditionalBranch(TargetInstruction* inst);
bool AARCH64IsReturn(TargetInstruction* inst);
bool AARCH64IsCall(TargetInstruction* inst);
bool AARCH64GeneratesOutput(TargetInstruction* inst);
bool AARCH64IsResult(TargetInstruction* inst);
bool AARCH64IsSpill(TargetInstruction* inst);
bool AARCH64IsLabel(TargetInstruction* inst);
bool AARCH64IsArgRegister(TargetInstruction* inst);
bool AARCH64IsVarRegister(TargetInstruction* inst);
bool AARCH64IsJumpTableEntry(TargetInstruction* inst);

TargetInstruction* AARCH64GetBranchTarget(TargetInstruction* inst);

const char* AARCH64OpcodeName(int op);
#endif /* aarch64_codegen_h */
