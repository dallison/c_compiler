#include "bpf_assembler.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "bpf_machine.h"
#include "elf.h"

static int CompareString(const void* a, const void* b) {
  const MapKeyValue* s1 = a;
  const MapKeyValue* s2 = b;
  return strcmp(s1->key.p, s2->key.p);
}

#define ASM assembler->base
#define ASMO (assembler->base.object)

static void BpfEmit(BPFAssembler* assembler, uint8_t code, uint8_t dst,
                    uint8_t src, int16_t off, int32_t imm) {
  uint64_t word = BpfEncodeInsn(code, dst, src, off, imm);
  AssemblerEmitByte(&ASM, ASMO.current_section, (uint8_t)word);
  AssemblerEmitByte(&ASM, ASMO.current_section, (uint8_t)(word >> 8));
  AssemblerEmitHalf(&ASM, ASMO.current_section, (uint16_t)(word >> 16));
  AssemblerEmitWord(&ASM, ASMO.current_section, (int32_t)(word >> 32));
}

static int ParseRegister(BPFAssembler* assembler) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "expected register");
    return 0;
  }
  const char* name = ASM.lex.spelling.value;
  int num = -1;
  if (strcmp(name, "fp") == 0) {
    num = BPF_REG_FP;
  } else if (name[0] == 'r') {
    num = atoi(name + 1);
  }
  LexNextToken(&ASM.lex);
  if (num < 0 || num >= BPF_NUM_REGS) {
    AssemblerError(&ASM, "illegal register %s", name);
    return 0;
  }
  return num;
}

static bool ParseMem(BPFAssembler* assembler, int* base, int16_t* off) {
  if (!LexMatch(&ASM.lex, TOK(lsquare))) {
    AssemblerError(&ASM, "expected [reg+off]");
    return false;
  }
  *base = ParseRegister(assembler);
  *off = 0;
  if (LexMatch(&ASM.lex, TOK(plus))) {
    *off = (int16_t)AssemblerEvaluateExpression(&ASM);
  } else if (LexMatch(&ASM.lex, TOK(minus))) {
    *off = (int16_t)(-AssemblerEvaluateExpression(&ASM));
  }
  if (!LexMatch(&ASM.lex, TOK(rsquare))) {
    AssemblerError(&ASM, "expected ]");
    return false;
  }
  return true;
}

static int16_t BranchOffset(BPFAssembler* assembler, int64_t target) {
  int64_t pc = AssemblerCurrentAddress(&ASM);
  int64_t delta = target - (pc + BPF_INSN_SIZE);
  if (delta % BPF_INSN_SIZE != 0) {
    AssemblerError(&ASM, "unaligned branch target");
    return 0;
  }
  int64_t insns = delta / BPF_INSN_SIZE;
  if (insns < -32768 || insns > 32767) {
    AssemblerError(&ASM, "branch out of range");
    return 0;
  }
  return (int16_t)insns;
}

typedef enum {
  kBpfFormAlu,
  kBpfFormNeg,
  kBpfFormLoad,
  kBpfFormStore,
  kBpfFormJa,
  kBpfFormJcc,
  kBpfFormCall,
  kBpfFormExit,
  kBpfFormLddw,
  kBpfFormNop,
} BpfForm;

typedef struct {
  const char* name;
  uint8_t code;
  BpfForm form;
} BpfMnemonic;

