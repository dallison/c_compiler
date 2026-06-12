//
//  x86_64_codegen.h
//  c_compiler_library
//
//  Created by David Allison on 2/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef x86_64_codegen_h
#define x86_64_codegen_h

#include "codegen.h"
#include "hashtable.h"
#include "list.h"
#include "x86_64_reg_alloc.h"
#include "target_generator.h"

struct TargetBasicBlock;

#define X86_64_REG_VAR 0x80000000
#define X86_64_REG_VAR_MASK 0xc0000000
#define X86_64_IS_REG_VAR(offset) ((offset & X86_64_REG_VAR_MASK) == X86_64_REG_VAR)

#define X86_64_OP(op) kX86_64_##op

typedef enum {
  // Must match TargetOpcode through fvarreg.
  X86_64_OP(save),
  X86_64_OP(restore),

  X86_64_OP(symbol),
  X86_64_OP(literal),
  X86_64_OP(tmp),

  X86_64_OP(const8),
  X86_64_OP(const16),
  X86_64_OP(const32),
  X86_64_OP(const64),
  X86_64_OP(constf),
  X86_64_OP(constd),

  X86_64_OP(mv),
  X86_64_OP(fmv_s),
  X86_64_OP(fmv_d),

  X86_64_OP(movc),
  X86_64_OP(movfc),
  X86_64_OP(movdc),
  X86_64_OP(movxc),

  X86_64_OP(ret),

  X86_64_OP(label),

  X86_64_OP(fp),
  X86_64_OP(sp),
  X86_64_OP(tp),

  X86_64_OP(resulti),
  X86_64_OP(resultf),
  X86_64_OP(resultd),

  X86_64_OP(structreturn),

  X86_64_OP(asm),

  X86_64_OP(loc),
  X86_64_OP(named_label),
  X86_64_OP(ivarreg),
  X86_64_OP(fvarreg),

  // End of TargetOpcode enumeration.

  // Integer memory operations.
  X86_64_OP(loadb),
  X86_64_OP(loadb_z),
  X86_64_OP(loadw),
  X86_64_OP(loadw_z),
  X86_64_OP(loadl),
  X86_64_OP(loadl_z),
  X86_64_OP(loadq),

  X86_64_OP(storeb),
  X86_64_OP(storew),
  X86_64_OP(storel),
  X86_64_OP(storeq),

  // Floating point memory operations.
  X86_64_OP(loadss),
  X86_64_OP(loadsd),
  X86_64_OP(storess),
  X86_64_OP(storesd),

  // Integer arithmetic and logic.
  X86_64_OP(add),
  X86_64_OP(addl),
  X86_64_OP(sub),
  X86_64_OP(subl),
  X86_64_OP(imul),
  X86_64_OP(imull),
  X86_64_OP(idiv),
  X86_64_OP(div),
  X86_64_OP(mod),
  X86_64_OP(and),
  X86_64_OP(or),
  X86_64_OP(xor),
  X86_64_OP(not),
  X86_64_OP(neg),
  X86_64_OP(shl),
  X86_64_OP(shr),
  X86_64_OP(sar),
  X86_64_OP(shll),
  X86_64_OP(shrl),
  X86_64_OP(sarl),
  X86_64_OP(cmp),
  X86_64_OP(test),
  X86_64_OP(setl),
  X86_64_OP(setb),
  X86_64_OP(setg),

  // Branches and control flow.
  X86_64_OP(je),
  X86_64_OP(jne),
  X86_64_OP(jl),
  X86_64_OP(jge),
  X86_64_OP(jb),
  X86_64_OP(jae),
  X86_64_OP(jz),
  X86_64_OP(jnz),
  X86_64_OP(jmp),
  X86_64_OP(call),
  X86_64_OP(rcall),
  X86_64_OP(callf),
  X86_64_OP(rcallf),

  // Address formation and materialization.
  X86_64_OP(mov),
  X86_64_OP(movabs),
  X86_64_OP(lea),
  X86_64_OP(lea_rip),

  // Floating point arithmetic and conversions.
  X86_64_OP(addss),
  X86_64_OP(addsd),
  X86_64_OP(subss),
  X86_64_OP(subsd),
  X86_64_OP(mulss),
  X86_64_OP(mulsd),
  X86_64_OP(divss),
  X86_64_OP(divsd),
  X86_64_OP(sqrtss),
  X86_64_OP(sqrtsd),
  X86_64_OP(ucomiss),
  X86_64_OP(ucomisd),
  X86_64_OP(cvtsi2ss),
  X86_64_OP(cvtsi2sd),
  X86_64_OP(cvttss2si),
  X86_64_OP(cvttsd2si),
  X86_64_OP(cvtss2sd),
  X86_64_OP(cvtsd2ss),
  X86_64_OP(movss),
  X86_64_OP(movsd),
  X86_64_OP(movd),
  X86_64_OP(movq_xmm),
  X86_64_OP(fneg_ss),
  X86_64_OP(fneg_sd),

  // Pseudo ops and helpers.
  X86_64_OP(nop),
  X86_64_OP(movslq),
  X86_64_OP(sete),
  X86_64_OP(setne),

  // Integer argument registers.
  X86_64_OP(a0),
  X86_64_OP(a1),
  X86_64_OP(a2),
  X86_64_OP(a3),
  X86_64_OP(a4),
  X86_64_OP(a5),
  X86_64_OP(a6),
  X86_64_OP(a7),

  // Floating point argument registers.
  X86_64_OP(fa0),
  X86_64_OP(fa1),
  X86_64_OP(fa2),
  X86_64_OP(fa3),
  X86_64_OP(fa4),
  X86_64_OP(fa5),
  X86_64_OP(fa6),
  X86_64_OP(fa7),

  X86_64_OP(nrvoval),

  X86_64_OP(x0),
  X86_64_OP(t0),

  X86_64_OP(regarg),

  X86_64_OP(spill),
  X86_64_OP(reload),
} X86_64Opcode;

