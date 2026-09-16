//
//  x86_codegen.h
//  c_compiler_library
//
//  Created by David Allison on 2/20/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef x86_codegen_h
#define x86_codegen_h

#include "codegen.h"
#include "hashtable.h"
#include "list.h"
#include "x86_profile.h"
#include "x86_reg_alloc.h"
#include "target_generator.h"

struct TargetBasicBlock;

#define X86_REG_VAR 0x80000000
#define X86_REG_VAR_MASK 0xc0000000
#define X86_IS_REG_VAR(offset) ((offset & X86_REG_VAR_MASK) == X86_REG_VAR)

#define X86_OP(op) kX86_##op

typedef enum {
  // Must match TargetOpcode through fvarreg.
  X86_OP(save),
  X86_OP(restore),

  X86_OP(symbol),
  X86_OP(literal),
  X86_OP(tmp),

  X86_OP(const8),
  X86_OP(const16),
  X86_OP(const32),
  X86_OP(const64),
  X86_OP(constf),
  X86_OP(constd),

  X86_OP(mv),
  X86_OP(fmv_s),
  X86_OP(fmv_d),

  X86_OP(movc),
  X86_OP(movfc),
  X86_OP(movdc),
  X86_OP(movxc),

  X86_OP(ret),

  X86_OP(label),

  X86_OP(fp),
  X86_OP(sp),
  X86_OP(tp),

  X86_OP(resulti),
  X86_OP(resultf),
  X86_OP(resultd),

  X86_OP(structreturn),

  X86_OP(asm),

  X86_OP(loc),
  X86_OP(named_label),
  X86_OP(ivarreg),
  X86_OP(fvarreg),

  // End of TargetOpcode enumeration.
  X86_OP(resultv),

  // Integer memory operations.
  X86_OP(loadb),
  X86_OP(loadb_z),
  X86_OP(loadw),
  X86_OP(loadw_z),
  X86_OP(loadl),
  X86_OP(loadl_z),
  X86_OP(loadq),

  X86_OP(storeb),
  X86_OP(storew),
  X86_OP(storel),
  X86_OP(storeq),

  // Floating point memory operations.
  X86_OP(loadss),
  X86_OP(loadsd),
  X86_OP(loadv),
  X86_OP(storess),
  X86_OP(storesd),
  X86_OP(storev),

  // SSE2 packed integer operations.
  X86_OP(paddb),
  X86_OP(paddw),
  X86_OP(paddd),
  X86_OP(paddq),
  X86_OP(psubb),
  X86_OP(psubw),
  X86_OP(psubd),
  X86_OP(psubq),
  X86_OP(pand),
  X86_OP(por),
  X86_OP(pxor),
  X86_OP(pcmpeqb),
  X86_OP(pcmpeqw),
  X86_OP(pcmpeqd),
  X86_OP(pcmpgtb),
  X86_OP(pcmpgtw),
  X86_OP(pcmpgtd),
  X86_OP(addps),
  X86_OP(addpd),
  X86_OP(subps),
  X86_OP(subpd),
  X86_OP(mulps),
  X86_OP(mulpd),
  X86_OP(divps),
  X86_OP(divpd),

  // Integer arithmetic and logic.
  X86_OP(add),
  X86_OP(addl),
  X86_OP(adcl),
  X86_OP(sub),
  X86_OP(subl),
  X86_OP(sbbl),
  X86_OP(imul),
  X86_OP(imull),
  X86_OP(idiv),
  X86_OP(div),
  X86_OP(mod),
  X86_OP(and),
  X86_OP(or),
  X86_OP(xor),
  X86_OP(not),
  X86_OP(neg),
  X86_OP(shl),
  X86_OP(shr),
  X86_OP(sar),
  X86_OP(rol),
  X86_OP(ror),
  X86_OP(bsf),
  X86_OP(bsr),
  X86_OP(roll),
  X86_OP(rorl),
  X86_OP(bsfl),
  X86_OP(bsrl),
  X86_OP(shll),
  X86_OP(shrl),
  X86_OP(sarl),
  X86_OP(shldl),
  X86_OP(shrdl),
  X86_OP(cmp),
  X86_OP(test),
  X86_OP(setl),
  X86_OP(setb),
  X86_OP(setg),
  X86_OP(setge),
  X86_OP(setae),
  X86_OP(resulth),

  // Branches and control flow.
  X86_OP(je),
  X86_OP(jne),
  X86_OP(jl),
  X86_OP(jge),
  X86_OP(jb),
  X86_OP(jae),
  X86_OP(jz),
  X86_OP(jnz),
  X86_OP(jmp),
  X86_OP(call),
  X86_OP(rcall),
  X86_OP(callf),
  X86_OP(rcallf),

  // Address formation and materialization.
  X86_OP(mov),
  X86_OP(movabs),
  X86_OP(lea),
  X86_OP(lea_rip),

  // Floating point arithmetic and conversions.
  X86_OP(addss),
  X86_OP(addsd),
  X86_OP(subss),
  X86_OP(subsd),
  X86_OP(mulss),
  X86_OP(mulsd),
  X86_OP(divss),
  X86_OP(divsd),
  X86_OP(sqrtss),
  X86_OP(sqrtsd),
  X86_OP(ucomiss),
  X86_OP(ucomisd),
  X86_OP(cvtsi2ss),
  X86_OP(cvtsi2sd),
  X86_OP(cvttss2si),
  X86_OP(cvttsd2si),
  X86_OP(cvtss2sd),
  X86_OP(cvtsd2ss),
  X86_OP(movss),
  X86_OP(movsd),
  X86_OP(movd),
  X86_OP(movq_xmm),
  X86_OP(fneg_ss),
  X86_OP(fneg_sd),

  // Pseudo ops and helpers.
  X86_OP(nop),
  X86_OP(movslq),
  X86_OP(sete),
  X86_OP(setne),
  X86_OP(atomic_compare_exchange_bool),
  X86_OP(atomic_compare_exchange_val),
  X86_OP(atomic_compare_exchange_n),
  X86_OP(atomic_fetch_add_sub),

  // Integer argument registers.
  X86_OP(a0),
  X86_OP(a1),
  X86_OP(a2),
  X86_OP(a3),
  X86_OP(a4),
  X86_OP(a5),
  X86_OP(a6),
  X86_OP(a7),

  // Floating point argument registers.
  X86_OP(fa0),
  X86_OP(fa1),
  X86_OP(fa2),
  X86_OP(fa3),
  X86_OP(fa4),
  X86_OP(fa5),
  X86_OP(fa6),
  X86_OP(fa7),

  X86_OP(nrvoval),

  X86_OP(x0),
  X86_OP(t0),

  X86_OP(regarg),

  X86_OP(spill),
  X86_OP(reload),
} X86Opcode;

