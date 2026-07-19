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

// Instruction size bits are bist 16:17 of flags (2 bits).  Not 0 and 1 so we
// can tell if the size is not set correctly.
#define kARMInstructionSize (1 << 16)
#define kSize32Bit 1
#define kSize64Bit 2

// ARM addressing modes. The value is stored in bit 21:18
//  of the TargetInstruction's flags member.
typedef enum {
  kAddrModeUnknown = 0,
  kAddrModeImplied = 1,
  kAddrModeRelative,
  kAddrModeInvalid,
} AddressingMode;

// This comparison was generated.  Used to in conditional branch.
#define kARMComparisonGenerated (1 << 22)

// A register-to-register move that copies a value into an outgoing call's
// argument register.  A contiguous run of these forms a parallel move; the
// register allocator post-pass re-sequences them and breaks cycles (see
// arm_reg_alloc.c) so a swap like r2<->r3 is not miscompiled.
#define kARMArgMove (1 << 23)

// Marks a bl/blr whose callee returns a floating-point value, so the register
// allocator assigns its result to the floating-point return register (d0/s0)
// rather than the integer return register (r0).
#define kARMFpReturn (1 << 24)

// Marks a generic temporary (ARM_OP(tmp)) that holds a floating-point value, so
// the register allocator gives it a floating-point register.  The tmp opcode is
// otherwise type-agnostic and defaults to an integer register, which would be
// wrong for e.g. the merge slot of a `double` conditional expression.
#define kARMFloatValue (1 << 25)

// Atomic pseudo metadata in the target-instruction flag word.
#define ARM_ATOMIC_SIZE_SHIFT 18
#define ARM_ATOMIC_SIZE_MASK (3 << ARM_ATOMIC_SIZE_SHIFT)
#define ARM_ATOMIC_WEAK (1 << 20)
#define ARM_ATOMIC_ORDER_SHIFT 26
#define ARM_ATOMIC_ORDER_MASK (7 << ARM_ATOMIC_ORDER_SHIFT)
#define ARM_ATOMIC_FAILURE_ORDER_SHIFT 29
#define ARM_ATOMIC_FAILURE_ORDER_MASK (7u << ARM_ATOMIC_FAILURE_ORDER_SHIFT)
#define kARMIndirectCallTarget (1 << 21)
#define kARMExtendedAsm (1 << 26)

#define ARM_MAX_ASM_OPERANDS 16