static const BpfMnemonic kMnemonics[] = {
    {"mov64", BPF_ALU64 | BPF_MOV, kBpfFormAlu},
    {"mov32", BPF_ALU | BPF_MOV, kBpfFormAlu},
    {"mov", BPF_ALU64 | BPF_MOV, kBpfFormAlu},
    {"add64", BPF_ALU64 | BPF_ADD, kBpfFormAlu},
    {"add32", BPF_ALU | BPF_ADD, kBpfFormAlu},
    {"add", BPF_ALU64 | BPF_ADD, kBpfFormAlu},
    {"sub64", BPF_ALU64 | BPF_SUB, kBpfFormAlu},
    {"sub32", BPF_ALU | BPF_SUB, kBpfFormAlu},
    {"sub", BPF_ALU64 | BPF_SUB, kBpfFormAlu},
    {"mul64", BPF_ALU64 | BPF_MUL, kBpfFormAlu},
    {"mul32", BPF_ALU | BPF_MUL, kBpfFormAlu},
    {"mul", BPF_ALU64 | BPF_MUL, kBpfFormAlu},
    {"div64", BPF_ALU64 | BPF_DIV, kBpfFormAlu},
    {"div32", BPF_ALU | BPF_DIV, kBpfFormAlu},
    {"div", BPF_ALU64 | BPF_DIV, kBpfFormAlu},
    {"mod64", BPF_ALU64 | BPF_MOD, kBpfFormAlu},
    {"mod32", BPF_ALU | BPF_MOD, kBpfFormAlu},
    {"mod", BPF_ALU64 | BPF_MOD, kBpfFormAlu},
    {"or64", BPF_ALU64 | BPF_OR, kBpfFormAlu},
    {"or32", BPF_ALU | BPF_OR, kBpfFormAlu},
    {"or", BPF_ALU64 | BPF_OR, kBpfFormAlu},
    {"and64", BPF_ALU64 | BPF_AND, kBpfFormAlu},
    {"and32", BPF_ALU | BPF_AND, kBpfFormAlu},
    {"and", BPF_ALU64 | BPF_AND, kBpfFormAlu},
    {"xor64", BPF_ALU64 | BPF_XOR, kBpfFormAlu},
    {"xor32", BPF_ALU | BPF_XOR, kBpfFormAlu},
    {"xor", BPF_ALU64 | BPF_XOR, kBpfFormAlu},
    {"lsh64", BPF_ALU64 | BPF_LSH, kBpfFormAlu},
    {"lsh32", BPF_ALU | BPF_LSH, kBpfFormAlu},
    {"lsh", BPF_ALU64 | BPF_LSH, kBpfFormAlu},
    {"rsh64", BPF_ALU64 | BPF_RSH, kBpfFormAlu},
    {"rsh32", BPF_ALU | BPF_RSH, kBpfFormAlu},
    {"rsh", BPF_ALU64 | BPF_RSH, kBpfFormAlu},
    {"arsh64", BPF_ALU64 | BPF_ARSH, kBpfFormAlu},
    {"arsh32", BPF_ALU | BPF_ARSH, kBpfFormAlu},
    {"arsh", BPF_ALU64 | BPF_ARSH, kBpfFormAlu},
    {"neg64", BPF_ALU64 | BPF_NEG, kBpfFormNeg},
    {"neg32", BPF_ALU | BPF_NEG, kBpfFormNeg},
    {"neg", BPF_ALU64 | BPF_NEG, kBpfFormNeg},
    {"li", BPF_ALU64 | BPF_MOV, kBpfFormAlu},
    {"ldxb", BPF_LDX | BPF_MEM | BPF_B, kBpfFormLoad},
    {"ldxh", BPF_LDX | BPF_MEM | BPF_H, kBpfFormLoad},
    {"ldxw", BPF_LDX | BPF_MEM | BPF_W, kBpfFormLoad},
    {"ldxdw", BPF_LDX | BPF_MEM | BPF_DW, kBpfFormLoad},
    {"stxb", BPF_STX | BPF_MEM | BPF_B, kBpfFormStore},
    {"stxh", BPF_STX | BPF_MEM | BPF_H, kBpfFormStore},
    {"stxw", BPF_STX | BPF_MEM | BPF_W, kBpfFormStore},
    {"stxdw", BPF_STX | BPF_MEM | BPF_DW, kBpfFormStore},
    {"lddw", BPF_LD | BPF_IMM | BPF_DW, kBpfFormLddw},
    {"ja", BPF_JMP | BPF_JA, kBpfFormJa},
    {"jeq", BPF_JMP | BPF_JEQ, kBpfFormJcc},
    {"jgt", BPF_JMP | BPF_JGT, kBpfFormJcc},
    {"jge", BPF_JMP | BPF_JGE, kBpfFormJcc},
    {"jlt", BPF_JMP | BPF_JLT, kBpfFormJcc},
    {"jle", BPF_JMP | BPF_JLE, kBpfFormJcc},
    {"jset", BPF_JMP | BPF_JSET, kBpfFormJcc},
    {"jne", BPF_JMP | BPF_JNE, kBpfFormJcc},
    {"jsgt", BPF_JMP | BPF_JSGT, kBpfFormJcc},
    {"jsge", BPF_JMP | BPF_JSGE, kBpfFormJcc},
    {"jslt", BPF_JMP | BPF_JSLT, kBpfFormJcc},
    {"jsle", BPF_JMP | BPF_JSLE, kBpfFormJcc},
    {"call", BPF_JMP | BPF_CALL, kBpfFormCall},
    {"exit", BPF_JMP | BPF_EXIT, kBpfFormExit},
    {"nop", BPF_ALU64 | BPF_MOV, kBpfFormNop},
};