#define X86_HI_RELOC 0x1000
#define X86_LO_RELOC 0x2000
#define X86_PCREL_HI_RELOC 0x4000
#define X86_PCREL_LO_RELOC 0x8000
#define X86_VECTOR_VALUE 0x80
#define X86_EXPORTED_LABEL 0x10000
#define X86_UNSIGNED_MOD 0x20000
#define X86_GOTPCREL_RELOC 0x40000
// Marks a setcc instruction whose flags come from a scalar floating-point
// comparison (ucomiss/ucomisd) rather than an integer cmp.  See
// PrintCompareAndSet in the emitter.
#define X86_FCMP_SS 0x80000
#define X86_FCMP_SD 0x100000
#define X86_INST_EXTENDED_ASM 0x200000
// Jump-table start label or a jmp that occupies one 8-byte table slot.
#define X86_INST_TABLE_ENTRY 0x1000000
#define X86_TLS_RELOC 0x400000
#define X86_TLSGD_RELOC 0x4000000
#define X86_MFENCE 0x800000
#define X86_ATOMIC_SIZE_SHIFT 24
#define X86_ATOMIC_SIZE_MASK (3 << X86_ATOMIC_SIZE_SHIFT)

#define X86_MAX_ASM_OPERANDS 16

typedef struct {
  TargetInstruction base;
  AsmASTNode* asm_node;
  int num_operands;
  int reg_nums[X86_MAX_ASM_OPERANDS];
  bool is_fp[X86_MAX_ASM_OPERANDS];
  int sizes[X86_MAX_ASM_OPERANDS];
  int64_t immediate_values[X86_MAX_ASM_OPERANDS];
} X86AsmInstruction;

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