#define X86_64_HI_RELOC 0x1000
#define X86_64_LO_RELOC 0x2000
#define X86_64_PCREL_HI_RELOC 0x4000
#define X86_64_PCREL_LO_RELOC 0x8000
#define X86_64_EXPORTED_LABEL 0x10000
#define X86_64_UNSIGNED_MOD 0x20000
#define X86_64_GOTPCREL_RELOC 0x40000
// Marks a setcc instruction whose flags come from a scalar floating-point
// comparison (ucomiss/ucomisd) rather than an integer cmp.  See
// PrintCompareAndSet in the emitter.
#define X86_64_FCMP_SS 0x80000
#define X86_64_FCMP_SD 0x100000

typedef struct {
  int reg_num;
  int base_reg_num;
  int offset;
  bool is_fp;
  int value_bytes;
  // When non-zero this entry is not a single register store but a copy of
  // copy_bytes bytes from the memory pointed to by reg_num into
  // offset(base_reg_num).  Used to make a local copy of a large struct
  // argument passed by reference (the incoming pointer is in reg_num).
  int copy_bytes;
} SavedArgumentRegister;

typedef struct {
  TargetInstruction* inst;
  int page_offset;
} Offset;

typedef struct {
  TargetInstruction* inst;
  int varnum;
  bool is_fp;
} RegisterVariable;

typedef struct X86_64Generator {
  TargetGenerator base;

  int num_int_arg_regs;
  int num_fp_arg_regs;
  int num_int_reg_vars;
  int num_fp_reg_vars;
  int struct_return_reg;
  bool not_leaf;
  // Set when the function reads an incoming argument passed on the stack.  Such
  // arguments are addressed relative to the frame pointer, so the prologue must
  // establish a frame even for an otherwise leaf/empty function.
  bool has_incoming_stack_args;

  Vector saved_regs;
  // Total number of bytes allocated in the saved-argument area below the frame
  // pointer (used for saved argument registers and for local copies of large
  // struct arguments passed by reference).  These offsets are fixed relative to
  // the frame pointer and do not depend on stack_frame_size.
  int saved_arg_area_size;
  Vector offsets;

  TargetInstruction* int_argument_registers[X86_64_NUM_INT_ARGS];
  TargetInstruction* fp_argument_registers[X86_64_NUM_FP_ARGS];
  Vector var_regs;

  TargetInstruction* zero;
  TargetInstruction* tmp;

  X86_64RegisterAllocator register_allocator;
} X86_64Generator;

void X86_64GeneratorInit(X86_64Generator* pcode, Generator* gen);
X86_64Generator* NewX86_64Generator(Generator* gen);

void X86_64GeneratorDestruct(X86_64Generator* pcode);
void X86_64GeneratorDelete(X86_64Generator* pcode);

void X86_64Lower(X86_64Generator* pcode, Generator* gen);
void X86_64Print(X86_64Generator* pcode, FILE* fp);

bool X86_64IsExpression(TargetInstruction* inst);
bool X86_64IsFloatingPoint(TargetInstruction* inst);
bool X86_64IsLoad(TargetInstruction* inst);
bool X86_64IsSignedLoad(TargetInstruction* inst);
bool X86_64IsStore(TargetInstruction* inst);
bool X86_64IsFixedRegister(TargetInstruction* inst);
bool X86_64IsConst(TargetInstruction* inst);
bool X86_64IsSymbol(TargetInstruction* inst);
bool X86_64IsIntConst(TargetInstruction* inst);
int X86_64IntValue(TargetInstruction* inst);
bool X86_64IsPossibleImmediate(int64_t value);
bool X86_64IsBranch(TargetInstruction* inst);
bool X86_64IsConditionalBranch(TargetInstruction* inst);
bool X86_64IsReturn(TargetInstruction* inst);
bool X86_64IsCall(TargetInstruction* inst);
bool X86_64GeneratesOutput(TargetInstruction* inst);
bool X86_64IsResult(TargetInstruction* inst);
bool X86_64IsSpill(TargetInstruction* inst);
bool X86_64IsLabel(TargetInstruction* inst);
bool X86_64IsArgRegister(TargetInstruction* inst);
bool X86_64IsVarRegister(TargetInstruction* inst);
bool X86_64IsJumpTableEntry(TargetInstruction* inst);

TargetInstruction* X86_64GetBranchTarget(TargetInstruction* inst);

const char* X86_64OpcodeName(int op);

#endif /* x86_64_codegen_h */