static const BpfMnemonic* FindMnemonic(const char* name) {
  for (size_t i = 0; i < sizeof(kMnemonics) / sizeof(kMnemonics[0]); i++) {
    if (strcmp(kMnemonics[i].name, name) == 0) {
      return &kMnemonics[i];
    }
  }
  return NULL;
}

static AssemblerSymbol* NeedSymbol(BPFAssembler* assembler, const char* name) {
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, name);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(name, 0, SYM_TYPE(none), SYM_BIND(global), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  return sym;
}

static void AssembleAlu(BPFAssembler* assembler, uint8_t base_code) {
  int dst = ParseRegister(assembler);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "missing comma");
    return;
  }
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    const char* spelling = ASM.lex.spelling.value;
    if (spelling[0] == 'r' || strcmp(spelling, "fp") == 0) {
      int src = ParseRegister(assembler);
      BpfEmit(assembler, (uint8_t)(base_code | BPF_X), (uint8_t)dst,
              (uint8_t)src, 0, 0);
      return;
    }
  }
  int64_t imm = AssemblerEvaluateExpression(&ASM);
  BpfEmit(assembler, (uint8_t)(base_code | BPF_K), (uint8_t)dst, 0, 0,
          (int32_t)imm);
}

static void AssembleNeg(BPFAssembler* assembler, uint8_t code) {
  int dst = ParseRegister(assembler);
  BpfEmit(assembler, code, (uint8_t)dst, 0, 0, 0);
}

static void AssembleLoad(BPFAssembler* assembler, uint8_t code) {
  int dst = ParseRegister(assembler);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "missing comma");
    return;
  }
  int base;
  int16_t off;
  if (!ParseMem(assembler, &base, &off)) {
    return;
  }
  BpfEmit(assembler, code, (uint8_t)dst, (uint8_t)base, off, 0);
}

static void AssembleStore(BPFAssembler* assembler, uint8_t code) {
  int base;
  int16_t off;
  if (!ParseMem(assembler, &base, &off)) {
    return;
  }
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "missing comma");
    return;
  }
  int src = ParseRegister(assembler);
  BpfEmit(assembler, code, (uint8_t)base, (uint8_t)src, off, 0);
}

static void AssembleJa(BPFAssembler* assembler) {
  int64_t target = AssemblerEvaluateExpression(&ASM);
  BpfEmit(assembler, BPF_JMP | BPF_JA, 0, 0, BranchOffset(assembler, target), 0);
}

static void AssembleJcc(BPFAssembler* assembler, uint8_t base_code) {
  int dst = ParseRegister(assembler);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "missing comma");
    return;
  }
  uint8_t src = 0;
  int32_t imm = 0;
  uint8_t code = base_code;
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    const char* spelling = ASM.lex.spelling.value;
    if (spelling[0] == 'r' || strcmp(spelling, "fp") == 0) {
      src = (uint8_t)ParseRegister(assembler);
      code = (uint8_t)(base_code | BPF_X);
    } else {
      imm = (int32_t)AssemblerEvaluateExpression(&ASM);
      code = (uint8_t)(base_code | BPF_K);
    }
  } else {
    imm = (int32_t)AssemblerEvaluateExpression(&ASM);
    code = (uint8_t)(base_code | BPF_K);
  }
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "missing comma before branch target");
    return;
  }
  int64_t target = AssemblerEvaluateExpression(&ASM);
  BpfEmit(assembler, code, (uint8_t)dst, src, BranchOffset(assembler, target),
          imm);
}