typedef struct {
  TargetInstruction* try_start;
  TargetInstruction* try_end;
  TargetInstruction* catch_label;
  EHTypeInfo* catch_typeinfo;
  bool is_cleanup;
} X86ExceptionRange;

typedef struct X86Generator {
  TargetGenerator base;
  const X86Profile* profile;

  int num_int_arg_regs;
  int num_fp_arg_regs;
  int num_int_reg_vars;
  int num_fp_reg_vars;
  int struct_return_reg;
  int struct_return_spill_offset;
  bool not_leaf;
  // Set when the function reads an incoming argument passed on the stack.  Such
  // arguments are addressed relative to the frame pointer, so the prologue must
  // establish a frame even for an otherwise leaf/empty function.
  bool has_incoming_stack_args;
  int named_stack_arg_end;
  int incoming_stack_arg_bytes;

  Vector saved_regs;
  // Total number of bytes allocated in the saved-argument area below the frame
  // pointer (used for saved argument registers and for local copies of large
  // struct arguments passed by reference).  These offsets are fixed relative to
  // the frame pointer and do not depend on stack_frame_size.
  int saved_arg_area_size;
  Vector offsets;
  Vector exception_ranges;
  Vector exception_typeinfos;

  TargetInstruction* int_argument_registers[X86_MAX_INT_ARGS];
  TargetInstruction* fp_argument_registers[X86_MAX_FP_ARGS];
  Vector var_regs;

  TargetInstruction* zero;
  TargetInstruction* tmp;

  X86RegisterAllocator register_allocator;
} X86Generator;

void X86GeneratorInit(X86Generator* pcode, Generator* gen);
X86Generator* NewX86Generator(Generator* gen);
X86Generator* NewX86GeneratorWithProfile(Generator* gen, const X86Profile* profile);

void X86GeneratorDestruct(X86Generator* pcode);
void X86GeneratorDelete(X86Generator* pcode);

void X86Lower(X86Generator* pcode, Generator* gen);
void X86Print(X86Generator* pcode, FILE* fp);

bool X86IsExpression(TargetInstruction* inst);
bool X86IsFloatingPoint(TargetInstruction* inst);
bool X86IsLoad(TargetInstruction* inst);
bool X86IsSignedLoad(TargetInstruction* inst);
bool X86IsStore(TargetInstruction* inst);
bool X86IsFixedRegister(TargetInstruction* inst);
bool X86IsConst(TargetInstruction* inst);
bool X86IsSymbol(TargetInstruction* inst);
bool X86IsIntConst(TargetInstruction* inst);
int64_t X86IntValue(TargetInstruction* inst);
bool X86IsPossibleImmediate(int64_t value);
bool X86IsBranch(TargetInstruction* inst);
bool X86IsConditionalBranch(TargetInstruction* inst);
bool X86IsReturn(TargetInstruction* inst);
bool X86IsCall(TargetInstruction* inst);
bool X86GeneratesOutput(TargetInstruction* inst);
bool X86IsResult(TargetInstruction* inst);
bool X86IsSpill(TargetInstruction* inst);
bool X86IsLabel(TargetInstruction* inst);
bool X86IsArgRegister(TargetInstruction* inst);
bool X86IsVarRegister(TargetInstruction* inst);
bool X86IsJumpTableEntry(TargetInstruction* inst);

TargetInstruction* X86GetBranchTarget(TargetInstruction* inst);

const char* X86OpcodeName(int op);

#endif /* x86_codegen_h */