typedef struct {
  TargetInstruction base;
  AsmASTNode* asm_node;
  int num_operands;
  int reg_nums[ARM_MAX_ASM_OPERANDS];
  bool is_fp[ARM_MAX_ASM_OPERANDS];
  int sizes[ARM_MAX_ASM_OPERANDS];
  int64_t immediate_values[ARM_MAX_ASM_OPERANDS];
} ARMAsmInstruction;

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

  // Now follow the actual ARMv8 64 instruction directly from the
  // specifications.

  ARM_OP(tprel),  // Materialize a local-exec TLS offset.

  // Atomic operations expanded by the emitter after register allocation.
  ARM_OP(atomic_load),
  ARM_OP(atomic_store),
  ARM_OP(atomic_fetch_add),
  ARM_OP(atomic_fetch_sub),
  ARM_OP(atomic_add_fetch),
  ARM_OP(atomic_sub_fetch),
  ARM_OP(atomic_compare_exchange_bool),
  ARM_OP(atomic_compare_exchange_val),
  ARM_OP(atomic_compare_exchange_n),
  ARM_OP(atomic_fence),

  ARM_OP(adc),
  ARM_OP(add),
  ARM_OP(adcs),
  ARM_OP(adds),
  ARM_OP(adr),
  ARM_OP(adrp),
  ARM_OP(cmn),
  ARM_OP(cmp),
  ARM_OP(madd),
  ARM_OP(mneg),
  ARM_OP(msub),
  ARM_OP(mul),
  ARM_OP(neg),
  ARM_OP(ngc),
  ARM_OP(sbc),
  ARM_OP(negs),
  ARM_OP(ngcs),
  ARM_OP(sbcs),
  ARM_OP(sdiv),
  ARM_OP(smod),     // Pseudo instruction, emitter produces 2 instructions.
  ARM_OP(smaddl),
  ARM_OP(smnegl),
  ARM_OP(smsubl),
  ARM_OP(smulh),
  ARM_OP(smull),
  ARM_OP(sub),
  ARM_OP(subs),
  ARM_OP(udiv),
  ARM_OP(umod),     // Pseudo instruction.
  ARM_OP(umaddl),
  ARM_OP(umnegl),
  ARM_OP(umsubl),
  ARM_OP(umulh),
  ARM_OP(umull),
  
  ARM_OP(bfi),
  ARM_OP(bfxil),
  ARM_OP(cls),
  ARM_OP(clz),
  ARM_OP(extr),
  ARM_OP(rbit),
  ARM_OP(rev),
  ARM_OP(rev16),
  ARM_OP(rev32),
  ARM_OP(sbfiz),
  ARM_OP(ubfiz),
  ARM_OP(sbfx),
  ARM_OP(ubfx),
  ARM_OP(sbxt),
  ARM_OP(sbxtb),
  ARM_OP(sbxth),
  ARM_OP(ubxt),
  ARM_OP(ubxtb),
  ARM_OP(ubxth),
  ARM_OP(sxtb),
  ARM_OP(sxth),
  ARM_OP(sxtw),

  ARM_OP(and),
  ARM_OP(ands),
  ARM_OP(asr),
  ARM_OP(bic),
  ARM_OP(bics),
  ARM_OP(eon),
  ARM_OP(eons),
  ARM_OP(lsl),
  ARM_OP(lsr),
  ARM_OP(mov),
  ARM_OP(movk),
  ARM_OP(movn),
  ARM_OP(movz),
  ARM_OP(movw),
  ARM_OP(movt),
  ARM_OP(mvn),
  ARM_OP(orn),
  ARM_OP(orr),
  ARM_OP(orrs),
  ARM_OP(ror),
  ARM_OP(tst),
  ARM_OP(eor),

  ARM_OP(b),
  ARM_OP(bl),
  ARM_OP(blr),
  ARM_OP(br),
  ARM_OP(cbnz),
  ARM_OP(cbz),
  ARM_OP(tbnz),
  ARM_OP(tbz),
  
  // Condition codes for csel instruction.
  ARM_OP(eq),
  ARM_OP(ne),
  ARM_OP(cs),
  ARM_OP(hs),
  ARM_OP(cc),
  ARM_OP(lo),
  ARM_OP(mi),
  ARM_OP(pl),
  ARM_OP(vs),
  ARM_OP(vc),
  ARM_OP(hi),
  ARM_OP(ls),
  ARM_OP(ge),
  ARM_OP(lt),
  ARM_OP(gt),
  ARM_OP(le),
  ARM_OP(al),
 

  ARM_OP(ccmn),
  ARM_OP(ccmp),
  ARM_OP(cinc),
  ARM_OP(cinv),
  ARM_OP(cneg),
  ARM_OP(csel),
  ARM_OP(cset),
  ARM_OP(csetm),
  ARM_OP(csinc),
  ARM_OP(csinv),
  ARM_OP(csneg),
  
  ARM_OP(ldp),
  ARM_OP(ldpsw),
  ARM_OP(ldr),
  ARM_OP(ldur),
  ARM_OP(ldrb),
  ARM_OP(ldrh),
  ARM_OP(ldurb),
  ARM_OP(ldurh),
  ARM_OP(ldrsb),
  ARM_OP(ldrsh),
  ARM_OP(ldursb),
  ARM_OP(ldursh),
  ARM_OP(ldursw),
  ARM_OP(prfm),
  ARM_OP(stp),
  ARM_OP(str),
  ARM_OP(stur),
  ARM_OP(strb),
  ARM_OP(strh),
  ARM_OP(sturb),
  ARM_OP(sturh),

  ARM_OP(fldr),
  ARM_OP(fstr),
  ARM_OP(fadd),
  ARM_OP(fsub),
  ARM_OP(fmul),
  ARM_OP(fdiv),
  ARM_OP(fsqrt),
  ARM_OP(fmin),
  ARM_OP(fmax),
  ARM_OP(fcvtsd),     // Single to double
  ARM_OP(fcvtds),     // Double to single.
  ARM_OP(fcvtns),
  ARM_OP(fcvtnu),
  ARM_OP(fcvt),     // Copy from int reg (no conversion)
  ARM_OP(fmov),
  ARM_OP(fcmp),
  ARM_OP(scvtf),
  ARM_OP(ucvtf),
  ARM_OP(fneg),
   
  ARM_OP(xxx),
  ARM_OP(not),
  ARM_OP(nop),

  ARM_OP(nrvoval),
  
  // Argument Registers
  // int
  ARM_OP(r0),
  ARM_OP(r1),
  ARM_OP(r2),
  ARM_OP(r3),
  ARM_OP(r4),
  ARM_OP(r5),
  ARM_OP(r6),
  ARM_OP(r7),
  ARM_OP(r8),
  ARM_OP(r9),


  // Floating point args.
  ARM_OP(d0),
  ARM_OP(d1),
  ARM_OP(d2),
  ARM_OP(d3),
  ARM_OP(d4),
  ARM_OP(d5),
  ARM_OP(d6),
  ARM_OP(d7),

  ARM_OP(zr),
  ARM_OP(lr),
  ARM_OP(xr),

  ARM_OP(oplsl),    // Operand LSL

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
  bool is_fp;
  bool is_double;  // 64-bit floating point argument (saved as a d-register).
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
} ARMExceptionRange;

typedef struct ARMGenerator {
  TargetGenerator base;

  int num_int_arg_regs;   // Number of args in int regs.
  int num_fp_arg_regs;    // Number of args in floating point regs.
  int num_int_reg_vars;   // Number of int regs used for variables.
  int num_fp_reg_vars;    // Number of floating point regs for vars.
  int struct_return_reg;
  bool not_leaf;          // Not a leaf procedure.
  bool has_stack_args;    // Has at least one incoming argument passed on the
                          // stack; such args are addressed via fp, so the
                          // function needs a frame pointer.
  bool uses_dynamic_stack;  // Uses VLA/alloca (sp adjusted dynamically); the
                            // epilogue must restore sp from fp.
  int saved_arg_size;     // Bytes used by register-passed argument homes.
  
  Vector saved_regs;
  Vector offsets;         // Pointers to Offset.
  
  TargetInstruction* int_argument_registers[ARM_NUM_INT_ARGS];
  TargetInstruction* fp_argument_registers[ARM_NUM_FP_ARGS];
  Vector var_regs;
  Vector exception_ranges;
  Vector exception_typeinfos;

  TargetInstruction* zero;  // Explicit zero register pseudo-op.
  TargetInstruction* tmp;  // Temp reg.
  TargetInstruction* lsl;   // Left shift for register.
  
  Map conditions;       // Map of ARMOpcode vs TargetInstruction* for conds.
  
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

int ARMGetRegisterSize(TargetInstruction* inst);

const char* ARMOpcodeName(int op);
#endif /* arm_codegen_h */