static void AssembleCall(BPFAssembler* assembler) {
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String name;
    StringInit(&name, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    AssemblerSymbol* sym = NeedSymbol(assembler, name.value);
    int32_t offset = (int32_t)AssemblerCurrentAddress(&ASM);
    AssemblerAddRelocationForSymbol(&ASM, sym, R_BPF_64_32, ASMO.current_section,
                                    offset, 0);
    BpfEmit(assembler, BPF_JMP | BPF_CALL, 0, BPF_PSEUDO_CALL, 0, 0);
    StringDestruct(&name);
    return;
  }
  int32_t imm = (int32_t)AssemblerEvaluateExpression(&ASM);
  BpfEmit(assembler, BPF_JMP | BPF_CALL, 0, 0, 0, imm);
}

static void AssembleLddw(BPFAssembler* assembler) {
  int dst = ParseRegister(assembler);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "missing comma");
    return;
  }
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String name;
    StringInit(&name, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    AssemblerSymbol* sym = NeedSymbol(assembler, name.value);
    int32_t offset = (int32_t)AssemblerCurrentAddress(&ASM);
    AssemblerAddRelocationForSymbol(&ASM, sym, R_BPF_64_64, ASMO.current_section,
                                    offset, 0);
    BpfEmit(assembler, BPF_LD | BPF_IMM | BPF_DW, (uint8_t)dst, 0, 0, 0);
    BpfEmit(assembler, 0, 0, 0, 0, 0);
    StringDestruct(&name);
    return;
  }
  int64_t value = AssemblerEvaluateExpression(&ASM);
  BpfEmit(assembler, BPF_LD | BPF_IMM | BPF_DW, (uint8_t)dst, 0, 0,
          (int32_t)value);
  BpfEmit(assembler, 0, 0, 0, 0, (int32_t)(value >> 32));
}

void AssembleBPFInstruction(Assembler* base, String* word) {
  BPFAssembler* assembler = (BPFAssembler*)base;
  const BpfMnemonic* m = FindMnemonic(word->value);
  if (m == NULL) {
    AssemblerError(&ASM, "unknown eBPF instruction: %s", word->value);
    return;
  }
  switch (m->form) {
    case kBpfFormAlu:
      AssembleAlu(assembler, m->code);
      break;
    case kBpfFormNeg:
      AssembleNeg(assembler, m->code);
      break;
    case kBpfFormLoad:
      AssembleLoad(assembler, m->code);
      break;
    case kBpfFormStore:
      AssembleStore(assembler, m->code);
      break;
    case kBpfFormJa:
      AssembleJa(assembler);
      break;
    case kBpfFormJcc:
      AssembleJcc(assembler, m->code);
      break;
    case kBpfFormCall:
      AssembleCall(assembler);
      break;
    case kBpfFormExit:
      BpfEmit(assembler, BPF_JMP | BPF_EXIT, 0, 0, 0, 0);
      break;
    case kBpfFormLddw:
      AssembleLddw(assembler);
      break;
    case kBpfFormNop:
      BpfEmit(assembler, BPF_ALU64 | BPF_MOV | BPF_X, 0, 0, 0, 0);
      break;
  }
}

bool BPFAssemblerInit(BPFAssembler* assembler, String* infile, String* outfile) {
  static int reloc_types[] = {
      R_BPF_64_NODYLD32, R_BPF_64_ABS32, R_BPF_64_ABS64,
      R_BPF_64_NODYLD32, R_BPF_64_ABS32, R_BPF_64_ABS64,
      R_BPF_64_NODYLD32, R_BPF_64_ABS32, R_BPF_64_ABS64,
  };
  if (!AssemblerInit(&assembler->base, ELF_MACHINE_TYPE_BPF, 0, reloc_types,
                     infile, outfile)) {
    return false;
  }
  MapInit(&assembler->instructions, CompareString);
  AssemblerAddSection(&assembler->base, NULL, SHT(null), 0, 0);
  assembler->bss = AssemblerAddSection(&assembler->base, NewString(".bss"),
                                       SHT(nobits), SHF(alloc) | SHF(write), 8);
  return true;
}

BPFAssembler* NewBPFAssembler(String* infile, String* outfile) {
  BPFAssembler* assembler = malloc(sizeof(BPFAssembler));
  BPFAssemblerInit(assembler, infile, outfile);
  return assembler;
}

void BPFAssemblerDestruct(BPFAssembler* assembler) {
  AssemblerDestruct(&assembler->base);
  MapDestruct(&assembler->instructions);
}

void BPFAssemblerDelete(BPFAssembler* assembler) {
  BPFAssemblerDestruct(assembler);
  free(assembler);
}
