#ifndef bpf_codegen_h
#define bpf_codegen_h

#include "codegen.h"
#include "bpf_reg_alloc.h"
#include "target_generator.h"

#define BPF_OP(op) kBPF_##op

typedef enum {
  BPF_OP(save),
  BPF_OP(restore),
  BPF_OP(symbol),
  BPF_OP(literal),
  BPF_OP(tmp),
  BPF_OP(const8),
  BPF_OP(const16),
  BPF_OP(const32),
  BPF_OP(const64),
  BPF_OP(constf),
  BPF_OP(constd),
  BPF_OP(mov),
  BPF_OP(movf),
  BPF_OP(movd),
  BPF_OP(movc),
  BPF_OP(movfc),
  BPF_OP(movdc),
  BPF_OP(movxc),
  BPF_OP(ret),
  BPF_OP(label),
  BPF_OP(fp),
  BPF_OP(sp),
  BPF_OP(tp),
  BPF_OP(resulti),
  BPF_OP(resultf),
  BPF_OP(resultd),
  BPF_OP(structreturn),
  BPF_OP(asm),
  BPF_OP(loc),
  BPF_OP(named_label),
  BPF_OP(ivarreg),
  BPF_OP(fvarreg),

  BPF_OP(mov32),
  BPF_OP(add),
  BPF_OP(add32),
  BPF_OP(sub),
  BPF_OP(sub32),
  BPF_OP(mul),
  BPF_OP(mul32),
  BPF_OP(div),
  BPF_OP(div32),
  BPF_OP(mod),
  BPF_OP(mod32),
  BPF_OP(or),
  BPF_OP(or32),
  BPF_OP(and),
  BPF_OP(and32),
  BPF_OP(xor),
  BPF_OP(xor32),
  BPF_OP(lsh),
  BPF_OP(lsh32),
  BPF_OP(rsh),
  BPF_OP(rsh32),
  BPF_OP(arsh),
  BPF_OP(arsh32),
  BPF_OP(neg),
  BPF_OP(neg32),
  BPF_OP(li),
  BPF_OP(lddw),
  BPF_OP(ldxb),
  BPF_OP(ldxh),
  BPF_OP(ldxw),
  BPF_OP(ldxdw),
  BPF_OP(stxb),
  BPF_OP(stxh),
  BPF_OP(stxw),
  BPF_OP(stxdw),
  BPF_OP(ja),
  BPF_OP(jeq),
  BPF_OP(jgt),
  BPF_OP(jge),
  BPF_OP(jlt),
  BPF_OP(jle),
  BPF_OP(jset),
  BPF_OP(jne),
  BPF_OP(jsgt),
  BPF_OP(jsge),
  BPF_OP(jslt),
  BPF_OP(jsle),
  BPF_OP(call),
  BPF_OP(exit),
  BPF_OP(r0),
  BPF_OP(r1),
  BPF_OP(r2),
  BPF_OP(r3),
  BPF_OP(r4),
  BPF_OP(r5),
  BPF_OP(spill),
  BPF_OP(reload),
  BPF_OP(nop),
} BPFOpcode;

#define BPF_INST_ALU32 0x10000
#define BPF_INST_IMM 0x20000

typedef struct {
  TargetInstruction* inst;
  int varnum;
} BPFRegisterVariable;

typedef struct BPFGenerator {
  TargetGenerator base;
  int num_int_arg_regs;
  int struct_return_reg;
  bool not_leaf;
  TargetInstruction* int_argument_registers[BPF_NUM_ARG_REGS];
  Vector var_regs;
  Vector saved_args;
  TargetInstruction* r0;
  BPFRegisterAllocator register_allocator;
} BPFGenerator;

typedef struct {
  int reg_num;
  int offset;
  int size;
} BPFSavedArgument;

void BPFGeneratorInit(BPFGenerator* bpf, Generator* gen);
BPFGenerator* NewBPFGenerator(Generator* gen);
void BPFGeneratorDestruct(BPFGenerator* bpf);
void BPFGeneratorDelete(BPFGenerator* bpf);
void BPFLower(BPFGenerator* bpf, Generator* gen);
void BPFPrint(BPFGenerator* bpf, FILE* fp);

const char* BPFOpcodeName(int op);
bool BPFIsExpression(TargetInstruction* inst);
bool BPFIsFloatingPoint(TargetInstruction* inst);
bool BPFIsFixedRegister(TargetInstruction* inst);
bool BPFIsConst(TargetInstruction* inst);
bool BPFIsSymbol(TargetInstruction* inst);
bool BPFIsBranch(TargetInstruction* inst);
bool BPFIsConditionalBranch(TargetInstruction* inst);
bool BPFIsReturn(TargetInstruction* inst);
bool BPFIsCall(TargetInstruction* inst);
bool BPFIsSpill(TargetInstruction* inst);
bool BPFIsLabel(TargetInstruction* inst);
bool BPFGeneratesOutput(TargetInstruction* inst);
TargetInstruction* BPFGetBranchTarget(TargetInstruction* inst);

#endif /* bpf_codegen_h */
